#ifndef CWRAPPER_MATCHER_NAMESPACES_HPP
#define CWRAPPER_MATCHER_NAMESPACES_HPP

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/AST/Decl.h>
#include <unordered_map>
#include <vector>
#include <string>


class Namespaces {
public:
    Namespaces() = default;

    std::string type2CWDef(const std::string& filepath,
        const clang::QualType& type);

    void update(const std::string& filepath, const clang::DeclContext* context);

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
        std::string type2CWDef(const clang::QualType& type);
        void push(const std::string& name);
        void pop(bool redefine = true);
        void set(const std::vector<std::string>& nss);
        void terminate();
        std::string toString(const std::string& separator = "_") const;
    private:
        void _setRoot(bool undefSpace = true);
        std::string _filepath;
        std::vector<std::string> _nss;
    };

    static std::vector<std::string> _getFullName(
        const clang::DeclContext* context);

    std::unordered_map<std::string, Namespace> _curNSs;
};

#endif /* CWRAPPER_MATCHER_NAMESPACES_HPP */
