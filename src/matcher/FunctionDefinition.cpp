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
        std::cout << "Function: " << fct->getReturnType().getAsString()
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

        std::ostringstream oss;
        bool isMethod = false;
        bool error = false;
        std::string returnType;
        try {
            /* Update the namespaces */
            _namespaces.update(to, fct->getDeclContext());

            /* check if the function is templated */
            if (fct->isTemplated()) {
                throw std::runtime_error("Skipping templated function");
            }

            /* Write the function declaration */
            returnType = _namespaces.type2CWDef(to, fct->getReturnType());
            oss << returnType << "\n"
                << "CW("
                << FunctionDefinition::_nomalizeName(fct->getNameAsString())
                << ")\n"
                << "(\n";

            /* if it is a class member, we add the class as first parameter */
            const auto method = clang::dyn_cast<clang::CXXMethodDecl>(fct);
            if (method != nullptr && !method->isStatic()) {
                if (clang::dyn_cast<clang::CXXConstructorDecl>(method)) {
                    throw std::runtime_error("Skipping constructor");
                }
                oss << "    CW() * self";
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
        } catch (const std::exception& e) {
            error = true;
            oss.str("");
            std::ostringstream err;
            err << "Error while processing function \""
                << fct->getNameAsString()
                << "\" at "
                << Result.SourceManager->getFilename(
                    fct->getLocation()).str()
                << ":"
                << Result.SourceManager->getSpellingLineNumber(
                    fct->getLocation())
                << ":"
                << Result.SourceManager->getSpellingColumnNumber(
                    fct->getLocation())
                << " (exported to " << to << ") : " << e.what();
            oss << "/* " << err.str() << " */\n"
                << '\n';
            std::cerr << err.str() << std::endl;
        }

        std::ofstream header(to, std::ios::app);
        if (header.is_open()) {
            header << oss.str();
            if (!error) {
                header << ";\n"
                    << '\n';
            }
            header.close();
        } else {
            std::cerr << "Could not open file: " << to << std::endl;
            return;
        }

        const auto srcFilepath = getSourceFromHeader(to);
        std::ofstream source(srcFilepath, std::ios::app);
        if (source.is_open()) {
            source << oss.str();
            if (!error) {
                source << " {\n"
                    << "    ";
                if (!fct->getReturnType()->isVoidType()) {
                    source << "return static_cast<" << returnType << ">(\n"
                        << "    ";
                }
                /* if it is a class member, call it from the self object */
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
                    source << "        static_cast<"
                        << param->getType().getAsString() << ">("
                        << param->getNameAsString() << ")";
                    if (i + 1 < fct->getNumParams()) {
                        source << ',';
                    }
                    source << '\n';
                }
                source << "    ";
                if (!fct->getReturnType()->isVoidType()) {
                    source << ")";
                }
                source << ");\n"
                    << "}\n"
                    << '\n';
            }
            source.close();
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
