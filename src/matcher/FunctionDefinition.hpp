#ifndef CWRAPPER_MATCHER_FUNCTIONDEFINITION_HPP
#define CWRAPPER_MATCHER_FUNCTIONDEFINITION_HPP

#include "../Namespaces.hpp"
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>


namespace matcher {

/* TODO handle constructors */
class FunctionDefinition :
    public clang::ast_matchers::MatchFinder::MatchCallback {
public:
    FunctionDefinition(clang::ast_matchers::MatchFinder& finder,
        Namespaces& Namespaces);

    void run(const clang::ast_matchers::MatchFinder::MatchResult& Result)
        override;
private:
    static std::string _nomalizeName(const std::string& name);

    Namespaces& _namespaces;
};

} /* namespace macther */

#endif /* CWRAPPER_MATCHER_FUNCTIONDEFINITION_HPP */
