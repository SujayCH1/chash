#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "ast.h"
#include "shell_context.h"
#include "shell_result.h"

ShellResult executePipeline(Pipeline& pipeline, const ShellContext& context);

#endif
