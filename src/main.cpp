#include "container.hpp"
#include "utils.hpp"
#include "string"

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

  // root directory for container
  utils::cmd("mkdir -p /tmp/aegis_root/bin /tmp/aegis_root/proc /tmp/aegis_root/lib /tmp/aegis_root/lib64 /tmp/aegis_root/usr/bin");

  // A list of essential binaries for container
  const std::vector<std::string> bins = {"/bin/bash", "/bin/ps", "/bin/hostname", "/bin/ls", "/bin/ip", "/bin/ping"};

  for(const auto& bin : bins) {
    // copy binary inside container
    utils::cmd("cp " + bin + " /tmp/aegis_root/bin");

    // command to find and copy all shared library dependencies for the binary
    std::string ldd_cmd = "ldd " + bin + " | grep -oE '/[^ ]+'" + " | xargs -I '{}' cp --parents '{}' /tmp/aegis_root/";
    utils::cmd(ldd_cmd);
  }

  // this is responsible for loading and linking shared libraries at runtime
  utils::cmd("cp /lib64/ld-linux-x86-64.so.2 /tmp/aegis_root/lib64/");

  // python executable
  utils::cmd("cp /usr/bin/python3 /tmp/aegis_root/usr/bin/");


  utils::cmd("mkdir -p /tmp/aegis_root/etc");
  // dns config 
  utils::cmd("cp /etc/resolv.conf /tmp/aegis_root/etc/");


  Config config;
  config.hostname = "Aegis";
  config.rootfs = "/tmp/aegis_root";

  for(int i=1; i<argc; i++) {
    config.command.push_back(argv[i]);
  }
  
  Container container(config);
  return container.run(); // return the success code from run()
}
