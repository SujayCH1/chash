#ifndef SHELL_CONTEXT_H
#define SHELL_CONTEXT_H

#include<sys/types.h>
#include "job_table.h"

struct ShellContext {
    bool interactive;
    pid_t shellPgid;
    JobTable jobs;
};

#endif
