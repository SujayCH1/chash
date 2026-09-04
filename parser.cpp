#include "parser.h"
#include<cctype>
#include<limits>

namespace {

bool isRedirection(TokenType type) {

    return type == TokenType::RedirectInput ||
           type == TokenType::RedirectOutput ||
           type == TokenType::RedirectAppend ||
           type == TokenType::RedirectReadWrite ||
           type == TokenType::RedirectDuplicateInput ||
           type == TokenType::RedirectDuplicateOutput ||
           type == TokenType::RedirectClobber ||
           type == TokenType::RedirectOutputAndError ||
           type == TokenType::RedirectAppendOutputAndError;
}

int defaultTarget(TokenType type) {

    if(type == TokenType::RedirectInput ||
       type == TokenType::RedirectReadWrite ||
       type == TokenType::RedirectDuplicateInput) {
        return 0;
    }

    return 1;
}

bool parseFileDescriptor(const std::string& value, int& descriptor) {

    if(value.empty()) {
        return false;
    }

    int number = 0;

    for(char character : value) {
        if(!std::isdigit(static_cast<unsigned char>(character))) {
            return false;
        }

        int digit = character - '0';

        if(number > (std::numeric_limits<int>::max() - digit) / 10) {
            return false;
        }

        number = number * 10 + digit;
    }

    descriptor = number;
    return true;
}

}

ParserStatus parse (const std::vector<Token>& tokens) {

    Pipeline pipeline;
    Command command;

    for(std::size_t i = 0; i < tokens.size(); i++) {

        const Token& token = tokens[i];

        if(token.type == TokenType::Word) {
            command.args.push_back(token.val);
            continue;
        }

        if(isRedirection(token.type)) {

            if(i + 1 >= tokens.size() || tokens[i + 1].type != TokenType::Word) {
                return {false, {}, "expected redirection target", token.pos};
            }

            int targetFd = token.ioNumber == -1 ? defaultTarget(token.type) : token.ioNumber;
            RedirectionType type;

            if(token.type == TokenType::RedirectOutputAndError ||
               token.type == TokenType::RedirectAppendOutputAndError) {

                RedirectionType outputType = token.type == TokenType::RedirectAppendOutputAndError
                    ? RedirectionType::Append
                    : RedirectionType::Output;

                command.redirections.push_back({outputType, 1, -1, tokens[i + 1].val});
                command.redirections.push_back({RedirectionType::Duplicate, 2, 1, ""});
                i++;
                continue;
            }

            if(token.type == TokenType::RedirectDuplicateInput ||
               token.type == TokenType::RedirectDuplicateOutput) {

                if(tokens[i + 1].val == "-") {
                    command.redirections.push_back({RedirectionType::Close, targetFd, -1, ""});
                    i++;
                    continue;
                }

                int sourceFd;

                if(!parseFileDescriptor(tokens[i + 1].val, sourceFd)) {
                    return {false, {}, "expected file descriptor or - after redirection", tokens[i + 1].pos};
                }

                command.redirections.push_back({RedirectionType::Duplicate, targetFd, sourceFd, ""});
                i++;
                continue;
            }

            if(token.type == TokenType::RedirectInput) {
                type = RedirectionType::Input;
            } else if(token.type == TokenType::RedirectOutput ||
                      token.type == TokenType::RedirectClobber) {
                type = RedirectionType::Output;
            } else if(token.type == TokenType::RedirectAppend) {
                type = RedirectionType::Append;
            } else {
                type = RedirectionType::ReadWrite;
            }

            command.redirections.push_back({type, targetFd, -1, tokens[i + 1].val});
            i++;
            continue;
        }

        if(token.type == TokenType::Pipe) {
            if(command.args.empty()) {
                return {false, {}, "expected command before pipe", token.pos};
            }

            pipeline.commands.push_back(command);
            command = {};
            continue;
        }

        if(token.type == TokenType::Unsupported) {
            return {false, {}, "unsupported operator " + token.val, token.pos};
        }

        if(token.type == TokenType::End) {
            if(i + 1 != tokens.size()) {
                return {false, {}, "unexpected token after end of input", tokens[i + 1].pos};
            }

            if(command.args.empty()) {
                if(!command.redirections.empty()) {
                    return {false, {}, "expected command", token.pos};
                }

                if(!pipeline.commands.empty()) {
                    return {false, {}, "expected command after pipe", token.pos};
                }

                return {true, pipeline, "", token.pos};
            }

            pipeline.commands.push_back(command);
            return {true, pipeline, "", token.pos};
        }
    }

    std::size_t errorPosition = tokens.empty() ? 0 : tokens.back().pos + tokens.back().val.size();
    return {false, {}, "expected end of input", errorPosition};
}
