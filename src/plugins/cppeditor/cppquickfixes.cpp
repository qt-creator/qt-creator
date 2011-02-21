/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "cppcompleteswitch.h"
#include "cppeditor.h"
#include "cppquickfix.h"
#include "cppinsertdecldef.h"
#include "cppinsertqtpropertymembers.h"
#include "cppquickfixcollector.h"

#include <ASTVisitor.h>
#include <AST.h>
#include <ASTMatcher.h>
#include <ASTPatternBuilder.h>
#include <CoreTypes.h>
#include <Literals.h>
#include <Name.h>
#include <Names.h>
#include <Symbol.h>
#include <Symbols.h>
#include <Token.h>
#include <TranslationUnit.h>
#include <Type.h>

#include <cplusplus/DependencyTable.h>
#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>
#include <cplusplus/CppRewriter.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cpprefactoringchanges.h>
#include <cpptools/insertionpointlocator.h>
#include <extensionsystem/iplugin.h>

#include <QtCore/QFileInfo>
#include <QtGui/QApplication>
#include <QtGui/QTextBlock>
#include <QtGui/QTextCursor>

using namespace CppEditor;
using namespace CppEditor::Internal;
using namespace CppTools;
using namespace TextEditor;
using namespace CPlusPlus;
using namespace Utils;

namespace {

/*
    Rewrite
    a op b -> !(a invop b)
    (a op b) -> !(a invop b)
    !(a op b) -> (a invob b)

    Activates on: <= < > >= == !=
*/
class UseInverseOp: public CppQuickFixFactory
{
public:
    virtual QList<CppQuickFixOperation::Ptr> match(const CppQuickFixState &state)
    {
        QList<CppQuickFixOperation::Ptr> result;
        const CppRefactoringFile &file = state.currentFile();

        const QList<AST *> &path = state.path();
        int index = path.size() - 1;
        BinaryExpressionAST *binary = path.at(index)->asBinaryExpression();
        if (! binary)
            return result;
        if (! state.isCursorOn(binary->binary_op_token))
            return result;

        Kind invertToken;
        switch (file.tokenAt(binary->binary_op_token).kind()) {
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
            return result;
        }

        result.append(CppQuickFixOperation::Ptr(new Operation(state, index, binary, invertToken)));
        return result;
    }

private:
    class Operation: public CppQuickFixOperation
    {
        BinaryExpressionAST *binary;
        NestedExpressionAST *nested;
        UnaryExpressionAST *negation;

        QString replacement;

    public:
        Operation(const CppQuickFixState &state, int priority, BinaryExpressionAST *binary, Kind invertToken)
            : CppQuickFixOperation(state, priority)
            , binary(binary)
        {
            Token tok;
            tok.f.kind = invertToken;
            replacement = QLatin1String(tok.spell());

            // check for enclosing nested expression
            if (priority - 1 >= 0)
                nested = state.path()[priority - 1]->asNestedExpression();

            // check for ! before parentheses
            if (nested && priority - 2 >= 0) {
                negation = state.path()[priority - 2]->asUnaryExpression();
                if (negation && ! state.currentFile().tokenAt(negation->unary_op_token).is(T_EXCLAIM))
                    negation = 0;
            }
        }

        virtual QString description() const
        {
            return QApplication::translate("CppTools::QuickFix", "Rewrite Using %1").arg(replacement);
        }

        virtual void performChanges(CppRefactoringFile *currentFile, CppRefactoringChanges *)
        {
            ChangeSet changes;
            if (negation) {
                // can't remove parentheses since that might break precedence
                changes.remove(currentFile->range(negation->unary_op_token));
            } else if (nested) {
                changes.insert(currentFile->startOf(nested), "!");
            } else {
                changes.insert(currentFile->startOf(binary), "!(");
                changes.insert(currentFile->endOf(binary), ")");
            }
            changes.replace(currentFile->range(binary->binary_op_token), replacement);
            currentFile->change(changes);
        }
    };
};

/*
    Rewrite
    a op b

    As
    b flipop a

    Activates on: <= < > >= == != && ||
*/
class FlipBinaryOp: public CppQuickFixFactory
{
public:
    virtual QList<QuickFixOperation::Ptr> match(const CppQuickFixState &state)
    {
        QList<QuickFixOperation::Ptr> result;
        const QList<AST *> &path = state.path();
        const CppRefactoringFile &file = state.currentFile();

        int index = path.size() - 1;
        BinaryExpressionAST *binary = path.at(index)->asBinaryExpression();
        if (! binary)
            return result;
        if (! state.isCursorOn(binary->binary_op_token))
            return result;

        Kind flipToken;
        switch (file.tokenAt(binary->binary_op_token).kind()) {
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
            return result;
        }

        QString replacement;
        if (flipToken != T_EOF_SYMBOL) {
            Token tok;
            tok.f.kind = flipToken;
            replacement = QLatin1String(tok.spell());
        }

        result.append(QuickFixOperation::Ptr(new Operation(state, index, binary, replacement)));
        return result;
    }

private:
    class Operation: public CppQuickFixOperation
    {
    public:
        Operation(const CppQuickFixState &state, int priority, BinaryExpressionAST *binary, QString replacement)
            : CppQuickFixOperation(state)
            , binary(binary)
            , replacement(replacement)
        {
            setPriority(priority);
        }

        virtual QString description() const
        {
            if (replacement.isEmpty())
                return QApplication::translate("CppTools::QuickFix", "Swap Operands");
            else
                return QApplication::translate("CppTools::QuickFix", "Rewrite Using %1").arg(replacement);
        }

        virtual void performChanges(CppRefactoringFile *currentFile, CppRefactoringChanges *)
        {
            ChangeSet changes;

            changes.flip(currentFile->range(binary->left_expression), currentFile->range(binary->right_expression));
            if (! replacement.isEmpty())
                changes.replace(currentFile->range(binary->binary_op_token), replacement);

            currentFile->change(changes);
        }

    private:
        BinaryExpressionAST *binary;
        QString replacement;
    };
};

/*
    Rewrite
    !a && !b

    As
    !(a || b)

    Activates on: &&
*/
class RewriteLogicalAndOp: public CppQuickFixFactory
{
public:
    virtual QList<QuickFixOperation::Ptr> match(const CppQuickFixState &state)
    {
        QList<QuickFixOperation::Ptr> result;
        BinaryExpressionAST *expression = 0;
        const QList<AST *> &path = state.path();
        const CppRefactoringFile &file = state.currentFile();

        int index = path.size() - 1;
        for (; index != -1; --index) {
            expression = path.at(index)->asBinaryExpression();
            if (expression)
                break;
        }

        if (! expression)
            return result;

        if (! state.isCursorOn(expression->binary_op_token))
            return result;

        QSharedPointer<Operation> op(new Operation(state));

        if (expression->match(op->pattern, &matcher) &&
                file.tokenAt(op->pattern->binary_op_token).is(T_AMPER_AMPER) &&
                file.tokenAt(op->left->unary_op_token).is(T_EXCLAIM) &&
                file.tokenAt(op->right->unary_op_token).is(T_EXCLAIM)) {
            op->setDescription(QApplication::translate("CppTools::QuickFix", "Rewrite Condition Using ||"));
            op->setPriority(index);
            result.append(op);
        }

        return result;
    }

private:
    class Operation: public CppQuickFixOperation
    {
    public:
        QSharedPointer<ASTPatternBuilder> mk;
        UnaryExpressionAST *left;
        UnaryExpressionAST *right;
        BinaryExpressionAST *pattern;

        Operation(const CppQuickFixState &state)
            : CppQuickFixOperation(state)
            , mk(new ASTPatternBuilder)
        {
            left = mk->UnaryExpression();
            right = mk->UnaryExpression();
            pattern = mk->BinaryExpression(left, right);
        }

        virtual void performChanges(CppRefactoringFile *currentFile, CppRefactoringChanges *)
        {
            ChangeSet changes;
            changes.replace(currentFile->range(pattern->binary_op_token), QLatin1String("||"));
            changes.remove(currentFile->range(left->unary_op_token));
            changes.remove(currentFile->range(right->unary_op_token));
            const int start = currentFile->startOf(pattern);
            const int end = currentFile->endOf(pattern);
            changes.insert(start, QLatin1String("!("));
            changes.insert(end, QLatin1String(")"));

            currentFile->change(changes);
            currentFile->indent(currentFile->range(pattern));
        }
    };

private:
    ASTMatcher matcher;
};

/*
    Rewrite
    int *a, b;

    As
    int *a;
    int b;

    Activates on: the type or the variable names.
*/
class SplitSimpleDeclarationOp: public CppQuickFixFactory
{
    static bool checkDeclaration(SimpleDeclarationAST *declaration)
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

public:
    virtual QList<CppQuickFixOperation::Ptr> match(const CppQuickFixState &state)
    {
        QList<CppQuickFixOperation::Ptr> result;
        CoreDeclaratorAST *core_declarator = 0;
        const QList<AST *> &path = state.path();
        const CppRefactoringFile &file = state.currentFile();

        for (int index = path.size() - 1; index != -1; --index) {
            AST *node = path.at(index);

            if (CoreDeclaratorAST *coreDecl = node->asCoreDeclarator())
                core_declarator = coreDecl;

            else if (SimpleDeclarationAST *simpleDecl = node->asSimpleDeclaration()) {
                if (checkDeclaration(simpleDecl)) {
                    SimpleDeclarationAST *declaration = simpleDecl;

                    const int cursorPosition = file.cursor().selectionStart();

                    const int startOfDeclSpecifier = file.startOf(declaration->decl_specifier_list->firstToken());
                    const int endOfDeclSpecifier = file.endOf(declaration->decl_specifier_list->lastToken() - 1);

                    if (cursorPosition >= startOfDeclSpecifier && cursorPosition <= endOfDeclSpecifier) {
                        // the AST node under cursor is a specifier.
                        return singleResult(new Operation(state, index, declaration));
                    }

                    if (core_declarator && state.isCursorOn(core_declarator)) {
                        // got a core-declarator under the text cursor.
                        return singleResult(new Operation(state, index, declaration));
                    }
                }

                break;
            }
        }

        return result;
    }

private:
    class Operation: public CppQuickFixOperation
    {
    public:
        Operation(const CppQuickFixState &state, int priority, SimpleDeclarationAST *decl)
            : CppQuickFixOperation(state, priority)
            , declaration(decl)
        {
            setDescription(QApplication::translate("CppTools::QuickFix",
                                                   "Split Declaration"));
        }

        virtual void performChanges(CppRefactoringFile *currentFile, CppRefactoringChanges *)
        {
            ChangeSet changes;

            SpecifierListAST *specifiers = declaration->decl_specifier_list;
            int declSpecifiersStart = currentFile->startOf(specifiers->firstToken());
            int declSpecifiersEnd = currentFile->endOf(specifiers->lastToken() - 1);
            int insertPos = currentFile->endOf(declaration->semicolon_token);

            DeclaratorAST *prevDeclarator = declaration->declarator_list->value;

            for (DeclaratorListAST *it = declaration->declarator_list->next; it; it = it->next) {
                DeclaratorAST *declarator = it->value;

                changes.insert(insertPos, QLatin1String("\n"));
                changes.copy(declSpecifiersStart, declSpecifiersEnd, insertPos);
                changes.insert(insertPos, QLatin1String(" "));
                changes.move(currentFile->range(declarator), insertPos);
                changes.insert(insertPos, QLatin1String(";"));

                const int prevDeclEnd = currentFile->endOf(prevDeclarator);
                changes.remove(prevDeclEnd, currentFile->startOf(declarator));

                prevDeclarator = declarator;
            }

            currentFile->change(changes);
            currentFile->indent(currentFile->range(declaration));
        }

    private:
        SimpleDeclarationAST *declaration;
    };
};

/*
    Add curly braces to a if statement that doesn't already contain a
    compound statement. I.e.

    if (a)
        b;
    becomes
    if (a) {
        b;
    }

    Activates on: the if
*/
class AddBracesToIfOp: public CppQuickFixFactory
{
public:
    virtual QList<CppQuickFixOperation::Ptr> match(const CppQuickFixState &state)
    {
        const QList<AST *> &path = state.path();

        // show when we're on the 'if' of an if statement
        int index = path.size() - 1;
        IfStatementAST *ifStatement = path.at(index)->asIfStatement();
        if (ifStatement && state.isCursorOn(ifStatement->if_token) && ifStatement->statement
            && ! ifStatement->statement->asCompoundStatement()) {
            return singleResult(new Operation(state, index, ifStatement->statement));
        }

        // or if we're on the statement contained in the if
        // ### This may not be such a good idea, consider nested ifs...
        for (; index != -1; --index) {
            IfStatementAST *ifStatement = path.at(index)->asIfStatement();
            if (ifStatement && ifStatement->statement
                && state.isCursorOn(ifStatement->statement)
                && ! ifStatement->statement->asCompoundStatement()) {
                return singleResult(new Operation(state, index, ifStatement->statement));
            }
        }

        // ### This could very well be extended to the else branch
        // and other nodes entirely.

        return noResult();
    }

private:
    class Operation: public CppQuickFixOperation
    {
    public:
        Operation(const CppQuickFixState &state, int priority, StatementAST *statement)
            : CppQuickFixOperation(state, priority)
            , _statement(statement)
        {
            setDescription(QApplication::translate("CppTools::QuickFix",
                                                   "Add Curly Braces"));
        }

        virtual void performChanges(CppRefactoringFile *currentFile, CppRefactoringChanges *)
        {
            ChangeSet changes;

            const int start = currentFile->endOf(_statement->firstToken() - 1);
            changes.insert(start, QLatin1String(" {"));

            const int end = currentFile->endOf(_statement->lastToken() - 1);
            changes.insert(end, "\n}");

            currentFile->change(changes);
            currentFile->indent(Utils::ChangeSet::Range(start, end));
        }

    private:
        StatementAST *_statement;
    };
};

/*
    Replace
    if (Type name = foo()) {...}

    With
    Type name = foo;
    if (name) {...}

    Activates on: the name of the introduced variable
*/
class MoveDeclarationOutOfIfOp: public CppQuickFixFactory
{
public:
    virtual QList<CppQuickFixOperation::Ptr> match(const CppQuickFixState &state)
    {
        const QList<AST *> &path = state.path();
        QSharedPointer<Operation> op(new Operation(state));

        int index = path.size() - 1;
        for (; index != -1; --index) {
            if (IfStatementAST *statement = path.at(index)->asIfStatement()) {
                if (statement->match(op->pattern, &op->matcher) && op->condition->declarator) {
                    DeclaratorAST *declarator = op->condition->declarator;
                    op->core = declarator->core_declarator;
                    if (! op->core)
                        return noResult();

                    if (state.isCursorOn(op->core)) {
                        QList<CppQuickFixOperation::Ptr> result;
                        op->setPriority(index);
                        result.append(op);
                        return result;
                    }
                }
            }
        }

        return noResult();
    }

private:
    class Operation: public CppQuickFixOperation
    {
    public:
        Operation(const CppQuickFixState &state)
            : CppQuickFixOperation(state)
        {
            setDescription(QApplication::translate("CppTools::QuickFix",
                                                   "Move Declaration out of Condition"));

            condition = mk.Condition();
            pattern = mk.IfStatement(condition);
        }

        virtual void performChanges(CppRefactoringFile *currentFile, CppRefactoringChanges *)
        {
            ChangeSet changes;

            changes.copy(currentFile->range(core), currentFile->startOf(condition));

            int insertPos = currentFile->startOf(pattern);
            changes.move(currentFile->range(condition), insertPos);
            changes.insert(insertPos, QLatin1String(";\n"));

            currentFile->change(changes);
            currentFile->indent(currentFile->range(pattern));
        }

        ASTMatcher matcher;
        ASTPatternBuilder mk;
        CPPEditorWidget *editor;
        ConditionAST *condition;
        IfStatementAST *pattern;
        CoreDeclaratorAST *core;
    };
};

/*
    Replace
    while (Type name = foo()) {...}

    With
    Type name;
    while ((name = foo()) != 0) {...}

    Activates on: the name of the introduced variable
*/
class MoveDeclarationOutOfWhileOp: public CppQuickFixFactory
{
public:
    virtual QList<CppQuickFixOperation::Ptr> match(const CppQuickFixState &state)
    {
        const QList<AST *> &path = state.path();
        QSharedPointer<Operation> op(new Operation(state));

        int index = path.size() - 1;
        for (; index != -1; --index) {
            if (WhileStatementAST *statement = path.at(index)->asWhileStatement()) {
                if (statement->match(op->pattern, &op->matcher) && op->condition->declarator) {
                    DeclaratorAST *declarator = op->condition->declarator;
                    op->core = declarator->core_declarator;

                    if (! op->core)
                        return noResult();

                    else if (! declarator->equal_token)
                        return noResult();

                    else if (! declarator->initializer)
                        return noResult();

                    if (state.isCursorOn(op->core)) {
                        QList<CppQuickFixOperation::Ptr> result;
                        op->setPriority(index);
                        result.append(op);
                        return result;
                    }
                }
            }
        }

        return noResult();
    }

private:
    class Operation: public CppQuickFixOperation
    {
    public:
        Operation(const CppQuickFixState &state)
            : CppQuickFixOperation(state)
        {
            setDescription(QApplication::translate("CppTools::QuickFix",
                                                   "Move Declaration out of Condition"));

            condition = mk.Condition();
            pattern = mk.WhileStatement(condition);
        }

        virtual void performChanges(CppRefactoringFile *currentFile, CppRefactoringChanges *)
        {
            ChangeSet changes;

            changes.insert(currentFile->startOf(condition), QLatin1String("("));
            changes.insert(currentFile->endOf(condition), QLatin1String(") != 0"));

            int insertPos = currentFile->startOf(pattern);
            const int conditionStart = currentFile->startOf(condition);
            changes.move(conditionStart, currentFile->startOf(core), insertPos);
            changes.copy(currentFile->range(core), insertPos);
            changes.insert(insertPos, QLatin1String(";\n"));

            currentFile->change(changes);
            currentFile->indent(currentFile->range(pattern));
        }

        ASTMatcher matcher;
        ASTPatternBuilder mk;
        CPPEditorWidget *editor;
        ConditionAST *condition;
        WhileStatementAST *pattern;
        CoreDeclaratorAST *core;
    };
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

    Activates on: && or ||
*/
class SplitIfStatementOp: public CppQuickFixFactory
{
public:
    virtual QList<CppQuickFixOperation::Ptr> match(const CppQuickFixState &state)
    {
        IfStatementAST *pattern = 0;
        const QList<AST *> &path = state.path();

        int index = path.size() - 1;
        for (; index != -1; --index) {
            AST *node = path.at(index);
            if (IfStatementAST *stmt = node->asIfStatement()) {
                pattern = stmt;
                break;
            }
        }

        if (! pattern || ! pattern->statement)
            return noResult();

        unsigned splitKind = 0;
        for (++index; index < path.size(); ++index) {
            AST *node = path.at(index);
            BinaryExpressionAST *condition = node->asBinaryExpression();
            if (! condition)
                return noResult();

            Token binaryToken = state.currentFile().tokenAt(condition->binary_op_token);

            // only accept a chain of ||s or &&s - no mixing
            if (! splitKind) {
                splitKind = binaryToken.kind();
                if (splitKind != T_AMPER_AMPER && splitKind != T_PIPE_PIPE)
                    return noResult();
                // we can't reliably split &&s in ifs with an else branch
                if (splitKind == T_AMPER_AMPER && pattern->else_statement)
                    return noResult();
            } else if (splitKind != binaryToken.kind()) {
                return noResult();
            }

            if (state.isCursorOn(condition->binary_op_token))
                return singleResult(new Operation(state, index, pattern, condition));
        }

        return noResult();
    }

private:
    class Operation: public CppQuickFixOperation
    {
    public:
        Operation(const CppQuickFixState &state, int priority,
                  IfStatementAST *pattern, BinaryExpressionAST *condition)
            : CppQuickFixOperation(state, priority)
            , pattern(pattern)
            , condition(condition)
        {
            setDescription(QApplication::translate("CppTools::QuickFix",
                                                   "Split if Statement"));
        }

        virtual void performChanges(CppRefactoringFile *currentFile, CppRefactoringChanges *)
        {
            const Token binaryToken = currentFile->tokenAt(condition->binary_op_token);

            if (binaryToken.is(T_AMPER_AMPER))
                splitAndCondition(currentFile);
            else
                splitOrCondition(currentFile);
        }

        void splitAndCondition(CppRefactoringFile *currentFile)
        {
            ChangeSet changes;

            int startPos = currentFile->startOf(pattern);
            changes.insert(startPos, QLatin1String("if ("));
            changes.move(currentFile->range(condition->left_expression), startPos);
            changes.insert(startPos, QLatin1String(") {\n"));

            const int lExprEnd = currentFile->endOf(condition->left_expression);
            changes.remove(lExprEnd, currentFile->startOf(condition->right_expression));
            changes.insert(currentFile->endOf(pattern), QLatin1String("\n}"));

            currentFile->change(changes);
            currentFile->indent(currentFile->range(pattern));
        }

        void splitOrCondition(CppRefactoringFile *currentFile)
        {
            ChangeSet changes;

            StatementAST *ifTrueStatement = pattern->statement;
            CompoundStatementAST *compoundStatement = ifTrueStatement->asCompoundStatement();

            int insertPos = currentFile->endOf(ifTrueStatement);
            if (compoundStatement)
                changes.insert(insertPos, QLatin1String(" "));
            else
                changes.insert(insertPos, QLatin1String("\n"));
            changes.insert(insertPos, QLatin1String("else if ("));

            const int rExprStart = currentFile->startOf(condition->right_expression);
            changes.move(rExprStart, currentFile->startOf(pattern->rparen_token), insertPos);
            changes.insert(insertPos, QLatin1String(")"));

            const int rParenEnd = currentFile->endOf(pattern->rparen_token);
            changes.copy(rParenEnd, currentFile->endOf(pattern->statement), insertPos);

            const int lExprEnd = currentFile->endOf(condition->left_expression);
            changes.remove(lExprEnd, currentFile->startOf(condition->right_expression));

            currentFile->change(changes);
            currentFile->indent(currentFile->range(pattern));
        }

    private:
        IfStatementAST *pattern;
        BinaryExpressionAST *condition;
    };
};

/*
  Replace
    "abcd" -> QLatin1String("abcd")
    @"abcd" -> QLatin1String("abcd")
    'a' -> QLatin1Char('a')
  Except if they are already enclosed in
    QLatin1Char, QT_TRANSLATE_NOOP, tr,
    trUtf8, QLatin1Literal, QLatin1String

    Activates on: the string literal
*/
class WrapStringLiteral: public CppQuickFixFactory
{
public:
    enum Type { TypeString, TypeObjCString, TypeChar, TypeNone };

    virtual QList<CppQuickFixOperation::Ptr> match(const CppQuickFixState &state)
    {
        ExpressionAST *literal = 0;
        Type type = TypeNone;
        const QList<AST *> &path = state.path();
        const CppRefactoringFile &file = state.currentFile();

        if (path.isEmpty())
            return noResult(); // nothing to do

        literal = path.last()->asStringLiteral();

        if (! literal) {
            literal = path.last()->asNumericLiteral();
            if (!literal || !file.tokenAt(literal->asNumericLiteral()->literal_token).is(T_CHAR_LITERAL))
                return noResult();
            else
                type = TypeChar;
        } else {
            type = TypeString;
        }

        if (path.size() > 1) {
            if (CallAST *call = path.at(path.size() - 2)->asCall()) {
                if (call->base_expression) {
                    if (IdExpressionAST *idExpr = call->base_expression->asIdExpression()) {
                        if (SimpleNameAST *functionName = idExpr->name->asSimpleName()) {
                            const QByteArray id(file.tokenAt(functionName->identifier_token).identifier->chars());

                            if (id == "QT_TRANSLATE_NOOP" || id == "tr" || id == "trUtf8"
                                    || (type == TypeString && (id == "QLatin1String" || id == "QLatin1Literal"))
                                    || (type == TypeChar && id == "QLatin1Char"))
                                return noResult(); // skip it
                        }
                    }
                }
            }
        }

        if (type == TypeString) {
            if (file.charAt(file.startOf(literal)) == QLatin1Char('@'))
                type = TypeObjCString;
        }
        return singleResult(new Operation(state,
                                          path.size() - 1, // very high priority
                                          type,
                                          literal));
    }

private:
    class Operation: public CppQuickFixOperation
    {
    public:
        Operation(const CppQuickFixState &state, int priority, Type type,
                  ExpressionAST *literal)
            : CppQuickFixOperation(state, priority)
            , type(type)
            , literal(literal)
        {
            if (type == TypeChar)
                setDescription(QApplication::translate("CppTools::QuickFix",
                                                       "Enclose in QLatin1Char(...)"));
            else
                setDescription(QApplication::translate("CppTools::QuickFix",
                                                       "Enclose in QLatin1String(...)"));
        }

        virtual void performChanges(CppRefactoringFile *currentFile, CppRefactoringChanges *)
        {
            ChangeSet changes;

            const int startPos = currentFile->startOf(literal);
            QLatin1String replacement = (type == TypeChar ? QLatin1String("QLatin1Char(")
                                                          : QLatin1String("QLatin1String("));

            if (type == TypeObjCString)
                changes.replace(startPos, startPos + 1, replacement);
            else
                changes.insert(startPos, replacement);

            changes.insert(currentFile->endOf(literal), ")");

            currentFile->change(changes);
        }

    private:
        Type type;
        ExpressionAST *literal;
    };
};

/*
  Replace
    "abcd"
  With
    tr("abcd") or
    QCoreApplication::translate("CONTEXT", "abcd") or
    QT_TRANSLATE_NOOP("GLOBAL", "abcd")
  depending on what is available.

    Activates on: the string literal
*/
class TranslateStringLiteral: public CppQuickFixFactory
{
public:
    enum TranslationOption { unknown, useTr, useQCoreApplicationTranslate, useMacro };

    virtual QList<CppQuickFixOperation::Ptr> match(const CppQuickFixState &state)
    {
        const QList<AST *> &path = state.path();
        // Initialize
        ExpressionAST *literal = 0;
        QString trContext;

        if (path.isEmpty())
            return noResult();

        literal = path.last()->asStringLiteral();
        if (!literal)
            return noResult(); // No string, nothing to do

        // Do we already have a translation markup?
        if (path.size() >= 2) {
            if (CallAST *call = path.at(path.size() - 2)->asCall()) {
                if (call->base_expression) {
                    if (IdExpressionAST *idExpr = call->base_expression->asIdExpression()) {
                        if (SimpleNameAST *functionName = idExpr->name->asSimpleName()) {
                            const QByteArray id(state.currentFile().tokenAt(functionName->identifier_token).identifier->chars());

                            if (id == "tr" || id == "trUtf8"
                                    || id == "translate"
                                    || id == "QT_TRANSLATE_NOOP"
                                    || id == "QLatin1String" || id == "QLatin1Literal")
                                return noResult(); // skip it
                        }
                    }
                }
            }
        }

        QSharedPointer<Control> control = state.context().control();
        const Name *trName = control->identifier("tr");

        // Check whether we are in a method:
        for (int i = path.size() - 1; i >= 0; --i)
        {
            if (FunctionDefinitionAST *definition = path.at(i)->asFunctionDefinition()) {
                Function *function = definition->symbol;
                ClassOrNamespace *b = state.context().lookupType(function);
                if (b) {
                    // Do we have a tr method?
                    foreach(const LookupItem &r, b->find(trName)) {
                        Symbol *s = r.declaration();
                        if (s->type()->isFunctionType()) {
                            // no context required for tr
                            return singleResult(new Operation(state, path.size() - 1, literal, useTr, trContext));
                        }
                    }
                }
                // We need to do a QCA::translate, so we need a context.
                // Use fully qualified class name:
                Overview oo;
                foreach (const Name *n, LookupContext::path(function)) {
                    if (!trContext.isEmpty())
                        trContext.append(QLatin1String("::"));
                    trContext.append(oo.prettyName(n));
                }
                // ... or global if none available!
                if (trContext.isEmpty())
                    trContext = QLatin1String("GLOBAL");
                return singleResult(new Operation(state, path.size() - 1, literal, useQCoreApplicationTranslate, trContext));
            }
        }

        // We need to use Q_TRANSLATE_NOOP
        return singleResult(new Operation(state, path.size() - 1, literal, useMacro, QLatin1String("GLOBAL")));
    }

private:
    class Operation: public CppQuickFixOperation
    {
    public:
        Operation(const CppQuickFixState &state, int priority, ExpressionAST *literal, TranslationOption option, const QString &context)
            : CppQuickFixOperation(state, priority)
            , m_literal(literal)
            , m_option(option)
            , m_context(context)
        {
            setDescription(QApplication::translate("CppTools::QuickFix", "Mark as Translatable"));
        }

        virtual void performChanges(CppRefactoringFile *currentFile, CppRefactoringChanges *)
        {
            ChangeSet changes;

            const int startPos = currentFile->startOf(m_literal);
            QString replacement(QLatin1String("tr("));
            if (m_option == useQCoreApplicationTranslate) {
                replacement = QLatin1String("QCoreApplication::translate(\"")
                        + m_context + QLatin1String("\", ");
            } else if (m_option == useMacro) {
                replacement = QLatin1String("QT_TRANSLATE_NOOP(\"")
                        + m_context + QLatin1String("\", ");
            }

            changes.insert(startPos, replacement);
            changes.insert(currentFile->endOf(m_literal), QLatin1String(")"));

            currentFile->change(changes);
        }

    private:
        ExpressionAST *m_literal;
        TranslationOption m_option;
        QString m_context;
    };
};

/**
 * Replace
 *    "abcd"
 *    QLatin1String("abcd")
 *    QLatin1Literal("abcd")
 * With
 *    @"abcd"
 *
 * Activates on: the string literal, if the file type is a Objective-C(++) file.
 */
class CStringToNSString: public CppQuickFixFactory
{
public:
    virtual QList<CppQuickFixOperation::Ptr> match(const CppQuickFixState &state)
    {
        const CppRefactoringFile &file = state.currentFile();

        if (state.editor()->mimeType() != CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE)
            return noResult();

        StringLiteralAST *stringLiteral = 0;
        CallAST *qlatin1Call = 0;
        const QList<AST *> &path = state.path();

        if (path.isEmpty())
            return noResult(); // nothing to do

        stringLiteral = path.last()->asStringLiteral();

        if (! stringLiteral)
            return noResult();

        else if (file.charAt(file.startOf(stringLiteral)) == QLatin1Char('@'))
            return noResult(); // it's already an objc string literal.

        else if (path.size() > 1) {
            if (CallAST *call = path.at(path.size() - 2)->asCall()) {
                if (call->base_expression) {
                    if (IdExpressionAST *idExpr = call->base_expression->asIdExpression()) {
                        if (SimpleNameAST *functionName = idExpr->name->asSimpleName()) {
                            const QByteArray id(state.currentFile().tokenAt(functionName->identifier_token).identifier->chars());

                            if (id == "QLatin1String" || id == "QLatin1Literal")
                                qlatin1Call = call;
                        }
                    }
                }
            }
        }

        return singleResult(new Operation(state, path.size() - 1, stringLiteral, qlatin1Call));
    }

private:
    class Operation: public CppQuickFixOperation
    {
    public:
        Operation(const CppQuickFixState &state, int priority, StringLiteralAST *stringLiteral, CallAST *qlatin1Call)
            : CppQuickFixOperation(state, priority)
            , stringLiteral(stringLiteral)
            , qlatin1Call(qlatin1Call)
        {
            setDescription(QApplication::translate("CppTools::QuickFix",
                                                   "Convert to Objective-C String Literal"));
        }

        virtual void performChanges(CppRefactoringFile *currentFile, CppRefactoringChanges *)
        {
            ChangeSet changes;

            if (qlatin1Call) {
                changes.replace(currentFile->startOf(qlatin1Call), currentFile->startOf(stringLiteral), QLatin1String("@"));
                changes.remove(currentFile->endOf(stringLiteral), currentFile->endOf(qlatin1Call));
            } else {
                changes.insert(currentFile->startOf(stringLiteral), "@");
            }

            currentFile->change(changes);
        }

    private:
        StringLiteralAST *stringLiteral;
        CallAST *qlatin1Call;
    };
};

/*
  Base class for converting numeric literals between decimal, octal and hex.
  Does the base check for the specific ones and parses the number.
  Test cases:
    0xFA0Bu;
    0X856A;
    298.3;
    199;
    074;
    199L;
    074L;
    -199;
    -017;
    0783; // invalid octal
    0; // border case, allow only hex<->decimal

    Activates on: numeric literals
*/
class ConvertNumericLiteral: public CppQuickFixFactory
{
public:
    virtual QList<QuickFixOperation::Ptr> match(const CppQuickFixState &state)
    {
        QList<QuickFixOperation::Ptr> result;

        const QList<AST *> &path = state.path();
        const CppRefactoringFile &file = state.currentFile();

        if (path.isEmpty())
            return result; // nothing to do

        NumericLiteralAST *literal = path.last()->asNumericLiteral();

        if (! literal)
            return result;

        Token token = file.tokenAt(literal->asNumericLiteral()->literal_token);
        if (!token.is(T_NUMERIC_LITERAL))
            return result;
        const NumericLiteral *numeric = token.number;
        if (numeric->isDouble() || numeric->isFloat())
            return result;

        // remove trailing L or U and stuff
        const char * const spell = numeric->chars();
        int numberLength = numeric->size();
        while (numberLength > 0 && (spell[numberLength-1] < '0' || spell[numberLength-1] > 'F'))
            --numberLength;
        if (numberLength < 1)
            return result;

        // convert to number
        bool valid;
        ulong value = QString::fromUtf8(spell).left(numberLength).toULong(&valid, 0);
        if (!valid) // e.g. octal with digit > 7
            return result;

        const int priority = path.size() - 1; // very high priority
        const int start = file.startOf(literal);
        const char * const str = numeric->chars();

        if (!numeric->isHex()) {
            /*
              Convert integer literal to hex representation.
              Replace
                32
                040
              With
                0x20

            */
            QString replacement;
            replacement.sprintf("0x%lX", value);
            QuickFixOperation::Ptr op(new ConvertNumeric(state, start, start + numberLength, replacement));
            op->setDescription(QApplication::translate("CppTools::QuickFix", "Convert to Hexadecimal"));
            op->setPriority(priority);
            result.append(op);
        }

        if (value != 0) {
            if (!(numberLength > 1 && str[0] == '0' && str[1] != 'x' && str[1] != 'X')) {
                /*
                  Convert integer literal to octal representation.
                  Replace
                    32
                    0x20
                  With
                    040
                */
                QString replacement;
                replacement.sprintf("0%lo", value);
                QuickFixOperation::Ptr op(new ConvertNumeric(state, start, start + numberLength, replacement));
                op->setDescription(QApplication::translate("CppTools::QuickFix", "Convert to Octal"));
                op->setPriority(priority);
                result.append(op);
            }
        }

        if (value != 0 || numeric->isHex()) {
            if (!(numberLength > 1 && str[0] != '0')) {
                /*
                  Convert integer literal to decimal representation.
                  Replace
                    0x20
                    040
                  With
                    32
                */
                QString replacement;
                replacement.sprintf("%lu", value);
                QuickFixOperation::Ptr op(new ConvertNumeric(state, start, start + numberLength, replacement));
                op->setDescription(QApplication::translate("CppTools::QuickFix", "Convert to Decimal"));
                op->setPriority(priority);
                result.append(op);
            }
        }

        return result;
    }

private:
    class ConvertNumeric: public CppQuickFixOperation
    {
    public:
        ConvertNumeric(const CppQuickFixState &state, int start, int end, const QString &replacement)
            : CppQuickFixOperation(state)
            , start(start)
            , end(end)
            , replacement(replacement)
        {}

        virtual void performChanges(CppRefactoringFile *currentFile, CppRefactoringChanges *)
        {
            ChangeSet changes;
            changes.replace(start, end, replacement);
            currentFile->change(changes);
        }

    protected:
        int start, end;
        QString replacement;
    };
};

/*
    Can be triggered on a class forward declaration to add the matching #include.

    Activates on: the name of a forward-declared class or struct
*/
class FixForwardDeclarationOp: public CppQuickFixFactory
{
public:
    virtual QList<CppQuickFixOperation::Ptr> match(const CppQuickFixState &state)
    {
        const QList<AST *> &path = state.path();

        for (int index = path.size() - 1; index != -1; --index) {
            AST *ast = path.at(index);
            if (NamedTypeSpecifierAST *namedTy = ast->asNamedTypeSpecifier()) {
                if (Symbol *fwdClass = checkName(state, namedTy->name))
                    return singleResult(new Operation(state, index, fwdClass));
            } else if (ElaboratedTypeSpecifierAST *eTy = ast->asElaboratedTypeSpecifier()) {
                if (Symbol *fwdClass = checkName(state, eTy->name))
                    return singleResult(new Operation(state, index, fwdClass));
            }
        }

        return noResult();
    }

protected:
    static Symbol *checkName(const CppQuickFixState &state, NameAST *ast)
    {
        if (ast && state.isCursorOn(ast)) {
            if (const Name *name = ast->name) {
                unsigned line, column;
                state.document()->translationUnit()->getTokenStartPosition(ast->firstToken(), &line, &column);

                Symbol *fwdClass = 0;

                foreach (const LookupItem &r, state.context().lookup(name, state.document()->scopeAt(line, column))) {
                    if (! r.declaration())
                        continue;
                    else if (ForwardClassDeclaration *fwd = r.declaration()->asForwardClassDeclaration())
                        fwdClass = fwd;
                    else if (r.declaration()->isClass())
                        return 0; // nothing to do.
                }

                return fwdClass;
            }
        }

        return 0;
    }

private:
    class Operation: public CppQuickFixOperation
    {
    public:
        Operation(const CppQuickFixState &state, int priority, Symbol *fwdClass)
            : CppQuickFixOperation(state, priority)
            , fwdClass(fwdClass)
        {
            setDescription(QApplication::translate("CppTools::QuickFix",
                                                   "#include Header File"));
        }

        virtual void performChanges(CppRefactoringFile *currentFile, CppRefactoringChanges *)
        {
            Q_ASSERT(fwdClass != 0);

            if (Class *k = state().snapshot().findMatchingClassDeclaration(fwdClass)) {
                const QString headerFile = QString::fromUtf8(k->fileName(), k->fileNameLength());

                // collect the fwd headers
                Snapshot fwdHeaders;
                fwdHeaders.insert(state().snapshot().document(headerFile));
                foreach (Document::Ptr doc, state().snapshot()) {
                    QFileInfo headerFileInfo(doc->fileName());
                    if (doc->globalSymbolCount() == 0 && doc->includes().size() == 1)
                        fwdHeaders.insert(doc);
                    else if (headerFileInfo.suffix().isEmpty())
                        fwdHeaders.insert(doc);
                }


                DependencyTable dep;
                dep.build(fwdHeaders);
                QStringList candidates = dep.dependencyTable().value(headerFile);

                const QString className = QString::fromUtf8(k->identifier()->chars());

                QString best;
                foreach (const QString &c, candidates) {
                    QFileInfo headerFileInfo(c);
                    if (headerFileInfo.fileName() == className) {
                        best = c;
                        break;
                    } else if (headerFileInfo.fileName().at(0).isUpper()) {
                        best = c;
                        // and continue
                    } else if (! best.isEmpty()) {
                        if (c.count(QLatin1Char('/')) < best.count(QLatin1Char('/')))
                            best = c;
                    }
                }

                if (best.isEmpty())
                    best = headerFile;

                int pos = currentFile->startOf(1);

                unsigned currentLine = currentFile->cursor().blockNumber() + 1;
                unsigned bestLine = 0;
                foreach (const Document::Include &incl, state().document()->includes()) {
                    if (incl.line() < currentLine)
                        bestLine = incl.line();
                }

                if (bestLine)
                    pos = currentFile->document()->findBlockByNumber(bestLine).position();

                Utils::ChangeSet changes;
                changes.insert(pos, QString("#include <%1>\n").arg(QFileInfo(best).fileName()));
                currentFile->change(changes);
            }
        }

    private:
        Symbol *fwdClass;
    };
};

/*
  Rewrites
    a = foo();
  As
    Type a = foo();
  Where Type is the return type of foo()

    Activates on: the assignee, if the type of the right-hand side of the assignment is known.
*/
class AddLocalDeclarationOp: public CppQuickFixFactory
{
public:
    virtual QList<CppQuickFixOperation::Ptr> match(const CppQuickFixState &state)
    {
        const QList<AST *> &path = state.path();
        const CppRefactoringFile &file = state.currentFile();

        for (int index = path.size() - 1; index != -1; --index) {
            if (BinaryExpressionAST *binary = path.at(index)->asBinaryExpression()) {
                if (binary->left_expression && binary->right_expression && file.tokenAt(binary->binary_op_token).is(T_EQUAL)) {
                    IdExpressionAST *idExpr = binary->left_expression->asIdExpression();
                    if (state.isCursorOn(binary->left_expression) && idExpr && idExpr->name->asSimpleName() != 0) {
                        SimpleNameAST *nameAST = idExpr->name->asSimpleName();
                        const QList<LookupItem> results = state.context().lookup(nameAST->name, file.scopeAt(nameAST->firstToken()));
                        Declaration *decl = 0;
                        foreach (const LookupItem &r, results) {
                            if (! r.declaration())
                                continue;
                            else if (Declaration *d = r.declaration()->asDeclaration()) {
                                if (! d->type()->isFunctionType()) {
                                    decl = d;
                                    break;
                                }
                            }
                        }

                        if (! decl) {
                            return singleResult(new Operation(state, index, binary));
                        }
                    }
                }
            }
        }

        return noResult();
    }

private:
    class Operation: public CppQuickFixOperation
    {
    public:
        Operation(const CppQuickFixState &state, int priority, BinaryExpressionAST *binaryAST)
            : CppQuickFixOperation(state, priority)
            , binaryAST(binaryAST)
        {
            setDescription(QApplication::translate("CppTools::QuickFix", "Add local Declaration"));
        }

        virtual void performChanges(CppRefactoringFile *currentFile, CppRefactoringChanges *)
        {
            TypeOfExpression typeOfExpression;
            typeOfExpression.init(state().document(), state().snapshot(), state().context().bindings());
            const QList<LookupItem> result = typeOfExpression(currentFile->textOf(binaryAST->right_expression),
                                                              currentFile->scopeAt(binaryAST->firstToken()),
                                                              TypeOfExpression::Preprocess);

            if (! result.isEmpty()) {

                SubstitutionEnvironment env;
                env.setContext(state().context());
                env.switchScope(result.first().scope());
                UseQualifiedNames q;
                env.enter(&q);

                Control *control = state().context().control().data();
                FullySpecifiedType tn = rewriteType(result.first().type(), &env, control);

                Overview oo;
                QString ty = oo(tn);
                if (! ty.isEmpty()) {
                    const QChar ch = ty.at(ty.size() - 1);

                    if (ch.isLetterOrNumber() || ch == QLatin1Char(' ') || ch == QLatin1Char('>'))
                        ty += QLatin1Char(' ');

                    Utils::ChangeSet changes;
                    changes.insert(currentFile->startOf(binaryAST), ty);
                    currentFile->change(changes);
                }
            }
        }

    private:
        BinaryExpressionAST *binaryAST;
    };
};

/**
 * Turns "an_example_symbol" into "anExampleSymbol" and
 * "AN_EXAMPLE_SYMBOL" into "AnExampleSymbol".
 *
 * Activates on: identifiers
 */
class ToCamelCaseConverter : public CppQuickFixFactory
{
public:
    virtual QList<CppQuickFixOperation::Ptr> match(const CppQuickFixState &state)
    {
        const QList<AST *> &path = state.path();

        if (path.isEmpty())
            return noResult();

        AST * const ast = path.last();
        const Name *name = 0;
        if (const NameAST * const nameAst = ast->asName()) {
            if (nameAst->name && nameAst->name->asNameId())
                name = nameAst->name;
        } else if (const NamespaceAST * const namespaceAst = ast->asNamespace()) {
            name = namespaceAst->symbol->name();
        }

        if (!name)
            return noResult();

        QString newName = QString::fromUtf8(name->identifier()->chars());
        if (newName.length() < 3)
            return noResult();
        for (int i = 1; i < newName.length() - 1; ++i) {
            if (Operation::isConvertibleUnderscore(newName, i))
                return singleResult(new Operation(state, path.size() - 1, newName));
        }

        return noResult();
    }

private:
    class Operation: public CppQuickFixOperation
    {
    public:
        Operation(const CppQuickFixState &state, int priority, const QString &newName)
            : CppQuickFixOperation(state, priority)
            , m_name(newName)
        {
            setDescription(QApplication::translate("CppTools::QuickFix",
                                                   "Convert to Camel Case ..."));
        }

        virtual void performChanges(CppRefactoringFile *, CppRefactoringChanges *)
        {
            for (int i = 1; i < m_name.length(); ++i) {
                QCharRef c = m_name[i];
                if (c.isUpper()) {
                    c = c.toLower();
                } else if (i < m_name.length() - 1
                           && isConvertibleUnderscore(m_name, i)) {
                    m_name.remove(i, 1);
                    m_name[i] = m_name.at(i).toUpper();
                }
            }
            static_cast<CppEditor::Internal::CPPEditorWidget*>(state().editor())->renameUsagesNow(m_name);
        }

        static bool isConvertibleUnderscore(const QString &name, int pos)
        {
            return name.at(pos) == QLatin1Char('_') && name.at(pos+1).isLetter()
                    && !(pos == 1 && name.at(0) == QLatin1Char('m'));
        }

    private:
        QString m_name;
    };
};

} // end of anonymous namespace

void CppQuickFixCollector::registerQuickFixes(ExtensionSystem::IPlugin *plugIn)
{
    plugIn->addAutoReleasedObject(new UseInverseOp);
    plugIn->addAutoReleasedObject(new FlipBinaryOp);
    plugIn->addAutoReleasedObject(new RewriteLogicalAndOp);
    plugIn->addAutoReleasedObject(new SplitSimpleDeclarationOp);
    plugIn->addAutoReleasedObject(new AddBracesToIfOp);
    plugIn->addAutoReleasedObject(new MoveDeclarationOutOfIfOp);
    plugIn->addAutoReleasedObject(new MoveDeclarationOutOfWhileOp);
    plugIn->addAutoReleasedObject(new SplitIfStatementOp);
    plugIn->addAutoReleasedObject(new WrapStringLiteral);
    plugIn->addAutoReleasedObject(new TranslateStringLiteral);
    plugIn->addAutoReleasedObject(new CStringToNSString);
    plugIn->addAutoReleasedObject(new ConvertNumericLiteral);
    plugIn->addAutoReleasedObject(new Internal::CompleteSwitchCaseStatement);
    plugIn->addAutoReleasedObject(new FixForwardDeclarationOp);
    plugIn->addAutoReleasedObject(new AddLocalDeclarationOp);
    plugIn->addAutoReleasedObject(new ToCamelCaseConverter);
    plugIn->addAutoReleasedObject(new Internal::InsertQtPropertyMembers);
    plugIn->addAutoReleasedObject(new Internal::DeclFromDef);
    plugIn->addAutoReleasedObject(new Internal::DefFromDecl);
}
