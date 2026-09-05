#ifndef TOKENIZER_H
#define TOKENIZER_H

#include<string>
#include<vector>
#include<cstddef>

enum class TokenType {
    Word,
    RedirectInput,
    RedirectOutput,
    RedirectAppend,
    RedirectReadWrite,
    RedirectDuplicateInput,
    RedirectDuplicateOutput,
    RedirectClobber,
    RedirectOutputAndError,
    RedirectAppendOutputAndError,
    Pipe,
    Background,
    Unsupported,
    End
};

struct Token {
    TokenType type;
    std::string val;
    std::size_t pos;
    int ioNumber = -1;
};


struct TokenizationStatus {
    bool  status;
    std::vector<Token> tokens;
    std::string err;
    std::size_t errPos;
};

TokenizationStatus tokenize (const std::string& input, int lastStatus = 0);

#endif 
