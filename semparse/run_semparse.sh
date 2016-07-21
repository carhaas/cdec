#!/bin/bash
#usage: ./run_semparse.sh train 
#or ./run_semparse.sh test
# or ./run_semparse.sh both

# Define a timestamp function
timestamp() {
  date +"%Y-%m-%dT%H.%M.%S"
}

#shared variables
CDEC=/workspace/osm/cdec-semparse
#if giza or moses are set to "", fast align is used instead
MOSES=/toolbox/moses/
GIZA=/toolbox/giza-pp/bin/
SRILM_NGRAM_COUNT=/toolbox/srilm_1_7_0/lm/bin/i686-m64/ngram-count
OVERPASS_NLMAPS=/workspace/osm/overpass/osm3s_v0.7.51/
DB_DIR=/workspace/osm/overpass/db/
FILE_LANG=en

#train variable
TRAIN=nlmaps.train #assumes that the following files exist: nlmaps.train.mrl, nlmaps.train.FILE_LANG
TUNE=nlmaps.train
SYMM=intersect
TIGHT=0
STEM=1
SPARSE=0
NR_MERT_RUNS=1
PARALLEL_MERT_JOBS=16 #modify this value to use more cores for mert training

#test variables
DIR=/workspace/osm/cdec-semparse/semparse/work/2016-07-12T10.32.47/
TEST=nlmaps.test #assumes that the following files exist: nlmaps.test.mrl, nlmaps.test.FILE_LANG and nlmaps.test.gold
KBEST=100
CFG=$CDEC/semparse/data/nlmaps/nlmaps.cfg #leave blank to use no cfg
PASS=1

if [ $# -eq 0 ]; then
	>&2 echo "Please specify either train, test or both"
	exit 1
fi

if ! ( [ $1 == "train" ] || [ $1 == "test" ] || [ $1 == "both" ] ); then
	>&2 echo "Please specify either train, test or both"
	exit 1
fi


#if train
if [ $1 == "train" ] || [ $1 == "both" ]; then
	if [ ! -d "$CDEC/semparse/work" ]; then
		mkdir $CDEC/semparse/work
	fi
	echo "Starting train run"
	CWD=$(pwd)
	NAME=$(timestamp)
	DIR="$CWD/work/$NAME"

	mkdir $DIR
	
	#write log file
	echo "FILE_LANG=$FILE_LANG" >$DIR/model_settings.train
	echo "TRAIN=$TRAIN" >>$DIR/model_settings.train
	echo "TUNE=$TUNE" >>$DIR/model_settings.train
	echo "TIGHT=$TIGHT" >>$DIR/model_settings.train
	echo "STEM=$STEM" >>$DIR/model_settings.train
	echo "SPARSE=$SPARSE" >>$DIR/model_settings.train
	echo "NR_MERT_RUNS=$NR_MERT_RUNS" >>$DIR/model_settings.train
	echo "PARALLEL_MERT_JOBS=$PARALLEL_MERT_JOBS" >>$DIR/model_settings.train
	echo "TIGHT=$TIGHT" >>$DIR/model_settings.train

	# prepare data
	echo "Preparing training and tuning data"
	$CDEC/semparse/bin/extract_data -d $DIR -f $CDEC/semparse/data/nlmaps/$TRAIN -t train -l $FILE_LANG -s $STEM
	$CDEC/semparse/bin/extract_data -d $DIR -f $CDEC/semparse/data/nlmaps/$TUNE -t tune -l $FILE_LANG -s $STEM
	
	if [[ -z $MOSES ]] || [[ -z $GIZA ]]; then
		echo "Using cdec's fast_align"
		# use cdec's fast_align
		mkdir $DIR/model
		$CDEC/corpus/paste-files.pl $DIR/train.nl $DIR/train.mrl >$DIR/train.both
		$CDEC/word-aligner/fast_align -i $DIR/train.both -d -v -o >$DIR/train.fdw_align 2>$DIR/run.log
		$CDEC/word-aligner/fast_align -i $DIR/train.both -d -v -o -r >$DIR/train.rev_align 2>$DIR/run.log
		$CDEC/utils/atools -i $DIR/train.fdw_align -j $DIR/train.rev_align -c intersect >$DIR/model/aligned.intersect
	else
		echo "Using Giza and Moses"
		# get alignment with giza via moses
		$MOSES/scripts/training/train-model.perl \
			--root-dir $DIR \
			--corpus $DIR/train \
			--f nl \
			--e mrl \
			--first-step 1 \
			--last-step 3 \
			--parallel \
			-hierarchical \
			-glue-grammar \
			--alignment $SYMM \
			--external-bin-dir $GIZA
	fi

	# create lm
	echo "Creating language model"
	/toolbox/srilm_1_7_0/lm/bin/i686-m64/ngram-count -text $DIR/train.mrl.lm -order 5 -no-sos -no-eos -lm $DIR/mrl.arpa -unk
	# get grammar

	# compile
	echo "Compiling grammar"
	$CDEC/extractor/sacompile \
		-a $DIR/model/aligned.$SYMM  \
		-f $DIR/train.nl  \
		-e $DIR/train.mrl \
		-c $DIR/extract.ini \
		-o $DIR/train.sa

	# extract tune set
	echo "Rxtracting grammar for tune set"
	$CDEC/extractor/extract \
		-c $DIR/extract.ini \
		-g $DIR/grammar_tune \
		--tight_phrases $TIGHT \
		--gzip <$DIR/tune.nl >$DIR/tune.inline.nl 2>$DIR/run.log

	#tune
	if [ $SPARSE == 1 ]; then
		#mira (only one run is alright because mira always converges to the same results [as long as only 1 core is used])
		echo "Tune using dense+sparse features and mira"
		cp $CDEC/semparse/data/ini_files/cdec_mira.ini $DIR
		sed -i "s|mrl.arpa|${DIR}/mrl.arpa|g" $DIR/cdec_mira.ini
		mkdir $DIR/mira
		cd $DIR/mira

		$CDEC/training/mira/kbest_mira \
			-w $CDEC/semparse/data/ini_files/weights.start \
			-c $DIR/cdec_mira.ini \
			-i $DIR/tune.inline.nl \
			-r $DIR/tune.mrl
		cp $DIR/mira/weights.mira-final.gz $DIR/weights.mira
	else
		#mert (recommended: 3 runs due to different results from random initialization)
		echo "Tune using dense features and mert"
		cp $CDEC/semparse/data/ini_files/cdec_mert.ini $DIR
		sed -i "s|mrl.arpa|${DIR}/mrl.arpa|g" $DIR/cdec_mert.ini
		$CDEC/corpus/paste-files.pl $DIR/tune.inline.nl $DIR/tune.mrl >$DIR/tune.mert
		COUNTER=1
		while [ $COUNTER -le $NR_MERT_RUNS ] 
		do
			echo "Mert run ${COUNTER} of ${NR_MERT_RUNS}"
			mkdir $DIR/mert$COUNTER
			cd $DIR/mert$COUNTER

			$CDEC/training/dpmert/dpmert.pl \
				-w $CDEC/semparse/data/ini_files/weights.start \
				-c $DIR/cdec_mert.ini \
				-d $DIR/tune.mert \
				--jobs $PARALLEL_MERT_JOBS
			cp $DIR/mert$COUNTER/dpmert/weights.final $DIR/weights.mert$COUNTER
			((COUNTER++))
		done
	fi
fi	

if [ $1 == "test" ] || [ $1 == "both" ]; then
	echo "Starting test run"	
	
	#write log file
	echo "FILE_LANG=$FILE_LANG" >$DIR/model_settings.test
	echo "DIR=$DIR" >>$DIR/model_settings.test
	echo "TEST=$TEST" >>$DIR/model_settings.test
	echo "KBEST=$KBEST" >>$DIR/model_settings.test
	echo "CFG=$CFG" >>$DIR/model_settings.test
	echo "PASS=$PASS" >>$DIR/model_settings.test
	
	# prepare data
	echo "Preparing test data"
	$CDEC/semparse/bin/extract_data -d $DIR -f $CDEC/semparse/data/nlmaps/$TEST -t test -l $FILE_LANG -s $STEM

	# extract test set
	echo "Extracting grammar for test set"
	$CDEC/extractor/extract \
		-c $DIR/extract.ini \
		-g $DIR/grammar_test \
		--tight_phrases $TIGHT \
		--gzip <$DIR/test.nl >$DIR/test.inline.nl 2>$DIR/run.log
		
	if [ $SPARSE == 1 ]; then
		#mira
		echo "Test using the mira weights"
		cp $CDEC/semparse/data/ini_files/cdec_test_mira.ini $DIR
		sed -i "s|mrl.arpa|${DIR}/mrl.arpa|g" $DIR/cdec_test_mira.ini
		sed -i "s|^k_best=|k_best=${KBEST}|g" $DIR/cdec_test_mira.ini
		WEIGHTS=$DIR/weights.mira
		if [[ -z $CFG ]] ; then
			$CDEC/semparse/bin/decode_test -d $DIR -m $DIR/hyp.mira.mrls -p $PASS -w $WEIGHTS -c $DIR/cdec_test_mira.ini
		else
			$CDEC/semparse/bin/decode_test -d $DIR -m $DIR/hyp.mira.mrls -p $PASS -g $CFG -w $WEIGHTS -c $DIR/cdec_test_mira.ini
		fi
		$OVERPASS_NLMAPS/query_db -d $DB_DIR -a $DIR/hyp.mira.answers -f $DIR/hyp.mira.mrls
		$CDEC/semparse/bin/eval -h $DIR/hyp.mira.answers -g $CDEC/semparse/data/nlmaps/$TEST.gold -e $DIR/hyp.mira.eval -s $DIR/hyp.mira.sigf
	else
		#mert
		echo "Test using mert weights"
		cp $CDEC/semparse/data/ini_files/cdec_test_mert.ini $DIR
		sed -i "s|mrl.arpa|${DIR}/mrl.arpa|g" $DIR/cdec_test_mert.ini
		sed -i "s|^k_best=|k_best=${KBEST}|g" $DIR/cdec_test_mert.ini
		COUNTER=1
		while [ $COUNTER -le $NR_MERT_RUNS ] 
		do
			echo "Mert run ${COUNTER} of ${NR_MERT_RUNS}"
			WEIGHTS=$DIR/weights.mert$COUNTER
			if [[ -z $CFG ]] ; then
				$CDEC/semparse/bin/decode_test -d $DIR -m $DIR/hyp.mert$COUNTER.mrls -p $PASS -w $WEIGHTS -c $DIR/cdec_test_mert.ini
			else
				$CDEC/semparse/bin/decode_test -d $DIR -m $DIR/hyp.mert$COUNTER.mrls -p $PASS -g $CFG -w $WEIGHTS -c $DIR/cdec_test_mert.ini
			fi
			$OVERPASS_NLMAPS/query_db -d $DB_DIR -a $DIR/hyp.mert$COUNTER.answers -f $DIR/hyp.mert$COUNTER.mrls
			$CDEC/semparse/bin/eval -h $DIR/hyp.mert$COUNTER.answers -g $CDEC/semparse/data/nlmaps/$TEST.gold -e $DIR/hyp.mert$COUNTER.eval -s $DIR/hyp.mert$COUNTER.sigf
			((COUNTER++))
		done
	fi	
fi

