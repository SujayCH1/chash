#include<glob.h>
#include<string>
#include<vector>
#include "glob_expander.h"

namespace {

bool isFileRedirection(TokenType type) {

    return type == TokenType::RedirectInput ||
           type == TokenType::RedirectOutput ||
           type == TokenType::RedirectAppend ||
           type == TokenType::RedirectReadWrite ||
           type == TokenType::RedirectClobber ||
           type == TokenType::RedirectOutputAndError ||
           type == TokenType::RedirectAppendOutputAndError;
}

bool containsGlobPattern(const Token& token) {

    for(std::size_t i = 0; i < token.val.size(); i++) {
        bool protectedCharacter = false;

        if(i < token.globProtected.size()) {
            protectedCharacter = token.globProtected[i];
        }

        if(protectedCharacter) {
            continue;
        }

        char character = token.val[i];

        if(character == '*' || character == '?' || character == '[') {
            return true;
        }
    }

    return false;
}

bool needsGlobEscape(char character) {

    return character == '*' ||
           character == '?' ||
           character == '[' ||
           character == ']' ||
           character == '\\' ||
           character == '-' ||
           character == '!' ||
           character == '^';
}

std::string buildGlobPattern(const Token& token) {

    std::string pattern;

    for(std::size_t i = 0; i < token.val.size(); i++) {
        char character = token.val[i];
        bool protectedCharacter = false;

        if(i < token.globProtected.size()) {
            protectedCharacter = token.globProtected[i];
        }

        if(protectedCharacter && needsGlobEscape(character)) {
            pattern.push_back('\\');
        }

        pattern.push_back(character);
    }

    return pattern;
}

GlobExpansionStatus expandWord(const Token& token) {

    glob_t matches {};
    std::string pattern = buildGlobPattern(token);
    int result = glob(pattern.c_str(), 0, nullptr, &matches);

    if(result == GLOB_NOMATCH) {
        globfree(&matches);
        return {true, {token}, "", token.pos};
    }

    if(result != 0) {
        globfree(&matches);

        if(result == GLOB_NOSPACE) {
            return {false, {}, "not enough memory for glob expansion", token.pos};
        }

        return {false, {}, "could not expand glob pattern", token.pos};
    }

    std::vector<Token> expandedTokens;
    expandedTokens.reserve(matches.gl_pathc);

    for(std::size_t i = 0; i < matches.gl_pathc; i++) {
        std::string value = matches.gl_pathv[i];
        std::vector<bool> protection(value.size(), true);
        expandedTokens.push_back({TokenType::Word, value, token.pos, -1, protection});
    }

    globfree(&matches);
    return {true, expandedTokens, "", token.pos};
}

}

GlobExpansionStatus expandGlobs(const std::vector<Token>& tokens) {

    std::vector<Token> expandedTokens;

    for(std::size_t i = 0; i < tokens.size(); i++) {
        const Token& token = tokens[i];
        bool redirectionTarget = i > 0 && isFileRedirection(tokens[i - 1].type);

        if(token.type != TokenType::Word ||
           !containsGlobPattern(token)) {
            expandedTokens.push_back(token);
            continue;
        }

        GlobExpansionStatus wordStatus = expandWord(token);

        if(!wordStatus.status) {
            return wordStatus;
        }

        if(redirectionTarget && wordStatus.tokens.size() > 1) {
            return {
                false,
                {},
                "ambiguous redirection: " + token.val,
                token.pos
            };
        }

        for(Token& expandedToken : wordStatus.tokens) {
            expandedTokens.push_back(expandedToken);
        }
    }

    return {true, expandedTokens, "", 0};
}
