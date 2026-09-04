#include<iostream>
#include<string>
#include<unistd.h>
#include "interpreter.h"
#include "prompt.h"
#include "signals.h"

int main() {

    std:: string input;
    int lastStatus = 0;
    ShellContext context{
        isatty(STDIN_FILENO) != 0,
        getpgrp()
    };

    if(context.interactive && !configureShellSignals()) {
        perror("sigaction");
        return 1;
    }

    while(1) {
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

            break;
        }

        ShellResult result = interpretCommands(input, lastStatus, context);
        lastStatus = result.status;

        if(result.shouldExit) {
            return result.status;
        }
    }

    return lastStatus;

}
