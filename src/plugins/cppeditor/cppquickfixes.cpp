/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppquickfixes.h"

#include "cppcompleteswitch.h"
#include "cppeditor.h"
#include "cppinsertdecldef.h"
#include "cppinsertqtpropertymembers.h"
#include "cppquickfixassistant.h"

#include <cpptools/cppclassesfilter.h>
#include <cpptools/cppcodestylesettings.h>
#include <cpptools/cpppointerdeclarationformatter.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/symbolfinder.h>

#include <cplusplus/CppRewriter.h>
#include <cplusplus/DependencyTable.h>
#include <cplusplus/TypeOfExpression.h>

#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QFileInfo>
#include <QSharedPointer>
#include <QTextBlock>
#include <QTextCursor>

#include <cctype>

using namespace CppEditor;
using namespace CppEditor::Internal;

void CppEditor::Internal::registerQuickFixes(ExtensionSystem::IPlugin *plugIn)
{
    plugIn->addAutoReleasedObject(new AddIncludeForUndefinedIdentifier);
    plugIn->addAutoReleasedObject(new AddIncludeForForwardDeclaration);

    plugIn->addAutoReleasedObject(new FlipLogicalOperands);
    plugIn->addAutoReleasedObject(new InverseLogicalComparison);
    plugIn->addAutoReleasedObject(new RewriteLogicalAnd);

    plugIn->addAutoReleasedObject(new ConvertToCamelCase);

    plugIn->addAutoReleasedObject(new ConvertCStringToNSString);
    plugIn->addAutoReleasedObject(new ConvertNumericLiteral);
    plugIn->addAutoReleasedObject(new TranslateStringLiteral);
    plugIn->addAutoReleasedObject(new WrapStringLiteral);

    plugIn->addAutoReleasedObject(new MoveDeclarationOutOfIf);
    plugIn->addAutoReleasedObject(new MoveDeclarationOutOfWhile);

    plugIn->addAutoReleasedObject(new SplitIfStatement);
    plugIn->addAutoReleasedObject(new SplitSimpleDeclaration);

    plugIn->addAutoReleasedObject(new AddLocalDeclaration);
    plugIn->addAutoReleasedObject(new AddBracesToIf);
    plugIn->addAutoReleasedObject(new RearrangeParamDeclarationList);
    plugIn->addAutoReleasedObject(new ReformatPointerDeclaration);

    plugIn->addAutoReleasedObject(new CompleteSwitchCaseStatement);
    plugIn->addAutoReleasedObject(new InsertQtPropertyMembers);

    plugIn->addAutoReleasedObject(new ApplyDeclDefLinkChanges);
    plugIn->addAutoReleasedObject(new ExtractFunction);
    plugIn->addAutoReleasedObject(new GenerateGetterSetter);
    plugIn->addAutoReleasedObject(new InsertDeclFromDef);
    plugIn->addAutoReleasedObject(new InsertDefFromDecl);
}

static inline bool isQtStringLiteral(const QByteArray &id)
{
    return id == "QLatin1String" || id == "QLatin1Literal" || id == "QStringLiteral";
}

static inline bool isQtStringTranslation(const QByteArray &id)
{
    return id == "tr" || id == "trUtf8" || id == "translate" || id == "QT_TRANSLATE_NOOP";
}

class InverseLogicalComparisonOp: public CppQuickFixOperation
{
public:
    InverseLogicalComparisonOp(const CppQuickFixInterface &interface, int priority,
                               BinaryExpressionAST *binary, Kind invertToken)
        : CppQuickFixOperation(interface, priority)
        , binary(binary), nested(0), negation(0)
    {
        Token tok;
        tok.f.kind = invertToken;
        replacement = QLatin1String(tok.spell());

        // check for enclosing nested expression
        if (priority - 1 >= 0)
            nested = interface->path()[priority - 1]->asNestedExpression();

        // check for ! before parentheses
        if (nested && priority - 2 >= 0) {
            negation = interface->path()[priority - 2]->asUnaryExpression();
            if (negation && ! interface->currentFile()->tokenAt(negation->unary_op_token).is(T_EXCLAIM))
                negation = 0;
        }
    }

    QString description() const
    {
        return QApplication::translate("CppTools::QuickFix", "Rewrite Using %1").arg(replacement);
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());

        ChangeSet changes;
        if (negation) {
            // can't remove parentheses since that might break precedence
            changes.remove(currentFile->range(negation->unary_op_token));
        } else if (nested) {
            changes.insert(currentFile->startOf(nested), QLatin1String("!"));
        } else {
            changes.insert(currentFile->startOf(binary), QLatin1String("!("));
            changes.insert(currentFile->endOf(binary), QLatin1String(")"));
        }
        changes.replace(currentFile->range(binary->binary_op_token), replacement);
        currentFile->setChangeSet(changes);
        currentFile->apply();
    }

private:
    BinaryExpressionAST *binary;
    NestedExpressionAST *nested;
    UnaryExpressionAST *negation;

    QString replacement;
};

void InverseLogicalComparison::match(const CppQuickFixInterface &interface,
                                     QuickFixOperations &result)
{
    CppRefactoringFilePtr file = interface->currentFile();

    const QList<AST *> &path = interface->path();
    int index = path.size() - 1;
    BinaryExpressionAST *binary = path.at(index)->asBinaryExpression();
    if (! binary)
        return;
    if (! interface->isCursorOn(binary->binary_op_token))
        return;

    Kind invertToken;
    switch (file->tokenAt(binary->binary_op_token).kind()) {
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
        return;
    }

    result.append(CppQuickFixOperation::Ptr(
        new InverseLogicalComparisonOp(interface, index, binary, invertToken)));
}

class FlipLogicalOperandsOp: public CppQuickFixOperation
{
public:
    FlipLogicalOperandsOp(const CppQuickFixInterface &interface, int priority,
                          BinaryExpressionAST *binary, QString replacement)
        : CppQuickFixOperation(interface)
        , binary(binary)
        , replacement(replacement)
    {
        setPriority(priority);
    }

    QString description() const
    {
        if (replacement.isEmpty())
            return QApplication::translate("CppTools::QuickFix", "Swap Operands");
        else
            return QApplication::translate("CppTools::QuickFix", "Rewrite Using %1").arg(replacement);
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());

        ChangeSet changes;
        changes.flip(currentFile->range(binary->left_expression), currentFile->range(binary->right_expression));
        if (! replacement.isEmpty())
            changes.replace(currentFile->range(binary->binary_op_token), replacement);

        currentFile->setChangeSet(changes);
        currentFile->apply();
    }

private:
    BinaryExpressionAST *binary;
    QString replacement;
};

void FlipLogicalOperands::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface->path();
    CppRefactoringFilePtr file = interface->currentFile();

    int index = path.size() - 1;
    BinaryExpressionAST *binary = path.at(index)->asBinaryExpression();
    if (! binary)
        return;
    if (! interface->isCursorOn(binary->binary_op_token))
        return;

    Kind flipToken;
    switch (file->tokenAt(binary->binary_op_token).kind()) {
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
        return;
    }

    QString replacement;
    if (flipToken != T_EOF_SYMBOL) {
        Token tok;
        tok.f.kind = flipToken;
        replacement = QLatin1String(tok.spell());
    }

    result.append(QuickFixOperation::Ptr(
        new FlipLogicalOperandsOp(interface, index, binary, replacement)));
}

class RewriteLogicalAndOp: public CppQuickFixOperation
{
public:
    QSharedPointer<ASTPatternBuilder> mk;
    UnaryExpressionAST *left;
    UnaryExpressionAST *right;
    BinaryExpressionAST *pattern;

    RewriteLogicalAndOp(const CppQuickFixInterface &interface)
        : CppQuickFixOperation(interface)
        , mk(new ASTPatternBuilder)
    {
        left = mk->UnaryExpression();
        right = mk->UnaryExpression();
        pattern = mk->BinaryExpression(left, right);
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());

        ChangeSet changes;
        changes.replace(currentFile->range(pattern->binary_op_token), QLatin1String("||"));
        changes.remove(currentFile->range(left->unary_op_token));
        changes.remove(currentFile->range(right->unary_op_token));
        const int start = currentFile->startOf(pattern);
        const int end = currentFile->endOf(pattern);
        changes.insert(start, QLatin1String("!("));
        changes.insert(end, QLatin1String(")"));

        currentFile->setChangeSet(changes);
        currentFile->appendIndentRange(currentFile->range(pattern));
        currentFile->apply();
    }
};

void RewriteLogicalAnd::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    BinaryExpressionAST *expression = 0;
    const QList<AST *> &path = interface->path();
    CppRefactoringFilePtr file = interface->currentFile();

    int index = path.size() - 1;
    for (; index != -1; --index) {
        expression = path.at(index)->asBinaryExpression();
        if (expression)
            break;
    }

    if (! expression)
        return;

    if (! interface->isCursorOn(expression->binary_op_token))
        return;

    QSharedPointer<RewriteLogicalAndOp> op(new RewriteLogicalAndOp(interface));

    if (expression->match(op->pattern, &matcher) &&
            file->tokenAt(op->pattern->binary_op_token).is(T_AMPER_AMPER) &&
            file->tokenAt(op->left->unary_op_token).is(T_EXCLAIM) &&
            file->tokenAt(op->right->unary_op_token).is(T_EXCLAIM)) {
        op->setDescription(QApplication::translate("CppTools::QuickFix", "Rewrite Condition Using ||"));
        op->setPriority(index);
        result.append(op);
    }
}

bool SplitSimpleDeclaration::checkDeclaration(SimpleDeclarationAST *declaration)
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

class SplitSimpleDeclarationOp: public CppQuickFixOperation
{
public:
    SplitSimpleDeclarationOp(const CppQuickFixInterface &interface, int priority,
                             SimpleDeclarationAST *decl)
        : CppQuickFixOperation(interface, priority)
        , declaration(decl)
    {
        setDescription(QApplication::translate("CppTools::QuickFix",
                                               "Split Declaration"));
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());

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

        currentFile->setChangeSet(changes);
        currentFile->appendIndentRange(currentFile->range(declaration));
        currentFile->apply();
    }

private:
    SimpleDeclarationAST *declaration;
};

void SplitSimpleDeclaration::match(const CppQuickFixInterface &interface,
                                   QuickFixOperations &result)
{
    CoreDeclaratorAST *core_declarator = 0;
    const QList<AST *> &path = interface->path();
    CppRefactoringFilePtr file = interface->currentFile();
    const int cursorPosition = file->cursor().selectionStart();

    for (int index = path.size() - 1; index != -1; --index) {
        AST *node = path.at(index);

        if (CoreDeclaratorAST *coreDecl = node->asCoreDeclarator())
            core_declarator = coreDecl;

        else if (SimpleDeclarationAST *simpleDecl = node->asSimpleDeclaration()) {
            if (checkDeclaration(simpleDecl)) {
                SimpleDeclarationAST *declaration = simpleDecl;

                const int startOfDeclSpecifier = file->startOf(declaration->decl_specifier_list->firstToken());
                const int endOfDeclSpecifier = file->endOf(declaration->decl_specifier_list->lastToken() - 1);

                if (cursorPosition >= startOfDeclSpecifier && cursorPosition <= endOfDeclSpecifier) {
                    // the AST node under cursor is a specifier.
                    result.append(QuickFixOperation::Ptr(
                        new SplitSimpleDeclarationOp(interface, index, declaration)));
                    return;
                }

                if (core_declarator && interface->isCursorOn(core_declarator)) {
                    // got a core-declarator under the text cursor.
                    result.append(QuickFixOperation::Ptr(
                        new SplitSimpleDeclarationOp(interface, index, declaration)));
                    return;
                }
            }

            return;
        }
    }
}

class AddBracesToIfOp: public CppQuickFixOperation
{
public:
    AddBracesToIfOp(const CppQuickFixInterface &interface, int priority, StatementAST *statement)
        : CppQuickFixOperation(interface, priority)
        , _statement(statement)
    {
        setDescription(QApplication::translate("CppTools::QuickFix",
                                               "Add Curly Braces"));
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());

        ChangeSet changes;

        const int start = currentFile->endOf(_statement->firstToken() - 1);
        changes.insert(start, QLatin1String(" {"));

        const int end = currentFile->endOf(_statement->lastToken() - 1);
        changes.insert(end, QLatin1String("\n}"));

        currentFile->setChangeSet(changes);
        currentFile->appendIndentRange(Utils::ChangeSet::Range(start, end));
        currentFile->apply();
    }

private:
    StatementAST *_statement;
};

void AddBracesToIf::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface->path();

    // show when we're on the 'if' of an if statement
    int index = path.size() - 1;
    IfStatementAST *ifStatement = path.at(index)->asIfStatement();
    if (ifStatement && interface->isCursorOn(ifStatement->if_token) && ifStatement->statement
        && ! ifStatement->statement->asCompoundStatement()) {
        result.append(QuickFixOperation::Ptr(
            new AddBracesToIfOp(interface, index, ifStatement->statement)));
        return;
    }

    // or if we're on the statement contained in the if
    // ### This may not be such a good idea, consider nested ifs...
    for (; index != -1; --index) {
        IfStatementAST *ifStatement = path.at(index)->asIfStatement();
        if (ifStatement && ifStatement->statement
            && interface->isCursorOn(ifStatement->statement)
            && ! ifStatement->statement->asCompoundStatement()) {
            result.append(QuickFixOperation::Ptr(
                new AddBracesToIfOp(interface, index, ifStatement->statement)));
            return;
        }
    }

    // ### This could very well be extended to the else branch
    // and other nodes entirely.
}

class MoveDeclarationOutOfIfOp: public CppQuickFixOperation
{
public:
    MoveDeclarationOutOfIfOp(const CppQuickFixInterface &interface)
        : CppQuickFixOperation(interface)
    {
        setDescription(QApplication::translate("CppTools::QuickFix",
                                               "Move Declaration out of Condition"));

        condition = mk.Condition();
        pattern = mk.IfStatement(condition);
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());

        ChangeSet changes;

        changes.copy(currentFile->range(core), currentFile->startOf(condition));

        int insertPos = currentFile->startOf(pattern);
        changes.move(currentFile->range(condition), insertPos);
        changes.insert(insertPos, QLatin1String(";\n"));

        currentFile->setChangeSet(changes);
        currentFile->appendIndentRange(currentFile->range(pattern));
        currentFile->apply();
    }

    ASTMatcher matcher;
    ASTPatternBuilder mk;
    CPPEditorWidget *editor;
    ConditionAST *condition;
    IfStatementAST *pattern;
    CoreDeclaratorAST *core;
};

void MoveDeclarationOutOfIf::match(const CppQuickFixInterface &interface,
                                   QuickFixOperations &result)
{
    const QList<AST *> &path = interface->path();
    typedef QSharedPointer<MoveDeclarationOutOfIfOp> Ptr;
    Ptr op(new MoveDeclarationOutOfIfOp(interface));

    int index = path.size() - 1;
    for (; index != -1; --index) {
        if (IfStatementAST *statement = path.at(index)->asIfStatement()) {
            if (statement->match(op->pattern, &op->matcher) && op->condition->declarator) {
                DeclaratorAST *declarator = op->condition->declarator;
                op->core = declarator->core_declarator;
                if (! op->core)
                    return;

                if (interface->isCursorOn(op->core)) {
                    op->setPriority(index);
                    result.append(op);
                    return;
                }
            }
        }
    }
}

class MoveDeclarationOutOfWhileOp: public CppQuickFixOperation
{
public:
    MoveDeclarationOutOfWhileOp(const CppQuickFixInterface &interface)
        : CppQuickFixOperation(interface)
    {
        setDescription(QApplication::translate("CppTools::QuickFix",
                                               "Move Declaration out of Condition"));

        condition = mk.Condition();
        pattern = mk.WhileStatement(condition);
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());

        ChangeSet changes;

        changes.insert(currentFile->startOf(condition), QLatin1String("("));
        changes.insert(currentFile->endOf(condition), QLatin1String(") != 0"));

        int insertPos = currentFile->startOf(pattern);
        const int conditionStart = currentFile->startOf(condition);
        changes.move(conditionStart, currentFile->startOf(core), insertPos);
        changes.copy(currentFile->range(core), insertPos);
        changes.insert(insertPos, QLatin1String(";\n"));

        currentFile->setChangeSet(changes);
        currentFile->appendIndentRange(currentFile->range(pattern));
        currentFile->apply();
    }

    ASTMatcher matcher;
    ASTPatternBuilder mk;
    CPPEditorWidget *editor;
    ConditionAST *condition;
    WhileStatementAST *pattern;
    CoreDeclaratorAST *core;
};

void MoveDeclarationOutOfWhile::match(const CppQuickFixInterface &interface,
                                      QuickFixOperations &result)
{
    const QList<AST *> &path = interface->path();
    QSharedPointer<MoveDeclarationOutOfWhileOp> op(new MoveDeclarationOutOfWhileOp(interface));

    int index = path.size() - 1;
    for (; index != -1; --index) {
        if (WhileStatementAST *statement = path.at(index)->asWhileStatement()) {
            if (statement->match(op->pattern, &op->matcher) && op->condition->declarator) {
                DeclaratorAST *declarator = op->condition->declarator;
                op->core = declarator->core_declarator;

                if (! op->core)
                    return;

                if (! declarator->equal_token)
                    return;

                if (! declarator->initializer)
                    return;

                if (interface->isCursorOn(op->core)) {
                    op->setPriority(index);
                    result.append(op);
                    return;
                }
            }
        }
    }
}

class SplitIfStatementOp: public CppQuickFixOperation
{
public:
    SplitIfStatementOp(const CppQuickFixInterface &interface, int priority,
                       IfStatementAST *pattern, BinaryExpressionAST *condition)
        : CppQuickFixOperation(interface, priority)
        , pattern(pattern)
        , condition(condition)
    {
        setDescription(QApplication::translate("CppTools::QuickFix",
                                               "Split if Statement"));
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());

        const Token binaryToken = currentFile->tokenAt(condition->binary_op_token);

        if (binaryToken.is(T_AMPER_AMPER))
            splitAndCondition(currentFile);
        else
            splitOrCondition(currentFile);
    }

    void splitAndCondition(CppRefactoringFilePtr currentFile) const
    {
        ChangeSet changes;

        int startPos = currentFile->startOf(pattern);
        changes.insert(startPos, QLatin1String("if ("));
        changes.move(currentFile->range(condition->left_expression), startPos);
        changes.insert(startPos, QLatin1String(") {\n"));

        const int lExprEnd = currentFile->endOf(condition->left_expression);
        changes.remove(lExprEnd, currentFile->startOf(condition->right_expression));
        changes.insert(currentFile->endOf(pattern), QLatin1String("\n}"));

        currentFile->setChangeSet(changes);
        currentFile->appendIndentRange(currentFile->range(pattern));
        currentFile->apply();
    }

    void splitOrCondition(CppRefactoringFilePtr currentFile) const
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

        currentFile->setChangeSet(changes);
        currentFile->appendIndentRange(currentFile->range(pattern));
        currentFile->apply();
    }

private:
    IfStatementAST *pattern;
    BinaryExpressionAST *condition;
};

void SplitIfStatement::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    IfStatementAST *pattern = 0;
    const QList<AST *> &path = interface->path();

    int index = path.size() - 1;
    for (; index != -1; --index) {
        AST *node = path.at(index);
        if (IfStatementAST *stmt = node->asIfStatement()) {
            pattern = stmt;
            break;
        }
    }

    if (! pattern || ! pattern->statement)
        return;

    unsigned splitKind = 0;
    for (++index; index < path.size(); ++index) {
        AST *node = path.at(index);
        BinaryExpressionAST *condition = node->asBinaryExpression();
        if (! condition)
            return;

        Token binaryToken = interface->currentFile()->tokenAt(condition->binary_op_token);

        // only accept a chain of ||s or &&s - no mixing
        if (! splitKind) {
            splitKind = binaryToken.kind();
            if (splitKind != T_AMPER_AMPER && splitKind != T_PIPE_PIPE)
                return;
            // we can't reliably split &&s in ifs with an else branch
            if (splitKind == T_AMPER_AMPER && pattern->else_statement)
                return;
        } else if (splitKind != binaryToken.kind()) {
            return;
        }

        if (interface->isCursorOn(condition->binary_op_token)) {
            result.append(QuickFixOperation::Ptr(
                new SplitIfStatementOp(interface, index, pattern, condition)));
            return;
        }
    }
}

static inline QString msgQtStringLiteralDescription(const QString &replacement, int qtVersion)
{
    return QApplication::translate("CppTools::QuickFix", "Enclose in %1(...) (Qt %2)")
           .arg(replacement).arg(qtVersion);
}

static inline QString msgQtStringLiteralDescription(const QString &replacement)
{
    return QApplication::translate("CppTools::QuickFix", "Enclose in %1(...)").arg(replacement);
}

/* Analze a string/character literal like "x", QLatin1String("x") and return the literal
 * (StringLiteral or NumericLiteral for characters) and its type
 * and the enclosing function (QLatin1String, tr...) */
ExpressionAST *WrapStringLiteral::analyze(const QList<AST *> &path,
                                          const CppRefactoringFilePtr &file,
                                          Type *type,
                                          QByteArray *enclosingFunction /* = 0 */,
                                          CallAST **enclosingFunctionCall /* = 0 */)
{
    *type = TypeNone;
    if (enclosingFunction)
        enclosingFunction->clear();
    if (enclosingFunctionCall)
        *enclosingFunctionCall = 0;

    if (path.isEmpty())
        return 0;

    ExpressionAST *literal = path.last()->asExpression();
    if (literal) {
        if (literal->asStringLiteral()) {
            // Check for Objective C string (@"bla")
            const QChar firstChar = file->charAt(file->startOf(literal));
            *type = firstChar == QLatin1Char('@') ? TypeObjCString : TypeString;
        } else if (NumericLiteralAST *numericLiteral = literal->asNumericLiteral()) {
            // character ('c') constants are numeric.
            if (file->tokenAt(numericLiteral->literal_token).is(T_CHAR_LITERAL))
                *type = TypeChar;
        }
    }

    if (*type != TypeNone && enclosingFunction && path.size() > 1) {
        if (CallAST *call = path.at(path.size() - 2)->asCall()) {
            if (call->base_expression) {
                if (IdExpressionAST *idExpr = call->base_expression->asIdExpression()) {
                    if (SimpleNameAST *functionName = idExpr->name->asSimpleName()) {
                        *enclosingFunction = file->tokenAt(functionName->identifier_token).identifier->chars();
                        if (enclosingFunctionCall)
                            *enclosingFunctionCall = call;
                    }
                }
            }
        }
    }
    return literal;
}

/// Operation performs the operations of type ActionFlags passed in as actions.
class WrapStringLiteralOp : public CppQuickFixOperation
{
public:
    typedef WrapStringLiteral Factory;

    WrapStringLiteralOp(const CppQuickFixInterface &interface, int priority,
                        unsigned actions, const QString &description, ExpressionAST *literal,
                        const QString &translationContext = QString())
        : CppQuickFixOperation(interface, priority), m_actions(actions), m_literal(literal),
          m_translationContext(translationContext)
    {
        setDescription(description);
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());

        ChangeSet changes;

        const int startPos = currentFile->startOf(m_literal);
        const int endPos = currentFile->endOf(m_literal);

        // kill leading '@'. No need to adapt endPos, that is done by ChangeSet
        if (m_actions & Factory::RemoveObjectiveCAction)
            changes.remove(startPos, startPos + 1);

        // Fix quotes
        if (m_actions & (Factory::SingleQuoteAction | Factory::DoubleQuoteAction)) {
            const QString newQuote((m_actions & Factory::SingleQuoteAction) ? QLatin1Char('\'') : QLatin1Char('"'));
            changes.replace(startPos, startPos + 1, newQuote);
            changes.replace(endPos - 1, endPos, newQuote);
        }

        // Convert single character strings into character constants
        if (m_actions & Factory::ConvertEscapeSequencesToCharAction) {
            StringLiteralAST *stringLiteral = m_literal->asStringLiteral();
            QTC_ASSERT(stringLiteral, return ;);
            const QByteArray oldContents(currentFile->tokenAt(stringLiteral->literal_token).identifier->chars());
            const QByteArray newContents = Factory::stringToCharEscapeSequences(oldContents);
            QTC_ASSERT(!newContents.isEmpty(), return ;);
            if (oldContents != newContents)
                changes.replace(startPos + 1, endPos -1, QString::fromLatin1(newContents));
        }

        // Convert character constants into strings constants
        if (m_actions & Factory::ConvertEscapeSequencesToStringAction) {
            NumericLiteralAST *charLiteral = m_literal->asNumericLiteral(); // char 'c' constants are numerical.
            QTC_ASSERT(charLiteral, return ;);
            const QByteArray oldContents(currentFile->tokenAt(charLiteral->literal_token).identifier->chars());
            const QByteArray newContents = Factory::charToStringEscapeSequences(oldContents);
            QTC_ASSERT(!newContents.isEmpty(), return ;);
            if (oldContents != newContents)
                changes.replace(startPos + 1, endPos -1, QString::fromLatin1(newContents));
        }

        // Enclose in literal or translation function, macro.
        if (m_actions & (Factory::EncloseActionMask | Factory::TranslationMask)) {
            changes.insert(endPos, QString(QLatin1Char(')')));
            QString leading = Factory::replacement(m_actions);
            leading += QLatin1Char('(');
            if (m_actions & (Factory::TranslateQCoreApplicationAction | Factory::TranslateNoopAction)) {
                leading += QLatin1Char('"');
                leading += m_translationContext;
                leading += QLatin1String("\", ");
            }
            changes.insert(startPos, leading);
        }

        currentFile->setChangeSet(changes);
        currentFile->apply();
    }

private:
    const unsigned m_actions;
    ExpressionAST *m_literal;
    const QString m_translationContext;
};

void WrapStringLiteral::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    typedef CppQuickFixOperation::Ptr OperationPtr;

    Type type = TypeNone;
    QByteArray enclosingFunction;
    const QList<AST *> &path = interface->path();
    CppRefactoringFilePtr file = interface->currentFile();
    ExpressionAST *literal = analyze(path, file, &type, &enclosingFunction);
    if (!literal || type == TypeNone)
        return;
    if ((type == TypeChar && enclosingFunction == "QLatin1Char")
        || isQtStringLiteral(enclosingFunction)
        || isQtStringTranslation(enclosingFunction))
        return;

    const int priority = path.size() - 1; // very high priority
    if (type == TypeChar) {
        unsigned actions = EncloseInQLatin1CharAction;
        QString description = msgQtStringLiteralDescription(replacement(actions));
        result << OperationPtr(new WrapStringLiteralOp(interface, priority, actions,
                                                             description, literal));
        if (NumericLiteralAST *charLiteral = literal->asNumericLiteral()) {
            const QByteArray contents(file->tokenAt(charLiteral->literal_token).identifier->chars());
            if (!charToStringEscapeSequences(contents).isEmpty()) {
                actions = DoubleQuoteAction | ConvertEscapeSequencesToStringAction;
                description = QApplication::translate("CppTools::QuickFix",
                              "Convert to String Literal");
                result << OperationPtr(new WrapStringLiteralOp(interface, priority, actions,
                                                                     description, literal));
            }
        }
    } else {
        const unsigned objectiveCActions = type == TypeObjCString ?
                                           unsigned(RemoveObjectiveCAction) : 0u;
        unsigned actions = 0;
        if (StringLiteralAST *stringLiteral = literal->asStringLiteral()) {
            const QByteArray contents(file->tokenAt(stringLiteral->literal_token).identifier->chars());
            if (!stringToCharEscapeSequences(contents).isEmpty()) {
                actions = EncloseInQLatin1CharAction | SingleQuoteAction
                          | ConvertEscapeSequencesToCharAction | objectiveCActions;
                QString description = QApplication::translate("CppTools::QuickFix",
                                      "Convert to Character Literal and Enclose in QLatin1Char(...)");
                result << OperationPtr(new WrapStringLiteralOp(interface, priority,
                                                                     actions, description, literal));
                actions &= ~EncloseInQLatin1CharAction;
                description = QApplication::translate("CppTools::QuickFix",
                              "Convert to Character Literal");
                result << OperationPtr(new WrapStringLiteralOp(interface, priority,
                                                                     actions, description, literal));
            }
        }
        actions = EncloseInQLatin1StringAction | objectiveCActions;
        result << OperationPtr(
            new WrapStringLiteralOp(interface, priority, actions,
                msgQtStringLiteralDescription(replacement(actions), 4),
                    literal));
        actions = EncloseInQStringLiteralAction | objectiveCActions;
        result << OperationPtr(
            new WrapStringLiteralOp(interface, priority, actions,
                msgQtStringLiteralDescription(replacement(actions), 5), literal));
    }
}

QString WrapStringLiteral::replacement(unsigned actions)
{
    if (actions & EncloseInQLatin1CharAction)
        return QLatin1String("QLatin1Char");
    if (actions & EncloseInQLatin1StringAction)
        return QLatin1String("QLatin1String");
    if (actions & EncloseInQStringLiteralAction)
        return QLatin1String("QStringLiteral");
    if (actions & TranslateTrAction)
        return QLatin1String("tr");
    if (actions & TranslateQCoreApplicationAction)
        return QLatin1String("QCoreApplication::translate");
    if (actions & TranslateNoopAction)
        return QLatin1String("QT_TRANSLATE_NOOP");
    return QString();
}

/* Convert single-character string literals into character literals with some
 * special cases "a" --> 'a', "'" --> '\'', "\n" --> '\n', "\"" --> '"'. */
QByteArray WrapStringLiteral::stringToCharEscapeSequences(const QByteArray &content)
{
    if (content.size() == 1)
        return content.at(0) == '\'' ? QByteArray("\\'") : content;
    if (content.size() == 2 && content.at(0) == '\\')
        return content == "\\\"" ? QByteArray(1, '"') : content;
    return QByteArray();
}

/* Convert character literal into a string literal with some special cases
 * 'a' -> "a", '\n' -> "\n", '\'' --> "'", '"' --> "\"". */
QByteArray WrapStringLiteral::charToStringEscapeSequences(const QByteArray &content)
{
    if (content.size() == 1)
        return content.at(0) == '"' ? QByteArray("\\\"") : content;
    if (content.size() == 2)
        return content == "\\'" ? QByteArray("'") : content;
    return QByteArray();
}

void TranslateStringLiteral::match(const CppQuickFixInterface &interface,
                                   QuickFixOperations &result)
{
    // Initialize
    WrapStringLiteral::Type type = WrapStringLiteral::TypeNone;
    QByteArray enclosingFunction;
    const QList<AST *> &path = interface->path();
    CppRefactoringFilePtr file = interface->currentFile();
    ExpressionAST *literal = WrapStringLiteral::analyze(path, file, &type, &enclosingFunction);
    if (!literal || type != WrapStringLiteral::TypeString
       || isQtStringLiteral(enclosingFunction) || isQtStringTranslation(enclosingFunction))
        return;

    QString trContext;

    QSharedPointer<Control> control = interface->context().control();
    const Name *trName = control->identifier("tr");

    // Check whether we are in a method:
    const QString description = QApplication::translate("CppTools::QuickFix", "Mark as Translatable");
    for (int i = path.size() - 1; i >= 0; --i) {
        if (FunctionDefinitionAST *definition = path.at(i)->asFunctionDefinition()) {
            Function *function = definition->symbol;
            ClassOrNamespace *b = interface->context().lookupType(function);
            if (b) {
                // Do we have a tr method?
                foreach (const LookupItem &r, b->find(trName)) {
                    Symbol *s = r.declaration();
                    if (s->type()->isFunctionType()) {
                        // no context required for tr
                        result.append(QuickFixOperation::Ptr(
                            new WrapStringLiteralOp(interface, path.size() - 1,
                                                          WrapStringLiteral::TranslateTrAction,
                                                          description, literal)));
                        return;
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
            result.append(QuickFixOperation::Ptr(
                new WrapStringLiteralOp(interface, path.size() - 1,
                                              WrapStringLiteral::TranslateQCoreApplicationAction,
                                              description, literal, trContext)));
            return;
        }
    }

    // We need to use Q_TRANSLATE_NOOP
    result.append(QuickFixOperation::Ptr(
        new WrapStringLiteralOp(interface, path.size() - 1,
                                      WrapStringLiteral::TranslateNoopAction,
                                      description, literal, trContext)));
}

class ConvertCStringToNSStringOp: public CppQuickFixOperation
{
public:
    ConvertCStringToNSStringOp(const CppQuickFixInterface &interface, int priority,
                               StringLiteralAST *stringLiteral, CallAST *qlatin1Call)
        : CppQuickFixOperation(interface, priority)
        , stringLiteral(stringLiteral)
        , qlatin1Call(qlatin1Call)
    {
        setDescription(QApplication::translate("CppTools::QuickFix",
                                               "Convert to Objective-C String Literal"));
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());

        ChangeSet changes;

        if (qlatin1Call) {
            changes.replace(currentFile->startOf(qlatin1Call), currentFile->startOf(stringLiteral), QLatin1String("@"));
            changes.remove(currentFile->endOf(stringLiteral), currentFile->endOf(qlatin1Call));
        } else {
            changes.insert(currentFile->startOf(stringLiteral), QLatin1String("@"));
        }

        currentFile->setChangeSet(changes);
        currentFile->apply();
    }

private:
    StringLiteralAST *stringLiteral;
    CallAST *qlatin1Call;
};

void ConvertCStringToNSString::match(const CppQuickFixInterface &interface,
                                     QuickFixOperations &result)
{
    CppRefactoringFilePtr file = interface->currentFile();

    if (interface->editor()->mimeType() != QLatin1String(CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE))
        return;

    WrapStringLiteral::Type type = WrapStringLiteral::TypeNone;
    QByteArray enclosingFunction;
    CallAST *qlatin1Call;
    const QList<AST *> &path = interface->path();
    ExpressionAST *literal = WrapStringLiteral::analyze(path, file, &type, &enclosingFunction, &qlatin1Call);
    if (!literal || type != WrapStringLiteral::TypeString)
        return;
    if (!isQtStringLiteral(enclosingFunction))
        qlatin1Call = 0;

    result.append(QuickFixOperation::Ptr(
        new ConvertCStringToNSStringOp(interface, path.size() - 1, literal->asStringLiteral(),
            qlatin1Call)));
}

class ConvertNumericLiteralOp: public CppQuickFixOperation
{
public:
    ConvertNumericLiteralOp(const CppQuickFixInterface &interface, int start, int end,
                            const QString &replacement)
        : CppQuickFixOperation(interface)
        , start(start)
        , end(end)
        , replacement(replacement)
    {}

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());

        ChangeSet changes;
        changes.replace(start, end, replacement);
        currentFile->setChangeSet(changes);
        currentFile->apply();
    }

private:
    int start, end;
    QString replacement;
};

void ConvertNumericLiteral::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface->path();
    CppRefactoringFilePtr file = interface->currentFile();

    if (path.isEmpty())
        return;

    NumericLiteralAST *literal = path.last()->asNumericLiteral();

    if (! literal)
        return;

    Token token = file->tokenAt(literal->asNumericLiteral()->literal_token);
    if (!token.is(T_NUMERIC_LITERAL))
        return;
    const NumericLiteral *numeric = token.number;
    if (numeric->isDouble() || numeric->isFloat())
        return;

    // remove trailing L or U and stuff
    const char * const spell = numeric->chars();
    int numberLength = numeric->size();
    while (numberLength > 0 && !std::isxdigit(spell[numberLength - 1]))
        --numberLength;
    if (numberLength < 1)
        return;

    // convert to number
    bool valid;
    ulong value = QString::fromUtf8(spell).left(numberLength).toULong(&valid, 0);
    if (!valid) // e.g. octal with digit > 7
        return;

    const int priority = path.size() - 1; // very high priority
    const int start = file->startOf(literal);
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
        QuickFixOperation::Ptr op(
            new ConvertNumericLiteralOp(interface, start, start + numberLength, replacement));
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
            QuickFixOperation::Ptr op(
                new ConvertNumericLiteralOp(interface, start, start + numberLength, replacement));
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
            QuickFixOperation::Ptr op(
                new ConvertNumericLiteralOp(interface, start, start + numberLength, replacement));
            op->setDescription(QApplication::translate("CppTools::QuickFix", "Convert to Decimal"));
            op->setPriority(priority);
            result.append(op);
        }
    }
}

class AddIncludeForForwardDeclarationOp: public CppQuickFixOperation
{
public:
    AddIncludeForForwardDeclarationOp(const CppQuickFixInterface &interface, int priority,
                                      Symbol *fwdClass)
        : CppQuickFixOperation(interface, priority)
        , fwdClass(fwdClass)
    {
        setDescription(QApplication::translate("CppTools::QuickFix",
                                               "#include Header File"));
    }

    void perform()
    {
        QTC_ASSERT(fwdClass != 0, return);
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());

        CppTools::SymbolFinder symbolFinder;
        if (Class *k = symbolFinder.findMatchingClassDeclaration(fwdClass, snapshot())) {
            const QString headerFile = QString::fromUtf8(k->fileName(), k->fileNameLength());

            // collect the fwd headers
            Snapshot fwdHeaders;
            fwdHeaders.insert(snapshot().document(headerFile));
            foreach (Document::Ptr doc, snapshot()) {
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
            foreach (const Document::Include &incl, assistInterface()->semanticInfo().doc->includes()) {
                if (incl.line() < currentLine)
                    bestLine = incl.line();
            }

            if (bestLine)
                pos = currentFile->document()->findBlockByNumber(bestLine).position();

            Utils::ChangeSet changes;
            changes.insert(pos, QLatin1String("#include <")
                           + QFileInfo(best).fileName() + QLatin1String(">\n"));
            currentFile->setChangeSet(changes);
            currentFile->apply();
        }
    }

    static Symbol *checkName(const CppQuickFixInterface &interface, NameAST *ast)
    {
        if (ast && interface->isCursorOn(ast)) {
            if (const Name *name = ast->name) {
                unsigned line, column;
                interface->semanticInfo().doc->translationUnit()->getTokenStartPosition(ast->firstToken(), &line, &column);

                Symbol *fwdClass = 0;

                foreach (const LookupItem &r,
                         interface->context().lookup(name,
                                                     interface->semanticInfo().doc->scopeAt(line, column))) {
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
    Symbol *fwdClass;
};

void AddIncludeForForwardDeclaration::match(const CppQuickFixInterface &interface,
                                            QuickFixOperations &result)
{
    const QList<AST *> &path = interface->path();

    for (int index = path.size() - 1; index != -1; --index) {
        AST *ast = path.at(index);
        if (NamedTypeSpecifierAST *namedTy = ast->asNamedTypeSpecifier()) {
            if (Symbol *fwdClass = AddIncludeForForwardDeclarationOp::checkName(interface, namedTy->name)) {
                result.append(QuickFixOperation::Ptr(
                    new AddIncludeForForwardDeclarationOp(interface, index, fwdClass)));
                return;
            }
        } else if (ElaboratedTypeSpecifierAST *eTy = ast->asElaboratedTypeSpecifier()) {
            if (Symbol *fwdClass = AddIncludeForForwardDeclarationOp::checkName(interface, eTy->name)) {
                result.append(QuickFixOperation::Ptr(
                    new AddIncludeForForwardDeclarationOp(interface, index, fwdClass)));
                return;
            }
        }
    }
}

class AddLocalDeclarationOp: public CppQuickFixOperation
{
public:
    AddLocalDeclarationOp(const CppQuickFixInterface &interface,
                          int priority,
                          const BinaryExpressionAST *binaryAST,
                          const SimpleNameAST *simpleNameAST)
        : CppQuickFixOperation(interface, priority)
        , binaryAST(binaryAST)
        , simpleNameAST(simpleNameAST)
    {
        setDescription(QApplication::translate("CppTools::QuickFix", "Add Local Declaration"));
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());

        TypeOfExpression typeOfExpression;
        typeOfExpression.init(assistInterface()->semanticInfo().doc,
                              snapshot(), assistInterface()->context().bindings());
        Scope *scope = currentFile->scopeAt(binaryAST->firstToken());
        const QList<LookupItem> result =
                typeOfExpression(currentFile->textOf(binaryAST->right_expression).toUtf8(),
                                 scope,
                                 TypeOfExpression::Preprocess);

        if (! result.isEmpty()) {
            SubstitutionEnvironment env;
            env.setContext(assistInterface()->context());
            env.switchScope(result.first().scope());
            ClassOrNamespace *con = typeOfExpression.context().lookupType(scope);
            if (!con)
                con = typeOfExpression.context().globalNamespace();
            UseMinimalNames q(con);
            env.enter(&q);

            Control *control = assistInterface()->context().control().data();
            FullySpecifiedType tn = rewriteType(result.first().type(), &env, control);

            Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
            QString ty = oo.prettyType(tn, simpleNameAST->name);
            if (! ty.isEmpty()) {
                Utils::ChangeSet changes;
                changes.replace(currentFile->startOf(binaryAST),
                                currentFile->endOf(simpleNameAST),
                                ty);
                currentFile->setChangeSet(changes);
                currentFile->apply();
            }
        }
    }

private:
    const BinaryExpressionAST *binaryAST;
    const SimpleNameAST *simpleNameAST;
};

void AddLocalDeclaration::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface->path();
    CppRefactoringFilePtr file = interface->currentFile();

    for (int index = path.size() - 1; index != -1; --index) {
        if (BinaryExpressionAST *binary = path.at(index)->asBinaryExpression()) {
            if (binary->left_expression && binary->right_expression && file->tokenAt(binary->binary_op_token).is(T_EQUAL)) {
                IdExpressionAST *idExpr = binary->left_expression->asIdExpression();
                if (interface->isCursorOn(binary->left_expression) && idExpr && idExpr->name->asSimpleName() != 0) {
                    SimpleNameAST *nameAST = idExpr->name->asSimpleName();
                    const QList<LookupItem> results = interface->context().lookup(nameAST->name, file->scopeAt(nameAST->firstToken()));
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
                        result.append(QuickFixOperation::Ptr(
                            new AddLocalDeclarationOp(interface, index, binary, nameAST)));
                        return;
                    }
                }
            }
        }
    }
}

class ConvertToCamelCaseOp: public CppQuickFixOperation
{
public:
    ConvertToCamelCaseOp(const CppQuickFixInterface &interface, int priority,
                         const QString &newName)
        : CppQuickFixOperation(interface, priority)
        , m_name(newName)
    {
        setDescription(QApplication::translate("CppTools::QuickFix",
                                               "Convert to Camel Case"));
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());

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
        static_cast<CppEditor::Internal::CPPEditorWidget*>(assistInterface()->editor())->renameUsagesNow(m_name);
    }

    static bool isConvertibleUnderscore(const QString &name, int pos)
    {
        return name.at(pos) == QLatin1Char('_') && name.at(pos+1).isLetter()
                && !(pos == 1 && name.at(0) == QLatin1Char('m'));
    }

private:
    QString m_name;
};

void ConvertToCamelCase::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface->path();

    if (path.isEmpty())
        return;

    AST * const ast = path.last();
    const Name *name = 0;
    if (const NameAST * const nameAst = ast->asName()) {
        if (nameAst->name && nameAst->name->asNameId())
            name = nameAst->name;
    } else if (const NamespaceAST * const namespaceAst = ast->asNamespace()) {
        name = namespaceAst->symbol->name();
    }

    if (!name)
        return;

    QString newName = QString::fromUtf8(name->identifier()->chars());
    if (newName.length() < 3)
        return;
    for (int i = 1; i < newName.length() - 1; ++i) {
        if (ConvertToCamelCaseOp::isConvertibleUnderscore(newName, i)) {
            result.append(QuickFixOperation::Ptr(
                new ConvertToCamelCaseOp(interface, path.size() - 1, newName)));
            return;
        }
    }
}

class AddIncludeForUndefinedIdentifierOp: public CppQuickFixOperation
{
public:
    AddIncludeForUndefinedIdentifierOp(const CppQuickFixInterface &interface, int priority,
                                       const QString &include)
        : CppQuickFixOperation(interface, priority)
        , m_include(include)
    {
        setDescription(QApplication::translate("CppTools::QuickFix",
                                               "Add #include %1").arg(m_include));
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr file = refactoring.file(fileName());

        // find location of last include in file
        QList<Document::Include> includes = file->cppDocument()->includes();
        unsigned lastIncludeLine = 0;
        foreach (const Document::Include &include, includes) {
            if (include.line() > lastIncludeLine)
                lastIncludeLine = include.line();
        }

        // add include
        const int insertPos = file->position(lastIncludeLine + 1, 1) - 1;
        ChangeSet changes;
        changes.insert(insertPos, QLatin1String("\n#include ") + m_include);
        file->setChangeSet(changes);
        file->apply();
    }

private:
    QString m_include;
};

void AddIncludeForUndefinedIdentifier::match(const CppQuickFixInterface &interface,
                                             QuickFixOperations &result)
{
    CppClassesFilter *classesFilter = ExtensionSystem::PluginManager::getObject<CppClassesFilter>();
    if (!classesFilter)
        return;

    const QList<AST *> &path = interface->path();

    if (path.isEmpty())
        return;

    // find the largest enclosing Name
    const NameAST *enclosingName = 0;
    const SimpleNameAST *innermostName = 0;
    for (int i = path.size() - 1; i >= 0; --i) {
        if (NameAST *nameAst = path.at(i)->asName()) {
            enclosingName = nameAst;
            if (!innermostName) {
                innermostName = nameAst->asSimpleName();
                if (!innermostName)
                    return;
            }
        } else {
            break;
        }
    }
    if (!enclosingName || !enclosingName->name)
        return;

    // find the enclosing scope
    unsigned line, column;
    const Document::Ptr &doc = interface->semanticInfo().doc;
    doc->translationUnit()->getTokenStartPosition(enclosingName->firstToken(), &line, &column);
    Scope *scope = doc->scopeAt(line, column);
    if (!scope)
        return;

    // check if the name resolves to something
    QList<LookupItem> existingResults = interface->context().lookup(enclosingName->name, scope);
    if (!existingResults.isEmpty())
        return;

    const QString &className = Overview().prettyName(innermostName->name);
    if (className.isEmpty())
        return;

    // find the include paths
    QStringList includePaths;
    CppModelManagerInterface *modelManager = CppModelManagerInterface::instance();
    QList<CppModelManagerInterface::ProjectInfo> projectInfos = modelManager->projectInfos();
    bool inProject = false;
    foreach (const CppModelManagerInterface::ProjectInfo &info, projectInfos) {
        foreach (ProjectPart::Ptr part, info.projectParts()) {
            foreach (const ProjectFile &file, part->files) {
                if (file.path == doc->fileName()) {
                    inProject = true;
                    includePaths += part->includePaths;
                }
            }
        }
    }
    if (!inProject) {
        // better use all include paths than none
        foreach (const CppModelManagerInterface::ProjectInfo &info, projectInfos) {
            foreach (ProjectPart::Ptr part, info.projectParts())
                includePaths += part->includePaths;
        }
    }

    // find a include file through the locator
    QFutureInterface<Locator::FilterEntry> dummyInterface;
    QList<Locator::FilterEntry> matches = classesFilter->matchesFor(dummyInterface, className);
    bool classExists = false;
    foreach (const Locator::FilterEntry &entry, matches) {
        const ModelItemInfo info = entry.internalData.value<ModelItemInfo>();
        if (info.symbolName != className)
            continue;
        classExists = true;
        const QString &fileName = info.fileName;
        const QFileInfo fileInfo(fileName);

        // find the shortest way to include fileName given the includePaths
        QString shortestInclude;

        if (fileInfo.path() == QFileInfo(doc->fileName()).path()) {
            shortestInclude = QLatin1Char('"') + fileInfo.fileName() + QLatin1Char('"');
        } else {
            foreach (const QString &includePath, includePaths) {
                if (!fileName.startsWith(includePath))
                    continue;
                QString relativePath = fileName.mid(includePath.size());
                if (!relativePath.isEmpty() && relativePath.at(0) == QLatin1Char('/'))
                    relativePath = relativePath.mid(1);
                if (shortestInclude.isEmpty() || relativePath.size() + 2 < shortestInclude.size())
                    shortestInclude = QLatin1Char('<') + relativePath + QLatin1Char('>');
            }
        }

        if (!shortestInclude.isEmpty()) {
            result += CppQuickFixOperation::Ptr(
                new AddIncludeForUndefinedIdentifierOp(interface, 0, shortestInclude));
        }
    }

    // for QSomething, propose a <QSomething> include -- if such a class was in the locator
    if (classExists
            && className.size() > 2
            && className.at(0) == QLatin1Char('Q')
            && className.at(1).isUpper()) {
        const QString include = QLatin1Char('<') + className + QLatin1Char('>');
        result += CppQuickFixOperation::Ptr(
            new AddIncludeForUndefinedIdentifierOp(interface, 1, include));
    }
}

class RearrangeParamDeclarationListOp: public CppQuickFixOperation
{
public:
    enum Target { TargetPrevious, TargetNext };

    RearrangeParamDeclarationListOp(const CppQuickFixInterface &interface, AST *currentParam,
                                    AST *targetParam, Target target)
        : CppQuickFixOperation(interface)
        , m_currentParam(currentParam)
        , m_targetParam(targetParam)
    {
        QString targetString;
        if (target == TargetPrevious)
            targetString = QApplication::translate("CppTools::QuickFix",
                                                   "Switch with Previous Parameter");
        else
            targetString = QApplication::translate("CppTools::QuickFix",
                                                   "Switch with Next Parameter");
        setDescription(targetString);
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());

        int targetEndPos = currentFile->endOf(m_targetParam);
        ChangeSet changes;
        changes.flip(currentFile->startOf(m_currentParam), currentFile->endOf(m_currentParam),
                     currentFile->startOf(m_targetParam), targetEndPos);
        currentFile->setChangeSet(changes);
        currentFile->setOpenEditor(false, targetEndPos);
        currentFile->apply();
    }

private:
    AST *m_currentParam;
    AST *m_targetParam;
};

void RearrangeParamDeclarationList::match(const CppQuickFixInterface &interface,
                                          QuickFixOperations &result)
{
    const QList<AST *> path = interface->path();

    ParameterDeclarationAST *paramDecl = 0;
    int index = path.size() - 1;
    for (; index != -1; --index) {
        paramDecl = path.at(index)->asParameterDeclaration();
        if (paramDecl)
            break;
    }

    if (index < 1)
        return;

    ParameterDeclarationClauseAST *paramDeclClause = path.at(index-1)->asParameterDeclarationClause();
    QTC_ASSERT(paramDeclClause && paramDeclClause->parameter_declaration_list, return);

    ParameterDeclarationListAST *paramListNode = paramDeclClause->parameter_declaration_list;
    ParameterDeclarationListAST *prevParamListNode = 0;
    while (paramListNode) {
        if (paramDecl == paramListNode->value)
            break;
        prevParamListNode = paramListNode;
        paramListNode = paramListNode->next;
    }

    if (!paramListNode)
        return;

    if (prevParamListNode)
        result.append(CppQuickFixOperation::Ptr(
            new RearrangeParamDeclarationListOp(interface, paramListNode->value,
                prevParamListNode->value, RearrangeParamDeclarationListOp::TargetPrevious)));
    if (paramListNode->next)
        result.append(CppQuickFixOperation::Ptr(
            new RearrangeParamDeclarationListOp(interface, paramListNode->value,
                paramListNode->next->value, RearrangeParamDeclarationListOp::TargetNext)));
}

class ReformatPointerDeclarationOp: public CppQuickFixOperation
{
public:
    ReformatPointerDeclarationOp(const CppQuickFixInterface &interface,
                                 const Utils::ChangeSet change)
        : CppQuickFixOperation(interface)
        , m_change(change)
    {
        QString description;
        if (m_change.operationList().size() == 1) {
            description = QApplication::translate("CppTools::QuickFix",
                "Reformat to \"%1\"").arg(m_change.operationList().first().text);
        } else { // > 1
            description = QApplication::translate("CppTools::QuickFix",
                "Reformat Pointers or References");
        }
        setDescription(description);
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());
        currentFile->setChangeSet(m_change);
        currentFile->apply();
    }

private:
    Utils::ChangeSet m_change;
};

/// Filter the results of ASTPath.
/// The resulting list contains the supported AST types only once.
/// For this, the results of ASTPath are iterated in reverse order.
class ReformatPointerDeclarationASTPathResultsFilter
{
public:
    ReformatPointerDeclarationASTPathResultsFilter()
        : m_hasSimpleDeclaration(false)
        , m_hasFunctionDefinition(false)
        , m_hasParameterDeclaration(false)
        , m_hasIfStatement(false)
        , m_hasWhileStatement(false)
        , m_hasForStatement(false)
        , m_hasForeachStatement(false) {}

    QList<AST*> filter(const QList<AST*> &astPathList)
    {
        QList<AST*> filtered;

        for (int i = astPathList.size() - 1; i >= 0; --i) {
            AST *ast = astPathList.at(i);

            if (! m_hasSimpleDeclaration && ast->asSimpleDeclaration()) {
                m_hasSimpleDeclaration = true;
                filtered.append(ast);
            } else if (! m_hasFunctionDefinition && ast->asFunctionDefinition()) {
                m_hasFunctionDefinition = true;
                filtered.append(ast);
            } else if (! m_hasParameterDeclaration && ast->asParameterDeclaration()) {
                m_hasParameterDeclaration = true;
                filtered.append(ast);
            } else if (! m_hasIfStatement && ast->asIfStatement()) {
                m_hasIfStatement = true;
                filtered.append(ast);
            } else if (! m_hasWhileStatement && ast->asWhileStatement()) {
                m_hasWhileStatement = true;
                filtered.append(ast);
            } else if (! m_hasForStatement && ast->asForStatement()) {
                m_hasForStatement = true;
                filtered.append(ast);
            } else if (! m_hasForeachStatement && ast->asForeachStatement()) {
                m_hasForeachStatement = true;
                filtered.append(ast);
            }
        }

        return filtered;
    }
private:
    bool m_hasSimpleDeclaration;
    bool m_hasFunctionDefinition;
    bool m_hasParameterDeclaration;
    bool m_hasIfStatement;
    bool m_hasWhileStatement;
    bool m_hasForStatement;
    bool m_hasForeachStatement;
};

void ReformatPointerDeclaration::match(const CppQuickFixInterface &interface,
                                       QuickFixOperations &result)
{
    const QList<AST *> &path = interface->path();
    CppRefactoringFilePtr file = interface->currentFile();

    Overview overview = CppCodeStyleSettings::currentProjectCodeStyleOverview();
    overview.showArgumentNames = true;
    overview.showReturnTypes = true;

    const QTextCursor cursor = file->cursor();
    Utils::ChangeSet change;
    PointerDeclarationFormatter formatter(file, overview,
        PointerDeclarationFormatter::RespectCursor);

    if (cursor.hasSelection()) {
        // This will no work always as expected since this method is only called if
        // interface-path() is not empty. If the user selects the whole document via
        // ctrl-a and there is an empty line in the end, then the cursor is not on
        // any AST and therefore no quick fix will be triggered.
        change = formatter.format(file->cppDocument()->translationUnit()->ast());
        if (! change.isEmpty()) {
            result.append(QuickFixOperation::Ptr(
                new ReformatPointerDeclarationOp(interface, change)));
        }
    } else {
        const QList<AST *> suitableASTs
            = ReformatPointerDeclarationASTPathResultsFilter().filter(path);
        foreach (AST *ast, suitableASTs) {
            change = formatter.format(ast);
            if (! change.isEmpty()) {
                result.append(QuickFixOperation::Ptr(
                    new ReformatPointerDeclarationOp(interface, change)));
                return;
            }
        }
    }
}
