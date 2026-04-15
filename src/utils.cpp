#include "utils.hpp"
#include <cstdlib>

// system runs a command just like how it is run in terminal
// it creates a bash process runs the command and waits for the result
void utils::cmd(const std::string& c) {
  system(c.c_str());
}
