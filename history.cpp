#include "history.h"
#include<cctype>
#include<cerrno>
#include<cstdio>
#include<cstdlib>
#include<fstream>
#include<vector>
#include<fcntl.h>
#include<unistd.h>

namespace {

bool writeLine(int fileDescriptor, const std::string& command) {

    std::string line = command + '\n';
    std::size_t written = 0;

    while(written < line.size()) {
        ssize_t result = write(
            fileDescriptor,
            line.data() + written,
            line.size() - written
        );

        if(result == -1 && errno == EINTR) {
            continue;
        }

        if(result <= 0) {
            return false;
        }

        written += static_cast<std::size_t>(result);
    }

    return true;
}

}

bool initializeHistory(CommandHistory& history) {

    const char* home = std::getenv("HOME");

    if(home == nullptr || home[0] == '\0') {
        history.filePath.clear();
        return true;
    }

    history.filePath = std::string(home) + "/.chash_history";

    int historyFile = open(
        history.filePath.c_str(),
        O_WRONLY | O_CREAT | O_APPEND,
        0600
    );

    if(historyFile == -1) {
        return false;
    }

    if(close(historyFile) == -1) {
        return false;
    }

    std::ifstream input(history.filePath);

    if(!input) {
        return false;
    }

    std::string command;

    while(std::getline(input, command)) {
        if(!command.empty()) {
            history.entries.push_back(command);
        }
    }

    return !input.bad();
}

void addHistoryEntry(CommandHistory& history, const std::string& command) {

    bool hasNonWhitespaceCharacter = false;

    for(char character : command) {
        if(!std::isspace(static_cast<unsigned char>(character))) {
            hasNonWhitespaceCharacter = true;
            break;
        }
    }

    if(!hasNonWhitespaceCharacter) {
        return;
    }

    if(!history.entries.empty() && history.entries.back() == command) {
        return;
    }

    history.entries.push_back(command);
    history.sessionEntries.push_back(command);
}

bool saveHistory(const CommandHistory& history) {

    if(history.filePath.empty() || history.sessionEntries.empty()) {
        return true;
    }

    int historyFile = open(
        history.filePath.c_str(),
        O_WRONLY | O_CREAT | O_APPEND,
        0600
    );

    if(historyFile == -1) {
        return false;
    }

    bool success = true;

    for(const std::string& command : history.sessionEntries) {
        if(!writeLine(historyFile, command)) {
            success = false;
            break;
        }
    }

    if(close(historyFile) == -1) {
        success = false;
    }

    return success;
}

bool pruneHistoryFile(const CommandHistory& history, std::size_t maximumEntries) {

    if(history.filePath.empty()) {
        return true;
    }

    std::ifstream input(history.filePath);

    if(!input) {
        return false;
    }

    std::vector<std::string> entries;
    std::string command;

    while(std::getline(input, command)) {
        if(!command.empty()) {
            entries.push_back(command);
        }
    }

    if(input.bad()) {
        return false;
    }

    if(entries.size() <= maximumEntries) {
        return true;
    }

    std::string temporaryPath = history.filePath + ".tmp." + std::to_string(getpid());
    int temporaryFile = open(
        temporaryPath.c_str(),
        O_WRONLY | O_CREAT | O_EXCL,
        0600
    );

    if(temporaryFile == -1) {
        return false;
    }

    bool success = true;
    std::size_t firstEntry = entries.size() - maximumEntries;

    for(std::size_t i = firstEntry; i < entries.size(); i++) {
        if(!writeLine(temporaryFile, entries[i])) {
            success = false;
            break;
        }
    }

    if(close(temporaryFile) == -1) {
        success = false;
    }

    if(success) {
        if(rename(temporaryPath.c_str(), history.filePath.c_str()) == -1) {
            success = false;
        }
    }

    if(!success) {
        unlink(temporaryPath.c_str());
    }

    return success;
}
