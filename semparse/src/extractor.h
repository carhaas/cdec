#ifndef _SEMPARSE_EXTRACTOR_H_
#define	_SEMPARSE_EXTRACTOR_H_

#include <string>

/*
  Contains various functions that are used to prepare MRL formulae or
  natural language text to be used in the SMT pipeline
*/
namespace semparse {
  struct NLMRLPair;
  int remove_end_punctuation(std::string& nl);
  bool is_integer(const std::string & s);
  int normalize(std::string& nl);
  int stem_string(std::string& to_stem);
  int preprocess_nl(std::string& nl, NLMRLPair& nl_mrl_pair,
                    const bool& stem);
  int extract_set(const std::string& experiment_dir,
                   const std::string& files_type,
                   const std::string& set_files,
                   const std::string& language,
                   const bool& stem);
}

#endif
