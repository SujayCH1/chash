#include<iostream>
#include<limits.h>
#include<unistd.h>
#include "prompt.h"

void printPrompt() {

    char currentDirectory[PATH_MAX];

    if(getcwd(currentDirectory, sizeof(currentDirectory)) != nullptr) {
        std::cout << currentDirectory << " csh> " << std::flush;
    } else {
        perror("getcwd");
        std::cout << "csh> " << std::flush;
    }
}
