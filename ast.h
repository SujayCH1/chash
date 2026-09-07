#ifndef AST_H
#define AST_H

#include<string>
#include<vector>

enum class RedirectionType {
    Input,
    Output,
    Append,
    ReadWrite,
    Duplicate,
    Close
};

struct Redirection {
    RedirectionType type;
    int targetFd;
    int sourceFd;
    std::string filename;
};

struct Command {
    std::vector<std::string> args;
    std::vector<Redirection> redirections;
};

enum class ExecutionCondition {
    Always,
    OnSuccess,
    OnFailure
};

struct Pipeline {
    std::vector<Command> commands;
    bool background = false;
    ExecutionCondition condition = ExecutionCondition::Always;
};

struct CommandList {
    std::vector<Pipeline> pipelines;
};

#endif
