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

#if 0
    // Useful for debugging:
    static void dump(const QList<AST *> nodes)
    {
        qDebug() << "ASTPath dump," << nodes.size() << "nodes:";
        for (int i = 0; i < nodes.size(); ++i)
            qDebug() << qPrintable(QString(i + 1, QLatin1Char('-'))) << typeid(*nodes.at(i)).name();
    }
#endif

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

                if (_line < endLine || (_line == endLine && _column <= endColumn)) {
                    _nodes.append(ast);
                    return true;
                }
            }
        }

        return false;
    }
};

/*
    Rewrite
    a op b -> !(a invop b)
    (a op b) -> !(a invop b)
    !(a op b) -> (a invob b)
*/
class UseInverseOp: public QuickFixOperation
{
public:
    UseInverseOp()
        : binary(0), nested(0), negation(0)
    {}

    virtual QString description() const
    {
        return QLatin1String("Rewrite using ") + replacement; // ### tr?
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

    virtual void createChangeSet()
    {
        if (negation) {
            // can't remove parentheses since that might break precedence
            remove(negation->unary_op_token);
        } else if (nested) {
            insert(startOf(nested), "!");
        } else {
            insert(startOf(binary), "!(");
            insert(endOf(binary), ")");
        }
        replace(binary->binary_op_token, replacement);
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
class FlipBinaryOp: public QuickFixOperation
{
public:
    FlipBinaryOp()
        : binary(0)
    {}


    virtual QString description() const
    {
        if (replacement.isEmpty())
            return QLatin1String("Flip");
        else
            return QLatin1String("Flip to use ") + replacement; // ### tr?
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

    virtual void createChangeSet()
    {
        flip(binary->left_expression, binary->right_expression);
        if (! replacement.isEmpty())
            replace(binary->binary_op_token, replacement);
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
class RewriteLogicalAndOp: public QuickFixOperation
{
public:
    RewriteLogicalAndOp()
        : left(0), right(0), pattern(0)
    {}

    virtual QString description() const
    {
        return QLatin1String("Rewrite condition using ||"); // ### tr?
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

    virtual void createChangeSet()
    {
        setTopLevelNode(pattern);
        replace(pattern->binary_op_token, QLatin1String("||"));
        remove(left->unary_op_token);
        remove(right->unary_op_token);
        insert(startOf(pattern), QLatin1String("!("));
        insert(endOf(pattern), QLatin1String(")"));
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
    SplitSimpleDeclarationOp()
        : declaration(0)
    {}

    virtual QString description() const
    {
        return QLatin1String("Split declaration"); // ### tr?
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

    virtual void createChangeSet()
    {
        setTopLevelNode(declaration);
        SpecifierListAST *specifiers = declaration->decl_specifier_list;
        int declSpecifiersStart = startOf(specifiers->firstToken());
        int declSpecifiersEnd = endOf(specifiers->lastToken() - 1);
        int insertPos = endOf(declaration->semicolon_token);

        DeclaratorAST *prevDeclarator = declaration->declarator_list->value;

        for (DeclaratorListAST *it = declaration->declarator_list->next; it; it = it->next) {
            DeclaratorAST *declarator = it->value;

            insert(insertPos, QLatin1String("\n"));
            copy(declSpecifiersStart, declSpecifiersEnd, insertPos);
            insert(insertPos, QLatin1String(" "));
            move(declarator, insertPos);
            insert(insertPos, QLatin1String(";"));

            remove(endOf(prevDeclarator), startOf(declarator));

            prevDeclarator = declarator;
        }
    }

private:
    SimpleDeclarationAST *declaration;
};

/*
    Add curly braces to a if statement that doesn't already contain a
    compound statement.
*/
class AddBracesToIfOp: public QuickFixOperation
{
public:
    AddBracesToIfOp()
        : _statement(0)
    {}

    virtual QString description() const
    {
        return QLatin1String("Add curly braces"); // ### tr?
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

    virtual void createChangeSet()
    {
        setTopLevelNode(_statement);
        insert(endOf(_statement->firstToken() - 1), QLatin1String(" {"));
        insert(endOf(_statement->lastToken() - 1), "\n}");
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
class MoveDeclarationOutOfIfOp: public QuickFixOperation
{
public:
    MoveDeclarationOutOfIfOp()
        : condition(0), pattern(0), core(0)
    {}

    virtual QString description() const
    {
        return QLatin1String("Move declaration out of condition"); // ### tr?
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

    virtual void createChangeSet()
    {
        setTopLevelNode(pattern);

        copy(core, startOf(condition));

        int insertPos = startOf(pattern);
        move(condition, insertPos);
        insert(insertPos, QLatin1String(";\n"));
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
class MoveDeclarationOutOfWhileOp: public QuickFixOperation
{
public:
    MoveDeclarationOutOfWhileOp()
        : condition(0), pattern(0), core(0)
    {}

    virtual QString description() const
    {
        return QLatin1String("Move declaration out of condition"); // ### tr?
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

    virtual void createChangeSet()
    {
        setTopLevelNode(pattern);

        insert(startOf(condition), QLatin1String("("));
        insert(endOf(condition), QLatin1String(") != 0"));

        int insertPos = startOf(pattern);
        move(startOf(condition), startOf(core), insertPos);
        copy(core, insertPos);
        insert(insertPos, QLatin1String(";\n"));
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
class SplitIfStatementOp: public QuickFixOperation
{
public:
    SplitIfStatementOp()
        : condition(0), pattern(0)
    {}

    virtual QString description() const
    {
        return QLatin1String("Split if statement"); // ### tr?
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

    virtual void createChangeSet()
    {
        Token binaryToken = tokenAt(condition->binary_op_token);

        if (binaryToken.is(T_AMPER_AMPER))
            splitAndCondition();
        else
            splitOrCondition();
    }

    void splitAndCondition()
    {
        setTopLevelNode(pattern);

        int startPos = startOf(pattern);
        insert(startPos, QLatin1String("if ("));
        move(condition->left_expression, startPos);
        insert(startPos, QLatin1String(") {\n"));

        remove(endOf(condition->left_expression), startOf(condition->right_expression));
        insert(endOf(pattern), QLatin1String("\n}"));
    }

    void splitOrCondition()
    {
        StatementAST *ifTrueStatement = pattern->statement;
        CompoundStatementAST *compoundStatement = ifTrueStatement->asCompoundStatement();

        setTopLevelNode(pattern);

        int insertPos = endOf(ifTrueStatement);
        if (compoundStatement)
            insert(insertPos, QLatin1String(" "));
        else
            insert(insertPos, QLatin1String("\n"));
        insert(insertPos, QLatin1String("else if ("));
        move(startOf(condition->right_expression), startOf(pattern->rparen_token), insertPos);
        insert(insertPos, QLatin1String(")"));
        copy(endOf(pattern->rparen_token), endOf(pattern->statement), insertPos);
        
        remove(endOf(condition->left_expression), startOf(condition->right_expression));
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
class WrapStringLiteral: public QuickFixOperation
{
public:
    WrapStringLiteral()
        : stringLiteral(0), isObjCStringLiteral(false)
    {}

    virtual QString description() const
    {
        return QLatin1String("Enclose in QLatin1String(...)"); // ### tr?
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

    virtual void createChangeSet()
    {
        const int startPos = startOf(stringLiteral);
        const QLatin1String replacement("QLatin1String(");

        if (isObjCStringLiteral)
            replace(startPos, startPos + 1, replacement);
        else
            insert(startPos, replacement);

        insert(endOf(stringLiteral), ")");
    }

private:
    StringLiteralAST *stringLiteral;
    bool isObjCStringLiteral;
};

class CStringToNSString: public QuickFixOperation
{
public:
    CStringToNSString()
        : stringLiteral(0), qlatin1Call(0)
    {}

    virtual QString description() const
    { return QLatin1String("Convert to Objective-C string literal"); }// ### tr?

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

    virtual void createChangeSet()
    {
        if (qlatin1Call) {
            replace(startOf(qlatin1Call), startOf(stringLiteral), QLatin1String("@"));
            remove(endOf(stringLiteral), endOf(qlatin1Call));
        } else {
            insert(startOf(stringLiteral), "@");
        }
    }

private:
    StringLiteralAST *stringLiteral;
    PostfixExpressionAST *qlatin1Call;
};

/*
  Replace
    a + b
  With
    a % b
  If a and b are of string type.
*/
class UseFastStringConcatenation: public QuickFixOperation
{
public:
    UseFastStringConcatenation()
    {}

    virtual QString description() const
    {
        return QLatin1String("Use fast string concatenation with %"); // ### tr?
    }

    virtual int match(const QList<AST *> &path)
    {
        if (path.isEmpty())
            return -1;

        // need to search 'up' too
        int index = path.size() - 1;
        if (BinaryExpressionAST *binary = asPlusNode(path[index])) {
            while (0 != (binary = asPlusNode(binary->left_expression)))
                _binaryExpressions.prepend(binary);
        }

        // search 'down'
        for (index = path.size() - 1; index != -1; --index) {
            AST *node = path.at(index);
            if (BinaryExpressionAST *binary = asPlusNode(node)) {
                _binaryExpressions.append(binary);
            } else if (! _binaryExpressions.isEmpty()) {
                break;
            }
        }

        if (_binaryExpressions.isEmpty())
            return -1;

        // verify types of arguments
        BinaryExpressionAST *prevBinary = 0;
        foreach (BinaryExpressionAST *binary, _binaryExpressions) {
            if (binary->left_expression != prevBinary) {
                if (!hasCorrectType(binary->left_expression))
                    return -1;
            }
            if (binary->right_expression != prevBinary) {
                if (!hasCorrectType(binary->right_expression))
                    return -1;
            }
            prevBinary = binary;
        }

        return index + _binaryExpressions.size();
    }

    virtual void createChangeSet()
    {
        // replace + -> %
        foreach (BinaryExpressionAST *binary, _binaryExpressions)
            replace(binary->binary_op_token, "%");

        // wrap literals in QLatin1Literal
        foreach (StringLiteralAST *literal, _stringLiterals) {
            insert(startOf(literal), "QLatin1Literal(");
            insert(endOf(literal), ")");
        }

        // replace QLatin1String/QString/QByteArray(literal) -> QLatin1Literal(literal)
        foreach (PostfixExpressionAST *postfix, _incorrectlyWrappedLiterals) {
            replace(postfix->base_expression, "QLatin1Literal");
        }
    }

    BinaryExpressionAST *asPlusNode(AST *ast)
    {
        BinaryExpressionAST *binary = ast->asBinaryExpression();
        if (binary && tokenAt(binary->binary_op_token).kind() == T_PLUS)
            return binary;
        return 0;
    }

    bool hasCorrectType(ExpressionAST *ast)
    {
        if (StringLiteralAST *literal = ast->asStringLiteral()) {
            _stringLiterals += literal;
            return true;
        }

        if (PostfixExpressionAST *postfix = ast->asPostfixExpression()) {
            if (postfix->base_expression && postfix->postfix_expression_list
                && postfix->postfix_expression_list->value
                && !postfix->postfix_expression_list->next)
            {
                NameAST *name = postfix->base_expression->asName();
                CallAST *call = postfix->postfix_expression_list->value->asCall();
                if (name && call) {
                    QByteArray nameStr(name->name->identifier()->chars());
                    if ((nameStr == "QLatin1String"
                         || nameStr == "QString"
                         || nameStr == "QByteArray")
                        && call->expression_list
                        && call->expression_list->value
                        && call->expression_list->value->asStringLiteral()
                        && !call->expression_list->next)
                    {
                        _incorrectlyWrappedLiterals += postfix;
                        return true;
                    }
                }
            }
        }

        const QList<LookupItem> &lookup = typeOf(ast);
        if (lookup.isEmpty())
            return false;
        return isQtStringType(lookup[0].type());
    }

    bool isBuiltinStringType(FullySpecifiedType type)
    {
        // char*
        if (PointerType *ptrTy = type->asPointerType())
            if (IntegerType *intTy = ptrTy->elementType()->asIntegerType())
                if (intTy->kind() == IntegerType::Char)
                    return true;
        return false;
    }

    bool isQtStringType(FullySpecifiedType type)
    {
        if (NamedType *nameTy = type->asNamedType()) {
            if (!nameTy->name() || !nameTy->name()->identifier())
                return false;

            QByteArray name(nameTy->name()->identifier()->chars());
            if (name == "QString"
                || name == "QByteArray"
                || name == "QLatin1String"
                || name == "QLatin1Literal"
                || name == "QStringRef"
                || name == "QChar"
                )
                return true;
        }

        return false;
    }

private:
    QList<BinaryExpressionAST *> _binaryExpressions;
    QList<StringLiteralAST *> _stringLiterals;
    QList<PostfixExpressionAST *> _incorrectlyWrappedLiterals;
};

} // end of anonymous namespace


QuickFixOperation::QuickFixOperation()
    : _editor(0), _topLevelNode(0)
{ }

QuickFixOperation::~QuickFixOperation()
{ }

CPlusPlus::AST *QuickFixOperation::topLevelNode() const
{ return _topLevelNode; }

void QuickFixOperation::setTopLevelNode(CPlusPlus::AST *topLevelNode)
{ _topLevelNode = topLevelNode; }

const Utils::ChangeSet &QuickFixOperation::changeSet() const
{ return _changeSet; }

Document::Ptr QuickFixOperation::document() const
{ return _document; }

void QuickFixOperation::setDocument(CPlusPlus::Document::Ptr document)
{ _document = document; }

Snapshot QuickFixOperation::snapshot() const
{ return _snapshot; }

void QuickFixOperation::setSnapshot(const CPlusPlus::Snapshot &snapshot)
{
    _snapshot = snapshot;
}

CPPEditor *QuickFixOperation::editor() const
{ return _editor; }

void QuickFixOperation::setEditor(CPPEditor *editor)
{ _editor = editor; }

QTextCursor QuickFixOperation::textCursor() const
{ return _textCursor; }

void QuickFixOperation::setTextCursor(const QTextCursor &cursor)
{ _textCursor = cursor; }

int QuickFixOperation::selectionStart() const
{ return _textCursor.selectionStart(); }

int QuickFixOperation::selectionEnd() const
{ return _textCursor.selectionEnd(); }

const CPlusPlus::Token &QuickFixOperation::tokenAt(unsigned index) const
{ return _document->translationUnit()->tokenAt(index); }

int QuickFixOperation::startOf(unsigned index) const
{
    unsigned line, column;
    _document->translationUnit()->getPosition(tokenAt(index).begin(), &line, &column);
    return _textCursor.document()->findBlockByNumber(line - 1).position() + column - 1;
}

int QuickFixOperation::startOf(const CPlusPlus::AST *ast) const
{
    return startOf(ast->firstToken());
}

int QuickFixOperation::endOf(unsigned index) const
{
    unsigned line, column;
    _document->translationUnit()->getPosition(tokenAt(index).end(), &line, &column);
    return _textCursor.document()->findBlockByNumber(line - 1).position() + column - 1;
}

int QuickFixOperation::endOf(const CPlusPlus::AST *ast) const
{
    return endOf(ast->lastToken() - 1);
}

void QuickFixOperation::startAndEndOf(unsigned index, int *start, int *end) const
{
    unsigned line, column;
    CPlusPlus::Token token(tokenAt(index));
    _document->translationUnit()->getPosition(token.begin(), &line, &column);
    *start = _textCursor.document()->findBlockByNumber(line - 1).position() + column - 1;
    *end = *start + token.length();
}

bool QuickFixOperation::isCursorOn(unsigned tokenIndex) const
{
    QTextCursor tc = textCursor();
    int cursorBegin = tc.selectionStart();

    int start = startOf(tokenIndex);
    int end = endOf(tokenIndex);

    if (cursorBegin >= start && cursorBegin <= end)
        return true;

    return false;
}

bool QuickFixOperation::isCursorOn(const CPlusPlus::AST *ast) const
{
    QTextCursor tc = textCursor();
    int cursorBegin = tc.selectionStart();

    int start = startOf(ast);
    int end = endOf(ast);

    if (cursorBegin >= start && cursorBegin <= end)
        return true;

    return false;
}

QuickFixOperation::Range QuickFixOperation::createRange(AST *ast) const
{    
    QTextCursor tc = _textCursor;
    Range r(tc);
    r.begin.setPosition(startOf(ast));
    r.end.setPosition(endOf(ast));
    return r;
}

void QuickFixOperation::reindent(const Range &range)
{
    QTextCursor tc = range.begin;
    tc.setPosition(range.end.position(), QTextCursor::KeepAnchor);
    _editor->indentInsertedText(tc);
}

void QuickFixOperation::move(int start, int end, int to)
{
    _changeSet.move(start, end-start, to);
}

void QuickFixOperation::move(unsigned tokenIndex, int to)
{
    int start, end;
    startAndEndOf(tokenIndex, &start, &end);
    move(start, end, to);
}

void QuickFixOperation::move(const CPlusPlus::AST *ast, int to)
{
    move(startOf(ast), endOf(ast), to);
}

void QuickFixOperation::replace(int start, int end, const QString &replacement)
{
    _changeSet.replace(start, end-start, replacement);
}

void QuickFixOperation::replace(unsigned tokenIndex, const QString &replacement)
{
    int start, end;
    startAndEndOf(tokenIndex, &start, &end);
    replace(start, end, replacement);
}

void QuickFixOperation::replace(const CPlusPlus::AST *ast, const QString &replacement)
{
    replace(startOf(ast), endOf(ast), replacement);
}

void QuickFixOperation::insert(int at, const QString &text)
{
    _changeSet.insert(at, text);
}

void QuickFixOperation::remove(int start, int end)
{
    _changeSet.remove(start, end-start);
}

void QuickFixOperation::remove(unsigned tokenIndex)
{
    int start, end;
    startAndEndOf(tokenIndex, &start, &end);
    remove(start, end);
}

void QuickFixOperation::remove(const CPlusPlus::AST *ast)
{
    remove(startOf(ast), endOf(ast));
}

void QuickFixOperation::flip(int start1, int end1, int start2, int end2)
{
    _changeSet.flip(start1, end1-start1, start2, end2-start2);
}

void QuickFixOperation::flip(const CPlusPlus::AST *ast1, const CPlusPlus::AST *ast2)
{
    flip(startOf(ast1), endOf(ast1), startOf(ast2), endOf(ast2));
}

void QuickFixOperation::copy(int start, int end, int to)
{
    _changeSet.copy(start, end-start, to);
}

void QuickFixOperation::copy(unsigned tokenIndex, int to)
{
    int start, end;
    startAndEndOf(tokenIndex, &start, &end);
    copy(start, end, to);
}

void QuickFixOperation::copy(const CPlusPlus::AST *ast, int to)
{
    copy(startOf(ast), endOf(ast), to);
}

QString QuickFixOperation::textOf(int firstOffset, int lastOffset) const
{
    QTextCursor tc = _textCursor;
    tc.setPosition(firstOffset);
    tc.setPosition(lastOffset, QTextCursor::KeepAnchor);
    return tc.selectedText();
}

QString QuickFixOperation::textOf(const AST *ast) const
{
    return textOf(startOf(ast), endOf(ast));
}

QChar QuickFixOperation::charAt(int offset) const
{
    return textOf(offset, offset + 1).at(0);
}

void QuickFixOperation::apply()
{
    Range range;

    if (_topLevelNode)
        range = createRange(_topLevelNode);

    _textCursor.beginEditBlock();

    _changeSet.apply(&_textCursor);

    if (_topLevelNode)
        reindent(range);

    _textCursor.endEditBlock();
}

/**
 * Returns a list of possible fully specified types associated with the
 * given expression.
 *
 * NOTE: The fully specified types only stay valid until the next call to typeOf.
 */
const QList<LookupItem> QuickFixOperation::typeOf(CPlusPlus::ExpressionAST *ast)
{
    unsigned line, column;
    document()->translationUnit()->getTokenStartPosition(ast->firstToken(), &line, &column);
    Symbol *lastVisibleSymbol = document()->findSymbolAt(line, column);

    _lookupContext = LookupContext(lastVisibleSymbol, document(), document(), snapshot());

    ResolveExpression resolveExpression(_lookupContext);
    return resolveExpression(ast);
}

CPPQuickFixCollector::CPPQuickFixCollector()
    : _modelManager(CppTools::CppModelManagerInterface::instance()), _editable(0), _editor(0)
{ }

CPPQuickFixCollector::~CPPQuickFixCollector()
{ }

TextEditor::ITextEditable *CPPQuickFixCollector::editor() const
{ return _editable; }

int CPPQuickFixCollector::startPosition() const
{ return _editable->position(); }

bool CPPQuickFixCollector::supportsEditor(TextEditor::ITextEditable *editor)
{ return qobject_cast<CPPEditorEditable *>(editor) != 0; }

bool CPPQuickFixCollector::triggersCompletion(TextEditor::ITextEditable *)
{ return false; }

int CPPQuickFixCollector::startCompletion(TextEditor::ITextEditable *editable)
{
    Q_ASSERT(editable != 0);

    _editable = editable;
    _editor = qobject_cast<CPPEditor *>(editable->widget());
    Q_ASSERT(_editor != 0);

    const SemanticInfo info = _editor->semanticInfo();

    if (info.revision != _editor->editorRevision()) {
        // outdated
        qWarning() << "TODO: outdated semantic info, force a reparse.";
        return -1;
    }

    if (info.doc) {
        ASTPath astPath(info.doc);

        const QList<AST *> path = astPath(_editor->textCursor());
        if (path.isEmpty())
            return -1;

        QSharedPointer<RewriteLogicalAndOp> rewriteLogicalAndOp(new RewriteLogicalAndOp());
        QSharedPointer<SplitIfStatementOp> splitIfStatementOp(new SplitIfStatementOp());
        QSharedPointer<MoveDeclarationOutOfIfOp> moveDeclarationOutOfIfOp(new MoveDeclarationOutOfIfOp());
        QSharedPointer<MoveDeclarationOutOfWhileOp> moveDeclarationOutOfWhileOp(new MoveDeclarationOutOfWhileOp());
        QSharedPointer<SplitSimpleDeclarationOp> splitSimpleDeclarationOp(new SplitSimpleDeclarationOp());
        QSharedPointer<AddBracesToIfOp> addBracesToIfOp(new AddBracesToIfOp());
        QSharedPointer<UseInverseOp> useInverseOp(new UseInverseOp());
        QSharedPointer<FlipBinaryOp> flipBinaryOp(new FlipBinaryOp());
        QSharedPointer<WrapStringLiteral> wrapStringLiteral(new WrapStringLiteral());
        QSharedPointer<CStringToNSString> wrapCString(new CStringToNSString());
        QSharedPointer<UseFastStringConcatenation> useFastStringConcat(new UseFastStringConcatenation());

        QList<QuickFixOperationPtr> candidates;
        candidates.append(rewriteLogicalAndOp);
        candidates.append(splitIfStatementOp);
        candidates.append(moveDeclarationOutOfIfOp);
        candidates.append(moveDeclarationOutOfWhileOp);
        candidates.append(splitSimpleDeclarationOp);
        candidates.append(addBracesToIfOp);
        candidates.append(useInverseOp);
        candidates.append(flipBinaryOp);
        candidates.append(wrapStringLiteral);
        candidates.append(wrapCString);
        candidates.append(useFastStringConcat);

        QMap<int, QList<QuickFixOperationPtr> > matchedOps;

        foreach (QuickFixOperationPtr op, candidates) {
            op->setSnapshot(info.snapshot);
            op->setDocument(info.doc);
            op->setEditor(_editor);
            op->setTextCursor(_editor->textCursor());
            int priority = op->match(path);
            if (priority != -1)
                matchedOps[priority].append(op);
        }

        QMapIterator<int, QList<QuickFixOperationPtr> > it(matchedOps);
        it.toBack();
        if (it.hasPrevious()) {
            it.previous();

            _quickFixes = it.value();
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
        perform(quickFix);
    }
}

void CPPQuickFixCollector::perform(QuickFixOperationPtr op)
{
    op->setTextCursor(_editor->textCursor());
    op->createChangeSet();
    op->apply();
}

void CPPQuickFixCollector::cleanup()
{
    _quickFixes.clear();
}
