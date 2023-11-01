// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppquickfixassistant.h"

#include "cppeditorconstants.h"
#include "cppeditorwidget.h"
#include "cppmodelmanager.h"
#include "cppquickfixes.h"
#include "cpprefactoringchanges.h"

#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/iassistprocessor.h>
#include <texteditor/textdocument.h>

#include <cplusplus/ASTPath.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

using namespace CPlusPlus;
using namespace TextEditor;

namespace CppEditor {
namespace Internal {

// -------------------------
// CppQuickFixAssistProcessor
// -------------------------
class CppQuickFixAssistProcessor : public IAssistProcessor
{
    IAssistProposal *perform() override
    {
        return GenericProposal::createProposal(interface(), quickFixOperations(interface()));
    }
};

// -------------------------
// CppQuickFixAssistProvider
// -------------------------

IAssistProcessor *CppQuickFixAssistProvider::createProcessor(const AssistInterface *) const
{
    return new CppQuickFixAssistProcessor;
}

// --------------------------
// CppQuickFixAssistInterface
// --------------------------
CppQuickFixInterface::CppQuickFixInterface(CppEditorWidget *editor, AssistReason reason)
    : AssistInterface(editor->textCursor(), editor->textDocument()->filePath(), reason)
    , m_editor(editor)
    , m_semanticInfo(editor->semanticInfo())
    , m_snapshot(CppModelManager::snapshot())
    , m_currentFile(CppRefactoringChanges::file(editor, m_semanticInfo.doc))
    , m_context(m_semanticInfo.doc, m_snapshot)
{
    QTC_CHECK(m_semanticInfo.doc);
    QTC_CHECK(m_semanticInfo.doc->translationUnit());
    QTC_CHECK(m_semanticInfo.doc->translationUnit()->ast());
    ASTPath astPath(m_semanticInfo.doc);
    m_path = astPath(adjustedCursor());
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

// Some users like to select identifiers and expect the quickfix to apply to the selection.
// However, as the cursor position is at the end of the selection, it can happen that
// the quickfix is applied to the following token instead; see e.g. QTCREATORBUG-27886.
// We try to detect this condition: If there is a selection *and* this selection
// corresponds to a C++ token, we move the cursor to that token's position.
QTextCursor CppQuickFixInterface::adjustedCursor()
{
    QTextCursor cursor = this->cursor();
    if (!cursor.hasSelection())
        return cursor;

    const TranslationUnit * const tu = m_semanticInfo.doc->translationUnit();
    const int selStart = cursor.selectionStart();
    const int selEnd = cursor.selectionEnd();
    const QTextDocument * const doc = m_editor->textDocument()->document();

    // Binary search for matching token.
    for (int l = 0, u = tu->tokenCount() - 1; l <= u; ) {
        const int i = (l + u) / 2;
        const int tokenPos = tu->getTokenPositionInDocument(i, doc);
        if (selStart < tokenPos) {
            u = i - 1;
            continue;
        }
        if (selStart > tokenPos) {
            l = i + 1;
            continue;
        }

        // Selection does not end at token end.
        if (tokenPos + tu->tokenAt(i).utf16chars() != selEnd)
            break;

        cursor.setPosition(selStart);

        // Try not to have the cursor "at the edge", in order to prevent potential ambiguities.
        if (selEnd - selStart > 1)
            cursor.setPosition(cursor.position() + 1);

        return cursor;
    }
    return cursor;
}

QuickFixOperations quickFixOperations(const TextEditor::AssistInterface *interface)
{
    const auto cppInterface = dynamic_cast<const CppQuickFixInterface *>(interface);
    if (!cppInterface)
        return {};
    QuickFixOperations quickFixes;
    for (CppQuickFixFactory *factory : CppQuickFixFactory::cppQuickFixFactories())
        factory->match(*cppInterface, quickFixes);
    return quickFixes;
}

} // namespace Internal
} // namespace CppEditor
