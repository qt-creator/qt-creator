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

#include <iostream>

#include <nativefilepath.h>

namespace ClangBackEnd {

void ClangTool::addFile(FilePath &&filePath,
                        Utils::SmallString &&content,
                        Utils::SmallStringVector &&commandLine)
{
    NativeFilePath nativeFilePath{filePath};

    if (commandLine.back() != nativeFilePath.path())
        commandLine.emplace_back(nativeFilePath.path());

    m_compilationDatabase.addFile(nativeFilePath, std::move(commandLine));
    m_sourceFilePaths.push_back(Utils::SmallStringView{nativeFilePath});

    m_fileContents.emplace_back(std::move(nativeFilePath), std::move(content));
}

void ClangTool::addFiles(const FilePaths &filePaths, const Utils::SmallStringVector &arguments)
{
    for (const FilePath &filePath : filePaths)
        addFile(filePath.clone(), {}, arguments.clone());
}

void ClangTool::addUnsavedFiles(const V2::FileContainers &unsavedFiles)
{
    m_unsavedFileContents.reserve(m_unsavedFileContents.size() + unsavedFiles.size());

    auto convertToUnsavedFileContent = [](const V2::FileContainer &unsavedFile) {
        return UnsavedFileContent{NativeFilePath{unsavedFile.filePath},
                                  unsavedFile.unsavedFileContent.clone()};
    };

    std::transform(unsavedFiles.begin(),
                   unsavedFiles.end(),
                   std::back_inserter(m_unsavedFileContents),
                   convertToUnsavedFileContent);
}

namespace {
template<typename String>
llvm::StringRef toStringRef(const String &string)
{
    return llvm::StringRef(string.data(), string.size());
}

llvm::StringRef toStringRef(const NativeFilePath &path)
{
    return llvm::StringRef(path.path().data(), path.path().size());
}
} // namespace

clang::tooling::ClangTool ClangTool::createTool() const
{
    clang::tooling::ClangTool tool(m_compilationDatabase, m_sourceFilePaths);

    for (const auto &fileContent : m_fileContents) {
        if (fileContent.content.hasContent())
            tool.mapVirtualFile(toStringRef(fileContent.filePath), toStringRef(fileContent.content));
    }

    for (const auto &unsavedFileContent : m_unsavedFileContents) {
        tool.mapVirtualFile(toStringRef(unsavedFileContent.filePath),
                            toStringRef(unsavedFileContent.content));
    }

    tool.mapVirtualFile("/dummyFile", "#pragma once");

    return tool;
}

clang::tooling::ClangTool ClangTool::createOutputTool() const
{
    clang::tooling::ClangTool tool = createTool();

    tool.clearArgumentsAdjusters();

    return tool;
}

bool ClangTool::isClean() const
{
    return m_sourceFilePaths.empty() && m_fileContents.empty()
           && m_compilationDatabase.getAllFiles().empty() && m_unsavedFileContents.empty();
}

} // namespace ClangBackEnd
