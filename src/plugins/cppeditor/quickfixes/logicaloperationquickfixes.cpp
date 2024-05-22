// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "logicaloperationquickfixes.h"

#include "../cppeditortr.h"
#include "../cpprefactoringchanges.h"
#include "cppquickfix.h"

using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

class FlipLogicalOperandsOp : public CppQuickFixOperation
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

    QString description() const override
    {
        if (replacement.isEmpty())
            return Tr::tr("Swap Operands");
        else
            return Tr::tr("Rewrite Using %1").arg(replacement);
    }

    void perform() override
    {
        ChangeSet changes;
        changes.flip(currentFile()->range(binary->left_expression),
                     currentFile()->range(binary->right_expression));
        if (!replacement.isEmpty())
            changes.replace(currentFile()->range(binary->binary_op_token), replacement);

        currentFile()->setChangeSet(changes);
        currentFile()->apply();
    }

private:
    BinaryExpressionAST *binary;
    QString replacement;
};

class InverseLogicalComparisonOp : public CppQuickFixOperation
{
public:
    InverseLogicalComparisonOp(const CppQuickFixInterface &interface,
                               int priority,
                               BinaryExpressionAST *binary,
                               Kind invertToken)
        : CppQuickFixOperation(interface, priority)
        , binary(binary)
    {
        Token tok;
        tok.f.kind = invertToken;
        replacement = QLatin1String(tok.spell());

        // check for enclosing nested expression
        if (priority - 1 >= 0)
            nested = interface.path()[priority - 1]->asNestedExpression();

        // check for ! before parentheses
        if (nested && priority - 2 >= 0) {
            negation = interface.path()[priority - 2]->asUnaryExpression();
            if (negation && !interface.currentFile()->tokenAt(negation->unary_op_token).is(T_EXCLAIM))
                negation = nullptr;
        }
    }

    QString description() const override
    {
        return Tr::tr("Rewrite Using %1").arg(replacement);
    }

    void perform() override
    {
        ChangeSet changes;
        if (negation) {
            // can't remove parentheses since that might break precedence
            changes.remove(currentFile()->range(negation->unary_op_token));
        } else if (nested) {
            changes.insert(currentFile()->startOf(nested), QLatin1String("!"));
        } else {
            changes.insert(currentFile()->startOf(binary), QLatin1String("!("));
            changes.insert(currentFile()->endOf(binary), QLatin1String(")"));
        }
        changes.replace(currentFile()->range(binary->binary_op_token), replacement);
        currentFile()->setChangeSet(changes);
        currentFile()->apply();
    }

private:
    BinaryExpressionAST *binary = nullptr;
    NestedExpressionAST *nested = nullptr;
    UnaryExpressionAST *negation = nullptr;

    QString replacement;
};

class RewriteLogicalAndOp : public CppQuickFixOperation
{
public:
    std::shared_ptr<ASTPatternBuilder> mk;
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

    void perform() override
    {
        ChangeSet changes;
        changes.replace(currentFile()->range(pattern->binary_op_token), QLatin1String("||"));
        changes.remove(currentFile()->range(left->unary_op_token));
        changes.remove(currentFile()->range(right->unary_op_token));
        const int start = currentFile()->startOf(pattern);
        const int end = currentFile()->endOf(pattern);
        changes.insert(start, QLatin1String("!("));
        changes.insert(end, QLatin1String(")"));

        currentFile()->setChangeSet(changes);
        currentFile()->apply();
    }
};

/*!
  Rewrite
    a op b

  As
    b flipop a

  Activates on: <= < > >= == != && ||
*/
class FlipLogicalOperands : public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject *createTest() { return new QObject; }
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        const QList<AST *> &path = interface.path();
        if (path.isEmpty())
            return;
        CppRefactoringFilePtr file = interface.currentFile();

        int index = path.size() - 1;
        BinaryExpressionAST *binary = path.at(index)->asBinaryExpression();
        if (!binary)
            return;
        if (!interface.isCursorOn(binary->binary_op_token))
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

        result << new FlipLogicalOperandsOp(interface, index, binary, replacement);
    }
};

/*!
  Rewrite
    a op b -> !(a invop b)
    (a op b) -> !(a invop b)
    !(a op b) -> (a invob b)

  Activates on: <= < > >= == !=
*/
class InverseLogicalComparison : public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject *createTest() { return new QObject; }
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        CppRefactoringFilePtr file = interface.currentFile();

        const QList<AST *> &path = interface.path();
        if (path.isEmpty())
            return;
        int index = path.size() - 1;
        BinaryExpressionAST *binary = path.at(index)->asBinaryExpression();
        if (!binary)
            return;
        if (!interface.isCursorOn(binary->binary_op_token))
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

        result << new InverseLogicalComparisonOp(interface, index, binary, invertToken);
    }
};

/*!
  Rewrite
    !a && !b

  As
    !(a || b)

  Activates on: &&
*/
class RewriteLogicalAnd : public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject *createTest() { return new QObject; }
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        BinaryExpressionAST *expression = nullptr;
        const QList<AST *> &path = interface.path();
        CppRefactoringFilePtr file = interface.currentFile();

        int index = path.size() - 1;
        for (; index != -1; --index) {
            expression = path.at(index)->asBinaryExpression();
            if (expression)
                break;
        }

        if (!expression)
            return;

        if (!interface.isCursorOn(expression->binary_op_token))
            return;

        QSharedPointer<RewriteLogicalAndOp> op(new RewriteLogicalAndOp(interface));

        ASTMatcher matcher;

        if (expression->match(op->pattern, &matcher) &&
            file->tokenAt(op->pattern->binary_op_token).is(T_AMPER_AMPER) &&
            file->tokenAt(op->left->unary_op_token).is(T_EXCLAIM) &&
            file->tokenAt(op->right->unary_op_token).is(T_EXCLAIM)) {
            op->setDescription(Tr::tr("Rewrite Condition Using ||"));
            op->setPriority(index);
            result.append(op);
        }
    }
};

} // namespace

void registerLogicalOperationQuickfixes()
{
    CppQuickFixFactory::registerFactory<FlipLogicalOperands>();
    CppQuickFixFactory::registerFactory<InverseLogicalComparison>();
    CppQuickFixFactory::registerFactory<RewriteLogicalAnd>();
}

} // namespace CppEditor::Internal
