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

#include "cppuseselectionsupdater.h"

#include "cppcanonicalsymbol.h"
#include "cppeditor.h"

#include <cpptools/cpplocalsymbols.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolsreuse.h>
#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>
#include <texteditor/convenience.h>
#include <texteditor/fontsettings.h>

#include <cplusplus/Macro.h>
#include <cplusplus/TranslationUnit.h>

#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QTextBlock>
#include <QTextCursor>
#include <QTextEdit>

using namespace CPlusPlus;

enum { updateUseSelectionsInternalInMs = 500 };

namespace {

class FunctionDefinitionUnderCursor: protected ASTVisitor
{
    unsigned _line;
    unsigned _column;
    DeclarationAST *_functionDefinition;

public:
    FunctionDefinitionUnderCursor(TranslationUnit *translationUnit)
        : ASTVisitor(translationUnit),
          _line(0), _column(0)
    { }

    DeclarationAST *operator()(AST *ast, unsigned line, unsigned column)
    {
        _functionDefinition = 0;
        _line = line;
        _column = column;
        accept(ast);
        return _functionDefinition;
    }

protected:
    virtual bool preVisit(AST *ast)
    {
        if (_functionDefinition)
            return false;

        if (FunctionDefinitionAST *def = ast->asFunctionDefinition())
            return checkDeclaration(def);

        if (ObjCMethodDeclarationAST *method = ast->asObjCMethodDeclaration()) {
            if (method->function_body)
                return checkDeclaration(method);
        }

        return true;
    }

private:
    bool checkDeclaration(DeclarationAST *ast)
    {
        unsigned startLine, startColumn;
        unsigned endLine, endColumn;
        getTokenStartPosition(ast->firstToken(), &startLine, &startColumn);
        getTokenEndPosition(ast->lastToken() - 1, &endLine, &endColumn);

        if (_line > startLine || (_line == startLine && _column >= startColumn)) {
            if (_line < endLine || (_line == endLine && _column < endColumn)) {
                _functionDefinition = ast;
                return false;
            }
        }

        return true;
    }
};

QTextEdit::ExtraSelection extraSelection(const QTextCharFormat &format, const QTextCursor &cursor)
{
    QTextEdit::ExtraSelection selection;
    selection.format = format;
    selection.cursor = cursor;
    return selection;
}

class Params
{
public:
    Params(const QTextCursor &textCursor, const Document::Ptr document, const Snapshot &snapshot)
        : document(document), snapshot(snapshot)
    {
        TextEditor::Convenience::convertPosition(textCursor.document(), textCursor.position(),
                                                 &line, &column);
        CppEditor::Internal::CanonicalSymbol canonicalSymbol(document, snapshot);
        scope = canonicalSymbol.getScopeAndExpression(textCursor, &expression);
    }

public:
    // Shared
    Document::Ptr document;

    // For local use calculation
    int line;
    int column;

    // For references calculation
    Scope *scope;
    QString expression;
    Snapshot snapshot;
};

using CppEditor::Internal::SemanticUses;

void splitLocalUses(const CppTools::SemanticInfo::LocalUseMap &uses,
                    const Params &p,
                    SemanticUses *selectionsForLocalVariableUnderCursor,
                    SemanticUses *selectionsForLocalUnusedVariables)
{
    QTC_ASSERT(selectionsForLocalVariableUnderCursor, return);
    QTC_ASSERT(selectionsForLocalUnusedVariables, return);

    LookupContext context(p.document, p.snapshot);

    CppTools::SemanticInfo::LocalUseIterator it(uses);
    while (it.hasNext()) {
        it.next();
        const SemanticUses &uses = it.value();

        bool good = false;
        foreach (const CppTools::SemanticInfo::Use &use, uses) {
            unsigned l = p.line;
            unsigned c = p.column + 1; // convertCursorPosition() returns a 0-based column number.
            if (l == use.line && c >= use.column && c <= (use.column + use.length)) {
                good = true;
                break;
            }
        }

        if (uses.size() == 1) {
            if (!CppTools::isOwnershipRAIIType(it.key(), context))
                selectionsForLocalUnusedVariables->append(uses); // unused declaration
        } else if (good && selectionsForLocalVariableUnderCursor->isEmpty()) {
            selectionsForLocalVariableUnderCursor->append(uses);
        }
    }
}

CppTools::SemanticInfo::LocalUseMap findLocalUses(const Params &p)
{
    AST *ast = p.document->translationUnit()->ast();
    FunctionDefinitionUnderCursor functionDefinitionUnderCursor(p.document->translationUnit());
    DeclarationAST *declaration = functionDefinitionUnderCursor(ast, p.line, p.column);
    return CppTools::LocalSymbols(p.document, declaration).uses;
}

QList<int> findReferences(const Params &p)
{
    QList<int> result;
    if (!p.scope || p.expression.isEmpty())
        return result;

    TypeOfExpression typeOfExpression;
    Snapshot snapshot = p.snapshot;
    snapshot.insert(p.document);
    typeOfExpression.init(p.document, snapshot);
    typeOfExpression.setExpandTemplates(true);

    using CppEditor::Internal::CanonicalSymbol;
    if (Symbol *s = CanonicalSymbol::canonicalSymbol(p.scope, p.expression, typeOfExpression)) {
        CppTools::CppModelManager *mmi = CppTools::CppModelManager::instance();
        result = mmi->references(s, typeOfExpression.context());
    }

    return result;
}

CppEditor::Internal::UseSelectionsResult findUses(const Params p)
{
    CppEditor::Internal::UseSelectionsResult result;

    const CppTools::SemanticInfo::LocalUseMap localUses = findLocalUses(p);
    result.localUses = localUses;
    splitLocalUses(localUses, p, &result.selectionsForLocalVariableUnderCursor,
                                 &result.selectionsForLocalUnusedVariables);

    if (!result.selectionsForLocalVariableUnderCursor.isEmpty())
        return result;

    result.references = findReferences(p);
    return result; // OK, result.selectionsForLocalUnusedVariables will be passed on
}

} // anonymous namespace

namespace CppEditor {
namespace Internal {

CppUseSelectionsUpdater::CppUseSelectionsUpdater(TextEditor::TextEditorWidget *editorWidget)
    : m_editorWidget(editorWidget)
    , m_findUsesRevision(-1)
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(updateUseSelectionsInternalInMs);
    connect(&m_timer, &QTimer::timeout, this, [this]() { update(); });
}

void CppUseSelectionsUpdater::scheduleUpdate()
{
    m_timer.start();
}

void CppUseSelectionsUpdater::abortSchedule()
{
    m_timer.stop();
}

void CppUseSelectionsUpdater::update(CallType callType)
{
    CppEditorWidget *cppEditorWidget = qobject_cast<CppEditorWidget *>(m_editorWidget);
    QTC_ASSERT(cppEditorWidget, return);
    if (!cppEditorWidget->isSemanticInfoValidExceptLocalUses())
        return;

    const CppTools::SemanticInfo semanticInfo = cppEditorWidget->semanticInfo();
    const Document::Ptr document = semanticInfo.doc;
    const Snapshot snapshot = semanticInfo.snapshot;

    if (!document)
        return;

    if (semanticInfo.revision != static_cast<unsigned>(textDocument()->revision()))
        return;

    QTC_ASSERT(document->translationUnit(), return);
    QTC_ASSERT(document->translationUnit()->ast(), return);
    QTC_ASSERT(!snapshot.isEmpty(), return);

    if (handleMacroCase(document)) {
        emit finished(CppTools::SemanticInfo::LocalUseMap());
        return;
    }

    if (callType == Asynchronous)
        handleSymbolCaseAsynchronously(document, snapshot);
    else
        handleSymbolCaseSynchronously(document, snapshot);
}

void CppUseSelectionsUpdater::onFindUsesFinished()
{
    QTC_ASSERT(m_findUsesWatcher, return);
    if (m_findUsesWatcher->isCanceled())
        return;
    if (m_findUsesRevision != textDocument()->revision())
        return;
    // Optimizable: If the cursor is still on the same identifier the results are valid.
    if (m_findUsesCursorPosition != m_editorWidget->position())
        return;

    processSymbolCaseResults(m_findUsesWatcher->result());

    m_findUsesWatcher.reset();
    m_document.reset();
}

bool CppUseSelectionsUpdater::handleMacroCase(const Document::Ptr document)
{
    const Macro *macro = CppTools::findCanonicalMacro(m_editorWidget->textCursor(), document);
    if (!macro)
        return false;

    const QTextCharFormat &occurrencesFormat = textCharFormat(TextEditor::C_OCCURRENCES);
    ExtraSelections selections;

    // Macro definition
    if (macro->fileName() == document->fileName()) {
        QTextCursor cursor(textDocument());
        cursor.setPosition(macro->utf16CharOffset());
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                            macro->nameToQString().size());

        selections.append(extraSelection(occurrencesFormat, cursor));
    }

    // Other macro uses
    foreach (const Document::MacroUse &use, document->macroUses()) {
        const Macro &useMacro = use.macro();
        if (useMacro.line() != macro->line()
                || useMacro.utf16CharOffset() != macro->utf16CharOffset()
                || useMacro.length() != macro->length()
                || useMacro.fileName() != macro->fileName())
            continue;

        QTextCursor cursor(textDocument());
        cursor.setPosition(use.utf16charsBegin());
        cursor.setPosition(use.utf16charsEnd(), QTextCursor::KeepAnchor);

        selections.append(extraSelection(occurrencesFormat, cursor));
    }

    updateUseSelections(selections);
    return true;
}

void CppUseSelectionsUpdater::handleSymbolCaseAsynchronously(const Document::Ptr document,
                                                             const Snapshot &snapshot)
{
    m_document = document;

    if (m_findUsesWatcher)
        m_findUsesWatcher->cancel();
    m_findUsesWatcher.reset(new QFutureWatcher<UseSelectionsResult>);
    connect(m_findUsesWatcher.data(), &QFutureWatcherBase::finished, this, &CppUseSelectionsUpdater::onFindUsesFinished);

    m_findUsesRevision = textDocument()->revision();
    m_findUsesCursorPosition = m_editorWidget->position();

    const Params params = Params(m_editorWidget->textCursor(), document, snapshot);
    m_findUsesWatcher->setFuture(Utils::runAsync(findUses, params));
}

void CppUseSelectionsUpdater::handleSymbolCaseSynchronously(const Document::Ptr document,
                                                            const Snapshot &snapshot)
{
    const Document::Ptr previousDocument = m_document;
    m_document = document;

    const Params params = Params(m_editorWidget->textCursor(), document, snapshot);
    const UseSelectionsResult result = findUses(params);
    processSymbolCaseResults(result);

    m_document = previousDocument;
}

void CppUseSelectionsUpdater::processSymbolCaseResults(const UseSelectionsResult &result)
{
    const bool hasUsesForLocalVariable = !result.selectionsForLocalVariableUnderCursor.isEmpty();
    const bool hasReferences = !result.references.isEmpty();

    ExtraSelections localVariableSelections;
    if (hasUsesForLocalVariable) {
        localVariableSelections = toExtraSelections(result.selectionsForLocalVariableUnderCursor,
                                                    TextEditor::C_OCCURRENCES);
        updateUseSelections(localVariableSelections);
    } else if (hasReferences) {
        const ExtraSelections selections = toExtraSelections(result.references,
                                                             TextEditor::C_OCCURRENCES);
        updateUseSelections(selections);
    } else {
        if (!currentUseSelections().isEmpty())
            updateUseSelections(ExtraSelections());
    }

    updateUnusedSelections(toExtraSelections(result.selectionsForLocalUnusedVariables,
                                             TextEditor::C_OCCURRENCES_UNUSED));

    emit selectionsForVariableUnderCursorUpdated(localVariableSelections);
    emit finished(result.localUses);
}

ExtraSelections CppUseSelectionsUpdater::toExtraSelections(const SemanticUses &uses,
                                                           TextEditor::TextStyle style) const
{
    ExtraSelections result;

    foreach (const CppTools::SemanticInfo::Use &use, uses) {
        if (use.isInvalid())
            continue;

        QTextDocument *document = textDocument();
        const int position = document->findBlockByNumber(use.line - 1).position() + use.column - 1;
        const int anchor = position + use.length;

        QTextEdit::ExtraSelection sel;
        sel.format = textCharFormat(style);
        sel.cursor = QTextCursor(document);
        sel.cursor.setPosition(anchor);
        sel.cursor.setPosition(position, QTextCursor::KeepAnchor);

        result.append(sel);
    }

    return result;
}

ExtraSelections CppUseSelectionsUpdater::toExtraSelections(const QList<int> &references,
                                                           TextEditor::TextStyle style) const
{
    ExtraSelections selections;

    QTC_ASSERT(m_document, return selections);

    foreach (int index, references) {
        unsigned line, column;
        TranslationUnit *unit = m_document->translationUnit();
        unit->getTokenPosition(index, &line, &column);

        if (column)
            --column;  // adjust the column position.

        const int len = unit->tokenAt(index).utf16chars();

        QTextCursor cursor(textDocument()->findBlockByNumber(line - 1));
        cursor.setPosition(cursor.position() + column);
        cursor.setPosition(cursor.position() + len, QTextCursor::KeepAnchor);

        selections.append(extraSelection(textCharFormat(style), cursor));
    }

    return selections;
}

QTextCharFormat CppUseSelectionsUpdater::textCharFormat(TextEditor::TextStyle category) const
{
    return m_editorWidget->textDocument()->fontSettings().toTextCharFormat(category);
}

QTextDocument *CppUseSelectionsUpdater::textDocument() const
{
    return m_editorWidget->document();
}

ExtraSelections CppUseSelectionsUpdater::currentUseSelections() const
{
    return m_editorWidget->extraSelections(
        TextEditor::TextEditorWidget::CodeSemanticsSelection);
}

void CppUseSelectionsUpdater::updateUseSelections(const ExtraSelections &selections)
{
    m_editorWidget->setExtraSelections(TextEditor::TextEditorWidget::CodeSemanticsSelection,
                                       selections);
}

void CppUseSelectionsUpdater::updateUnusedSelections(const ExtraSelections &selections)
{
    m_editorWidget->setExtraSelections(TextEditor::TextEditorWidget::UnusedSymbolSelection,
                                       selections);
}

} // namespace Internal
} // namespace CppEditor
