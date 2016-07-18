#ifndef _SEMPARSE_READ_WRITE_H_
#define	_SEMPARSE_READ_WRITE_H_

#include <iostream>
#include <fstream>

/*
  Handles various reading and writing operations from and to files
*/
namespace semparse {

/*
  Given a path to a file, it reads each line into a vector
*/
static __inline__ int read_file(const std::string& file_name,
                                 std::vector<std::string>& lines){
  std::string line;
  std::ifstream ifile(file_name);
  if (!ifile.is_open()){
    std::cerr << "The following file does not exist: "
              << file_name << std::endl;
    exit (EXIT_FAILURE);
  }
  while(getline(ifile, line)){
    lines.push_back(line);
  }
  ifile.close();
  return 0;
}

/*
  Given a path to a file, it reads each line into a ostringstream
*/
static __inline__ int read_file(const std::string& file_name,
                                 std::ostringstream& oss){
  std::string line;
  std::ifstream ifile(file_name);
  if (!ifile.is_open()){
    std::cerr << "The following file does not exist: "
              << file_name << std::endl;
    exit (EXIT_FAILURE);
  }
  while(getline(ifile, line)){
    oss << line << std::endl;
  }
  ifile.close();
  return 0;
}

/*
  Given a path to a file, it write each line from a vector to the file
*/
static __inline__ int write_file(const std::string& file_name,
                                  const std::vector<std::string>& lines){
  std::ofstream ofile(file_name);
  if(!ofile.is_open()){
    std::cerr << "The following file cannot be opened for writing"
              << file_name << std::endl;
    exit (EXIT_FAILURE);
  }
  for(auto it : lines){
    ofile << it << std::endl;
  }
  ofile.close();
  return 0;
}

static __inline__ int write_file(const std::string& file_name,
                                  const std::string& line){
  std::ofstream ofile(file_name);
  if(!ofile.is_open()){
    std::cerr << "The following file cannot be opened for writing"
              << file_name << std::endl;
    exit (EXIT_FAILURE);
  }
  ofile << line << std::endl;
  ofile.close();
  return 0;
}

} // namespace semparse

#endif
