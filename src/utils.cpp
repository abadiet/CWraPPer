#include "utils.hpp"

#include <filesystem>
#include <iostream>
#include <fstream>


std::filesystem::path from, to;

void setupOutput(const std::string& from, const std::string& to) {
    ::from = std::filesystem::absolute(std::filesystem::path(from));
    ::to = std::filesystem::path(to);

    std::filesystem::remove_all(::to); /* Remove target folder */
    std::filesystem::create_directories(::to); /* Create target folder */

    /* Create CWraPPer.h in target folder */
    std::ofstream file(::to / "CWraPPer.h");
    if (file.is_open()) {
        file << "#ifndef CWRAPPER_CWRAPPER_H\n"
            << "#define CWRAPPER_CWRAPPER_H\n"
            << "\n"
            << "/* Parameters */\n"
            << "// #define CW_PREFIX pre /* uncomment to add a prefix to every name */\n"
            << "// #define CW_POSTFIX post /* uncomment to add a postfix to every name */\n"
            << "#define CW_SEPARATOR _ /* names' separator */\n"
            << "\n"
            << "/* Utils */\n"
            << "#define CW_CONCAT3(a, b, c) a ## b ## c\n"
            << "#define CW_CC3EX(a, b, c) CW_CONCAT3(a, b, c)\n"
            << "#define CW_BUILD_SPACE_RELATIVE(a, b) CW_CC3EX(a, CW_SEPARATOR, b)\n"
            << "#define CW_BUILD_SPACE_ABSOLUTE(b) b\n"
            << "#define CW_SWITCH_BUILD_SPACE(_1, _2, name, ...) name\n"
            << "#define CW_BUILD_SPACE(...) CW_SWITCH_BUILD_SPACE(__VA_ARGS__, CW_BUILD_SPACE_RELATIVE, CW_BUILD_SPACE_ABSOLUTE)(__VA_ARGS__)\n"
            << "\n"
            << "/* Name Retrieving */\n"
            << "#if defined(CW_PREFIX) && defined(CW_POSTFIX)\n"
            << "    #define CW_NAME_SPACE() CW_CC3EX(CW_PREFIX, CW_SEPARATOR, CW_CC3EX(CW_SPACE, CW_SEPARATOR, CW_POSTFIX))\n"
            << "    #define CW_NAME_AT_SPACE(name) CW_CC3EX(CW_PREFIX, CW_SEPARATOR, CW_CC3EX(CW_SPACE, CW_SEPARATOR, CW_CC3EX(name, CW_SEPARATOR, CW_POSTFIX)))\n"
            << "    #define CW_NAME_WITH_SPACE(space, name) CW_CC3EX(CW_PREFIX, CW_SEPARATOR, CW_CC3EX(space, CW_SEPARATOR, CW_CC3EX(name, CW_SEPARATOR, CW_POSTFIX)))\n"
            << "#elif defined(CW_PREFIX)\n"
            << "    #define CW_NAME_SPACE() CW_CC3EX(CW_PREFIX, CW_SEPARATOR, CW_SPACE)\n"
            << "    #define CW_NAME_AT_SPACE(name) CW_CC3EX(CW_PREFIX, CW_SEPARATOR, CW_CC3EX(CW_SPACE, CW_SEPARATOR, name))\n"
            << "    #define CW_NAME_WITH_SPACE(space, name) CW_CC3EX(CW_PREFIX, CW_SEPARATOR, CW_CC3EX(space, CW_SEPARATOR, name))\n"
            << "#elif defined(CW_POSTFIX)\n"
            << "    #define CW_NAME_SPACE() CW_CC3EX(CW_SPACE, CW_SEPARATOR, CW_POSTFIX)\n"
            << "    #define CW_NAME_AT_SPACE(name) CW_CC3EX(CW_SPACE, CW_SEPARATOR, CW_CC3EX(name, CW_SEPARATOR, CW_POSTFIX))\n"
            << "    #define CW_NAME_WITH_SPACE(space, name) CW_CC3EX(space, CW_SEPARATOR, CW_CC3EX(name, CW_SEPARATOR, CW_POSTFIX))\n"
            << "#else\n"
            << "    #define CW_NAME_SPACE() CW_SPACE\n"
            << "    #define CW_NAME_AT_SPACE(name) CW_CC3EX(CW_SPACE, CW_SEPARATOR, name)\n"
            << "    #define CW_NAME_WITH_SPACE(space, name) CW_CC3EX(space, CW_SEPARATOR, name)\n"
            << "#endif\n"
            << "#define CW_SWITCH_NAME(_1, _2, name, ...) name\n"
            << "#define CW(...) CW_SWITCH_NAME(__VA_OPT__(__VA_ARGS__,) CW_NAME_WITH_SPACE, CW_NAME_AT_SPACE, CW_NAME_SPACE)(__VA_ARGS__)\n"
            << "\n"
            << "#endif /* CWRAPPER_CWRAPPER_H */\n";
        file.close();
    } else {
        std::cerr << "Could not create file: " << (::to / "CWraPPer.h").string()
            << std::endl;
    }
}

void terminateOutput(matcher::NamespaceDefinition& nsDef) {
    /* close namespaces */
    nsDef.terminate();

    /* For each .cpp files in "to" */
    for (const auto& entry : std::filesystem::recursive_directory_iterator(to)) {
        if (entry.path().extension() == ".cpp") {
            std::ofstream file(entry.path(), std::ios::app);
            if (file.is_open()) {
                file << "#ifdef __cplusplus\n"
                    << "}\n"
                    << "#endif /* __cplusplus */\n";
                file.close();
            } else {
                std::cerr << "Could not open file: " << entry.path().string()
                    << std::endl;
            }
        }
    }

    /* For each .cpp files in "to" */
    for (const auto& entry : std::filesystem::recursive_directory_iterator(to)) {
        if (
            entry.path().extension() == ".h" &&
            entry.path().filename() != "CWraPPer.h"
        ) {
            std::ofstream file(entry.path(), std::ios::app);
            if (file.is_open()) {
                auto name = entry.path().filename().string();
                std::replace(name.begin(), name.end(), '.', '_');
                std::replace(name.begin(), name.end(), ' ', '_');
                std::transform(name.begin(), name.end(), name.begin(), ::toupper);
                file << "#ifdef __cplusplus\n"
                    << "}\n"
                    << "#endif /* __cplusplus */\n"
                    << "\n"
                    << "#endif /* CWRAPPER_" << name << " */\n";
                file.close();
            } else {
                std::cerr << "Could not open file: " << entry.path().string()
                    << std::endl;
            }
        }
    }
}

std::string getNewFilePath(const std::string& origin) {
    const auto orig = std::filesystem::path(origin);

    /* Get relative path from 'from' */
    const auto rela = std::filesystem::relative(orig, from);

    /* Create new path from 'to' */
    auto result = to / rela;

    /* Change .hpp extensions to .h */
    if (result.extension() == ".hpp") {
        result.replace_extension(".h");
    }

    return result.string();
}

void setupHeaderFile(const std::string& filePath, const std::string& origin) {
    const auto p = std::filesystem::path(filePath);

    /* Check if .h file */
    if (p.extension() != ".h") {
        return;
    }

    /* Create directories */
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }

    /* Create file if it does not exist */
    if (!std::filesystem::exists(p)) {
        std::ofstream file(filePath);
        if (file.is_open()) {
            const auto pathToRoot = std::filesystem::relative(
                to, p.parent_path()).string() + "/";
            file << "#pragma once\n"
                << "\n"
                << "#include \"" << pathToRoot << "CWraPPer.h\"\n"
                << "\n"
                << "#ifdef __cplusplus\n"
                << "extern \"C\" {\n"
                << "#endif /* __cplusplus */\n"
                << "\n"
                << "\n";
            file.close();
        } else {
            std::cerr << "Could not create file: " << filePath << std::endl;
        }
    }

    /* Create its .c pair if it does not exist */
    {
        auto cPath = p;
        cPath.replace_extension(".cpp");
        if (!std::filesystem::exists(cPath)) {
            std::ofstream file(cPath);
            if (file.is_open()) {
                file << "#include \"" << p.filename().string() << "\"\n"
                    << "#include \"" << origin << "\"\n"
                    << "\n"
                    << "#ifdef __cplusplus\n"
                    << "extern \"C\" {\n"
                    << "#endif /* __cplusplus */\n"
                    << "\n"
                    << "\n";
                file.close();
            } else {
                std::cerr << "Could not create file: " << cPath.string()
                    << std::endl;
            }
        }
    }
}
