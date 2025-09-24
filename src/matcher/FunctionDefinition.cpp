#include "FunctionDefinition.hpp"
#include "../utils.hpp"
#include <iostream>
#include <fstream>

#define BINDING_NAME "FunctionDefinition"

matcher::FunctionDefinition::FunctionDefinition(
    clang::ast_matchers::MatchFinder& finder)
{
    auto matcher = clang::ast_matchers::functionDecl(
        clang::ast_matchers::isExpansionInFileMatching(".*\\.(h|hpp)$"),
        clang::ast_matchers::unless(
            clang::ast_matchers::isInAnonymousNamespace()),
        clang::ast_matchers::unless(
            clang::ast_matchers::isPrivate()),
        clang::ast_matchers::unless(
            clang::ast_matchers::isProtected()),
        clang::ast_matchers::unless(
            clang::ast_matchers::isDeleted())
    ).bind(BINDING_NAME);
    finder.addMatcher(matcher, this);
}

void matcher::FunctionDefinition::run(
    const clang::ast_matchers::MatchFinder::MatchResult& Result)
{
    const auto fct = Result.Nodes.getNodeAs<clang::FunctionDecl>(BINDING_NAME);
    const auto from = Result.SourceManager->getFilename(fct->getLocation());
    if (
        fct != nullptr &&
        from.starts_with("/Users") /* TODO too dirty */
    ) {
        const auto to = getNewFilePath(from.str());

#ifdef CWRAPPER_DEBUG
        std::cout << "Function defined: " << fct->getReturnType().getAsString()
        << " " << fct->getNameAsString() << "(";
        for (unsigned int i = 0; i < fct->getNumParams(); ++i) {
            const auto param = fct->getParamDecl(i);
            std::cout << param->getType().getAsString() << " "
                << param->getNameAsString();
            if (param->hasDefaultArg()) {
                std::cout << " = " << param->getDefaultArg();
            }
            if (i + 1 < fct->getNumParams()) {
                std::cout << ", ";
            }
        }
        std::cout << ") at "
            << Result.SourceManager->getFilename(fct->getLocation()).str()
            << ":"
            << Result.SourceManager->getSpellingLineNumber(fct->getLocation())
            << ":"
            << Result.SourceManager->getSpellingColumnNumber(fct->getLocation())
            << " ---> " << to << std::endl;
#endif /* CWRAPPER_DEBUG */

        setupHeaderFile(to, from.str());

        // std::ofstream file(to, std::ios::app);
        // if (file.is_open()) {
        //     file << fct->getReturnType().getAsString()
        //         << " " << fct->getNameAsString() << "(";
        //     for (unsigned int i = 0; i < fct->getNumParams(); ++i) {
        //         const auto param = fct->getParamDecl(i);
        //         file << param->getType().getAsString() << " "
        //             << param->getNameAsString();
        //         if (param->hasDefaultArg()) {
        //             file << " = " << param->getDefaultArg()->IgnoreImplicit();
        //         }
        //         if (i + 1 < fct->getNumParams()) {
        //             file << ", ";
        //         }
        //     }
        //     file << ");\n";
        //     file.close();
        // } else {
        //     std::cerr << "Could not open file: " << to << std::endl;
        // }

    }
}
