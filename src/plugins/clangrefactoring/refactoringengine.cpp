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

#include "refactoringengine.h"

#include <refactoringcompileroptionsbuilder.h>

#include <refactoringserverinterface.h>
#include <requestsourcelocationforrenamingmessage.h>

#include <cpptools/cpptoolsreuse.h>

#include <QTextCursor>
#include <QTextDocument>

#include <algorithm>

namespace ClangRefactoring {

using ClangBackEnd::RequestSourceLocationsForRenamingMessage;

RefactoringEngine::RefactoringEngine(ClangBackEnd::RefactoringServerInterface &server,
                                     ClangBackEnd::RefactoringClientInterface &client)
    : server(server),
      client(client)
{
}

namespace {

ClangBackEnd::FilePath convertToClangBackEndFilePath(const Utils::FileName &filePath)
{
    Utils::SmallString utf8FilePath = filePath.toString();

    auto found = std::find(utf8FilePath.rbegin(), utf8FilePath.rend(), '/').base();

    Utils::SmallString fileName(found, utf8FilePath.end());
    utf8FilePath.resize(std::size_t(std::distance(utf8FilePath.begin(), --found)));

    return ClangBackEnd::FilePath(std::move(utf8FilePath), std::move(fileName));
}

CppTools::ProjectFile::Kind fileKind(CppTools::ProjectPart *projectPart, const QString &filePath)
{
    const auto &projectFiles = projectPart->files;

    auto comparePaths = [&] (const CppTools::ProjectFile &projectFile) {
        return projectFile.path == filePath;
    };

    auto found = std::find_if(projectFiles.begin(), projectFiles.end(), comparePaths);

    if (found != projectFiles.end())
        return found->kind;

    return CppTools::ProjectFile::Unclassified;
}

}

void RefactoringEngine::startLocalRenaming(const QTextCursor &textCursor,
                                           const Utils::FileName &filePath,
                                           int revision,
                                           CppTools::ProjectPart *projectPart,
                                           RenameCallback &&renameSymbolsCallback)
{
    isUsable_ = false;

    client.setLocalRenamingCallback(std::move(renameSymbolsCallback));

    auto commandLine = RefactoringCompilerOptionsBuilder::build(projectPart,
                                                                fileKind(projectPart, filePath.toString()),
                                                                CppTools::getPchUsage());

    commandLine.push_back(filePath.toString());
    qDebug() << commandLine.join(" ");

    RequestSourceLocationsForRenamingMessage message(convertToClangBackEndFilePath(filePath),
                                                     uint(textCursor.blockNumber() + 1),
                                                     uint(textCursor.positionInBlock() + 1),
                                                     textCursor.document()->toPlainText(),
                                                     std::move(commandLine),
                                                     revision);


    server.requestSourceLocationsForRenamingMessage(std::move(message));
}

bool RefactoringEngine::isUsable() const
{
    return isUsable_;
}

void RefactoringEngine::setUsable(bool isUsable)
{
    isUsable_ = isUsable;
}

} // namespace ClangRefactoring
