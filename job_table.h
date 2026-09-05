#ifndef JOB_TABLE_H
#define JOB_TABLE_H

#include<string>
#include<vector>
#include<sys/types.h>
#include "job.h"

class JobTable {
public:
    Job& addJob(pid_t pgid,
                const std::vector<pid_t>& processIds,
                const std::string& command,
                bool background);

    Job* findById(int jobId);
    Job* findByPid(pid_t pid);
    Job* findByPgid(pid_t pgid);

    int assignJobId(Job& job);
    void markJobRunning(Job& job);
    bool updateProcess(pid_t pid, int status);
    void removeJob(int jobId);

    std::vector<Job>& allJobs();
    const std::vector<Job>& allJobs() const;

private:
    void updateJobState(Job& job);

    std::vector<Job> jobs;
    int nextJobId = 1;
};

#endif
