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

  // overlayFS
  // Base dir
  utils::cmd("mkdir -p /tmp/aegis/lower /tmp/aegis/upper /tmp/aegis/work /tmp/aegis/merged");

  // base filesystem in lowerdir
  utils::cmd("mkdir -p /tmp/aegis/lower/bin /tmp/aegis/lower/proc /tmp/aegis/lower/lib /tmp/aegis/lower/lib64 /tmp/aegis/lower/usr/bin");

  // A list of essential binaries for container
  const std::vector<std::string> bins = {"/bin/bash", "/bin/ps", "/bin/hostname", "/bin/ls", "/bin/ip", "/bin/ping"};

  for(const auto& bin : bins) {
    // copy binary inside container (lower dir)
    utils::cmd("cp " + bin + " /tmp/aegis/lower/bin");

    // command to find and copy all shared library dependencies for the binary
    std::string ldd_cmd = "ldd " + bin + " | grep -oE '/[^ ]+' | xargs -I '{}' cp --parents '{}' /tmp/aegis/lower/";
    utils::cmd(ldd_cmd);
  }

  // this is responsible for loading and linking shared libraries at runtime
  utils::cmd("cp /lib64/ld-linux-x86-64.so.2 /tmp/aegis/lower/lib64/");

  // python executable
  utils::cmd("cp /usr/bin/python3 /tmp/aegis/lower/usr/bin/");

  // dns config 
  utils::cmd("mkdir -p /tmp/aegis/lower/etc");
  utils::cmd("cp /etc/resolv.conf /tmp/aegis/lower/etc/");

  // merged becomes the combined filesystem view
  std::string overlay_mount =
    "mount -t overlay overlay "
    "-o lowerdir=/tmp/aegis/lower,"
    "upperdir=/tmp/aegis/upper,"
    "workdir=/tmp/aegis/work "
    "/tmp/aegis/merged";

  utils::cmd(overlay_mount);

  Config config;
  config.hostname = "Aegis";
  config.rootfs = "/tmp/aegis/merged";

  for(int i=1; i<argc; i++) {
    config.command.push_back(argv[i]);
  }
  
  Container container(config);
  return container.run(); // return the success code from run()
}
