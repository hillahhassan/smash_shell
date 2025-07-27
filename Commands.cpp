#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <csignal>
#include "Commands.h"
#include <fstream>
#include <fcntl.h>
#include <sys/stat.h>

using namespace std;
const std::string WHITESPACE = " \n\r\t\f\v";
#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT() 
#define MAX_PATH_SIZE 0
#endif

//TODO MAKE SURE ALL NECESSARY PERRORS INSERTED

string getCurrentPath() {
    string result = "";
    char *path = getcwd(nullptr, MAX_PATH_SIZE);
    if (path != nullptr) {
        result = path;
    } else {
        std::cerr << "Error getting current working directory" << std::endl;
    }

    free(path);

    return result;
}

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

bool isInteger(char * buffer)
{
  if (!buffer)
    return false;

  string str(buffer);
  auto iterator = str.begin();
  if (*iterator != '-' && !isdigit(*iterator)) {
      return false;
  }
  iterator++;
  while(iterator !=  str.end()) {
      if (!isdigit(*iterator))
          return false;
      iterator++;
  }
  return true;
}

bool isComplex(const char * buffer) {
  string str(buffer);
  return ((str.find('*') != string::npos) || (str.find('?') != string::npos));
}

// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell() {
  PrevDir = "";
  CurrentDir = getCurrentPath();
  ProcessId = getpid();
  JobsList emptyJobs;
  ShellJobs = emptyJobs;
  currentForegroundCommandPid = -1;
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
std::shared_ptr<Command> SmallShell::CreateCommand(const char* cmd_line) {
	// For example:
  SmallShell& smash = SmallShell::getInstance();

  char* arguments[21];

  int numArgs = _parseCommandLine(cmd_line, arguments);

  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if(firstWord.empty())
    return nullptr;

  shared_ptr<Command> command;
  bool isTimeout = false;
  int timeoutLength;
  char* OGCommand;
  if (string(arguments[0]).compare("timeout") == 0) {
    isTimeout = true;
    timeoutLength = stoi(string(arguments[1]), nullptr, 10);
    cmd_s.erase(cmd_s.find(arguments[0]), string(arguments[0]).length());
    cmd_s.erase(cmd_s.find(arguments[1]), string(arguments[1]).length());
    OGCommand = (char*)cmd_line;
    cmd_line = cmd_s.c_str();
    numArgs = _parseCommandLine(cmd_line, arguments);
  }
  if (cmd_s.find("|") != string::npos) {
        command = std::make_shared<PipeCommand>(cmd_line);
  }
  else if (cmd_s.find(">") != string::npos) {
    command = std::make_shared<RedirectionCommand>(cmd_line);
  }
  else if (string(arguments[0]).compare("chprompt") == 0) {
    if (numArgs > 1)
      command = std::make_shared<ChangePromptCommand>(arguments[1]);
    else
      command = std::make_shared<ChangePromptCommand>("smash");
  }
  else if (string(arguments[0]).compare("showpid") == 0) {
    command = std::make_shared<ShowPidCommand>(cmd_line);
  }
  else if (string(arguments[0]).compare("pwd") == 0) {
    command = std::make_shared<GetCurrDirCommand>(cmd_line);
  }
  else if (string(arguments[0]).compare("cd") == 0) {
    command = std::make_shared<ChangeDirCommand>(cmd_line);
  }
  else if (string(arguments[0]).compare("fg") == 0) {
    command = std::make_shared<ForegroundCommand>(cmd_line);
  }
  else if (string(arguments[0]).compare("jobs") == 0) {
    command = std::make_shared<JobsCommand>(cmd_line);
  }
  else if (string(arguments[0]).compare("quit") == 0) {
    command = std::make_shared<QuitCommand>(cmd_line);
  }
  else if (string(arguments[0]).compare("kill") == 0) {
    command = std::make_shared<KillCommand>(cmd_line);
  }
  else {
    command = std::make_shared<ExternalCommand>(cmd_line);
  }

  if (isTimeout){
    smash.pushTimedCommand(command);
    time_t now = time(nullptr);
    command->timeoutTime = now + timeoutLength;
    command->OGcommandText = OGCommand;
    nextAlarm();
  }

  return command;
}

void safeSetPgrp()
{
    if(setpgrp() == -1)
    {
        perror("smash error: setpgrp failed");
        exit(0);
    }
}

void SmallShell::executeCommand(const char *cmd_line) {
    size_t length = strlen(cmd_line);
    if (length == 0) {
        return;
    }
    bool isBackgound = _isBackgroundComamnd(cmd_line);
    char* cmdLine = new char[length + 1];
    strcpy(cmdLine,cmd_line);
    if (isBackgound)
        _removeBackgroundSign(cmdLine);
  std::shared_ptr<Command> cmd = CreateCommand(cmdLine);
  if(isBackgound) {
      cmd->commandText = string(cmd_line);
      cmd->OGcommandText = string(cmd_line);
  }
  cmd->pid = getpid();

  if (dynamic_cast<ExternalCommand*>(cmd.get()) != nullptr) { //external

    int pid = fork();
      if(pid == 0){
          safeSetPgrp();
      }
    if (pid != 0) { //parent

      cmd->pid = pid;

      if (isBackgound){ // background parent adds job to job list
        jobsList.addJob(cmd, pid);  
      }
      else {// foreground
        currentForegroundCommandPid = pid;
      }
    }
    
    if (pid == 0) { // child executes

      if (isComplex(cmdLine)) {
        execlp("/bin/bash", "/bin/bash","-c", cmdLine, nullptr);
      }
      else { // simple external
        char* arguments[21];
        int numArgs = _parseCommandLine(cmdLine, arguments);
        
        arguments[numArgs + 1] = nullptr;

        execvp(arguments[0], arguments);
        perror("smash error: execvp failed");
        exit(0);
      }
    }

    if (!isBackgound && (pid != 0)) { // foreground parent (smash) waits for child
      int status;
      waitpid(pid, &status, WUNTRACED);
    }
   
  }
  
  else { // built-in
  if(cmd)
      cmd->execute();
  }
    delete[] cmdLine;
}

void SmallShell::nextAlarm() { 
    time_t nextAlarmTime = -1;
    time_t now = time(nullptr);
    for (shared_ptr<Command> command : timedCommands) {
      if (nextAlarmTime == -1) {
        nextAlarmTime = command->timeoutTime - now;
      }
      else if ((command->timeoutTime - now) < nextAlarmTime) {
        nextAlarmTime = command->timeoutTime - now;
      }
    }
    if (nextAlarmTime != -1) {
      alarm(nextAlarmTime);
    }
  }


ChangePromptCommand::ChangePromptCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {
  char* newPrompt = new char[strlen(cmd_line) + 3];  // +2 for the '>' and null terminator
  strcpy(newPrompt, cmd_line);
  strcat(newPrompt, "> ");
  this->newPrompt = newPrompt;
}

void ChangePromptCommand::execute() {
  SmallShell& smash = SmallShell::getInstance();
  smash.prompt = this->newPrompt;
}

ChangeDirCommand::ChangeDirCommand(const string &cmd_line) : BuiltInCommand(cmd_line.c_str()) {

}

void ChangeDirCommand::execute() {

  char* arguments[21];

  int numArgs = _parseCommandLine(commandText.c_str(), arguments);

    SmallShell &shell = SmallShell::getInstance();
    string targetPath = "";

    //more than one target path
    if (numArgs > 2) {
        cerr << "smash error: cd: too many arguments" << endl;
        return;
    }

    //no specifying what we want to change directory to
    //TODO DOUBLE CHECK THIS ERROR MESSAGE
    if (numArgs == 1) {
        cerr << "smash error:> \"" << commandLine << "\"" << endl;
        return;
    }

    //if we are here then there is exactly one argument (aside from command itself)
    //special case of this command

    if (!strcmp(arguments[1],"-")) {
        //first time calling cd and we use -
        if (shell.getPrevDir().empty()) {
            cerr << "smash error: cd: OLDPWD not set" << endl;
            return;
        } else {
            //correct use of cd -
            targetPath = shell.getPrevDir();
        }

    }
    else {
        //exactly one path provided, and no use of cd-
        targetPath = arguments[1];
    }

    const char *targetPathCStr = targetPath.c_str();
    if (chdir(targetPathCStr)) {
        perror("smash error: chdir failed");
        return;
    }

    string currentPath = getCurrentPath();
    if (!currentPath.empty())
        shell.setPath(currentPath);

}


BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line){

}

ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {

}

void ShowPidCommand::execute() {
    //any arguments need to be ignored - not sure if im doing this
    SmallShell &smash = SmallShell::getInstance();
    cout << "smash pid is " << smash.getProcessId() << endl;
}

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {

}

void GetCurrDirCommand::execute() {
    //any arguments should be ignored
    string current_path = getCurrentPath();
    if (!current_path.empty()) {
        cout << current_path << endl;
    }
}

KillCommand::KillCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {
}

void KillCommand::execute() {
  char* arguments[21];
  int numArgs = _parseCommandLine(commandText.c_str(), arguments);
  if ((numArgs < 3 )) {
    cerr << "smash error: kill: invalid arguments" << endl;
    return;
  }
  
  if  (!isInteger(arguments[2])) {
    cerr << "smash error: kill: invalid arguments" << endl;
    return;
  }

  char* unused_ptr;
  int jobId = strtol(arguments[2], &unused_ptr,10);
  int signal = strtol(arguments[1] + 1, &unused_ptr,10);

  SmallShell& smash = SmallShell::getInstance();

  JobsList::JobEntry* job = smash.jobsList.getJobById(jobId);
  if (job == nullptr) {
    string error("smash error: kill: job-id ");
    error.append(to_string(jobId));
    error.append(" does not exist");
    cerr << error.c_str() << endl;
    return;
  }
  if ((numArgs > 3) || (arguments[1][0] != '-') || !isInteger(arguments[1] + 1)) {
      cerr << "smash error: kill: invalid arguments" << endl;
      return;
  }

  if (kill(job->pid, signal) != 0)
    perror("smash error: kill failed");
  else
    cout << "signal number " << signal << " was sent to pid " << job->pid << endl;

}

ForegroundCommand::ForegroundCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {
  
}

void ForegroundCommand::execute() {

  char* arguments[21];
  int jobId;
  int numArgs = _parseCommandLine(commandText.c_str(), arguments);
  SmallShell& smash = SmallShell::getInstance();
  
  if (numArgs == 1 && (smash.jobsList.jobs.size() == 0)) {
    cerr << "smash error: fg: jobs list is empty" << endl;
    return;
  }

  char* unused_ptr;
  JobsList::JobEntry* job;
  if (numArgs == 1) {
      jobId = smash.jobsList.highestJobId;
      PRINT("got jobId " << jobId )
      job = smash.jobsList.getJobById(jobId);
  }
  else if ((numArgs < 2) || !isInteger(arguments[1])){
      cerr << "smash error: fg: invalid arguments" << endl;
    return;
  }
  else
    {
        jobId=strtol(arguments[1], &unused_ptr,10);
    job = smash.jobsList.getJobById(jobId);
    }

  if (job == nullptr) {
    string error("smash error: fg: job-id ");
    error.append(arguments[1]);
    error.append(" does not exist");
    cerr << error.c_str() << endl;
    return;
  }

    if (numArgs > 2){
        cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }
  
  cout << job->command->commandText << " " << job->command->pid << endl;
    PRINT(" before current pid")
  smash.setCurrentForegroundCommandPid(job->command->pid);

  int status;
    PRINT(" before wait")
  waitpid(job->command->pid, &status, WUNTRACED);
  //TODO: CHECK WHAT HAPPENS WHEN fg then ctrl z


    PRINT(" before removing")
    PRINT(" removing job " << jobId)
  smash.jobsList.removeJobById(jobId);
    PRINT(" after removing");
}

JobsCommand::JobsCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {

}

void JobsCommand::execute() {
  SmallShell& smash = SmallShell::getInstance();
  smash.jobsList.removeFinishedJobs();
  smash.jobsList.printJobsList();
}

QuitCommand::QuitCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {

}

void QuitCommand::execute() {
  SmallShell& smash = SmallShell::getInstance();

  char* arguments[21];
  _parseCommandLine(commandText.c_str(), arguments);
  if (arguments[1] && (strcmp(arguments[1], "kill") == 0)) {
    smash.jobsList.killAllJobs();
  }

  
  exit(0);
}


RedirectionCommand::RedirectionCommand(const char* cmd_line) : Command(cmd_line) {
  
}

/*void RedirectionCommand::execute() {
  SmallShell& smash = SmallShell::getInstance();

  char* arguments[21];
  int numArgs = _parseCommandLine(commandText.c_str(), arguments);

  if (commandText.find(">>") != std::string::npos) {

  }
  else { // ">"
    string command = commandText.substr(0, commandText.find(">"));

    std::ofstream fileOut(arguments[numArgs - 1]);

    shared_ptr<Command> cmd = smash.CreateCommand(command.c_str());
    cmd->execute();
  }

}*/

void RedirectionCommand::execute()
{
  const int CREATION_MODE = 0655;
  SmallShell& smash = SmallShell::getInstance();

  char* arguments[21];
  int numArgs = _parseCommandLine(commandText.c_str(), arguments);

  int outfdcriptor;
  if (commandText.find(">>") != std::string::npos)
  {
    outfdcriptor = open(arguments[numArgs - 1], O_WRONLY | O_CREAT | O_APPEND, CREATION_MODE);
  }
  else
  {
    outfdcriptor = open(arguments[numArgs - 1], O_WRONLY | O_CREAT | O_TRUNC, CREATION_MODE);
  }

  if (outfdcriptor == -1)
  {
      perror("smash error: open failed");
      return;
  }

  if ((dup2(STDOUT_FILENO, outfdcriptor + 1) == -1) || dup2(outfdcriptor, STDOUT_FILENO) == -1)
  {
      perror("smash error: dup2 failed");
      exit(0);
  }


  string command = commandText.substr(0, commandText.find(">"));
  smash.executeCommand(command.c_str());


  if (dup2(outfdcriptor + 1, STDOUT_FILENO) == -1)
  {
      perror("smash error: dup2 failed");
      exit(0);
  }

  if ((close(outfdcriptor) == -1) || (close(outfdcriptor + 1) == -1))
  {
      perror("smash error: close failed");
  }

}


PipeCommand::PipeCommand(const char *cmd_line) : Command(cmd_line) {
}

void PipeCommand::execute() {
    SmallShell &shell = SmallShell::getInstance();
    int fd[2];

    string dishmiminator = commandText.find("|&") == std::string::npos ? "|" : "|&";
    string secondCommand;
    string firstCommand = commandText.substr(0, commandText.find(dishmiminator));
    if (dishmiminator == "|&") {
        secondCommand = commandText.substr(commandText.find(dishmiminator) + 2, commandText.length());
    } else {
        secondCommand = commandText.substr(commandText.find(dishmiminator) + 1, commandText.length());
    }

    if   (pipe(fd) == -1)
    {
          perror("smash error: close failed");
    }

    pid_t  inputPid = fork(), outputPid;

    if (inputPid == -1)
    {
        perror("smash error: fork failed");
        if (close(fd[0]) == -1)
        {
            perror("smash error: close failed");
        }
        if (close(fd[1]) == -1)
        {
             perror("smash error: close failed");
        }
        return;
    }

    if (inputPid == 0)
    {

        if  (setpgrp() == -1)
        {
            perror("smash error: setpgrp failed");
            if   (close(fd[0]) == -1)
            {
                 perror("smash error: close failed");
            }
            while(true) {
                break;
            }

            if  (close(fd[1]) ==   -1)
            {
                 perror("smash error: close failed");
            }
            return;
        }

        if  (dishmiminator == "|") // if this
        {
            if (dup2(fd[1], 1)   == -1)
            {
                while(true) {
                    break;
                }
                perror("smash error: dup2 failed");
                if   (close(fd[0])   == -1)
                {
                    perror("smash error: close failed");
                }
                while(true) {
                    break;
                }
                if (close(fd[1]) ==   -1)
                {
                    perror("smash error: close failed");
                }
                return;
            }
        } else { //otherwise

            if (dup2(fd[1], 2) == -1) // if this
            {

                perror("smash error: dup2 failed");
                if (close(fd[0]) == -1)
                {
                    perror("smash error: close failed");
                }
                while(true) {
                    break;
                }
                if (close(fd[1]) == -1)
                {
                    perror("smash error: close failed");
                }
                return;
            }
        }

        if (close(fd[1]) == -1)
        {
            while(true) {
                break;
            }
            perror("smash error: close failed");
        }

        if (close(fd[0]) == -1)
        {
            while(true) {
                break;
            }
            perror("smash error: close failed");
        }

        shell.executeCommand(firstCommand.c_str());
        exit(0);
    }
    outputPid = fork(); // do a fork please

    if (outputPid == -1) { // mucho gracias achi
        perror("smash error: fork failed");
        while(true) {
            break;
        }
        if (close(fd[0]) == -1)
        {
            perror("smash error: close failed");
        }
        if (close(fd[1]) == -1)
        {
            while(true) {
                break;
            }
            perror("smash error: close failed");
        }
        return;
    }
    while(true) {
        break;
    }
    if (outputPid == 0)
    { //second son (he was a mistake)
        if (setpgrp() == -1)
        {
            perror("smash error: setpgrp failed");
            if (close(fd[0]) == -1)
            {
                while(true) {
                    break;
                }
                perror("smash error: close failed");
            }
            if (close(fd[1]) == -1)
            {
                perror("smash error: close failed");
            }
            return;
        }
        if (dup2(fd[0], 0) == -1)
        {
            perror("smash error: dup2 failed");
            if (close(fd[0]) == -1)
            {
                perror("smash error: close failed");
            }
            if (close(fd[1]) == -1)
            {
                perror("smash error: close failed");
                while(true) {
                    break;
                }
            }
            return;
        }
        while(true) {
            break;
        }
        if (close(fd[1]) == -1)
        {
            perror("smash error: close failed");
        }

        if (close(fd[0]) == -1)
        {
            perror("smash error: close failed");
        }

        shell.executeCommand(secondCommand.c_str());
        exit(0);
    }
    if (close(fd[0]) == -1)
    {
         perror("smash error: close failed");
    }
    if (close(fd[1]) == -1)
    {
        perror("smash error: close failed");
    }
    if (waitpid(inputPid,nullptr, WUNTRACED) == -1) {
        perror("smash error: waitpid failed");
        return;
    }
    if (waitpid(outputPid,nullptr, WUNTRACED) == -1) {
        perror("smash error: waitpid failed");
        return;
    }
}

ChmodCommand::ChmodCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {

}

void ChmodCommand::execute() {
  char* arguments[21];
  int numArgs = _parseCommandLine(commandText.c_str(), arguments);

  if (numArgs != 3 || (arguments[1] && !isInteger(arguments[1]))) {
    cerr << "smash error: chmod: invalid arguments" << endl;
    return;
  }

  chmod(arguments[2], stoi(string(arguments[1]), nullptr, 10));
  
}


ExternalCommand::ExternalCommand(const char* cmd_line) : Command(cmd_line) {

}

Command::Command(char const* cmd_line) {
  string newString(cmd_line);
  commandText = newString;
}

Command::~Command() {
  
}

void ExternalCommand::execute() {
    safeSetPgrp();
}


//Jobs List


JobsList::JobsList () {

}

JobsList::~JobsList () {
  
}

void JobsList::addJob(std::shared_ptr<Command> cmd, int pid, bool isStopped) {
    removeFinishedJobs();
  int jobId = highestJobId + 1;
  highestJobId++;
  this->jobs.push_back(JobEntry(cmd, jobId, pid, isStopped));
}

JobsList::JobEntry::JobEntry(std::shared_ptr<Command> cmd, int jobId, int pid, bool isStopped)
    : jobId(jobId), pid(pid), isStopped(isStopped), command(cmd) {
}

void JobsList::printJobsList() {
  for(JobEntry job : jobs) {
    std::cout << "[" << job.jobId << "]" <<  " " << job.command->commandText << std::endl;
  }
}
void JobsList::killAllJobs(){
  removeFinishedJobs();

  SmallShell& smash = SmallShell::getInstance();

  std::cout << "smash: sending SIGKILL signal to " << smash.jobsList.jobs.size() << " jobs:" << std::endl;

  auto it =  jobs.begin();
  while(it != jobs.end()) {
    cout << it->pid << ": " << it->command->commandText << std::endl;
    if (kill(it->pid, SIGKILL) == -1){
        perror("smash error: kill failed");
    }
    ++it; 
  }
}
void JobsList::removeFinishedJobs(){
  int* const status = nullptr;
  auto it =  jobs.begin();

  while(it != jobs.end()) {

    int result = waitpid(it->pid, status, WNOHANG);

    if (result == -1) {
        perror("smash error: waitpid failed");
    }
    else if (result == 0) {
        it++;
    }
    else {
        it = jobs.erase(it);
    }
  }

    int newHighestId = 0;
    for (JobEntry job : jobs) {
        if (job.jobId > newHighestId)
            newHighestId = job.jobId;
    }
    highestJobId = newHighestId;
}

void JobsList::removeJobById(int jobId){
    PRINT( " removing")
  auto it =  jobs.begin();

  while(it != jobs.end()) {
      PRINT(" checking " << it->jobId );
    if (it->jobId == jobId) {
        PRINT(" found " );
      jobs.erase(it);
        PRINT(" deleted ");
        removeFinishedJobs();
        return;
    }
    ++it; 
  }


}

JobsList::JobEntry* JobsList::getJobById(int jobId) {
    removeFinishedJobs();
  auto it =  jobs.begin();

  while(it != jobs.end()) {
    if (it->jobId == jobId) {
      return &(*it);
    }
    ++it; 
  }
  return nullptr;
}
