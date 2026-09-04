#include<iostream>
#include<string>
#include "interpreter.h"
#include "tokenizer.h"
#include "parser.h"
#include "executor.h"

ShellResult interpretCommands (const std::string& input,
                               int lastStatus,
                               const ShellContext& context) {

    TokenizationStatus tokenizationStatus = tokenize(input, lastStatus);

    if(!tokenizationStatus.status) {
        std::cerr << "csh: " << tokenizationStatus.err << " at position " << tokenizationStatus.errPos << '\n';
        return {2, false};
    }

    if(tokenizationStatus.tokens.size() == 1) {
        return {0, false};
    }

    ParserStatus parserStatus = parse(tokenizationStatus.tokens);

    if(!parserStatus.status) {
        std::cerr << "csh: " << parserStatus.err << " at position " << parserStatus.errPos << '\n';
        return {2, false};
    }

    return executePipeline(parserStatus.pipeline, context);
}
