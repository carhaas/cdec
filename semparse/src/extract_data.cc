#include <vector>
#include <ostream>
#include <iostream>

#include <boost/program_options.hpp>

#include "extractor.h"

/*
  Prepares train, tune or test files to be used in following pipeline steps
*/
int main(int argc, char** argv) {

  try {
    namespace po = boost::program_options;
    po::options_description desc("Options");
    desc.add_options()
      ("help", "Print help messages")
      ("set_files,f", po::value<std::string>()->required(),
        "path to data set (assumes that adding '.language' (see below) and '.mrl' lead to the correct files)")
      ("files_type,t", po::value<std::string>()->required(),
        "specify train, tune or test")
      ("language,l", po::value<std::string>()->required(),
        "the language, i.e. file ending to expected for the natural language file (e.g. 'en')")
      ("model_directory,d", po::value<std::string>()->required(),
        "The directory where the model will be written")
      ("stem,s", po::value<bool>()->required(),
        "if the natural language side should be stemmed");

    po::variables_map vm;
    try {
      po::store(po::parse_command_line(argc, argv, desc), vm);
      if(vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
      }
      po::notify(vm);
    } catch(po::error& e) {
      std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
      std::cerr << desc << std::endl;
      return 1;
    }

    std::string experiment_dir = vm["model_directory"].as<std::string>();
    std::string set_files = vm["set_files"].as<std::string>();
    std::string files_type = vm["files_type"].as<std::string>();
    std::string language = vm["language"].as<std::string>();
    bool stem = vm["stem"].as<bool>();

    if(files_type == "train" || files_type == "tune" || files_type == "test"){
      semparse::extract_set(experiment_dir, files_type,
                            set_files, language, stem);
    } else {
      throw po::validation_error(po::validation_error::invalid_option_value,
                                 "files_type");
    }

  }
  catch(std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  return 0;
}
