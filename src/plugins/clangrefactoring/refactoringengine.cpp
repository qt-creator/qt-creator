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

#include "projectpartutilities.h"


#include <refactoringserverinterface.h>
#include <requestsourcelocationforrenamingmessage.h>

#include <cpptools/clangcompileroptionsbuilder.h>
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

void RefactoringEngine::startLocalRenaming(const QTextCursor &textCursor,
                                           const Utils::FileName &filePath,
                                           int revision,
                                           CppTools::ProjectPart *projectPart,
                                           RenameCallback &&renameSymbolsCallback)
{
    using CppTools::ClangCompilerOptionsBuilder;

    setUsable(false);

    client.setLocalRenamingCallback(std::move(renameSymbolsCallback));

    Utils::SmallStringVector commandLine{ClangCompilerOptionsBuilder::build(
                    projectPart,
                    fileKindInProjectPart(projectPart, filePath.toString()),
                    CppTools::getPchUsage(),
                    CLANG_VERSION,
                    CLANG_RESOURCE_DIR)};

    commandLine.push_back(filePath.toString());

    RequestSourceLocationsForRenamingMessage message(ClangBackEnd::FilePath(filePath.toString()),
                                                     uint(textCursor.blockNumber() + 1),
                                                     uint(textCursor.positionInBlock() + 1),
                                                     textCursor.document()->toPlainText(),
                                                     std::move(commandLine),
                                                     revision);


    server.requestSourceLocationsForRenamingMessage(std::move(message));
}

bool RefactoringEngine::isUsable() const
{
    return server.isUsable();
}

void RefactoringEngine::setUsable(bool isUsable)
{
    server.setUsable(isUsable);
}

} // namespace ClangRefactoring
