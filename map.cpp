#include<iostream>
#include<unordered_map>
#include<string>
#include<functional>
#include<unistd.h>
#include<cstdlib>
#include "map.h"

using BuiltinFunction = std::function<int(const std::vector<std::string>&)>;

namespace {

std::unordered_map<std::string, BuiltinFunction> builtinRegistry;

} // namespace

void setBuiltinRegistry()
{
    builtinRegistry["cd"] = changeDirectory;
    builtinRegistry["exit"] = exitProcess;
}

// execute buitlin
bool executeBuiltin(const std::vector<std::string>& args)
{
    auto builtin = builtinRegistry.find(args[0]);

    if(builtin == builtinRegistry.end()) {
        return false;
    }

    builtin->second(args);
    return true;
}

// "cd" wrapper
int changeDirectory (const std::vector<std::string>& args) {

    if(args.size() > 2) {
        std::cerr << "cd: too many arguments\n";
        return 1;
    }

    const char* path;

    if(args.size() == 1) {
        path = std::getenv("HOME");
        
        if(path == nullptr) {
            std::cerr << "cd: HOME is not set\n";
            return 1; 
        }
    } else {
        path = args[1].c_str();
    }

    
    if(chdir(path) == -1) {
        perror("cd");
        return 1;
        
    }

    return 0;

}

// "exit" wrapper
int exitProcess (const std::vector<std::string>& args) {

    if(args.size() > 2) {
        std::cerr << "exit: too many arguments\n";
        return 1;
    }

    int status = 0;
    
    if(args.size() == 2) {
        try {
            status = std::stoi(args[1]);
        } catch (const std:: exception&) {
            std::cerr <<"exit: numeric argument required";
            return 1;
        }
    }

    std::exit(status);
    
}
