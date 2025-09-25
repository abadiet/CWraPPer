#include "Namespaces.hpp"
#include <iostream>
#include <fstream>


/* forward declaration from utils.hpp */
std::string getSourceFromHeader(const std::string& header);


std::string Namespaces::type2CWDef(const std::string& filepath,
    const clang::QualType& type)
{
    if (_curNSs.find(filepath) == _curNSs.end()) {
        /* first time the file is seen */
        _curNSs[filepath] = Namespace(filepath);
    }
    return _curNSs[filepath].type2CWDef(type);
}

void Namespaces::update(const std::string& filepath,
    const clang::DeclContext* context)
{
    if (_curNSs.find(filepath) == _curNSs.end()) {
        /* first time the file is seen */
        _curNSs[filepath] = Namespace(filepath);
    }

    /* get the namespaces */
    const auto nss = Namespaces::_getFullName(context);

    /* set the current namespace */
    _curNSs[filepath].set(nss);
}

void Namespaces::terminate() {
    for (auto& [filepath, ns] : _curNSs) {
        ns.terminate();
    }
}

std::string Namespaces::toString(const std::string& filepath,
    const std::string& separator, bool withRoot) const
{
    if (const auto it = _curNSs.find(filepath); it != _curNSs.end()) {
        return it->second.toString(separator, withRoot);
    }
    return "";
}

Namespaces::Namespace::Namespace(
    const std::string& filepath) : _filepath(filepath), _nss({})
{
    std::ofstream file(_filepath, std::ios::app);
    if (file.is_open()) {
        file << '\n'
            << '\n';
        file.close();
    } else {
        std::cerr << "Could not open file: " << _filepath << std::endl;
    }
    _setRoot(false);
}

std::string Namespaces::Namespace::type2CWDef(const clang::QualType& type) {
    if (type.isNull()) return "void";

    /* if the type is canonical, but not a class/struct/enum, return it as is */
    if (type.isCanonical() && !type->isRecordType()) {
        return type.getAsString();
    }

    /* get the name with its namespaces */
    std::string name;
    clang::QualType actualType = type;
    int nPointer = 0;
    {
        auto str = actualType.getAsString();
        if (str.substr(0, 6) == "class ") {
            str = str.substr(6);
        } else if (str.substr(0, 7) == "struct ") {
            str = str.substr(7);
        } else if (str.substr(0, 5) == "enum ") {
            str = str.substr(5);
        }

        /* check for '*' */
        while (str.back() == '*') {
            str.pop_back();
            actualType = actualType->getPointeeType();
            ++nPointer;
        }
        if (str.back() == '&') {
            std::cerr << "Warning: reference type in function definition: "
                << str << std::endl;
            return "REFERENCE_TYPE_NOT_SUPPORTED_YET";
        }
        if (nPointer > 0) str.pop_back(); /* remove ' ' before the first '*' */

        const auto pos = str.rfind("::");
        if (pos == std::string::npos) {
            name = str;
        } else {
            name = str.substr(pos + 2);
        }
    }
    clang::DeclContext* ctx;
    if (const auto tt = actualType->getAs<clang::TypedefType>()) {
        ctx = tt->getDecl()->getDeclContext();
    } else if (const auto rt = actualType->getAs<clang::RecordType>()) {
        ctx = rt->getDecl()->getDeclContext();
    } else if (const auto et = actualType->getAs<clang::EnumType>()) {
        ctx = et->getDecl()->getDeclContext();
    } else {
        std::cerr << "Warning: unsupported type in function definition: "
            << type.getAsString() << std::endl;
        return "UNSUPPORTED_TYPE_IN_FUNCTION_DEFINITION";
    }
    const auto nss = Namespaces::_getFullName(ctx);

    /* check for const attribute */
    std::string res;
    if (name.rfind("const ") == 0) {
        name = name.substr(6); /* remove "const " */
        res = "const CW(";
    } else {
        res = "CW(";
    }

    /* check if it is define within the namespace */
    if (nss == _nss) {
        res += name + ")";
        if (nPointer > 0) res += ' ';
        for (int i = 0; i < nPointer; ++i) {
            res += '*';
        }
        return res;
    }

    /* the type should be declared using an absolute space aka CW(., .) */
    res +=  "CW_root";
    for (const auto& ns : nss) {
        res += '_' + ns;
    }
    res += ", " + name + ')';
    if (nPointer > 0) res += ' ';
    for (int i = 0; i < nPointer; ++i) {
        res += '*';
    }

    return res;
}

void Namespaces::Namespace::push(const std::string& name) {
    if (name.empty()) return;

    const auto srcFilepath = getSourceFromHeader(_filepath);
    std::ofstream source(srcFilepath, std::ios::app);
    std::ofstream header(_filepath, std::ios::app);
    if (header.is_open() && source.is_open()) {
        /* previous */
        const auto prevCWName = toString("_");

        _nss.push_back(name);

        /* new */
        const auto newCWName = toString("_");
        const auto newComment = toString("::", false);

        header << "#undef CW_SPACE\n"
            << '\n'
            << '\n'
            << "/* ### " << newComment << " ### */\n"
            << "#ifndef CW_" << newCWName << '\n'
            << "#define CW_" << newCWName << " CW_BUILD_SPACE(CW_"
            << prevCWName << ", " << name << ")\n"
            << "#endif /* CW_" << newCWName << " */\n"
            << "#define CW_SPACE CW_" << newCWName << '\n'
            << '\n';

        source << "#undef CW_SPACE\n"
            << '\n'
            << '\n'
            << "/* ### " << newComment << " ### */\n"
            << "#define CW_SPACE CW_" << newCWName << '\n'
            << '\n';

        header.close();
        source.close();
    } else {
        std::cerr << "Could not open file: " << _filepath << " or "
            << srcFilepath << std::endl;
    }
}

void Namespaces::Namespace::pop(bool redefine) {
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
}

void Namespaces::Namespace::set(
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

void Namespaces::Namespace::terminate() {
    while (!_nss.empty()) {
        pop(false);
    }

    const auto srcFilepath = getSourceFromHeader(_filepath);
    std::ofstream source(srcFilepath, std::ios::app);
    std::ofstream header(_filepath, std::ios::app);
    if (header.is_open() && source.is_open()) {
        header << "#undef CW_SPACE\n"
            << '\n'
            << '\n';
        source << "#undef CW_SPACE\n"
            << '\n'
            << '\n';
        header.close();
        source.close();
    } else {
        std::cerr << "Could not open file: " << _filepath << " or "
            << srcFilepath << std::endl;
    }
}

std::string Namespaces::Namespace::toString(const std::string& separator,
    bool withRoot) const
{
    std::string res = (withRoot ? "root" : "");
    for (const auto& ns : _nss) {
        res += separator + ns;
    }
    if (!res.empty() && !withRoot) {
        res = res.substr(separator.size());
    }
    return res;
}

void Namespaces::Namespace::_setRoot(bool undefSpace) {
    const auto srcFilepath = getSourceFromHeader(_filepath);
    std::ofstream source(srcFilepath, std::ios::app);
    std::ofstream header(_filepath, std::ios::app);
    if (header.is_open() && source.is_open()) {
        if (undefSpace) {
            header << "#undef CW_SPACE\n"
                << '\n'
                << '\n';
            source << "#undef CW_SPACE\n"
                << '\n'
                << '\n';
        }
        header << "/* ### root ### */\n"
            << "#define CW_SPACE CW_root\n"
            << '\n';
        source << "/* ### root ### */\n"
            << "#define CW_SPACE CW_root\n"
            << '\n';
        header.close();
        source.close();
    } else {
        std::cerr << "Could not open file: " << _filepath << " or "
            << srcFilepath << std::endl;
    }
}

std::vector<std::string> Namespaces::_getFullName(
    const clang::DeclContext* context)
{
    std::vector<std::string> nss;
    auto ctx = context;
    auto ctxKind = ctx->getDeclKind();
    while (ctxKind != clang::DeclaratorDecl::TranslationUnit) {
        switch (ctxKind) {
            case clang::DeclaratorDecl::Namespace:
                nss.push_back(
                    static_cast<const clang::NamespaceDecl*>(ctx)
                        ->getNameAsString()
                );
                break;
            case clang::DeclaratorDecl::Function:
                nss.push_back(
                    static_cast<const clang::FunctionDecl*>(ctx)
                        ->getNameAsString()
                );
                break;
            case clang::DeclaratorDecl::CXXRecord:
            case clang::DeclaratorDecl::Record:
                nss.push_back(
                    static_cast<const clang::RecordDecl*>(ctx)
                        ->getNameAsString()
                );
                break;
            // case clang::DeclaratorDecl::Tag:
            //     nss.push_back(
            //         static_cast<clang::TagDecl*>(ctx)->getNameAsString()
            //     );
            //     break;
            default:
                std::cerr << "Unknown context kind: "
                    << ctxKind << " which is " << ctx->getDeclKindName()
                    << std::endl;
                break;
        }
        ctx = ctx->getParent()->getEnclosingNamespaceContext();
        ctxKind = ctx->getDeclKind();
    }
    std::reverse(nss.begin(), nss.end());
    return nss;
}
