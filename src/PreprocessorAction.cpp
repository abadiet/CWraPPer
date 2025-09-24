#include "PreprocessorAction.hpp"
#include "utils.hpp"
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Basic/SourceLocation.h>
#include <clang/Lex/Preprocessor.h>
#include <iostream>
#include <filesystem>
#include <memory>
#include <fstream>


class PreprocessorFinder : public clang::PPCallbacks {
public:
    inline PreprocessorFinder(const clang::CompilerInstance& compiler)
        : _compiler(compiler) {}

    void InclusionDirective(clang::SourceLocation HashLoc,
        const clang::Token &IncludeTok, clang::StringRef FileName,
        bool IsAngled, clang::CharSourceRange FilenameRange,
        clang::OptionalFileEntryRef File, clang::StringRef SearchPath,
        clang::StringRef RelativePath, const clang::Module *SuggestedModule,
        bool ModuleImported, clang::SrcMgr::CharacteristicKind FileType)
        override;

private:
    const clang::CompilerInstance& _compiler;
};

void PreprocessorFinder::InclusionDirective(clang::SourceLocation HashLoc,
    const clang::Token &IncludeTok,
    clang::StringRef FileName,
    bool IsAngled,
    clang::CharSourceRange FilenameRange,
    clang::OptionalFileEntryRef File,
    clang::StringRef SearchPath,
    clang::StringRef RelativePath,
    const clang::Module *SuggestedModule,
    bool ModuleImported, clang::SrcMgr::CharacteristicKind FileType)
{
    const auto from = _compiler.getSourceManager().getFilename(HashLoc);
    if (
        !from.empty() &&
        (from.ends_with(".h") || from.ends_with(".hpp")) &&
        from.starts_with("/Users")
    ) { /* TODO: too dirty */
        const auto to = getNewFilePath(from.str());

#ifdef CWRAPPER_DEBUG
        std::cout << "Include: " << FileName.str()
            << " from " << from.str() << " at "
            << _compiler.getSourceManager().getSpellingLineNumber(HashLoc)
            << ":"
            << _compiler.getSourceManager().getSpellingColumnNumber(HashLoc)
            << " ---> " << to << std::endl;
#endif /* CWRAPPER_DEBUG */

        setupHeaderFile(to, from.str());

        std::ofstream file(to, std::ios::app);
        if (file.is_open()) {
            file << "#include ";
            if (IsAngled) {
                file << "<" << FileName.str() << ">\n";
            } else {
                file << "\"" << FileName.str() << "\"\n";
            }
            file.close();
        } else {
            std::cerr << "Could not open file: " << to << std::endl;
        }

    }
}

void PreprocessorAction::ExecuteAction() {
    std::unique_ptr<PreprocessorFinder> find_includes_callback(
        new PreprocessorFinder(getCompilerInstance()));
    getCompilerInstance().getPreprocessor().addPPCallbacks(
        std::move(find_includes_callback));
    PreprocessOnlyAction::ExecuteAction();
}
