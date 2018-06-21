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

#pragma once

#include "refactoringcompilationdatabase.h"

#include <clangrefactoringbackend_global.h>

#include <filecontainerv2.h>
#include <sourcelocationscontainer.h>

#include <clang/Tooling/Refactoring.h>

#include <utils/smallstring.h>

#include <string>
#include <vector>

namespace ClangBackEnd {

struct FileContent
{
    FileContent(const std::string &directory,
                const std::string &fileName,
                const std::string &content,
                const std::vector<std::string> &commandLine)
        : directory(directory),
          fileName(fileName),
          filePath(directory + nativeSeparator + fileName),
          content(content),
          commandLine(commandLine)
    {}

    std::string directory;
    std::string fileName;
    std::string filePath;
    std::string content;
    std::vector<std::string> commandLine;
};

struct UnsavedFileContent
{
    UnsavedFileContent(Utils::PathString &&filePath,
                       Utils::SmallString &&content)
        : filePath(std::move(filePath)),
          content(std::move(content))
    {}

    Utils::PathString filePath;
    Utils::SmallString content;
};

class ClangTool
{
public:
    void addFile(std::string &&directory,
                 std::string &&fileName,
                 std::string &&content,
                 std::vector<std::string> &&commandLine);

    template <typename Container>
    void addFiles(const Container &filePaths,
                  const Utils::SmallStringVector &arguments);
    void addFiles(const FilePaths &filePaths,
                  const Utils::SmallStringVector &arguments);


    void addUnsavedFiles(const V2::FileContainers &unsavedFiles);

    clang::tooling::ClangTool createTool() const;

private:
    RefactoringCompilationDatabase m_compilationDatabase;
    std::vector<FileContent> m_fileContents;
    std::vector<std::string> m_sourceFilePaths;
    std::vector<UnsavedFileContent> m_unsavedFileContents;
};

extern template
void ClangTool::addFiles<Utils::SmallStringVector>(const Utils::SmallStringVector &filePaths,
                                                   const Utils::SmallStringVector &arguments);
extern template
void ClangTool::addFiles<Utils::PathStringVector>(const Utils::PathStringVector &filePaths,
                                                  const Utils::SmallStringVector &arguments);

} // namespace ClangBackEnd
