#include "FunctionDefinition.hpp"
#include "../utils.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

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
        std::ofstream header(to, std::ios::app);
        const auto srcFilepath = getSourceFromHeader(to);
        std::ofstream source(srcFilepath, std::ios::app);
        if (header.is_open() && source.is_open()) {
            std::ostringstream oss;

            oss << _namespaces.type2CWDef(to, fct->getReturnType()) << "\n"
                << "CW("
                << FunctionDefinition::_nomalizeName(fct->getNameAsString())
                << ")\n"
                << "(\n";

            /* if it is a class member, we add the class as first parameter */
            bool isMethod = false;
            const auto method = clang::dyn_cast<clang::CXXMethodDecl>(fct);
            if (method != nullptr && !method->isStatic()) {
                if (clang::dyn_cast<clang::CXXConstructorDecl>(method)) {
                    /* TODO handle constructors */
                    header.close();
                    source.close();
                    return;
                }
                const auto record = method->getParent();
                const auto type = clang::QualType(record->getTypeForDecl(), 0);
                oss << "    " << _namespaces.type2CWDef(to, type) << " * self";
                if (fct->getNumParams() > 0) {
                    oss << ',';
                }
                oss << '\n';
                isMethod = true;
            }

            /* then the other parameters */
            for (unsigned int i = 0; i < fct->getNumParams(); ++i) {
                const auto param = fct->getParamDecl(i);
                oss << "    " << _namespaces.type2CWDef(to, param->getType())
                    << ' ' << param->getNameAsString();
                if (i + 1 < fct->getNumParams()) {
                    oss << ',';
                }
                oss << '\n';
            }
            oss << ")";

            /* header */
            header << oss.str() << ";\n"
                << '\n';
            header.close();

            /* source */
            source << oss.str() << " {\n"
                << "    ";
            if (!fct->getReturnType()->isVoidType()) {
                source << "return ";
            }
            /* if it is a class member, we call it from the self object */
            {
                const auto nss = _namespaces.toString(to, "::", false);
                if (isMethod) {
                    source << "static_cast<" << nss << " *>(self)->";
                } else {
                    source << nss << (nss.empty() ? "" : "::");
                }
            }
            source << fct->getNameAsString() << "(\n";
            for (unsigned int i = 0; i < fct->getNumParams(); ++i) {
                const auto param = fct->getParamDecl(i);
                source << "        " << param->getNameAsString();
                if (i + 1 < fct->getNumParams()) {
                    source << ',';
                }
                source << '\n';
            }
            source << "    );\n"
                << "}\n"
                << '\n';
            source.close();
        } else {
            std::cerr << "Could not open file: " << to << " or "
                << srcFilepath << std::endl;
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
