#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <vector>

// argc is the no of arguments and argv is the pointer to the arguments array
// pointer
int main(int argc, char **argv) {
  std::vector<std::string> args(
      argv + 1, argv + argc); // first argument is the binary name
  std::cout << "Got first Arguement...";
  std::copy(
      args.begin(), args.end(),
      std::ostream_iterator<std::string>(
          std::cout,
          " ")); // copy args and print it to output stream we use iostream
                 // iterator coz std::copy expects an iterator not stream
  return EXIT_SUCCESS;
}
