#ifndef GLOB_EXPANDER_H
#define GLOB_EXPANDER_H

#include<cstddef>
#include<string>
#include<vector>
#include "tokenizer.h"

struct GlobExpansionStatus {
    bool status;
    std::vector<Token> tokens;
    std::string err;
    std::size_t errPos;
};

GlobExpansionStatus expandGlobs(const std::vector<Token>& tokens);

#endif
