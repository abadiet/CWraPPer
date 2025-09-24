#include "TypeDefinition.hpp"
#include "../utils.hpp"
#include <iostream>
#include <fstream>

#define BINDING_NAME "TypeDefinition"


matcher::TypeDefinition::TypeDefinition(
    clang::ast_matchers::MatchFinder& finder, NamespaceDefinition& nsDef)
    : _nsDef(nsDef)
{
    {
        auto matcher = clang::ast_matchers::tagDecl(
            clang::ast_matchers::isExpansionInFileMatching(".*\\.(h|hpp)$"),
            clang::ast_matchers::unless(
                clang::ast_matchers::isInAnonymousNamespace()),
            clang::ast_matchers::unless(
                clang::ast_matchers::isPrivate()),
            clang::ast_matchers::unless(
                clang::ast_matchers::isProtected())
        ).bind(BINDING_NAME);
        finder.addMatcher(matcher, this);
    }
    {
        auto matcher = clang::ast_matchers::typedefNameDecl(
            clang::ast_matchers::isExpansionInFileMatching(".*\\.(h|hpp)$"),
            clang::ast_matchers::unless(
                clang::ast_matchers::isInAnonymousNamespace()),
            clang::ast_matchers::unless(
                clang::ast_matchers::isPrivate()),
            clang::ast_matchers::unless(
                clang::ast_matchers::isProtected())
        ).bind(BINDING_NAME);
        finder.addMatcher(matcher, this);
    }
}

void matcher::TypeDefinition::run(
    const clang::ast_matchers::MatchFinder::MatchResult& Result)
{
    const auto obj = Result.Nodes.getNodeAs<clang::TypeDecl>(BINDING_NAME);
    const auto from = Result.SourceManager->getFilename(obj->getLocation());
    if (
        obj != nullptr &&
        from.starts_with("/Users") && /* TODO too dirty */
        /* to avoid defining it twice (idk why clang call it twice) */
        obj->getNameAsString() != _lastTypeDefined
    ) {
        const auto to = getNewFilePath(from.str());

#ifdef CWRAPPER_DEBUG
        std::cout << "Type defined: " << obj->getNameAsString() << " at "
            << Result.SourceManager->getFilename(obj->getLocation()).str()
            << ":"
            << Result.SourceManager->getSpellingLineNumber(obj->getLocation())
            << ":"
            << Result.SourceManager->getSpellingColumnNumber(obj->getLocation())
            << " ---> " << to << std::endl;
#endif /* CWRAPPER_DEBUG */

        setupHeaderFile(to, from.str());

        std::ofstream file(to, std::ios::app);
        if (file.is_open()) {
            /* Update the namespace (if we left some) */
            /* get the namespaces */
            std::vector<std::string> nss;
            nss.push_back(obj->getDeclContext()
                ->getEnclosingNamespaceContext()->getDeclKindName());
            while (nss.back() != "TranslationUnit") {
                nss.push_back(obj->getDeclContext()
                    ->getEnclosingNamespaceContext()->getDeclKindName());
            }
            nss.pop_back(); /* remove TranslationUnit */
            std::reverse(nss.begin(), nss.end());

            /* update the current namespace */
            _nsDef.updateNS(to, nss);

            auto type = Result.Context->getTypeDeclType(obj).getCanonicalType()
                .getAsString();
            /* TODO consider using struct for class definitions */
            if (type.substr(0, 6) == "class ") type = "void";
            file << "typedef " << type << " CW(" << obj->getNameAsString()
                << ");\n";
            file.close();
        } else {
            std::cerr << "Could not open file: " << to << std::endl;
        }

        _lastTypeDefined = obj->getNameAsString();
    }
}
