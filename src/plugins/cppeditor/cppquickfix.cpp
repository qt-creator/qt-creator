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
#include "cppdeclfromdef.h"

#include <cplusplus/ASTPath.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/ResolveExpression.h>
#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>

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
#include <Names.h>
#include <Literals.h>

#include <cppeditor/cpprefactoringchanges.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <extensionsystem/pluginmanager.h>

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
    Snapshot snapshot;
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
            changes.remove(range(negation->unary_op_token));
        } else if (nested) {
            changes.insert(startOf(nested), "!");
        } else {
            changes.insert(startOf(binary), "!(");
            changes.insert(endOf(binary), ")");
        }
        changes.replace(range(binary->binary_op_token), replacement);
        refactoringChanges()->changeFile(fileName(), changes);
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

        changes.flip(range(binary->left_expression), range(binary->right_expression));
        if (! replacement.isEmpty())
            changes.replace(range(binary->binary_op_token), replacement);

        refactoringChanges()->changeFile(fileName(), changes);
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
        changes.replace(range(pattern->binary_op_token), QLatin1String("||"));
        changes.remove(range(left->unary_op_token));
        changes.remove(range(right->unary_op_token));
        const int start = startOf(pattern);
        const int end = endOf(pattern);
        changes.insert(start, QLatin1String("!("));
        changes.insert(end, QLatin1String(")"));

        refactoringChanges()->changeFile(fileName(), changes);
        refactoringChanges()->reindent(fileName(), range(pattern));
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
            changes.copy(declSpecifiersStart, declSpecifiersEnd, insertPos);
            changes.insert(insertPos, QLatin1String(" "));
            changes.move(range(declarator), insertPos);
            changes.insert(insertPos, QLatin1String(";"));

            const int prevDeclEnd = endOf(prevDeclarator);
            changes.remove(prevDeclEnd, startOf(declarator));

            prevDeclarator = declarator;
        }

        refactoringChanges()->changeFile(fileName(), changes);
        refactoringChanges()->reindent(fileName(), range(declaration));
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
        if (ifStatement && isCursorOn(ifStatement->if_token) && ifStatement->statement
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

        refactoringChanges()->changeFile(fileName(), changes);
        refactoringChanges()->reindent(fileName(), range(start, end));
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

        changes.copy(range(core), startOf(condition));

        int insertPos = startOf(pattern);
        changes.move(range(condition), insertPos);
        changes.insert(insertPos, QLatin1String(";\n"));

        refactoringChanges()->changeFile(fileName(), changes);
        refactoringChanges()->reindent(fileName(), range(pattern));
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
        changes.move(conditionStart, startOf(core), insertPos);
        changes.copy(range(core), insertPos);
        changes.insert(insertPos, QLatin1String(";\n"));

        refactoringChanges()->changeFile(fileName(), changes);
        refactoringChanges()->reindent(fileName(), range(pattern));
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
        changes.move(range(condition->left_expression), startPos);
        changes.insert(startPos, QLatin1String(") {\n"));

        const int lExprEnd = endOf(condition->left_expression);
        changes.remove(lExprEnd, startOf(condition->right_expression));
        changes.insert(endOf(pattern), QLatin1String("\n}"));

        refactoringChanges()->changeFile(fileName(), changes);
        refactoringChanges()->reindent(fileName(), range(pattern));
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
        changes.move(rExprStart, startOf(pattern->rparen_token), insertPos);
        changes.insert(insertPos, QLatin1String(")"));

        const int rParenEnd = endOf(pattern->rparen_token);
        changes.copy(rParenEnd, endOf(pattern->statement), insertPos);

        const int lExprEnd = endOf(condition->left_expression);
        changes.remove(lExprEnd, startOf(condition->right_expression));

        refactoringChanges()->changeFile(fileName(), changes);
        refactoringChanges()->reindent(fileName(), range(pattern));
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
        : CppQuickFixOperation(editor), literal(0), type(TypeNone)
    {}

    enum Type { TypeString, TypeObjCString, TypeChar, TypeNone };

    virtual QString description() const
    {
        if (type == TypeChar)
            return QApplication::translate("CppTools::QuickFix", "Enclose in QLatin1Char(...)");
        return QApplication::translate("CppTools::QuickFix", "Enclose in QLatin1String(...)");
    }

    virtual int match(const QList<AST *> &path)
    {
        literal = 0;
        type = TypeNone;

        if (path.isEmpty())
            return -1; // nothing to do

        literal = path.last()->asStringLiteral();

        if (! literal) {
            literal = path.last()->asNumericLiteral();
            if (!literal || !tokenAt(literal->asNumericLiteral()->literal_token).is(T_CHAR_LITERAL))
                return -1;
            else
                type = TypeChar;
        } else {
            type = TypeString;
        }

        if (path.size() > 1) {
            if (CallAST *call = path.at(path.size() - 2)->asCall()) {
                if (call->base_expression) {
                    if (SimpleNameAST *functionName = call->base_expression->asSimpleName()) {
                        const QByteArray id(tokenAt(functionName->identifier_token).identifier->chars());

                        if (id == "QT_TRANSLATE_NOOP" || id == "tr" || id == "trUtf8"
                                || (type == TypeString && (id == "QLatin1String" || id == "QLatin1Literal"))
                                || (type == TypeChar && id == "QLatin1Char"))
                            return -1; // skip it
                    }
                }
            }
        }

        if (type == TypeString) {
            if (charAt(startOf(literal)) == QLatin1Char('@'))
                type = TypeObjCString;
        }
        return path.size() - 1; // very high priority
    }

    virtual void createChanges()
    {
        ChangeSet changes;

        const int startPos = startOf(literal);
        QLatin1String replacement = (type == TypeChar ? QLatin1String("QLatin1Char(")
            : QLatin1String("QLatin1String("));

        if (type == TypeObjCString)
            changes.replace(startPos, startPos + 1, replacement);
        else
            changes.insert(startPos, replacement);

        changes.insert(endOf(literal), ")");

        refactoringChanges()->changeFile(fileName(), changes);
    }

private:
    ExpressionAST *literal;
    Type type;
};

/*
  Replace
    "abcd"
  With
    tr("abcd") or
    QCoreApplication::translate("CONTEXT", "abcd") or
    QT_TRANSLATE_NOOP("GLOBAL", "abcd")
*/
class TranslateStringLiteral: public CppQuickFixOperation
{
public:
    TranslateStringLiteral(TextEditor::BaseTextEditor *editor)
        : CppQuickFixOperation(editor), m_literal(0)
    { }

    enum TranslationOption { unknown, useTr, useQCoreApplicationTranslate, useMacro };

    virtual QString description() const
    {
        return QApplication::translate("CppTools::QuickFix", "Mark as translateable");
    }

    virtual int match(const QList<AST *> &path)
    {
        // Initialize
        m_literal = 0;
        m_option = unknown;
        m_context.clear();

        if (path.isEmpty())
            return -1;

        m_literal = path.last()->asStringLiteral();
        if (!m_literal)
            return -1; // No string, nothing to do

        // Do we already have a translation markup?
        if (path.size() >= 2) {
            if (CallAST *call = path.at(path.size() - 2)->asCall()) {
                if (call->base_expression) {
                    if (SimpleNameAST *functionName = call->base_expression->asSimpleName()) {
                        const QByteArray id(tokenAt(functionName->identifier_token).identifier->chars());

                        if (id == "tr" || id == "trUtf8"
                                || id == "translate"
                                || id == "QT_TRANSLATE_NOOP"
                                || id == "QLatin1String" || id == "QLatin1Literal")
                            return -1; // skip it
                    }
                }
            }
        }

        LookupContext context(document(), snapshot());
        QSharedPointer<Control> control = context.control();
        const Name *trName = control->nameId(control->findOrInsertIdentifier("tr"));

        // Check whether we are in a method:
        for (int i = path.size() - 1; i >= 0; --i)
        {
            if (FunctionDefinitionAST *definition = path.at(i)->asFunctionDefinition()) {
                Function *function = definition->symbol;
                ClassOrNamespace *b = context.lookupType(function);
                if (b) {
                    // Do we have a tr method?
                    foreach(Symbol *s, b->find(trName)) {
                        if (s->type()->isFunctionType()) {
                            m_option = useTr;
                            // no context required for tr
                            return path.size() - 1;
                        }
                    }
                }
                // We need to do a QCA::translate, so we need a context.
                // Use fully qualified class name:
                Overview oo;
                foreach (const Name *n, LookupContext::fullyQualifiedName(function)) {
                    if (!m_context.isEmpty())
                        m_context.append(QLatin1String("::"));
                    m_context.append(oo.prettyName(n));
                }
                // ... or global if none available!
                if (m_context.isEmpty())
                    m_context = QLatin1String("GLOBAL");
                m_option = useQCoreApplicationTranslate;
                return path.size() - 1;
            }
        }

        // We need to use Q_TRANSLATE_NOOP
        m_context = QLatin1String("GLOBAL");
        m_option = useMacro;
        return path.size() - 1;
    }

    virtual void createChanges()
    {
        ChangeSet changes;

        const int startPos = startOf(m_literal);
        QString replacement(QLatin1String("tr("));
        if (m_option == useQCoreApplicationTranslate) {
            replacement = QLatin1String("QCoreApplication::translate(\"")
                          + m_context + QLatin1String("\", ");
        } else if (m_option == useMacro) {
            replacement = QLatin1String("QT_TRANSLATE_NOOP(\"")
                    + m_context + QLatin1String("\", ");
        }

        changes.insert(startPos, replacement);
        changes.insert(endOf(m_literal), QLatin1String(")"));

        refactoringChanges()->changeFile(fileName(), changes);
    }

private:
    ExpressionAST *m_literal;
    TranslationOption m_option;
    QString m_context;
};

class CStringToNSString: public CppQuickFixOperation
{
public:
    CStringToNSString(TextEditor::BaseTextEditor *editor)
        : CppQuickFixOperation(editor)
        , stringLiteral(0)
        , qlatin1Call(0)
    {}

    virtual QString description() const
    {
        return QApplication::translate("CppTools::QuickFix", "Convert to Objective-C String Literal");
    }

    virtual int match(const QList<AST *> &path)
    {
        stringLiteral = 0;
        qlatin1Call = 0;

        if (path.isEmpty())
            return -1; // nothing to do

        stringLiteral = path.last()->asStringLiteral();

        if (! stringLiteral)
            return -1;

        else if (charAt(startOf(stringLiteral)) == QLatin1Char('@'))
            return -1; // it's already an objc string literal.

        else if (path.size() > 1) {
            if (CallAST *call = path.at(path.size() - 2)->asCall()) {
                if (call->base_expression) {
                    if (SimpleNameAST *functionName = call->base_expression->asSimpleName()) {
                        const QByteArray id(tokenAt(functionName->identifier_token).identifier->chars());

                        if (id == "QLatin1String" || id == "QLatin1Literal")
                            qlatin1Call = call;
                    }
                }
            }
        }

        return path.size() - 1; // very high priority
    }

    virtual void createChanges()
    {
        ChangeSet changes;

        if (qlatin1Call) {
            changes.replace(startOf(qlatin1Call), startOf(stringLiteral), QLatin1String("@"));
            changes.remove(endOf(stringLiteral), endOf(qlatin1Call));
        } else {
            changes.insert(startOf(stringLiteral), "@");
        }

        refactoringChanges()->changeFile(fileName(), changes);
    }

private:
    StringLiteralAST *stringLiteral;
    CallAST *qlatin1Call;
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
*/
class ConvertNumericLiteral: public CppQuickFixOperation
{
public:
    ConvertNumericLiteral(TextEditor::BaseTextEditor *editor)
        : CppQuickFixOperation(editor)
    {}

    virtual int match(const QList<AST *> &path)
    {
        literal = 0;

        if (path.isEmpty())
            return -1; // nothing to do

        literal = path.last()->asNumericLiteral();

        if (! literal)
            return -1;

        Token token = tokenAt(literal->asNumericLiteral()->literal_token);
        if (!token.is(T_NUMERIC_LITERAL))
            return -1;
        numeric = token.number;
        if (numeric->isDouble() || numeric->isFloat())
            return -1;

        // remove trailing L or U and stuff
        const char * const spell = numeric->chars();
        numberLength = numeric->size();
        while (numberLength > 0 && (spell[numberLength-1] < '0' || spell[numberLength-1] > 'F'))
            --numberLength;
        if (numberLength < 1)
            return -1;

        // convert to number
        bool valid;
        value = QString::fromUtf8(spell).left(numberLength).toULong(&valid, 0);
        if (!valid) // e.g. octal with digit > 7
            return -1;

        return path.size() - 1; // very high priority
    }

    virtual void createChanges()
    {
        ChangeSet changes;
        int start = startOf(literal);
        changes.replace(start, start + numberLength, replacement);
        refactoringChanges()->changeFile(fileName(), changes);
    }

protected:
    NumericLiteralAST *literal;
    const NumericLiteral *numeric;
    ulong value;
    int numberLength;
    QString replacement;
};

/*
  Convert integer literal to hex representation.
  Replace
    32
    040
  With
    0x20

*/
class ConvertNumericToHex: public ConvertNumericLiteral
{
public:
    ConvertNumericToHex(TextEditor::BaseTextEditor *editor)
        : ConvertNumericLiteral(editor)
    {}

    virtual QString description() const
    {
        return QApplication::translate("CppTools::QuickFix", "Convert to Hexadecimal");
    }

    virtual int match(const QList<AST *> &path)
    {
        int ret = ConvertNumericLiteral::match(path);
        if (ret != -1 && !numeric->isHex()) {
            replacement.sprintf("0x%lX", value);
            return ret;
        }
        return -1;
    }

};

/*
  Convert integer literal to octal representation.
  Replace
    32
    0x20
  With
    040
*/
class ConvertNumericToOctal: public ConvertNumericLiteral
{
public:
    ConvertNumericToOctal(TextEditor::BaseTextEditor *editor)
        : ConvertNumericLiteral(editor)
    {}

    virtual QString description() const
    {
        return QApplication::translate("CppTools::QuickFix", "Convert to Octal");
    }

    virtual int match(const QList<AST *> &path)
    {
        int ret = ConvertNumericLiteral::match(path);
        if (ret != -1 && value != 0) {
            const char * const str = numeric->chars();
            if (numberLength > 1 && str[0] == '0' && str[1] != 'x' && str[1] != 'X')
                return -1;
            replacement.sprintf("0%lo", value);
            return ret;
        }
        return -1;
    }

};

/*
  Convert integer literal to decimal representation.
  Replace
    0x20
    040
  With
    32
*/
class ConvertNumericToDecimal: public ConvertNumericLiteral
{
public:
    ConvertNumericToDecimal(TextEditor::BaseTextEditor *editor)
        : ConvertNumericLiteral(editor)
    {}

    virtual QString description() const
    {
        return QApplication::translate("CppTools::QuickFix", "Convert to Decimal");
    }

    virtual int match(const QList<AST *> &path)
    {
        int ret = ConvertNumericLiteral::match(path);
        if (ret != -1 && (value != 0 || numeric->isHex())) {
            const char * const str = numeric->chars();
            if (numberLength > 1 && str[0] != '0')
                return -1;
            replacement.sprintf("%lu", value);
            return ret;
        }
        return -1;
    }

};

/*
  Adds missing case statements for "switch (enumVariable)"
*/
class CompleteSwitchCaseStatement: public CppQuickFixOperation
{
public:
    CompleteSwitchCaseStatement(TextEditor::BaseTextEditor *editor)
        : CppQuickFixOperation(editor)
    {}

    virtual QString description() const
    {
        return QApplication::translate("CppTools::QuickFix", "Complete Switch Statement");
    }

    virtual int match(const QList<AST *> &path)
    {
        if (path.isEmpty())
            return -1; // nothing to do

        // look for switch statement
        for (int depth = path.size()-1; depth >= 0; --depth) {
            AST *ast = path.at(depth);
            SwitchStatementAST *switchStatement = ast->asSwitchStatement();
            if (switchStatement) {
                if (!isCursorOn(switchStatement->switch_token) || !switchStatement->statement)
                    return -1;
                compoundStatement = switchStatement->statement->asCompoundStatement();
                if (!compoundStatement) // we ignore pathologic case "switch (t) case A: ;"
                    return -1;
                // look if the condition's type is an enum
                if (Enum *e = conditionEnum(switchStatement)) {
                    // check the possible enum values
                    values.clear();
                    Overview prettyPrint;
                    for (unsigned i = 0; i < e->memberCount(); ++i) {
                        if (Declaration *decl = e->memberAt(i)->asDeclaration()) {
                            values << prettyPrint(decl->name());
                        }
                    }
                    // Get the used values
                    CaseStatementCollector caseValues(document()->translationUnit());
                    QStringList usedValues = caseValues(switchStatement);
                    // save the values that would be added
                    foreach (const QString &usedValue, usedValues)
                        values.removeAll(usedValue);
                    if (values.isEmpty())
                        return -1;
                    return depth;
                }
                return -1;
            }
        }

        return -1;
    }

    virtual void createChanges()
    {
        ChangeSet changes;
        int start = endOf(compoundStatement->lbrace_token);
        changes.insert(start, QLatin1String("\ncase ")
                       + values.join(QLatin1String(":\nbreak;\ncase "))
                       + QLatin1String(":\nbreak;"));
        refactoringChanges()->changeFile(fileName(), changes);
        refactoringChanges()->reindent(fileName(), range(compoundStatement));
    }

protected:
    Enum *conditionEnum(SwitchStatementAST *statement)
    {
        Block *block = statement->symbol;
        Scope *scope = document()->scopeAt(block->line(), block->column());
        TypeOfExpression typeOfExpression;
        typeOfExpression.init(document(), snapshot());
        const QList<LookupItem> results = typeOfExpression(statement->condition,
                                                           document(),
                                                           scope);
        foreach (LookupItem result, results) {
            FullySpecifiedType fst = result.type();
            if (Enum *e = result.declaration()->type()->asEnumType())
                return e;
            if (NamedType *namedType = fst->asNamedType()) {
                QList<Symbol *> candidates =
                        typeOfExpression.context().lookup(namedType->name(), scope);
                foreach (Symbol *candidate, candidates) {
                    if (Enum *e = candidate->asEnum()) {
                        return e;
                    }
                }
            }
        }
        return 0;
    }
    class CaseStatementCollector : public ASTVisitor
    {
    public:
        CaseStatementCollector(TranslationUnit *unit) : ASTVisitor(unit) {}
        QStringList operator ()(AST *ast)
        {
            values.clear();
            foundCaseStatementLevel = false;
            accept(ast);
            return values;
        }

        bool preVisit(AST *ast) {
            if (CaseStatementAST *cs = ast->asCaseStatement()) {
                foundCaseStatementLevel = true;
                if (SimpleNameAST *sm = cs->expression->asSimpleName()) {
                    Overview prettyPrint;
                    values << prettyPrint(sm->name);
                }
                return true;
            } else if (foundCaseStatementLevel) {
                return false;
            }
            return true;
        }

        bool foundCaseStatementLevel;
        QStringList values;
    };

protected:
    CompoundStatementAST *compoundStatement;
    QStringList values;
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
    _refactoringChanges = new CppRefactoringChanges(s->snapshot);
    return match(s->path);
}

Utils::ChangeSet::Range CppQuickFixOperation::range(unsigned tokenIndex) const
{
    const CPlusPlus::Token &token = tokenAt(tokenIndex);
    unsigned line, column;
    _document->translationUnit()->getPosition(token.begin(), &line, &column);
    const int start = editor()->document()->findBlockByNumber(line - 1).position() + column - 1;
    return Utils::ChangeSet::Range(start, start + token.length());
}

Utils::ChangeSet::Range CppQuickFixOperation::range(CPlusPlus::AST *ast) const
{
    return Utils::ChangeSet::Range(startOf(ast), endOf(ast));
}

QString CppQuickFixOperation::fileName() const
{ return document()->fileName(); }

void CppQuickFixOperation::apply()
{
    refactoringChanges()->apply();
}

CppEditor::CppRefactoringChanges *CppQuickFixOperation::refactoringChanges() const
{ return _refactoringChanges; }

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
                state->snapshot = CppTools::CppModelManagerInterface::instance()->snapshot();
                return state;
            }
        }
    }

    return 0;
}

CppQuickFixFactory::CppQuickFixFactory(QObject *parent)
    : TextEditor::IQuickFixFactory(parent)
{
}

CppQuickFixFactory::~CppQuickFixFactory()
{
}

QList<TextEditor::QuickFixOperation::Ptr> CppQuickFixFactory::quickFixOperations(TextEditor::BaseTextEditor *editor)
{
    QList<TextEditor::QuickFixOperation::Ptr> quickFixOperations;

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
    QSharedPointer<TranslateStringLiteral> translateCString(new TranslateStringLiteral(editor));
    QSharedPointer<ConvertNumericToHex> convertNumericToHex(new ConvertNumericToHex(editor));
    QSharedPointer<ConvertNumericToOctal> convertNumericToOctal(new ConvertNumericToOctal(editor));
    QSharedPointer<ConvertNumericToDecimal> convertNumericToDecimal(new ConvertNumericToDecimal(editor));
    QSharedPointer<CompleteSwitchCaseStatement> completeSwitchCaseStatement(new CompleteSwitchCaseStatement(editor));
    QSharedPointer<DeclFromDef> declFromDef(new DeclFromDef(editor));

    quickFixOperations.append(rewriteLogicalAndOp);
    quickFixOperations.append(splitIfStatementOp);
    quickFixOperations.append(moveDeclarationOutOfIfOp);
    quickFixOperations.append(moveDeclarationOutOfWhileOp);
    quickFixOperations.append(splitSimpleDeclarationOp);
    quickFixOperations.append(addBracesToIfOp);
    quickFixOperations.append(useInverseOp);
    quickFixOperations.append(flipBinaryOp);
    quickFixOperations.append(wrapStringLiteral);
    quickFixOperations.append(translateCString);
    quickFixOperations.append(convertNumericToHex);
    quickFixOperations.append(convertNumericToOctal);
    quickFixOperations.append(convertNumericToDecimal);
    quickFixOperations.append(completeSwitchCaseStatement);
    quickFixOperations.append(declFromDef);

    if (editor->mimeType() == CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE)
        quickFixOperations.append(wrapCString);

    return quickFixOperations;
}
