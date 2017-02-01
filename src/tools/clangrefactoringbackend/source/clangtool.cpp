/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "clangtool.h"

namespace ClangBackEnd {

namespace {

// use std::filesystem::path if it is supported by all compilers

template <typename String>
String toNativePath(String &&path)
{
#ifdef _WIN32
    std::replace(path.begin(), path.end(), '/', '\\');
#endif

    return std::move(path);
}
}

void ClangTool::addFile(std::string &&directory,
                           std::string &&fileName,
                           std::string &&content,
                           std::vector<std::string> &&commandLine)
{
    fileContents.emplace_back(toNativePath(std::move(directory)),
                              std::move(fileName),
                              std::move(content),
                              std::move(commandLine));

    const auto &fileContent = fileContents.back();

    compilationDatabase.addFile(fileContent.directory, fileContent.fileName, fileContent.commandLine);
    sourceFilePaths.push_back(fileContent.filePath);
}

template <typename Container>
void ClangTool::addFiles(const Container &filePaths,
                         const Utils::SmallStringVector &arguments)
{
    for (const typename Container::value_type &filePath : filePaths) {
        auto found = std::find(filePath.rbegin(), filePath.rend(), '/');

        auto fileNameBegin = found.base();

        std::vector<std::string> commandLine(arguments.begin(), arguments.end());
        commandLine.push_back(filePath);

        addFile({filePath.begin(), std::prev(fileNameBegin)},
                {fileNameBegin, filePath.end()},
                {},
                std::move(commandLine));
    }
}

template
void ClangTool::addFiles<Utils::SmallStringVector>(const Utils::SmallStringVector &filePaths,
                                                   const Utils::SmallStringVector &arguments);
template
void ClangTool::addFiles<Utils::PathStringVector>(const Utils::PathStringVector &filePaths,
                                                  const Utils::SmallStringVector &arguments);

namespace {
Utils::SmallString toNativeFilePath(const FilePath &filePath)
{
    Utils::SmallString filePathString = filePath.directory().clone();
    filePathString.append("/");
    filePathString.append(filePath.name());

    return toNativePath(std::move(filePathString));
}
}

void ClangTool::addUnsavedFiles(const V2::FileContainers &unsavedFiles)
{
    unsavedFileContents.reserve(unsavedFileContents.size() + unsavedFiles.size());

    auto convertToUnsavedFileContent = [] (const V2::FileContainer &unsavedFile) {
        return UnsavedFileContent{toNativeFilePath(unsavedFile.filePath()),
                                  unsavedFile.unsavedFileContent().clone()};
    };

    std::transform(unsavedFiles.begin(),
                   unsavedFiles.end(),
                   std::back_inserter(unsavedFileContents),
                   convertToUnsavedFileContent);
}

namespace {
llvm::StringRef toStringRef(const Utils::SmallString &string)
{
    return llvm::StringRef(string.data(), string.size());
}
}

clang::tooling::ClangTool ClangTool::createTool() const
{
    clang::tooling::ClangTool tool(compilationDatabase, sourceFilePaths);

    for (const auto &fileContent : fileContents) {
        if (!fileContent.content.empty())
            tool.mapVirtualFile(fileContent.filePath, fileContent.content);
    }

    for (const auto &unsavedFileContent : unsavedFileContents)
            tool.mapVirtualFile(toStringRef(unsavedFileContent.filePath),
                                toStringRef(unsavedFileContent.content));

    return tool;
}

} // namespace ClangBackEnd
