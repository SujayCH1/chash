#include<iostream>
#include<vector>
#include<array>
#include<string>
#include<cerrno>
#include<limits>
#include<fcntl.h>
#include<signal.h>
#include<unistd.h>
#include<sys/wait.h>
#include "executor.h"
#include "builtins.h"
#include "signals.h"

namespace {

std::vector<char*> convertToCharPtrArr(std::vector<std::string>& args) {

    std::vector<char*> execArgs;
    execArgs.reserve(args.size() + 1);

    for(std::string& argument : args) {
        execArgs.push_back(argument.data());
    }

    execArgs.push_back(nullptr);
    return execArgs;
}

void closePipes(const std::vector<std::array<int, 2>>& pipes) {

    for(const std::array<int, 2>& pipefd : pipes) {
        close(pipefd[0]);
        close(pipefd[1]);
    }
}

bool applyRedirections(const std::vector<Redirection>& redirections) {

    for(const Redirection& redirection : redirections) {
        if(redirection.type == RedirectionType::Close) {
            if(close(redirection.targetFd) == -1 && errno != EBADF) {
                perror("close");
                return false;
            }

            continue;
        }

        if(redirection.type == RedirectionType::Duplicate) {
            if(dup2(redirection.sourceFd, redirection.targetFd) == -1) {
                perror("dup2");
                return false;
            }

            continue;
        }

        int fd;

        if(redirection.type == RedirectionType::Input) {
            fd = open(redirection.filename.c_str(), O_RDONLY);
        } else if(redirection.type == RedirectionType::Output) {
            fd = open(redirection.filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        } else if(redirection.type == RedirectionType::Append) {
            fd = open(redirection.filename.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0666);
        } else {
            fd = open(redirection.filename.c_str(), O_RDWR | O_CREAT, 0666);
        }

        if(fd == -1) {
            perror(redirection.filename.c_str());
            return false;
        }

        if(dup2(fd, redirection.targetFd) == -1) {
            perror("dup2");

            if(fd != redirection.targetFd) {
                close(fd);
            }

            return false;
        }

        if(fd != redirection.targetFd) {
            close(fd);
        }
    }

    return true;
}

struct SavedDescriptor {
    int target;
    int saved;
    bool wasOpen;
};

bool saveDescriptors(const std::vector<Redirection>& redirections,
                     std::vector<SavedDescriptor>& savedDescriptors) {

    int minimumSavedFd = 10;

    for(const Redirection& redirection : redirections) {
        if(redirection.targetFd == std::numeric_limits<int>::max()) {
            std::cerr << "csh: file descriptor number is too large\n";
            return false;
        }

        if(redirection.targetFd >= minimumSavedFd) {
            minimumSavedFd = redirection.targetFd + 1;
        }
    }

    for(const Redirection& redirection : redirections) {
        bool alreadySaved = false;

        for(const SavedDescriptor& descriptor : savedDescriptors) {
            if(descriptor.target == redirection.targetFd) {
                alreadySaved = true;
                break;
            }
        }

        if(alreadySaved) {
            continue;
        }

        int saved = fcntl(redirection.targetFd, F_DUPFD_CLOEXEC, minimumSavedFd);

        if(saved == -1) {
            if(errno == EBADF) {
                savedDescriptors.push_back({redirection.targetFd, -1, false});
                continue;
            }

            perror("fcntl");

            for(const SavedDescriptor& descriptor : savedDescriptors) {
                if(descriptor.wasOpen) {
                    close(descriptor.saved);
                }
            }

            savedDescriptors.clear();
            return false;
        }

        savedDescriptors.push_back({redirection.targetFd, saved, true});
    }

    return true;
}

bool restoreDescriptors(std::vector<SavedDescriptor>& savedDescriptors) {

    bool restored = true;

    for(auto descriptor = savedDescriptors.rbegin(); descriptor != savedDescriptors.rend(); descriptor++) {
        if(descriptor->wasOpen) {
            if(dup2(descriptor->saved, descriptor->target) == -1) {
                perror("dup2");
                restored = false;
            }

            close(descriptor->saved);
        } else if(close(descriptor->target) == -1 && errno != EBADF) {
            perror("close");
            restored = false;
        }
    }

    return restored;
}

ShellResult runCommandInParent(const Command& command, ShellContext& context) {

    bool assignment = !command.args.empty() && isVariableAssignment(command.args[0]);

    if(command.redirections.empty()) {
        return assignment
            ? executeVariableAssignments(command.args)
            : executeBuiltin(command.args, context);
    }

    std::cout.flush();
    std::cerr.flush();

    std::vector<SavedDescriptor> savedDescriptors;

    if(!saveDescriptors(command.redirections, savedDescriptors)) {
        return {1, false};
    }

    if(!applyRedirections(command.redirections)) {
        restoreDescriptors(savedDescriptors);
        return {1, false};
    }

    ShellResult result = assignment
        ? executeVariableAssignments(command.args)
        : executeBuiltin(command.args, context);

    std::cout.flush();
    std::cerr.flush();

    if(!restoreDescriptors(savedDescriptors)) {
        result.status = 1;
    }

    return result;
}

pid_t waitForChild(pid_t child, int* status) {

    pid_t result;

    do {
        result = waitpid(child, status, 0);
    } while(result == -1 && errno == EINTR);

    return result;
}

bool setTerminalForegroundGroup(pid_t processGroup) {

    struct sigaction ignoreAction {};
    struct sigaction previousAction {};
    ignoreAction.sa_handler = SIG_IGN;
    sigemptyset(&ignoreAction.sa_mask);

    if(sigaction(SIGTTOU, &ignoreAction, &previousAction) == -1) {
        perror("sigaction");
        return false;
    }

    bool success = true;

    if(tcsetpgrp(STDIN_FILENO, processGroup) == -1) {
        perror("tcsetpgrp");
        success = false;
    }

    if(sigaction(SIGTTOU, &previousAction, nullptr) == -1) {
        perror("sigaction");
        success = false;
    }

    return success;
}

struct LaunchedPipeline {
    bool success;
    pid_t pgid;
    std::vector<pid_t> children;
};

std::string describePipeline(const Pipeline& pipeline) {

    std::string description;

    for(std::size_t i = 0; i < pipeline.commands.size(); i++) {
        if(i > 0) {
            description.append(" | ");
        }

        const Command& command = pipeline.commands[i];

        for(std::size_t j = 0; j < command.args.size(); j++) {
            if(j > 0) {
                description.push_back(' ');
            }

            description.append(command.args[j]);
        }
    }

    return description;
}

LaunchedPipeline launchPipeline(Pipeline& pipeline, ShellContext& context) {

    std::vector<std::array<int, 2>> pipes(pipeline.commands.size() - 1);

    for(std::size_t i = 0; i < pipes.size(); i++) {
        if(pipe(pipes[i].data()) == -1) {
            perror("pipe");

            for(std::size_t j = 0; j < i; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            return {false, 0, {}};
        }
    }

    std::vector<pid_t> children;
    children.reserve(pipeline.commands.size());

    pid_t pipelinePgid = 0;

    for(std::size_t i = 0; i < pipeline.commands.size(); i++) {
        pid_t pid = fork();

        if(pid == -1) {
            perror("fork");
            closePipes(pipes);

            if(pipelinePgid != 0) {
                kill(-pipelinePgid, SIGTERM);
            }

            for(pid_t child : children) {
                waitForChild(child, nullptr);
            }

            return {false, 0, {}};
        } else if(pid == 0) {
            pid_t childPgid;

            if(pipelinePgid == 0) {
                childPgid = getpid();
            } else {
                childPgid = pipelinePgid;
            }

            if(setpgid(0, childPgid) == -1) {
                perror("setpgid");
                _exit(1);
            }

            if(!configureChildSignals()) {
                perror("sigaction");
                _exit(1);
            }

            if(i > 0 && dup2(pipes[i - 1][0], STDIN_FILENO) == -1) {
                perror("dup2");
                _exit(1);
            }

            if(i + 1 < pipeline.commands.size() && dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                perror("dup2");
                _exit(1);
            }

            if(!applyRedirections(pipeline.commands[i].redirections)) {
                _exit(1);
            }

            closePipes(pipes);

            if(isBuiltin(pipeline.commands[i].args)) {
                ShellResult result = executeBuiltin(pipeline.commands[i].args, context);
                std::cout.flush();
                std::cerr.flush();
                _exit(result.status);
            }

            if(!pipeline.commands[i].args.empty() &&
               isVariableAssignment(pipeline.commands[i].args[0])) {
                ShellResult result = executeVariableAssignments(pipeline.commands[i].args);
                std::cout.flush();
                std::cerr.flush();
                _exit(result.status);
            }

            std::vector<char*> execArgs = convertToCharPtrArr(pipeline.commands[i].args);

            execvp(execArgs[0], execArgs.data());
            int error = errno;
            perror(execArgs[0]);
            _exit(error == ENOENT ? 127 : 126);
        } else {
            if(pipelinePgid == 0) {
                pipelinePgid = pid;
            }

            if(setpgid(pid, pipelinePgid) == -1 && errno != EACCES && errno != ESRCH) {
                perror("setpgid");
            }

            children.push_back(pid);
        }
    }

    closePipes(pipes);
    return {true, pipelinePgid, children};
}

}

void updateBackgroundJobs(ShellContext& context) {

    while(true) {
        int status;
        pid_t pid = waitpid(
            -1,
            &status,
            WNOHANG | WUNTRACED | WCONTINUED
        );

        if(pid > 0) {
            context.jobs.updateProcess(pid, status);
            continue;
        }

        if(pid == 0) {
            break;
        }

        if(errno == EINTR) {
            continue;
        }

        if(errno != ECHILD) {
            perror("waitpid");
        }

        break;
    }
}

void reportJobChanges(ShellContext& context) {

    std::vector<int> completedJobIds;

    for(Job& job : context.jobs.allJobs()) {
        if(job.id == 0 || !job.stateChanged) {
            continue;
        }

        std::cout << '[' << job.id << "] ";

        if(job.state == JobState::Running) {
            std::cout << "Running";
        } else if(job.state == JobState::Stopped) {
            std::cout << "Stopped";
        } else {
            std::cout << "Done";
        }

        std::cout << "  " << job.command << '\n';
        job.stateChanged = false;

        if(job.state == JobState::Done) {
            completedJobIds.push_back(job.id);
        }
    }

    if(!completedJobIds.empty()) {
        std::cout << std::flush;
    }

    for(int jobId : completedJobIds) {
        context.jobs.removeJob(jobId);
    }
}

void shutdownJobs(ShellContext& context) {

    updateBackgroundJobs(context);

    std::size_t activeJobs = 0;

    for(const Job& job : context.jobs.allJobs()) {
        if(job.state != JobState::Done) {
            activeJobs++;
        }
    }

    if(activeJobs == 0) {
        return;
    }

    std::cerr << "csh: terminating " << activeJobs << " active ";

    if(activeJobs == 1) {
        std::cerr << "job\n";
    } else {
        std::cerr << "jobs\n";
    }

    for(const Job& job : context.jobs.allJobs()) {
        if(job.state == JobState::Done) {
            continue;
        }

        if(kill(-job.pgid, SIGHUP) == -1 && errno != ESRCH) {
            perror("SIGHUP");
        }

        if(job.state == JobState::Stopped) {
            if(kill(-job.pgid, SIGCONT) == -1 && errno != ESRCH) {
                perror("SIGCONT");
            }
        }
    }

    updateBackgroundJobs(context);
}

ShellResult moveJobToForeground(Job& job, ShellContext& context) {

    bool terminalTransferred = false;

    if(context.interactive) {
        terminalTransferred = setTerminalForegroundGroup(job.pgid);
    }

    if(job.state == JobState::Stopped) {
        if(kill(-job.pgid, SIGCONT) == -1) {
            perror("fg");

            if(terminalTransferred) {
                setTerminalForegroundGroup(context.shellPgid);
            }

            return {1, false};
        }

        context.jobs.markJobRunning(job);
    }

    job.background = false;
    job.stateChanged = false;

    int lastStatus = 0;
    bool lastCommandWasSignaled = false;

    while(job.state == JobState::Running) {
        int status;
        pid_t pid;

        do {
            pid = waitpid(
                -job.pgid,
                &status,
                WUNTRACED | WCONTINUED
            );
        } while(pid == -1 && errno == EINTR);

        if(pid == -1) {
            perror("waitpid");
            lastStatus = 1;
            break;
        }

        context.jobs.updateProcess(pid, status);
    }

    const ProcessInfo& lastProcess = job.processes.back();

    if(lastProcess.state == ProcessState::Completed) {
        if(WIFEXITED(lastProcess.status)) {
            lastStatus = WEXITSTATUS(lastProcess.status);
        } else if(WIFSIGNALED(lastProcess.status)) {
            lastStatus = 128 + WTERMSIG(lastProcess.status);
            lastCommandWasSignaled = true;
        }
    } else if(lastProcess.state == ProcessState::Stopped) {
        lastStatus = 128 + WSTOPSIG(lastProcess.status);
    }

    if(terminalTransferred) {
        if(!setTerminalForegroundGroup(context.shellPgid)) {
            lastStatus = 1;
        }
    }

    if(context.interactive && lastCommandWasSignaled) {
        std::cout << '\n';
    }

    if(job.state == JobState::Stopped) {
        if(context.interactive) {
            std::cout << '\n';
        }

        context.jobs.assignJobId(job);
        job.background = true;
        job.stateChanged = false;
        std::cout << '[' << job.id << "] Stopped  " << job.command << '\n';
    } else if(job.state == JobState::Done) {
        context.jobs.removeJob(job.id);
    }

    return {lastStatus, false};
}

ShellResult executePipeline(Pipeline& pipeline, ShellContext& context) {

    if(pipeline.commands.empty()) {
        return {0, false};
    }

    if(!pipeline.background &&
       pipeline.commands.size() == 1 &&
       (isBuiltin(pipeline.commands[0].args) ||
        (!pipeline.commands[0].args.empty() && isVariableAssignment(pipeline.commands[0].args[0])))) {
        return runCommandInParent(pipeline.commands[0], context);
    }

    LaunchedPipeline launched = launchPipeline(pipeline, context);

    if(!launched.success) {
        return {1, false};
    }

    Job& job = context.jobs.addJob(
        launched.pgid,
        launched.children,
        describePipeline(pipeline),
        pipeline.background
    );

    if(pipeline.background) {
        std::cout << '[' << job.id << "] " << job.pgid << '\n' << std::flush;
        return {0, false};
    }

    return moveJobToForeground(job, context);
}
