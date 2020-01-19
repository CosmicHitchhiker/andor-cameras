#include "andor-daemon-new.h"

using namespace std;

#define DEFAULT_PORT 1234

int Daemon(int argc, char* argv[]);
int Main(int argc, char* argv[]);


int main(int argc, char* argv[]) {
  return Daemon(argc, argv);
}

int Daemon(int argc, char* argv[]) {
  pid_t process_id = 0;
  pid_t sid = 0;

  // Create child process
  process_id = fork();
  // Indication of fork() failure
  if (process_id < 0) {
    perror("fork failed!\n");
    // Return failure in exit status
    exit(1);
  }

  // PARENT PROCESS. Need to kill it.
  if (process_id > 0) {
    printf("process_id of child process %d \n", process_id);
    // return success in exit status
    exit(0);
  }

  /* THE NEXT PART OF CODE WILL BE RUN ONLY IN CHILD PROCESS */
  // Set new session
  sid = setsid();
  if (sid < 0) {
    perror("Can't set new session.\n");
    // Return failure
    exit(1);
  }

  // Main program
  Main(argc, argv);

  return (0);
}

int Main(int argc, char* argv[]){
  Camera camera;
  string Model = camera.getModel();
  Log log(Model + ".log");
  Socket sock(DEFAULT_PORT, &log);
  camera.init(&log);

  string clientMessage = "";
  while (clientMessage.compare("EXIT")){
    clientMessage = "";
    clientMessage = sock.getMessage();
    camera.parseCommand(clientMessage);
  }

  sock.turnOff();
  return (0);
}
