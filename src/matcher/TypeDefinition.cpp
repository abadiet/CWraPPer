#include "TypeDefinition.hpp"
#include "../utils.hpp"
#include <iostream>
#include <fstream>

#define BINDING_NAME "TypeDefinition"


matcher::TypeDefinition::TypeDefinition(
    clang::ast_matchers::MatchFinder& finder, Namespaces& namespaces)
    : _namespaces(namespaces)
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
        obj->getNameAsString() != "" &&
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

        /* Ensure the file exists */
        setupHeaderFile(to, from.str());

        /* Update the namespaces */
        _namespaces.update(to, obj->getDeclContext());

        /* Write the typedef */
        std::ofstream file(to, std::ios::app);
        if (file.is_open()) {
            /* TODO: handle struct as struct */
            /* TOFIX: typedef struct/enum */

            std::string typeName;
            if (llvm::dyn_cast<clang::RecordDecl>(obj)) {
                typeName = "void";
            } else if (const auto enumDecl = llvm::dyn_cast<clang::EnumDecl>(obj)) {
                if (enumDecl->isScoped()) {
                    typeName = _namespaces.type2CWDef(to,
                        enumDecl->getIntegerType());
                } else {
                    typeName = _namespaces.type2CWDef(to,
                        enumDecl->getPromotionType());
                }
            } else if (
                const auto tdef = llvm::dyn_cast<clang::TypedefNameDecl>(obj)
            ) {
                typeName = _namespaces.type2CWDef(to,
                    tdef->getUnderlyingType());
            } else {
                typeName = _namespaces.type2CWDef(to,
                    clang::QualType(obj->getTypeForDecl(), 0));
            }
            file << "typedef " << typeName << " CW("
                << obj->getNameAsString() << ");\n"
                << '\n';
            file.close();
        } else {
            std::cerr << "Could not open file: " << to << std::endl;
        }

        _lastTypeDefined = obj->getNameAsString();
    }
}
