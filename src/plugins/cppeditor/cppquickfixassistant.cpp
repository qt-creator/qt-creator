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

#include "cppquickfixassistant.h"

#include "cppeditorconstants.h"
#include "cppeditorwidget.h"
#include "cppquickfixes.h"

#include <cpptools/cppmodelmanager.h>
#include <texteditor/textdocument.h>

#include <cplusplus/ASTPath.h>

#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

using namespace TextEditor;
using namespace CppTools;
using namespace CPlusPlus;

namespace CppEditor {
namespace Internal {

// -------------------------
// CppQuickFixAssistProvider
// -------------------------
IAssistProvider::RunType CppQuickFixAssistProvider::runType() const
{
    return Synchronous;
}

IAssistProcessor *CppQuickFixAssistProvider::createProcessor() const
{
    return new QuickFixAssistProcessor(this);
}

QList<QuickFixFactory *> CppQuickFixAssistProvider::quickFixFactories() const
{
    QList<QuickFixFactory *> results;
    foreach (CppQuickFixFactory *f, ExtensionSystem::PluginManager::getObjects<CppQuickFixFactory>())
        results.append(f);
    return results;
}

// --------------------------
// CppQuickFixAssistInterface
// --------------------------
CppQuickFixInterface::CppQuickFixInterface(CppEditorWidget *editor,
                                                       AssistReason reason)
    : AssistInterface(editor->document(), editor->position(),
                      editor->textDocument()->filePath().toString(), reason)
    , m_editor(editor)
    , m_semanticInfo(editor->semanticInfo())
    , m_snapshot(CppModelManager::instance()->snapshot())
    , m_currentFile(CppRefactoringChanges::file(editor, m_semanticInfo.doc))
    , m_context(m_semanticInfo.doc, m_snapshot)
{
    QTC_CHECK(m_semanticInfo.doc);
    QTC_CHECK(m_semanticInfo.doc->translationUnit());
    QTC_CHECK(m_semanticInfo.doc->translationUnit()->ast());
    ASTPath astPath(m_semanticInfo.doc);
    m_path = astPath(editor->textCursor());
}

const QList<AST *> &CppQuickFixInterface::path() const
{
    return m_path;
}

Snapshot CppQuickFixInterface::snapshot() const
{
    return m_snapshot;
}

SemanticInfo CppQuickFixInterface::semanticInfo() const
{
    return m_semanticInfo;
}

const LookupContext &CppQuickFixInterface::context() const
{
    return m_context;
}

CppEditorWidget *CppQuickFixInterface::editor() const
{
    return m_editor;
}

CppRefactoringFilePtr CppQuickFixInterface::currentFile() const
{
    return m_currentFile;
}

bool CppQuickFixInterface::isCursorOn(unsigned tokenIndex) const
{
    return currentFile()->isCursorOn(tokenIndex);
}

bool CppQuickFixInterface::isCursorOn(const AST *ast) const
{
    return currentFile()->isCursorOn(ast);
}

} // namespace Internal
} // namespace CppEditor
