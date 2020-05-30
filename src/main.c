#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  char *analyzerOutput = "outputAnalyzer.txt";
  char *analyzerLog = "log.txt";
  int pi[2];
  pipe(pi);
  int i = 0;
  if (fork() > 0) {
    int fd = open("/dev/null", O_WRONLY);
    dup2(fd, 2);
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
    dup2(log, 2);
    dup2(out, 1);
    close(pi[1]);
    dup2(pi[0], 0);
    close(pi[0]);

    execlp("./analyzer", "./analyzer", NULL);
    kill(getpid(), SIGKILL);
  }
}
