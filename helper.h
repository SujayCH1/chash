#ifndef HELPER_H
#define HELPER_H

#include<string>
#include<vector>

std::vector<std::string> parseInput(const std::string& input);

std::vector<char*> convertArgs(std::vector<std::string>& argv);

void printPrompt();

#endif
