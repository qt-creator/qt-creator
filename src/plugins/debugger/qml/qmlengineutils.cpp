/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlengine.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/consolemanagerinterface.h>

#include <coreplugin/editormanager/documentmodel.h>

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

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

    bool preVisit(Node *ast)
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

    bool visit(UiScriptBinding *ast)
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
            Block *block = static_cast<Block *>(ast->statement);
            if (!block || !block->statements)
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

    bool visit(FunctionDeclaration *ast) {
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

    bool visit(EmptyStatement *ast)
    {
        *line = ast->lastSourceLocation().startLine + 1;
        return true;
    }

    bool visit(VariableStatement *ast) { test(ast); return true; }
    bool visit(VariableDeclarationList *ast) { test(ast); return true; }
    bool visit(VariableDeclaration *ast) { test(ast); return true; }
    bool visit(ExpressionStatement *ast) { test(ast); return true; }
    bool visit(IfStatement *ast) { test(ast); return true; }
    bool visit(DoWhileStatement *ast) { test(ast); return true; }
    bool visit(WhileStatement *ast) { test(ast); return true; }
    bool visit(ForStatement *ast) { test(ast); return true; }
    bool visit(LocalForStatement *ast) { test(ast); return true; }
    bool visit(ForEachStatement *ast) { test(ast); return true; }
    bool visit(LocalForEachStatement *ast) { test(ast); return true; }
    bool visit(ContinueStatement *ast) { test(ast); return true; }
    bool visit(BreakStatement *ast) { test(ast); return true; }
    bool visit(ReturnStatement *ast) { test(ast); return true; }
    bool visit(WithStatement *ast) { test(ast); return true; }
    bool visit(SwitchStatement *ast) { test(ast); return true; }
    bool visit(CaseBlock *ast) { test(ast); return true; }
    bool visit(CaseClauses *ast) { test(ast); return true; }
    bool visit(CaseClause *ast) { test(ast); return true; }
    bool visit(DefaultClause *ast) { test(ast); return true; }
    bool visit(LabelledStatement *ast) { test(ast); return true; }
    bool visit(ThrowStatement *ast) { test(ast); return true; }
    bool visit(TryStatement *ast) { test(ast); return true; }
    bool visit(Catch *ast) { test(ast); return true; }
    bool visit(Finally *ast) { test(ast); return true; }
    bool visit(FunctionExpression *ast) { test(ast); return true; }
    bool visit(DebuggerStatement *ast) { test(ast); return true; }

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

bool adjustBreakpointLineAndColumn(const QString &filePath, quint32 *line, quint32 *column, bool *valid)
{
    bool success = false;
    //check if file is in the latest snapshot
    //ignoring documentChangedOnDisk
    //TODO:: update breakpoints if document is changed.
    ModelManagerInterface *mmIface = ModelManagerInterface::instance();
    if (mmIface) {
        Document::Ptr doc = mmIface->newestSnapshot().document(filePath);
        if (doc.isNull()) {
            ModelManagerInterface::instance()->updateSourceFiles(
                        QStringList() << filePath, false);
        } else {
            ASTWalker walker;
            walker(doc->ast(), line, column);
            *valid = walker.done;
            success = true;
        }
    }
    return success;
}

void appendDebugOutput(QtMsgType type, const QString &message, const QDebugContextInfo &info)
{
    ConsoleItem::ItemType itemType;
    switch (type) {
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
    default:
        //This case is not possible
        return;
    }

    if (auto consoleManager = ConsoleManagerInterface::instance()) {
        ConsoleItem *item = new ConsoleItem(consoleManager->rootItem(), itemType, message);
        item->file = info.file;
        item->line = info.line;
        consoleManager->printToConsolePane(item);
    }
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
    QList<IEditor *> editors = DocumentModel::editorsForFilePath(filePath);

    // set up the format for the errors
    QTextCharFormat errorFormat;
    errorFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    errorFormat.setUnderlineColor(Qt::red);

    foreach (IEditor *editor, editors) {
        TextEditorWidget *ed = qobject_cast<TextEditorWidget *>(editor->widget());
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
