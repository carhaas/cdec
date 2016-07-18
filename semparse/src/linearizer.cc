#include "linearizer.h"

#include <assert.h>
#include <vector>
#include <string>

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

namespace semparse {

Linearizer::~Linearizer() {}

int Linearizer::count_arguments(const std::string& s){
  bool arguments_found = false;
  int num_parenthesis = 0;
  int num_commas = 0 ;
  size_t i = 0;
  while(i < s.length() && ((!(arguments_found) && num_parenthesis == 0)
        || (arguments_found && num_parenthesis > 0))){
    const char& c = s.at(i);
    if(c == '('){
      arguments_found = true;
      num_parenthesis += 1;
    } else if(c == ')'){
      num_parenthesis -= 1;
    } else if(num_parenthesis == 1 && c == ','){
      num_commas += 1;
    } else if(num_parenthesis < 1 && c == ','){
      break;
    }
    i += 1;
  }
  if(arguments_found){
    return num_commas + 1; //equals number of arguments
  }
  assert(num_commas==0);
  return 0;
}

std::string Linearizer::delete_first_n_occurences(const std::string& token,
                                                  const std::string& mrl, int n){
  std::string shortened_string = mrl;
  while(n > 0){
    try{
      boost::smatch sm;
      std::stringstream ss_pattern;
      ss_pattern << "\\b"
                 << boost::regex_replace(token, boost::regex(
                   "[.^$|()\\[\\]{}*+?\\\\]"), "\\\\$&",
                   boost::match_default | boost::format_perl)
                 << "\\b";
      regex_search(shortened_string, sm, boost::regex(ss_pattern.str()));
      shortened_string = shortened_string.substr(sm.position() + sm.length());
      n = n - 1;
    } catch(std::exception& e){
      return "";
    }
  }
  return shortened_string;
}

int Linearizer::preprocess_mrl(std::string& mrl){
  boost::trim(mrl);
  //sequence of characters that does not contain ( or ) : [^\\(\\)]
  mrl = boost::regex_replace(mrl, boost::regex(",' *([^\\(\\)]*?)\\((.*?) *'\\)"),",'$1BRACKETOPEN$2')"); //need to protect brackets that occur in values, assumes that there is at most one open ( and 1 close)
  mrl = boost::regex_replace(mrl, boost::regex(",' *([^\\(\\)]*?)\\)([^\\(\\)]*?) *'\\)"),",'$1BRACKETCLOSE$2')");
  boost::replace_all(mrl, " ", "€");
  mrl = boost::regex_replace(mrl, boost::regex("(?<=([^,\\(\\)]))'(?=([^,\\(\\)]))"), "SAVEAPO");
  mrl = boost::regex_replace(mrl, boost::regex("and\\(' *([^\\(\\)]+?) *',' *([^\\(\\)]+?) *'\\)"), "and($1@s','$2@s)"); //for when a and() surrounds two end values
  mrl = boost::regex_replace(mrl, boost::regex("\\(' *([^\\(\\)]+?) *'\\)"), "($1@s)"); //a bracket ( or ) is not allowed withing any key or value
  mrl = boost::regex_replace(mrl, boost::regex("([,\\)\\(])or\\(([^\\(\\)]+?)','([^\\(\\)]+?)@s\\)"), "$1or($2@s','$3@s)"); //for when a or() surrounds two values
  mrl = boost::regex_replace(mrl, boost::regex("\\s+"), " ");
  mrl = boost::regex_replace(mrl, boost::regex("'"), "");
  return 0;
}

int Linearizer::linearize_mrl(std::string& to_linearize){
  std::ostringstream olinearized_mrl;
  std::map<std::string, int> seen_string_x_times;
  std::vector<std::string> mrl_elements;

  preprocess_mrl(to_linearize);

  std::string just_words = to_linearize;
  boost::replace_all(just_words, "(", " ");
  boost::replace_all(just_words, ")", " ");
  boost::replace_all(just_words, ",", " ");
  just_words = boost::regex_replace(just_words, boost::regex("\\s+"), " ");
  boost::trim(just_words);
  boost::split(mrl_elements, just_words, boost::is_any_of(" "));

  for(auto element: mrl_elements) {
   if (seen_string_x_times.find(element) == seen_string_x_times.end()){
     seen_string_x_times[element] = 1;
   } else {
     seen_string_x_times[element]++;
   }
   if(boost::ends_with(element, "@s")){
     olinearized_mrl << element << " ";
     continue;
   }
   std::string shortened_string = delete_first_n_occurences(element, to_linearize, seen_string_x_times[element]);
   int args = count_arguments(shortened_string);
   olinearized_mrl << element << "@" << args << " ";
  }

  to_linearize = olinearized_mrl.str();
  boost::trim(to_linearize);
  return 0;
}

}
