`cdec-semparse` offers scripts specific to using `cdec` to perform semantic parsing. It has been created with the [NLmaps corpus](http://www.cl.uni-heidelberg.de/statnlpgroup/nlmaps/) in mind. Other semantic parsing corpora can be used as well, as long as the MRL formulae can be linearised.

##Dependencies
Additionally to the dependencies required to build cdec, the training script for the semantic parsing models requires the following:

- [SRILM](http://www.speech.sri.com/projects/srilm/): The semantic parsing script builds language models using SRILM. (KenLM only implements Kneser-Ney smoothing which leads to errors using the NLmaps corpus, also see [this discussion](http://comments.gmane.org/gmane.comp.nlp.moses.user/9452))
- [Overpass-nlmaps](https://github.com/carhaas/overpass-nlmaps) + [database](http://wiki.openstreetmap.org/wiki/Overpass_API/Installation): for an evaluation against answers rather than MRL formulae, it is needed to install Overpass-nlmaps which converts a MRL formula to an executable database query. This is then executed against the OSM database which needs to be setup beforehand.
- (optional) [Moses](http://www.statmt.org/moses/) + [Giza++](http://www.statmt.org/moses/giza/GIZA++.html): optionally Moses and Giza++ can be used. This seems to produce more precise alignments than cdec's fast_align on this data set (needed to reproduce the results from the NAACL16 paper)

##Acknowledgements
<b>[smt-semparse](https://github.com/jacobandreas/smt-semparse):</b> J. Andreas, A. Vlachos, S. Clark. In *Proceedings of ACL*, August, 2013. [pdf](http://people.eecs.berkeley.edu/~jda/papers/avc_smt_semparse.pdf)

Large parts of this work were directly inspired by Jacob Andreas' [smt-semparse](https://github.com/jacobandreas/smt-semparse). Particularly the way in which MRL formulae are linearized and functionalized. Thank you for allowing me to use those ideas in this project!

<b>[Porter Stemmer](https://bitbucket.org/smassung/porter2_stemmer/wiki/Home):</b> A copy of the C++ porter stemmer, written by Sean Massung, is included in this project.

##Usage
The entry point is the script `run_semparse.sh`
First it is necessary to set the correct path to the needed dependecies (cdec, srilm, overpass-nlmaps, database, moses (optional), giza++(optional))

To train:
`./run_semparse.sh train`

To test (specify the `DIR` of the trained model first):
`./run_semparse.sh test`

To both train and test:
`./run_semparse.sh both`

Further you can choose to specify the following options:
For training:
`TIGHT=0`: the grammar extractor also extracts loose phrases. While it does slow the process down, it is crucial for semantic parsing

`STEM=1`: this will stem the input natural language sentences, change to 0 to switch it off

`SPARSE=1`: Switching the sparse variable to 1 will use next to the dense features also sparse ones (see [Simianer et al., 2012](https://simianer.de/P12-1002.pdf)). Due to the higher number of features, the script also switches from MERT to MIRA for the tuning step.

`NR_MERT_RUNS=3`: Only relevant if `SPARSE=0`. Due to random initialization, MERT runs are not deterministic and can vary by quite a bit, depening on the local minimum found. It is thus recommended for experiments to run MERT 3 times and report the average. (Note: this is not a problem when MIRA is used, as long as MIRA is run on 1 core only)

`PARALLEL_MERT_JOBS=4`: Running MERT in parallel greatly speeds up the tuning step.

For testing:
`KBEST=100`: Set the number of kbest entries to be traveresed on the search for a valid tree

`CFG=/path/to/cfg`: If a CFG file is specified, the parser will also ensure that a MRL satisfies the given CFG

`PASS=1`: If set to 1, this option will allow unknown words to be passed through. This can be helpful for named entities that where not seen during training. If the model uses stemming, this step will also ensure that the original word is found.

##Citation
If you use this semantic parser please cite:

Carolin Haas, Stefan Riezler. A Corpus and Semantic Parser for Natural Language Querying of OpenStreetMap.  In *Proceedings of NAACL*, June, 2016. [[bibtex](http://www.cl.uni-heidelberg.de/~haas1/publications/bib/HaasRiezler2016.txt)] [[pdf](http://www.cl.uni-heidelberg.de/~haas1/publications/papers/HaasRiezler2016.pdf)]
