#include<algorithm>
#include<sys/wait.h>
#include "job_table.h"

Job& JobTable::addJob(pid_t pgid,
                      const std::vector<pid_t>& processIds,
                      const std::string& command,
                      bool background) {

    std::vector<ProcessInfo> processes;
    processes.reserve(processIds.size());

    for(pid_t pid : processIds) {
        processes.push_back({pid, ProcessState::Running, 0});
    }

    int jobId = 0;

    if(background) {
        jobId = nextJobId;
        nextJobId++;
    }

    jobs.push_back({
        jobId,
        pgid,
        processes,
        command,
        JobState::Running,
        background,
        false
    });

    return jobs.back();
}

Job* JobTable::findById(int jobId) {

    for(Job& job : jobs) {
        if(job.id == jobId) {
            return &job;
        }
    }

    return nullptr;
}

Job* JobTable::findByPid(pid_t pid) {

    for(Job& job : jobs) {
        for(const ProcessInfo& process : job.processes) {
            if(process.pid == pid) {
                return &job;
            }
        }
    }

    return nullptr;
}

Job* JobTable::findByPgid(pid_t pgid) {

    for(Job& job : jobs) {
        if(job.pgid == pgid) {
            return &job;
        }
    }

    return nullptr;
}

int JobTable::assignJobId(Job& job) {

    if(job.id == 0) {
        job.id = nextJobId;
        nextJobId++;
    }

    return job.id;
}

void JobTable::markJobRunning(Job& job) {

    for(ProcessInfo& process : job.processes) {
        if(process.state == ProcessState::Stopped) {
            process.state = ProcessState::Running;
        }
    }

    job.state = JobState::Running;
    job.stateChanged = false;
}

bool JobTable::updateProcess(pid_t pid, int status) {

    Job* job = findByPid(pid);

    if(job == nullptr) {
        return false;
    }

    for(ProcessInfo& process : job->processes) {
        if(process.pid != pid) {
            continue;
        }

        process.status = status;

        if(WIFEXITED(status) || WIFSIGNALED(status)) {
            process.state = ProcessState::Completed;
        } else if(WIFSTOPPED(status)) {
            process.state = ProcessState::Stopped;
        } else if(WIFCONTINUED(status)) {
            process.state = ProcessState::Running;
        }

        break;
    }

    updateJobState(*job);
    return true;
}

void JobTable::removeJob(int jobId) {

    jobs.erase(
        std::remove_if(
            jobs.begin(),
            jobs.end(),
            [jobId](const Job& job) {
                return job.id == jobId;
            }
        ),
        jobs.end()
    );
}

std::vector<Job>& JobTable::allJobs() {

    return jobs;
}

const std::vector<Job>& JobTable::allJobs() const {

    return jobs;
}

void JobTable::updateJobState(Job& job) {

    JobState previousState = job.state;
    bool allCompleted = true;
    bool anyRunning = false;

    for(const ProcessInfo& process : job.processes) {
        if(process.state != ProcessState::Completed) {
            allCompleted = false;
        }

        if(process.state == ProcessState::Running) {
            anyRunning = true;
        }
    }

    if(allCompleted) {
        job.state = JobState::Done;
    } else if(anyRunning) {
        job.state = JobState::Running;
    } else {
        job.state = JobState::Stopped;
    }

    if(job.state != previousState) {
        job.stateChanged = true;
    }
}
