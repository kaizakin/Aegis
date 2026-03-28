#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <vector>
#include <unistd.h>

// argc is the no of arguments and argv is the pointer to the arguments array
// pointer
int main(int argc, char **argv) {
  std::vector<std::string> args(argv + 1, argv + argc); // first argument is the binary name
  std::cout << "Got first Arguement...\n";
  std::copy(args.begin(), args.end(), std::ostream_iterator<std::string>(std::cout, " ")); // copy args and print it to output stream we use iostream
  // iterator coz std::copy expects an iterator not stream
  std::cout << std::flush;
  // execvp C API expect char*[] so convert vector to c compatible format
  std::vector<char*> run_arg;

  for(auto &str : args){
     run_arg.push_back(const_cast<char*>(str.c_str())); // .c_str gives const char* but c api doesn't expect const so we remove const using const_cast
  }

  run_arg.push_back(nullptr); // NULL denotes the end of the arguements

  // first arguement is the binary name used to look up for that binary in system PATH
  execvp(run_arg[0], run_arg.data()); // run_arg.data() gives the pointer to the arguements array
  return EXIT_SUCCESS;
}
