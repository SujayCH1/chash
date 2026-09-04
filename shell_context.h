#ifndef SHELL_CONTEXT_H
#define SHELL_CONTEXT_H

#include<sys/types.h>

struct ShellContext {
    bool interactive;
    pid_t shellPgid;
};

#endif
