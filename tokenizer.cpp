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

void appendVariable(const std::string& name, std::string& word) {

    const char* value = std::getenv(name.c_str());

    if(value != nullptr) {
        word.append(value);
    }
}

bool expandVariable(const std::string& input,
                    std::size_t& position,
                    std::string& word,
                    int lastStatus,
                    std::string& error,
                    std::size_t& errorPosition) {

    if(position + 1 >= input.size()) {
        word.push_back('$');
        return true;
    }

    if(input[position + 1] == '?') {
        word.append(std::to_string(lastStatus));
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
            word.append(std::to_string(lastStatus));
        } else if(validVariableName(name)) {
            appendVariable(name, word);
        } else {
            error = "invalid variable name";
            errorPosition = position;
            return false;
        }

        position = closing;
        return true;
    }

    if(!isVariableStart(input[position + 1])) {
        word.push_back('$');
        return true;
    }

    std::size_t end = position + 1;

    while(end < input.size() && isVariableCharacter(input[end])) {
        end++;
    }

    appendVariable(input.substr(position + 1, end - position - 1), word);
    position = end - 1;
    return true;
}

}

TokenizationStatus tokenize (const std::string& input, int lastStatus) {

    std::vector<Token> tokens;
    TokenizerState state = TokenizerState::Normal;
    std::string temp;
    std::size_t wordPos = 0;
    std::size_t quotePos = 0;
    bool wordStarted = false;
    bool wordQuotedOrEscaped = false;
    std::string expansionError;
    std::size_t expansionErrorPosition = 0;

    auto addWord = [&]() {
        if(wordStarted) {
            tokens.push_back({TokenType::Word, temp, wordPos});
            temp.clear();
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
                temp.push_back(character);
            }

            continue;
        }

        if(state == TokenizerState::DoubleQuote) {
            if(character == '"') {
                state = TokenizerState::Normal;
            } else if(character == '$') {
                if(!expandVariable(input, i, temp, lastStatus,
                                   expansionError, expansionErrorPosition)) {
                    return {false, {}, expansionError, expansionErrorPosition};
                }
            } else if(character == '\\') {
                if(i + 1 >= input.size()) {
                    return {false, {}, "incomplete escape", i};
                }

                char escaped = input[i + 1];

                if(escaped == '"' || escaped == '\\' || escaped == '$' || escaped == '`') {
                    temp.push_back(escaped);
                    i++;
                } else {
                    temp.push_back(character);
                }
            } else {
                temp.push_back(character);
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
            temp.push_back(input[++i]);
            continue;
        }

        if(character == '$') {
            if(!wordStarted) {
                wordStarted = true;
                wordPos = i;
            }

            if(!expandVariable(input, i, temp, lastStatus,
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
                tokens.push_back({TokenType::Unsupported, "||", i});
                i++;
            } else if(character == '|') {
                tokens.push_back({TokenType::Pipe, "|", i});
            } else if(character == '&' && i + 1 < input.size() && input[i + 1] == '&') {
                tokens.push_back({TokenType::Unsupported, "&&", i});
                i++;
            } else if(character == '&') {
                tokens.push_back({TokenType::Background, "&", i});
            } else {
                tokens.push_back({TokenType::Unsupported, std::string(1, character), i});
            }

            continue;
        }

        if(!wordStarted) {
            wordStarted = true;
            wordPos = i;
        }

        temp.push_back(character);
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
