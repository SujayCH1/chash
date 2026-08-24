#include<cctype>
#include<vector>
#include<iostream>
#include<limits.h>
#include<unistd.h>
#include "helper.h"


// parser helper functions
std::vector<std::string> parseInput(const std::string& input) {

    std::string temp;
    std::vector<std::string> argv;

    for(char character : input) {
        if(std::isspace(static_cast<unsigned char>(character))) {
            if(!temp.empty()) {
                argv.push_back(temp);
                temp.clear();
            }
        } else {
            temp.push_back(character);
        }
    }

    if(!temp.empty()) {
        argv.push_back(temp);
    }

    return argv;
}

// converts a string vector into a char* vector
std::vector<char*> convertArgs (std::vector<std::string>& argv) {

    std::vector<char*> exec_args;

    for(std::string& argument : argv) {
        exec_args.push_back(argument.data());
    }

    return exec_args;

}

// logic to print zsh prompt with or withour current working directory
void printPrompt()
{
    char currentDirectory[PATH_MAX];

    if(getcwd(currentDirectory, sizeof(currentDirectory)) != nullptr) {
        std::cout << currentDirectory << " csh> ";
    } else {
        perror("getcwd");
        std::cout << "csh> ";
    }
}

// built-in map helper functions
