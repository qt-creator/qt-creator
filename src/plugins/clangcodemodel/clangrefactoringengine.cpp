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

#include "clangdclient.h"
#include "clangmodelmanagersupport.h"

#include <cppeditor/cppmodelmanager.h>
#include <languageclient/languageclientsymbolsupport.h>
#include <utils/textutils.h>
#include <utils/qtcassert.h>

namespace ClangCodeModel {
namespace Internal {

void RefactoringEngine::startLocalRenaming(const CppEditor::CursorInEditor &data,
                                           const CppEditor::ProjectPart *projectPart,
                                           RenameCallback &&renameSymbolsCallback)
{
    ClangdClient * const client
            = ClangModelManagerSupport::instance()->clientForFile(data.filePath());
    if (client && client->reachable()) {
        client->findLocalUsages(data.textDocument(), data.cursor(),
                                std::move(renameSymbolsCallback));
        return;
    }

    CppEditor::CppModelManager::builtinRefactoringEngine()
            ->startLocalRenaming(data, projectPart, std::move(renameSymbolsCallback));
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
