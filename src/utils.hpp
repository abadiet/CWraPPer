#ifndef CWRAPPER_UTILS_HPP
#define CWRAPPER_UTILS_HPP

#include <string>
#include "matcher/NamespaceDefinition.hpp"

void setupOutput(const std::string& from, const std::string& to);

void terminateOutput(matcher::NamespaceDefinition& nsDef);

std::string getNewFilePath(const std::string& origin);

void setupHeaderFile(const std::string& filePath, const std::string& origin);

#endif /* CWRAPPER_UTILS_HPP */
