#include <boost/program_options.hpp>

#include <fstream>
#include <string>
#include <vector>
#include <ostream>
#include <iostream>

#include "nl_mrl_pair.h"
#include "functionalizer.h"
#include "read_write.h"

#include "ff_register.h"
#include "decoder.h"

/*
  Given a trained model, it decodes a file of natural language questions into MRL formulae
*/
int main(int argc, char** argv) {

  try {
    namespace po = boost::program_options;
    po::options_description desc("Options");
    desc.add_options()
      ("help", "Print help messages")
      ("model_directory,d", po::value<std::string>()->required(),
        "Parser model's directory path")
      ("cfg_file,g", po::value<std::string>()->default_value(""),
        "Path to language specific CFG that is used to further validate the MRL trees")
      ("weights_file,w", po::value<std::string>()->required(),
        "Use this weight file to decode")
      ("config_file,c", po::value<std::string>()->required(),
        "Use this config file to decode")
      ("mrl_file,m", po::value<std::string>()->required(),
        "MRL file's output path")
      ("pass_through,p", po::value<bool>()->default_value(1),
        "Pass through unknown words");

    po::variables_map vm;
    try {
      po::store(po::parse_command_line(argc, argv, desc), vm);
      if(vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
      }
      po::notify(vm);
    }  catch(po::error& e) {
      std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
      std::cerr << desc << std::endl;
      return 1;
    }

    std::string model_directory = vm["model_directory"].as<std::string>();
    std::string cfg = vm["cfg_file"].as<std::string>();
    bool pass_through = vm["pass_through"].as<bool>();

    std::vector<std::string> preprocessed_sentence;
    std::vector<std::string> nonstemmed_lines; //not stemmed for sure. (needed to recover the original word when a unknown word is passed through)
    std::vector<std::string> grammar_lines; //line contains the grammar file location

    semparse::read_file(model_directory+"/test.nl", preprocessed_sentence);
    semparse::read_file(model_directory+"/test.nostem.nl", nonstemmed_lines);
    semparse::read_file(model_directory+"/test.inline.nl", grammar_lines);

    if(preprocessed_sentence.size()!=nonstemmed_lines.size()
       || preprocessed_sentence.size()!=grammar_lines.size()){
      std::cerr << "The files test.nl, test.nostem.nl and test.inline.nl need to be of equal length" << std::endl;
      exit (EXIT_FAILURE);
    }

    std::ostringstream oconfig;
    semparse::read_file(vm["config_file"].as<std::string>(), oconfig);
    oconfig << "weights=" << vm["weights_file"].as<std::string>() << std::endl;
    std::string config = oconfig.str();

    std::ofstream mrl_out_file(vm["mrl_file"].as<std::string>());
    if(!mrl_out_file.is_open()){
      std::cerr << "The following file cannot be opened for writing"
                << vm["mrl"].as<std::string>() << std::endl;
      exit (EXIT_FAILURE);
    }

    register_feature_functions();
    semparse::Functionalizer functionalizer;
    for(unsigned c = 0; c < grammar_lines.size(); c++){
      semparse::NLMRLPair nl_mrl_pair = {"", "", "", ""};

      //preprocessed sentence
      nl_mrl_pair.non_stemmed = nonstemmed_lines[c];
      nl_mrl_pair.preprocessed_sentence = preprocessed_sentence[c];

      //decode
      std::istringstream iconfig(config);
      Decoder decoder(&iconfig);
      std::vector<std::string> kbest_out;
      decoder.Decode(grammar_lines[c], NULL, &kbest_out);

      if(kbest_out.size()>0){
        //search kbest for valid tree
        for(auto candidate : kbest_out) {
          functionalizer.functionalize(candidate, nl_mrl_pair, pass_through, cfg);
          if(nl_mrl_pair.mrl != ""){
            break;
          }
        }
        if(nl_mrl_pair.mrl!=""){
          mrl_out_file << nl_mrl_pair.mrl << std::endl;
        } else {
          mrl_out_file << "no mrl found" << std::endl;
        }
      }
    }

    mrl_out_file.close();
  }
  catch(std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  return 0;
}
