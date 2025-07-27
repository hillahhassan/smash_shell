#include <iostream>
#include <signal.h>
#include <sys/wait.h>
#include <cerrno>
#include <cstring>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlCHandler(int sig_num) {
  cout << "smash: got ctrl-C" << endl;

  SmallShell& shell = SmallShell::getInstance();

  pid_t pid = shell.getCurrentForegroundCommandPid();

  if (pid == -1)
    return;


  if (pid == shell.getProcessId())
    return;

  int result = waitpid(pid, nullptr, WNOHANG);

  if (result != 0)
    return;


  if (kill(pid, SIGKILL))
  {
      perror("smash error: kill failed");
  }

  cout << "smash: process " << pid << " was killed" << endl;

  return;

}

void alarmHandler(int sig_num) {
  cout << "smash: got an alarm" << endl;
  SmallShell& shell = SmallShell::getInstance();
  vector<shared_ptr<Command>> timedCommands = shell.getTimedCommands();
  time_t now = time(nullptr);
  for (shared_ptr<Command> cmd : timedCommands) {
    int result = waitpid(cmd->pid, nullptr, WNOHANG);

    if (result != 0) { // if job is finished then we dont need to do anything
      shell.popTimedCommand(cmd);
      continue;    
    }

    if (now >= cmd->timeoutTime) {
      cout << "smash: " << cmd->OGcommandText << " timed out!" << endl;
      shell.popTimedCommand(cmd);
      
      int result = kill(cmd->pid, SIGKILL);
      if (result != 0)
        perror("smash error: kill failed");
    } 
  }

  shell.nextAlarm();

}

