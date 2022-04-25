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

#include "clangrefactoringengine.h"
#include "clangeditordocumentprocessor.h"

#include "clangdclient.h"
#include "clangmodelmanagersupport.h"
#include "sourcelocationscontainer.h"

#include <cppeditor/cppmodelmanager.h>
#include <languageclient/languageclientsymbolsupport.h>
#include <utils/textutils.h>
#include <utils/qtcassert.h>

namespace ClangCodeModel {
namespace Internal {

void RefactoringEngine::startLocalRenaming(const CppEditor::CursorInEditor &data,
                                           const CppEditor::ProjectPart *,
                                           RenameCallback &&renameSymbolsCallback)
{
    ClangdClient * const client
            = ClangModelManagerSupport::instance()->clientForFile(data.filePath());
    if (client && client->reachable()) {
        client->findLocalUsages(data.textDocument(), data.cursor(),
                                std::move(renameSymbolsCallback));
        return;
    }

    ClangEditorDocumentProcessor *processor = ClangEditorDocumentProcessor::get(
        data.filePath().toString());
    const int startRevision = data.cursor().document()->revision();

    using ClangBackEnd::SourceLocationsContainer;
    auto defaultCallback = [renameSymbolsCallback, startRevision]() {
        return renameSymbolsCallback(QString(), SourceLocationsContainer{}, startRevision);
    };

    if (!processor)
        return defaultCallback();

    QFuture<CppEditor::CursorInfo> cursorFuture = processor->requestLocalReferences(data.cursor());
    if (cursorFuture.isCanceled())
        return defaultCallback();

    if (m_watcher)
        m_watcher->cancel();

    m_watcher.reset(new FutureCursorWatcher());
    QObject::connect(m_watcher.get(), &FutureCursorWatcher::finished, [=]() {
        if (m_watcher->isCanceled())
            return defaultCallback();
        const CppEditor::CursorInfo info = m_watcher->result();
        if (info.useRanges.empty())
            return defaultCallback();

        QTextCursor cursor = Utils::Text::wordStartCursor(data.cursor());
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                            info.useRanges.first().length);
        const QString symbolName = cursor.selectedText();
        ClangBackEnd::SourceLocationsContainer container;
        for (auto& use : info.useRanges) {
            container.insertSourceLocation({},
                                           use.line,
                                           use.column);
        }
        renameSymbolsCallback(symbolName, container, data.cursor().document()->revision());
    });

    m_watcher->setFuture(cursorFuture);
}

void RefactoringEngine::globalRename(const CppEditor::CursorInEditor &cursor,
                                     CppEditor::UsagesCallback &&callback,
                                     const QString &replacement)
{
    ClangdClient * const client
            = ClangModelManagerSupport::instance()->clientForFile(cursor.filePath());
    if (!client || !client->isFullyIndexed()) {
        CppEditor::CppModelManager::builtinRefactoringEngine()
                ->globalRename(cursor, std::move(callback), replacement);
        return;
    }
    QTC_ASSERT(client->documentOpen(cursor.textDocument()),
               client->openDocument(cursor.textDocument()));
    client->findUsages(cursor.textDocument(), cursor.cursor(), replacement);
}

void RefactoringEngine::findUsages(const CppEditor::CursorInEditor &cursor,
                                   CppEditor::UsagesCallback &&callback) const
{
    ClangdClient * const client
            = ClangModelManagerSupport::instance()->clientForFile(cursor.filePath());
    if (!client || !client->isFullyIndexed()) {
        CppEditor::CppModelManager::builtinRefactoringEngine()
                ->findUsages(cursor, std::move(callback));
        return;
    }
    QTC_ASSERT(client->documentOpen(cursor.textDocument()),
               client->openDocument(cursor.textDocument()));
    client->findUsages(cursor.textDocument(), cursor.cursor(), {});
}

} // namespace Internal
} // namespace ClangCodeModel
