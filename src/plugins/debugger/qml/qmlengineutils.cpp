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

#include "qmlengine.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <debugger/console/console.h>

#include <coreplugin/editormanager/documentmodel.h>

#include <texteditor/fontsettings.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorsettings.h>

#include <QTextBlock>

using namespace Core;
using namespace QmlDebug;
using namespace QmlJS;
using namespace QmlJS::AST;
using namespace TextEditor;

namespace Debugger {
namespace Internal {

class ASTWalker : public Visitor
{
public:
    void operator()(Node *ast, quint32 *l, quint32 *c)
    {
        done = false;
        line = l;
        column = c;
        Node::accept(ast, this);
    }

    bool preVisit(Node *ast) override
    {
        return !done && ast->lastSourceLocation().startLine >= *line;
    }

    //Case 1: Breakpoint is between sourceStart(exclusive) and
    //        sourceEnd(inclusive) --> End tree walk.
    //Case 2: Breakpoint is on sourceStart --> Check for the start
    //        of the first executable code. Set the line number and
    //        column number. End tree walk.
    //Case 3: Breakpoint is on "unbreakable" code --> Find the next "breakable"
    //        code and check for Case 2. End tree walk.

    //Add more types when suitable.

    bool visit(UiScriptBinding *ast) override
    {
        if (!ast->statement)
            return true;

        quint32 sourceStartLine = ast->firstSourceLocation().startLine;
        quint32 statementStartLine;
        quint32 statementColumn;

        if (ast->statement->kind == Node::Kind_ExpressionStatement) {
            statementStartLine = ast->statement->firstSourceLocation().startLine;
            statementColumn = ast->statement->firstSourceLocation().startColumn;

        } else if (ast->statement->kind == Node::Kind_Block) {
            auto block = static_cast<Block *>(ast->statement);
            if (!block->statements)
                return true;
            statementStartLine = block->statements->firstSourceLocation().startLine;
            statementColumn = block->statements->firstSourceLocation().startColumn;

        } else {
            return true;
        }


        //Case 1
        //Check for possible relocation within the binding statement

        //Rewritten to (function <token>() { { }})
        //The offset 16 is position of inner lbrace without token length.
        const int offset = 16;

        //Case 2
        if (statementStartLine == *line) {
            if (sourceStartLine == *line)
                *column = offset + ast->qualifiedId->identifierToken.length;
            done = true;
        }

        //Case 3
        if (statementStartLine > *line) {
            *line = statementStartLine;
            if (sourceStartLine == *line)
                *column = offset + ast->qualifiedId->identifierToken.length;
            else
                *column = statementColumn;
            done = true;
        }
        return true;
    }

    bool visit(FunctionDeclaration *ast) override {
        quint32 sourceStartLine = ast->firstSourceLocation().startLine;
        quint32 sourceStartColumn = ast->firstSourceLocation().startColumn;
        quint32 statementStartLine = ast->body->firstSourceLocation().startLine;
        quint32 statementColumn = ast->body->firstSourceLocation().startColumn;

        //Case 1
        //Check for possible relocation within the function declaration

        //Case 2
        if (statementStartLine == *line) {
            if (sourceStartLine == *line)
                *column = statementColumn - sourceStartColumn + 1;
            done = true;
        }

        //Case 3
        if (statementStartLine > *line) {
            *line = statementStartLine;
            if (sourceStartLine == *line)
                *column = statementColumn - sourceStartColumn + 1;
            else
                *column = statementColumn;
            done = true;
        }
        return true;
    }

    bool visit(EmptyStatement *ast) override
    {
        *line = ast->lastSourceLocation().startLine + 1;
        return true;
    }

    bool visit(VariableStatement *ast) override { test(ast); return true; }
    bool visit(VariableDeclarationList *ast) override { test(ast); return true; }
    bool visit(VariableDeclaration *ast) override { test(ast); return true; }
    bool visit(ExpressionStatement *ast) override { test(ast); return true; }
    bool visit(IfStatement *ast) override { test(ast); return true; }
    bool visit(DoWhileStatement *ast) override { test(ast); return true; }
    bool visit(WhileStatement *ast) override { test(ast); return true; }
    bool visit(ForStatement *ast) override { test(ast); return true; }
    bool visit(LocalForStatement *ast) override { test(ast); return true; }
    bool visit(ForEachStatement *ast) override { test(ast); return true; }
    bool visit(LocalForEachStatement *ast) override { test(ast); return true; }
    bool visit(ContinueStatement *ast) override { test(ast); return true; }
    bool visit(BreakStatement *ast) override { test(ast); return true; }
    bool visit(ReturnStatement *ast) override { test(ast); return true; }
    bool visit(WithStatement *ast) override { test(ast); return true; }
    bool visit(SwitchStatement *ast) override { test(ast); return true; }
    bool visit(CaseBlock *ast) override { test(ast); return true; }
    bool visit(CaseClauses *ast) override { test(ast); return true; }
    bool visit(CaseClause *ast) override { test(ast); return true; }
    bool visit(DefaultClause *ast) override { test(ast); return true; }
    bool visit(LabelledStatement *ast) override { test(ast); return true; }
    bool visit(ThrowStatement *ast) override { test(ast); return true; }
    bool visit(TryStatement *ast) override { test(ast); return true; }
    bool visit(Catch *ast) override { test(ast); return true; }
    bool visit(Finally *ast) override { test(ast); return true; }
    bool visit(FunctionExpression *ast) override { test(ast); return true; }
    bool visit(DebuggerStatement *ast) override { test(ast); return true; }

    void test(Node *ast)
    {
        quint32 statementStartLine = ast->firstSourceLocation().startLine;
        //Case 1/2
        if (statementStartLine <= *line && *line <= ast->lastSourceLocation().startLine)
            done = true;

        //Case 3
        if (statementStartLine > *line) {
            *line = statementStartLine;
            *column = ast->firstSourceLocation().startColumn;
            done = true;
        }
    }

    bool done;
    quint32 *line;
    quint32 *column;
};

void appendDebugOutput(QtMsgType type, const QString &message, const QDebugContextInfo &info)
{
    ConsoleItem::ItemType itemType;
    switch (type) {
    case QtInfoMsg:
    case QtDebugMsg:
        itemType = ConsoleItem::DebugType;
        break;
    case QtWarningMsg:
        itemType = ConsoleItem::WarningType;
        break;
    case QtCriticalMsg:
    case QtFatalMsg:
        itemType = ConsoleItem::ErrorType;
        break;
    }

    QTC_ASSERT(itemType != ConsoleItem::DefaultType, return);

    debuggerConsole()->printItem(new ConsoleItem(itemType, message, info.file, info.line));
}

void clearExceptionSelection()
{
    QList<QTextEdit::ExtraSelection> selections;

    foreach (IEditor *editor, DocumentModel::editorsForOpenedDocuments()) {
        if (auto ed = qobject_cast<TextEditorWidget *>(editor->widget()))
            ed->setExtraSelections(TextEditorWidget::DebuggerExceptionSelection, selections);
    }
}

QStringList highlightExceptionCode(int lineNumber, const QString &filePath, const QString &errorMessage)
{
    QStringList messages;
    const QList<IEditor *> editors = DocumentModel::editorsForFilePath(filePath);

    const  TextEditor::FontSettings &fontSettings = TextEditor::TextEditorSettings::instance()->fontSettings();
    QTextCharFormat errorFormat = fontSettings.toTextCharFormat(TextEditor::C_ERROR);

    for (IEditor *editor : editors) {
        auto ed = qobject_cast<TextEditorWidget *>(editor->widget());
        if (!ed)
            continue;

        QList<QTextEdit::ExtraSelection> selections;
        QTextEdit::ExtraSelection sel;
        sel.format = errorFormat;
        QTextCursor c(ed->document()->findBlockByNumber(lineNumber - 1));
        const QString text = c.block().text();
        for (int i = 0; i < text.size(); ++i) {
            if (!text.at(i).isSpace()) {
                c.setPosition(c.position() + i);
                break;
            }
        }
        c.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        sel.cursor = c;

        sel.format.setToolTip(errorMessage);

        selections.append(sel);
        ed->setExtraSelections(TextEditorWidget::DebuggerExceptionSelection, selections);

        messages.append(QString::fromLatin1("%1: %2: %3").arg(filePath).arg(lineNumber).arg(errorMessage));
    }
    return messages;
}

} // Internal
} // Debugger
