#ifndef MAP_H
#define MAP_H

#include <string>
#include <vector>

void setBuiltinRegistry();
bool executeBuiltin(const std::vector<std::string>& args);
int changeDirectory(const std::vector<std::string>& args);
int exitProcess(const std::vector<std::string>& args);

#endif
