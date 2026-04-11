#include "container.hpp"
#include <iostream>

// argc is the no of arguments and argv is the pointer to the arguments array
// pointer
int main(int argc, char **argv) {
  if(argc < 2) {
    std::cerr << "Usage: ./container <cmd>";
    return 1;
  } 
  // std::vector<std::string> args(argv + 1, argv + argc); // first argument is the binary name
  // std::cout << "Running Arguement: ";
  // std::copy(args.begin(), args.end(), std::ostream_iterator<std::string>(std::cout, " ")); // copy args and print it to output stream we use iostream
  // iterator coz std::copy expects an iterator not stream
  // std::cout << std::flush;
  // execvp C API expect char*[] so convert vector to c compatible format
  
  Config config;
  config.hostname = "Aegis";
  config.rootfs = "/home/karthik/Projects/Aegis/ubuntu-file-system";

  for(int i=1; i<argc; i++) {
    config.command.push_back(argv[i]);
  }
  
  Container container(config);
  return container.run(); // return the success code from run()
}
