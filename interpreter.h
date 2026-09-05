#ifndef INTERPRETER_H
#define INTERPRETER_H

#include<string>
#include "shell_context.h"
#include "shell_result.h"

ShellResult interpretCommands(const std::string& input,
                              int lastStatus,
                              ShellContext& context);

#endif
