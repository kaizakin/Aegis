#include "container.hpp"
#include "network.hpp"

#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>

#include <unistd.h>
#include <sched.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <filesystem>
#include <fstream>

struct ChildArgs {
    int read_fd;
    int write_fd;
    Container* container;
};

// constructor
Container::Container(const Config& config) : config_(config) {}

static void cmd(const std::string& c){
  system(c.c_str());
}


void Container::setup_env() {
  // make this mount system private from the outer environment, MS_REC applies this recursively to all files and folders
  // MS_ = mount flags
  if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) == -1) {
    perror("mount private root failed");
    exit(1);
  }


  if (sethostname(config_.hostname.c_str(), config_.hostname.length()) == -1) {
    perror("sethostname failed");
    exit(1);
  }
  // change the root file system to a new file system
  if (chroot(config_.rootfs.c_str()) == -1) {
    perror("chroot failed");
    exit(1);
  }
  // bring the working dir to currect fake root
  if (chdir("/") == -1) {
    perror("chdir failed");
    exit(1);
  }

  // since the CLONE_NEWPID is set this creates a new proc view for this namespace
  if (mount("proc", "proc", "proc", 0, "") == -1) {
    perror("mount proc failed");
    exit(1);
  }
}

// child function for clone()
int Container::child_func(void* arg) {
//   Container* container = static_cast<Container*>(arg);
   ChildArgs* args = (ChildArgs*)arg;

   close(args->write_fd); // close unused write fd

   char buf;
   read(args->read_fd, &buf, 1); // BLOCK until a byte is read

   close(args->read_fd);

   cmd("ip link set lo up") ; // bring up the loopback interface
   cmd("ip link set veth1 up"); 
   cmd("ip addr add 10.0.0.2/24 dev veth1"); 
   cmd("ip route add default via 10.0.0.1");
   // setup container environment
   args->container->setup_env();

   std::vector<char*> argv;
   for(auto& str : args->container->config_.command){
     argv.push_back(const_cast<char*>(str.c_str()));
   }
   argv.push_back(NULL);

  pid_t pid = fork(); // this splits the execution into two (a child and a parent both process continues execution after this point)
                      // this fork() returns twice 0 for the child process and child's pid for the parent 
  if (pid == -1) {
    perror("fork failed");
    return 1;
  }
  if (pid == 0) { // both process continue execution, if the process is child it runs the if statement, if it is the parent it executes the else statement
    // first arguement is the binary name used to look up for that binary in system PATH
    execvp(argv[0], argv.data()); // argv.data() gives the pointer to the arguements array
    perror("execvp failed");
    exit(1);
  } else  {
    waitpid(pid, nullptr, 0); // block until child exits
    std::cout<<"cleaning up the proc mount"<<std::endl;
    if (umount("proc") == -1) {
      perror("umount proc failed");
      return 1;
    }
  }
  return 0;
}


// cgroups v2
// Order matters in v2
// 1. create cgroup
// 2. Enable controllers
// 3. set limits
// 4. add process
void Container::apply_cgroups(pid_t pid) {
  
  namespace fs = std::filesystem;
  // create path
  fs::path cg_path{"/sys/fs/cgroup/Aegis"};

  // create those in the filesystem
  fs::create_directories(cg_path);

  std::ofstream ofs;

  // in v2 controllers are not enabled by default
  // enabling controllers (required in v2)
  ofs.open("/sys/fs/cgroup/cgroup.subtree_control");
  ofs << "+pids +memory";
  ofs.close();

  // limit no of processes
  ofs.open(cg_path / "pids.max");
  ofs << "3";
  ofs.close();

  ofs.open(cg_path / "memory.max");
  ofs << "209715200"; // memory byte limit 200MB
  ofs.close();

  // add process to cgroup
  ofs.open(cg_path / "cgroup.procs");
  ofs << std::to_string(pid);
  ofs.close();
}

// entry point - run the container
int Container::run() {
  // malloc points to the start of the created memory
  constexpr std::size_t stack_size = 1024 * 1024; // constexpr = constant expression (this is computed at compile time rather than runtime)
  auto* stack = (char*)malloc(stack_size); // sizeof char coz char has exactly 1 byte size, 1024*1024 makes it 1MB
  if (stack == nullptr) {
    std::cerr << "malloc failed" << std::endl;
    return 1;
  }
  auto* stackTop = stack + stack_size; // in most architectures stack grows from top to bottom(higher memory address to lower memory address) so this pointer points to top of the stack
  // clone expects the developer to manage memory for the child process

  // child needs to wait until parent sets up network 
  // use pipe to block and notify once the network setup is complte
  int fd[2];
  pipe(fd);

  ChildArgs args = { fd[0], fd[1], this };
  
  // SIGCHLD notifies the parent when the child exits
  // child_func is the function that first gets executed in child process
  // CLONE_NEWUTS creates a new uts namespace (new hostname) and arg is the arguments that the child process gets
  pid_t child_process_id = clone(child_func, stackTop, SIGCHLD | CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWNET, &args); // this passes the pointer to the current object
  if (child_process_id == -1) {
    perror("clone failed");
    free(stack);
    return 1;
  }

  close(fd[0]); // close unused read end in the parent side (avoid unecessary errors)

  Network network(child_process_id);
  network.setup(); // setup network for container 

  apply_cgroups(child_process_id);

  write(fd[1], "x", 1); // send data thru pipe to let child know network setup is done

  close(fd[1]); // close it once the write is done


  waitpid(child_process_id, nullptr, 0);// wait until child exits, ignore exit status, 0 => block until done 

  // network.teardown(); // child has exited, so it's safe to clear up the networking state
  // teardown will be called automatically in desctructor

  free(stack); // developer is responsible for managing memory so free it once the process ends

  return 0;
}
