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

#include "refactoringcompilationdatabase.h"

#include "clangrefactoringbackend_global.h"

#include <filepath.h>
#include <nativefilepath.h>

namespace ClangBackEnd {

RefactoringCompilationDatabase::RefactoringCompilationDatabase()
{
}

std::vector<clang::tooling::CompileCommand>
RefactoringCompilationDatabase::getCompileCommands(llvm::StringRef filePath) const
{
    std::vector<clang::tooling::CompileCommand> foundCommands;

    std::copy_if(m_compileCommands.begin(),
                 m_compileCommands.end(),
                 std::back_inserter(foundCommands),
                 [&] (const clang::tooling::CompileCommand &compileCommand) {
        return filePath == compileCommand.Filename;
    });

    return foundCommands;
}

std::vector<std::string>
RefactoringCompilationDatabase::getAllFiles() const
{
    std::vector<std::string> filePaths;
    filePaths.reserve(m_compileCommands.size());

    std::transform(m_compileCommands.begin(),
                   m_compileCommands.end(),
                   std::back_inserter(filePaths),
                   [&] (const clang::tooling::CompileCommand &compileCommand) {
          return compileCommand.Filename;
      });

    return filePaths;
}

std::vector<clang::tooling::CompileCommand>
RefactoringCompilationDatabase::getAllCompileCommands() const
{
    return m_compileCommands;
}

void RefactoringCompilationDatabase::addFile(NativeFilePathView filePath,
                                             Utils::SmallStringVector &&commandLine)
{
    m_compileCommands.emplace_back(std::string(filePath.directory()),
                                   std::string(filePath),
                                   std::vector<std::string>(commandLine),
                                   llvm::StringRef());
}

} // namespace ClangBackEnd
