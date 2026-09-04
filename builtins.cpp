#include<iostream>
#include<unordered_map>
#include<string>
#include<functional>
#include<cctype>
#include<limits.h>
#include<unistd.h>
#include<cstdlib>
#include "builtins.h"

namespace {

using BuiltinFunction = std::function<ShellResult(const std::vector<std::string>&)>;

const std::unordered_map<std::string, BuiltinFunction>& builtinRegistry() {

    static const std::unordered_map<std::string, BuiltinFunction> registry = {
        {"cd", changeDirectory},
        {"exit", exitProcess},
        {"export", exportVariables},
        {"unset", unsetVariables}
    };

    return registry;
}

bool validVariableName(const std::string& name) {

    if(name.empty() ||
       (!std::isalpha(static_cast<unsigned char>(name[0])) && name[0] != '_')) {
        return false;
    }

    for(char character : name) {
        if(!std::isalnum(static_cast<unsigned char>(character)) && character != '_') {
            return false;
        }
    }

    return true;
}

bool setVariable(const std::string& assignment) {

    std::size_t separator = assignment.find('=');

    if(separator == std::string::npos) {
        return false;
    }

    std::string name = assignment.substr(0, separator);

    if(!validVariableName(name)) {
        return false;
    }

    if(setenv(name.c_str(), assignment.substr(separator + 1).c_str(), 1) == -1) {
        perror("setenv");
        return false;
    }

    return true;
}

}

bool isBuiltin(const std::vector<std::string>& args) {

    if(args.empty()) {
        return false;
    }

    return builtinRegistry().find(args[0]) != builtinRegistry().end();
}

bool isVariableAssignment(const std::string& value) {

    std::size_t separator = value.find('=');

    return separator != std::string::npos &&
           validVariableName(value.substr(0, separator));
}

ShellResult executeBuiltin(const std::vector<std::string>& args) {

    if(args.empty()) {
        return {1, false};
    }

    auto builtin = builtinRegistry().find(args[0]);

    if(builtin == builtinRegistry().end()) {
        return {127, false};
    }

    return builtin->second(args);
}

ShellResult executeVariableAssignments(const std::vector<std::string>& args) {

    for(const std::string& assignment : args) {
        if(!isVariableAssignment(assignment)) {
            std::cerr << "csh: temporary command environments are not supported\n";
            return {2, false};
        }

        if(!setVariable(assignment)) {
            return {1, false};
        }
    }

    return {0, false};
}

ShellResult changeDirectory (const std::vector<std::string>& args) {

    if(args.size() > 2) {
        std::cerr << "cd: too many arguments\n";
        return {1, false};
    }

    const char* path;

    if(args.size() == 1) {
        path = std::getenv("HOME");

        if(path == nullptr) {
            std::cerr << "cd: HOME is not set\n";
            return {1, false};
        }
    } else {
        path = args[1].c_str();
    }

    char previousDirectory[PATH_MAX];
    bool hasPreviousDirectory = getcwd(previousDirectory, sizeof(previousDirectory)) != nullptr;

    if(chdir(path) == -1) {
        perror("cd");
        return {1, false};
    }

    char currentDirectory[PATH_MAX];

    if(getcwd(currentDirectory, sizeof(currentDirectory)) == nullptr) {
        perror("getcwd");
        return {1, false};
    }

    if(setenv("PWD", currentDirectory, 1) == -1) {
        perror("setenv");
        return {1, false};
    }

    if(hasPreviousDirectory && setenv("OLDPWD", previousDirectory, 1) == -1) {
        perror("setenv");
        return {1, false};
    }

    return {0, false};
}

ShellResult exitProcess (const std::vector<std::string>& args) {

    if(args.size() > 2) {
        std::cerr << "exit: too many arguments\n";
        return {1, false};
    }

    int status = 0;

    if(args.size() == 2) {
        try {
            status = std::stoi(args[1]);
        } catch(const std::exception&) {
            std::cerr << "exit: numeric argument required\n";
            return {2, true};
        }
    }

    return {status & 255, true};
}

ShellResult exportVariables (const std::vector<std::string>& args) {

    for(std::size_t i = 1; i < args.size(); i++) {
        std::size_t separator = args[i].find('=');

        if(separator != std::string::npos) {
            if(!setVariable(args[i])) {
                std::cerr << "export: invalid assignment: " << args[i] << '\n';
                return {1, false};
            }

            continue;
        }

        if(!validVariableName(args[i])) {
            std::cerr << "export: invalid variable name: " << args[i] << '\n';
            return {1, false};
        }

        const char* value = std::getenv(args[i].c_str());

        if(value == nullptr && setenv(args[i].c_str(), "", 0) == -1) {
            perror("setenv");
            return {1, false};
        }
    }

    return {0, false};
}

ShellResult unsetVariables (const std::vector<std::string>& args) {

    for(std::size_t i = 1; i < args.size(); i++) {
        if(!validVariableName(args[i])) {
            std::cerr << "unset: invalid variable name: " << args[i] << '\n';
            return {1, false};
        }

        if(unsetenv(args[i].c_str()) == -1) {
            perror("unsetenv");
            return {1, false};
        }
    }

    return {0, false};
}
