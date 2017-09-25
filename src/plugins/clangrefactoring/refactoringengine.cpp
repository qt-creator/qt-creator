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

#include <cpptools/compileroptionsbuilder.h>
#include <cpptools/cpptoolsreuse.h>

#include <clangsupport/filepathcachinginterface.h>

#include <utils/textutils.h>

#include <QTextCursor>
#include <QTextDocument>
#include <QTextBlock>
#include <QDir>

#include <algorithm>

namespace ClangRefactoring {

using ClangBackEnd::RequestSourceLocationsForRenamingMessage;

RefactoringEngine::RefactoringEngine(ClangBackEnd::RefactoringServerInterface &server,
                                     ClangBackEnd::RefactoringClientInterface &client,
                                     ClangBackEnd::FilePathCachingInterface &filePathCache,
                                     SymbolQueryInterface &symbolQuery)
    : m_server(server),
      m_client(client),
      m_filePathCache(filePathCache),
      m_symbolQuery(symbolQuery)
{
}

RefactoringEngine::~RefactoringEngine() = default;

void RefactoringEngine::startLocalRenaming(const CppTools::CursorInEditor &data,
                                           CppTools::ProjectPart *projectPart,
                                           RenameCallback &&renameSymbolsCallback)
{
    using CppTools::CompilerOptionsBuilder;

    setRefactoringEngineAvailable(false);

    m_client.setLocalRenamingCallback(std::move(renameSymbolsCallback));

    QString filePath = data.filePath().toString();
    QTextCursor textCursor = data.cursor();
    CompilerOptionsBuilder optionsBuilder{*projectPart, CLANG_VERSION, CLANG_RESOURCE_DIR};
    Utils::SmallStringVector commandLine{optionsBuilder.build(
                    fileKindInProjectPart(projectPart, filePath),
                    CppTools::getPchUsage())};

    commandLine.push_back(filePath);

    RequestSourceLocationsForRenamingMessage message(ClangBackEnd::FilePath(filePath),
                                                     uint(textCursor.blockNumber() + 1),
                                                     uint(textCursor.positionInBlock() + 1),
                                                     textCursor.document()->toPlainText(),
                                                     std::move(commandLine),
                                                     textCursor.document()->revision());


    m_server.requestSourceLocationsForRenamingMessage(std::move(message));
}

CppTools::Usages RefactoringEngine::locationsAt(const CppTools::CursorInEditor &data) const
{
    int line = 0, column = 0;
    QTextCursor cursor = Utils::Text::wordStartCursor(data.cursor());
    Utils::Text::convertPosition(cursor.document(), cursor.position(), &line, &column);

    const QByteArray filePath = data.filePath().toString().toLatin1();
    const ClangBackEnd::FilePathId filePathId = m_filePathCache.filePathId(filePath.constData());
    ClangRefactoring::SourceLocations usages = m_symbolQuery.locationsAt(filePathId, line,
                                                                         column + 1);
    CppTools::Usages result;
    result.reserve(usages.size());
    for (const auto &location : usages) {
        const Utils::SmallStringView path = m_filePathCache.filePath(location.filePathId).path();
        result.push_back({path, location.line, location.column});
    }
    return result;
}

void RefactoringEngine::globalRename(const CppTools::CursorInEditor &data,
                                     CppTools::UsagesCallback &&renameUsagesCallback,
                                     const QString &)
{
    renameUsagesCallback(locationsAt(data));
}

void RefactoringEngine::findUsages(const CppTools::CursorInEditor &data,
                                   CppTools::UsagesCallback &&showUsagesCallback) const
{
    showUsagesCallback(locationsAt(data));
}

bool RefactoringEngine::isRefactoringEngineAvailable() const
{
    return m_server.isAvailable();
}

void RefactoringEngine::setRefactoringEngineAvailable(bool isAvailable)
{
    m_server.setAvailable(isAvailable);
}

} // namespace ClangRefactoring
