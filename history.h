#ifndef HISTORY_H
#define HISTORY_H

#include<cstddef>
#include<string>
#include<vector>

struct CommandHistory {
    std::vector<std::string> entries;
    std::vector<std::string> sessionEntries;
    std::string filePath;
};

bool initializeHistory(CommandHistory& history);
void addHistoryEntry(CommandHistory& history, const std::string& command);
bool saveHistory(const CommandHistory& history);
bool pruneHistoryFile(const CommandHistory& history, std::size_t maximumEntries = 1000);

#endif
