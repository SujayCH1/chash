#include<string>
#include<cctype>
#include<limits>
#include<cstdlib>
#include "tokenizer.h"

namespace {

enum class TokenizerState {
    Normal,
    SingleQuote,
    DoubleQuote,
};

bool parseIoNumber(const std::string& value, int& result) {

    int number = 0;

    for(char character : value) {
        int digit = character - '0';

        if(number > (std::numeric_limits<int>::max() - digit) / 10) {
            return false;
        }

        number = number * 10 + digit;
    }

    result = number;
    return true;
}

bool isVariableStart(char character) {

    return std::isalpha(static_cast<unsigned char>(character)) || character == '_';
}

bool isVariableCharacter(char character) {

    return std::isalnum(static_cast<unsigned char>(character)) || character == '_';
}

bool validVariableName(const std::string& name) {

    if(name.empty() || !isVariableStart(name[0])) {
        return false;
    }

    for(char character : name) {
        if(!isVariableCharacter(character)) {
            return false;
        }
    }

    return true;
}

void appendCharacter(std::string& word,
                     std::vector<bool>& globProtection,
                     char character,
                     bool protectedCharacter) {

    word.push_back(character);
    globProtection.push_back(protectedCharacter);
}

void appendText(const std::string& text,
                std::string& word,
                std::vector<bool>& globProtection,
                bool protectedCharacters) {

    for(char character : text) {
        appendCharacter(word, globProtection, character, protectedCharacters);
    }
}

void appendVariable(const std::string& name,
                    std::string& word,
                    std::vector<bool>& globProtection,
                    bool protectedCharacters) {

    const char* value = std::getenv(name.c_str());

    if(value != nullptr) {
        appendText(value, word, globProtection, protectedCharacters);
    }
}

bool expandVariable(const std::string& input,
                    std::size_t& position,
                    std::string& word,
                    std::vector<bool>& globProtection,
                    bool protectedCharacters,
                    int lastStatus,
                    std::string& error,
                    std::size_t& errorPosition) {

    if(position + 1 >= input.size()) {
        appendCharacter(word, globProtection, '$', protectedCharacters);
        return true;
    }

    if(input[position + 1] == '?') {
        appendText(
            std::to_string(lastStatus),
            word,
            globProtection,
            protectedCharacters
        );
        position++;
        return true;
    }

    if(input[position + 1] == '{') {
        std::size_t closing = position + 2;

        while(closing < input.size() && input[closing] != '}') {
            closing++;
        }

        if(closing >= input.size()) {
            error = "unclosed variable expansion";
            errorPosition = position;
            return false;
        }

        std::string name = input.substr(position + 2, closing - position - 2);

        if(name == "?") {
            appendText(
                std::to_string(lastStatus),
                word,
                globProtection,
                protectedCharacters
            );
        } else if(validVariableName(name)) {
            appendVariable(
                name,
                word,
                globProtection,
                protectedCharacters
            );
        } else {
            error = "invalid variable name";
            errorPosition = position;
            return false;
        }

        position = closing;
        return true;
    }

    if(!isVariableStart(input[position + 1])) {
        appendCharacter(word, globProtection, '$', protectedCharacters);
        return true;
    }

    std::size_t end = position + 1;

    while(end < input.size() && isVariableCharacter(input[end])) {
        end++;
    }

    appendVariable(
        input.substr(position + 1, end - position - 1),
        word,
        globProtection,
        protectedCharacters
    );
    position = end - 1;
    return true;
}

}

TokenizationStatus tokenize (const std::string& input, int lastStatus) {

    std::vector<Token> tokens;
    TokenizerState state = TokenizerState::Normal;
    std::string temp;
    std::vector<bool> globProtection;
    std::size_t wordPos = 0;
    std::size_t quotePos = 0;
    bool wordStarted = false;
    bool wordQuotedOrEscaped = false;
    std::string expansionError;
    std::size_t expansionErrorPosition = 0;

    auto addWord = [&]() {
        if(wordStarted) {
            tokens.push_back({TokenType::Word, temp, wordPos, -1, globProtection});
            temp.clear();
            globProtection.clear();
            wordStarted = false;
            wordQuotedOrEscaped = false;
        }
    };

    for(std::size_t i = 0; i < input.size(); i++) {

        char character = input[i];

        if(state == TokenizerState::SingleQuote) {
            if(character == '\'') {
                state = TokenizerState::Normal;
            } else {
                appendCharacter(temp, globProtection, character, true);
            }

            continue;
        }

        if(state == TokenizerState::DoubleQuote) {
            if(character == '"') {
                state = TokenizerState::Normal;
            } else if(character == '$') {
                if(!expandVariable(input, i, temp, globProtection, true, lastStatus,
                                   expansionError, expansionErrorPosition)) {
                    return {false, {}, expansionError, expansionErrorPosition};
                }
            } else if(character == '\\') {
                if(i + 1 >= input.size()) {
                    return {false, {}, "incomplete escape", i};
                }

                char escaped = input[i + 1];

                if(escaped == '"' || escaped == '\\' || escaped == '$' || escaped == '`') {
                    appendCharacter(temp, globProtection, escaped, true);
                    i++;
                } else {
                    appendCharacter(temp, globProtection, character, true);
                }
            } else {
                appendCharacter(temp, globProtection, character, true);
            }

            continue;
        }

        if(std::isspace(static_cast<unsigned char>(character))) {
            addWord();
            continue;
        } 

        if(character == '\'' || character == '"') {
            if(!wordStarted) {
                wordStarted = true;
                wordPos = i;
            }

            wordQuotedOrEscaped = true;
            quotePos = i;
            state = character == '\'' ? TokenizerState::SingleQuote : TokenizerState::DoubleQuote;
            continue;
        }

        if(character == '\\') {
            if(i + 1 >= input.size()) {
                return {false, {}, "incomplete escape", i};
            }

            if(!wordStarted) {
                wordStarted = true;
                wordPos = i;
            }

            wordQuotedOrEscaped = true;
            appendCharacter(temp, globProtection, input[++i], true);
            continue;
        }

        if(character == '$') {
            if(!wordStarted) {
                wordStarted = true;
                wordPos = i;
            }

            if(!expandVariable(input, i, temp, globProtection, false, lastStatus,
                               expansionError, expansionErrorPosition)) {
                return {false, {}, expansionError, expansionErrorPosition};
            }

            continue;
        }

        if(character == '<' || character == '>' || character == '|' ||
           character == '&' || character == ';') {

            int ioNumber = -1;
            std::size_t operatorPos = i;

            if((character == '<' || character == '>') && wordStarted &&
               !wordQuotedOrEscaped && wordPos + temp.size() == i) {
                bool ioNumberCandidate = !temp.empty();

                for(char value : temp) {
                    if(!std::isdigit(static_cast<unsigned char>(value))) {
                        ioNumberCandidate = false;
                        break;
                    }
                }

                if(ioNumberCandidate) {
                    if(!parseIoNumber(temp, ioNumber)) {
                        return {false, {}, "file descriptor number is too large", wordPos};
                    }

                    operatorPos = wordPos;
                    temp.clear();
                    globProtection.clear();
                    wordStarted = false;
                    wordQuotedOrEscaped = false;
                }
            }

            addWord();

            if(character == '&' && i + 2 < input.size() &&
               input[i + 1] == '>' && input[i + 2] == '>') {
                tokens.push_back({TokenType::RedirectAppendOutputAndError, "&>>", operatorPos});
                i += 2;
            } else if(character == '&' && i + 1 < input.size() && input[i + 1] == '>') {
                tokens.push_back({TokenType::RedirectOutputAndError, "&>", operatorPos});
                i++;
            } else if(character == '<' && i + 1 < input.size() && input[i + 1] == '<') {
                tokens.push_back({TokenType::Unsupported, "<<", operatorPos, ioNumber});
                i++;
            } else if(character == '<' && i + 1 < input.size() && input[i + 1] == '&') {
                tokens.push_back({TokenType::RedirectDuplicateInput, "<&", operatorPos, ioNumber});
                i++;
            } else if(character == '<' && i + 1 < input.size() && input[i + 1] == '>') {
                tokens.push_back({TokenType::RedirectReadWrite, "<>", operatorPos, ioNumber});
                i++;
            } else if(character == '<') {
                tokens.push_back({TokenType::RedirectInput, "<", operatorPos, ioNumber});
            } else if(character == '>' && i + 1 < input.size() && input[i + 1] == '>') {
                tokens.push_back({TokenType::RedirectAppend, ">>", operatorPos, ioNumber});
                i++;
            } else if(character == '>' && i + 1 < input.size() && input[i + 1] == '&') {
                tokens.push_back({TokenType::RedirectDuplicateOutput, ">&", operatorPos, ioNumber});
                i++;
            } else if(character == '>' && i + 1 < input.size() && input[i + 1] == '|') {
                tokens.push_back({TokenType::RedirectClobber, ">|", operatorPos, ioNumber});
                i++;
            } else if(character == '>') {
                tokens.push_back({TokenType::RedirectOutput, ">", operatorPos, ioNumber});
            } else if(character == '|' && i + 1 < input.size() && input[i + 1] == '|') {
                tokens.push_back({TokenType::Or, "||", i});
                i++;
            } else if(character == '|') {
                tokens.push_back({TokenType::Pipe, "|", i});
            } else if(character == '&' && i + 1 < input.size() && input[i + 1] == '&') {
                tokens.push_back({TokenType::And, "&&", i});
                i++;
            } else if(character == '&') {
                tokens.push_back({TokenType::Background, "&", i});
            } else if(character == ';') {
                tokens.push_back({TokenType::Sequence, ";", i});
            } else {
                tokens.push_back({TokenType::Unsupported, std::string(1, character), i});
            }

            continue;
        }

        if(!wordStarted) {
            wordStarted = true;
            wordPos = i;
        }

        appendCharacter(temp, globProtection, character, false);
    }

    if(state == TokenizerState::SingleQuote) {
        return {false, {}, "unclosed single quote", quotePos};
    }

    if(state == TokenizerState::DoubleQuote) {
        return {false, {}, "unclosed double quote", quotePos};
    }

    addWord();
    tokens.push_back({TokenType::End, "", input.size()});

    return {true, tokens, "", input.size()};
}
