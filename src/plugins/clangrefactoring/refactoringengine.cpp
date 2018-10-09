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

#include <filepath.h>
#include <refactoringserverinterface.h>
#include <requestsourcelocationforrenamingmessage.h>

#include <cpptools/compileroptionsbuilder.h>
#include <cpptools/cpptoolsreuse.h>

#include <clangsupport/filepathcachinginterface.h>

#include <utils/algorithm.h>
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
    CompilerOptionsBuilder optionsBuilder{*projectPart, CppTools::UseSystemHeader::Yes};
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
    CppTools::Usages usages;

    QTextCursor cursor = Utils::Text::wordStartCursor(data.cursor());
    Utils::OptionalLineColumn lineColumn = Utils::Text::convertPosition(cursor.document(),
                                                                        cursor.position());

    if (lineColumn) {
        const QByteArray filePath = data.filePath().toString().toUtf8();
        const ClangBackEnd::FilePathId filePathId = m_filePathCache.filePathId(ClangBackEnd::FilePathView(filePath));

        usages = m_symbolQuery.sourceUsagesAt(filePathId, lineColumn->line, lineColumn->column);
    }

    return usages;
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

void RefactoringEngine::globalFollowSymbol(const CppTools::CursorInEditor &data,
                                           Utils::ProcessLinkCallback &&processLinkCallback,
                                           const CPlusPlus::Snapshot &,
                                           const CPlusPlus::Document::Ptr &,
                                           CppTools::SymbolFinder *,
                                           bool) const
{
    // TODO: replace that with specific followSymbol query
    const CppTools::Usages usages = locationsAt(data);
    CppTools::Usage usage = Utils::findOrDefault(usages, [&data](const CppTools::Usage &usage) {
        // We've already searched in the current file, skip it.
        if (usage.path == data.filePath().toString())
            return false;
        return true;
    });

    processLinkCallback(Link(usage.path, usage.line, usage.column));
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
