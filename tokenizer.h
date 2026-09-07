#ifndef TOKENIZER_H
#define TOKENIZER_H

#include<string>
#include<utility>
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
    Sequence,
    And,
    Or,
    Unsupported,
    End
};

struct Token {
    Token(TokenType tokenType,
          std::string value,
          std::size_t position,
          int descriptor = -1,
          std::vector<bool> protection = {})
        : type(tokenType),
          val(std::move(value)),
          pos(position),
          ioNumber(descriptor),
          globProtected(std::move(protection)) {
    }

    TokenType type;
    std::string val;
    std::size_t pos;
    int ioNumber = -1;
    std::vector<bool> globProtected;
};


struct TokenizationStatus {
    bool  status;
    std::vector<Token> tokens;
    std::string err;
    std::size_t errPos;
};

TokenizationStatus tokenize (const std::string& input, int lastStatus = 0);

#endif 
