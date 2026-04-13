#pragma once

#include <string>
#include <sys/types.h>

class Network {
public:
  Network(pid_t pid);

  void setup();
  void teardown();

private:
  pid_t pid;
  std::string veth_host;
  std::string veth_container;
  std::string bridge;
};


