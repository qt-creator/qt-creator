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

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined(_MSC_VER)
#    pragma warning(push)
#    pragma warning( disable : 4100 )
#endif

#include "clang/Tooling/CompilationDatabase.h"

#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

namespace ClangBackEnd {


class RefactoringCompilationDatabase : public clang::tooling::CompilationDatabase
{
public:
    RefactoringCompilationDatabase();

    std::vector<clang::tooling::CompileCommand> getCompileCommands(llvm::StringRef filePath) const override;
    std::vector<std::string> getAllFiles() const override;
    std::vector<clang::tooling::CompileCommand> getAllCompileCommands() const override;

    void addFile(const std::string &directory,
                            const std::string &fileName,
                            const std::vector<std::string> &commandLine);

private:
    std::vector<clang::tooling::CompileCommand> compileCommands;
};

} // namespace ClangBackEnd
