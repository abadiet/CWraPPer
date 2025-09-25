#ifndef CWRAPPER_MATCHER_TYPEDEFINITION_HPP
#define CWRAPPER_MATCHER_TYPEDEFINITION_HPP

#include "../Namespaces.hpp"
#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>

namespace matcher {

class TypeDefinition :
    public clang::ast_matchers::MatchFinder::MatchCallback {
public:
    TypeDefinition(clang::ast_matchers::MatchFinder& finder,
        Namespaces& Namespaces);

    void run(const clang::ast_matchers::MatchFinder::MatchResult& Result)
        override;
private:
    Namespaces& _namespaces;
    std::string _lastTypeDefined;
};

} /* namespace macther */

#endif /* CWRAPPER_MATCHER_TYPEDEFINITION_HPP */
