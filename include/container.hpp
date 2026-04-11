#pragma once // include guard

#include <string>
#include <vector>

struct Config {
  std::string hostname;
  std::string rootfs;
  std::vector<std::string> command;
};

class Container {
  public:
    explicit Container(const Config& config);

    int run(); // entry point

  private:
    Config config_;

    void setup_env(); //hostname, mount, chroot

    // clone() expects a plain function pointer so declare this child_func as static function so it has no 'this' pointer
    static int child_func(void* arg); // clone entry

    void apply_cgroups(pid_t pid); 
};
