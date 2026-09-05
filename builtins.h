#ifndef BUILTINS_H
#define BUILTINS_H

#include<string>
#include<vector>
#include "shell_context.h"
#include "shell_result.h"

bool isBuiltin(const std::vector<std::string>& args);
bool isVariableAssignment(const std::string& value);
ShellResult executeBuiltin(const std::vector<std::string>& args, ShellContext& context);
ShellResult executeVariableAssignments(const std::vector<std::string>& args);
ShellResult changeDirectory(const std::vector<std::string>& args, ShellContext& context);
ShellResult exitProcess(const std::vector<std::string>& args, ShellContext& context);
ShellResult exportVariables(const std::vector<std::string>& args, ShellContext& context);
ShellResult unsetVariables(const std::vector<std::string>& args, ShellContext& context);
ShellResult listJobs(const std::vector<std::string>& args, ShellContext& context);
ShellResult resumeBackgroundJob(const std::vector<std::string>& args, ShellContext& context);
ShellResult moveForegroundJob(const std::vector<std::string>& args, ShellContext& context);

#endif
