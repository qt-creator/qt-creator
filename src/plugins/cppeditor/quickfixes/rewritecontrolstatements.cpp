// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rewritecontrolstatements.h"

#include "../cppcodestylesettings.h"
#include "../cppeditortr.h"
#include "../cppeditorwidget.h"
#include "../cpprefactoringchanges.h"
#include "cppquickfix.h"

#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>

#ifdef WITH_TESTS
#include "cppquickfix_test.h"
#include <QTest>
#endif

using namespace CPlusPlus;
using namespace TextEditor;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

template<typename Statement> Statement *asControlStatement(AST *node)
{
    if constexpr (std::is_same_v<Statement, IfStatementAST>)
        return node->asIfStatement();
    if constexpr (std::is_same_v<Statement, WhileStatementAST>)
        return node->asWhileStatement();
    if constexpr (std::is_same_v<Statement, ForStatementAST>)
        return node->asForStatement();
    if constexpr (std::is_same_v<Statement, RangeBasedForStatementAST>)
        return node->asRangeBasedForStatement();
    if constexpr (std::is_same_v<Statement, DoStatementAST>)
        return node->asDoStatement();
    return nullptr;
}

template<typename Statement>
int triggerToken(const Statement *statement)
{
    if constexpr (std::is_same_v<Statement, IfStatementAST>)
        return statement->if_token;
    if constexpr (std::is_same_v<Statement, WhileStatementAST>)
        return statement->while_token;
    if constexpr (std::is_same_v<Statement, DoStatementAST>)
        return statement->do_token;
    if constexpr (std::is_same_v<Statement, ForStatementAST>
                  || std::is_same_v<Statement, RangeBasedForStatementAST>) {
        return statement->for_token;
    }
}

template<typename Statement>
int tokenToInsertOpeningBraceAfter(const Statement *statement)
{
    if constexpr (std::is_same_v<Statement, DoStatementAST>)
        return statement->do_token;
    return statement->rparen_token;
}

template<typename Statement> class AddBracesToControlStatementOp : public CppQuickFixOperation
{
public:
    AddBracesToControlStatementOp(const CppQuickFixInterface &interface,
                                  const QList<Statement *> &statements,
                                  StatementAST *elseStatement,
                                  int elseToken)
        : CppQuickFixOperation(interface, 0)
        , m_statements(statements), m_elseStatement(elseStatement), m_elseToken(elseToken)
    {
        setDescription(Tr::tr("Add Curly Braces"));
    }

    void perform() override
    {
        ChangeSet changes;
        for (Statement * const statement : m_statements) {
            const int start = currentFile()->endOf(tokenToInsertOpeningBraceAfter(statement));
            changes.insert(start, QLatin1String(" {"));
            if constexpr (std::is_same_v<Statement, DoStatementAST>) {
                const int end = currentFile()->startOf(statement->while_token);
                changes.insert(end, QLatin1String("} "));
            } else if constexpr (std::is_same_v<Statement, IfStatementAST>) {
                if (statement->else_statement) {
                    changes.insert(currentFile()->startOf(statement->else_token), "} ");
                } else {
                    changes.insert(currentFile()->endOf(statement->statement->lastToken() - 1),
                                   "\n}");
                }

            } else {
                const int end = currentFile()->endOf(statement->statement->lastToken() - 1);
                changes.insert(end, QLatin1String("\n}"));
            }
        }
        if (m_elseStatement) {
            changes.insert(currentFile()->endOf(m_elseToken), " {");
            changes.insert(currentFile()->endOf(m_elseStatement->lastToken() - 1), "\n}");
        }

        currentFile()->setChangeSet(changes);
        currentFile()->apply();
    }

private:
    const QList<Statement *> m_statements;
    StatementAST * const m_elseStatement;
    const int m_elseToken;
};

template<typename Statement>
bool checkControlStatementsHelper(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    Statement * const statement = asControlStatement<Statement>(interface.path().last());
    if (!statement)
        return false;

    QList<Statement *> statements;
    if (interface.isCursorOn(triggerToken(statement)) && statement->statement
        && !statement->statement->asCompoundStatement()) {
        statements << statement;
    }

    StatementAST *elseStmt = nullptr;
    int elseToken = 0;
    if constexpr (std::is_same_v<Statement, IfStatementAST>) {
        IfStatementAST *currentIfStmt = statement;
        for (elseStmt = currentIfStmt->else_statement, elseToken = currentIfStmt->else_token;
             elseStmt && (currentIfStmt = elseStmt->asIfStatement());
             elseStmt = currentIfStmt->else_statement, elseToken = currentIfStmt->else_token) {
            if (currentIfStmt->statement && !currentIfStmt->statement->asCompoundStatement())
                statements << currentIfStmt;
        }
        if (elseStmt && (elseStmt->asIfStatement() || elseStmt->asCompoundStatement())) {
            elseStmt = nullptr;
            elseToken = 0;
        }
    }

    if (!statements.isEmpty() || elseStmt)
        result << new AddBracesToControlStatementOp(interface, statements, elseStmt, elseToken);
    return true;
}

template<typename ...Statements>
void checkControlStatements(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    (... || checkControlStatementsHelper<Statements>(interface, result));
}

class MoveDeclarationOutOfIfOp: public CppQuickFixOperation
{
public:
    MoveDeclarationOutOfIfOp(const CppQuickFixInterface &interface)
        : CppQuickFixOperation(interface)
    {
        setDescription(Tr::tr("Move Declaration out of Condition"));

        reset();
    }

    void reset()
    {
        condition = mk.Condition();
        pattern = mk.IfStatement(condition);
    }

    void perform() override
    {
        ChangeSet changes;

        changes.copy(currentFile()->range(core), currentFile()->startOf(condition));

        int insertPos = currentFile()->startOf(pattern);
        changes.move(currentFile()->range(condition), insertPos);
        changes.insert(insertPos, QLatin1String(";\n"));

        currentFile()->setChangeSet(changes);
        currentFile()->apply();
    }

    ASTMatcher matcher;
    ASTPatternBuilder mk;
    ConditionAST *condition = nullptr;
    IfStatementAST *pattern = nullptr;
    CoreDeclaratorAST *core = nullptr;
};

class MoveDeclarationOutOfWhileOp: public CppQuickFixOperation
{
public:
    MoveDeclarationOutOfWhileOp(const CppQuickFixInterface &interface)
        : CppQuickFixOperation(interface)
    {
        setDescription(Tr::tr("Move Declaration out of Condition"));
        reset();
    }

    void reset()
    {
        condition = mk.Condition();
        pattern = mk.WhileStatement(condition);
    }

    void perform() override
    {
        ChangeSet changes;

        changes.insert(currentFile()->startOf(condition), QLatin1String("("));
        changes.insert(currentFile()->endOf(condition), QLatin1String(") != 0"));

        int insertPos = currentFile()->startOf(pattern);
        const int conditionStart = currentFile()->startOf(condition);
        changes.move(conditionStart, currentFile()->startOf(core), insertPos);
        changes.copy(currentFile()->range(core), insertPos);
        changes.insert(insertPos, QLatin1String(";\n"));

        currentFile()->setChangeSet(changes);
        currentFile()->apply();
    }

    ASTMatcher matcher;
    ASTPatternBuilder mk;
    ConditionAST *condition = nullptr;
    WhileStatementAST *pattern = nullptr;
    CoreDeclaratorAST *core = nullptr;
};

class SplitIfStatementOp: public CppQuickFixOperation
{
public:
    SplitIfStatementOp(const CppQuickFixInterface &interface, int priority,
                       IfStatementAST *pattern, BinaryExpressionAST *condition)
        : CppQuickFixOperation(interface, priority)
        , pattern(pattern)
        , condition(condition)
    {
        setDescription(Tr::tr("Split if Statement"));
    }

    void perform() override
    {
        const Token binaryToken = currentFile()->tokenAt(condition->binary_op_token);

        if (binaryToken.is(T_AMPER_AMPER))
            splitAndCondition();
        else
            splitOrCondition();
    }

    void splitAndCondition() const
    {
        ChangeSet changes;

        int startPos = currentFile()->startOf(pattern);
        changes.insert(startPos, QLatin1String("if ("));
        changes.move(currentFile()->range(condition->left_expression), startPos);
        changes.insert(startPos, QLatin1String(") {\n"));

        const int lExprEnd = currentFile()->endOf(condition->left_expression);
        changes.remove(lExprEnd, currentFile()->startOf(condition->right_expression));
        changes.insert(currentFile()->endOf(pattern), QLatin1String("\n}"));

        currentFile()->setChangeSet(changes);
        currentFile()->apply();
    }

    void splitOrCondition() const
    {
        ChangeSet changes;

        StatementAST *ifTrueStatement = pattern->statement;
        CompoundStatementAST *compoundStatement = ifTrueStatement->asCompoundStatement();

        int insertPos = currentFile()->endOf(ifTrueStatement);
        if (compoundStatement)
            changes.insert(insertPos, QLatin1String(" "));
        else
            changes.insert(insertPos, QLatin1String("\n"));
        changes.insert(insertPos, QLatin1String("else if ("));

        const int rExprStart = currentFile()->startOf(condition->right_expression);
        changes.move(rExprStart, currentFile()->startOf(pattern->rparen_token), insertPos);
        changes.insert(insertPos, QLatin1String(")"));

        const int rParenEnd = currentFile()->endOf(pattern->rparen_token);
        changes.copy(rParenEnd, currentFile()->endOf(pattern->statement), insertPos);

        const int lExprEnd = currentFile()->endOf(condition->left_expression);
        changes.remove(lExprEnd, currentFile()->startOf(condition->right_expression));

        currentFile()->setChangeSet(changes);
        currentFile()->apply();
    }

private:
    IfStatementAST *pattern;
    BinaryExpressionAST *condition;
};

class OptimizeForLoopOperation: public CppQuickFixOperation
{
public:
    OptimizeForLoopOperation(const CppQuickFixInterface &interface, const ForStatementAST *forAst,
                             const bool optimizePostcrement, const ExpressionAST *expression,
                             const FullySpecifiedType &type)
        : CppQuickFixOperation(interface)
        , m_forAst(forAst)
        , m_optimizePostcrement(optimizePostcrement)
        , m_expression(expression)
        , m_type(type)
    {
        setDescription(Tr::tr("Optimize for-Loop"));
    }

    void perform() override
    {
        QTC_ASSERT(m_forAst, return);

        const CppRefactoringFilePtr file = currentFile();
        ChangeSet change;

        // Optimize post (in|de)crement operator to pre (in|de)crement operator
        if (m_optimizePostcrement && m_forAst->expression) {
            PostIncrDecrAST *incrdecr = m_forAst->expression->asPostIncrDecr();
            if (incrdecr && incrdecr->base_expression && incrdecr->incr_decr_token) {
                change.flip(file->range(incrdecr->base_expression),
                            file->range(incrdecr->incr_decr_token));
            }
        }

        // Optimize Condition
        int renamePos = -1;
        if (m_expression) {
            QString varName = QLatin1String("total");

            if (file->textOf(m_forAst->initializer).length() == 1) {
                Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
                const QString typeAndName = oo.prettyType(m_type, varName);
                renamePos = file->endOf(m_forAst->initializer) - 1 + typeAndName.length();
                change.insert(file->endOf(m_forAst->initializer) - 1, // "-1" because of ";"
                              typeAndName + QLatin1String(" = ") + file->textOf(m_expression));
            } else {
                // Check if varName is already used
                if (DeclarationStatementAST *ds = m_forAst->initializer->asDeclarationStatement()) {
                    if (DeclarationAST *decl = ds->declaration) {
                        if (SimpleDeclarationAST *sdecl = decl->asSimpleDeclaration()) {
                            for (;;) {
                                bool match = false;
                                for (DeclaratorListAST *it = sdecl->declarator_list; it;
                                     it = it->next) {
                                    if (file->textOf(it->value->core_declarator) == varName) {
                                        varName += QLatin1Char('X');
                                        match = true;
                                        break;
                                    }
                                }
                                if (!match)
                                    break;
                            }
                        }
                    }
                }

                renamePos = file->endOf(m_forAst->initializer) + 1;
                change.insert(file->endOf(m_forAst->initializer) - 1, // "-1" because of ";"
                              QLatin1String(", ") + varName + QLatin1String(" = ")
                                  + file->textOf(m_expression));
            }

            ChangeSet::Range exprRange(file->startOf(m_expression), file->endOf(m_expression));
            change.replace(exprRange, varName);
        }

        file->setChangeSet(change);
        file->apply();

        // Select variable name and trigger symbol rename
        if (renamePos != -1) {
            QTextCursor c = file->cursor();
            c.setPosition(renamePos);
            editor()->setTextCursor(c);
            editor()->renameSymbolUnderCursor();
            c.select(QTextCursor::WordUnderCursor);
            editor()->setTextCursor(c);
        }
    }

private:
    const ForStatementAST *m_forAst;
    const bool m_optimizePostcrement;
    const ExpressionAST *m_expression;
    const FullySpecifiedType m_type;
};

/*!
  Replace
    if (Type name = foo()) {...}

  With
    Type name = foo();
    if (name) {...}

  Activates on: the name of the introduced variable
*/
class MoveDeclarationOutOfIf: public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject *createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const QList<AST *> &path = interface.path();
        using Ptr = QSharedPointer<MoveDeclarationOutOfIfOp>;
        Ptr op(new MoveDeclarationOutOfIfOp(interface));

        int index = path.size() - 1;
        for (; index != -1; --index) {
            if (IfStatementAST *statement = path.at(index)->asIfStatement()) {
                if (statement->match(op->pattern, &op->matcher) && op->condition->declarator) {
                    DeclaratorAST *declarator = op->condition->declarator;
                    op->core = declarator->core_declarator;
                    if (!op->core)
                        return;

                    if (interface.isCursorOn(op->core)) {
                        op->setPriority(index);
                        result.append(op);
                        return;
                    }

                    op->reset();
                }
            }
        }
    }
};

/*!
  Replace
    while (Type name = foo()) {...}

  With
    Type name;
    while ((name = foo()) != 0) {...}

  Activates on: the name of the introduced variable
*/
class MoveDeclarationOutOfWhile: public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject *createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const QList<AST *> &path = interface.path();
        QSharedPointer<MoveDeclarationOutOfWhileOp> op(new MoveDeclarationOutOfWhileOp(interface));

        int index = path.size() - 1;
        for (; index != -1; --index) {
            if (WhileStatementAST *statement = path.at(index)->asWhileStatement()) {
                if (statement->match(op->pattern, &op->matcher) && op->condition->declarator) {
                    DeclaratorAST *declarator = op->condition->declarator;
                    op->core = declarator->core_declarator;

                    if (!op->core)
                        return;

                    if (!declarator->equal_token)
                        return;

                    if (!declarator->initializer)
                        return;

                    if (interface.isCursorOn(op->core)) {
                        op->setPriority(index);
                        result.append(op);
                        return;
                    }

                    op->reset();
                }
            }
        }
    }
};

/*!
  Replace
     if (something && something_else) {
     }

  with
     if (something)
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
class SplitIfStatement: public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject *createTest() { return new QObject; }
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        IfStatementAST *pattern = nullptr;
        const QList<AST *> &path = interface.path();

        int index = path.size() - 1;
        for (; index != -1; --index) {
            AST *node = path.at(index);
            if (IfStatementAST *stmt = node->asIfStatement()) {
                pattern = stmt;
                break;
            }
        }

        if (!pattern || !pattern->statement)
            return;

        unsigned splitKind = 0;
        for (++index; index < path.size(); ++index) {
            AST *node = path.at(index);
            BinaryExpressionAST *condition = node->asBinaryExpression();
            if (!condition)
                return;

            Token binaryToken = interface.currentFile()->tokenAt(condition->binary_op_token);

            // only accept a chain of ||s or &&s - no mixing
            if (!splitKind) {
                splitKind = binaryToken.kind();
                if (splitKind != T_AMPER_AMPER && splitKind != T_PIPE_PIPE)
                    return;
                // we can't reliably split &&s in ifs with an else branch
                if (splitKind == T_AMPER_AMPER && pattern->else_statement)
                    return;
            } else if (splitKind != binaryToken.kind()) {
                return;
            }

            if (interface.isCursorOn(condition->binary_op_token)) {
                result << new SplitIfStatementOp(interface, index, pattern, condition);
                return;
            }
        }
    }
};

/*!
  Add curly braces to a control statement that doesn't already contain a
  compound statement. I.e.

  if (a)
      b;
  becomes
  if (a) {
      b;
  }

  Activates on: the keyword
*/
class AddBracesToControlStatement : public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject *createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        if (interface.path().isEmpty())
            return;
        checkControlStatements<IfStatementAST,
                               WhileStatementAST,
                               ForStatementAST,
                               RangeBasedForStatementAST,
                               DoStatementAST>(interface, result);
    }
};

/*!
  Optimizes a for loop to avoid permanent condition check and forces to use preincrement
  or predecrement operators in the expression of the for loop.
 */
class OptimizeForLoop : public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject *createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const QList<AST *> path = interface.path();
        ForStatementAST *forAst = nullptr;
        if (!path.isEmpty())
            forAst = path.last()->asForStatement();
        if (!forAst || !interface.isCursorOn(forAst))
            return;

        // Check for optimizing a postcrement
        const CppRefactoringFilePtr file = interface.currentFile();
        bool optimizePostcrement = false;
        if (forAst->expression) {
            if (PostIncrDecrAST *incrdecr = forAst->expression->asPostIncrDecr()) {
                const Token t = file->tokenAt(incrdecr->incr_decr_token);
                if (t.is(T_PLUS_PLUS) || t.is(T_MINUS_MINUS))
                    optimizePostcrement = true;
            }
        }

        // Check for optimizing condition
        bool optimizeCondition = false;
        FullySpecifiedType conditionType;
        ExpressionAST *conditionExpression = nullptr;
        if (forAst->initializer && forAst->condition) {
            if (BinaryExpressionAST *binary = forAst->condition->asBinaryExpression()) {
                // Get the expression against which we should evaluate
                IdExpressionAST *conditionId = binary->left_expression->asIdExpression();
                if (conditionId) {
                    conditionExpression = binary->right_expression;
                } else {
                    conditionId = binary->right_expression->asIdExpression();
                    conditionExpression = binary->left_expression;
                }

                if (conditionId && conditionExpression
                    && !(conditionExpression->asNumericLiteral()
                         || conditionExpression->asStringLiteral()
                         || conditionExpression->asIdExpression()
                         || conditionExpression->asUnaryExpression())) {
                    // Determine type of for initializer
                    FullySpecifiedType initializerType;
                    if (DeclarationStatementAST *stmt = forAst->initializer->asDeclarationStatement()) {
                        if (stmt->declaration) {
                            if (SimpleDeclarationAST *decl = stmt->declaration->asSimpleDeclaration()) {
                                if (decl->symbols) {
                                    if (Symbol *symbol = decl->symbols->value)
                                        initializerType = symbol->type();
                                }
                            }
                        }
                    }

                    // Determine type of for condition
                    TypeOfExpression typeOfExpression;
                    typeOfExpression.init(interface.semanticInfo().doc, interface.snapshot(),
                                          interface.context().bindings());
                    typeOfExpression.setExpandTemplates(true);
                    Scope *scope = file->scopeAt(conditionId->firstToken());
                    const QList<LookupItem> conditionItems = typeOfExpression(
                        conditionId, interface.semanticInfo().doc, scope);
                    if (!conditionItems.isEmpty())
                        conditionType = conditionItems.first().type();

                    if (conditionType.isValid()
                        && (file->textOf(forAst->initializer) == QLatin1String(";")
                            || initializerType == conditionType)) {
                        optimizeCondition = true;
                    }
                }
            }
        }

        if (optimizePostcrement || optimizeCondition) {
            result << new OptimizeForLoopOperation(interface, forAst, optimizePostcrement,
                                                   optimizeCondition ? conditionExpression : nullptr,
                                                   conditionType);
        }
    }
};

#ifdef WITH_TESTS
using namespace Tests;

class MoveDeclarationOutOfIfTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data()
    {
        QTest::addColumn<QByteArray>("original");
        QTest::addColumn<QByteArray>("expected");

        QTest::newRow("ifOnly")
            << QByteArray(
                   "void f()\n"
                   "{\n"
                   "    if (Foo *@foo = g())\n"
                   "        h();\n"
                   "}\n")
            << QByteArray(
                   "void f()\n"
                   "{\n"
                   "    Foo *foo = g();\n"
                   "    if (foo)\n"
                   "        h();\n"
                   "}\n");
        QTest::newRow("ifElse")
            << QByteArray(
                   "void f()\n"
                   "{\n"
                   "    if (Foo *@foo = g())\n"
                   "        h();\n"
                   "    else\n"
                   "        i();\n"
                   "}\n")
            << QByteArray(
                   "void f()\n"
                   "{\n"
                   "    Foo *foo = g();\n"
                   "    if (foo)\n"
                   "        h();\n"
                   "    else\n"
                   "        i();\n"
                   "}\n");

        QTest::newRow("MoveDeclarationOutOfIf_ifElseIf")
            << QByteArray(
                   "void f()\n"
                   "{\n"
                   "    if (Foo *foo = g()) {\n"
                   "        if (Bar *@bar = x()) {\n"
                   "            h();\n"
                   "            j();\n"
                   "        }\n"
                   "    } else {\n"
                   "        i();\n"
                   "    }\n"
                   "}\n")
            << QByteArray(
                   "void f()\n"
                   "{\n"
                   "    if (Foo *foo = g()) {\n"
                   "        Bar *bar = x();\n"
                   "        if (bar) {\n"
                   "            h();\n"
                   "            j();\n"
                   "        }\n"
                   "    } else {\n"
                   "        i();\n"
                   "    }\n"
                   "}\n");
    }

    void test()
    {
        QFETCH(QByteArray, original);
        QFETCH(QByteArray, expected);
        MoveDeclarationOutOfIf factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }
};

class MoveDeclarationOutOfWhileTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data()
    {
        QTest::addColumn<QByteArray>("original");
        QTest::addColumn<QByteArray>("expected");

        QTest::newRow("singleWhile")
            << QByteArray(
                   "void f()\n"
                   "{\n"
                   "    while (Foo *@foo = g())\n"
                   "        j();\n"
                   "}\n")
            << QByteArray(
                   "void f()\n"
                   "{\n"
                   "    Foo *foo;\n"
                   "    while ((foo = g()) != 0)\n"
                   "        j();\n"
                   "}\n");
        QTest::newRow("whileInWhile")
            << QByteArray(
                   "void f()\n"
                   "{\n"
                   "    while (Foo *foo = g()) {\n"
                   "        while (Bar *@bar = h()) {\n"
                   "            i();\n"
                   "            j();\n"
                   "        }\n"
                   "    }\n"
                   "}\n")
            << QByteArray(
                   "void f()\n"
                   "{\n"
                   "    while (Foo *foo = g()) {\n"
                   "        Bar *bar;\n"
                   "        while ((bar = h()) != 0) {\n"
                   "            i();\n"
                   "            j();\n"
                   "        }\n"
                   "    }\n"
                   "}\n"
                   );

    }

    void test()
    {
        QFETCH(QByteArray, original);
        QFETCH(QByteArray, expected);
        MoveDeclarationOutOfWhile factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }
};

class OptimizeForLoopTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data()
    {
        QTest::addColumn<QByteArray>("original");
        QTest::addColumn<QByteArray>("expected");

        // Check: optimize postcrement
        QTest::newRow("OptimizeForLoop_postcrement")
            << QByteArray("void foo() {f@or (int i = 0; i < 3; i++) {}}\n")
            << QByteArray("void foo() {for (int i = 0; i < 3; ++i) {}}\n");

        // Check: optimize condition
        QTest::newRow("OptimizeForLoop_condition")
            << QByteArray("void foo() {f@or (int i = 0; i < 3 + 5; ++i) {}}\n")
            << QByteArray("void foo() {for (int i = 0, total = 3 + 5; i < total; ++i) {}}\n");

        // Check: optimize fliped condition
        QTest::newRow("OptimizeForLoop_flipedCondition")
            << QByteArray("void foo() {f@or (int i = 0; 3 + 5 > i; ++i) {}}\n")
            << QByteArray("void foo() {for (int i = 0, total = 3 + 5; total > i; ++i) {}}\n");

        // Check: if "total" used, create other name.
        QTest::newRow("OptimizeForLoop_alterVariableName")
            << QByteArray("void foo() {f@or (int i = 0, total = 0; i < 3 + 5; ++i) {}}\n")
            << QByteArray("void foo() {for (int i = 0, total = 0, totalX = 3 + 5; i < totalX; ++i) {}}\n");

        // Check: optimize postcrement and condition
        QTest::newRow("OptimizeForLoop_optimizeBoth")
            << QByteArray("void foo() {f@or (int i = 0; i < 3 + 5; i++) {}}\n")
            << QByteArray("void foo() {for (int i = 0, total = 3 + 5; i < total; ++i) {}}\n");

        // Check: empty initializier
        QTest::newRow("OptimizeForLoop_emptyInitializer")
            << QByteArray("int i; void foo() {f@or (; i < 3 + 5; ++i) {}}\n")
            << QByteArray("int i; void foo() {for (int total = 3 + 5; i < total; ++i) {}}\n");

        // Check: wrong initializier type -> no trigger
        QTest::newRow("OptimizeForLoop_wrongInitializer")
            << QByteArray("int i; void foo() {f@or (double a = 0; i < 3 + 5; ++i) {}}\n")
            << QByteArray();

        // Check: No trigger when numeric
        QTest::newRow("OptimizeForLoop_noTriggerNumeric1")
            << QByteArray("void foo() {fo@r (int i = 0; i < 3; ++i) {}}\n")
            << QByteArray();

        // Check: No trigger when numeric
        QTest::newRow("OptimizeForLoop_noTriggerNumeric2")
            << QByteArray("void foo() {fo@r (int i = 0; i < -3; ++i) {}}\n")
            << QByteArray();
    }

    void test()
    {
        QFETCH(QByteArray, original);
        QFETCH(QByteArray, expected);
        OptimizeForLoop factory;
        QuickFixOperationTest(singleDocument(original, expected), &factory);
    }
};

class AddBracesToControlStatementTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data()
    {
        QTest::addColumn<QByteArray>("original");
        QTest::addColumn<QByteArray>("expected");

        QByteArray original = R"delim(
void MyObject::f()
{
    @if (true)
        emit mySig();
})delim";
        QByteArray expected = R"delim(
void MyObject::f()
{
    if (true) {
        emit mySig();
    }
})delim";
        QTest::newRow("if") << original << expected;

        original = R"delim(
void MyObject::f()
{
    @if (true)
        emit mySig();
    else
        emit otherSig();
})delim";
        expected = R"delim(
void MyObject::f()
{
    @if (true) {
        emit mySig();
    } else {
        emit otherSig();
    }
})delim";
        QTest::newRow("if with one else, unbraced") << original << expected;

        original = R"delim(
void MyObject::f()
{
    @if (true) {
        emit mySig();
    } else
        emit otherSig();
})delim";
        expected = R"delim(
void MyObject::f()
{
    @if (true) {
        emit mySig();
    } else {
        emit otherSig();
    }
})delim";
        QTest::newRow("if with one else, if braced") << original << expected;

        original = R"delim(
void MyObject::f()
{
    @if (true)
        emit mySig();
    else {
        emit otherSig();
    }
})delim";
        expected = R"delim(
void MyObject::f()
{
    @if (true) {
        emit mySig();
    } else {
        emit otherSig();
    }
})delim";
        QTest::newRow("if with one else, else braced") << original << expected;

        original = R"delim(
void MyObject::f()
{
    @if (true) {
        emit mySig();
    } else {
        emit otherSig();
    }
})delim";
        expected.clear();
        QTest::newRow("if with one else, both braced") << original << expected;

        original = R"delim(
void MyObject::f()
{
    @if (x == 1)
        emit sig1();
    else if (x == 2)
        emit sig2();
})delim";
        expected = R"delim(
void MyObject::f()
{
    if (x == 1) {
        emit sig1();
    } else if (x == 2) {
        emit sig2();
    }
})delim";
        QTest::newRow("if-else chain without final else, unbraced") << original << expected;

        original = R"delim(
void MyObject::f()
{
    @if (x == 1) {
        emit sig1();
    } else if (x == 2)
        emit sig2();
})delim";
        expected = R"delim(
void MyObject::f()
{
    if (x == 1) {
        emit sig1();
    } else if (x == 2) {
        emit sig2();
    }
})delim";
        QTest::newRow("if-else chain without final else, partially braced 1") << original << expected;

        original = R"delim(
void MyObject::f()
{
    @if (x == 1)
        emit sig1();
    else if (x == 2) {
        emit sig2();
    }
})delim";
        expected = R"delim(
void MyObject::f()
{
    if (x == 1) {
        emit sig1();
    } else if (x == 2) {
        emit sig2();
    }
})delim";
        QTest::newRow("if-else chain without final else, partially braced 2") << original << expected;

        original = R"delim(
void MyObject::f()
{
    @if (x == 1) {
        emit sig1();
    } else if (x == 2) {
        emit sig2();
    }
})delim";
        expected.clear();
        QTest::newRow("if-else chain without final else, fully braced") << original << expected;

        original = R"delim(
void MyObject::f()
{
    @if (x == 1)
        emit sig1();
    else if (x == 2)
        emit sig2();
    else if (x == 3)
        emit sig3();
    else
        emit otherSig();
})delim";
        expected = R"delim(
void MyObject::f()
{
    if (x == 1) {
        emit sig1();
    } else if (x == 2) {
        emit sig2();
    } else if (x == 3) {
        emit sig3();
    } else {
        emit otherSig();
    }
})delim";
        QTest::newRow("if-else chain, unbraced") << original << expected;

        original = R"delim(
void MyObject::f()
{
    @if (x == 1) {
        emit sig1();
    } else if (x == 2)
        emit sig2();
    else if (x == 3)
        emit sig3();
    else
        emit otherSig();
})delim";
        expected = R"delim(
void MyObject::f()
{
    if (x == 1) {
        emit sig1();
    } else if (x == 2) {
        emit sig2();
    } else if (x == 3) {
        emit sig3();
    } else {
        emit otherSig();
    }
})delim";
        QTest::newRow("if-else chain, partially braced 1") << original << expected;

        original = R"delim(
void MyObject::f()
{
    @if (x == 1)
        emit sig1();
    else if (x == 2) {
        emit sig2();
    } else if (x == 3)
        emit sig3();
    else
        emit otherSig();
})delim";
        expected = R"delim(
void MyObject::f()
{
    if (x == 1) {
        emit sig1();
    } else if (x == 2) {
        emit sig2();
    } else if (x == 3) {
        emit sig3();
    } else {
        emit otherSig();
    }
})delim";
        QTest::newRow("if-else chain, partially braced 2") << original << expected;

        original = R"delim(
void MyObject::f()
{
    @if (x == 1)
        emit sig1();
    else if (x == 2)
        emit sig2();
    else if (x == 3) {
        emit sig3();
    } else
        emit otherSig();
})delim";
        expected = R"delim(
void MyObject::f()
{
    if (x == 1) {
        emit sig1();
    } else if (x == 2) {
        emit sig2();
    } else if (x == 3) {
        emit sig3();
    } else {
        emit otherSig();
    }
})delim";
        QTest::newRow("if-else chain, partially braced 3") << original << expected;

        original = R"delim(
void MyObject::f()
{
    @if (x == 1)
        emit sig1();
    else if (x == 2)
        emit sig2();
    else if (x == 3)
        emit sig3();
    else {
        emit otherSig();
    }
})delim";
        expected = R"delim(
void MyObject::f()
{
    if (x == 1) {
        emit sig1();
    } else if (x == 2) {
        emit sig2();
    } else if (x == 3) {
        emit sig3();
    } else {
        emit otherSig();
    }
})delim";
        QTest::newRow("if-else chain, partially braced 4") << original << expected;

        original = R"delim(
void MyObject::f()
{
    @if (x == 1) {
        emit sig1();
    } else if (x == 2) {
        emit sig2();
    } else if (x == 3) {
        emit sig3();
    } else {
        emit otherSig();
    }
})delim";
        expected.clear();
        QTest::newRow("if-else chain, fully braced") << original << expected;

        original = R"delim(
void MyObject::f()
{
    @while (true)
        emit mySig();
})delim";
        expected = R"delim(
void MyObject::f()
{
    while (true) {
        emit mySig();
    }
})delim";
        QTest::newRow("while") << original << expected;

        original = R"delim(
void MyObject::f()
{
    @for (int i = 0; i < 10; ++i)
        emit mySig();
})delim";
        expected = R"delim(
void MyObject::f()
{
    for (int i = 0; i < 10; ++i) {
        emit mySig();
    }
})delim";
        QTest::newRow("for") << original << expected;

        original = R"delim(
void MyObject::f()
{
    @for (int i : list)
        emit mySig();
})delim";
        expected = R"delim(
void MyObject::f()
{
    for (int i : list) {
        emit mySig();
    }
})delim";
        QTest::newRow("range-based for") << original << expected;

        original = R"delim(
void MyObject::f()
{
    @do
        emit mySig();
    while (true);
})delim";
        expected = R"delim(
void MyObject::f()
{
    do {
        emit mySig();
    } while (true);
})delim";
        QTest::newRow("do") << original << expected;

        original = R"delim(
void MyObject::f()
{
    @do {
        emit mySig();
    } while (true);
})delim";
        expected.clear();
        QTest::newRow("already has braces") << original << expected;
    }

    void test()
    {
        QFETCH(QByteArray, original);
        QFETCH(QByteArray, expected);

        AddBracesToControlStatement factory;
        QuickFixOperationTest({CppTestDocument::create("file.cpp", original, expected)}, &factory);
    }
};

QObject *MoveDeclarationOutOfIf::createTest() { return new MoveDeclarationOutOfIfTest; }
QObject *MoveDeclarationOutOfWhile::createTest() { return new MoveDeclarationOutOfWhileTest; }
QObject *OptimizeForLoop::createTest() { return new OptimizeForLoopTest; }
QObject *AddBracesToControlStatement::createTest() { return new AddBracesToControlStatementTest; }

#endif // WITH_TESTS
} // namespace

void registerRewriteControlStatementQuickfixes()
{
    CppQuickFixFactory::registerFactory<AddBracesToControlStatement>();
    CppQuickFixFactory::registerFactory<MoveDeclarationOutOfIf>();
    CppQuickFixFactory::registerFactory<MoveDeclarationOutOfWhile>();
    CppQuickFixFactory::registerFactory<OptimizeForLoop>();
    CppQuickFixFactory::registerFactory<SplitIfStatement>();
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <rewritecontrolstatements.moc>
#endif
