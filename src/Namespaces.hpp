#ifndef CWRAPPER_MATCHER_NAMESPACES_HPP
#define CWRAPPER_MATCHER_NAMESPACES_HPP

#include <clang/ASTMatchers/ASTMatchers.h>
#include <clang/AST/Decl.h>
#include <unordered_map>
#include <vector>
#include <string>
#include <fstream>
#include <memory>


class Namespaces {
public:
    Namespaces() = default;

    std::string type2CWDef(const std::string& filepath,
        const clang::QualType& type);

    void update(const std::string& filepath, const clang::DeclContext* context);

    void terminate();

    std::string toString(const std::string& filepath,
        const std::string& separator = "_", bool withRoot = true) const;

private:
    class Namespace {
    public:
        Namespace(const std::string& filepath);
        Namespace() = default;
        ~Namespace();
        std::string type2CWDef(const clang::QualType& type);
        void push(const std::string& name);
        void pop(bool redefine = true);
        void set(const std::vector<std::string>& nss);
        void terminate();
        std::string toString(const std::string& separator = "_",
            bool withRoot = true) const;
    private:
        void _setRoot(bool undefSpace = true);
        std::string _filepath;
        std::ofstream _header, _source;
        std::vector<std::string> _nss;
    };

    static std::vector<std::string> _getFullName(
        const clang::DeclContext* context);

    static const std::string _stdlibTypes[28];
    std::unordered_map<std::string, std::unique_ptr<Namespace>> _curNSs;
};

#endif /* CWRAPPER_MATCHER_NAMESPACES_HPP */
