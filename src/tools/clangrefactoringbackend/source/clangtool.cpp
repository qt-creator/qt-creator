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

std::string toNativePath(std::string &&path)
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

void ClangTool::addFiles(const Utils::SmallStringVector &filePaths,
                         const Utils::SmallStringVector &arguments)
{
    for (const Utils::SmallString &filePath : filePaths) {
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

clang::tooling::ClangTool ClangTool::createTool() const
{
    clang::tooling::ClangTool tool(compilationDatabase, sourceFilePaths);

    for (auto &&fileContent : fileContents) {
        if (!fileContent.content.empty())
            tool.mapVirtualFile(fileContent.filePath, fileContent.content);
    }

    return tool;
}

} // namespace ClangBackEnd
