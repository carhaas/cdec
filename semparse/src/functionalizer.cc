#include "functionalizer.h"

#include <stack>
#include <vector>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/lexical_cast.hpp>

#include "nl_mrl_pair.h"
#include "ff_register.h"
#include "decoder.h"

namespace semparse {

Functionalizer::~Functionalizer() {}

int Functionalizer::insert_pass_through_words(std::vector<std::string>& words, const NLMRLPair& nl_mrl_pair){
  std::vector<std::string> stemmed_words;
  std::vector<std::string> non_stemmed_words;
  boost::split(stemmed_words, nl_mrl_pair.preprocessed_sentence, boost::is_any_of(" "));
  boost::split(non_stemmed_words, nl_mrl_pair.non_stemmed, boost::is_any_of(" "));
  int word_position = -1;
  for(unsigned i = 0; i < words.size(); i++){
    word_position = -1;
    if(!boost::contains(words[i], "@")){
      for(unsigned j = 0; j < stemmed_words.size(); j++){
        if(words[i] == stemmed_words[j]){
          word_position = j;
        }
        if(word_position != -1){
          std::ostringstream onew_word;
          onew_word << non_stemmed_words[word_position] << "@s";
          words[i] = onew_word.str();
          word_position = -1;
        }
      }
    }
  }
  return 0;
}

/*
  Checks if the string is a valid tree.
*/
std::string Functionalizer::validate_if_tree(const std::vector<std::string>& words){
  std::stack<int> stack_arity;
  std::stringstream functionalized_mrl;
  std::string prev = "";
  for(auto word : words){
    if(boost::contains(word, "@")){ //every word needs @ at the end to indicate number of children
      std::vector<std::string> text_and_arity;
      boost::split(text_and_arity, word, boost::is_any_of("@"));
      if(text_and_arity.size()!=2){ //only 1 @ is allowed. any other (e.g. in names) need to be escaped
        return "";
      }
      //arity is either a int or "s". s is used for value positions, corresponding key positions have 0
      std::string& text = text_and_arity[0];
      int numbered_arity;
      bool s_arity_found = false;
      if(text_and_arity[1]=="s"){
        numbered_arity = -1;
        s_arity_found = true;
      } else {
        try{
          numbered_arity = boost::lexical_cast<int>(text_and_arity[1]);
        } catch (boost::bad_lexical_cast) {
          return "";
        }
      }
      if(numbered_arity > 0){
        functionalized_mrl << text << "(";
        stack_arity.push(numbered_arity);
      } else {
        if(numbered_arity == -1 && stack_arity.size() == 0){
          return "";
        }
        //add quotes
        if(s_arity_found || prev == "keyval" || prev == "findkey"){ //findkey is unnecessary if it only holds a key but if there is additionally a topx then we need this here
          boost::replace_all(text, "€", " ");
          std::stringstream add_quotes;
          add_quotes << "'" << text << "'";
          text = add_quotes.str();
        }
        functionalized_mrl << text;
        while(stack_arity.size() > 0){
          int top = stack_arity.top();
          stack_arity.pop();
          if(top > 1){
            functionalized_mrl << ",";
            stack_arity.push(top - 1);
            break;
          } else {
            functionalized_mrl << ")";
          }
        }
      }
      prev = text; //need this if the prev token is keyval, then we have a key here which also needs to be wrapped in '
    } else {
      return "";
    }
  }
  if(stack_arity.size() != 0){
    return "";
  }
  return functionalized_mrl.str();
}

int Functionalizer::split_number_into_digits(std::string& digits_to_split_up){
  std::stringstream assemble_single_digits;
  for(auto character : digits_to_split_up){
    assemble_single_digits << character << " ";
  }
  digits_to_split_up = assemble_single_digits.str();
  return 0;
}

/*
  Given a cdec CFG it is possible to further verify if the basic tree is also
  a tree under the given CFG
*/
bool Functionalizer::check_MRL_tree(std::string check_mrl, const std::string& cfg){
  //get the mrl into the correct format for the CFG check
  check_mrl = boost::regex_replace(check_mrl, boost::regex("\\("), "( ");
  check_mrl = boost::regex_replace(check_mrl, boost::regex(","), " , ");
  check_mrl = boost::regex_replace(check_mrl, boost::regex("\\)"), " )");
  check_mrl = boost::regex_replace(check_mrl, boost::regex("name:.*? \\)"), "name:lg' )");
  check_mrl = boost::regex_replace(check_mrl, boost::regex("keyval\\( '([^\\(\\)]+?)' , '[^\\(\\)]+?' "), "keyval( '$1' , 'valvariable' "); // the comma is there to ensure that only -value- positions are replaced with valvariable
  check_mrl = boost::regex_replace(check_mrl, boost::regex("keyval\\( '([^\\(\\)]+?)' , or\\( '[^\\(\\)]+?' , '[^\\(\\)]+?' "), "keyval( '$1' , or( 'valvariable' , 'valvariable' "); //nasty hack for when we have a or() around values:  or( ' greek ' , ' valvariable ' )
  check_mrl = boost::regex_replace(check_mrl, boost::regex("keyval\\( '([^\\(\\)]+?)' , and\\( '[^\\(\\)]+?' , '[^\\(\\)]+?' "), "keyval( '$1' , and( 'valvariable' , 'valvariable' "); //nasty hack for when we have a and() around values:  and( ' greek ' , ' valvariable ' )
  check_mrl = boost::regex_replace(check_mrl, boost::regex(" '(.*?)' "), " ' $1 ' ");
  //split up numbers into individual digits
  boost::smatch match;
  if(boost::regex_search(check_mrl, match, boost::regex("topx\\( (.*?) \\)"))){
    std::string number = match[1];
    split_number_into_digits(number);
    check_mrl = boost::regex_replace(check_mrl, boost::regex("topx\\( .*?\\)"), "topx( "+number+")");
  }
  if(boost::regex_search(check_mrl, match, boost::regex("maxdist\\( ([0-9]+?) \\)"))){
    std::string number = match[1];
    split_number_into_digits(number);
    check_mrl = boost::regex_replace(check_mrl, boost::regex("maxdist\\( .*?\\)"), "maxdist( "+number+")");
  }
  std::ostringstream oconfig;
  oconfig << "formalism=scfg" << std::endl;
  oconfig << "intersection_strategy=cube_pruning" << std::endl;
  oconfig << "cubepruning_pop_limit=1000" << std::endl;
  oconfig << "grammar=" << cfg << std::endl;
  oconfig << "scfg_max_span_limit=1000" << std::endl;
  bool parse = true;
  std::istringstream iconfig(oconfig.str());
  Decoder decoder_validate(&iconfig);
  decoder_validate.Decode(check_mrl, NULL, NULL, &parse);
  if(!parse){
    return false;
  }
  return true;
}

int Functionalizer::functionalize(std::string& candidate, NLMRLPair& nl_mrl_pair, const bool& pass, const std::string& cfg){
  boost::replace_all(candidate, "<topx>", "");
  boost::replace_all(candidate, "</topx>", "@0");

  std::vector<std::string> words;
  boost::split(words, candidate, boost::is_any_of(" "));

  if(pass){
    insert_pass_through_words(words, nl_mrl_pair);
  }

  std::string functionalized_mrl = validate_if_tree(words);
  if(functionalized_mrl == ""){ return 0; }

  if(cfg!=""){
    bool valid_under_CFG = check_MRL_tree(functionalized_mrl, cfg);
    if(!valid_under_CFG){
      return 0;
    }
  }

  boost::replace_all(functionalized_mrl, "SAVEAPO", "'");
  boost::replace_all(functionalized_mrl, "BRACKETOPEN", "(");
  boost::replace_all(functionalized_mrl, "BRACKETCLOSE", ")");

  nl_mrl_pair.mrl = functionalized_mrl;
  return 0;
}

} // namespace semparse
