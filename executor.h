#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "ast.h"
#include "shell_context.h"
#include "shell_result.h"

ShellResult executePipeline(Pipeline& pipeline, ShellContext& context);
ShellResult moveJobToForeground(Job& job, ShellContext& context);
void updateBackgroundJobs(ShellContext& context);
void reportJobChanges(ShellContext& context);
void shutdownJobs(ShellContext& context);

#endif
