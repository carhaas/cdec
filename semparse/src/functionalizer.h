#ifndef _SEMPARSE_FUNCTIONALIZER_H_
#define	_SEMPARSE_FUNCTIONALIZER_H_

#include <string>
#include <vector>

namespace semparse {
/*
  Convert a linearized MRL into a functionalized MRL
  i.e.
  query@2 nwr@1 keyval@2 Schalansky_ref@0 *@s qtype@1 count@0
  is converted to:
  query(nwr(keyval('Schalansky_ref','*')),qtype(count))
*/
struct NLMRLPair;
class Functionalizer {
public:
  virtual ~Functionalizer();
  int functionalize(std::string& candidate, NLMRLPair& nl_mrl_pair, const bool& pass, const std::string& cfg);
private:
  int insert_pass_through_words(std::vector<std::string>& words, const NLMRLPair& nl_mrl_pair);
  std::string validate_if_tree(const std::vector<std::string>& words);
  int split_number_into_digits(std::string& digits_to_split_up);
  bool check_MRL_tree(std::string check_mrl, const std::string& cfg);
};

} // namespace semparse

#endif
