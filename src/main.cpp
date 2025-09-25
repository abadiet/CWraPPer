#include "utils.hpp"
#include "Namespaces.hpp"
#include "matcher/TypeDefinition.hpp"
#include "matcher/FunctionDefinition.hpp"
#include "PreprocessorAction.hpp"
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/Tooling/Tooling.h>
#include <iostream>
#include <filesystem>

#define RUN(action)                                                            \
    {                                                                          \
        const auto res = tool.run(action.get());                               \
        switch (res) {                                                         \
        case 0:                                                                \
            break;                                                             \
        case 1:                                                                \
            llvm::errs() << "Error during preprocessing\n";                    \
            return 1;                                                          \
        case 2:                                                                \
            llvm::errs()                                                       \
                << "Some files were skipped due to missing compile commands."  \
                << '\n';                                                       \
            break;                                                             \
        default:                                                               \
            llvm::errs() << "Unknown error\n";                                 \
            return 1;                                                          \
        }                                                                      \
    }


static llvm::cl::OptionCategory category("my-tool options");
static llvm::cl::extrahelp CommonHelp(
    clang::tooling::CommonOptionsParser::HelpMessage);


int main(int argc, const char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: cwrapper <from> <to> <source0> [... <sourceN>]"
            " -- [compiler options]" << std::endl;
        return 1;
    }
    setupOutput(argv[1], argv[2]);
    argc -= 2;
    argv += 2;

    auto expectedParser = clang::tooling::CommonOptionsParser::create(argc,
        argv, category);
    if (!expectedParser) {
        llvm::errs() << expectedParser.takeError();
        return 1;
    }

    auto& optionsParser = expectedParser.get();

    clang::tooling::ClangTool tool(optionsParser.getCompilations(),
        optionsParser.getSourcePathList());

    RUN(clang::tooling::newFrontendActionFactory<PreprocessorAction>())

    Namespaces namespaces;

    {
        clang::ast_matchers::MatchFinder finder;
        matcher::TypeDefinition typeDefinition(finder, namespaces);
        matcher::FunctionDefinition functionDefinition(finder, namespaces);
        RUN(clang::tooling::newFrontendActionFactory(&finder))

    }

    terminateOutput(namespaces);

    return 0;
}
