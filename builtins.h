#ifndef BUILTINS_H
#define BUILTINS_H

#include<string>
#include<vector>
#include "shell_result.h"

bool isBuiltin(const std::vector<std::string>& args);
bool isVariableAssignment(const std::string& value);
ShellResult executeBuiltin(const std::vector<std::string>& args);
ShellResult executeVariableAssignments(const std::vector<std::string>& args);
ShellResult changeDirectory(const std::vector<std::string>& args);
ShellResult exitProcess(const std::vector<std::string>& args);
ShellResult exportVariables(const std::vector<std::string>& args);
ShellResult unsetVariables(const std::vector<std::string>& args);

#endif
