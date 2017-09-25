/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "cppcanonicalsymbol.h"
#include "cppmodelmanager.h"
#include "cpprefactoringengine.h"
#include "cppsemanticinfo.h"
#include "cpptoolsreuse.h"

#include <texteditor/texteditor.h>

#include <utils/qtcassert.h>

namespace CppTools {

void CppRefactoringEngine::startLocalRenaming(const CursorInEditor &data,
                                              ProjectPart *,
                                              RenameCallback &&renameSymbolsCallback)
{
    CppEditorWidgetInterface *editorWidget = data.editorWidget();
    QTC_ASSERT(editorWidget, renameSymbolsCallback(QString(),
                                                   ClangBackEnd::SourceLocationsContainer(),
                                                   0); return;);
    editorWidget->updateSemanticInfo();
    // Call empty callback
    renameSymbolsCallback(QString(),
                          ClangBackEnd::SourceLocationsContainer(),
                          data.cursor().document()->revision());
}

void CppRefactoringEngine::globalRename(const CursorInEditor &data,
                                        UsagesCallback &&,
                                        const QString &replacement)
{
    CppModelManager *modelManager = CppModelManager::instance();
    if (!modelManager)
        return;

    CppEditorWidgetInterface *editorWidget = data.editorWidget();
    QTC_ASSERT(editorWidget, return;);

    SemanticInfo info = editorWidget->semanticInfo();
    info.snapshot = modelManager->snapshot();
    info.snapshot.insert(info.doc);
    const QTextCursor &cursor = data.cursor();
    if (const CPlusPlus::Macro *macro = findCanonicalMacro(cursor, info.doc)) {
        modelManager->renameMacroUsages(*macro, replacement);
    } else {
        CanonicalSymbol cs(info.doc, info.snapshot);
        CPlusPlus::Symbol *canonicalSymbol = cs(cursor);
        if (canonicalSymbol)
            modelManager->renameUsages(canonicalSymbol, cs.context(), replacement);
    }
}

void CppRefactoringEngine::findUsages(const CursorInEditor &data,
                                      UsagesCallback &&) const
{
    CppModelManager *modelManager = CppModelManager::instance();
    if (!modelManager)
        return;

    CppEditorWidgetInterface *editorWidget = data.editorWidget();
    QTC_ASSERT(editorWidget, return;);

    SemanticInfo info = editorWidget->semanticInfo();
    info.snapshot = modelManager->snapshot();
    info.snapshot.insert(info.doc);
    const QTextCursor &cursor = data.cursor();
    if (const CPlusPlus::Macro *macro = findCanonicalMacro(cursor, info.doc)) {
        modelManager->findMacroUsages(*macro);
    } else {
        CanonicalSymbol cs(info.doc, info.snapshot);
        CPlusPlus::Symbol *canonicalSymbol = cs(cursor);
        if (canonicalSymbol)
            modelManager->findUsages(canonicalSymbol, cs.context());
    }
}

} // namespace CppEditor
