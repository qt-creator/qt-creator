/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "cppquickfix.h"
#include "cppeditor.h"

#include <cplusplus/CppDocument.h>

#include <TranslationUnit.h>
#include <ASTVisitor.h>
#include <AST.h>
#include <ASTPatternBuilder.h>
#include <ASTMatcher.h>
#include <Token.h>

#include <cpptools/cppmodelmanagerinterface.h>
#include <QtDebug>

using namespace CppEditor::Internal;
using namespace CPlusPlus;

namespace {

class ASTPath: public ASTVisitor
{
    Document::Ptr _doc;
    unsigned _line;
    unsigned _column;
    QList<AST *> _nodes;

public:
    ASTPath(Document::Ptr doc)
        : ASTVisitor(doc->translationUnit()),
          _doc(doc), _line(0), _column(0)
    {}

    QList<AST *> operator()(const QTextCursor &cursor)
    {
        _nodes.clear();
        _line = cursor.blockNumber() + 1;
        _column = cursor.columnNumber() + 1;
        accept(_doc->translationUnit()->ast());
        return _nodes;
    }

protected:
    virtual bool preVisit(AST *ast)
    {
        unsigned firstToken = ast->firstToken();
        unsigned lastToken = ast->lastToken();

        if (firstToken > 0 && lastToken > firstToken) {
            unsigned startLine, startColumn;
            getTokenStartPosition(firstToken, &startLine, &startColumn);

            if (_line > startLine || (_line == startLine && _column >= startColumn)) {

                unsigned endLine, endColumn;
                getTokenEndPosition(lastToken - 1, &endLine, &endColumn);

                if (_line < endLine || (_line == endLine && _column < endColumn)) {
                    _nodes.append(ast);
                    return true;
                }
            }
        }

        return false;
    }
};

class HelloQuickFixOp: public QuickFixOperation
{
public:
    HelloQuickFixOp(Document::Ptr doc, const Snapshot &snapshot)
        : QuickFixOperation(doc, snapshot)
    {}

    virtual QString description() const
    {
        return QLatin1String("Hello"); // ### tr?
    }

    virtual void apply()
    {
        // nothing to do.
    }
};

class RewriteLogicalAndOp: public QuickFixOperation
{
public:
    RewriteLogicalAndOp(Document::Ptr doc, const Snapshot &snapshot)
        : QuickFixOperation(doc, snapshot), matcher(doc->translationUnit()),
           left(0), right(0), pattern(0)
    {}

    virtual QString description() const
    {
        return QLatin1String("Rewrite condition using ||"); // ### tr?
    }

    bool match(BinaryExpressionAST *expression)
    {
        left = mk.UnaryExpression();
        right = mk.UnaryExpression();
        pattern = mk.BinaryExpression(left, right);

        if (expression->match(pattern, &matcher) &&
                tokenAt(pattern->binary_op_token).is(T_AMPER_AMPER) &&
                tokenAt(left->unary_op_token).is(T_EXCLAIM) &&
                tokenAt(right->unary_op_token).is(T_EXCLAIM)) {
            return true;
        }

        return false;
    }

    virtual void apply()
    {
        // nothing to do.

        QTextCursor binaryOp = selectToken(pattern->binary_op_token);
        QTextCursor firstUnaryOp = selectToken(left->unary_op_token);
        QTextCursor secondUnaryOp = selectToken(right->unary_op_token);

        QTextCursor tc = textCursor();
        tc.beginEditBlock();
        firstUnaryOp.removeSelectedText();
        secondUnaryOp.removeSelectedText();
        binaryOp.insertText(QLatin1String("||"));
        firstUnaryOp.insertText(QLatin1String("!("));

        QTextCursor endOfRightUnaryExpression = selectToken(right->lastToken() - 1);
        endOfRightUnaryExpression.setPosition(endOfRightUnaryExpression.position()); // ### method

        endOfRightUnaryExpression.insertText(QLatin1String(")"));
        tc.endEditBlock();
    }

private:
    ASTMatcher matcher;
    ASTPatternBuilder mk;
    UnaryExpressionAST *left;
    UnaryExpressionAST *right;
    BinaryExpressionAST *pattern;
};


} // end of anonymous namespace


QuickFixOperation::QuickFixOperation(CPlusPlus::Document::Ptr doc,
                                     const CPlusPlus::Snapshot &snapshot)
    : _doc(doc), _snapshot(snapshot)
{ }

QuickFixOperation::~QuickFixOperation()
{ }

QTextCursor QuickFixOperation::textCursor() const
{ return _textCursor; }

void QuickFixOperation::setTextCursor(const QTextCursor &cursor)
{ _textCursor = cursor; }

const CPlusPlus::Token &QuickFixOperation::tokenAt(unsigned index) const
{ return _doc->translationUnit()->tokenAt(index); }

int QuickFixOperation::tokenStartPosition(unsigned index) const
{
    unsigned line, column;
    _doc->translationUnit()->getPosition(tokenAt(index).begin(), &line, &column);
    return _textCursor.document()->findBlockByNumber(line - 1).position() + column - 1;
}

int QuickFixOperation::tokenEndPosition(unsigned index) const
{
    unsigned line, column;
    _doc->translationUnit()->getPosition(tokenAt(index).end(), &line, &column);
    return _textCursor.document()->findBlockByNumber(line - 1).position() + column - 1;
}

QTextCursor QuickFixOperation::selectToken(unsigned index) const
{
    QTextCursor tc = _textCursor;
    tc.setPosition(tokenStartPosition(index));
    tc.setPosition(tokenEndPosition(index), QTextCursor::KeepAnchor);
    return tc;
}

QTextCursor QuickFixOperation::selectNode(AST *ast) const
{
    QTextCursor tc = _textCursor;
    tc.setPosition(tokenStartPosition(ast->firstToken()));
    tc.setPosition(tokenEndPosition(ast->lastToken() - 1), QTextCursor::KeepAnchor);
    return tc;
}

CPPQuickFixCollector::CPPQuickFixCollector()
    : _modelManager(CppTools::CppModelManagerInterface::instance()), _editor(0)
{ }

CPPQuickFixCollector::~CPPQuickFixCollector()
{ }

bool CPPQuickFixCollector::supportsEditor(TextEditor::ITextEditable *editor)
{ return qobject_cast<CPPEditorEditable *>(editor) != 0; }

bool CPPQuickFixCollector::triggersCompletion(TextEditor::ITextEditable *)
{ return false; }

int CPPQuickFixCollector::startCompletion(TextEditor::ITextEditable *editable)
{
    Q_ASSERT(editable != 0);

    _editor = qobject_cast<CPPEditor *>(editable->widget());
    Q_ASSERT(_editor != 0);

    const SemanticInfo info = _editor->semanticInfo();

    if (info.revision != _editor->document()->revision()) {
        // outdated
        qWarning() << "TODO: outdated semantic info, force a reparse.";
        return -1;
    }

    if (info.doc) {
        ASTPath astPath(info.doc);

        const QList<AST *> path = astPath(_editor->textCursor());
        // ### build the list of the quick fix ops by scanning path.

        RewriteLogicalAndOp *op = new RewriteLogicalAndOp(info.doc, info.snapshot);
        QuickFixOperationPtr quickFix(op);

        for (int i = path.size() - 1; i != -1; --i) {
            AST *node = path.at(i);
            if (BinaryExpressionAST *binary = node->asBinaryExpression()) {
                if (op->match(binary)) {
                    _quickFixes.append(quickFix);
                    break;
                }
            }
        }

        if (! _quickFixes.isEmpty())
            return editable->position();
    }

    return -1;
}

void CPPQuickFixCollector::completions(QList<TextEditor::CompletionItem> *quickFixItems)
{
    for (int i = 0; i < _quickFixes.size(); ++i) {
        QuickFixOperationPtr op = _quickFixes.at(i);

        TextEditor::CompletionItem item(this);
        item.text = op->description();
        item.data = QVariant::fromValue(i);
        quickFixItems->append(item);
    }
}

void CPPQuickFixCollector::complete(const TextEditor::CompletionItem &item)
{
    const int index = item.data.toInt();

    if (index < _quickFixes.size()) {
        QuickFixOperationPtr quickFix = _quickFixes.at(index);
        quickFix->setTextCursor(_editor->textCursor());
        quickFix->apply();
    }
}

void CPPQuickFixCollector::cleanup()
{
    _quickFixes.clear();
}
