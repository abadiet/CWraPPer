#ifndef CWRAPPER_MATCHER_VARIABLEDEFINITION_HPP
#define CWRAPPER_MATCHER_VARIABLEDEFINITION_HPP

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace matcher {

class VariableDefinition :
    public clang::ast_matchers::MatchFinder::MatchCallback {
public:
    VariableDefinition(clang::ast_matchers::MatchFinder& finder);

    void run(const clang::ast_matchers::MatchFinder::MatchResult& Result)
        override;
};

} /* namespace macther */

#endif /* CWRAPPER_MATCHER_VARIABLEDEFINITION_HPP */
