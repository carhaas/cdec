#ifndef _SEMPARSE_LINEARIZER_H_
#define	_SEMPARSE_LINEARIZER_H_

#include <string>

namespace semparse {
/*
  Contains various functions that are used to prepare MRL formulae or
  natural language text to be used in the SMT pipeline
*/
class Linearizer {
public:
  virtual ~Linearizer();
  int linearize_mrl(std::string& to_linearize);
private:
  int count_arguments(const std::string& s);
  std::string delete_first_n_occurences(const std::string& token,
                                       const std::string& mrl, int n);
  int preprocess_mrl(std::string& mrl);
};

} // namespace semparse

#endif
