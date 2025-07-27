#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <memory>


//#define PRINT(x) cout << __FILE__ << " " <<__func__ << " " << __LINE__ << x << endl;
#define PRINT(x) do {} while(0);

using namespace std;

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

class Command {
// TODO: Add your data members
protected:
    string commandLine;

 public:
  Command(const char* cmd_line);
  string commandText;
  string OGcommandText;
  time_t timeoutTime;
  pid_t pid;
  virtual ~Command();
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() = default;
  void execute() override;
};


class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
public:
  ChangeDirCommand(const string &cmd_line);
  virtual ~ChangeDirCommand() {}
  void execute() override;
};

class ChangePromptCommand : public BuiltInCommand {
// TODO: Add your data members public:
  public:
    ChangePromptCommand(const char* cmd_line);
    virtual ~ChangePromptCommand() {}
    void execute() override;
    char* newPrompt;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class ChmodCommand : public BuiltInCommand {
 public:
  ChmodCommand(const char* cmd_line);
  virtual ~ChmodCommand() {}
  void execute() override;
};

class QuitCommand : public BuiltInCommand {
 public:
  QuitCommand(const char* cmd_line);
  virtual ~QuitCommand() {}
  void execute() override;
};


class JobsList {
 public:
  class JobEntry {
    public:
      int jobId;
      int pid;
      bool isStopped;
      JobEntry(std::shared_ptr<Command> cmd, int pid, int jobId, bool isStopped = false);
      std::shared_ptr<Command> command;
   // TODO: Add your data members
  };
 // TODO: Add your data members
 public:
 int highestJobId = 0;
  vector<JobEntry> jobs; 
  JobsList();
  ~JobsList();
  void addJob(std::shared_ptr<Command> cmd, int pid, bool isStopped = false);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  JobsCommand(const char* cmd_line);
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  KillCommand(const char* cmd_line);
  virtual ~KillCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  ForegroundCommand(const char* cmd_line);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class SmallShell {
 private:
    pid_t ProcessId;
    JobsList ShellJobs;
    string CurrentDir;
    string PrevDir;
    pid_t currentForegroundCommandPid;
    vector<std::shared_ptr<Command>> timedCommands;
    SmallShell();
 public:
  char* prompt = (char*)"smash> ";
  JobsList jobsList;
  std::shared_ptr<Command> CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);

  pid_t getProcessId() const { 
    return ProcessId; 
  }
  pid_t getCurrentForegroundCommandPid() const { 
    return currentForegroundCommandPid; 
  }

  void setCurrentForegroundCommandPid(pid_t id) { 
    currentForegroundCommandPid = id; 
  }

  void pushTimedCommand(std::shared_ptr<Command> command) { 
    timedCommands.push_back(command);
  }

  void popTimedCommand(std::shared_ptr<Command> command) {
      auto it = timedCommands.begin();
      while(it != timedCommands.end()) {
          if (command == *it) {
              timedCommands.erase(it);
              ++it;
              return;
          }
      }
  }

  void nextAlarm();

  /*void setAlarmIsSet(bool set) {
    alarmIsSet = set;
  }

  bool getAlarmIsSet() {
    return alarmIsSet;
  }*/


  vector<std::shared_ptr<Command>> getTimedCommands() const { 
    return timedCommands; 
  }

  string getPrevDir() const {
    return PrevDir;
  };

  void setPath(const string &targetPath) {
    if (!targetPath.empty()) {
        PrevDir = CurrentDir;
        CurrentDir = targetPath;
    }
  }
};

#endif //SMASH_COMMAND_H_
