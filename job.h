#ifndef JOB_H
#define JOB_H

#include<string>
#include<vector>
#include<sys/types.h>

enum class ProcessState {
    Running,
    Stopped,
    Completed
};

struct ProcessInfo {
    pid_t pid;
    ProcessState state;
    int status;
};

enum class JobState {
    Running,
    Stopped,
    Done
};

struct Job {
    int id;
    pid_t pgid;
    std::vector<ProcessInfo> processes;
    std::string command;
    JobState state;
    bool background;
    bool stateChanged;
};

#endif
