#include "extractor.h"

#include <map>
#include <fstream>
#include <vector>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include "porter2_stemmer.h"
#include "nl_mrl_pair.h"
#include "read_write.h"
#include "linearizer.h"

namespace semparse {

int remove_end_punctuation(std::string& nl){
  if(boost::ends_with(nl, " .") || boost::ends_with(nl, " ?")
     || boost::ends_with(nl, " !")){
    nl = nl.substr(0, nl.length()-2);
  }
  if(boost::ends_with(nl, ".") || boost::ends_with(nl, "?")
    || boost::ends_with(nl, "!")){
    nl = nl.substr(0, nl.length()-1);
  }
  return 0;
}

bool is_integer(const std::string & s){
 if(s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false ;

 char * p ;
 strtol(s.c_str(), &p, 10) ;

 return (*p == 0);
}

int normalize(std::string& nl){
  boost::replace_all(nl, "\r", "");
  boost::replace_all(nl, "„", "\"");
  boost::replace_all(nl, "“", "\"");
  boost::replace_all(nl, "”", "\"");
  boost::replace_all(nl, "´", "'");
  nl = boost::regex_replace(nl, boost::regex("([a-z])‘([a-z])"),"$1\'$2");
  nl = boost::regex_replace(nl, boost::regex("([a-z])’([a-z])"),"$1\'$2");
  boost::replace_all(nl, "‘", "\"");
  boost::replace_all(nl, "‚", "\"");
  boost::replace_all(nl, "''", "\"");
  boost::replace_all(nl, "´´", "\"");
  boost::replace_all(nl, "…", "...");
  boost::replace_all(nl, " « ", "\"");
  boost::replace_all(nl, "« ", "\"");
  boost::replace_all(nl, "«", "\"");
  boost::replace_all(nl, " » ", "\"");
  boost::replace_all(nl, " »", "\"");
  boost::replace_all(nl, "»", "\"");
  boost::replace_all(nl, " %", "%");
  boost::replace_all(nl, "nº ", "nº ");
  boost::replace_all(nl, " :", ":");
  boost::replace_all(nl, " ºC", " ºC");
  boost::replace_all(nl, " cm", " cm");
  boost::replace_all(nl, " ?", "?");
  boost::replace_all(nl, " !", "!");
  boost::replace_all(nl, " ;", ";");
  nl = boost::regex_replace(nl, boost::regex("[/:&_\\.,;!\?\\(\\)\\[\\]{}\"=<>'´`]"),"");
  boost::replace_all(nl, "-", "");
  boost::replace_all(nl, "|||", "PIPEPROTECT");
  nl = boost::regex_replace(nl, boost::regex(" +")," ");
  return 0;
}

int stem_string(std::string& to_stem){
  std::vector<std::string> words_to_stem;
  std::ostringstream ss;
  boost::split(words_to_stem, to_stem, boost::is_any_of(" "));
  for(auto word : words_to_stem) {
    if(is_integer(word)){
       //ensure  correct pass through of numbers
      ss << "<topx>" << word << "</topx> ";
      continue;
    }
    Porter2Stemmer::stem(word);
    ss << word << " ";
  }
  to_stem = ss.str();
  return 1;
}

int preprocess_nl(std::string& nl, NLMRLPair& nl_mrl_pair,
                  const bool& stem){
  boost::trim(nl);
  boost::to_lower(nl);
  remove_end_punctuation(nl);

  nl_mrl_pair.non_stemmed = nl;
  nl_mrl_pair.preprocessed_sentence = nl;

  normalize(nl_mrl_pair.preprocessed_sentence);

  //stem
  if(stem){
    stem_string(nl_mrl_pair.preprocessed_sentence);
  }

  boost::trim(nl_mrl_pair.preprocessed_sentence);

  return 0;
}

int extract_set(const std::string& experiment_dir,
                 const std::string& files_type,
                 const std::string& set_files,
                 const std::string& language,
                 const bool& stem){
  std::vector<std::string> nl_in;
  std::vector<std::string> mrl_in;

  std::vector<std::string> nl_out;
  std::vector<std::string> no_stem_out;
  std::vector<std::string> mrl_out;
  std::vector<std::string> mrl_lm_out;

  read_file(set_files+"."+language, nl_in);
  read_file(set_files+".mrl", mrl_in);

  NLMRLPair nl_mrl_pair;
  for(auto line : nl_in){
    nl_mrl_pair = {"", "", "", ""};
    preprocess_nl(line, nl_mrl_pair, stem);
    nl_out.push_back(nl_mrl_pair.preprocessed_sentence);
    if(files_type == "test"){
      no_stem_out.push_back(nl_mrl_pair.non_stemmed);
    }
  }

  Linearizer linearizer;
  for(auto line : mrl_in){
    linearizer.linearize_mrl(line);
    mrl_out.push_back(line);
    if(files_type == "train"){
      mrl_lm_out.push_back("<s> "+line+" </s>");
    }
  }

  if(files_type == "train"){
    write_file(experiment_dir+"/train.nl", nl_out);
    write_file(experiment_dir+"/train.mrl", mrl_out);
    write_file(experiment_dir+"/train.mrl.lm", mrl_lm_out);
  } else if(files_type == "tune"){
    write_file(experiment_dir+"/tune.nl", nl_out);
    write_file(experiment_dir+"/tune.mrl", mrl_out);
  } else if(files_type == "test"){
    write_file(experiment_dir+"/test.nl", nl_out);
    write_file(experiment_dir+"/test.nostem.nl", no_stem_out);
    write_file(experiment_dir+"/test.mrl", mrl_out);
  }
  return 0;
}

} // namespace semparse
