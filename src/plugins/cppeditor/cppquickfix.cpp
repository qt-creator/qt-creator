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

    virtual int match(const QList<AST *> &path, QTextCursor tc)
    {
        setTextCursor(tc);

        BinaryExpressionAST *expression = 0;

        int index = path.size() - 1;
        for (; index != -1; --index) {
            expression = path.at(index)->asBinaryExpression();
            if (expression)
                break;
        }

        if (! expression)
            return -1;

        if (! contains(expression->binary_op_token))
            return -1;

        left = mk.UnaryExpression();
        right = mk.UnaryExpression();
        pattern = mk.BinaryExpression(left, right);

        if (expression->match(pattern, &matcher) &&
                tokenAt(pattern->binary_op_token).is(T_AMPER_AMPER) &&
                tokenAt(left->unary_op_token).is(T_EXCLAIM) &&
                tokenAt(right->unary_op_token).is(T_EXCLAIM)) {
            return index;
        }

        return -1;
    }

    virtual void apply()
    {
        replace(pattern->binary_op_token, QLatin1String("||"));
        replace(left->unary_op_token, QLatin1String("!("));
        replace(right->unary_op_token, QLatin1String(""));
        insert(endOf(pattern), QLatin1String(")"));

        execute();
    }

private:
    ASTMatcher matcher;
    ASTPatternBuilder mk;
    UnaryExpressionAST *left;
    UnaryExpressionAST *right;
    BinaryExpressionAST *pattern;
};

class SplitSimpleDeclarationOp: public QuickFixOperation
{
public:
    SplitSimpleDeclarationOp(Document::Ptr doc, const Snapshot &snapshot, CPPEditor *editor)
        : QuickFixOperation(doc, snapshot), declaration(0), editor(editor)
    {}

    virtual QString description() const
    {
        return QLatin1String("Split declaration"); // ### tr?
    }

    bool checkDeclaration(SimpleDeclarationAST *declaration) const
    {
        if (! declaration->decl_specifier_list)
            return false;

        for (SpecifierListAST *it = declaration->decl_specifier_list; it; it = it->next) {
            SpecifierAST *specifier = it->value;

            if (specifier->asEnumSpecifier() != 0)
                return false;

            else if (specifier->asClassSpecifier() != 0)
                return false;
        }

        if (! declaration->declarator_list)
            return false;

        else if (! declaration->declarator_list->next)
            return false;

        return true;
    }

    virtual int match(const QList<AST *> &path, QTextCursor tc)
    {
        setTextCursor(tc);

        CoreDeclaratorAST *core_declarator = 0;

        int index = path.size() - 1;
        for (; index != -1; --index) {
            AST *node = path.at(index);

            if (CoreDeclaratorAST *coreDecl = node->asCoreDeclarator())
                core_declarator = coreDecl;

            else if (SimpleDeclarationAST *simpleDecl = node->asSimpleDeclaration()) {
                if (checkDeclaration(simpleDecl)) {
                    declaration = simpleDecl;

                    const int cursorPosition = tc.selectionStart();

                    const int startOfDeclSpecifier = startOf(declaration->decl_specifier_list->firstToken());
                    const int endOfDeclSpecifier = endOf(declaration->decl_specifier_list->lastToken() - 1);

                    if (cursorPosition >= startOfDeclSpecifier && cursorPosition <= endOfDeclSpecifier)
                        return index; // the AST node under cursor is a specifier.

                    if (core_declarator) {
                        const int startOfCoreDeclarator = startOf(core_declarator);
                        const int endOfCoreDeclarator = endOf(core_declarator);

                        if (cursorPosition >= startOfCoreDeclarator && cursorPosition <= endOfCoreDeclarator)
                            return index; // got a core-declarator under the text cursor.
                    }
                }

                break;
            }
        }

        return -1;
    }

    virtual void apply()
    {
        QTextCursor completeDeclaration = createCursor(declaration);

        SpecifierListAST *specifiers = declaration->decl_specifier_list;
        const QString declSpecifiers = textOf(startOf(specifiers->firstToken()), endOf(specifiers->lastToken() - 1));

        DeclaratorAST *declarator = declaration->declarator_list->value;
        replace(endOf(declarator), startOf(declaration->semicolon_token), QString());

        QString text;
        for (DeclaratorListAST *it = declaration->declarator_list->next; it; it = it->next) {
            DeclaratorAST *declarator = it->value;

            text += QLatin1Char('\n');
            text += declSpecifiers;
            text += QLatin1Char(' ');
            text += textOf(declarator);
            text += QLatin1String(";");
        }

        insert(endOf(declaration->semicolon_token), text);

        completeDeclaration.beginEditBlock();
        execute();
        editor->indentInsertedText(completeDeclaration);
        completeDeclaration.endEditBlock();
    }

private:
    SimpleDeclarationAST *declaration;
    CPPEditor *editor;
};

class TakeDeclarationOp: public QuickFixOperation
{
public:
    TakeDeclarationOp(Document::Ptr doc, const Snapshot &snapshot, CPPEditor *editor)
        : QuickFixOperation(doc, snapshot), matcher(doc->translationUnit()), editor(editor),
           condition(0), pattern(0), core(0)
    {}

    virtual QString description() const
    {
        return QLatin1String("Take declaration"); // ### tr?
    }

    virtual int match(const QList<AST *> &path, QTextCursor tc)
    {
        setTextCursor(tc);

        condition = mk.Condition();
        pattern = mk.IfStatement(condition);

        int index = path.size() - 1;
        for (; index != -1; --index) {
            if (IfStatementAST *statement = path.at(index)->asIfStatement()) {
                if (statement->match(pattern, &matcher) && condition->declarator) {
                    DeclaratorAST *declarator = condition->declarator;
                    core = declarator->core_declarator;
                    if (! core)
                        return -1;

                    int startOfCoreDeclarator = startOf(core->firstToken());
                    int endOfCoreDeclarator = endOf(core->lastToken() - 1);

                    int position = tc.selectionStart();

                    if (position >= startOfCoreDeclarator && position <= endOfCoreDeclarator) {
                        return index;
                    }
                }
            }
        }

        return -1;
    }

    virtual void apply()
    {
        QTextCursor completeIfStatement = createCursor(pattern);

        QString name = selectNode(core).selectedText();
        QString declaration = selectNode(condition).selectedText();
        declaration += QLatin1String(";\n");

        insert(startOf(pattern), declaration);
        insert(endOf(pattern->lparen_token), name);
        replace(condition, name);

        completeIfStatement.beginEditBlock();
        execute();
        editor->indentInsertedText(completeIfStatement);
        completeIfStatement.endEditBlock();
    }

private:
    ASTMatcher matcher;
    ASTPatternBuilder mk;
    CPPEditor *editor;
    ConditionAST *condition;
    IfStatementAST *pattern;
    CoreDeclaratorAST *core;
};

/*
  Replace
     if (something && something_else) {
     }

  with
     if (something) {
        if (something_else) {
        }
     }

  and
    if (something || something_else)
      x;

  with
    if (something)
      x;
    else if (something_else)
      x;
*/
class SplitIfStatementOp: public QuickFixOperation
{
public:
    SplitIfStatementOp(Document::Ptr doc, const Snapshot &snapshot, CPPEditor *editor)
        : QuickFixOperation(doc, snapshot),
          condition(0), pattern(0), editor(editor)
    {}

    virtual QString description() const
    {
        return QLatin1String("Split if statement"); // ### tr?
    }

    virtual int match(const QList<AST *> &path, QTextCursor tc)
    {
        setTextCursor(tc);

        pattern = 0;

        int index = path.size() - 1;
        for (; index != -1; --index) {
            AST *node = path.at(index);
            if (IfStatementAST *stmt = node->asIfStatement()) {
                pattern = stmt;
                break;
            }
        }

        if (! pattern)
            return -1;

        for (++index; index < path.size(); ++index) {
            AST *node = path.at(index);
            condition = node->asBinaryExpression();
            if (! condition)
                return -1;

            Token binaryToken = tokenAt(condition->binary_op_token);
            if (binaryToken.is(T_AMPER_AMPER) || binaryToken.is(T_PIPE_PIPE)) {
                if (contains(condition->binary_op_token))
                    return index;
                if (binaryToken.is(T_PIPE_PIPE))
                    return -1;
            } else {
                return -1;
            }
        }

        return -1;
    }

    virtual void apply()
    {
        Token binaryToken = tokenAt(condition->binary_op_token);

        if (binaryToken.is(T_AMPER_AMPER))
            splitAndCondition();
        else
            splitOrCondition();
    }

    void splitAndCondition()
    {
        QTextCursor completeIfStatement = createCursor(pattern);

        StatementAST *ifTrueStatement = pattern->statement;
        CompoundStatementAST *compoundStatement = ifTrueStatement->asCompoundStatement();

        // take the right-expression from the condition.
        QTextCursor rightCursor = textCursor();
        rightCursor.setPosition(startOf(condition->right_expression));
        rightCursor.setPosition(endOf(pattern->rparen_token - 1), QTextCursor::KeepAnchor);
        const QString rightCondition = rightCursor.selectedText();
        replace(endOf(condition->left_expression), startOf(pattern->rparen_token), QString());

        int offset = 0;

        if (compoundStatement)
            offset = endOf(compoundStatement->lbrace_token);
        else
            offset = endOf(pattern->rparen_token);

        // create the nested if statement
        QString nestedIfStatement;

        if (! compoundStatement)
            nestedIfStatement += QLatin1String(" {"); // open a compound statement

        nestedIfStatement += QLatin1String("\nif (");
        nestedIfStatement += rightCondition;
        nestedIfStatement += QLatin1String(") {\n}");

        insert(offset, nestedIfStatement);

        if (! compoundStatement)
            insert(endOf(ifTrueStatement), "\n}"); // finish the compound statement

        QTextCursor tc = textCursor();
        tc.beginEditBlock();
        execute();
        editor->indentInsertedText(completeIfStatement);
        tc.endEditBlock();
    }

    void splitOrCondition()
    {
        QTextCursor completeIfStatement = createCursor(pattern);

        StatementAST *ifTrueStatement = pattern->statement;
        CompoundStatementAST *compoundStatement = ifTrueStatement->asCompoundStatement();

        // take the right-expression from the condition.
        QTextCursor rightCursor = textCursor();
        rightCursor.setPosition(startOf(condition->right_expression));
        rightCursor.setPosition(endOf(pattern->rparen_token - 1), QTextCursor::KeepAnchor);
        const QString rightCondition = rightCursor.selectedText();
        replace(endOf(condition->left_expression), startOf(pattern->rparen_token), QString());

        // copy the if-body
        QTextCursor bodyCursor = textCursor();
        bodyCursor.setPosition(endOf(pattern->rparen_token));
        bodyCursor.setPosition(endOf(pattern->statement), QTextCursor::KeepAnchor);
        const QString body = bodyCursor.selectedText();

        QString elseIfStatement;
        if (compoundStatement)
            elseIfStatement += QLatin1String(" ");
        else
            elseIfStatement += QLatin1String("\n");

        elseIfStatement += QLatin1String("else if (");
        elseIfStatement += rightCondition;
        elseIfStatement += QLatin1String(")");
        elseIfStatement += body;

        insert(endOf(pattern), elseIfStatement);

        QTextCursor tc = textCursor();
        tc.beginEditBlock();
        execute();
        editor->indentInsertedText(completeIfStatement);
        tc.endEditBlock();
    }

private:
    BinaryExpressionAST *condition;
    IfStatementAST *pattern;
    QPointer<CPPEditor> editor;
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

int QuickFixOperation::startOf(unsigned index) const
{
    unsigned line, column;
    _doc->translationUnit()->getPosition(tokenAt(index).begin(), &line, &column);
    return _textCursor.document()->findBlockByNumber(line - 1).position() + column - 1;
}

int QuickFixOperation::startOf(const CPlusPlus::AST *ast) const
{
    return startOf(ast->firstToken());
}

int QuickFixOperation::endOf(unsigned index) const
{
    unsigned line, column;
    _doc->translationUnit()->getPosition(tokenAt(index).end(), &line, &column);
    return _textCursor.document()->findBlockByNumber(line - 1).position() + column - 1;
}

int QuickFixOperation::endOf(const CPlusPlus::AST *ast) const
{
    return endOf(ast->lastToken() - 1);
}

bool QuickFixOperation::contains(unsigned tokenIndex) const
{
    QTextCursor tc = textCursor();
    int cursorBegin = tc.selectionStart();

    int start = startOf(tokenIndex);
    int end = endOf(tokenIndex);

    if (cursorBegin >= start && cursorBegin <= end)
        return true;

    return false;
}

QTextCursor QuickFixOperation::selectToken(unsigned index) const
{
    QTextCursor tc = _textCursor;
    tc.setPosition(startOf(index));
    tc.setPosition(endOf(index), QTextCursor::KeepAnchor);
    return tc;
}

QTextCursor QuickFixOperation::selectNode(AST *ast) const
{
    QTextCursor tc = _textCursor;
    tc.setPosition(startOf(ast->firstToken()));
    tc.setPosition(endOf(ast->lastToken() - 1), QTextCursor::KeepAnchor);
    return tc;
}

QTextCursor QuickFixOperation::createCursor(AST *ast) const
{
    QTextCursor cursor = selectNode(ast);
    // ### HACK
    const int anchor = cursor.anchor();
    const int position = cursor.position();
    cursor.setPosition(position);
    cursor.setPosition(anchor, QTextCursor::KeepAnchor);
    return cursor;
}

void QuickFixOperation::move(int start, int end, int to)
{
    if (end > start)
        _textWriter.move(start, end-start, to);
}

void QuickFixOperation::move(unsigned tokenIndex, int to)
{
    move(startOf(tokenIndex), endOf(tokenIndex), to);
}

void QuickFixOperation::move(const CPlusPlus::AST *ast, int to)
{
    move(startOf(ast), endOf(ast), to);
}

void QuickFixOperation::replace(int start, int end, const QString &replacement)
{
    if (end >= start)
        _textWriter.replace(start, end-start, replacement);
}

void QuickFixOperation::replace(unsigned tokenIndex, const QString &replacement)
{
    replace(startOf(tokenIndex), endOf(tokenIndex), replacement);
}

void QuickFixOperation::replace(const CPlusPlus::AST *ast, const QString &replacement)
{
    replace(startOf(ast), endOf(ast), replacement);
}

void QuickFixOperation::insert(int at, const QString &text)
{
    replace(at, at, text);
}

QString QuickFixOperation::textOf(int firstOffset, int lastOffset) const
{
    QTextCursor tc = _textCursor;
    tc.setPosition(firstOffset);
    tc.setPosition(lastOffset, QTextCursor::KeepAnchor);
    return tc.selectedText();
}

QString QuickFixOperation::textOf(AST *ast) const
{
    return selectNode(ast).selectedText();
}

void QuickFixOperation::execute()
{
    _textWriter.write(&_textCursor);
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

        QSharedPointer<RewriteLogicalAndOp> rewriteLogicalAndOp(new RewriteLogicalAndOp(info.doc, info.snapshot));
        QSharedPointer<SplitIfStatementOp> splitIfStatementOp(new SplitIfStatementOp(info.doc, info.snapshot, _editor));
        QSharedPointer<TakeDeclarationOp> takeDeclarationOp(new TakeDeclarationOp(info.doc, info.snapshot, _editor));
        QSharedPointer<SplitSimpleDeclarationOp> splitSimpleDeclarationOp(new SplitSimpleDeclarationOp(info.doc, info.snapshot, _editor));

        QList<QuickFixOperationPtr> candidates;
        candidates.append(rewriteLogicalAndOp);
        candidates.append(splitIfStatementOp);
        candidates.append(takeDeclarationOp);
        candidates.append(splitSimpleDeclarationOp);

        QMultiMap<int, QuickFixOperationPtr> matchedOps;

        foreach (QuickFixOperationPtr op, candidates) {
            int priority = op->match(path, _editor->textCursor());
            if (priority != -1)
                matchedOps.insert(priority, op);
        }

        QMapIterator<int, QuickFixOperationPtr> it(matchedOps);
        it.toBack();
        if (it.hasPrevious()) {
            it.previous();

            int priority = it.key();
            _quickFixes.append(it.value());

            while (it.hasPrevious()) {
                it.previous();

                if (it.key() != priority)
                    break;

                _quickFixes.append(it.value());
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
