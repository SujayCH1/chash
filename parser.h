#ifndef PARSER_H
#define PARSER_H

#include<string>
#include<vector>
#include<cstddef>
#include "tokenizer.h"
#include "ast.h"

struct ParserStatus {
    bool status;
    CommandList commandList;
    std::string err;
    std::size_t errPos;
};

ParserStatus parse (const std::vector<Token>& tokens);


#endif
