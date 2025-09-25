#include "FunctionDefinition.hpp"
#include "../utils.hpp"
#include <iostream>
#include <fstream>

#define BINDING_NAME "FunctionDefinition"


matcher::FunctionDefinition::FunctionDefinition(
    clang::ast_matchers::MatchFinder& finder, Namespaces& Namespaces)
    : _namespaces(Namespaces)
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

        /* Ensure the file exists */
        setupHeaderFile(to, from.str());

        /* Update the namespaces */
        _namespaces.update(to, fct->getDeclContext());

        /* Write the function declaration */
        std::ofstream file(to, std::ios::app);
        if (file.is_open()) {
            file << _namespaces.type2CWDef(to, fct->getReturnType())
                << " CW("
                << FunctionDefinition::_nomalizeName(fct->getNameAsString())
                << ") (";
            for (unsigned int i = 0; i < fct->getNumParams(); ++i) {
                const auto param = fct->getParamDecl(i);
                file << _namespaces.type2CWDef(to, param->getType())
                    << " "
                    << param->getNameAsString();
                if (i + 1 < fct->getNumParams()) {
                    file << ", ";
                }
            }
            file << ");\n"
                << '\n';
            file.close();
        } else {
            std::cerr << "Could not open file: " << to << std::endl;
        }
    }
}

std::string matcher::FunctionDefinition::_nomalizeName(const std::string& name) {
    auto res = name;
    std::replace(res.begin(), res.end(), '~', '_');
    return res;
}

// const clang::Type* getNamedType(const clang::QualType& type) {
//     auto T = type.getTypePtrOrNull();

//     while (true) {
//         if (const PointerType *PT = dyn_cast<PointerType>(T))
//             T = PT->getPointeeType().getTypePtr();
//         else if (const LValueReferenceType *LT = dyn_cast<LValueReferenceType>(T))
//             T = LT->getPointeeType().getTypePtr();
//         else if (const RValueReferenceType *RT = dyn_cast<RValueReferenceType>(T))
//             T = RT->getPointeeType().getTypePtr();
//         else if (const ArrayType *AT = dyn_cast<ArrayType>(T))
//             T = AT->getElementType().getTypePtr();
//         else if (const ElaboratedType *ET = dyn_cast<ElaboratedType>(T))
//             T = ET->getNamedType().getTypePtr();
//         else
//             break; // reached a "named" or builtin type
//     }

//     return T;
// }
