#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "./config/config.h"
#include "./wrapping/wrapping.h"

int main(int argc, char *argv[]) {
  char *analyzerOutput = "outputAnalyzer.txt";
  char *analyzerLog = "log.txt";
  int pi[2];
  int rc_pi = pipe(pi);
  int i = 0;
  if (rc_pi == SUCCESS) {
    if (fork() > 0) {
      // TODO implement log file if we want to save some error (if there are)
      int fd = open("/dev/null", O_WRONLY);
      createDup(fd, 2);

      char *args[argc + 1];
      int i = 0;
      for (i = 1; i < argc; i++) {
        args[i] = argv[i];
      }
      args[0] = "./reporter";
      args[argc] = NULL;

      execvp("./reporter", args);
      kill(getpid(), SIGKILL);
    } else {
      int out = open(analyzerOutput, O_WRONLY | O_TRUNC | O_CREAT, 00644);
      int log = open(analyzerLog, O_WRONLY | O_TRUNC | O_CREAT, 00644);
      if (out > SUCCESS && log > SUCCESS) {
        createDup(log, 2);
        createDup(out, 1);
      } else {
        int theVoid = open("/dev/null", O_WRONLY);
        createDup(theVoid, 2);
        createDup(theVoid, 1);
      }
      close(pi[1]);
      createDup(pi[0], 0);
      close(pi[0]);

      execlp("./analyzer", "./analyzer", NULL);
      kill(getpid(), SIGKILL);
    }
  }
}
