#include "Namespaces.hpp"
#include <iostream>


const std::string Namespaces::_stdTypes[33] = {
    "int8_t", "int16_t","int32_t", "int64_t", "uint8_t", "uint16_t", "uint32_t",
    "uint64_t", "int_least8_t", "int_least16_t", "int_least32_t",
    "int_least64_t", "uint_least8_t", "uint_least16_t", "uint_least32_t",
    "uint_least64_t", "int_fast8_t", "int_fast16_t", "int_fast32_t",
    "int_fast64_t", "uint_fast8_t", "uint_fast16_t", "uint_fast32_t",
    "uint_fast64_t", "intptr_t", "uintptr_t", "intmax_t", "uintmax_t", "size_t",
    "div_t", "ldiv_t", "ptrdiff_t", "wchar_t"
};

/* forward declaration from utils.hpp */
std::string getSourceFromHeader(const std::string& _header);


std::string Namespaces::type2CWDef(const std::string& filepath,
    const clang::QualType& type)
{
    if (_curNSs.find(filepath) == _curNSs.end()) {
        /* first time the file is seen */
        _curNSs[filepath] = std::make_unique<Namespace>(filepath);
    }
    return _curNSs[filepath]->type2CWDef(type);
}

void Namespaces::update(const std::string& filepath,
    const clang::DeclContext* context)
{
    if (_curNSs.find(filepath) == _curNSs.end()) {
        /* first time the file is seen */
        _curNSs[filepath] = std::make_unique<Namespace>(filepath);
    }

    /* get the namespaces */
    const auto nss = Namespaces::_getFullName(context);

    /* set the current namespace */
    _curNSs[filepath]->set(nss);
}

void Namespaces::terminate() {
    for (auto& [filepath, ns] : _curNSs) {
        ns->terminate();
    }
}

std::string Namespaces::toString(const std::string& filepath,
    const std::string& separator, bool withRoot) const
{
    if (const auto it = _curNSs.find(filepath); it != _curNSs.end()) {
        return it->second->toString(separator, withRoot);
    }
    return "";
}

Namespaces::Namespace::Namespace(
    const std::string& filepath) : _filepath(filepath), _nss({})
{
    _header.open(_filepath, std::ios::app);
    if (_header.is_open()) {
        _header << '\n'
            << '\n';
        _header.flush();
    } else {
        throw std::runtime_error("Could not open file: " + _filepath);
    }

    const auto srcFilepath = getSourceFromHeader(_filepath);
    _source.open(srcFilepath, std::ios::app);
    if (!_source.is_open()) {
        throw std::runtime_error("Could not open file: " + srcFilepath);
    }

    _setRoot(false);
}

Namespaces::Namespace::~Namespace() {
    if (_header.is_open()) {
        _header.close();
    }
    if (_source.is_open()) {
        _source.close();
    }
}

std::string Namespaces::Namespace::type2CWDef(const clang::QualType& type) {
    if (type.isNull()) return "void";

    /* check if it a template */
    if (type->isTemplateTypeParmType()) {
        throw std::runtime_error("Template type is not supported: " +
            type.getAsString());
    }

    /* if the type is canonical, but not a class/struct/enum, return it as is */
    if (type.isCanonical() && !type->isRecordType()) {
        return type.getAsString();
    }

    /* check if it is a std type */
    {
        auto name = type.getAsString();
        bool isConst = false;
        if (name.find("const ") == 0) {
            name = name.substr(6);
            isConst = true;
        }
        if (name.find("std::") == 0) {
            name = name.substr(5);
        }
        for (const auto& stdType : _stdTypes) {
            if (name.substr(0, stdType.size()) == stdType) {
                return (isConst ? "const " : "") + name;
            }
        }
    }

    /* get the name with its namespaces */
    std::string name;
    clang::QualType actualType = type;
    int nPointer = 0;
    bool isConst = false;
    {
        auto str = actualType.getAsString();
        if (str.find("const ") == 0) {
            str = str.substr(6);
            isConst = true;
        }
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
            throw std::runtime_error("Reference type is not supported: " +
                type.getAsString());
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
        throw std::runtime_error("Unsupported type: " + type.getAsString());
    }
    const auto nss = Namespaces::_getFullName(ctx);

    /* check for const attribute */
    std::string res;
    if (isConst) {
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

    /* previous */
    const auto prevCWName = toString("_");

    _nss.push_back(name);

    /* new */
    const auto newCWName = toString("_");
    const auto newComment = toString("::", false);

    _header << "#undef CW_SPACE\n"
        << '\n'
        << '\n'
        << "/* ### " << newComment << " ### */\n"
        << "#ifndef CW_" << newCWName << '\n'
        << "#define CW_" << newCWName << " CW_BUILD_SPACE(CW_"
        << prevCWName << ", " << name << ")\n"
        << "#endif /* CW_" << newCWName << " */\n"
        << "#define CW_SPACE CW_" << newCWName << '\n'
        << '\n';
    _header.flush();

    _source << "#undef CW_SPACE\n"
        << '\n'
        << '\n'
        << "/* ### " << newComment << " ### */\n"
        << "#define CW_SPACE CW_" << newCWName << '\n'
        << '\n';
    _source.flush();
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
    } else if (i == nss.size()) {
        /* redefine the last one */
        const auto back = _nss.back();
        _nss.pop_back();
        push(back);
    } else {
        /* push some new ones */
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

    _header << "#undef CW_SPACE\n"
        << '\n'
        << '\n';
    _header.flush();
    _source << "#undef CW_SPACE\n"
        << '\n'
        << '\n';
    _source.flush();
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
    if (undefSpace) {
        _header << "#undef CW_SPACE\n"
            << '\n'
            << '\n';
        _source << "#undef CW_SPACE\n"
            << '\n'
            << '\n';
    }
    _header << "/* ### root ### */\n"
        << "#define CW_SPACE CW_root\n"
        << '\n';
    _header.flush();
    _source << "/* ### root ### */\n"
        << "#define CW_SPACE CW_root\n"
        << '\n';
    _source.flush();
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
                throw std::runtime_error("Unsupported context kind: " +
                    std::to_string(ctxKind) + " which is " +
                    ctx->getDeclKindName());
        }
        ctx = ctx->getParent()->getEnclosingNamespaceContext();
        ctxKind = ctx->getDeclKind();
    }
    std::reverse(nss.begin(), nss.end());
    return nss;
}
