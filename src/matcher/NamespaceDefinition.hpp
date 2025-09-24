#ifndef CWRAPPER_MATCHER_NAMESPACEDEFINITION_HPP
#define CWRAPPER_MATCHER_NAMESPACEDEFINITION_HPP

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <unordered_map>


namespace matcher {

class NamespaceDefinition :
    public clang::ast_matchers::MatchFinder::MatchCallback {
public:
    NamespaceDefinition(clang::ast_matchers::MatchFinder& finder);

    void run(const clang::ast_matchers::MatchFinder::MatchResult& Result)
        override;

    void updateNS(const std::string& filepath,
        const std::vector<std::string>& nss);

    void terminate();

private:
    class Namespace {
    public:
        Namespace(const std::string& filepath);
        Namespace() = default;
        ~Namespace() = default;
        Namespace(const Namespace& other) = default;
        Namespace(Namespace&& other) noexcept = default;
        Namespace& operator=(const Namespace& other) = default;
        Namespace& operator=(Namespace&& other) noexcept = default;
        void push(const std::string& name);
        void pop(bool redefine = true);
        void set(const std::vector<std::string>& nss);
        void terminate();
        std::string toString(const std::string& separator = "_") const;
    private:
        void _setRoot(bool define = false);
        std::string _filepath;
        std::vector<std::string> _nss;
    };

    std::unordered_map<std::string, Namespace> _curNSs;
};

} /* namespace macther */

#endif /* CWRAPPER_MATCHER_NAMESPACEDEFINITION_HPP */
