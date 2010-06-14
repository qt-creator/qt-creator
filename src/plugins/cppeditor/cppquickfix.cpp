/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include <cplusplus/ASTPath.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/ResolveExpression.h>

#include <TranslationUnit.h>
#include <ASTVisitor.h>
#include <AST.h>
#include <ASTPatternBuilder.h>
#include <ASTMatcher.h>
#include <Token.h>
#include <Type.h>
#include <CoreTypes.h>
#include <Symbol.h>
#include <Symbols.h>
#include <Name.h>
#include <Literals.h>

#include <cpptools/cpprefactoringchanges.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cppmodelmanagerinterface.h>

#include <QtGui/QApplication>
#include <QtGui/QTextBlock>

using namespace CppEditor::Internal;
using namespace CPlusPlus;
using namespace Utils;

namespace {

class CppQuickFixState: public TextEditor::QuickFixState
{
public:
    QList<CPlusPlus::AST *> path;
    SemanticInfo info;
};

/*
    Rewrite
    a op b -> !(a invop b)
    (a op b) -> !(a invop b)
    !(a op b) -> (a invob b)
*/
class UseInverseOp: public CppQuickFixOperation
{
public:
    UseInverseOp(TextEditor::BaseTextEditor *editor)
        : CppQuickFixOperation(editor), binary(0), nested(0), negation(0)
    {}

    virtual QString description() const
    {
        return QApplication::translate("CppTools::QuickFix", "Rewrite Using %1").arg(replacement);
    }

    virtual int match(const QList<AST *> &path)
    {
        int index = path.size() - 1;
        binary = path.at(index)->asBinaryExpression();
        if (! binary)
            return -1;
        if (! isCursorOn(binary->binary_op_token))
            return -1;

        CPlusPlus::Kind invertToken;
        switch (tokenAt(binary->binary_op_token).kind()) {
        case T_LESS_EQUAL:
            invertToken = T_GREATER;
            break;
        case T_LESS:
            invertToken = T_GREATER_EQUAL;
            break;
        case T_GREATER:
            invertToken = T_LESS_EQUAL;
            break;
        case T_GREATER_EQUAL:
            invertToken = T_LESS;
            break;
        case T_EQUAL_EQUAL:
            invertToken = T_EXCLAIM_EQUAL;
            break;
        case T_EXCLAIM_EQUAL:
            invertToken = T_EQUAL_EQUAL;
            break;
        default:
            return -1;
        }

        CPlusPlus::Token tok;
        tok.f.kind = invertToken;
        replacement = QLatin1String(tok.spell());

        // check for enclosing nested expression
        if (index - 1 >= 0)
            nested = path[index - 1]->asNestedExpression();

        // check for ! before parentheses
        if (nested && index - 2 >= 0) {
            negation = path[index - 2]->asUnaryExpression();
            if (negation && ! tokenAt(negation->unary_op_token).is(T_EXCLAIM))
                negation = 0;
        }

        return index;
    }

    virtual void createChanges()
    {
        ChangeSet changes;
        if (negation) {
            // can't remove parentheses since that might break precedence
            remove(&changes, negation->unary_op_token);
        } else if (nested) {
            changes.insert(startOf(nested), "!");
        } else {
            changes.insert(startOf(binary), "!(");
            changes.insert(endOf(binary), ")");
        }
        replace(&changes, binary->binary_op_token, replacement);
        cppRefactoringChanges()->changeFile(fileName(), changes);
    }

private:
    BinaryExpressionAST *binary;
    NestedExpressionAST *nested;
    UnaryExpressionAST *negation;

    QString replacement;
};

/*
    Rewrite
    a op b

    As
    b flipop a
*/
class FlipBinaryOp: public CppQuickFixOperation
{
public:
    FlipBinaryOp(TextEditor::BaseTextEditor *editor)
        : CppQuickFixOperation(editor), binary(0)
    {}


    virtual QString description() const
    {
        if (replacement.isEmpty())
            return QApplication::translate("CppTools::QuickFix", "Swap Operands");
        else
            return QApplication::translate("CppTools::QuickFix", "Rewrite Using %1").arg(replacement);
    }

    virtual int match(const QList<AST *> &path)
    {
        int index = path.size() - 1;
        binary = path.at(index)->asBinaryExpression();
        if (! binary)
            return -1;
        if (! isCursorOn(binary->binary_op_token))
            return -1;

        CPlusPlus::Kind flipToken;
        switch (tokenAt(binary->binary_op_token).kind()) {
        case T_LESS_EQUAL:
            flipToken = T_GREATER_EQUAL;
            break;
        case T_LESS:
            flipToken = T_GREATER;
            break;
        case T_GREATER:
            flipToken = T_LESS;
            break;
        case T_GREATER_EQUAL:
            flipToken = T_LESS_EQUAL;
            break;
        case T_EQUAL_EQUAL:
        case T_EXCLAIM_EQUAL:
        case T_AMPER_AMPER:
        case T_PIPE_PIPE:
            flipToken = T_EOF_SYMBOL;
            break;
        default:
            return -1;
        }

        if (flipToken != T_EOF_SYMBOL) {
            CPlusPlus::Token tok;
            tok.f.kind = flipToken;
            replacement = QLatin1String(tok.spell());
        }
        return index;
    }

    virtual void createChanges()
    {
        ChangeSet changes;

        flip(&changes, binary->left_expression, binary->right_expression);
        if (! replacement.isEmpty())
            replace(&changes, binary->binary_op_token, replacement);

        cppRefactoringChanges()->changeFile(fileName(), changes);
    }

private:
    BinaryExpressionAST *binary;
    QString replacement;
};

/*
    Rewrite
    !a && !b

    As
    !(a || b)
*/
class RewriteLogicalAndOp: public CppQuickFixOperation
{
public:
    RewriteLogicalAndOp(TextEditor::BaseTextEditor *editor)
        : CppQuickFixOperation(editor), left(0), right(0), pattern(0)
    {}

    virtual QString description() const
    {
        return QApplication::translate("CppTools::QuickFix", "Rewrite Condition Using ||");
    }

    virtual int match(const QList<AST *> &path)
    {
        BinaryExpressionAST *expression = 0;

        int index = path.size() - 1;
        for (; index != -1; --index) {
            expression = path.at(index)->asBinaryExpression();
            if (expression)
                break;
        }

        if (! expression)
            return -1;

        if (! isCursorOn(expression->binary_op_token))
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

    virtual void createChanges()
    {
        ChangeSet changes;
        replace(&changes, pattern->binary_op_token, QLatin1String("||"));
        remove(&changes, left->unary_op_token);
        remove(&changes, right->unary_op_token);
        const int start = startOf(pattern);
        const int end = endOf(pattern);
        changes.insert(start, QLatin1String("!("));
        changes.insert(end, QLatin1String(")"));

        cppRefactoringChanges()->changeFile(fileName(), changes);
        cppRefactoringChanges()->reindent(fileName(), range(start, end));
    }

private:
    ASTMatcher matcher;
    ASTPatternBuilder mk;
    UnaryExpressionAST *left;
    UnaryExpressionAST *right;
    BinaryExpressionAST *pattern;
};

class SplitSimpleDeclarationOp: public CppQuickFixOperation
{
public:
    SplitSimpleDeclarationOp(TextEditor::BaseTextEditor *editor)
        : CppQuickFixOperation(editor), declaration(0)
    {}

    virtual QString description() const
    {
        return QApplication::translate("CppTools::QuickFix", "Split Declaration");
    }

    bool checkDeclaration(SimpleDeclarationAST *declaration) const
    {
        if (! declaration->semicolon_token)
            return false;

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

    virtual int match(const QList<AST *> &path)
    {
        CoreDeclaratorAST *core_declarator = 0;

        int index = path.size() - 1;
        for (; index != -1; --index) {
            AST *node = path.at(index);

            if (CoreDeclaratorAST *coreDecl = node->asCoreDeclarator())
                core_declarator = coreDecl;

            else if (SimpleDeclarationAST *simpleDecl = node->asSimpleDeclaration()) {
                if (checkDeclaration(simpleDecl)) {
                    declaration = simpleDecl;

                    const int cursorPosition = selectionStart();

                    const int startOfDeclSpecifier = startOf(declaration->decl_specifier_list->firstToken());
                    const int endOfDeclSpecifier = endOf(declaration->decl_specifier_list->lastToken() - 1);

                    if (cursorPosition >= startOfDeclSpecifier && cursorPosition <= endOfDeclSpecifier)
                        return index; // the AST node under cursor is a specifier.

                    if (core_declarator && isCursorOn(core_declarator))
                        return index; // got a core-declarator under the text cursor.
                }

                break;
            }
        }

        return -1;
    }

    virtual void createChanges()
    {
        ChangeSet changes;

        SpecifierListAST *specifiers = declaration->decl_specifier_list;
        int declSpecifiersStart = startOf(specifiers->firstToken());
        int declSpecifiersEnd = endOf(specifiers->lastToken() - 1);
        int insertPos = endOf(declaration->semicolon_token);

        DeclaratorAST *prevDeclarator = declaration->declarator_list->value;

        for (DeclaratorListAST *it = declaration->declarator_list->next; it; it = it->next) {
            DeclaratorAST *declarator = it->value;

            changes.insert(insertPos, QLatin1String("\n"));
            changes.copy(declSpecifiersStart, declSpecifiersEnd - declSpecifiersStart, insertPos);
            changes.insert(insertPos, QLatin1String(" "));
            move(&changes, declarator, insertPos);
            changes.insert(insertPos, QLatin1String(";"));

            const int prevDeclEnd = endOf(prevDeclarator);
            changes.remove(prevDeclEnd, startOf(declarator) - prevDeclEnd);

            prevDeclarator = declarator;
        }

        cppRefactoringChanges()->changeFile(fileName(), changes);
        cppRefactoringChanges()->reindent(fileName(), range(startOf(declaration->firstToken()),
                                                            endOf(declaration->lastToken() - 1)));
    }

private:
    SimpleDeclarationAST *declaration;
};

/*
    Add curly braces to a if statement that doesn't already contain a
    compound statement.
*/
class AddBracesToIfOp: public CppQuickFixOperation
{
public:
    AddBracesToIfOp(TextEditor::BaseTextEditor *editor)
        : CppQuickFixOperation(editor), _statement(0)
    {}

    virtual QString description() const
    {
        return QApplication::translate("CppTools::QuickFix", "Add Curly Braces");
    }

    virtual int match(const QList<AST *> &path)
    {
        // show when we're on the 'if' of an if statement
        int index = path.size() - 1;
        IfStatementAST *ifStatement = path.at(index)->asIfStatement();
        if (ifStatement && isCursorOn(ifStatement->if_token)
            && ! ifStatement->statement->asCompoundStatement()) {
            _statement = ifStatement->statement;
            return index;
        }

        // or if we're on the statement contained in the if
        // ### This may not be such a good idea, consider nested ifs...
        for (; index != -1; --index) {
            IfStatementAST *ifStatement = path.at(index)->asIfStatement();
            if (ifStatement && ifStatement->statement
                && isCursorOn(ifStatement->statement)
                && ! ifStatement->statement->asCompoundStatement()) {
                _statement = ifStatement->statement;
                return index;
            }
        }

        // ### This could very well be extended to the else branch
        // and other nodes entirely.

        return -1;
    }

    virtual void createChanges()
    {
        ChangeSet changes;

        const int start = endOf(_statement->firstToken() - 1);
        changes.insert(start, QLatin1String(" {"));

        const int end = endOf(_statement->lastToken() - 1);
        changes.insert(end, "\n}");

        cppRefactoringChanges()->changeFile(fileName(), changes);
        cppRefactoringChanges()->reindent(fileName(), range(start, end));
    }

private:
    StatementAST *_statement;
};

/*
    Replace
    if (Type name = foo()) {...}

    With
    Type name = foo;
    if (name) {...}
*/
class MoveDeclarationOutOfIfOp: public CppQuickFixOperation
{
public:
    MoveDeclarationOutOfIfOp(TextEditor::BaseTextEditor *editor)
        : CppQuickFixOperation(editor), condition(0), pattern(0), core(0)
    {}

    virtual QString description() const
    {
        return QApplication::translate("CppTools::QuickFix", "Move Declaration out of Condition");
    }

    virtual int match(const QList<AST *> &path)
    {
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

                    if (isCursorOn(core))
                        return index;
                }
            }
        }

        return -1;
    }

    virtual void createChanges()
    {
        ChangeSet changes;

        copy(&changes, core, startOf(condition));

        int insertPos = startOf(pattern);
        move(&changes, condition, insertPos);
        changes.insert(insertPos, QLatin1String(";\n"));

        cppRefactoringChanges()->changeFile(fileName(), changes);
        cppRefactoringChanges()->reindent(fileName(), range(startOf(pattern),
                                                            endOf(pattern)));
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
    while (Type name = foo()) {...}

    With
    Type name;
    while ((name = foo()) != 0) {...}
*/
class MoveDeclarationOutOfWhileOp: public CppQuickFixOperation
{
public:
    MoveDeclarationOutOfWhileOp(TextEditor::BaseTextEditor *editor)
        : CppQuickFixOperation(editor), condition(0), pattern(0), core(0)
    {}

    virtual QString description() const
    {
        return QApplication::translate("CppTools::QuickFix", "Move Declaration out of Condition");
    }

    virtual int match(const QList<AST *> &path)
    {
        condition = mk.Condition();
        pattern = mk.WhileStatement(condition);

        int index = path.size() - 1;
        for (; index != -1; --index) {
            if (WhileStatementAST *statement = path.at(index)->asWhileStatement()) {
                if (statement->match(pattern, &matcher) && condition->declarator) {
                    DeclaratorAST *declarator = condition->declarator;
                    core = declarator->core_declarator;

                    if (! core)
                        return -1;

                    else if (! declarator->equals_token)
                        return -1;

                    else if (! declarator->initializer)
                        return -1;

                    if (isCursorOn(core))
                        return index;
                }
            }
        }

        return -1;
    }

    virtual void createChanges()
    {
        ChangeSet changes;

        changes.insert(startOf(condition), QLatin1String("("));
        changes.insert(endOf(condition), QLatin1String(") != 0"));

        int insertPos = startOf(pattern);
        const int conditionStart = startOf(condition);
        changes.move(conditionStart, startOf(core) - conditionStart, insertPos);
        copy(&changes, core, insertPos);
        changes.insert(insertPos, QLatin1String(";\n"));

        cppRefactoringChanges()->changeFile(fileName(), changes);
        cppRefactoringChanges()->reindent(fileName(), range(startOf(pattern),
                                                            endOf(pattern)));
    }

private:
    ASTMatcher matcher;
    ASTPatternBuilder mk;
    CPPEditor *editor;
    ConditionAST *condition;
    WhileStatementAST *pattern;
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
class SplitIfStatementOp: public CppQuickFixOperation
{
public:
    SplitIfStatementOp(TextEditor::BaseTextEditor *editor)
        : CppQuickFixOperation(editor), condition(0), pattern(0)
    {}

    virtual QString description() const
    {
        return QApplication::translate("CppTools::QuickFix", "Split if Statement");
    }

    virtual int match(const QList<AST *> &path)
    {
        pattern = 0;

        int index = path.size() - 1;
        for (; index != -1; --index) {
            AST *node = path.at(index);
            if (IfStatementAST *stmt = node->asIfStatement()) {
                pattern = stmt;
                break;
            }
        }

        if (! pattern || ! pattern->statement)
            return -1;

        unsigned splitKind = 0;
        for (++index; index < path.size(); ++index) {
            AST *node = path.at(index);
            condition = node->asBinaryExpression();
            if (! condition)
                return -1;

            Token binaryToken = tokenAt(condition->binary_op_token);

            // only accept a chain of ||s or &&s - no mixing
            if (! splitKind) {
                splitKind = binaryToken.kind();
                if (splitKind != T_AMPER_AMPER && splitKind != T_PIPE_PIPE)
                    return -1;
                // we can't reliably split &&s in ifs with an else branch
                if (splitKind == T_AMPER_AMPER && pattern->else_statement)
                    return -1;
            } else if (splitKind != binaryToken.kind()) {
                return -1;
            }

            if (isCursorOn(condition->binary_op_token))
                return index;
        }

        return -1;
    }

    virtual void createChanges()
    {
        Token binaryToken = tokenAt(condition->binary_op_token);

        if (binaryToken.is(T_AMPER_AMPER))
            splitAndCondition();
        else
            splitOrCondition();
    }

    void splitAndCondition()
    {
        ChangeSet changes;

        int startPos = startOf(pattern);
        changes.insert(startPos, QLatin1String("if ("));
        move(&changes, condition->left_expression, startPos);
        changes.insert(startPos, QLatin1String(") {\n"));

        const int lExprEnd = endOf(condition->left_expression);
        changes.remove(lExprEnd,
                       startOf(condition->right_expression) - lExprEnd);
        changes.insert(endOf(pattern), QLatin1String("\n}"));

        cppRefactoringChanges()->changeFile(fileName(), changes);
        cppRefactoringChanges()->reindent(fileName(), range(startOf(pattern),
                                                            endOf(pattern)));
    }

    void splitOrCondition()
    {
        ChangeSet changes;

        StatementAST *ifTrueStatement = pattern->statement;
        CompoundStatementAST *compoundStatement = ifTrueStatement->asCompoundStatement();

        int insertPos = endOf(ifTrueStatement);
        if (compoundStatement)
            changes.insert(insertPos, QLatin1String(" "));
        else
            changes.insert(insertPos, QLatin1String("\n"));
        changes.insert(insertPos, QLatin1String("else if ("));

        const int rExprStart = startOf(condition->right_expression);
        changes.move(rExprStart, startOf(pattern->rparen_token) - rExprStart,
                     insertPos);
        changes.insert(insertPos, QLatin1String(")"));

        const int rParenEnd = endOf(pattern->rparen_token);
        changes.copy(rParenEnd, endOf(pattern->statement) - rParenEnd, insertPos);

        const int lExprEnd = endOf(condition->left_expression);
        changes.remove(lExprEnd, startOf(condition->right_expression) - lExprEnd);

        cppRefactoringChanges()->changeFile(fileName(), changes);
        cppRefactoringChanges()->reindent(fileName(), range(startOf(pattern),
                                                            endOf(pattern)));
    }

private:
    BinaryExpressionAST *condition;
    IfStatementAST *pattern;
};

/*
  Replace
    "abcd"
  With
    QLatin1String("abcd")
*/
class WrapStringLiteral: public CppQuickFixOperation
{
public:
    WrapStringLiteral(TextEditor::BaseTextEditor *editor)
        : CppQuickFixOperation(editor), stringLiteral(0), isObjCStringLiteral(false)
    {}

    virtual QString description() const
    {
        return QApplication::translate("CppTools::QuickFix", "Enclose in QLatin1String(...)");
    }

    virtual int match(const QList<AST *> &path)
    {
        if (path.isEmpty())
            return -1;

        int index = path.size() - 1;
        stringLiteral = path[index]->asStringLiteral();

        if (!stringLiteral)
            return -1;

        isObjCStringLiteral = charAt(startOf(stringLiteral)) == QLatin1Char('@');

        // check if it is already wrapped in QLatin1String or -Literal
        if (index-2 < 0)
            return index;

        CallAST *call = path[index-1]->asCall();
        PostfixExpressionAST *postfixExp = path[index-2]->asPostfixExpression();
        if (call && postfixExp
            && postfixExp->base_expression
            && postfixExp->postfix_expression_list
            && postfixExp->postfix_expression_list->value == call)
        {
            NameAST *callName = postfixExp->base_expression->asName();
            if (!callName)
                return index;

            QByteArray callNameString(tokenAt(callName->firstToken()).spell());
            if (callNameString == "QLatin1String"
                || callNameString == "QLatin1Literal"
                )
                return -1;
        }

        return index;
    }

    virtual void createChanges()
    {
        ChangeSet changes;

        const int startPos = startOf(stringLiteral);
        const QLatin1String replacement("QLatin1String(");

        if (isObjCStringLiteral)
            changes.replace(startPos, 1, replacement);
        else
            changes.insert(startPos, replacement);

        changes.insert(endOf(stringLiteral), ")");

        cppRefactoringChanges()->changeFile(fileName(), changes);
    }

private:
    StringLiteralAST *stringLiteral;
    bool isObjCStringLiteral;
};

class CStringToNSString: public CppQuickFixOperation
{
public:
    CStringToNSString(TextEditor::BaseTextEditor *editor)
        : CppQuickFixOperation(editor), stringLiteral(0), qlatin1Call(0)
    {}

    virtual QString description() const
    {
        return QApplication::translate("CppTools::QuickFix", "Convert to Objective-C String Literal");
    }

    virtual int match(const QList<AST *> &path)
    {
        if (path.isEmpty())
            return -1;

        int index = path.size() - 1;
        stringLiteral = path[index]->asStringLiteral();

        if (!stringLiteral)
            return -1;

        if (charAt(startOf(stringLiteral)) == QLatin1Char('@'))
            return -1;

        // check if it is already wrapped in QLatin1String or -Literal
        if (index-2 < 0)
            return index;

        CallAST *call = path[index-1]->asCall();
        PostfixExpressionAST *postfixExp = path[index-2]->asPostfixExpression();
        if (call && postfixExp
            && postfixExp->base_expression
            && postfixExp->postfix_expression_list
            && postfixExp->postfix_expression_list->value == call)
        {
            NameAST *callName = postfixExp->base_expression->asName();
            if (!callName)
                return index;

            if (!(postfixExp->postfix_expression_list->next)) {
                QByteArray callNameString(tokenAt(callName->firstToken()).spell());
                if (callNameString == "QLatin1String"
                    || callNameString == "QLatin1Literal"
                    )
                    qlatin1Call = postfixExp;
            }
        }

        return index;
    }

    virtual void createChanges()
    {
        ChangeSet changes;

        if (qlatin1Call) {
            changes.replace(startOf(qlatin1Call),
                            startOf(stringLiteral) - startOf(qlatin1Call),
                            QLatin1String("@"));
            changes.remove(endOf(stringLiteral),
                           endOf(qlatin1Call) - endOf(stringLiteral));
        } else {
            changes.insert(startOf(stringLiteral), "@");
        }

        cppRefactoringChanges()->changeFile(fileName(), changes);
    }

private:
    StringLiteralAST *stringLiteral;
    PostfixExpressionAST *qlatin1Call;
};

} // end of anonymous namespace


CppQuickFixOperation::CppQuickFixOperation(TextEditor::BaseTextEditor *editor)
    : TextEditor::QuickFixOperation(editor)
    , _refactoringChanges(0)
    , _topLevelNode(0)
{ }

CppQuickFixOperation::~CppQuickFixOperation()
{
    if (_refactoringChanges)
        delete _refactoringChanges;
}

int CppQuickFixOperation::match(TextEditor::QuickFixState *state)
{
    CppQuickFixState *s = static_cast<CppQuickFixState *>(state);
    _document = s->info.doc;
    if (_refactoringChanges)
        delete _refactoringChanges;
    CPPEditor *cppEditor = qobject_cast<CPPEditor*>(editor());
    _refactoringChanges = new CppTools::CppRefactoringChanges(s->info.snapshot, cppEditor->modelManager());
    return match(s->path);
}

QString CppQuickFixOperation::fileName() const
{ return document()->fileName(); }

void CppQuickFixOperation::apply()
{
    cppRefactoringChanges()->apply();
}

CppTools::CppRefactoringChanges *CppQuickFixOperation::cppRefactoringChanges() const
{ return _refactoringChanges; }

TextEditor::RefactoringChanges *CppQuickFixOperation::refactoringChanges() const
{ return cppRefactoringChanges(); }

Document::Ptr CppQuickFixOperation::document() const
{ return _document; }

const Snapshot &CppQuickFixOperation::snapshot() const
{
    return _refactoringChanges->snapshot();
}

const CPlusPlus::Token &CppQuickFixOperation::tokenAt(unsigned index) const
{ return _document->translationUnit()->tokenAt(index); }

int CppQuickFixOperation::startOf(unsigned index) const
{
    unsigned line, column;
    _document->translationUnit()->getPosition(tokenAt(index).begin(), &line, &column);
    return editor()->document()->findBlockByNumber(line - 1).position() + column - 1;
}

int CppQuickFixOperation::startOf(const CPlusPlus::AST *ast) const
{
    return startOf(ast->firstToken());
}

int CppQuickFixOperation::endOf(unsigned index) const
{
    unsigned line, column;
    _document->translationUnit()->getPosition(tokenAt(index).end(), &line, &column);
    return editor()->document()->findBlockByNumber(line - 1).position() + column - 1;
}

int CppQuickFixOperation::endOf(const CPlusPlus::AST *ast) const
{
    if (unsigned end = ast->lastToken())
        return endOf(end - 1);
    else
        return 0;
}

void CppQuickFixOperation::startAndEndOf(unsigned index, int *start, int *end) const
{
    unsigned line, column;
    CPlusPlus::Token token(tokenAt(index));
    _document->translationUnit()->getPosition(token.begin(), &line, &column);
    *start = editor()->document()->findBlockByNumber(line - 1).position() + column - 1;
    *end = *start + token.length();
}

bool CppQuickFixOperation::isCursorOn(unsigned tokenIndex) const
{
    QTextCursor tc = textCursor();
    int cursorBegin = tc.selectionStart();

    int start = startOf(tokenIndex);
    int end = endOf(tokenIndex);

    if (cursorBegin >= start && cursorBegin <= end)
        return true;

    return false;
}

bool CppQuickFixOperation::isCursorOn(const CPlusPlus::AST *ast) const
{
    QTextCursor tc = textCursor();
    int cursorBegin = tc.selectionStart();

    int start = startOf(ast);
    int end = endOf(ast);

    if (cursorBegin >= start && cursorBegin <= end)
        return true;

    return false;
}

void CppQuickFixOperation::move(ChangeSet *changeSet, unsigned tokenIndex,
                                int to)
{
    Q_ASSERT(changeSet);

    int start, end;
    startAndEndOf(tokenIndex, &start, &end);
    changeSet->move(start, end - start, to);
}

void CppQuickFixOperation::move(ChangeSet *changeSet, const CPlusPlus::AST *ast,
                                int to)
{
    Q_ASSERT(changeSet);

    const int start = startOf(ast);
    changeSet->move(start, endOf(ast) - start, to);
}

void CppQuickFixOperation::replace(ChangeSet *changeSet, unsigned tokenIndex,
                                   const QString &replacement)
{
    Q_ASSERT(changeSet);

    int start, end;
    startAndEndOf(tokenIndex, &start, &end);
    changeSet->replace(start, end - start, replacement);
}

void CppQuickFixOperation::replace(ChangeSet *changeSet,
                                   const CPlusPlus::AST *ast,
                                   const QString &replacement)
{
    Q_ASSERT(changeSet);

    const int start = startOf(ast);
    changeSet->replace(start, endOf(ast) - start, replacement);
}

void CppQuickFixOperation::remove(ChangeSet *changeSet, unsigned tokenIndex)
{
    Q_ASSERT(changeSet);

    int start, end;
    startAndEndOf(tokenIndex, &start, &end);
    changeSet->remove(start, end - start);
}

void CppQuickFixOperation::remove(ChangeSet *changeSet, const CPlusPlus::AST *ast)
{
    Q_ASSERT(changeSet);

    const int start = startOf(ast);
    changeSet->remove(start, endOf(ast) - start);
}

void CppQuickFixOperation::flip(ChangeSet *changeSet,
                                const CPlusPlus::AST *ast1,
                                const CPlusPlus::AST *ast2)
{
    Q_ASSERT(changeSet);

    const int start1 = startOf(ast1);
    const int start2 = startOf(ast2);
    changeSet->flip(start1, endOf(ast1) - start1,
                    start2, endOf(ast2) - start2);
}

void CppQuickFixOperation::copy(ChangeSet *changeSet, unsigned tokenIndex,
                                int to)
{
    Q_ASSERT(changeSet);

    int start, end;
    startAndEndOf(tokenIndex, &start, &end);
    changeSet->copy(start, end - start, to);
}

void CppQuickFixOperation::copy(ChangeSet *changeSet, const CPlusPlus::AST *ast,
                                int to)
{
    Q_ASSERT(changeSet);

    const int start = startOf(ast);
    changeSet->copy(start, endOf(ast) - start, to);
}

QString CppQuickFixOperation::textOf(const AST *ast) const
{
    return textOf(startOf(ast), endOf(ast));
}

CppQuickFixCollector::CppQuickFixCollector()
{
}

CppQuickFixCollector::~CppQuickFixCollector()
{
}

bool CppQuickFixCollector::supportsEditor(TextEditor::ITextEditable *editor)
{
    return CppTools::CppModelManagerInterface::instance()->isCppEditor(editor);
}

TextEditor::QuickFixState *CppQuickFixCollector::initializeCompletion(TextEditor::ITextEditable *editable)
{
    if (CPPEditor *editor = qobject_cast<CPPEditor *>(editable->widget())) {
        const SemanticInfo info = editor->semanticInfo();

        if (info.revision != editor->editorRevision()) {
            // outdated
            qWarning() << "TODO: outdated semantic info, force a reparse.";
            return 0;
        }

        if (info.doc) {
            ASTPath astPath(info.doc);

            const QList<AST *> path = astPath(editor->textCursor());
            if (! path.isEmpty()) {
                CppQuickFixState *state = new CppQuickFixState;
                state->path = path;
                state->info = info;
                return state;
            }
        }
    }

    return 0;
}

QList<TextEditor::QuickFixOperation::Ptr> CppQuickFixCollector::quickFixOperations(TextEditor::BaseTextEditor *editor) const
{
    QSharedPointer<RewriteLogicalAndOp> rewriteLogicalAndOp(new RewriteLogicalAndOp(editor));
    QSharedPointer<SplitIfStatementOp> splitIfStatementOp(new SplitIfStatementOp(editor));
    QSharedPointer<MoveDeclarationOutOfIfOp> moveDeclarationOutOfIfOp(new MoveDeclarationOutOfIfOp(editor));
    QSharedPointer<MoveDeclarationOutOfWhileOp> moveDeclarationOutOfWhileOp(new MoveDeclarationOutOfWhileOp(editor));
    QSharedPointer<SplitSimpleDeclarationOp> splitSimpleDeclarationOp(new SplitSimpleDeclarationOp(editor));
    QSharedPointer<AddBracesToIfOp> addBracesToIfOp(new AddBracesToIfOp(editor));
    QSharedPointer<UseInverseOp> useInverseOp(new UseInverseOp(editor));
    QSharedPointer<FlipBinaryOp> flipBinaryOp(new FlipBinaryOp(editor));
    QSharedPointer<WrapStringLiteral> wrapStringLiteral(new WrapStringLiteral(editor));
    QSharedPointer<CStringToNSString> wrapCString(new CStringToNSString(editor));

    QList<TextEditor::QuickFixOperation::Ptr> quickFixOperations;
    quickFixOperations.append(rewriteLogicalAndOp);
    quickFixOperations.append(splitIfStatementOp);
    quickFixOperations.append(moveDeclarationOutOfIfOp);
    quickFixOperations.append(moveDeclarationOutOfWhileOp);
    quickFixOperations.append(splitSimpleDeclarationOp);
    quickFixOperations.append(addBracesToIfOp);
    quickFixOperations.append(useInverseOp);
    quickFixOperations.append(flipBinaryOp);
    quickFixOperations.append(wrapStringLiteral);

    if (editor->mimeType() == CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE)
        quickFixOperations.append(wrapCString);

    return quickFixOperations;
}
