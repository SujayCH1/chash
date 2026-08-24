#include<iostream>
#include<string>
#include<unistd.h>
#include<vector>
#include<sys/wait.h>
#include<limits.h>
#include "helper.h"
#include "map.h"

int main() {

    std:: string input;
    std:: string output;
    std:: vector<std::string> argv;
    std::vector<char*> exec_args;

    setBuiltinRegistry();

    while(1) {
        pid_t pid;
        int status = 0;

        char currentDir[PATH_MAX];


        // std::cout<<"csh >";
        printPrompt();

        if(!std::getline(std::cin, input)) {
            std::cout << '\n';
            break;
        }

        argv = parseInput(input);

        if(argv.empty()) {
            continue;
        }

        if(executeBuiltin(argv)) {
            continue;
        }

        exec_args = convertArgs(argv);
        exec_args.push_back(nullptr);

        switch(pid = fork()) {

            case -1:
                perror("fork");
                return 1;

            case 0:
                execvp(exec_args[0], exec_args.data());
                if(errno) {
                    perror(exec_args[0]);
                    status = errno;
                    return status;
                }
                return status;

            default:
                wait(&status);

        }

    }

    return 0;

}
