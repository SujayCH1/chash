#include<cerrno>
#include<csignal>
#include<unistd.h>
#include "signals.h"

namespace {

volatile sig_atomic_t shellInterrupted = 0;

void handleInterrupt(int) {

    shellInterrupted = 1;

    const char newline = '\n';
    int previousError = errno;
    write(STDOUT_FILENO, &newline, 1);
    errno = previousError;
}

bool setSignalDisposition(int signalNumber, void (*handler)(int)) {

    struct sigaction action {};
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    return sigaction(signalNumber, &action, nullptr) != -1;
}

}

bool configureShellSignals() {

    if(!setSignalDisposition(SIGINT, handleInterrupt)) {
        return false;
    }

    if(!setSignalDisposition(SIGQUIT, SIG_IGN)) {
        return false;
    }

    if(!setSignalDisposition(SIGTSTP, SIG_IGN)) {
        return false;
    }

    if(!setSignalDisposition(SIGTTIN, SIG_IGN)) {
        return false;
    }

    if(!setSignalDisposition(SIGTTOU, SIG_IGN)) {
        return false;
    }

    return true;
}

bool configureChildSignals() {

    const int defaultSignals[] = {
        SIGINT,
        SIGQUIT,
        SIGTSTP,
        SIGTTIN,
        SIGTTOU
    };

    for(int signalNumber : defaultSignals) {
        if(!setSignalDisposition(signalNumber, SIG_DFL)) {
            return false;
        }
    }

    return true;
}

bool consumeShellInterrupt() {

    if(shellInterrupted == 0) {
        return false;
    }

    shellInterrupted = 0;
    return true;
}
