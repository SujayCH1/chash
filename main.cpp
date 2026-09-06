#include<iostream>
#include<string>
#include<unistd.h>
#include "executor.h"
#include "interpreter.h"
#include "prompt.h"
#include "signals.h"

int main() {

    std:: string input;
    int lastStatus = 0;
    ShellContext context{
        isatty(STDIN_FILENO) != 0,
        getpgrp(),
        JobTable{},
        CommandHistory{}
    };

    if(!initializeHistory(context.history)) {
        std::cerr << "csh: could not initialize command history\n";
    }

    if(context.interactive && !configureShellSignals()) {
        perror("sigaction");
        return 1;
    }

    while(1) {
        updateBackgroundJobs(context);
        reportJobChanges(context);

        if(context.interactive) {
            printPrompt();
        }

        if(!std::getline(std::cin, input)) {
            if(context.interactive && consumeShellInterrupt()) {
                std::cin.clear();
                lastStatus = 130;
                continue;
            }

            if(context.interactive) {
                std::cout << '\n';
            }

            shutdownJobs(context);

            if(!saveHistory(context.history)) {
                std::cerr << "csh: could not save command history\n";
            } else if(!pruneHistoryFile(context.history)) {
                std::cerr << "csh: could not prune command history\n";
            }

            break;
        }

        addHistoryEntry(context.history, input);

        updateBackgroundJobs(context);
        reportJobChanges(context);

        ShellResult result = interpretCommands(input, lastStatus, context);
        lastStatus = result.status;

        if(result.shouldExit) {
            shutdownJobs(context);

            if(!saveHistory(context.history)) {
                std::cerr << "csh: could not save command history\n";
            } else if(!pruneHistoryFile(context.history)) {
                std::cerr << "csh: could not prune command history\n";
            }

            return result.status;
        }
    }

    return lastStatus;

}
