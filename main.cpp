#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <vector>
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/mount.h>


int runtime(void* args){
  std::vector<char*>* arg = (std::vector<char*>*) args;
  std::string hostname = "Aegis";
  sethostname(hostname.c_str(), hostname.length());
  // change the root file system to a new file system
  chroot("/home/karthik/Projects/Aegis/ubuntu-file-system");
  // bring the working dir to currect fake root
  chdir("/");

  // since the CLONE_NEWPID is set this creates a new proc view for this namespace
  mount("proc", "proc", "proc", 0, "");// source, target, filesystem type, flags, extra data
   
  // first arguement is the binary name used to look up for that binary in system PATH
  execvp((*arg)[0], (*arg).data()); // run_arg.data() gives the pointer to the arguements array
  return 1;
}

// argc is the no of arguments and argv is the pointer to the arguments array
// pointer
int main(int argc, char **argv) {
  std::vector<std::string> args(argv + 1, argv + argc); // first argument is the binary name
  std::cout << "Running Arguement: ";
  std::copy(args.begin(), args.end(), std::ostream_iterator<std::string>(std::cout, " ")); // copy args and print it to output stream we use iostream
  // iterator coz std::copy expects an iterator not stream
  std::cout << std::flush;
  // execvp C API expect char*[] so convert vector to c compatible format
  std::vector<char*> run_arg;

  for(auto &str : args){
     run_arg.push_back(const_cast<char*>(str.c_str())); // .c_str gives const char* but c api doesn't expect const so we remove const using const_cast
  }

  run_arg.push_back(nullptr); // NULL denotes the end of the arguements
  
  // malloc points to the start of the created memory
  auto* stack = (char*)malloc(sizeof(char) * 1024 * 1024); // sizeof char coz char has exactly 1 byte size, 1024*1024 makes it 1MB
  auto* stackTop = stack + sizeof(char) * 1024 + 1024; // in most architectures stack grows from top to bottom(higher memory address to lower memory address) so this pointer points to top of the stack
  // clone expects the developer to manage memory for the child process
  
  void* arg = static_cast<void*>(&run_arg);// clone expects void* so conver em
  // we're passing SIGCHLD which notifies the parent when the child exits
  // runtime is the function that first gets executed in child process
  // CLONE_NEWUTS creates a new uts namespace (new hostname) and arg is the arguments that the child process gets
  pid_t child_process_id = clone(runtime, stackTop, SIGCHLD | CLONE_NEWUTS | CLONE_NEWPID, arg);
  waitpid(child_process_id, nullptr, 0);// wait until child exits, ignore exit status, 0 => block until done 
  free(stack); // developer is responsible for managing memory so free it once the process ends

  return EXIT_SUCCESS;
}
