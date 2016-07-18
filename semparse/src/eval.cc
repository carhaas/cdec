#include <boost/program_options.hpp>

#include <fstream>
#include <string>
#include <vector>
#include <ostream>
#include <iostream>

#include "read_write.h"

/*
  Given a set of answers and gold answers, this script produces an evaluation file
  containg recall, precision and F1 as well as a file that can be used to test
  significances.
*/
int main(int argc, char** argv) {

  try {
    namespace po = boost::program_options;
    po::options_description desc("Options");
    desc.add_options()
      ("help", "Print help messages")
      ("evaluation_file,e", po::value<std::string>()->required(),
        "file path where the evaluation results should be written")
      ("significane_testing_file,s", po::value<std::string>()->required(),
        "file path where the file for signficiant testing should be written")
      ("hypothesis_answer_file,h", po::value<std::string>()->required(),
        "file path to the hypothesis' answers")
      ("gold_answer_file,g", po::value<std::string>()->default_value(""),
        "file path to the gold answers");

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

    int empty = 0;
    int false_positive = 0;
    int true_positive = 0;

    std::vector<std::string> gold_answers;
    std::vector<std::string> hyp_answers;
    std::vector<std::string> sigf;

    std::string gold_answer_file = vm["gold_answer_file"].as<std::string>();
    std::string hypothesis_answer_file = vm["hypothesis_answer_file"].as<std::string>();

    semparse::read_file(gold_answer_file, gold_answers);
    semparse::read_file(hypothesis_answer_file, hyp_answers);

    int counter = 0;
    for(auto it : hyp_answers){
      //the gold MRL can lead to an empty answer if the database has been updated
      //and a before relevant record cannot be found anymore
      //(e.g. "What are the opening times of x?""
      //but x has since been closed and thus deleted from the db)
      if(gold_answers[counter] == ""){
        counter++;
        continue;
      }
      if(hyp_answers[counter] == gold_answers[counter]){
        true_positive++;
        sigf.push_back("1 1 1");
      } else if(hyp_answers[counter] == "empty" || hyp_answers[counter] == ""){
        empty++;
        sigf.push_back("0 0 1");
      } else {
        false_positive++;
        sigf.push_back("0 1 1");
      }
      counter++;
    }

    semparse::write_file(vm["significane_testing_file"].as<std::string>(), sigf);

    double precision = 0;
    double recall = 0;
    double f1 = 0;

    if((true_positive+false_positive)!=0){
      precision = 1.0 * true_positive / (true_positive + false_positive);
    }
    if(counter!=0){
      recall = 1.0 * true_positive /counter;
    }
    if((precision + recall)!=0){
      f1 = 2.0 * precision * recall / (precision + recall);
    }

    std::ostringstream oeval;
    oeval << "p:\t" << precision << "\tr:\t" << recall << "\tf1:\t" << f1;
    semparse::write_file(vm["evaluation_file"].as<std::string>(), oeval.str());
  }
  catch(std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  return 0;
}
