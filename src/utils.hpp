#ifndef CWRAPPER_UTILS_HPP
#define CWRAPPER_UTILS_HPP

#include <string>
#include "Namespaces.hpp"


void setupOutput(const std::string& from, const std::string& to);

void terminateOutput(Namespaces& namespaces);

std::string getNewFilePath(const std::string& origin);

void setupHeaderFile(const std::string& filePath, const std::string& origin);

std::string normalizeFileName(const std::string& name);

std::string getSourceFromHeader(const std::string& header);

#endif /* CWRAPPER_UTILS_HPP */
