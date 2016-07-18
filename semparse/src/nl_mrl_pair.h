#ifndef _SEMPARSE_STRUCT_H_
#define	_SEMPARSE_STRUCT_H_

#include <string>

/*
  Defines a structure for a NL-MRL pair, additionally holding any needed
  intermediate steps
*/
namespace semparse {

struct NLMRLPair{
  std::string linearised_mrl; // linearised
  std::string mrl;
  std::string preprocessed_sentence; //may or may not be stemmed, depending on settings
  std::string non_stemmed; //definitely not stemmed
};

} // namespace semparse

#endif
