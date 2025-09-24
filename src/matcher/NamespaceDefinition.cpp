#include "NamespaceDefinition.hpp"
#include "../utils.hpp"
#include <iostream>
#include <fstream>

#define BINDING_NAME "NamespaceDefinition"

matcher::NamespaceDefinition::NamespaceDefinition(
    clang::ast_matchers::MatchFinder& finder)
{
    auto matcher = clang::ast_matchers::namespaceDecl(
        clang::ast_matchers::isExpansionInFileMatching(".*\\.(h|hpp)$"),
        clang::ast_matchers::unless(
            clang::ast_matchers::isInAnonymousNamespace()),
        clang::ast_matchers::unless(
            clang::ast_matchers::isAnonymous()),
        clang::ast_matchers::unless(
            clang::ast_matchers::isPrivate()),
        clang::ast_matchers::unless(
            clang::ast_matchers::isProtected())
    ).bind(BINDING_NAME);
    // auto matcher = clang::ast_matchers::namespaceAliasDecl().bind(BINDING_NAME);
    finder.addMatcher(matcher, this);
}

void matcher::NamespaceDefinition::run(
    const clang::ast_matchers::MatchFinder::MatchResult& Result)
{
    const auto ns = Result.Nodes.getNodeAs<clang::NamespaceDecl>(BINDING_NAME);
    const auto from = Result.SourceManager->getFilename(ns->getLocation());
    if (ns != nullptr && from.starts_with("/Users")) { /* TODO too dirty */
        const auto to = getNewFilePath(from.str());

#ifdef CWRAPPER_DEBUG
        std::cout << "Namespace defined: " << ns << " at "
            << Result.SourceManager->getFilename(ns->getLocation()).str()
            << ":"
            << Result.SourceManager->getSpellingLineNumber(ns->getLocation())
            << ":"
            << Result.SourceManager->getSpellingColumnNumber(ns->getLocation())
            << " ---> " << to << std::endl;
#endif /* CWRAPPER_DEBUG */

        setupHeaderFile(to, from.str());

        if (_curNSs.find(to) == _curNSs.end()) {
            /* first time the file is seen */
            _curNSs[to] = Namespace(to);
        }

        /* push the new namespace */
        _curNSs[to].push(ns->getNameAsString());
    }
}

void matcher::NamespaceDefinition::updateNS(const std::string& filepath,
    const std::vector<std::string>& nss)
{
    if (_curNSs.find(filepath) == _curNSs.end()) {
        /* first time the file is seen */
        _curNSs[filepath] = Namespace(filepath);
    }

    /* set the current namespace */
    _curNSs[filepath].set(nss);
}

void matcher::NamespaceDefinition::terminate() {
    for (auto& [filepath, ns] : _curNSs) {
        ns.terminate();
    }
}

matcher::NamespaceDefinition::Namespace::Namespace(
    const std::string& filepath) : _filepath(filepath), _nss({})
{
    _setRoot(true);
}

void matcher::NamespaceDefinition::Namespace::push(const std::string& name) {
    if (name.empty()) return;

    std::ofstream file(_filepath, std::ios::app);
    if (file.is_open()) {
        /* previous */
        const auto prevCWName = toString("_");
        const auto prevComment = toString("::");

        _nss.push_back(name);

        /* new */
        const auto newCWName = toString("_");
        const auto newComment = toString("::");

        file << "#undef CW_SPACE /* " << prevComment << " */\n"
            << "\n"
            << "\n"
            << "/* ### " << newComment << " ### */\n"
            << "#define CW_" << newCWName << " CW_BUILD_SPACE(CW_"
            << prevCWName << ", " << name << ")\n"
            << "#define CW_SPACE CW_" << newCWName << "\n"
            << "\n";
        file.close();
    } else {
        std::cerr << "Could not open file: " << _filepath << std::endl;
    }
}

void matcher::NamespaceDefinition::Namespace::pop(bool redefine) {
    std::ofstream file(_filepath, std::ios::app);
    if (file.is_open()) {
        file << "#undef CW_" << toString("_") << " /* " << toString("::")
            << " */\n"
            << "#undef CW_SPACE /* " << toString("::") << " */\n"
            << "\n";

        file.close();

        if (!_nss.empty()) _nss.pop_back();

        if (redefine) {
            std::string back;
            if (_nss.empty()) {
                _setRoot();
            } else {
                const auto back = _nss.back();
                _nss.pop_back();
                push(back);
            }
        }
    } else {
        std::cerr << "Could not open file: " << _filepath << std::endl;
    }
}

void matcher::NamespaceDefinition::Namespace::set(
    const std::vector<std::string>& nss)
{
    /* Find the last common namespace */
    size_t i = 0;
    while (i < _nss.size() && i < nss.size() && _nss[i] == nss[i]) {
        ++i;
    }

    if (i == _nss.size() && i == nss.size()) {
        /* same namespace */
        return;
    }

    /* Pop the namespaces that are not common */
    while (_nss.size() > i) {
        pop(false);
    }

    /* Push the new namespaces */
    if (nss.empty()) {
        _setRoot();
    } else {
        while (i < nss.size()) {
            push(nss[i]);
            ++i;
        }
    }
}

void matcher::NamespaceDefinition::Namespace::terminate() {
    while (!_nss.empty()) {
        pop(false);
    }

    std::ofstream file(_filepath, std::ios::app);
    if (file.is_open()) {
        file << "\n"
            << "#undef CW_SPACE\n"
            << "\n"
            << "\n";
        file.close();
    } else {
        std::cerr << "Could not open file: " << _filepath << std::endl;
    }
}

std::string matcher::NamespaceDefinition::Namespace::toString(
    const std::string& separator) const
{
    std::string res = "root";
    for (const auto& ns : _nss) {
        res += separator + ns;
    }
    return res;
}

void matcher::NamespaceDefinition::Namespace::_setRoot(bool define) {
    std::ofstream file(_filepath, std::ios::app);
    if (file.is_open()) {
        file << "/* ### root ### */\n";
        if (define) {
            file << "#define CW_root CW_BUILD_SPACE(root)\n";
        }
        file << "#define CW_SPACE CW_root\n"
            << "\n";
        file.close();
    } else {
        std::cerr << "Could not open file: " << _filepath << std::endl;
    }
}
