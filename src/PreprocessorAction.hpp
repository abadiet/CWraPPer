#ifndef CWRAPPER_PREPROCESSORACTION_HPP
#define CWRAPPER_PREPROCESSORACTION_HPP

#include <clang/Frontend/FrontendActions.h>
#include <clang/Lex/PPCallbacks.h>


class PreprocessorAction : public clang::PreprocessOnlyAction {
protected:
    void ExecuteAction() override;

};

#endif /* CWRAPPER_PREPROCESSORACTION_HPP */
