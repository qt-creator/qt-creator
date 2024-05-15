// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppquickfixes.h"

#include "../baseeditordocumentprocessor.h"
#include "../cppcodestylesettings.h"
#include "../cppeditordocument.h"
#include "../cppeditortr.h"
#include "../cppeditorwidget.h"
#include "../cppfunctiondecldeflink.h"
#include "../cpplocatordata.h"
#include "../cpppointerdeclarationformatter.h"
#include "../cpprefactoringchanges.h"
#include "../cpptoolsreuse.h"
#include "../includeutils.h"
#include "../indexitem.h"
#include "../insertionpointlocator.h"
#include "../symbolfinder.h"
#include "cppcodegenerationquickfixes.h"
#include "cppinsertvirtualmethods.h"
#include "cppquickfixassistant.h"
#include "cppquickfixhelpers.h"
#include "cppquickfixprojectsettings.h"
#include "convertqt4connect.h"
#include "moveclasstoownfile.h"
#include "movefunctiondefinition.h"
#include "removeusingnamespace.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <cplusplus/ASTPath.h>
#include <cplusplus/CPlusPlusForwardDeclarations.h>
#include <cplusplus/CppRewriter.h>
#include <cplusplus/declarationcomments.h>
#include <cplusplus/NamePrettyPrinter.h>
#include <cplusplus/TypeOfExpression.h>
#include <cplusplus/TypePrettyPrinter.h>

#include <projectexplorer/editorconfiguration.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/projectmanager.h>

#include <texteditor/tabsettings.h>

#include <utils/algorithm.h>
#include <utils/basetreeview.h>
#include <utils/codegeneration.h>
#include <utils/layoutbuilder.h>
#include <utils/fancylineedit.h>
#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>
#include <utils/treeviewcombobox.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QHash>
#include <QHeaderView>
#include <QInputDialog>
#include <QMimeData>
#include <QPair>
#include <QProxyStyle>
#include <QPushButton>
#include <QRegularExpression>
#include <QSharedPointer>
#include <QStack>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QTextCodec>
#include <QTextCursor>
#include <QVBoxLayout>

#include <bitset>
#include <cctype>
#include <functional>
#include <limits>
#include <vector>

using namespace CPlusPlus;
using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace CppEditor {

static QList<CppQuickFixFactory *> g_cppQuickFixFactories;

CppQuickFixFactory::CppQuickFixFactory()
{
    g_cppQuickFixFactories.append(this);
}

CppQuickFixFactory::~CppQuickFixFactory()
{
    g_cppQuickFixFactories.removeOne(this);
}

void CppQuickFixFactory::match(const Internal::CppQuickFixInterface &interface,
                               QuickFixOperations &result)
{
    if (m_clangdReplacement) {
        if (const auto clangdVersion = CppModelManager::usesClangd(
                    interface.currentFile()->editor()->textDocument());
                clangdVersion && clangdVersion >= m_clangdReplacement) {
            return;
        }
    }

    doMatch(interface, result);
}

const QList<CppQuickFixFactory *> &CppQuickFixFactory::cppQuickFixFactories()
{
    return g_cppQuickFixFactories;
}

namespace Internal {

// In the following anonymous namespace all functions are collected, which could be of interest for
// different quick fixes.
namespace {

enum DefPos {
    DefPosInsideClass,
    DefPosOutsideClass,
    DefPosImplementationFile
};


inline bool isQtStringLiteral(const QByteArray &id)
{
    return id == "QLatin1String" || id == "QLatin1Literal" || id == "QStringLiteral"
           || id == "QByteArrayLiteral";
}

inline bool isQtStringTranslation(const QByteArray &id)
{
    return id == "tr" || id == "trUtf8" || id == "translate" || id == "QT_TRANSLATE_NOOP";
}

QString nameString(const NameAST *name)
{
    return CppCodeStyleSettings::currentProjectCodeStyleOverview().prettyName(name->name);
}

static FullySpecifiedType typeOfExpr(const ExpressionAST *expr,
                                     const CppRefactoringFilePtr &file,
                                     const Snapshot &snapshot,
                                     const LookupContext &context)
{
    TypeOfExpression typeOfExpression;
    typeOfExpression.init(file->cppDocument(), snapshot, context.bindings());
    Scope *scope = file->scopeAt(expr->firstToken());
    const QList<LookupItem> result = typeOfExpression(
        file->textOf(expr).toUtf8(), scope, TypeOfExpression::Preprocess);
    if (result.isEmpty())
        return {};

    SubstitutionEnvironment env;
    env.setContext(context);
    env.switchScope(result.first().scope());
    ClassOrNamespace *con = typeOfExpression.context().lookupType(scope);
    if (!con)
        con = typeOfExpression.context().globalNamespace();
    UseMinimalNames q(con);
    env.enter(&q);

    Control *control = context.bindings()->control().get();
    return rewriteType(result.first().type(), &env, control);
}

// FIXME: Needs to consider the scope at the insertion site.
QString declFromExpr(const TypeOrExpr &typeOrExpr, const CallAST *call, const NameAST *varName,
                     const Snapshot &snapshot, const LookupContext &context,
                     const CppRefactoringFilePtr &file, bool makeConst)
{
    const auto getTypeFromUser = [varName, call]() -> QString {
        if (call)
            return {};
        const QString typeFromUser = QInputDialog::getText(Core::ICore::dialogParent(),
                                                           Tr::tr("Provide the type"),
                                                           Tr::tr("Data type:"), QLineEdit::Normal);
        if (!typeFromUser.isEmpty())
            return typeFromUser + ' ' + nameString(varName);
        return {};
    };
    const auto getTypeOfExpr = [&](const ExpressionAST *expr) -> FullySpecifiedType {
        return typeOfExpr(expr, file, snapshot, context);
    };

    const Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
    const FullySpecifiedType type = std::holds_alternative<FullySpecifiedType>(typeOrExpr)
        ? std::get<FullySpecifiedType>(typeOrExpr)
        : getTypeOfExpr(std::get<const ExpressionAST *>(typeOrExpr));
    if (!call)
        return type.isValid() ? oo.prettyType(type, varName->name) : getTypeFromUser();

    Function func(file->cppDocument()->translationUnit(), 0, varName->name);
    func.setConst(makeConst);
    for (ExpressionListAST *it = call->expression_list; it; it = it->next) {
        Argument * const arg = new Argument(nullptr, 0, nullptr);
        arg->setType(getTypeOfExpr(it->value));
        func.addMember(arg);
    }
    return oo.prettyType(type) + ' ' + oo.prettyType(func.type(), varName->name);
}

} // anonymous namespace

namespace {

class InverseLogicalComparisonOp: public CppQuickFixOperation
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
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());

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
    BinaryExpressionAST *binary = nullptr;
    NestedExpressionAST *nested = nullptr;
    UnaryExpressionAST *negation = nullptr;

    QString replacement;
};

} // anonymous namespace

void InverseLogicalComparison::doMatch(const CppQuickFixInterface &interface,
                                       QuickFixOperations &result)
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

namespace {

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

    QString description() const override
    {
        if (replacement.isEmpty())
            return Tr::tr("Swap Operands");
        else
            return Tr::tr("Rewrite Using %1").arg(replacement);
    }

    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());

        ChangeSet changes;
        changes.flip(currentFile->range(binary->left_expression),
                     currentFile->range(binary->right_expression));
        if (!replacement.isEmpty())
            changes.replace(currentFile->range(binary->binary_op_token), replacement);

        currentFile->setChangeSet(changes);
        currentFile->apply();
    }

private:
    BinaryExpressionAST *binary;
    QString replacement;
};

} // anonymous namespace

void FlipLogicalOperands::doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result)
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

namespace {

class RewriteLogicalAndOp: public CppQuickFixOperation
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
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());

        ChangeSet changes;
        changes.replace(currentFile->range(pattern->binary_op_token), QLatin1String("||"));
        changes.remove(currentFile->range(left->unary_op_token));
        changes.remove(currentFile->range(right->unary_op_token));
        const int start = currentFile->startOf(pattern);
        const int end = currentFile->endOf(pattern);
        changes.insert(start, QLatin1String("!("));
        changes.insert(end, QLatin1String(")"));

        currentFile->setChangeSet(changes);
        currentFile->apply();
    }
};

} // anonymous namespace

void RewriteLogicalAnd::doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result)
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

static bool checkDeclarationForSplit(SimpleDeclarationAST *declaration)
{
    if (!declaration->semicolon_token)
        return false;

    if (!declaration->decl_specifier_list)
        return false;

    for (SpecifierListAST *it = declaration->decl_specifier_list; it; it = it->next) {
        SpecifierAST *specifier = it->value;
        if (specifier->asEnumSpecifier() || specifier->asClassSpecifier())
            return false;
    }

    return declaration->declarator_list && declaration->declarator_list->next;
}

namespace {

class SplitSimpleDeclarationOp: public CppQuickFixOperation
{
public:
    SplitSimpleDeclarationOp(const CppQuickFixInterface &interface, int priority,
                             SimpleDeclarationAST *decl)
        : CppQuickFixOperation(interface, priority)
        , declaration(decl)
    {
        setDescription(Tr::tr("Split Declaration"));
    }

    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());

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
        currentFile->apply();
    }

private:
    SimpleDeclarationAST *declaration;
};

} // anonymous namespace

void SplitSimpleDeclaration::doMatch(const CppQuickFixInterface &interface,
                                     QuickFixOperations &result)
{
    CoreDeclaratorAST *core_declarator = nullptr;
    const QList<AST *> &path = interface.path();
    CppRefactoringFilePtr file = interface.currentFile();
    const int cursorPosition = file->cursor().selectionStart();

    for (int index = path.size() - 1; index != -1; --index) {
        AST *node = path.at(index);

        if (CoreDeclaratorAST *coreDecl = node->asCoreDeclarator()) {
            core_declarator = coreDecl;
        } else if (SimpleDeclarationAST *simpleDecl = node->asSimpleDeclaration()) {
            if (checkDeclarationForSplit(simpleDecl)) {
                SimpleDeclarationAST *declaration = simpleDecl;

                const int startOfDeclSpecifier = file->startOf(declaration->decl_specifier_list->firstToken());
                const int endOfDeclSpecifier = file->endOf(declaration->decl_specifier_list->lastToken() - 1);

                if (cursorPosition >= startOfDeclSpecifier && cursorPosition <= endOfDeclSpecifier) {
                    // the AST node under cursor is a specifier.
                    result << new SplitSimpleDeclarationOp(interface, index, declaration);
                    return;
                }

                if (core_declarator && interface.isCursorOn(core_declarator)) {
                    // got a core-declarator under the text cursor.
                    result << new SplitSimpleDeclarationOp(interface, index, declaration);
                    return;
                }
            }

            return;
        }
    }
}

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
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());

        ChangeSet changes;
        for (Statement * const statement : m_statements) {
            const int start = currentFile->endOf(tokenToInsertOpeningBraceAfter(statement));
            changes.insert(start, QLatin1String(" {"));
            if constexpr (std::is_same_v<Statement, DoStatementAST>) {
                const int end = currentFile->startOf(statement->while_token);
                changes.insert(end, QLatin1String("} "));
            } else if constexpr (std::is_same_v<Statement, IfStatementAST>) {
                 if (statement->else_statement) {
                     changes.insert(currentFile->startOf(statement->else_token), "} ");
                 } else {
                     changes.insert(currentFile->endOf(statement->statement->lastToken() - 1),
                                    "\n}");
                 }

            } else {
                const int end = currentFile->endOf(statement->statement->lastToken() - 1);
                changes.insert(end, QLatin1String("\n}"));
            }
        }
        if (m_elseStatement) {
            changes.insert(currentFile->endOf(m_elseToken), " {");
            changes.insert(currentFile->endOf(m_elseStatement->lastToken() - 1), "\n}");
        }

        currentFile->setChangeSet(changes);
        currentFile->apply();
    }

private:
    const QList<Statement *> m_statements;
    StatementAST * const m_elseStatement;
    const int m_elseToken;
};

} // anonymous namespace

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

void AddBracesToControlStatement::doMatch(const CppQuickFixInterface &interface,
                                          QuickFixOperations &result)
{
    if (interface.path().isEmpty())
        return;
    checkControlStatements<IfStatementAST,
                           WhileStatementAST,
                           ForStatementAST,
                           RangeBasedForStatementAST,
                           DoStatementAST>(interface, result);
}

namespace {

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
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());

        ChangeSet changes;

        changes.copy(currentFile->range(core), currentFile->startOf(condition));

        int insertPos = currentFile->startOf(pattern);
        changes.move(currentFile->range(condition), insertPos);
        changes.insert(insertPos, QLatin1String(";\n"));

        currentFile->setChangeSet(changes);
        currentFile->apply();
    }

    ASTMatcher matcher;
    ASTPatternBuilder mk;
    ConditionAST *condition = nullptr;
    IfStatementAST *pattern = nullptr;
    CoreDeclaratorAST *core = nullptr;
};

} // anonymous namespace

void MoveDeclarationOutOfIf::doMatch(const CppQuickFixInterface &interface,
                                     QuickFixOperations &result)
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

namespace {

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
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());

        ChangeSet changes;

        changes.insert(currentFile->startOf(condition), QLatin1String("("));
        changes.insert(currentFile->endOf(condition), QLatin1String(") != 0"));

        int insertPos = currentFile->startOf(pattern);
        const int conditionStart = currentFile->startOf(condition);
        changes.move(conditionStart, currentFile->startOf(core), insertPos);
        changes.copy(currentFile->range(core), insertPos);
        changes.insert(insertPos, QLatin1String(";\n"));

        currentFile->setChangeSet(changes);
        currentFile->apply();
    }

    ASTMatcher matcher;
    ASTPatternBuilder mk;
    ConditionAST *condition = nullptr;
    WhileStatementAST *pattern = nullptr;
    CoreDeclaratorAST *core = nullptr;
};

} // anonymous namespace

void MoveDeclarationOutOfWhile::doMatch(const CppQuickFixInterface &interface,
                                        QuickFixOperations &result)
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

namespace {

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
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());

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
        currentFile->apply();
    }

private:
    IfStatementAST *pattern;
    BinaryExpressionAST *condition;
};

} // anonymous namespace

void SplitIfStatement::doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result)
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

/* Analze a string/character literal like "x", QLatin1String("x") and return the literal
 * (StringLiteral or NumericLiteral for characters) and its type
 * and the enclosing function (QLatin1String, tr...) */

enum StringLiteralType { TypeString, TypeObjCString, TypeChar, TypeNone };

enum ActionFlags {
    EncloseInQLatin1CharAction = 0x1,
    EncloseInQLatin1StringAction = 0x2,
    EncloseInQStringLiteralAction = 0x4,
    EncloseInQByteArrayLiteralAction = 0x8,
    EncloseActionMask = EncloseInQLatin1CharAction | EncloseInQLatin1StringAction
                        | EncloseInQStringLiteralAction | EncloseInQByteArrayLiteralAction,
    TranslateTrAction = 0x10,
    TranslateQCoreApplicationAction = 0x20,
    TranslateNoopAction = 0x40,
    TranslationMask = TranslateTrAction | TranslateQCoreApplicationAction | TranslateNoopAction,
    RemoveObjectiveCAction = 0x100,
    ConvertEscapeSequencesToCharAction = 0x200,
    ConvertEscapeSequencesToStringAction = 0x400,
    SingleQuoteAction = 0x800,
    DoubleQuoteAction = 0x1000
};

/* Convert single-character string literals into character literals with some
 * special cases "a" --> 'a', "'" --> '\'', "\n" --> '\n', "\"" --> '"'. */
static QByteArray stringToCharEscapeSequences(const QByteArray &content)
{
    if (content.size() == 1)
        return content.at(0) == '\'' ? QByteArray("\\'") : content;
    if (content.size() == 2 && content.at(0) == '\\')
        return content == "\\\"" ? QByteArray(1, '"') : content;
    return QByteArray();
}

/* Convert character literal into a string literal with some special cases
 * 'a' -> "a", '\n' -> "\n", '\'' --> "'", '"' --> "\"". */
static QByteArray charToStringEscapeSequences(const QByteArray &content)
{
    if (content.size() == 1)
        return content.at(0) == '"' ? QByteArray("\\\"") : content;
    if (content.size() == 2)
        return content == "\\'" ? QByteArray("'") : content;
    return QByteArray();
}

static QString msgQtStringLiteralDescription(const QString &replacement)
{
    return Tr::tr("Enclose in %1(...)").arg(replacement);
}

static QString stringLiteralReplacement(unsigned actions)
{
    if (actions & EncloseInQLatin1CharAction)
        return QLatin1String("QLatin1Char");
    if (actions & EncloseInQLatin1StringAction)
        return QLatin1String("QLatin1String");
    if (actions & EncloseInQStringLiteralAction)
        return QLatin1String("QStringLiteral");
    if (actions & EncloseInQByteArrayLiteralAction)
        return QLatin1String("QByteArrayLiteral");
    if (actions & TranslateTrAction)
        return QLatin1String("tr");
    if (actions & TranslateQCoreApplicationAction)
        return QLatin1String("QCoreApplication::translate");
    if (actions & TranslateNoopAction)
        return QLatin1String("QT_TRANSLATE_NOOP");
    return QString();
}

static ExpressionAST *analyzeStringLiteral(const QList<AST *> &path,
                                           const CppRefactoringFilePtr &file, StringLiteralType *type,
                                           QByteArray *enclosingFunction = nullptr,
                                           CallAST **enclosingFunctionCall = nullptr)
{
    *type = TypeNone;
    if (enclosingFunction)
        enclosingFunction->clear();
    if (enclosingFunctionCall)
        *enclosingFunctionCall = nullptr;

    if (path.isEmpty())
        return nullptr;

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

namespace {

/// Operation performs the operations of type ActionFlags passed in as actions.
class WrapStringLiteralOp : public CppQuickFixOperation
{
public:
    WrapStringLiteralOp(const CppQuickFixInterface &interface, int priority,
                        unsigned actions, const QString &description, ExpressionAST *literal,
                        const QString &translationContext = QString())
        : CppQuickFixOperation(interface, priority), m_actions(actions), m_literal(literal),
          m_translationContext(translationContext)
    {
        setDescription(description);
    }

    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());

        ChangeSet changes;

        const int startPos = currentFile->startOf(m_literal);
        const int endPos = currentFile->endOf(m_literal);

        // kill leading '@'. No need to adapt endPos, that is done by ChangeSet
        if (m_actions & RemoveObjectiveCAction)
            changes.remove(startPos, startPos + 1);

        // Fix quotes
        if (m_actions & (SingleQuoteAction | DoubleQuoteAction)) {
            const QString newQuote((m_actions & SingleQuoteAction)
                                   ? QLatin1Char('\'') : QLatin1Char('"'));
            changes.replace(startPos, startPos + 1, newQuote);
            changes.replace(endPos - 1, endPos, newQuote);
        }

        // Convert single character strings into character constants
        if (m_actions & ConvertEscapeSequencesToCharAction) {
            StringLiteralAST *stringLiteral = m_literal->asStringLiteral();
            QTC_ASSERT(stringLiteral, return ;);
            const QByteArray oldContents(currentFile->tokenAt(stringLiteral->literal_token).identifier->chars());
            const QByteArray newContents = stringToCharEscapeSequences(oldContents);
            QTC_ASSERT(!newContents.isEmpty(), return ;);
            if (oldContents != newContents)
                changes.replace(startPos + 1, endPos -1, QString::fromLatin1(newContents));
        }

        // Convert character constants into strings constants
        if (m_actions & ConvertEscapeSequencesToStringAction) {
            NumericLiteralAST *charLiteral = m_literal->asNumericLiteral(); // char 'c' constants are numerical.
            QTC_ASSERT(charLiteral, return ;);
            const QByteArray oldContents(currentFile->tokenAt(charLiteral->literal_token).identifier->chars());
            const QByteArray newContents = charToStringEscapeSequences(oldContents);
            QTC_ASSERT(!newContents.isEmpty(), return ;);
            if (oldContents != newContents)
                changes.replace(startPos + 1, endPos -1, QString::fromLatin1(newContents));
        }

        // Enclose in literal or translation function, macro.
        if (m_actions & (EncloseActionMask | TranslationMask)) {
            changes.insert(endPos, QString(QLatin1Char(')')));
            QString leading = stringLiteralReplacement(m_actions);
            leading += QLatin1Char('(');
            if (m_actions
                    & (TranslateQCoreApplicationAction | TranslateNoopAction)) {
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

} // anonymous namespace

void WrapStringLiteral::doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    StringLiteralType type = TypeNone;
    QByteArray enclosingFunction;
    const QList<AST *> &path = interface.path();
    CppRefactoringFilePtr file = interface.currentFile();
    ExpressionAST *literal = analyzeStringLiteral(path, file, &type, &enclosingFunction);
    if (!literal || type == TypeNone)
        return;
    if ((type == TypeChar && enclosingFunction == "QLatin1Char")
        || isQtStringLiteral(enclosingFunction)
        || isQtStringTranslation(enclosingFunction))
        return;

    const int priority = path.size() - 1; // very high priority
    if (type == TypeChar) {
        unsigned actions = EncloseInQLatin1CharAction;
        QString description = msgQtStringLiteralDescription(stringLiteralReplacement(actions));
        result << new WrapStringLiteralOp(interface, priority, actions, description, literal);
        if (NumericLiteralAST *charLiteral = literal->asNumericLiteral()) {
            const QByteArray contents(file->tokenAt(charLiteral->literal_token).identifier->chars());
            if (!charToStringEscapeSequences(contents).isEmpty()) {
                actions = DoubleQuoteAction | ConvertEscapeSequencesToStringAction;
                description = Tr::tr("Convert to String Literal");
                result << new WrapStringLiteralOp(interface, priority, actions,
                                                  description, literal);
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
                QString description =
                    Tr::tr("Convert to Character Literal and Enclose in QLatin1Char(...)");
                result << new WrapStringLiteralOp(interface, priority, actions,
                                                  description, literal);
                actions &= ~EncloseInQLatin1CharAction;
                description = Tr::tr("Convert to Character Literal");
                result << new WrapStringLiteralOp(interface, priority, actions,
                                                  description, literal);
            }
        }
        actions = EncloseInQLatin1StringAction | objectiveCActions;
        result << new WrapStringLiteralOp(interface, priority, actions,
                                          msgQtStringLiteralDescription(stringLiteralReplacement(actions)), literal);
        actions = EncloseInQStringLiteralAction | objectiveCActions;
        result << new WrapStringLiteralOp(interface, priority, actions,
                                          msgQtStringLiteralDescription(stringLiteralReplacement(actions)), literal);
        actions = EncloseInQByteArrayLiteralAction | objectiveCActions;
        result << new WrapStringLiteralOp(interface, priority, actions,
                                          msgQtStringLiteralDescription(stringLiteralReplacement(actions)), literal);
    }
}

void TranslateStringLiteral::doMatch(const CppQuickFixInterface &interface,
                                     QuickFixOperations &result)
{
    // Initialize
    StringLiteralType type = TypeNone;
    QByteArray enclosingFunction;
    const QList<AST *> &path = interface.path();
    CppRefactoringFilePtr file = interface.currentFile();
    ExpressionAST *literal = analyzeStringLiteral(path, file, &type, &enclosingFunction);
    if (!literal || type != TypeString
       || isQtStringLiteral(enclosingFunction) || isQtStringTranslation(enclosingFunction))
        return;

    QString trContext;

    std::shared_ptr<Control> control = interface.context().bindings()->control();
    const Name *trName = control->identifier("tr");

    // Check whether we are in a function:
    const QString description = Tr::tr("Mark as Translatable");
    for (int i = path.size() - 1; i >= 0; --i) {
        if (FunctionDefinitionAST *definition = path.at(i)->asFunctionDefinition()) {
            Function *function = definition->symbol;
            ClassOrNamespace *b = interface.context().lookupType(function);
            if (b) {
                // Do we have a tr function?
                const QList<LookupItem> items = b->find(trName);
                for (const LookupItem &r : items) {
                    Symbol *s = r.declaration();
                    if (s->type()->asFunctionType()) {
                        // no context required for tr
                        result << new WrapStringLiteralOp(interface, path.size() - 1,
                                                          TranslateTrAction,
                                                          description, literal);
                        return;
                    }
                }
            }
            // We need to do a QCA::translate, so we need a context.
            // Use fully qualified class name:
            Overview oo;
            const QList<const Name *> names = LookupContext::path(function);
            for (const Name *n : names) {
                if (!trContext.isEmpty())
                    trContext.append(QLatin1String("::"));
                trContext.append(oo.prettyName(n));
            }
            // ... or global if none available!
            if (trContext.isEmpty())
                trContext = QLatin1String("GLOBAL");
            result << new WrapStringLiteralOp(interface, path.size() - 1,
                                              TranslateQCoreApplicationAction,
                                              description, literal, trContext);
            return;
        }
    }

    // We need to use Q_TRANSLATE_NOOP
    result << new WrapStringLiteralOp(interface, path.size() - 1,
                                      TranslateNoopAction,
                                      description, literal, trContext);
}

namespace {

class ConvertCStringToNSStringOp: public CppQuickFixOperation
{
public:
    ConvertCStringToNSStringOp(const CppQuickFixInterface &interface, int priority,
                               StringLiteralAST *stringLiteral, CallAST *qlatin1Call)
        : CppQuickFixOperation(interface, priority)
        , stringLiteral(stringLiteral)
        , qlatin1Call(qlatin1Call)
    {
        setDescription(Tr::tr("Convert to Objective-C String Literal"));
    }

    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());

        ChangeSet changes;

        if (qlatin1Call) {
            changes.replace(currentFile->startOf(qlatin1Call), currentFile->startOf(stringLiteral),
                            QLatin1String("@"));
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

} // anonymous namespace

void ConvertCStringToNSString::doMatch(const CppQuickFixInterface &interface,
                                       QuickFixOperations &result)
{
    CppRefactoringFilePtr file = interface.currentFile();

    if (!interface.editor()->cppEditorDocument()->isObjCEnabled())
        return;

    StringLiteralType type = TypeNone;
    QByteArray enclosingFunction;
    CallAST *qlatin1Call;
    const QList<AST *> &path = interface.path();
    ExpressionAST *literal = analyzeStringLiteral(path, file, &type, &enclosingFunction,
                                                  &qlatin1Call);
    if (!literal || type != TypeString)
        return;
    if (!isQtStringLiteral(enclosingFunction))
        qlatin1Call = nullptr;

    result << new ConvertCStringToNSStringOp(interface, path.size() - 1, literal->asStringLiteral(),
                                             qlatin1Call);
}

namespace {

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

    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());

        ChangeSet changes;
        changes.replace(start, end, replacement);
        currentFile->setChangeSet(changes);
        currentFile->apply();
    }

private:
    int start, end;
    QString replacement;
};

} // anonymous namespace

void ConvertNumericLiteral::doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();
    CppRefactoringFilePtr file = interface.currentFile();

    if (path.isEmpty())
        return;

    NumericLiteralAST *literal = path.last()->asNumericLiteral();

    if (!literal)
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
    ulong value = 0;
    const QString x = QString::fromUtf8(spell).left(numberLength);
    if (x.startsWith("0b", Qt::CaseInsensitive))
        value = x.mid(2).toULong(&valid, 2);
    else
        value = x.toULong(&valid, 0);

    if (!valid)
        return;

    const int priority = path.size() - 1; // very high priority
    const int start = file->startOf(literal);
    const char * const str = numeric->chars();

    const bool isBinary = numberLength > 2 && str[0] == '0' && (str[1] == 'b' || str[1] == 'B');
    const bool isOctal = numberLength >= 2 && str[0] == '0' && str[1] >= '0' && str[1] <= '7';
    const bool isDecimal = !(isBinary || isOctal || numeric->isHex());

    if (!numeric->isHex()) {
        /*
          Convert integer literal to hex representation.
          Replace
            0b100000
            32
            040
          With
            0x20

        */
        const QString replacement = QString::asprintf("0x%lX", value);
        auto op = new ConvertNumericLiteralOp(interface, start, start + numberLength, replacement);
        op->setDescription(Tr::tr("Convert to Hexadecimal"));
        op->setPriority(priority);
        result << op;
    }

    if (!isOctal) {
        /*
          Convert integer literal to octal representation.
          Replace
            0b100000
            32
            0x20
          With
            040
        */
        const QString replacement = QString::asprintf("0%lo", value);
        auto op = new ConvertNumericLiteralOp(interface, start, start + numberLength, replacement);
        op->setDescription(Tr::tr("Convert to Octal"));
        op->setPriority(priority);
        result << op;
    }

    if (!isDecimal) {
        /*
          Convert integer literal to decimal representation.
          Replace
            0b100000
            0x20
            040
           With
            32
        */
        const QString replacement = QString::asprintf("%lu", value);
        auto op = new ConvertNumericLiteralOp(interface, start, start + numberLength, replacement);
        op->setDescription(Tr::tr("Convert to Decimal"));
        op->setPriority(priority);
        result << op;
    }

    if (!isBinary) {
        /*
          Convert integer literal to binary representation.
          Replace
            32
            0x20
            040
          With
            0b100000
        */
        QString replacement = "0b";
        if (value == 0) {
            replacement.append('0');
        } else {
            std::bitset<std::numeric_limits<decltype (value)>::digits> b(value);
            QRegularExpression re("^[0]*");
            replacement.append(QString::fromStdString(b.to_string()).remove(re));
        }
        auto op = new ConvertNumericLiteralOp(interface, start, start + numberLength, replacement);
        op->setDescription(Tr::tr("Convert to Binary"));
        op->setPriority(priority);
        result << op;
    }
}

namespace {

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
        setDescription(Tr::tr("Add Local Declaration"));
    }

    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());
        QString declaration = getDeclaration();

        if (!declaration.isEmpty()) {
            ChangeSet changes;
            changes.replace(currentFile->startOf(binaryAST),
                            currentFile->endOf(simpleNameAST),
                            declaration);
            currentFile->setChangeSet(changes);
            currentFile->apply();
        }
    }

private:
    QString getDeclaration()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());
        Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
        const auto settings = CppQuickFixProjectsSettings::getQuickFixSettings(
                    ProjectExplorer::ProjectTree::currentProject());

        if (currentFile->cppDocument()->languageFeatures().cxx11Enabled && settings->useAuto)
            return "auto " + oo.prettyName(simpleNameAST->name);
        return declFromExpr(binaryAST->right_expression, nullptr, simpleNameAST, snapshot(),
                            context(), currentFile, false);
    }

    const BinaryExpressionAST *binaryAST;
    const SimpleNameAST *simpleNameAST;
};

} // anonymous namespace

namespace {

class ConvertToCamelCaseOp: public CppQuickFixOperation
{
public:
    ConvertToCamelCaseOp(const CppQuickFixInterface &interface, const QString &name,
                         const AST *nameAst, bool test)
        : CppQuickFixOperation(interface, -1)
        , m_name(name)
        , m_nameAst(nameAst)
        , m_isAllUpper(name.isUpper())
        , m_test(test)
    {
        setDescription(Tr::tr("Convert to Camel Case"));
    }

    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());

        QString newName = m_isAllUpper ? m_name.toLower() : m_name;
        for (int i = 1; i < newName.length(); ++i) {
            const QChar c = newName.at(i);
            if (c.isUpper() && m_isAllUpper) {
                newName[i] = c.toLower();
            } else if (i < newName.length() - 1 && isConvertibleUnderscore(newName, i)) {
                newName.remove(i, 1);
                newName[i] = newName.at(i).toUpper();
            }
        }
        if (m_test) {
            ChangeSet changeSet;
            changeSet.replace(currentFile->range(m_nameAst), newName);
            currentFile->setChangeSet(changeSet);
            currentFile->apply();
        } else {
            editor()->renameUsages(newName);
        }
    }

    static bool isConvertibleUnderscore(const QString &name, int pos)
    {
        return name.at(pos) == QLatin1Char('_') && name.at(pos+1).isLetter()
                && !(pos == 1 && name.at(0) == QLatin1Char('m'));
    }

private:
    const QString m_name;
    const AST * const m_nameAst;
    const bool m_isAllUpper;
    const bool m_test;
};

} // anonymous namespace

void ConvertToCamelCase::doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();

    if (path.isEmpty())
        return;

    AST * const ast = path.last();
    const Name *name = nullptr;
    const AST *astForName = nullptr;
    if (const NameAST * const nameAst = ast->asName()) {
        if (nameAst->name && nameAst->name->asNameId()) {
            astForName = nameAst;
            name = nameAst->name;
        }
    } else if (const NamespaceAST * const namespaceAst = ast->asNamespace()) {
        astForName = namespaceAst;
        name = namespaceAst->symbol->name();
    }

    if (!name)
        return;

    QString nameString = QString::fromUtf8(name->identifier()->chars());
    if (nameString.length() < 3)
        return;
    for (int i = 1; i < nameString.length() - 1; ++i) {
        if (ConvertToCamelCaseOp::isConvertibleUnderscore(nameString, i)) {
            result << new ConvertToCamelCaseOp(interface, nameString, astForName, m_test);
            return;
        }
    }
}

AddIncludeForUndefinedIdentifierOp::AddIncludeForUndefinedIdentifierOp(
        const CppQuickFixInterface &interface, int priority, const QString &include)
    : CppQuickFixOperation(interface, priority)
    , m_include(include)
{
    setDescription(Tr::tr("Add #include %1").arg(m_include));
}

void AddIncludeForUndefinedIdentifierOp::perform()
{
    CppRefactoringChanges refactoring(snapshot());
    CppRefactoringFilePtr file = refactoring.cppFile(filePath());

    ChangeSet changes;
    insertNewIncludeDirective(m_include, file, semanticInfo().doc, changes);
    file->setChangeSet(changes);
    file->apply();
}

AddForwardDeclForUndefinedIdentifierOp::AddForwardDeclForUndefinedIdentifierOp(
        const CppQuickFixInterface &interface,
        int priority,
        const QString &fqClassName,
        int symbolPos)
    : CppQuickFixOperation(interface, priority), m_className(fqClassName), m_symbolPos(symbolPos)
{
    setDescription(Tr::tr("Add Forward Declaration for %1").arg(m_className));
}

void AddForwardDeclForUndefinedIdentifierOp::perform()
{
    const QStringList parts = m_className.split("::");
    QTC_ASSERT(!parts.isEmpty(), return);
    const QStringList namespaces = parts.mid(0, parts.length() - 1);

    CppRefactoringChanges refactoring(snapshot());
    CppRefactoringFilePtr file = refactoring.cppFile(filePath());

    NSVisitor visitor(file.data(), namespaces, m_symbolPos);
    visitor.accept(file->cppDocument()->translationUnit()->ast());
    const auto stringToInsert = [&visitor, symbol = parts.last()] {
        QString s = "\n";
        for (const QString &ns : visitor.remainingNamespaces())
            s += "namespace " + ns + " { ";
        s += "class " + symbol + ';';
        for (int i = 0; i < visitor.remainingNamespaces().size(); ++i)
            s += " }";
        return s;
    };

    int insertPos = 0;

    // Find the position to insert:
    //   If we have a matching namespace, we do the insertion there.
    //   If we don't have a matching namespace, but there is another namespace in the file,
    //   we assume that to be a good position for our insertion.
    //   Otherwise, do the insertion after the last include that comes before the use of the symbol.
    //   If there is no such include, do the insertion before the first token.
    if (visitor.enclosingNamespace()) {
        insertPos = file->startOf(visitor.enclosingNamespace()->linkage_body) + 1;
    } else if (visitor.firstNamespace()) {
        insertPos = file->startOf(visitor.firstNamespace());
    } else {
        const QTextCursor tc = file->document()->find(
                    QRegularExpression("^\\s*#include .*$"),
                    m_symbolPos,
                    QTextDocument::FindBackward | QTextDocument::FindCaseSensitively);
        if (!tc.isNull())
            insertPos = tc.position() + 1;
        else if (visitor.firstToken())
            insertPos = file->startOf(visitor.firstToken());
    }

    QString insertion = stringToInsert();
    if (file->charAt(insertPos - 1) != QChar::ParagraphSeparator)
        insertion.prepend('\n');
    if (file->charAt(insertPos) != QChar::ParagraphSeparator)
        insertion.append('\n');
    ChangeSet s;
    s.insert(insertPos, insertion);
    file->setChangeSet(s);
    file->apply();
}

namespace {

QString findShortestInclude(const QString currentDocumentFilePath,
                            const QString candidateFilePath,
                            const ProjectExplorer::HeaderPaths &headerPaths)
{
    QString result;

    const QFileInfo fileInfo(candidateFilePath);

    if (fileInfo.path() == QFileInfo(currentDocumentFilePath).path()) {
        result = QLatin1Char('"') + fileInfo.fileName() + QLatin1Char('"');
    } else {
        for (const ProjectExplorer::HeaderPath &headerPath : headerPaths) {
            if (!candidateFilePath.startsWith(headerPath.path))
                continue;
            QString relativePath = candidateFilePath.mid(headerPath.path.size());
            if (!relativePath.isEmpty() && relativePath.at(0) == QLatin1Char('/'))
                relativePath = relativePath.mid(1);
            if (result.isEmpty() || relativePath.size() + 2 < result.size())
                result = QLatin1Char('<') + relativePath + QLatin1Char('>');
        }
    }

    return result;
}

QString findMatchingInclude(const QString &className,
                            const ProjectExplorer::HeaderPaths &headerPaths)
{
    const QStringList candidateFileNames{className, className + ".h", className + ".hpp",
                className.toLower(), className.toLower() + ".h", className.toLower() + ".hpp"};
    for (const QString &fileName : candidateFileNames) {
        for (const ProjectExplorer::HeaderPath &headerPath : headerPaths) {
            const QString headerPathCandidate = headerPath.path + QLatin1Char('/') + fileName;
            const QFileInfo fileInfo(headerPathCandidate);
            if (fileInfo.exists() && fileInfo.isFile())
                return '<' + fileName + '>';
        }
    }
    return {};
}

ProjectExplorer::HeaderPaths relevantHeaderPaths(const QString &filePath)
{
    ProjectExplorer::HeaderPaths headerPaths;

    const QList<ProjectPart::ConstPtr> projectParts = CppModelManager::projectPart(filePath);
    if (projectParts.isEmpty()) { // Not part of any project, better use all include paths than none
        headerPaths += CppModelManager::headerPaths();
    } else {
        for (const ProjectPart::ConstPtr &part : projectParts)
            headerPaths += part->headerPaths;
    }

    return headerPaths;
}

NameAST *nameUnderCursor(const QList<AST *> &path)
{
    if (path.isEmpty())
        return nullptr;

    NameAST *nameAst = nullptr;
    for (int i = path.size() - 1; i >= 0; --i) {
        AST * const ast = path.at(i);
        if (SimpleNameAST *simpleName = ast->asSimpleName()) {
            nameAst = simpleName;
        } else if (TemplateIdAST *templateId = ast->asTemplateId()) {
            nameAst = templateId;
        } else if (nameAst && ast->asNamedTypeSpecifier()) {
            break; // Stop at "Foo" for "N::Bar<@Foo>"
        } else if (QualifiedNameAST *qualifiedName = ast->asQualifiedName()) {
            nameAst = qualifiedName;
            break;
        }
    }

    return nameAst;
}

enum class LookupResult { Declared, ForwardDeclared, NotDeclared };
LookupResult lookUpDefinition(const CppQuickFixInterface &interface, const NameAST *nameAst)
{
    QTC_ASSERT(nameAst && nameAst->name, return LookupResult::NotDeclared);

    // Find the enclosing scope
    int line, column;
    const Document::Ptr doc = interface.semanticInfo().doc;
    doc->translationUnit()->getTokenPosition(nameAst->firstToken(), &line, &column);
    Scope *scope = doc->scopeAt(line, column);
    if (!scope)
        return LookupResult::NotDeclared;

    // Try to find the class/template definition
    const Name *name = nameAst->name;
    const QList<LookupItem> results = interface.context().lookup(name, scope);
    LookupResult best = LookupResult::NotDeclared;
    for (const LookupItem &item : results) {
        if (Symbol *declaration = item.declaration()) {
            if (declaration->asClass())
                return LookupResult::Declared;
            if (declaration->asForwardClassDeclaration()) {
                best = LookupResult::ForwardDeclared;
                continue;
            }
            if (Template *templ = declaration->asTemplate()) {
                if (Symbol *declaration = templ->declaration()) {
                    if (declaration->asClass())
                        return LookupResult::Declared;
                    if (declaration->asForwardClassDeclaration()) {
                        best = LookupResult::ForwardDeclared;
                        continue;
                    }
                }
            }
            return LookupResult::Declared;
        }
    }

    return best;
}

QString templateNameAsString(const TemplateNameId *templateName)
{
    const Identifier *id = templateName->identifier();
    return QString::fromUtf8(id->chars(), id->size());
}

Snapshot forwardingHeaders(const CppQuickFixInterface &interface)
{
    Snapshot result;

    const Snapshot docs = interface.snapshot();
    for (Document::Ptr doc : docs) {
        if (doc->globalSymbolCount() == 0 && doc->resolvedIncludes().size() == 1)
            result.insert(doc);
    }

    return result;
}

QList<IndexItem::Ptr> matchName(const Name *name, QString *className)
{
    if (!name)
        return {};

    QString simpleName;
    QList<IndexItem::Ptr> matches;
    CppLocatorData *locatorData = CppModelManager::locatorData();
    const Overview oo;
    if (const QualifiedNameId *qualifiedName = name->asQualifiedNameId()) {
        const Name *name = qualifiedName->name();
        if (const TemplateNameId *templateName = name->asTemplateNameId()) {
            *className = templateNameAsString(templateName);
        } else {
            simpleName = oo.prettyName(name);
            *className = simpleName;
            matches = locatorData->findSymbols(IndexItem::Class, *className);
            if (matches.isEmpty()) {
                if (const Name *name = qualifiedName->base()) {
                    if (const TemplateNameId *templateName = name->asTemplateNameId())
                        *className = templateNameAsString(templateName);
                    else
                        *className = oo.prettyName(name);
                }
            }
        }
    } else if (const TemplateNameId *templateName = name->asTemplateNameId()) {
        *className = templateNameAsString(templateName);
    } else {
        *className = oo.prettyName(name);
    }

    if (matches.isEmpty())
        matches = locatorData->findSymbols(IndexItem::Class, *className);

    if (matches.isEmpty() && !simpleName.isEmpty())
        *className = simpleName;

    return matches;
}

} // anonymous namespace

void AddIncludeForUndefinedIdentifier::doMatch(const CppQuickFixInterface &interface,
                                               QuickFixOperations &result)
{
    const NameAST *nameAst = nameUnderCursor(interface.path());
    if (!nameAst || !nameAst->name)
        return;

    const LookupResult lookupResult = lookUpDefinition(interface, nameAst);
    if (lookupResult == LookupResult::Declared)
        return;

    QString className;
    const QString currentDocumentFilePath = interface.semanticInfo().doc->filePath().toString();
    const ProjectExplorer::HeaderPaths headerPaths = relevantHeaderPaths(currentDocumentFilePath);
    FilePaths headers;

    const QList<IndexItem::Ptr> matches = matchName(nameAst->name, &className);
    // Find an include file through the locator
    if (!matches.isEmpty()) {
        QList<IndexItem::Ptr> indexItems;
        const Snapshot forwardHeaders = forwardingHeaders(interface);
        for (const IndexItem::Ptr &info : matches) {
            if (!info || info->symbolName() != className)
                continue;
            indexItems << info;

            Snapshot localForwardHeaders = forwardHeaders;
            localForwardHeaders.insert(interface.snapshot().document(info->filePath()));
            FilePaths headerAndItsForwardingHeaders;
            headerAndItsForwardingHeaders << info->filePath();
            headerAndItsForwardingHeaders += localForwardHeaders.filesDependingOn(info->filePath());

            for (const FilePath &header : std::as_const(headerAndItsForwardingHeaders)) {
                const QString include = findShortestInclude(currentDocumentFilePath,
                                                            header.toString(),
                                                            headerPaths);
                if (include.size() > 2) {
                    const QString headerFileName = info->filePath().fileName();
                    QTC_ASSERT(!headerFileName.isEmpty(), break);

                    int priority = 0;
                    if (headerFileName == className)
                        priority = 2;
                    else if (headerFileName.at(1).isUpper())
                        priority = 1;

                    result << new AddIncludeForUndefinedIdentifierOp(interface, priority,
                                                                     include);
                    headers << header;
                }
            }
        }

        if (lookupResult == LookupResult::NotDeclared && indexItems.size() == 1) {
            QString qualifiedName = Overview().prettyName(nameAst->name);
            if (qualifiedName.startsWith("::"))
                qualifiedName.remove(0, 2);
            if (indexItems.first()->scopedSymbolName().endsWith(qualifiedName)) {
                const ProjectExplorer::Node * const node = ProjectExplorer::ProjectTree
                        ::nodeForFile(interface.filePath());
                ProjectExplorer::FileType fileType = node && node->asFileNode()
                        ? node->asFileNode()->fileType() : ProjectExplorer::FileType::Unknown;
                if (fileType == ProjectExplorer::FileType::Unknown
                        && ProjectFile::isHeader(ProjectFile::classify(interface.filePath().toString()))) {
                    fileType = ProjectExplorer::FileType::Header;
                }
                if (fileType == ProjectExplorer::FileType::Header) {
                    result << new AddForwardDeclForUndefinedIdentifierOp(
                                  interface, 0, indexItems.first()->scopedSymbolName(),
                                  interface.currentFile()->startOf(nameAst));
                }
            }
        }
    }

    if (className.isEmpty())
        return;

    // Fallback: Check the include paths for files that look like candidates
    //           for the given name.
    if (!Utils::contains(headers,
            [&className](const Utils::FilePath &fp) { return fp.fileName() == className; })) {
        const QString include = findMatchingInclude(className, headerPaths);
        const auto matcher = [&include](const QuickFixOperation::Ptr &o) {
            const auto includeOp = o.dynamicCast<AddIncludeForUndefinedIdentifierOp>();
            return includeOp && includeOp->include() == include;
        };
        if (!include.isEmpty() && !Utils::contains(result, matcher))
            result << new AddIncludeForUndefinedIdentifierOp(interface, 1, include);
    }
}

namespace {

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
            targetString = Tr::tr("Switch with Previous Parameter");
        else
            targetString = Tr::tr("Switch with Next Parameter");
        setDescription(targetString);
    }

    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());

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

} // anonymous namespace

void RearrangeParamDeclarationList::doMatch(const CppQuickFixInterface &interface,
                                          QuickFixOperations &result)
{
    const QList<AST *> path = interface.path();

    ParameterDeclarationAST *paramDecl = nullptr;
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
    ParameterDeclarationListAST *prevParamListNode = nullptr;
    while (paramListNode) {
        if (paramDecl == paramListNode->value)
            break;
        prevParamListNode = paramListNode;
        paramListNode = paramListNode->next;
    }

    if (!paramListNode)
        return;

    if (prevParamListNode)
        result << new RearrangeParamDeclarationListOp(interface, paramListNode->value,
                                                      prevParamListNode->value, RearrangeParamDeclarationListOp::TargetPrevious);
    if (paramListNode->next)
        result << new RearrangeParamDeclarationListOp(interface, paramListNode->value,
                                                      paramListNode->next->value, RearrangeParamDeclarationListOp::TargetNext);
}

namespace {

class ReformatPointerDeclarationOp: public CppQuickFixOperation
{
public:
    ReformatPointerDeclarationOp(const CppQuickFixInterface &interface, const ChangeSet change)
        : CppQuickFixOperation(interface)
        , m_change(change)
    {
        QString description;
        if (m_change.operationList().size() == 1) {
            description = Tr::tr(
                        "Reformat to \"%1\"").arg(m_change.operationList().constFirst().text());
        } else { // > 1
            description = Tr::tr("Reformat Pointers or References");
        }
        setDescription(description);
    }

    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());
        currentFile->setChangeSet(m_change);
        currentFile->apply();
    }

private:
    ChangeSet m_change;
};

/// Filter the results of ASTPath.
/// The resulting list contains the supported AST types only once.
/// For this, the results of ASTPath are iterated in reverse order.
class ReformatPointerDeclarationASTPathResultsFilter
{
public:
    QList<AST*> filter(const QList<AST*> &astPathList)
    {
        QList<AST*> filtered;

        for (int i = astPathList.size() - 1; i >= 0; --i) {
            AST *ast = astPathList.at(i);

            if (!m_hasSimpleDeclaration && ast->asSimpleDeclaration()) {
                m_hasSimpleDeclaration = true;
                filtered.append(ast);
            } else if (!m_hasFunctionDefinition && ast->asFunctionDefinition()) {
                m_hasFunctionDefinition = true;
                filtered.append(ast);
            } else if (!m_hasParameterDeclaration && ast->asParameterDeclaration()) {
                m_hasParameterDeclaration = true;
                filtered.append(ast);
            } else if (!m_hasIfStatement && ast->asIfStatement()) {
                m_hasIfStatement = true;
                filtered.append(ast);
            } else if (!m_hasWhileStatement && ast->asWhileStatement()) {
                m_hasWhileStatement = true;
                filtered.append(ast);
            } else if (!m_hasForStatement && ast->asForStatement()) {
                m_hasForStatement = true;
                filtered.append(ast);
            } else if (!m_hasForeachStatement && ast->asForeachStatement()) {
                m_hasForeachStatement = true;
                filtered.append(ast);
            }
        }

        return filtered;
    }

private:
    bool m_hasSimpleDeclaration = false;
    bool m_hasFunctionDefinition = false;
    bool m_hasParameterDeclaration = false;
    bool m_hasIfStatement = false;
    bool m_hasWhileStatement = false;
    bool m_hasForStatement = false;
    bool m_hasForeachStatement = false;
};

} // anonymous namespace

void ReformatPointerDeclaration::doMatch(const CppQuickFixInterface &interface,
                                       QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();
    CppRefactoringFilePtr file = interface.currentFile();

    Overview overview = CppCodeStyleSettings::currentProjectCodeStyleOverview();
    overview.showArgumentNames = true;
    overview.showReturnTypes = true;

    const QTextCursor cursor = file->cursor();
    ChangeSet change;
    PointerDeclarationFormatter formatter(file, overview,
        PointerDeclarationFormatter::RespectCursor);

    if (cursor.hasSelection()) {
        // This will no work always as expected since this function is only called if
        // interface-path() is not empty. If the user selects the whole document via
        // ctrl-a and there is an empty line in the end, then the cursor is not on
        // any AST and therefore no quick fix will be triggered.
        change = formatter.format(file->cppDocument()->translationUnit()->ast());
        if (!change.isEmpty())
            result << new ReformatPointerDeclarationOp(interface, change);
    } else {
        const QList<AST *> suitableASTs
            = ReformatPointerDeclarationASTPathResultsFilter().filter(path);
        for (AST *ast : suitableASTs) {
            change = formatter.format(ast);
            if (!change.isEmpty()) {
                result << new ReformatPointerDeclarationOp(interface, change);
                return;
            }
        }
    }
}

namespace {

class CaseStatementCollector : public ASTVisitor
{
public:
    CaseStatementCollector(Document::Ptr document, const Snapshot &snapshot,
                           Scope *scope)
        : ASTVisitor(document->translationUnit()),
        document(document),
        scope(scope)
    {
        typeOfExpression.init(document, snapshot);
    }

    QStringList operator ()(AST *ast)
    {
        values.clear();
        foundCaseStatementLevel = false;
        accept(ast);
        return values;
    }

    bool preVisit(AST *ast) override {
        if (CaseStatementAST *cs = ast->asCaseStatement()) {
            foundCaseStatementLevel = true;
            if (ExpressionAST *csExpression = cs->expression) {
                if (ExpressionAST *expression = csExpression->asIdExpression()) {
                    QList<LookupItem> candidates = typeOfExpression(expression, document, scope);
                    if (!candidates.isEmpty() && candidates.first().declaration()) {
                        Symbol *decl = candidates.first().declaration();
                        values << prettyPrint.prettyName(LookupContext::fullyQualifiedName(decl));
                    }
                }
            }
            return true;
        } else if (foundCaseStatementLevel) {
            return false;
        }
        return true;
    }

    Overview prettyPrint;
    bool foundCaseStatementLevel = false;
    QStringList values;
    TypeOfExpression typeOfExpression;
    Document::Ptr document;
    Scope *scope;
};

class CompleteSwitchCaseStatementOp: public CppQuickFixOperation
{
public:
    CompleteSwitchCaseStatementOp(const CppQuickFixInterface &interface,
              int priority, CompoundStatementAST *compoundStatement, const QStringList &values)
        : CppQuickFixOperation(interface, priority)
        , compoundStatement(compoundStatement)
        , values(values)
    {
        setDescription(Tr::tr("Complete Switch Statement"));
    }

    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());

        ChangeSet changes;
        int start = currentFile->endOf(compoundStatement->lbrace_token);
        changes.insert(start, QLatin1String("\ncase ")
                       + values.join(QLatin1String(":\nbreak;\ncase "))
                       + QLatin1String(":\nbreak;"));
        currentFile->setChangeSet(changes);
        currentFile->apply();
    }

    CompoundStatementAST *compoundStatement;
    QStringList values;
};

static Enum *findEnum(const QList<LookupItem> &results, const LookupContext &ctxt)
{
    for (const LookupItem &result : results) {
        const FullySpecifiedType fst = result.type();

        Type *type = result.declaration() ? result.declaration()->type().type()
                                          : fst.type();

        if (!type)
            continue;
        if (Enum *e = type->asEnumType())
            return e;
        if (const NamedType *namedType = type->asNamedType()) {
            if (ClassOrNamespace *con = ctxt.lookupType(namedType->name(), result.scope())) {
                QList<Enum *> enums = con->unscopedEnums();
                const QList<Symbol *> symbols = con->symbols();
                for (Symbol * const s : symbols) {
                    if (const auto e = s->asEnum())
                        enums << e;
                }
                const Name *referenceName = namedType->name();
                if (const QualifiedNameId *qualifiedName = referenceName->asQualifiedNameId())
                    referenceName = qualifiedName->name();
                for (Enum *e : std::as_const(enums)) {
                    if (const Name *candidateName = e->name()) {
                        if (candidateName->match(referenceName))
                            return e;
                    }
                }
            }
        }
    }

    return nullptr;
}

Enum *conditionEnum(const CppQuickFixInterface &interface, SwitchStatementAST *statement)
{
    Block *block = statement->symbol;
    Scope *scope = interface.semanticInfo().doc->scopeAt(block->line(), block->column());
    TypeOfExpression typeOfExpression;
    typeOfExpression.setExpandTemplates(true);
    typeOfExpression.init(interface.semanticInfo().doc, interface.snapshot());
    const QList<LookupItem> results = typeOfExpression(statement->condition,
                                                       interface.semanticInfo().doc,
                                                       scope);

    return findEnum(results, typeOfExpression.context());
}

} // anonymous namespace

void CompleteSwitchCaseStatement::doMatch(const CppQuickFixInterface &interface,
                                          QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();

    if (path.isEmpty())
        return;

    // look for switch statement
    for (int depth = path.size() - 1; depth >= 0; --depth) {
        AST *ast = path.at(depth);
        SwitchStatementAST *switchStatement = ast->asSwitchStatement();
        if (switchStatement) {
            if (!switchStatement->statement || !switchStatement->symbol)
                return;
            CompoundStatementAST *compoundStatement = switchStatement->statement->asCompoundStatement();
            if (!compoundStatement) // we ignore pathologic case "switch (t) case A: ;"
                return;
            // look if the condition's type is an enum
            if (Enum *e = conditionEnum(interface, switchStatement)) {
                // check the possible enum values
                QStringList values;
                Overview prettyPrint;
                for (int i = 0; i < e->memberCount(); ++i) {
                    if (Declaration *decl = e->memberAt(i)->asDeclaration())
                        values << prettyPrint.prettyName(LookupContext::fullyQualifiedName(decl));
                }
                // Get the used values
                Block *block = switchStatement->symbol;
                CaseStatementCollector caseValues(interface.semanticInfo().doc, interface.snapshot(),
                                                  interface.semanticInfo().doc->scopeAt(block->line(), block->column()));
                const QStringList usedValues = caseValues(switchStatement);
                // save the values that would be added
                for (const QString &usedValue : usedValues)
                    values.removeAll(usedValue);
                if (!values.isEmpty())
                    result << new CompleteSwitchCaseStatementOp(interface, depth,
                                                                compoundStatement, values);
                return;
            }

            return;
        }
    }
}

namespace {

class InsertDeclOperation: public CppQuickFixOperation
{
public:
    InsertDeclOperation(const CppQuickFixInterface &interface,
                        const FilePath &targetFilePath, const Class *targetSymbol,
                        InsertionPointLocator::AccessSpec xsSpec, const QString &decl, int priority)
        : CppQuickFixOperation(interface, priority)
        , m_targetFilePath(targetFilePath)
        , m_targetSymbol(targetSymbol)
        , m_xsSpec(xsSpec)
        , m_decl(decl)
    {
        setDescription(Tr::tr("Add %1 Declaration")
                       .arg(InsertionPointLocator::accessSpecToString(xsSpec)));
    }

    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());

        InsertionPointLocator locator(refactoring);
        const InsertionLocation loc = locator.methodDeclarationInClass(
                    m_targetFilePath, m_targetSymbol, m_xsSpec);
        QTC_ASSERT(loc.isValid(), return);

        CppRefactoringFilePtr targetFile = refactoring.cppFile(m_targetFilePath);
        int targetPosition = targetFile->position(loc.line(), loc.column());

        ChangeSet target;
        target.insert(targetPosition, loc.prefix() + m_decl);
        targetFile->setChangeSet(target);
        targetFile->setOpenEditor(true, targetPosition);
        targetFile->apply();
    }

    static QString generateDeclaration(const Function *function);

private:
    FilePath m_targetFilePath;
    const Class *m_targetSymbol;
    InsertionPointLocator::AccessSpec m_xsSpec;
    QString m_decl;
};

class DeclOperationFactory
{
public:
    DeclOperationFactory(const CppQuickFixInterface &interface, const FilePath &filePath,
                         const Class *matchingClass, const QString &decl)
        : m_interface(interface)
        , m_filePath(filePath)
        , m_matchingClass(matchingClass)
        , m_decl(decl)
    {}

    QuickFixOperation *operator()(InsertionPointLocator::AccessSpec xsSpec, int priority)
    {
        return new InsertDeclOperation(m_interface, m_filePath, m_matchingClass, xsSpec, m_decl, priority);
    }

private:
    const CppQuickFixInterface &m_interface;
    const FilePath &m_filePath;
    const Class *m_matchingClass;
    const QString &m_decl;
};

} // anonymous namespace

void InsertDeclFromDef::doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();
    CppRefactoringFilePtr file = interface.currentFile();

    FunctionDefinitionAST *funDef = nullptr;
    int idx = 0;
    for (; idx < path.size(); ++idx) {
        AST *node = path.at(idx);
        if (idx > 1) {
            if (DeclaratorIdAST *declId = node->asDeclaratorId()) {
                if (file->isCursorOn(declId)) {
                    if (FunctionDefinitionAST *candidate = path.at(idx - 2)->asFunctionDefinition()) {
                        funDef = candidate;
                        break;
                    }
                }
            }
        }

        if (node->asClassSpecifier())
            return;
    }

    if (!funDef || !funDef->symbol)
        return;

    Function *fun = funDef->symbol;
    if (Class *matchingClass = isMemberFunction(interface.context(), fun)) {
        const QualifiedNameId *qName = fun->name()->asQualifiedNameId();
        for (Symbol *symbol = matchingClass->find(qName->identifier());
             symbol; symbol = symbol->next()) {
            Symbol *s = symbol;
            if (fun->enclosingScope()->asTemplate()) {
                if (const Template *templ = s->type()->asTemplateType()) {
                    if (Symbol *decl = templ->declaration()) {
                        if (decl->type()->asFunctionType())
                            s = decl;
                    }
                }
            }
            if (!s->name()
                    || !qName->identifier()->match(s->identifier())
                    || !s->type()->asFunctionType())
                continue;

            if (s->type().match(fun->type())) {
                // Declaration exists.
                return;
            }
        }
        const FilePath fileName = matchingClass->filePath();
        const QString decl = InsertDeclOperation::generateDeclaration(fun);

        // Add several possible insertion locations for declaration
        DeclOperationFactory operation(interface, fileName, matchingClass, decl);

        result << operation(InsertionPointLocator::Public, 5)
            << operation(InsertionPointLocator::PublicSlot, 4)
            << operation(InsertionPointLocator::Protected, 3)
            << operation(InsertionPointLocator::ProtectedSlot, 2)
            << operation(InsertionPointLocator::Private, 1)
            << operation(InsertionPointLocator::PrivateSlot, 0);
    }
}

QString InsertDeclOperation::generateDeclaration(const Function *function)
{
    Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
    oo.showFunctionSignatures = true;
    oo.showReturnTypes = true;
    oo.showArgumentNames = true;
    oo.showEnclosingTemplate = true;

    QString decl;
    decl += oo.prettyType(function->type(), function->unqualifiedName());
    decl += QLatin1String(";\n");

    return decl;
}

namespace {

class InsertDefOperation: public CppQuickFixOperation
{
public:
    // Make sure that either loc is valid or targetFileName is not empty.
    InsertDefOperation(const CppQuickFixInterface &interface,
                       Declaration *decl, DeclaratorAST *declAST, const InsertionLocation &loc,
                       const DefPos defpos, const FilePath &targetFileName = {},
                       bool freeFunction = false)
        : CppQuickFixOperation(interface, 0)
        , m_decl(decl)
        , m_declAST(declAST)
        , m_loc(loc)
        , m_defpos(defpos)
        , m_targetFilePath(targetFileName)
    {
        if (m_defpos == DefPosImplementationFile) {
            const FilePath declFile = decl->filePath();
            const FilePath targetFile =  m_loc.isValid() ? m_loc.filePath() : m_targetFilePath;
            const FilePath resolved = targetFile.relativePathFrom(declFile.parentDir());
            setPriority(2);
            setDescription(Tr::tr("Add Definition in %1").arg(resolved.displayName()));
        } else if (freeFunction) {
            setDescription(Tr::tr("Add Definition Here"));
        } else if (m_defpos == DefPosInsideClass) {
            setDescription(Tr::tr("Add Definition Inside Class"));
        } else if (m_defpos == DefPosOutsideClass) {
            setPriority(1);
            setDescription(Tr::tr("Add Definition Outside Class"));
        }
    }

    static void insertDefinition(
            const CppQuickFixOperation *op,
            InsertionLocation loc,
            DefPos defPos,
            DeclaratorAST *declAST,
            Declaration *decl,
            const FilePath &targetFilePath,
            ChangeSet *changeSet = nullptr)
    {
        CppRefactoringChanges refactoring(op->snapshot());
        if (!loc.isValid())
            loc = insertLocationForMethodDefinition(decl, true, NamespaceHandling::Ignore,
                                                      refactoring, targetFilePath);
        QTC_ASSERT(loc.isValid(), return);

        CppRefactoringFilePtr targetFile = refactoring.cppFile(loc.filePath());
        Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
        oo.showFunctionSignatures = true;
        oo.showReturnTypes = true;
        oo.showArgumentNames = true;
        oo.showEnclosingTemplate = true;

        // What we really want is to show template parameters for the class, but not for the
        // function, but we cannot express that. This is an approximation that will work
        // as long as either the surrounding class or the function is not a template.
        oo.showTemplateParameters = decl->enclosingClass()
                && decl->enclosingClass()->enclosingTemplate();

        if (defPos == DefPosInsideClass) {
            const int targetPos = targetFile->position(loc.line(), loc.column());
            ChangeSet localChangeSet;
            ChangeSet * const target = changeSet ? changeSet : &localChangeSet;
            target->replace(targetPos - 1, targetPos, QLatin1String("\n {\n\n}")); // replace ';'

            if (!changeSet) {
                targetFile->setChangeSet(*target);
                targetFile->setOpenEditor(true, targetPos);
                targetFile->apply();

                // Move cursor inside definition
                QTextCursor c = targetFile->cursor();
                c.setPosition(targetPos);
                c.movePosition(QTextCursor::Down);
                c.movePosition(QTextCursor::EndOfLine);
                op->editor()->setTextCursor(c);
            }
        } else {
            // make target lookup context
            Document::Ptr targetDoc = targetFile->cppDocument();
            Scope *targetScope = targetDoc->scopeAt(loc.line(), loc.column());

            // Correct scope in case of a function try-block. See QTCREATORBUG-14661.
            if (targetScope && targetScope->asBlock()) {
                if (Class * const enclosingClass = targetScope->enclosingClass())
                    targetScope = enclosingClass;
                else
                    targetScope = targetScope->enclosingNamespace();
            }

            LookupContext targetContext(targetDoc, op->snapshot());
            ClassOrNamespace *targetCoN = targetContext.lookupType(targetScope);
            if (!targetCoN)
                targetCoN = targetContext.globalNamespace();

            // setup rewriting to get minimally qualified names
            SubstitutionEnvironment env;
            env.setContext(op->context());
            env.switchScope(decl->enclosingScope());
            UseMinimalNames q(targetCoN);
            env.enter(&q);
            Control *control = op->context().bindings()->control().get();

            // rewrite the function type
            const FullySpecifiedType tn = rewriteType(decl->type(), &env, control);

            // rewrite the function name
            if (nameIncludesOperatorName(decl->name())) {
                CppRefactoringFilePtr file = refactoring.cppFile(op->filePath());
                const QString operatorNameText = file->textOf(declAST->core_declarator);
                oo.includeWhiteSpaceInOperatorName = operatorNameText.contains(QLatin1Char(' '));
            }
            const QString name = oo.prettyName(LookupContext::minimalName(decl, targetCoN,
                                                                          control));

            const QString inlinePref = inlinePrefix(targetFilePath, [defPos] {
                return defPos == DefPosOutsideClass;
            });

            const QString prettyType = oo.prettyType(tn, name);

            QString input = prettyType;
            int index = 0;
            while (input.startsWith("template")) {
                QRegularExpression templateRegex("template\\s*<[^>]*>");
                QRegularExpressionMatch match = templateRegex.match(input);
                if (match.hasMatch()) {
                    index += match.captured().size() + 1;
                    input = input.mid(match.captured().size() + 1);
                }
            }

            QString defText = prettyType;
            defText.insert(index, inlinePref);
            defText += QLatin1String("\n{\n\n}");

            ChangeSet localChangeSet;
            ChangeSet * const target = changeSet ? changeSet : &localChangeSet;
            const int targetPos = targetFile->position(loc.line(), loc.column());
            target->insert(targetPos,  loc.prefix() + defText + loc.suffix());

            if (!changeSet) {
                targetFile->setChangeSet(*target);
                targetFile->setOpenEditor(true, targetPos);
                targetFile->apply();

                // Move cursor inside definition
                QTextCursor c = targetFile->cursor();
                c.setPosition(targetPos);
                c.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor,
                               loc.prefix().count(QLatin1String("\n")) + 2);
                c.movePosition(QTextCursor::EndOfLine);
                if (defPos == DefPosImplementationFile) {
                    if (targetFile->editor())
                        targetFile->editor()->setTextCursor(c);
                } else {
                    op->editor()->setTextCursor(c);
                }
            }
        }
    }

private:
    void perform() override
    {
        insertDefinition(this, m_loc, m_defpos, m_declAST, m_decl, m_targetFilePath);
    }

    Declaration *m_decl;
    DeclaratorAST *m_declAST;
    InsertionLocation m_loc;
    const DefPos m_defpos;
    const FilePath m_targetFilePath;
};

} // anonymous namespace

void InsertDefFromDecl::doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();

    int idx = path.size() - 1;
    for (; idx >= 0; --idx) {
        AST *node = path.at(idx);
        if (SimpleDeclarationAST *simpleDecl = node->asSimpleDeclaration()) {
            if (idx > 0 && path.at(idx - 1)->asStatement())
                return;
            if (simpleDecl->symbols && !simpleDecl->symbols->next) {
                if (Symbol *symbol = simpleDecl->symbols->value) {
                    if (Declaration *decl = symbol->asDeclaration()) {
                        if (Function *func = decl->type()->asFunctionType()) {
                            if (func->isSignal() || func->isPureVirtual() || func->isFriend())
                                return;

                            // Check if there is already a definition
                            SymbolFinder symbolFinder;
                            if (symbolFinder.findMatchingDefinition(decl, interface.snapshot(),
                                                                    true)) {
                                return;
                            }

                            // Insert Position: Implementation File
                            DeclaratorAST *declAST = simpleDecl->declarator_list->value;
                            InsertDefOperation *op = nullptr;
                            ProjectFile::Kind kind = ProjectFile::classify(interface.filePath().toString());
                            const bool isHeaderFile = ProjectFile::isHeader(kind);
                            if (isHeaderFile) {
                                CppRefactoringChanges refactoring(interface.snapshot());
                                InsertionPointLocator locator(refactoring);
                                // find appropriate implementation file, but do not use this
                                // location, because insertLocationForMethodDefinition() should
                                // be used in perform() to get consistent insert positions.
                                for (const InsertionLocation &location :
                                     locator.methodDefinition(decl, false, {})) {
                                    if (!location.isValid())
                                        continue;

                                    const FilePath filePath = location.filePath();
                                    if (ProjectFile::isHeader(ProjectFile::classify(filePath.path()))) {
                                        const FilePath source = correspondingHeaderOrSource(filePath);
                                        if (!source.isEmpty()) {
                                            op = new InsertDefOperation(interface, decl, declAST,
                                                                        InsertionLocation(),
                                                                        DefPosImplementationFile,
                                                                        source);
                                        }
                                    } else {
                                        op = new InsertDefOperation(interface, decl, declAST,
                                                                    InsertionLocation(),
                                                                    DefPosImplementationFile,
                                                                    filePath);
                                    }

                                    if (op)
                                        result << op;
                                    break;
                                }
                            }

                            // Determine if we are dealing with a free function
                            const bool isFreeFunction = func->enclosingClass() == nullptr;

                            // Insert Position: Outside Class
                            if (!isFreeFunction || m_defPosOutsideClass) {
                                result << new InsertDefOperation(interface, decl, declAST,
                                                                 InsertionLocation(),
                                                                 DefPosOutsideClass,
                                                                 interface.filePath());
                            }

                            // Insert Position: Inside Class
                            // Determine insert location direct after the declaration.
                            int line, column;
                            const CppRefactoringFilePtr file = interface.currentFile();
                            file->lineAndColumn(file->endOf(simpleDecl), &line, &column);
                            const InsertionLocation loc
                                    = InsertionLocation(interface.filePath(), QString(),
                                                        QString(), line, column);
                            result << new InsertDefOperation(interface, decl, declAST, loc,
                                                             DefPosInsideClass, FilePath(),
                                                             isFreeFunction);

                            return;
                        }
                    }
                }
            }
            break;
        }
    }
}

class InsertMemberFromInitializationOp : public CppQuickFixOperation
{
public:
    InsertMemberFromInitializationOp(
        const CppQuickFixInterface &interface,
        const Class *theClass,
        const NameAST *memberName,
        const TypeOrExpr &typeOrExpr,
        const CallAST *call,
        InsertionPointLocator::AccessSpec accessSpec,
        bool makeStatic,
        bool makeConst)
        : CppQuickFixOperation(interface),
          m_class(theClass), m_memberName(memberName), m_typeOrExpr(typeOrExpr), m_call(call),
          m_accessSpec(accessSpec), m_makeStatic(makeStatic), m_makeConst(makeConst)
    {
        if (call)
            setDescription(Tr::tr("Add Member Function \"%1\"").arg(nameString(memberName)));
        else
            setDescription(Tr::tr("Add Class Member \"%1\"").arg(nameString(memberName)));
    }

private:
    void perform() override
    {
        QString decl = declFromExpr(m_typeOrExpr, m_call, m_memberName, snapshot(), context(),
                                    currentFile(), m_makeConst);
        if (decl.isEmpty())
            return;
        if (m_makeStatic)
            decl.prepend("static ");

        const CppRefactoringChanges refactoring(snapshot());
        const InsertionPointLocator locator(refactoring);
        const FilePath filePath = FilePath::fromUtf8(m_class->fileName());
        const InsertionLocation loc = locator.methodDeclarationInClass(
                    filePath, m_class, m_accessSpec);
        QTC_ASSERT(loc.isValid(), return);

        CppRefactoringFilePtr targetFile = refactoring.cppFile(filePath);
        const int targetPosition = targetFile->position(loc.line(), loc.column());
        ChangeSet target;
        target.insert(targetPosition, loc.prefix() + decl + ";\n");
        targetFile->setChangeSet(target);
        targetFile->apply();
    }

    const Class * const m_class;
    const NameAST * const m_memberName;
    const TypeOrExpr m_typeOrExpr;
    const CallAST * m_call;
    const InsertionPointLocator::AccessSpec m_accessSpec;
    const bool m_makeStatic;
    const bool m_makeConst;
};

void AddDeclarationForUndeclaredIdentifier::doMatch(const CppQuickFixInterface &interface,
                                                    QuickFixOperations &result)
{
    // Are we on a name?
    const QList<AST *> &path = interface.path();
    if (path.isEmpty())
        return;
    if (!path.last()->asSimpleName())
        return;

    // Special case: Member initializer.
    if (!checkForMemberInitializer(interface, result))
        return;

    // Are we inside a function?
    const FunctionDefinitionAST *func = nullptr;
    for (auto it = path.rbegin(); !func && it != path.rend(); ++it)
        func = (*it)->asFunctionDefinition();
    if (!func)
        return;

    // Is this name declared somewhere already?
    const CursorInEditor cursorInEditor(interface.cursor(), interface.filePath(),
                                        interface.editor(), interface.editor()->textDocument());
    const auto followSymbolFallback = [&](const Link &link) {
        if (!link.hasValidTarget())
            collectOperations(interface, result);
    };
    CppModelManager::followSymbol(cursorInEditor, followSymbolFallback, false, false,
                                  FollowSymbolMode::Exact,
                                  CppModelManager::Backend::Builtin);
}

void AddDeclarationForUndeclaredIdentifier::collectOperations(
    const CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();
    const CppRefactoringFilePtr &file = interface.currentFile();
    for (int index = path.size() - 1; index != -1; --index) {
        if (const auto call = path.at(index)->asCall())
            return handleCall(call, interface, result);

        // We only trigger if the identifier appears on the left-hand side of an
        // assignment expression.
        const auto binExpr = path.at(index)->asBinaryExpression();
        if (!binExpr)
            continue;
        if (!binExpr->left_expression || !binExpr->right_expression
            || file->tokenAt(binExpr->binary_op_token).kind() != T_EQUAL
            || !interface.isCursorOn(binExpr->left_expression)) {
            return;
        }

        // In the case of "a.|b = c", find out the type of a, locate the class declaration
        // and add a member b there.
        if (const auto memberAccess = binExpr->left_expression->asMemberAccess()) {
            if (interface.isCursorOn(memberAccess->member_name)
                && memberAccess->member_name == path.last()) {
                maybeAddMember(interface, file->scopeAt(memberAccess->firstToken()),
                               file->textOf(memberAccess->base_expression).toUtf8(),
                               binExpr->right_expression, nullptr, result);
            }
            return;
        }

        const auto idExpr = binExpr->left_expression->asIdExpression();
        if (!idExpr || !idExpr->name)
            return;

        // In the case of "A::|b = c", add a static member b to A.
        if (const auto qualName = idExpr->name->asQualifiedName()) {
            return maybeAddStaticMember(interface, qualName, binExpr->right_expression, nullptr,
                                        result);
        }

        // For an unqualified access, offer a local declaration and, if we are
        // in a member function, a member declaration.
        if (const auto simpleName = idExpr->name->asSimpleName()) {
            if (!m_membersOnly)
                result << new AddLocalDeclarationOp(interface, index, binExpr, simpleName);
            maybeAddMember(interface, file->scopeAt(idExpr->firstToken()), "this",
                           binExpr->right_expression, nullptr, result);
            return;
        }
    }
}

void AddDeclarationForUndeclaredIdentifier::handleCall(
    const CallAST *call, const CppQuickFixInterface &interface,
    TextEditor::QuickFixOperations &result)
{
    if (!call->base_expression)
        return;

    // In order to find out the return type, we need to check the context of the call.
    // If it is a statement expression, the type is void, if it's a binary expression,
    // we assume the type of the other side of the expression, if it's a return statement,
    // we use the return type of the surrounding function, and if it's a declaration,
    // we use the type of the variable. Other cases are not supported.
    const QList<AST *> &path = interface.path();
    const CppRefactoringFilePtr &file = interface.currentFile();
    TypeOrExpr returnTypeOrExpr;
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
        if ((*it)->asCompoundStatement())
            return;
        if ((*it)->asExpressionStatement()) {
            returnTypeOrExpr = FullySpecifiedType(new VoidType);
            break;
        }
        if (const auto binExpr = (*it)->asBinaryExpression()) {
            returnTypeOrExpr = interface.isCursorOn(binExpr->left_expression)
                                 ? binExpr->right_expression : binExpr->left_expression;
            break;
        }
        if (const auto returnExpr = (*it)->asReturnStatement()) {
            for (auto it2 = std::next(it); it2 != path.rend(); ++it2) {
                if (const auto func = (*it2)->asFunctionDefinition()) {
                    if (!func->symbol)
                        return;
                    returnTypeOrExpr = func->symbol->returnType();
                    break;
                }
            }
            break;
        }
        if (const auto declarator = (*it)->asDeclarator()) {
            if (!interface.isCursorOn(declarator->initializer))
                return;
            const auto decl = (*std::next(it))->asSimpleDeclaration();
            if (!decl || !decl->symbols)
                return;
            if (!decl->symbols->value->type().isValid())
                return;
            returnTypeOrExpr = decl->symbols->value->type();
            break;
        }
    }

    if (std::holds_alternative<const ExpressionAST *>(returnTypeOrExpr)
        && !std::get<const ExpressionAST *>(returnTypeOrExpr)) {
        return;
    }

    // a.f()
    if (const auto memberAccess = call->base_expression->asMemberAccess()) {
        if (!interface.isCursorOn(memberAccess->member_name))
            return;
        maybeAddMember(
            interface, file->scopeAt(call->firstToken()),
            file->textOf(memberAccess->base_expression).toUtf8(), returnTypeOrExpr, call, result);
    }

    const auto idExpr = call->base_expression->asIdExpression();
    if (!idExpr || !idExpr->name)
        return;

    // A::f()
    if (const auto qualName = idExpr->name->asQualifiedName())
        return maybeAddStaticMember(interface, qualName, returnTypeOrExpr, call, result);

    // f()
    if (const auto simpleName = idExpr->name->asSimpleName()) {
        maybeAddMember(interface, file->scopeAt(idExpr->firstToken()), "this",
                       returnTypeOrExpr, call, result);
    }
}

bool AddDeclarationForUndeclaredIdentifier::checkForMemberInitializer(
    const CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();
    const int size = path.size();
    if (size < 4)
        return true;
    const MemInitializerAST * const memInitializer = path.at(size - 2)->asMemInitializer();
    if (!memInitializer)
        return true;
    if (!path.at(size - 3)->asCtorInitializer())
        return true;
    const FunctionDefinitionAST * ctor = path.at(size - 4)->asFunctionDefinition();
    if (!ctor)
        return false;

    // Now find the class.
    const Class *theClass = nullptr;
    if (size > 4) {
        const ClassSpecifierAST * const classSpec = path.at(size - 5)->asClassSpecifier();
        if (classSpec) // Inline constructor. We get the class directly.
            theClass = classSpec->symbol;
    }
    if (!theClass) {
        // Out-of-line constructor. We need to find the class.
        SymbolFinder finder;
        const QList<Declaration *> matches = finder.findMatchingDeclaration(
            LookupContext(interface.currentFile()->cppDocument(), interface.snapshot()),
            ctor->symbol);
        if (!matches.isEmpty())
            theClass = matches.first()->enclosingClass();
    }

    if (!theClass)
        return false;

    const SimpleNameAST * const name = path.at(size - 1)->asSimpleName();
    QTC_ASSERT(name, return false);

    // Check whether the member exists already.
    if (theClass->find(interface.currentFile()->cppDocument()->translationUnit()->identifier(
            name->identifier_token))) {
        return false;
    }

    result << new InsertMemberFromInitializationOp(
        interface, theClass, memInitializer->name->asSimpleName(), memInitializer->expression,
        nullptr, InsertionPointLocator::Private, false, false);
    return false;
}

void AddDeclarationForUndeclaredIdentifier::maybeAddMember(
    const CppQuickFixInterface &interface, Scope *scope, const QByteArray &classTypeExpr,
    const TypeOrExpr &typeOrExpr, const CallAST *call, TextEditor::QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();

    TypeOfExpression typeOfExpression;
    typeOfExpression.init(interface.semanticInfo().doc, interface.snapshot(),
                          interface.context().bindings());
    const QList<LookupItem> lhsTypes = typeOfExpression(
        classTypeExpr, scope,
        TypeOfExpression::Preprocess);
    if (lhsTypes.isEmpty())
        return;

    const Type *type = lhsTypes.first().type().type();
    if (!type)
        return;
    if (type->asPointerType()) {
        type = type->asPointerType()->elementType().type();
        if (!type)
            return;
    }
    const auto namedType = type->asNamedType();
    if (!namedType)
        return;
    const ClassOrNamespace * const classOrNamespace
        = interface.context().lookupType(namedType->name(), scope);
    if (!classOrNamespace || !classOrNamespace->rootClass())
        return;

    const Class * const theClass = classOrNamespace->rootClass();
    bool needsStatic = lhsTypes.first().type().isStatic();

    // If the base expression refers to the same class that the member function is in,
    // then we want to insert a private member, otherwise a public one.
    const FunctionDefinitionAST *func = nullptr;
    for (auto it = path.rbegin(); !func && it != path.rend(); ++it)
        func = (*it)->asFunctionDefinition();
    QTC_ASSERT(func, return);
    InsertionPointLocator::AccessSpec accessSpec = InsertionPointLocator::Public;
    for (int i = 0; i < theClass->memberCount(); ++i) {
        if (theClass->memberAt(i) == func->symbol) {
            accessSpec = InsertionPointLocator::Private;
            needsStatic = func->symbol->isStatic();
            break;
        }
    }
    if (accessSpec == InsertionPointLocator::Public) {
        QList<Declaration *> decls;
        QList<Declaration *> dummy;
        SymbolFinder().findMatchingDeclaration(interface.context(), func->symbol, &decls,
                                               &dummy, &dummy);
        for (const Declaration * const decl : std::as_const(decls)) {
            for (int i = 0; i < theClass->memberCount(); ++i) {
                if (theClass->memberAt(i) == decl) {
                    accessSpec = InsertionPointLocator::Private;
                    needsStatic = decl->isStatic();
                    break;
                }
            }
            if (accessSpec == InsertionPointLocator::Private)
                break;
        }
    }
    result << new InsertMemberFromInitializationOp(interface, theClass, path.last()->asName(),
                                                   typeOrExpr, call, accessSpec, needsStatic,
                                                   func->symbol->isConst());
}

void AddDeclarationForUndeclaredIdentifier::maybeAddStaticMember(
    const CppQuickFixInterface &interface, const QualifiedNameAST *qualName,
    const TypeOrExpr &typeOrExpr, const CallAST *call, TextEditor::QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();

    if (!interface.isCursorOn(qualName->unqualified_name))
        return;
    if (qualName->unqualified_name != path.last())
        return;
    if (!qualName->nested_name_specifier_list)
        return;

    const NameAST * const topLevelName
        = qualName->nested_name_specifier_list->value->class_or_namespace_name;
    if (!topLevelName)
        return;
    ClassOrNamespace * const classOrNamespace = interface.context().lookupType(
        topLevelName->name, interface.currentFile()->scopeAt(qualName->firstToken()));
    if (!classOrNamespace)
        return;
    QList<const Name *> otherNames;
    for (auto it = qualName->nested_name_specifier_list->next; it; it = it->next) {
        if (!it->value || !it->value->class_or_namespace_name)
            return;
        otherNames << it->value->class_or_namespace_name->name;
    }

    const Class *theClass = nullptr;
    if (!otherNames.isEmpty()) {
        const Symbol * const symbol = classOrNamespace->lookupInScope(otherNames);
        if (!symbol)
            return;
        theClass = symbol->asClass();
    } else {
        theClass = classOrNamespace->rootClass();
    }
    if (theClass) {
        result << new InsertMemberFromInitializationOp(
            interface, theClass, path.last()->asName(), typeOrExpr, call,
            InsertionPointLocator::Public, true, false);
    }
}

class MemberFunctionImplSetting
{
public:
    Symbol *func = nullptr;
    DefPos defPos = DefPosImplementationFile;
};
using MemberFunctionImplSettings = QList<MemberFunctionImplSetting>;

class AddImplementationsDialog : public QDialog
{
public:
    AddImplementationsDialog(const QList<Symbol *> &candidates, const FilePath &implFile)
        : QDialog(Core::ICore::dialogParent()), m_candidates(candidates)
    {
        setWindowTitle(Tr::tr("Member Function Implementations"));

        const auto defaultImplTargetComboBox = new QComboBox;
        QStringList implTargetStrings{Tr::tr("None"), Tr::tr("Inline"), Tr::tr("Outside Class")};
        if (!implFile.isEmpty())
            implTargetStrings.append(implFile.fileName());
        defaultImplTargetComboBox->insertItems(0, implTargetStrings);
        connect(defaultImplTargetComboBox, &QComboBox::currentIndexChanged, this,
                [this](int index) {
            for (int i = 0; i < m_implTargetBoxes.size(); ++i) {
                if (!m_candidates.at(i)->type()->asFunctionType()->isPureVirtual())
                    static_cast<QComboBox *>(m_implTargetBoxes.at(i))->setCurrentIndex(index);
            }
        });
        const auto defaultImplTargetLayout = new QHBoxLayout;
        defaultImplTargetLayout->addWidget(new QLabel(Tr::tr("Default implementation location:")));
        defaultImplTargetLayout->addWidget(defaultImplTargetComboBox);

        const auto candidatesLayout = new QGridLayout;
        Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
        oo.showFunctionSignatures = true;
        oo.showReturnTypes = true;
        for (int i = 0; i < m_candidates.size(); ++i) {
            const Function * const func = m_candidates.at(i)->type()->asFunctionType();
            QTC_ASSERT(func, continue);
            const auto implTargetComboBox = new QComboBox;
            m_implTargetBoxes.append(implTargetComboBox);
            implTargetComboBox->insertItems(0, implTargetStrings);
            if (func->isPureVirtual())
                implTargetComboBox->setCurrentIndex(0);
            candidatesLayout->addWidget(new QLabel(oo.prettyType(func->type(), func->name())),
                                        i, 0);
            candidatesLayout->addWidget(implTargetComboBox, i, 1);
        }

        const auto buttonBox
                = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        defaultImplTargetComboBox->setCurrentIndex(implTargetStrings.size() - 1);
        const auto mainLayout = new QVBoxLayout(this);
        mainLayout->addLayout(defaultImplTargetLayout);
        mainLayout->addWidget(Layouting::createHr(this));
        mainLayout->addLayout(candidatesLayout);
        mainLayout->addWidget(buttonBox);
    }

    MemberFunctionImplSettings settings() const
    {
        QTC_ASSERT(m_candidates.size() == m_implTargetBoxes.size(), return {});
        MemberFunctionImplSettings settings;
        for (int i = 0; i < m_candidates.size(); ++i) {
            MemberFunctionImplSetting setting;
            const int index = m_implTargetBoxes.at(i)->currentIndex();
            const bool addImplementation = index != 0;
            if (!addImplementation)
                continue;
            setting.func = m_candidates.at(i);
            setting.defPos = static_cast<DefPos>(index - 1);
            settings << setting;
        }
        return settings;
    }

private:
    const QList<Symbol *> m_candidates;
    QList<QComboBox *> m_implTargetBoxes;
};

class InsertDefsOperation: public CppQuickFixOperation
{
public:
    InsertDefsOperation(const CppQuickFixInterface &interface)
        : CppQuickFixOperation(interface)
    {
        setDescription(Tr::tr("Create Implementations for Member Functions"));

        m_classAST = astForClassOperations(interface);
        if (!m_classAST)
            return;
        const Class * const theClass = m_classAST->symbol;
        if (!theClass)
            return;

        // Collect all member functions.
        for (auto it = theClass->memberBegin(); it != theClass->memberEnd(); ++it) {
            Symbol * const s = *it;
            if (!s->identifier() || !s->type() || !s->asDeclaration() || s->asFunction())
                continue;
            Function * const func = s->type()->asFunctionType();
            if (!func || func->isSignal() || func->isFriend())
                continue;
            Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
            oo.showFunctionSignatures = true;
            if (magicQObjectFunctions().contains(oo.prettyName(func->name())))
                continue;
            m_declarations << s;
        }
    }

    bool isApplicable() const { return !m_declarations.isEmpty(); }
    void setMode(InsertDefsFromDecls::Mode mode) { m_mode = mode; }

private:
    void perform() override
    {
        QList<Symbol *> unimplemented;
        SymbolFinder symbolFinder;
        for (Symbol * const s : std::as_const(m_declarations)) {
            if (!symbolFinder.findMatchingDefinition(s, snapshot()))
                unimplemented << s;
        }
        if (unimplemented.isEmpty())
            return;

        CppRefactoringChanges refactoring(snapshot());
        const bool isHeaderFile = ProjectFile::isHeader(ProjectFile::classify(filePath().toString()));
        FilePath cppFile; // Only set if the class is defined in a header file.
        if (isHeaderFile) {
            InsertionPointLocator locator(refactoring);
            for (const InsertionLocation &location
                 : locator.methodDefinition(unimplemented.first(), false, {})) {
                if (!location.isValid())
                    continue;
                const FilePath filePath = location.filePath();
                if (ProjectFile::isHeader(ProjectFile::classify(filePath.path()))) {
                    const FilePath source = correspondingHeaderOrSource(filePath);
                    if (!source.isEmpty())
                        cppFile = source;
                } else {
                    cppFile = filePath;
                }
                break;
            }
        }

        MemberFunctionImplSettings settings;
        switch (m_mode) {
        case InsertDefsFromDecls::Mode::User: {
            AddImplementationsDialog dlg(unimplemented, cppFile);
            if (dlg.exec() == QDialog::Accepted)
                settings = dlg.settings();
            break;
        }
        case InsertDefsFromDecls::Mode::Impl: {
            for (Symbol * const func : std::as_const(unimplemented)) {
                MemberFunctionImplSetting setting;
                setting.func = func;
                setting.defPos = DefPosImplementationFile;
                settings << setting;
            }
            break;
        }
        case InsertDefsFromDecls::Mode::Alternating: {
            int defPos = DefPosImplementationFile;
            const auto incDefPos = [&defPos] {
                defPos = (defPos + 1) % (DefPosImplementationFile + 2);
            };
            for (Symbol * const func : std::as_const(unimplemented)) {
                incDefPos();
                if (defPos > DefPosImplementationFile)
                    continue;
                MemberFunctionImplSetting setting;
                setting.func = func;
                setting.defPos = static_cast<DefPos>(defPos);
                settings << setting;
            }
            break;
        }
        case InsertDefsFromDecls::Mode::Off:
            break;
        }

        if (settings.isEmpty())
            return;

        class DeclFinder : public ASTVisitor
        {
        public:
            DeclFinder(const CppRefactoringFile *file, const Symbol *func)
                : ASTVisitor(file->cppDocument()->translationUnit()), m_func(func) {}

            SimpleDeclarationAST *decl() const { return m_decl; }

        private:
            bool visit(SimpleDeclarationAST *decl) override
            {
                if (m_decl)
                    return false;
                if (decl->symbols && decl->symbols->value == m_func)
                    m_decl = decl;
                return !m_decl;
            }

            const Symbol * const m_func;
            SimpleDeclarationAST *m_decl = nullptr;
        };

        QHash<FilePath, ChangeSet> changeSets;
        for (const MemberFunctionImplSetting &setting : std::as_const(settings)) {
            DeclFinder finder(currentFile().data(), setting.func);
            finder.accept(m_classAST);
            QTC_ASSERT(finder.decl(), continue);
            InsertionLocation loc;
            const FilePath targetFilePath = setting.defPos == DefPosImplementationFile
                    ? cppFile : filePath();
            QTC_ASSERT(!targetFilePath.isEmpty(), continue);
            if (setting.defPos == DefPosInsideClass) {
                int line, column;
                currentFile()->lineAndColumn(currentFile()->endOf(finder.decl()), &line, &column);
                loc = InsertionLocation(filePath(), QString(), QString(), line, column);
            }
            ChangeSet &changeSet = changeSets[targetFilePath];
            InsertDefOperation::insertDefinition(
                        this, loc, setting.defPos, finder.decl()->declarator_list->value,
                        setting.func->asDeclaration(),targetFilePath, &changeSet);
        }
        for (auto it = changeSets.cbegin(); it != changeSets.cend(); ++it) {
            const CppRefactoringFilePtr file = refactoring.cppFile(it.key());
            file->setChangeSet(it.value());
            file->apply();
        }
    }

    ClassSpecifierAST *m_classAST = nullptr;
    InsertDefsFromDecls::Mode m_mode;
    QList<Symbol *> m_declarations;
};


void InsertDefsFromDecls::doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const auto op = QSharedPointer<InsertDefsOperation>::create(interface);
    op->setMode(m_mode);
    if (op->isApplicable())
        result << op;
}

namespace {

class ExtractFunctionOptions
{
public:
    static bool isValidFunctionName(const QString &name)
    {
        return !name.isEmpty() && isValidIdentifier(name);
    }

    bool hasValidFunctionName() const
    {
        return isValidFunctionName(funcName);
    }

    QString funcName;
    InsertionPointLocator::AccessSpec access = InsertionPointLocator::Public;
};

class ExtractFunctionOperation : public CppQuickFixOperation
{
public:
    ExtractFunctionOperation(const CppQuickFixInterface &interface,
                    int extractionStart,
                    int extractionEnd,
                    FunctionDefinitionAST *refFuncDef,
                    Symbol *funcReturn,
                    QList<QPair<QString, QString> > relevantDecls,
                    ExtractFunction::FunctionNameGetter functionNameGetter
                             = ExtractFunction::FunctionNameGetter())
        : CppQuickFixOperation(interface)
        , m_extractionStart(extractionStart)
        , m_extractionEnd(extractionEnd)
        , m_refFuncDef(refFuncDef)
        , m_funcReturn(funcReturn)
        , m_relevantDecls(relevantDecls)
        , m_functionNameGetter(functionNameGetter)
    {
        setDescription(Tr::tr("Extract Function"));
    }

    void perform() override
    {
        QTC_ASSERT(!m_funcReturn || !m_relevantDecls.isEmpty(), return);
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());

        ExtractFunctionOptions options;
        if (m_functionNameGetter)
            options.funcName = m_functionNameGetter();
        else
            options = getOptions();

        if (!options.hasValidFunctionName())
            return;
        const QString &funcName = options.funcName;

        Function *refFunc = m_refFuncDef->symbol;

        // We don't need to rewrite the type for declarations made inside the reference function,
        // since their scope will remain the same. Then we preserve the original spelling style.
        // However, we must do so for the return type in the definition.
        SubstitutionEnvironment env;
        env.setContext(context());
        env.switchScope(refFunc);
        ClassOrNamespace *targetCoN = context().lookupType(refFunc->enclosingScope());
        if (!targetCoN)
            targetCoN = context().globalNamespace();
        UseMinimalNames subs(targetCoN);
        env.enter(&subs);

        Overview printer = CppCodeStyleSettings::currentProjectCodeStyleOverview();
        Control *control = context().bindings()->control().get();
        QString funcDef;
        QString funcDecl; // We generate a declaration only in the case of a member function.
        QString funcCall;

        Class *matchingClass = isMemberFunction(context(), refFunc);

        // Write return type.
        if (!m_funcReturn) {
            funcDef.append(QLatin1String("void "));
            if (matchingClass)
                funcDecl.append(QLatin1String("void "));
        } else {
            const FullySpecifiedType &fullType = rewriteType(m_funcReturn->type(), &env, control);
            funcDef.append(printer.prettyType(fullType) + QLatin1Char(' '));
            funcDecl.append(printer.prettyType(m_funcReturn->type()) + QLatin1Char(' '));
        }

        // Write class qualification, if any.
        if (matchingClass) {
            const Scope *current = matchingClass;
            QVector<const Name *> classes{matchingClass->name()};
            while (current->enclosingScope()->asClass()) {
                current = current->enclosingScope()->asClass();
                classes.prepend(current->name());
            }
            while (current->enclosingScope() && current->enclosingScope()->asNamespace()) {
                current = current->enclosingScope()->asNamespace();
                if (current->name())
                    classes.prepend(current->name());
            }
            for (const Name *n : classes) {
                const Name *name = rewriteName(n, &env, control);
                funcDef.append(printer.prettyName(name));
                funcDef.append(QLatin1String("::"));
            }
        }

        // Write the extracted function itself and its call.
        funcDef.append(funcName);
        if (matchingClass)
            funcDecl.append(funcName);
        funcCall.append(funcName);
        funcDef.append(QLatin1Char('('));
        if (matchingClass)
            funcDecl.append(QLatin1Char('('));
        funcCall.append(QLatin1Char('('));
        for (int i = m_funcReturn ? 1 : 0; i < m_relevantDecls.length(); ++i) {
            QPair<QString, QString> p = m_relevantDecls.at(i);
            funcCall.append(p.first);
            funcDef.append(p.second);
            if (matchingClass)
                funcDecl.append(p.second);
            if (i < m_relevantDecls.length() - 1) {
                funcCall.append(QLatin1String(", "));
                funcDef.append(QLatin1String(", "));
                if (matchingClass)
                    funcDecl.append(QLatin1String(", "));
            }
        }
        funcDef.append(QLatin1Char(')'));
        if (matchingClass)
            funcDecl.append(QLatin1Char(')'));
        funcCall.append(QLatin1Char(')'));
        if (refFunc->isConst()) {
            funcDef.append(QLatin1String(" const"));
            funcDecl.append(QLatin1String(" const"));
        }
        funcDef.append(QLatin1String("\n{\n"));
        QString extract = currentFile->textOf(m_extractionStart, m_extractionEnd);
        extract.replace(QChar::ParagraphSeparator, QLatin1String("\n"));
        if (!extract.endsWith(QLatin1Char('\n')) && m_funcReturn)
            extract.append(QLatin1Char('\n'));
        funcDef.append(extract);
        if (matchingClass)
            funcDecl.append(QLatin1String(";\n"));
        if (m_funcReturn) {
            funcDef.append(QLatin1String("\nreturn ")
                        + m_relevantDecls.at(0).first
                        + QLatin1Char(';'));
            funcCall.prepend(m_relevantDecls.at(0).second + QLatin1String(" = "));
        }
        funcDef.append(QLatin1String("\n}\n\n"));
        funcDef.replace(QChar::ParagraphSeparator, QLatin1String("\n"));
        funcDef.prepend(inlinePrefix(currentFile->filePath()));
        funcCall.append(QLatin1Char(';'));

        // Do not insert right between the function and an associated comment.
        int position = currentFile->startOf(m_refFuncDef);
        const QList<Token> functionDoc = commentsForDeclaration(
            m_refFuncDef->symbol, m_refFuncDef, *currentFile->document(),
            currentFile->cppDocument());
        if (!functionDoc.isEmpty()) {
            position = currentFile->cppDocument()->translationUnit()->getTokenPositionInDocument(
                functionDoc.first(), currentFile->document());
        }

        ChangeSet change;
        change.insert(position, funcDef);
        change.replace(m_extractionStart, m_extractionEnd, funcCall);
        currentFile->setChangeSet(change);
        currentFile->apply();

        // Write declaration, if necessary.
        if (matchingClass) {
            InsertionPointLocator locator(refactoring);
            const FilePath filePath = FilePath::fromUtf8(matchingClass->fileName());
            const InsertionLocation &location =
                    locator.methodDeclarationInClass(filePath, matchingClass, options.access);
            CppRefactoringFilePtr declFile = refactoring.cppFile(filePath);
            change.clear();
            position = declFile->position(location.line(), location.column());
            change.insert(position, location.prefix() + funcDecl + location.suffix());
            declFile->setChangeSet(change);
            declFile->apply();
        }
    }

    ExtractFunctionOptions getOptions() const
    {
        QDialog dlg(Core::ICore::dialogParent());
        dlg.setWindowTitle(Tr::tr("Extract Function Refactoring"));
        auto layout = new QFormLayout(&dlg);

        auto funcNameEdit = new FancyLineEdit;
        funcNameEdit->setValidationFunction([](FancyLineEdit *edit, QString *) {
            return ExtractFunctionOptions::isValidFunctionName(edit->text());
        });
        layout->addRow(Tr::tr("Function name"), funcNameEdit);

        auto accessCombo = new QComboBox;
        accessCombo->addItem(
                    InsertionPointLocator::accessSpecToString(InsertionPointLocator::Public),
                    InsertionPointLocator::Public);
        accessCombo->addItem(
                    InsertionPointLocator::accessSpecToString(InsertionPointLocator::PublicSlot),
                    InsertionPointLocator::PublicSlot);
        accessCombo->addItem(
                    InsertionPointLocator::accessSpecToString(InsertionPointLocator::Protected),
                    InsertionPointLocator::Protected);
        accessCombo->addItem(
                    InsertionPointLocator::accessSpecToString(InsertionPointLocator::ProtectedSlot),
                    InsertionPointLocator::ProtectedSlot);
        accessCombo->addItem(
                    InsertionPointLocator::accessSpecToString(InsertionPointLocator::Private),
                    InsertionPointLocator::Private);
        accessCombo->addItem(
                    InsertionPointLocator::accessSpecToString(InsertionPointLocator::PrivateSlot),
                    InsertionPointLocator::PrivateSlot);
        layout->addRow(Tr::tr("Access"), accessCombo);

        auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        QPushButton *ok = buttonBox->button(QDialogButtonBox::Ok);
        ok->setEnabled(false);
        QObject::connect(funcNameEdit, &Utils::FancyLineEdit::validChanged,
                         ok, &QPushButton::setEnabled);
        layout->addWidget(buttonBox);

        if (dlg.exec() == QDialog::Accepted) {
            ExtractFunctionOptions options;
            options.funcName = funcNameEdit->text();
            options.access = static_cast<InsertionPointLocator::AccessSpec>(accessCombo->
                                                                           currentData().toInt());
            return options;
        }
        return ExtractFunctionOptions();
    }

    int m_extractionStart;
    int m_extractionEnd;
    FunctionDefinitionAST *m_refFuncDef;
    Symbol *m_funcReturn;
    QList<QPair<QString, QString> > m_relevantDecls;
    ExtractFunction::FunctionNameGetter m_functionNameGetter;
};

QPair<QString, QString> assembleDeclarationData(const QString &specifiers, DeclaratorAST *decltr,
                                                const CppRefactoringFilePtr &file,
                                                const Overview &printer)
{
    QTC_ASSERT(decltr, return (QPair<QString, QString>()));
    if (decltr->core_declarator
            && decltr->core_declarator->asDeclaratorId()
            && decltr->core_declarator->asDeclaratorId()->name) {
        QString decltrText = file->textOf(file->startOf(decltr),
                                          file->endOf(decltr->core_declarator));
        if (!decltrText.isEmpty()) {
            const QString &name = printer.prettyName(
                    decltr->core_declarator->asDeclaratorId()->name->name);
            QString completeDecl = specifiers;
            if (!decltrText.contains(QLatin1Char(' ')))
                completeDecl.append(QLatin1Char(' ') + decltrText);
            else
                completeDecl.append(decltrText);
            return {name, completeDecl};
        }
    }
    return QPair<QString, QString>();
}

class FunctionExtractionAnalyser : public ASTVisitor
{
public:
    FunctionExtractionAnalyser(TranslationUnit *unit,
                               const int selStart,
                               const int selEnd,
                               const CppRefactoringFilePtr &file,
                               const Overview &printer)
        : ASTVisitor(unit)
        , m_done(false)
        , m_failed(false)
        , m_selStart(selStart)
        , m_selEnd(selEnd)
        , m_extractionStart(0)
        , m_extractionEnd(0)
        , m_file(file)
        , m_printer(printer)
    {}

    bool operator()(FunctionDefinitionAST *refFunDef)
    {
        accept(refFunDef);

        if (!m_failed && m_extractionStart == m_extractionEnd)
            m_failed = true;

        return !m_failed;
    }

    bool preVisit(AST *) override
    {
        return !m_done;
    }

    void statement(StatementAST *stmt)
    {
        if (!stmt)
            return;

        const int stmtStart = m_file->startOf(stmt);
        const int stmtEnd = m_file->endOf(stmt);

        if (stmtStart >= m_selEnd
                || (m_extractionStart && stmtEnd > m_selEnd)) {
            m_done = true;
            return;
        }

        if (stmtStart >= m_selStart && !m_extractionStart)
            m_extractionStart = stmtStart;
        if (stmtEnd > m_extractionEnd && m_extractionStart)
            m_extractionEnd = stmtEnd;

        accept(stmt);
    }

    bool visit(CaseStatementAST *stmt) override
    {
        statement(stmt->statement);
        return false;
    }

    bool visit(CompoundStatementAST *stmt) override
    {
        for (StatementListAST *it = stmt->statement_list; it; it = it->next) {
            statement(it->value);
            if (m_done)
                break;
        }
        return false;
    }

    bool visit(DoStatementAST *stmt) override
    {
        statement(stmt->statement);
        return false;
    }

    bool visit(ForeachStatementAST *stmt) override
    {
        statement(stmt->statement);
        return false;
    }

    bool visit(RangeBasedForStatementAST *stmt) override
    {
        statement(stmt->statement);
        return false;
    }

    bool visit(ForStatementAST *stmt) override
    {
        statement(stmt->initializer);
        if (!m_done)
            statement(stmt->statement);
        return false;
    }

    bool visit(IfStatementAST *stmt) override
    {
        statement(stmt->statement);
        if (!m_done)
            statement(stmt->else_statement);
        return false;
    }

    bool visit(TryBlockStatementAST *stmt) override
    {
        statement(stmt->statement);
        for (CatchClauseListAST *it = stmt->catch_clause_list; it; it = it->next) {
            statement(it->value);
            if (m_done)
                break;
        }
        return false;
    }

    bool visit(WhileStatementAST *stmt) override
    {
        statement(stmt->statement);
        return false;
    }

    bool visit(DeclarationStatementAST *declStmt) override
    {
        // We need to collect the declarations we see before the extraction or even inside it.
        // They might need to be used as either a parameter or return value. Actually, we could
        // still obtain their types from the local uses, but it's good to preserve the original
        // typing style.
        if (declStmt
                && declStmt->declaration
                && declStmt->declaration->asSimpleDeclaration()) {
            SimpleDeclarationAST *simpleDecl = declStmt->declaration->asSimpleDeclaration();
            if (simpleDecl->decl_specifier_list
                    && simpleDecl->declarator_list) {
                const QString &specifiers =
                        m_file->textOf(m_file->startOf(simpleDecl),
                                     m_file->endOf(simpleDecl->decl_specifier_list->lastValue()));
                for (DeclaratorListAST *decltrList = simpleDecl->declarator_list;
                     decltrList;
                     decltrList = decltrList->next) {
                    const QPair<QString, QString> p =
                        assembleDeclarationData(specifiers, decltrList->value, m_file, m_printer);
                    if (!p.first.isEmpty())
                        m_knownDecls.insert(p.first, p.second);
                }
            }
        }

        return false;
    }

    bool visit(ReturnStatementAST *) override
    {
        if (m_extractionStart) {
            m_done = true;
            m_failed = true;
        }

        return false;
    }

    bool m_done;
    bool m_failed;
    const int m_selStart;
    const int m_selEnd;
    int m_extractionStart;
    int m_extractionEnd;
    QHash<QString, QString> m_knownDecls;
    CppRefactoringFilePtr m_file;
    const Overview &m_printer;
};

} // anonymous namespace

ExtractFunction::ExtractFunction(FunctionNameGetter functionNameGetter)
    : m_functionNameGetter(functionNameGetter)
{
}

void ExtractFunction::doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const CppRefactoringFilePtr file = interface.currentFile();

    // TODO: Fix upstream and uncomment; see QTCREATORBUG-28030.
//    if (CppModelManager::usesClangd(file->editor()->textDocument())
//            && file->cppDocument()->languageFeatures().cxxEnabled) {
//        return;
//    }

    QTextCursor cursor = file->cursor();
    if (!cursor.hasSelection())
        return;

    const QList<AST *> &path = interface.path();
    FunctionDefinitionAST *refFuncDef = nullptr; // The "reference" function, which we will extract from.
    for (int i = path.size() - 1; i >= 0; --i) {
        refFuncDef = path.at(i)->asFunctionDefinition();
        if (refFuncDef)
            break;
    }

    if (!refFuncDef
            || !refFuncDef->function_body
            || !refFuncDef->function_body->asCompoundStatement()
            || !refFuncDef->function_body->asCompoundStatement()->statement_list
            || !refFuncDef->symbol
            || !refFuncDef->symbol->name()
            || refFuncDef->symbol->enclosingScope()->asTemplate() /* TODO: Templates... */) {
        return;
    }

    // Adjust selection ends.
    int selStart = cursor.selectionStart();
    int selEnd = cursor.selectionEnd();
    if (selStart > selEnd)
        std::swap(selStart, selEnd);

    Overview printer;

    // Analyze the content to be extracted, which consists of determining the statements
    // which are complete and collecting the declarations seen.
    FunctionExtractionAnalyser analyser(interface.semanticInfo().doc->translationUnit(),
                                        selStart, selEnd,
                                        file,
                                        printer);
    if (!analyser(refFuncDef))
        return;

    // We also need to collect the declarations of the parameters from the reference function.
    QSet<QString> refFuncParams;
    if (refFuncDef->declarator->postfix_declarator_list
            && refFuncDef->declarator->postfix_declarator_list->value
            && refFuncDef->declarator->postfix_declarator_list->value->asFunctionDeclarator()) {
        FunctionDeclaratorAST *funcDecltr =
            refFuncDef->declarator->postfix_declarator_list->value->asFunctionDeclarator();
        if (funcDecltr->parameter_declaration_clause
                && funcDecltr->parameter_declaration_clause->parameter_declaration_list) {
            for (ParameterDeclarationListAST *it =
                    funcDecltr->parameter_declaration_clause->parameter_declaration_list;
                 it;
                 it = it->next) {
                ParameterDeclarationAST *paramDecl = it->value->asParameterDeclaration();
                if (paramDecl->declarator) {
                    const QString &specifiers =
                            file->textOf(file->startOf(paramDecl),
                                         file->endOf(paramDecl->type_specifier_list->lastValue()));
                    const QPair<QString, QString> &p =
                            assembleDeclarationData(specifiers, paramDecl->declarator,
                                                    file, printer);
                    if (!p.first.isEmpty()) {
                        analyser.m_knownDecls.insert(p.first, p.second);
                        refFuncParams.insert(p.first);
                    }
                }
            }
        }
    }

    // Identify what would be parameters for the new function and its return value, if any.
    Symbol *funcReturn = nullptr;
    QList<QPair<QString, QString> > relevantDecls;
    const SemanticInfo::LocalUseMap localUses = interface.semanticInfo().localUses;
    for (auto it = localUses.cbegin(), end = localUses.cend(); it != end; ++it) {
        bool usedBeforeExtraction = false;
        bool usedAfterExtraction = false;
        bool usedInsideExtraction = false;
        const QList<SemanticInfo::Use> &uses = it.value();
        for (const SemanticInfo::Use &use : uses) {
            if (use.isInvalid())
                continue;

            const int position = file->position(use.line, use.column);
            if (position < analyser.m_extractionStart)
                usedBeforeExtraction = true;
            else if (position >= analyser.m_extractionEnd)
                usedAfterExtraction = true;
            else
                usedInsideExtraction = true;
        }

        const QString &name = printer.prettyName(it.key()->name());

        if ((usedBeforeExtraction && usedInsideExtraction)
                || (usedInsideExtraction && refFuncParams.contains(name))) {
            QTC_ASSERT(analyser.m_knownDecls.contains(name), return);
            relevantDecls.push_back({name, analyser.m_knownDecls.value(name)});
        }

        // We assume that the first use of a local corresponds to its declaration.
        if (usedInsideExtraction && usedAfterExtraction && !usedBeforeExtraction) {
            if (!funcReturn) {
                QTC_ASSERT(analyser.m_knownDecls.contains(name), return);
                // The return, if any, is stored as the first item in the list.
                relevantDecls.push_front({name, analyser.m_knownDecls.value(name)});
                funcReturn = it.key();
            } else {
                // Would require multiple returns. (Unless we do fancy things, as pointed below.)
                return;
            }
        }
    }

    // The current implementation doesn't try to be too smart since it preserves the original form
    // of the declarations. This might be or not the desired effect. An improvement would be to
    // let the user somehow customize the function interface.
    result << new ExtractFunctionOperation(interface,
                                           analyser.m_extractionStart,
                                           analyser.m_extractionEnd,
                                           refFuncDef, funcReturn, relevantDecls,
                                           m_functionNameGetter);
}

namespace {

struct ReplaceLiteralsResult
{
    Token token;
    QString literalText;
};

template <class T>
class ReplaceLiterals : private ASTVisitor
{
public:
    ReplaceLiterals(const CppRefactoringFilePtr &file, ChangeSet *changes, T *literal)
        : ASTVisitor(file->cppDocument()->translationUnit()), m_file(file), m_changes(changes),
          m_literal(literal)
    {
        m_result.token = m_file->tokenAt(literal->firstToken());
        m_literalTokenText = m_result.token.spell();
        m_result.literalText = QLatin1String(m_literalTokenText);
        if (m_result.token.isCharLiteral()) {
            m_result.literalText.prepend(QLatin1Char('\''));
            m_result.literalText.append(QLatin1Char('\''));
            if (m_result.token.kind() == T_WIDE_CHAR_LITERAL)
                m_result.literalText.prepend(QLatin1Char('L'));
            else if (m_result.token.kind() == T_UTF16_CHAR_LITERAL)
                m_result.literalText.prepend(QLatin1Char('u'));
            else if (m_result.token.kind() == T_UTF32_CHAR_LITERAL)
                m_result.literalText.prepend(QLatin1Char('U'));
        } else if (m_result.token.isStringLiteral()) {
            m_result.literalText.prepend(QLatin1Char('"'));
            m_result.literalText.append(QLatin1Char('"'));
            if (m_result.token.kind() == T_WIDE_STRING_LITERAL)
                m_result.literalText.prepend(QLatin1Char('L'));
            else if (m_result.token.kind() == T_UTF16_STRING_LITERAL)
                m_result.literalText.prepend(QLatin1Char('u'));
            else if (m_result.token.kind() == T_UTF32_STRING_LITERAL)
                m_result.literalText.prepend(QLatin1Char('U'));
        }
    }

    ReplaceLiteralsResult apply(AST *ast)
    {
        ast->accept(this);
        return m_result;
    }

private:
    bool visit(T *ast) override
    {
        if (ast != m_literal
                && strcmp(m_file->tokenAt(ast->firstToken()).spell(), m_literalTokenText) != 0) {
            return true;
        }
        int start, end;
        m_file->startAndEndOf(ast->firstToken(), &start, &end);
        m_changes->replace(start, end, QLatin1String("newParameter"));
        return true;
    }

    const CppRefactoringFilePtr &m_file;
    ChangeSet *m_changes;
    T *m_literal;
    const char *m_literalTokenText;
    ReplaceLiteralsResult m_result;
};

class ExtractLiteralAsParameterOp : public CppQuickFixOperation
{
public:
    ExtractLiteralAsParameterOp(const CppQuickFixInterface &interface, int priority,
                                ExpressionAST *literal, FunctionDefinitionAST *function)
        : CppQuickFixOperation(interface, priority),
          m_literal(literal),
          m_functionDefinition(function)
    {
        setDescription(Tr::tr("Extract Constant as Function Parameter"));
    }

    struct FoundDeclaration
    {
        FunctionDeclaratorAST *ast = nullptr;
        CppRefactoringFilePtr file;
    };

    FoundDeclaration findDeclaration(const CppRefactoringChanges &refactoring,
                                     FunctionDefinitionAST *ast)
    {
        FoundDeclaration result;
        Function *func = ast->symbol;
        if (Class *matchingClass = isMemberFunction(context(), func)) {
            // Dealing with member functions
            const QualifiedNameId *qName = func->name()->asQualifiedNameId();
            for (Symbol *s = matchingClass->find(qName->identifier()); s; s = s->next()) {
                if (!s->name()
                        || !qName->identifier()->match(s->identifier())
                        || !s->type()->asFunctionType()
                        || !s->type().match(func->type())
                        || s->asFunction()) {
                    continue;
                }

                const FilePath declFilePath = matchingClass->filePath();
                result.file = refactoring.cppFile(declFilePath);
                ASTPath astPath(result.file->cppDocument());
                const QList<AST *> path = astPath(s->line(), s->column());
                SimpleDeclarationAST *simpleDecl = nullptr;
                for (AST *node : path) {
                    simpleDecl = node->asSimpleDeclaration();
                    if (simpleDecl) {
                        if (simpleDecl->symbols && !simpleDecl->symbols->next) {
                            result.ast = functionDeclarator(simpleDecl);
                            return result;
                        }
                    }
                }

                if (simpleDecl)
                    break;
            }
        } else if (Namespace *matchingNamespace = isNamespaceFunction(context(), func)) {
            // Dealing with free functions and inline member functions.
            bool isHeaderFile;
            FilePath declFilePath = correspondingHeaderOrSource(filePath(), &isHeaderFile);
            if (!declFilePath.exists())
                return FoundDeclaration();
            result.file = refactoring.cppFile(declFilePath);
            if (!result.file)
                return FoundDeclaration();
            const LookupContext lc(result.file->cppDocument(), snapshot());
            const QList<LookupItem> candidates = lc.lookup(func->name(), matchingNamespace);
            for (const LookupItem &candidate : candidates) {
                if (Symbol *s = candidate.declaration()) {
                    if (s->asDeclaration()) {
                        ASTPath astPath(result.file->cppDocument());
                        const QList<AST *> path = astPath(s->line(), s->column());
                        for (AST *node : path) {
                            SimpleDeclarationAST *simpleDecl = node->asSimpleDeclaration();
                            if (simpleDecl) {
                                result.ast = functionDeclarator(simpleDecl);
                                return result;
                            }
                        }
                    }
                }
            }
        }
        return result;
    }

    void perform() override
    {
        FunctionDeclaratorAST *functionDeclaratorOfDefinition
                = functionDeclarator(m_functionDefinition);
        const CppRefactoringChanges refactoring(snapshot());
        const CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());
        deduceTypeNameOfLiteral(currentFile->cppDocument());

        ChangeSet changes;
        if (NumericLiteralAST *concreteLiteral = m_literal->asNumericLiteral()) {
            m_literalInfo = ReplaceLiterals<NumericLiteralAST>(currentFile, &changes,
                                                               concreteLiteral)
                    .apply(m_functionDefinition->function_body);
        } else if (StringLiteralAST *concreteLiteral = m_literal->asStringLiteral()) {
            m_literalInfo = ReplaceLiterals<StringLiteralAST>(currentFile, &changes,
                                                              concreteLiteral)
                    .apply(m_functionDefinition->function_body);
        } else if (BoolLiteralAST *concreteLiteral = m_literal->asBoolLiteral()) {
            m_literalInfo = ReplaceLiterals<BoolLiteralAST>(currentFile, &changes,
                                                            concreteLiteral)
                    .apply(m_functionDefinition->function_body);
        }
        const FoundDeclaration functionDeclaration
                = findDeclaration(refactoring, m_functionDefinition);
        appendFunctionParameter(functionDeclaratorOfDefinition, currentFile, &changes,
                !functionDeclaration.ast);
        if (functionDeclaration.ast) {
            if (currentFile->filePath() != functionDeclaration.file->filePath()) {
                ChangeSet declChanges;
                appendFunctionParameter(functionDeclaration.ast, functionDeclaration.file, &declChanges,
                                        true);
                functionDeclaration.file->setChangeSet(declChanges);
                functionDeclaration.file->apply();
            } else {
                appendFunctionParameter(functionDeclaration.ast, currentFile, &changes,
                                        true);
            }
        }
        currentFile->setChangeSet(changes);
        currentFile->apply();
        QTextCursor c = currentFile->cursor();
        c.setPosition(c.position() - parameterName().length());
        editor()->setTextCursor(c);
        editor()->renameSymbolUnderCursor();
    }

private:
    bool hasParameters(FunctionDeclaratorAST *ast) const
    {
        return ast->parameter_declaration_clause
                && ast->parameter_declaration_clause->parameter_declaration_list
                && ast->parameter_declaration_clause->parameter_declaration_list->value;
    }

    void deduceTypeNameOfLiteral(const Document::Ptr &document)
    {
        TypeOfExpression typeOfExpression;
        typeOfExpression.init(document, snapshot());
        Overview overview;
        Scope *scope = m_functionDefinition->symbol->enclosingScope();
        const QList<LookupItem> items = typeOfExpression(m_literal, document, scope);
        if (!items.isEmpty())
            m_typeName = overview.prettyType(items.first().type());
    }

    static QString parameterName() { return QLatin1String("newParameter"); }

    QString parameterDeclarationTextToInsert(FunctionDeclaratorAST *ast) const
    {
        QString str;
        if (hasParameters(ast))
            str = QLatin1String(", ");
        str += m_typeName;
        if (!m_typeName.endsWith(QLatin1Char('*')))
                str += QLatin1Char(' ');
        str += parameterName();
        return str;
    }

    FunctionDeclaratorAST *functionDeclarator(SimpleDeclarationAST *ast) const
    {
        for (DeclaratorListAST *decls = ast->declarator_list; decls; decls = decls->next) {
            FunctionDeclaratorAST * const functionDeclaratorAST = functionDeclarator(decls->value);
            if (functionDeclaratorAST)
                return functionDeclaratorAST;
        }
        return nullptr;
    }

    FunctionDeclaratorAST *functionDeclarator(DeclaratorAST *ast) const
    {
        for (PostfixDeclaratorListAST *pds = ast->postfix_declarator_list; pds; pds = pds->next) {
            FunctionDeclaratorAST *funcdecl = pds->value->asFunctionDeclarator();
            if (funcdecl)
                return funcdecl;
        }
        return nullptr;
    }

    FunctionDeclaratorAST *functionDeclarator(FunctionDefinitionAST *ast) const
    {
        return functionDeclarator(ast->declarator);
    }

    void appendFunctionParameter(FunctionDeclaratorAST *ast, const CppRefactoringFileConstPtr &file,
               ChangeSet *changes, bool addDefaultValue)
    {
        if (!ast)
            return;
        if (m_declarationInsertionString.isEmpty())
            m_declarationInsertionString = parameterDeclarationTextToInsert(ast);
        QString insertion = m_declarationInsertionString;
        if (addDefaultValue)
            insertion += QLatin1String(" = ") + m_literalInfo.literalText;
        changes->insert(file->startOf(ast->rparen_token), insertion);
    }

    ExpressionAST *m_literal;
    FunctionDefinitionAST *m_functionDefinition;
    QString m_typeName;
    QString m_declarationInsertionString;
    ReplaceLiteralsResult m_literalInfo;
};

} // anonymous namespace

void ExtractLiteralAsParameter::doMatch(const CppQuickFixInterface &interface,
        QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();
    if (path.count() < 2)
        return;

    AST * const lastAst = path.last();
    ExpressionAST *literal;
    if (!((literal = lastAst->asNumericLiteral())
          || (literal = lastAst->asStringLiteral())
          || (literal = lastAst->asBoolLiteral()))) {
            return;
    }

    FunctionDefinitionAST *function;
    int i = path.count() - 2;
    while (!(function = path.at(i)->asFunctionDefinition())) {
        // Ignore literals in lambda expressions for now.
        if (path.at(i)->asLambdaExpression())
            return;
        if (--i < 0)
            return;
    }

    PostfixDeclaratorListAST * const declaratorList = function->declarator->postfix_declarator_list;
    if (!declaratorList)
        return;
    if (FunctionDeclaratorAST *declarator = declaratorList->value->asFunctionDeclarator()) {
        if (declarator->parameter_declaration_clause
                && declarator->parameter_declaration_clause->dot_dot_dot_token) {
            // Do not handle functions with ellipsis parameter.
            return;
        }
    }

    const int priority = path.size() - 1;
    result << new ExtractLiteralAsParameterOp(interface, priority, literal, function);
}

namespace {

class ConvertFromAndToPointerOp : public CppQuickFixOperation
{
public:
    enum Mode { FromPointer, FromVariable, FromReference };

    ConvertFromAndToPointerOp(const CppQuickFixInterface &interface, int priority, Mode mode,
                              bool isAutoDeclaration,
                              const SimpleDeclarationAST *simpleDeclaration,
                              const DeclaratorAST *declaratorAST,
                              const SimpleNameAST *identifierAST,
                              Symbol *symbol)
        : CppQuickFixOperation(interface, priority)
        , m_mode(mode)
        , m_isAutoDeclaration(isAutoDeclaration)
        , m_simpleDeclaration(simpleDeclaration)
        , m_declaratorAST(declaratorAST)
        , m_identifierAST(identifierAST)
        , m_symbol(symbol)
        , m_refactoring(snapshot())
        , m_file(m_refactoring.cppFile(filePath()))
        , m_document(interface.semanticInfo().doc)
    {
        setDescription(
                mode == FromPointer
                ? Tr::tr("Convert to Stack Variable")
                : Tr::tr("Convert to Pointer"));
    }

    void perform() override
    {
        ChangeSet changes;

        switch (m_mode) {
        case FromPointer:
            removePointerOperator(changes);
            convertToStackVariable(changes);
            break;
        case FromReference:
            removeReferenceOperator(changes);
            Q_FALLTHROUGH();
        case FromVariable:
            convertToPointer(changes);
            break;
        }

        m_file->setChangeSet(changes);
        m_file->apply();
    }

private:
    void removePointerOperator(ChangeSet &changes) const
    {
        if (!m_declaratorAST->ptr_operator_list)
            return;
        PointerAST *ptrAST = m_declaratorAST->ptr_operator_list->value->asPointer();
        QTC_ASSERT(ptrAST, return);
        const int pos = m_file->startOf(ptrAST->star_token);
        changes.remove(pos, pos + 1);
    }

    void removeReferenceOperator(ChangeSet &changes) const
    {
        ReferenceAST *refAST = m_declaratorAST->ptr_operator_list->value->asReference();
        QTC_ASSERT(refAST, return);
        const int pos = m_file->startOf(refAST->reference_token);
        changes.remove(pos, pos + 1);
    }

    void removeNewExpression(ChangeSet &changes, NewExpressionAST *newExprAST) const
    {
        ExpressionListAST *exprlist = nullptr;
        if (newExprAST->new_initializer) {
            if (ExpressionListParenAST *ast = newExprAST->new_initializer->asExpressionListParen())
                exprlist = ast->expression_list;
            else if (BracedInitializerAST *ast = newExprAST->new_initializer->asBracedInitializer())
                exprlist = ast->expression_list;
        }

        if (exprlist) {
            // remove 'new' keyword and type before initializer
            changes.remove(m_file->startOf(newExprAST->new_token),
                           m_file->startOf(newExprAST->new_initializer));

            changes.remove(m_file->endOf(m_declaratorAST->equal_token - 1),
                           m_file->startOf(m_declaratorAST->equal_token + 1));
        } else {
            // remove the whole new expression
            changes.remove(m_file->endOf(m_identifierAST->firstToken()),
                           m_file->startOf(newExprAST->lastToken()));
        }
    }

    void removeNewKeyword(ChangeSet &changes, NewExpressionAST *newExprAST) const
    {
        // remove 'new' keyword before initializer
        changes.remove(m_file->startOf(newExprAST->new_token),
                       m_file->startOf(newExprAST->new_type_id));
    }

    void convertToStackVariable(ChangeSet &changes) const
    {
        // Handle the initializer.
        if (m_declaratorAST->initializer) {
            if (NewExpressionAST *newExpression = m_declaratorAST->initializer->asNewExpression()) {
                if (m_isAutoDeclaration) {
                    if (!newExpression->new_initializer)
                        changes.insert(m_file->endOf(newExpression), QStringLiteral("()"));
                    removeNewKeyword(changes, newExpression);
                } else {
                    removeNewExpression(changes, newExpression);
                }
            }
        }

        // Fix all occurrences of the identifier in this function.
        ASTPath astPath(m_document);
        const QList<SemanticInfo::Use> uses = semanticInfo().localUses.value(m_symbol);
        for (const SemanticInfo::Use &use : uses) {
            const QList<AST *> path = astPath(use.line, use.column);
            AST *idAST = path.last();
            bool declarationFound = false;
            bool starFound = false;
            int ampersandPos = 0;
            bool memberAccess = false;
            bool deleteCall = false;

            for (int i = path.count() - 2; i >= 0; --i) {
                if (path.at(i) == m_declaratorAST) {
                    declarationFound = true;
                    break;
                }
                if (MemberAccessAST *memberAccessAST = path.at(i)->asMemberAccess()) {
                    if (m_file->tokenAt(memberAccessAST->access_token).kind() != T_ARROW)
                        continue;
                    int pos = m_file->startOf(memberAccessAST->access_token);
                    changes.replace(pos, pos + 2, QLatin1String("."));
                    memberAccess = true;
                    break;
                } else if (DeleteExpressionAST *deleteAST = path.at(i)->asDeleteExpression()) {
                    const int pos = m_file->startOf(deleteAST->delete_token);
                    changes.insert(pos, QLatin1String("// "));
                    deleteCall = true;
                    break;
                } else if (UnaryExpressionAST *unaryExprAST = path.at(i)->asUnaryExpression()) {
                    const Token tk = m_file->tokenAt(unaryExprAST->unary_op_token);
                    if (tk.kind() == T_STAR) {
                        if (!starFound) {
                            int pos = m_file->startOf(unaryExprAST->unary_op_token);
                            changes.remove(pos, pos + 1);
                        }
                        starFound = true;
                    } else if (tk.kind() == T_AMPER) {
                        ampersandPos = m_file->startOf(unaryExprAST->unary_op_token);
                    }
                } else if (PointerAST *ptrAST = path.at(i)->asPointer()) {
                    if (!starFound) {
                        const int pos = m_file->startOf(ptrAST->star_token);
                        changes.remove(pos, pos);
                    }
                    starFound = true;
                } else if (path.at(i)->asFunctionDefinition()) {
                    break;
                }
            }
            if (!declarationFound && !starFound && !memberAccess && !deleteCall) {
                if (ampersandPos) {
                    changes.insert(ampersandPos, QLatin1String("&("));
                    changes.insert(m_file->endOf(idAST->firstToken()), QLatin1String(")"));
                } else {
                    changes.insert(m_file->startOf(idAST), QLatin1String("&"));
                }
            }
        }
    }

    QString typeNameOfDeclaration() const
    {
        if (!m_simpleDeclaration
                || !m_simpleDeclaration->decl_specifier_list
                || !m_simpleDeclaration->decl_specifier_list->value) {
            return QString();
        }
        NamedTypeSpecifierAST *namedType
                = m_simpleDeclaration->decl_specifier_list->value->asNamedTypeSpecifier();
        if (!namedType)
            return QString();

        Overview overview;
        return overview.prettyName(namedType->name->name);
    }

    void insertNewExpression(ChangeSet &changes, ExpressionAST *ast) const
    {
        const QString typeName = typeNameOfDeclaration();
        if (CallAST *callAST = ast->asCall()) {
            if (typeName.isEmpty()) {
                changes.insert(m_file->startOf(callAST), QLatin1String("new "));
            } else {
                changes.insert(m_file->startOf(callAST),
                               QLatin1String("new ") + typeName + QLatin1Char('('));
                changes.insert(m_file->startOf(callAST->lastToken()), QLatin1String(")"));
            }
        } else {
            if (typeName.isEmpty())
                return;
            changes.insert(m_file->startOf(ast), QLatin1String(" = new ") + typeName);
        }
    }

    void insertNewExpression(ChangeSet &changes) const
    {
        const QString typeName = typeNameOfDeclaration();
        if (typeName.isEmpty())
            return;
        changes.insert(m_file->endOf(m_identifierAST->firstToken()),
                       QLatin1String(" = new ") + typeName);
    }

    void convertToPointer(ChangeSet &changes) const
    {
        // Handle initializer.
        if (m_declaratorAST->initializer) {
            if (IdExpressionAST *idExprAST = m_declaratorAST->initializer->asIdExpression()) {
                changes.insert(m_file->startOf(idExprAST), QLatin1String("&"));
            } else if (CallAST *callAST = m_declaratorAST->initializer->asCall()) {
                insertNewExpression(changes, callAST);
            } else if (ExpressionListParenAST *exprListAST = m_declaratorAST->initializer
                                                                 ->asExpressionListParen()) {
                insertNewExpression(changes, exprListAST);
            } else if (BracedInitializerAST *bracedInitializerAST = m_declaratorAST->initializer
                                                                        ->asBracedInitializer()) {
                insertNewExpression(changes, bracedInitializerAST);
            }
        } else {
            insertNewExpression(changes);
        }

        // Fix all occurrences of the identifier in this function.
        ASTPath astPath(m_document);
        const QList<SemanticInfo::Use> uses = semanticInfo().localUses.value(m_symbol);
        for (const SemanticInfo::Use &use : uses) {
            const QList<AST *> path = astPath(use.line, use.column);
            AST *idAST = path.last();
            bool insertStar = true;
            for (int i = path.count() - 2; i >= 0; --i) {
                if (m_isAutoDeclaration && path.at(i) == m_declaratorAST) {
                    insertStar = false;
                    break;
                }
                if (MemberAccessAST *memberAccessAST = path.at(i)->asMemberAccess()) {
                    const int pos = m_file->startOf(memberAccessAST->access_token);
                    changes.replace(pos, pos + 1, QLatin1String("->"));
                    insertStar = false;
                    break;
                } else if (UnaryExpressionAST *unaryExprAST = path.at(i)->asUnaryExpression()) {
                    if (m_file->tokenAt(unaryExprAST->unary_op_token).kind() == T_AMPER) {
                        const int pos = m_file->startOf(unaryExprAST->unary_op_token);
                        changes.remove(pos, pos + 1);
                        insertStar = false;
                        break;
                    }
                } else if (path.at(i)->asFunctionDefinition()) {
                    break;
                }
            }
            if (insertStar)
                changes.insert(m_file->startOf(idAST), QLatin1String("*"));
        }
    }

    const Mode m_mode;
    const bool m_isAutoDeclaration;
    const SimpleDeclarationAST * const m_simpleDeclaration;
    const DeclaratorAST * const m_declaratorAST;
    const SimpleNameAST * const m_identifierAST;
    Symbol * const m_symbol;
    const CppRefactoringChanges m_refactoring;
    const CppRefactoringFilePtr m_file;
    const Document::Ptr m_document;
};

} // anonymous namespace

void ConvertFromAndToPointer::doMatch(const CppQuickFixInterface &interface,
                                      QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();
    if (path.count() < 2)
        return;
    SimpleNameAST *identifier = path.last()->asSimpleName();
    if (!identifier)
        return;
    SimpleDeclarationAST *simpleDeclaration = nullptr;
    DeclaratorAST *declarator = nullptr;
    bool isFunctionLocal = false;
    bool isClassLocal = false;
    ConvertFromAndToPointerOp::Mode mode = ConvertFromAndToPointerOp::FromVariable;
    for (int i = path.count() - 2; i >= 0; --i) {
        AST *ast = path.at(i);
        if (!declarator && (declarator = ast->asDeclarator()))
            continue;
        if (!simpleDeclaration && (simpleDeclaration = ast->asSimpleDeclaration()))
            continue;
        if (declarator && simpleDeclaration) {
            if (ast->asClassSpecifier()) {
                isClassLocal = true;
            } else if (ast->asFunctionDefinition() && !isClassLocal) {
                isFunctionLocal = true;
                break;
            }
        }
    }
    if (!isFunctionLocal || !simpleDeclaration || !declarator)
        return;

    Symbol *symbol = nullptr;
    for (List<Symbol *> *lst = simpleDeclaration->symbols; lst; lst = lst->next) {
        if (lst->value->name() == identifier->name) {
            symbol = lst->value;
            break;
        }
    }
    if (!symbol)
        return;

    bool isAutoDeclaration = false;
    if (symbol->storage() == Symbol::Auto) {
        // For auto variables we must deduce the type from the initializer.
        if (!declarator->initializer)
            return;

        isAutoDeclaration = true;
        TypeOfExpression typeOfExpression;
        typeOfExpression.init(interface.semanticInfo().doc, interface.snapshot());
        typeOfExpression.setExpandTemplates(true);
        CppRefactoringFilePtr file = interface.currentFile();
        Scope *scope = file->scopeAt(declarator->firstToken());
        QList<LookupItem> result = typeOfExpression(file->textOf(declarator->initializer).toUtf8(),
                                                    scope, TypeOfExpression::Preprocess);
        if (!result.isEmpty() && result.first().type()->asPointerType())
            mode = ConvertFromAndToPointerOp::FromPointer;
    } else if (declarator->ptr_operator_list) {
        for (PtrOperatorListAST *ops = declarator->ptr_operator_list; ops; ops = ops->next) {
            if (ops != declarator->ptr_operator_list) {
                // Bail out on more complex pointer types (e.g. pointer of pointer,
                // or reference of pointer).
                return;
            }
            if (ops->value->asPointer())
                mode = ConvertFromAndToPointerOp::FromPointer;
            else if (ops->value->asReference())
                mode = ConvertFromAndToPointerOp::FromReference;
        }
    }

    const int priority = path.size() - 1;
    result << new ConvertFromAndToPointerOp(interface, priority, mode, isAutoDeclaration,
                                            simpleDeclaration, declarator, identifier, symbol);
}

namespace {

class ApplyDeclDefLinkOperation : public CppQuickFixOperation
{
public:
    explicit ApplyDeclDefLinkOperation(const CppQuickFixInterface &interface,
            const std::shared_ptr<FunctionDeclDefLink> &link)
        : CppQuickFixOperation(interface, 100)
        , m_link(link)
    {}

    void perform() override
    {
        if (editor()->declDefLink() == m_link)
            editor()->applyDeclDefLinkChanges(/*don't jump*/false);
    }

protected:
    virtual void performChanges(const CppRefactoringFilePtr &, const CppRefactoringChanges &)
    { /* never called since perform is overridden */ }

private:
    std::shared_ptr<FunctionDeclDefLink> m_link;
};

} // anonymous namespace

void ApplyDeclDefLinkChanges::doMatch(const CppQuickFixInterface &interface,
                                      QuickFixOperations &result)
{
    std::shared_ptr<FunctionDeclDefLink> link = interface.editor()->declDefLink();
    if (!link || !link->isMarkerVisible())
        return;

    auto op = new ApplyDeclDefLinkOperation(interface, link);
    op->setDescription(Tr::tr("Apply Function Signature Changes"));
    result << op;
}

namespace {

class AssignToLocalVariableOperation : public CppQuickFixOperation
{
public:
    explicit AssignToLocalVariableOperation(const CppQuickFixInterface &interface,
                                            const int insertPos, const AST *ast, const Name *name)
        : CppQuickFixOperation(interface)
        , m_insertPos(insertPos)
        , m_ast(ast)
        , m_name(name)
        , m_oo(CppCodeStyleSettings::currentProjectCodeStyleOverview())
        , m_originalName(m_oo.prettyName(m_name))
        , m_file(CppRefactoringChanges(snapshot()).cppFile(filePath()))
    {
        setDescription(Tr::tr("Assign to Local Variable"));
    }

private:
    void perform() override
    {
        QString type = deduceType();
        if (type.isEmpty())
            return;
        const int origNameLength = m_originalName.length();
        const QString varName = constructVarName();
        const QString insertString = type.replace(type.length() - origNameLength, origNameLength,
                                                  varName + QLatin1String(" = "));
        ChangeSet changes;
        changes.insert(m_insertPos, insertString);
        m_file->setChangeSet(changes);
        m_file->apply();

        // move cursor to new variable name
        QTextCursor c = m_file->cursor();
        c.setPosition(m_insertPos + insertString.length() - varName.length() - 3);
        c.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
        editor()->setTextCursor(c);
    }

    QString deduceType() const
    {
        const auto settings = CppQuickFixProjectsSettings::getQuickFixSettings(
                    ProjectExplorer::ProjectTree::currentProject());
        if (m_file->cppDocument()->languageFeatures().cxx11Enabled && settings->useAuto)
            return "auto " + m_originalName;

        TypeOfExpression typeOfExpression;
        typeOfExpression.init(semanticInfo().doc, snapshot(), context().bindings());
        typeOfExpression.setExpandTemplates(true);
        Scope * const scope = m_file->scopeAt(m_ast->firstToken());
        const QList<LookupItem> result = typeOfExpression(m_file->textOf(m_ast).toUtf8(),
                                                          scope, TypeOfExpression::Preprocess);
        if (result.isEmpty())
            return {};

        SubstitutionEnvironment env;
        env.setContext(context());
        env.switchScope(result.first().scope());
        ClassOrNamespace *con = typeOfExpression.context().lookupType(scope);
        if (!con)
            con = typeOfExpression.context().globalNamespace();
        UseMinimalNames q(con);
        env.enter(&q);

        Control *control = context().bindings()->control().get();
        FullySpecifiedType type = rewriteType(result.first().type(), &env, control);

        return m_oo.prettyType(type, m_name);
    }

    QString constructVarName() const
    {
        QString newName = m_originalName;
        if (newName.startsWith(QLatin1String("get"), Qt::CaseInsensitive)
                && newName.length() > 3
                && newName.at(3).isUpper()) {
            newName.remove(0, 3);
            newName.replace(0, 1, newName.at(0).toLower());
        } else if (newName.startsWith(QLatin1String("to"), Qt::CaseInsensitive)
                   && newName.length() > 2
                   && newName.at(2).isUpper()) {
            newName.remove(0, 2);
            newName.replace(0, 1, newName.at(0).toLower());
        } else {
            newName.replace(0, 1, newName.at(0).toUpper());
            newName.prepend(QLatin1String("local"));
        }
        return newName;
    }

    const int m_insertPos;
    const AST * const m_ast;
    const Name * const m_name;
    const Overview m_oo;
    const QString m_originalName;
    const CppRefactoringFilePtr m_file;
};

} // anonymous namespace

void AssignToLocalVariable::doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();
    AST *outerAST = nullptr;
    SimpleNameAST *nameAST = nullptr;

    for (int i = path.size() - 3; i >= 0; --i) {
        if (CallAST *callAST = path.at(i)->asCall()) {
            if (!interface.isCursorOn(callAST))
                return;
            if (i - 2 >= 0) {
                const int idx = i - 2;
                if (path.at(idx)->asSimpleDeclaration())
                    return;
                if (path.at(idx)->asExpressionStatement())
                    return;
                if (path.at(idx)->asMemInitializer())
                    return;
                if (path.at(idx)->asCall()) { // Fallback if we have a->b()->c()...
                    --i;
                    continue;
                }
            }
            for (int a = i - 1; a > 0; --a) {
                if (path.at(a)->asBinaryExpression())
                    return;
                if (path.at(a)->asReturnStatement())
                    return;
                if (path.at(a)->asCall())
                    return;
            }

            if (MemberAccessAST *member = path.at(i + 1)->asMemberAccess()) { // member
                if (NameAST *name = member->member_name)
                    nameAST = name->asSimpleName();
            } else if (QualifiedNameAST *qname = path.at(i + 2)->asQualifiedName()) { // static or
                nameAST = qname->unqualified_name->asSimpleName();                    // func in ns
            } else { // normal
                nameAST = path.at(i + 2)->asSimpleName();
            }

            if (nameAST) {
                outerAST = callAST;
                break;
            }
        } else if (NewExpressionAST *newexp = path.at(i)->asNewExpression()) {
            if (!interface.isCursorOn(newexp))
                return;
            if (i - 2 >= 0) {
                const int idx = i - 2;
                if (path.at(idx)->asSimpleDeclaration())
                    return;
                if (path.at(idx)->asExpressionStatement())
                    return;
                if (path.at(idx)->asMemInitializer())
                    return;
            }
            for (int a = i - 1; a > 0; --a) {
                if (path.at(a)->asReturnStatement())
                    return;
                if (path.at(a)->asCall())
                    return;
            }

            if (NamedTypeSpecifierAST *ts = path.at(i + 2)->asNamedTypeSpecifier()) {
                nameAST = ts->name->asSimpleName();
                outerAST = newexp;
                break;
            }
        }
    }

    if (outerAST && nameAST) {
        const CppRefactoringFilePtr file = interface.currentFile();
        QList<LookupItem> items;
        TypeOfExpression typeOfExpression;
        typeOfExpression.init(interface.semanticInfo().doc, interface.snapshot(),
                              interface.context().bindings());
        typeOfExpression.setExpandTemplates(true);

        // If items are empty, AssignToLocalVariableOperation will fail.
        items = typeOfExpression(file->textOf(outerAST).toUtf8(),
                                 file->scopeAt(outerAST->firstToken()),
                                 TypeOfExpression::Preprocess);
        if (items.isEmpty())
            return;

        if (CallAST *callAST = outerAST->asCall()) {
            items = typeOfExpression(file->textOf(callAST->base_expression).toUtf8(),
                                     file->scopeAt(callAST->base_expression->firstToken()),
                                     TypeOfExpression::Preprocess);
        } else {
            items = typeOfExpression(file->textOf(nameAST).toUtf8(),
                                     file->scopeAt(nameAST->firstToken()),
                                     TypeOfExpression::Preprocess);
        }

        for (const LookupItem &item : std::as_const(items)) {
            if (!item.declaration())
                continue;

            if (Function *func = item.declaration()->asFunction()) {
                if (func->isSignal() || func->returnType()->asVoidType())
                    return;
            } else if (Declaration *dec = item.declaration()->asDeclaration()) {
                if (Function *func = dec->type()->asFunctionType()) {
                    if (func->isSignal() || func->returnType()->asVoidType())
                      return;
                }
            }

            const Name *name = nameAST->name;
            const int insertPos = interface.currentFile()->startOf(outerAST);
            result << new AssignToLocalVariableOperation(interface, insertPos, outerAST, name);
            return;
        }
    }
}

namespace {

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

        const Utils::FilePath filePath = currentFile()->filePath();
        const CppRefactoringChanges refactoring(snapshot());
        const CppRefactoringFilePtr file = refactoring.cppFile(filePath);
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

} // anonymous namespace

void OptimizeForLoop::doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result)
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

namespace {

class EscapeStringLiteralOperation: public CppQuickFixOperation
{
public:
    EscapeStringLiteralOperation(const CppQuickFixInterface &interface,
                                 ExpressionAST *literal, bool escape)
        : CppQuickFixOperation(interface)
        , m_literal(literal)
        , m_escape(escape)
    {
        if (m_escape) {
            setDescription(Tr::tr("Escape String Literal as UTF-8"));
        } else {
            setDescription(Tr::tr("Unescape String Literal as UTF-8"));
        }
    }

private:
    static inline bool isDigit(quint8 ch, int base)
    {
        if (base == 8)
            return ch >= '0' && ch < '8';
        if (base == 16)
            return isxdigit(ch);
        return false;
    }

    static QByteArrayList escapeString(const QByteArray &contents)
    {
        QByteArrayList newContents;
        QByteArray chunk;
        bool wasEscaped = false;
        for (const quint8 c : contents) {
            const bool needsEscape = !isascii(c) || !isprint(c);
            if (!needsEscape && wasEscaped && std::isxdigit(c) && !chunk.isEmpty()) {
                newContents << chunk;
                chunk.clear();
            }
            if (needsEscape)
                chunk += QByteArray("\\x") + QByteArray::number(c, 16).rightJustified(2, '0');
            else
                chunk += c;
            wasEscaped = needsEscape;
        }
        if (!chunk.isEmpty())
            newContents << chunk;
        return newContents;
    }

    static QByteArray unescapeString(const QByteArray &contents)
    {
        QByteArray newContents;
        const int len = contents.length();
        for (int i = 0; i < len; ++i) {
            quint8 c = contents.at(i);
            if (c == '\\' && i < len - 1) {
                int idx = i + 1;
                quint8 ch = contents.at(idx);
                int base = 0;
                int maxlen = 0;
                if (isDigit(ch, 8)) {
                    base = 8;
                    maxlen = 3;
                } else if ((ch == 'x' || ch == 'X') && idx < len - 1) {
                    base = 16;
                    maxlen = 2;
                    ch = contents.at(++idx);
                }
                if (base > 0) {
                    QByteArray buf;
                    while (isDigit(ch, base) && idx < len && buf.length() < maxlen) {
                        buf += ch;
                        ++idx;
                        if (idx == len)
                            break;
                        ch = contents.at(idx);
                    }
                    if (!buf.isEmpty()) {
                        bool ok;
                        uint value = buf.toUInt(&ok, base);
                        // Don't unescape isascii() && !isprint()
                        if (ok && (!isascii(value) || isprint(value))) {
                            newContents += value;
                            i = idx - 1;
                            continue;
                        }
                    }
                }
                newContents += c;
                c = contents.at(++i);
            }
            newContents += c;
        }
        return newContents;
    }

    // QuickFixOperation interface
public:
    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.cppFile(filePath());

        const int startPos = currentFile->startOf(m_literal);
        const int endPos = currentFile->endOf(m_literal);

        StringLiteralAST *stringLiteral = m_literal->asStringLiteral();
        QTC_ASSERT(stringLiteral, return);
        const QByteArray oldContents(currentFile->tokenAt(stringLiteral->literal_token).
                                     identifier->chars());
        QByteArrayList newContents;
        if (m_escape)
            newContents = escapeString(oldContents);
        else
            newContents = {unescapeString(oldContents)};

        if (newContents.isEmpty()
                || (newContents.size() == 1 && newContents.first() == oldContents)) {
            return;
        }

        QTextCodec *utf8codec = QTextCodec::codecForName("UTF-8");
        QScopedPointer<QTextDecoder> decoder(utf8codec->makeDecoder());
        ChangeSet changes;

        bool replace = true;
        for (const QByteArray &chunk : std::as_const(newContents)) {
            const QString str = decoder->toUnicode(chunk);
            const QByteArray utf8buf = str.toUtf8();
            if (!utf8codec->canEncode(str) || chunk != utf8buf)
                return;
            if (replace)
                changes.replace(startPos + 1, endPos - 1, str);
            else
                changes.insert(endPos, "\"" + str + "\"");
            replace = false;
        }
        currentFile->setChangeSet(changes);
        currentFile->apply();
    }

private:
    ExpressionAST *m_literal;
    bool m_escape;
};

} // anonymous namespace

void EscapeStringLiteral::doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();
    if (path.isEmpty())
        return;

    AST * const lastAst = path.last();
    ExpressionAST *literal = lastAst->asStringLiteral();
    if (!literal)
        return;

    StringLiteralAST *stringLiteral = literal->asStringLiteral();
    CppRefactoringFilePtr file = interface.currentFile();
    const QByteArray contents(file->tokenAt(stringLiteral->literal_token).identifier->chars());

    bool canEscape = false;
    bool canUnescape = false;
    for (int i = 0; i < contents.length(); ++i) {
        quint8 c = contents.at(i);
        if (!isascii(c) || !isprint(c)) {
            canEscape = true;
        } else if (c == '\\' && i < contents.length() - 1) {
            c = contents.at(++i);
            if ((c >= '0' && c < '8') || c == 'x' || c == 'X')
                canUnescape = true;
        }
    }

    if (canEscape)
        result << new EscapeStringLiteralOperation(interface, literal, true);

    if (canUnescape)
        result << new EscapeStringLiteralOperation(interface, literal, false);
}

void ExtraRefactoringOperations::doMatch(const CppQuickFixInterface &interface,
                                         QuickFixOperations &result)
{
    const auto processor = CppModelManager::cppEditorDocumentProcessor(interface.filePath());
    if (processor) {
        const auto clangFixItOperations = processor->extraRefactoringOperations(interface);
        result.append(clangFixItOperations);
    }
}

namespace {
class ConvertCommentStyleOp : public CppQuickFixOperation
{
public:
    ConvertCommentStyleOp(const CppQuickFixInterface &interface, const QList<Token> &tokens,
                          Kind kind)
        : CppQuickFixOperation(interface),
          m_tokens(tokens),
          m_kind(kind),
          m_wasCxxStyle(m_kind == T_CPP_COMMENT || m_kind == T_CPP_DOXY_COMMENT),
          m_isDoxygen(m_kind == T_DOXY_COMMENT || m_kind == T_CPP_DOXY_COMMENT)
    {
        setDescription(m_wasCxxStyle ? Tr::tr("Convert Comment to C-Style")
                                     : Tr::tr("Convert Comment to C++-Style"));
    }

private:
    // Turns every line of a C-style comment into a C++-style comment and vice versa.
    // For C++ -> C, we use one /* */ comment block per line. However, doxygen
    // requires a single comment, so there we just replace the prefix with whitespace and
    // add the start and end comment in extra lines.
    // For cosmetic reasons, we offer some convenience functionality:
    //   - Turn /***** ... into ////// ... and vice versa
    //   - With C -> C++, remove leading asterisks.
    //   - With C -> C++, remove the first and last line of a block if they have no content
    //     other than the comment start and end characters.
    //   - With C++ -> C, try to align the end comment characters.
    // These are obviously heuristics; we do not guarantee perfect results for everybody.
    // We also don't second-guess the users's selection: E.g. if there is an empty
    // line between the tokens, then it's not the same doxygen comment, but we merge
    // it anyway in C++ to C mode.
    void perform() override
    {
        TranslationUnit * const tu = currentFile()->cppDocument()->translationUnit();
        const QString newCommentStart = getNewCommentStart();
        ChangeSet changeSet;
        int endCommentColumn = -1;
        const QChar oldFillChar = m_wasCxxStyle ? '/' : '*';
        const QChar newFillChar = m_wasCxxStyle ? '*' : '/';

        for (const Token &token : m_tokens) {
            const int startPos = tu->getTokenPositionInDocument(token, textDocument());
            const int endPos = tu->getTokenEndPositionInDocument(token, textDocument());

            if (m_wasCxxStyle && m_isDoxygen) {
                // Replace "///" characters with whitespace (to keep alignment).
                // The insertion of "/*" and "*/" is done once after the loop.
                changeSet.replace(startPos, startPos + 3, "   ");
                continue;
            }

            const QTextBlock firstBlock = textDocument()->findBlock(startPos);
            const QTextBlock lastBlock = textDocument()->findBlock(endPos);
            for (QTextBlock block = firstBlock; block.isValid() && block.position() <= endPos;
                 block = block.next()) {
                const QString &blockText = block.text();
                const int firstColumn = block == firstBlock ? startPos - block.position() : 0;
                const int endColumn = block == lastBlock ? endPos - block.position()
                                                         : block.length();

                // Returns true if the current line looks like "/********/" or "//////////",
                // as is often the case at the start and end of comment blocks.
                const auto fillChecker = [&] {
                    if (m_isDoxygen)
                        return false;
                    QString textToCheck = blockText;
                    if (block == firstBlock)
                        textToCheck.remove(0, 1);
                    if (block == lastBlock)
                        textToCheck.chop(block.length() - endColumn);
                    return Utils::allOf(textToCheck, [oldFillChar](const QChar &c)
                                        { return c == oldFillChar || c == ' ';
                    }) && textToCheck.count(oldFillChar) > 2;
                };

                // Returns the index of the first character of actual comment content,
                // as opposed to visual stuff like slashes, stars or whitespace.
                const auto indexOfActualContent = [&] {
                    const int offset = block == firstBlock ? firstColumn + newCommentStart.length()
                                                           : firstColumn;

                    for (int i = offset, lastFillChar = -1; i < blockText.length(); ++i) {
                        if (blockText.at(i) == oldFillChar) {
                            lastFillChar = i;
                            continue;
                        }
                        if (!blockText.at(i).isSpace())
                            return lastFillChar + 1;
                    }
                    return -1;
                };

                if (fillChecker()) {
                    const QString replacement = QString(endColumn - 1 - firstColumn, newFillChar);
                    changeSet.replace(block.position() + firstColumn,
                                      block.position() + endColumn - 1,
                                      replacement);
                    if (m_wasCxxStyle) {
                        changeSet.replace(block.position() + firstColumn,
                                          block.position() + firstColumn + 1, "/");
                        changeSet.insert(block.position() + endColumn - 1, "*");
                        endCommentColumn = endColumn - 1;
                    }
                    continue;
                }

                // Remove leading noise or even the entire block, if applicable.
                const bool blockIsRemovable = (block == firstBlock || block == lastBlock)
                                              && firstBlock != lastBlock;
                const auto removeBlock = [&] {
                    changeSet.remove(block.position() + firstColumn, block.position() + endColumn);
                };
                const int contentIndex = indexOfActualContent();
                int removed = 0;
                if (contentIndex == -1) {
                    if (blockIsRemovable) {
                        removeBlock();
                        continue;
                    } else if (!m_wasCxxStyle) {
                        changeSet.replace(block.position() + firstColumn,
                                          block.position() + endColumn - 1, newCommentStart);
                        continue;
                    }
                } else if (block == lastBlock && contentIndex == endColumn - 1) {
                    if (blockIsRemovable) {
                        removeBlock();
                        break;
                    }
                } else {
                    changeSet.remove(block.position() + firstColumn,
                                     block.position() + firstColumn + contentIndex);
                    removed = contentIndex;
                }

                if (block == firstBlock) {
                    changeSet.replace(startPos, startPos + newCommentStart.length(),
                                      newCommentStart);
                } else {
                    // If the line starts with enough whitespace, replace it with the
                    // comment start characters, so we don't move the content to the right
                    // unnecessarily. Otherwise, insert the comment start characters.
                    if (blockText.startsWith(QString(newCommentStart.size() + removed + 1, ' '))) {
                        changeSet.replace(block.position(),
                                          block.position() + newCommentStart.length(),
                                          newCommentStart);
                    } else {
                        changeSet.insert(block.position(), newCommentStart);
                    }
                }

                if (block == lastBlock) {
                    if (m_wasCxxStyle) {
                        // This is for proper alignment of the end comment character.
                        if (endCommentColumn != -1) {
                            const int endCommentPos = block.position() + endCommentColumn;
                            if (endPos < endCommentPos)
                                changeSet.insert(endPos, QString(endCommentPos - endPos - 1, ' '));
                        }
                        changeSet.insert(endPos, " */");
                    } else {
                        changeSet.remove(endPos - 2, endPos);
                    }
                }
            }
        }

        if (m_wasCxxStyle && m_isDoxygen) {
            const int startPos = tu->getTokenPositionInDocument(m_tokens.first(), textDocument());
            const int endPos = tu->getTokenEndPositionInDocument(m_tokens.last(), textDocument());
            changeSet.insert(startPos, "/*!\n");
            changeSet.insert(endPos, "\n*/");
        }

        changeSet.apply(textDocument());
    }

    QString getNewCommentStart() const
    {
        if (m_wasCxxStyle) {
            if (m_isDoxygen)
                return "/*!";
            return "/*";
        }
        if (m_isDoxygen)
            return "//!";
        return "//";
    }

    const QList<Token> m_tokens;
    const Kind m_kind;
    const bool m_wasCxxStyle;
    const bool m_isDoxygen;
};
} // namespace

void ConvertCommentStyle::doMatch(const CppQuickFixInterface &interface,
                                  TextEditor::QuickFixOperations &result)
{
    // If there's a selection, then it must entirely consist of comment tokens.
    // If there's no selection, the cursor must be on a comment.
    const QList<Token> &cursorTokens = interface.currentFile()->tokensForCursor();
    if (cursorTokens.empty())
        return;
    if (!cursorTokens.front().isComment())
        return;

    // All tokens must be the same kind of comment, but we make an exception for doxygen comments
    // that start with "///", as these are often not intended to be doxygen. For our purposes,
    // we treat them as normal comments.
    const auto effectiveKind = [&interface](const Token &token) {
        if (token.kind() != T_CPP_DOXY_COMMENT)
            return token.kind();
        TranslationUnit * const tu = interface.currentFile()->cppDocument()->translationUnit();
        const int startPos = tu->getTokenPositionInDocument(token, interface.textDocument());
        const QString commentStart = interface.textAt(startPos, 3);
        return commentStart == "///" ? T_CPP_COMMENT : T_CPP_DOXY_COMMENT;
    };
    const Kind kind = effectiveKind(cursorTokens.first());
    for (int i = 1; i < cursorTokens.count(); ++i) {
        if (effectiveKind(cursorTokens.at(i)) != kind)
            return;
    }

    // Ok, all tokens are of same(ish) comment type, offer quickfix.
    result << new ConvertCommentStyleOp(interface, cursorTokens, kind);
}

namespace {
class MoveFunctionCommentsOp : public CppQuickFixOperation
{
public:
    enum class Direction { ToDecl, ToDef };
    MoveFunctionCommentsOp(const CppQuickFixInterface &interface, const Symbol *symbol,
                           const QList<Token> &commentTokens, Direction direction)
        : CppQuickFixOperation(interface), m_symbol(symbol), m_commentTokens(commentTokens)
    {
        setDescription(direction == Direction::ToDecl
                           ? Tr::tr("Move Function Documentation to Declaration")
                           : Tr::tr("Move Function Documentation to Definition"));
    }

private:
    void perform() override
    {
        const auto textDoc = const_cast<QTextDocument *>(currentFile()->document());
        const int pos = currentFile()->cppDocument()->translationUnit()->getTokenPositionInDocument(
            m_symbol->sourceLocation(), textDoc);
        QTextCursor cursor(textDoc);
        cursor.setPosition(pos);
        const CursorInEditor cursorInEditor(cursor, currentFile()->filePath(), editor(),
                                            editor()->textDocument());
        const auto callback = [symbolLoc = m_symbol->toLink(), comments = m_commentTokens]
            (const Link &link) {
            moveComments(link, symbolLoc, comments);
        };
        CppModelManager::followSymbol(cursorInEditor, callback, true, false,
                                      FollowSymbolMode::Exact);
    }

    static void moveComments(const Link &targetLoc, const Link &symbolLoc,
                             const QList<Token> &comments)
    {
        if (!targetLoc.hasValidTarget() || targetLoc.hasSameLocation(symbolLoc))
            return;

        CppRefactoringChanges changes(CppModelManager::snapshot());
        const CppRefactoringFilePtr sourceFile = changes.cppFile(symbolLoc.targetFilePath);
        const CppRefactoringFilePtr targetFile
            = targetLoc.targetFilePath == symbolLoc.targetFilePath
                  ? sourceFile
                  : changes.cppFile(targetLoc.targetFilePath);
        const Document::Ptr &targetCppDoc = targetFile->cppDocument();
        const QList<AST *> targetAstPath = ASTPath(targetCppDoc)(
            targetLoc.targetLine, targetLoc.targetColumn + 1);
        if (targetAstPath.isEmpty())
            return;
        const AST *targetDeclAst = nullptr;
        for (auto it = std::next(std::rbegin(targetAstPath));
             it != std::rend(targetAstPath); ++it) {
            AST * const node = *it;
            if (node->asDeclaration()) {
                targetDeclAst = node;
                continue;
            }
            if (targetDeclAst)
                break;
        }
        if (!targetDeclAst)
            return;
        const int insertionPos = targetCppDoc->translationUnit()->getTokenPositionInDocument(
            targetDeclAst->firstToken(), targetFile->document());
        const TranslationUnit * const sourceTu = sourceFile->cppDocument()->translationUnit();
        const int sourceCommentStartPos = sourceTu->getTokenPositionInDocument(
            comments.first(), sourceFile->document());
        const int sourceCommentEndPos = sourceTu->getTokenEndPositionInDocument(
            comments.last(), sourceFile->document());

        // Manually adjust indentation, as both our built-in indenter and ClangFormat
        // are unreliable with regards to comment continuation lines.
        auto tabSettings = [](CppRefactoringFilePtr file) {
            if (auto editor = file->editor())
                return editor->textDocument()->tabSettings();
            return ProjectExplorer::actualTabSettings(file->filePath(), nullptr);
        };
        const TabSettings &sts = tabSettings(sourceFile);
        const TabSettings &tts = tabSettings(targetFile);
        const QTextBlock insertionBlock = targetFile->document()->findBlock(insertionPos);
        const int insertionColumn = tts.columnAt(insertionBlock.text(),
                                                 insertionPos - insertionBlock.position());
        const QTextBlock removalBlock = sourceFile->document()->findBlock(sourceCommentStartPos);
        const QTextBlock removalBlockEnd = sourceFile->document()->findBlock(sourceCommentEndPos);
        const int removalColumn = sts.columnAt(removalBlock.text(),
                                               sourceCommentStartPos - removalBlock.position());
        const int columnOffset = insertionColumn - removalColumn;
        QString functionDoc;
        if (columnOffset != 0) {
            for (QTextBlock block = removalBlock;
                 block.isValid() && block != removalBlockEnd.next();
                 block = block.next()) {
                QString text = block.text() + QChar::ParagraphSeparator;
                if (block == removalBlockEnd)
                    text = text.left(sourceCommentEndPos - block.position());
                if (block == removalBlock) {
                    text = text.mid(sourceCommentStartPos - block.position());
                } else {
                    int lineIndentColumn = sts.indentationColumn(text) + columnOffset;
                    text.replace(0,
                                 TabSettings::firstNonSpace(text),
                                 tts.indentationString(0, lineIndentColumn, 0, insertionBlock));
                }
                functionDoc += text;
            }
        } else {
            functionDoc = sourceFile->textOf(sourceCommentStartPos, sourceCommentEndPos);
        }

        // Remove comment plus leading and trailing whitespace, including trailing newline.
        const auto removeAtSource = [&](ChangeSet &changeSet) {
            int removalPos = sourceCommentStartPos;
            const QChar newline(QChar::ParagraphSeparator);
            while (true) {
                const int prev = removalPos - 1;
                if (prev < 0)
                    break;
                const QChar prevChar = sourceFile->charAt(prev);
                if (!prevChar.isSpace() || prevChar == newline)
                    break;
                removalPos = prev;
            }
            int removalEndPos = sourceCommentEndPos;
            while (true) {
                if (removalEndPos == sourceFile->document()->characterCount())
                    break;
                const QChar nextChar = sourceFile->charAt(removalEndPos);
                if (!nextChar.isSpace())
                    break;
                ++removalEndPos;
                if (nextChar == newline)
                    break;
            }
            changeSet.remove(removalPos, removalEndPos);
        };

        ChangeSet targetChangeSet;
        targetChangeSet.insert(insertionPos, functionDoc);
        targetChangeSet.insert(insertionPos, "\n");
        targetChangeSet.insert(insertionPos, QString(insertionColumn, ' '));
        if (targetFile == sourceFile)
            removeAtSource(targetChangeSet);
        targetFile->setChangeSet(targetChangeSet);
        const bool targetFileSuccess = targetFile->apply();
        if (targetFile == sourceFile || !targetFileSuccess)
            return;
        ChangeSet sourceChangeSet;
        removeAtSource(sourceChangeSet);
        sourceFile->setChangeSet(sourceChangeSet);
        sourceFile->apply();
    }

    const Symbol * const m_symbol;
    const QList<Token> m_commentTokens;
};
} // namespace

void MoveFunctionComments::doMatch(const CppQuickFixInterface &interface,
                                   TextEditor::QuickFixOperations &result)
{
    const QList<AST *> &astPath = interface.path();
    if (astPath.isEmpty())
        return;
    const Symbol *symbol = nullptr;
    MoveFunctionCommentsOp::Direction direction = MoveFunctionCommentsOp::Direction::ToDecl;
    for (auto it = std::next(std::rbegin(astPath)); it != std::rend(astPath); ++it) {
        if (const auto func = (*it)->asFunctionDefinition()) {
            symbol = func->symbol;
            direction = MoveFunctionCommentsOp::Direction::ToDecl;
            break;
        }
        const auto decl = (*it)->asSimpleDeclaration();
        if (!decl || !decl->declarator_list)
            continue;
        for (auto it = decl->declarator_list->begin();
             !symbol && it != decl->declarator_list->end(); ++it) {
            PostfixDeclaratorListAST * const funcDecls = (*it)->postfix_declarator_list;
            if (!funcDecls)
                continue;
            for (auto it = funcDecls->begin(); it != funcDecls->end(); ++it) {
                if (const auto func = (*it)->asFunctionDeclarator()) {
                    symbol = func->symbol;
                    direction = MoveFunctionCommentsOp::Direction::ToDef;
                    break;
                }
            }
        }

    }
    if (!symbol)
        return;

    if (const QList<Token> commentTokens = commentsForDeclaration(
            symbol, *interface.textDocument(), interface.currentFile()->cppDocument());
        !commentTokens.isEmpty()) {
        result << new MoveFunctionCommentsOp(interface, symbol, commentTokens, direction);
    }
}

namespace {
class ConvertToMetaMethodCallOp : public CppQuickFixOperation
{
public:
    ConvertToMetaMethodCallOp(const CppQuickFixInterface &interface, CallAST *callAst)
        : CppQuickFixOperation(interface), m_callAst(callAst)
    {
        setDescription(Tr::tr("Convert Function Call to Qt Meta-Method Invocation"));
    }

private:
    void perform() override
    {
        // Construct the argument list.
        Overview ov;
        QStringList arguments;
        for (ExpressionListAST *it = m_callAst->expression_list; it; it = it->next) {
            if (!it->value)
                return;
            const FullySpecifiedType argType
                = typeOfExpr(it->value, currentFile(), snapshot(), context());
            if (!argType.isValid())
                return;
            arguments << QString::fromUtf8("Q_ARG(%1, %2)")
                             .arg(ov.prettyType(argType), currentFile()->textOf(it->value));
        }
        QString argsString = arguments.join(", ");
        if (!argsString.isEmpty())
            argsString.prepend(", ");

        // Construct the replace string.
        const auto memberAccessAst = m_callAst->base_expression->asMemberAccess();
        QTC_ASSERT(memberAccessAst, return);
        QString baseExpr = currentFile()->textOf(memberAccessAst->base_expression);
        const FullySpecifiedType baseExprType
            = typeOfExpr(memberAccessAst->base_expression, currentFile(), snapshot(), context());
        if (!baseExprType.isValid())
            return;
        if (!baseExprType->asPointerType())
            baseExpr.prepend('&');
        const QString functionName = currentFile()->textOf(memberAccessAst->member_name);
        const QString qMetaObject = "QMetaObject";
        const QString newCall = QString::fromUtf8("%1::invokeMethod(%2, \"%3\"%4)")
                                    .arg(qMetaObject, baseExpr, functionName, argsString);

        // Determine the start and end positions of the replace operation.
        // If the call is preceded by an "emit" keyword, that one has to be removed as well.
        int firstToken = m_callAst->firstToken();
        if (firstToken > 0)
            switch (semanticInfo().doc->translationUnit()->tokenKind(firstToken - 1)) {
            case T_EMIT: case T_Q_EMIT: --firstToken; break;
            default: break;
        }
        const TranslationUnit *const tu = semanticInfo().doc->translationUnit();
        const int startPos = tu->getTokenPositionInDocument(firstToken, textDocument());
        const int endPos = tu->getTokenPositionInDocument(m_callAst->lastToken(), textDocument());

        // Replace the old call with the new one.
        ChangeSet changes;
        changes.replace(startPos, endPos, newCall);

        // Insert include for QMetaObject, if necessary.
        const Identifier qMetaObjectId(qPrintable(qMetaObject), qMetaObject.size());
        Scope * const scope = currentFile()->scopeAt(firstToken);
        const QList<LookupItem> results = context().lookup(&qMetaObjectId, scope);
        bool isDeclared = false;
        for (const LookupItem &item : results) {
            if (Symbol *declaration = item.declaration(); declaration && declaration->asClass()) {
                isDeclared = true;
                break;
            }
        }
        if (!isDeclared) {
            insertNewIncludeDirective('<' + qMetaObject + '>', currentFile(), semanticInfo().doc,
                                      changes);
        }

        // Apply the changes.
        currentFile()->setChangeSet(changes);
        currentFile()->apply();
    }

    const CallAST * const m_callAst;
};
} // namespace

void ConvertToMetaMethodCall::doMatch(const CppQuickFixInterface &interface,
                                      TextEditor::QuickFixOperations &result)
{
    const Document::Ptr &cppDoc = interface.currentFile()->cppDocument();
    const QList<AST *> path = ASTPath(cppDoc)(interface.cursor());
    if (path.isEmpty())
        return;

    // Are we on a member function call?
    CallAST *callAst = nullptr;
    for (auto it = path.crbegin(); it != path.crend(); ++it) {
        if ((callAst = (*it)->asCall()))
            break;
    }
    if (!callAst || !callAst->base_expression)
        return;
    ExpressionAST *baseExpr = nullptr;
    const NameAST *nameAst = nullptr;
    if (const MemberAccessAST * const ast = callAst->base_expression->asMemberAccess()) {
        baseExpr = ast->base_expression;
        nameAst = ast->member_name;
    }
    if (!baseExpr || !nameAst || !nameAst->name)
        return;

    // Locate called function and check whether it is invokable.
    Scope *scope = cppDoc->globalNamespace();
    for (auto it = path.crbegin(); it != path.crend(); ++it) {
        if (const CompoundStatementAST * const stmtAst = (*it)->asCompoundStatement()) {
            scope = stmtAst->symbol;
            break;
        }
    }
    const LookupContext context(cppDoc, interface.snapshot());
    TypeOfExpression exprType;
    exprType.setExpandTemplates(true);
    exprType.init(cppDoc, interface.snapshot());
    const QList<LookupItem> typeMatches = exprType(callAst->base_expression, cppDoc, scope);
    for (const LookupItem &item : typeMatches) {
        if (const auto func = item.type()->asFunctionType(); func && func->methodKey()) {
            result << new ConvertToMetaMethodCallOp(interface, callAst);
            return;
        }
    }
}

void createCppQuickFixes()
{
    new AddIncludeForUndefinedIdentifier;

    new FlipLogicalOperands;
    new InverseLogicalComparison;
    new RewriteLogicalAnd;

    new ConvertToCamelCase;

    new ConvertCStringToNSString;
    new ConvertNumericLiteral;
    new TranslateStringLiteral;
    new WrapStringLiteral;

    new MoveDeclarationOutOfIf;
    new MoveDeclarationOutOfWhile;

    new SplitIfStatement;
    new SplitSimpleDeclaration;

    new AddBracesToControlStatement;
    new RearrangeParamDeclarationList;
    new ReformatPointerDeclaration;

    new CompleteSwitchCaseStatement;

    new ApplyDeclDefLinkChanges;
    new ConvertFromAndToPointer;
    new ExtractFunction;
    new ExtractLiteralAsParameter;
    new InsertDeclFromDef;
    new InsertDefFromDecl;
    new AddDeclarationForUndeclaredIdentifier;
    new InsertDefsFromDecls;

    new AssignToLocalVariable;

    registerInsertVirtualMethodsQuickfix();
    registerMoveClassToOwnFileQuickfix();
    registerRemoveUsingNamespaceQuickfix();
    registerCodeGenerationQuickfixes();
    registerConvertQt4ConnectQuickfix();
    registerMoveFunctionDefinitionQuickfixes();

    new OptimizeForLoop;

    new EscapeStringLiteral;

    new ExtraRefactoringOperations;

    new ConvertCommentStyle;
    new MoveFunctionComments;
    new ConvertToMetaMethodCall;
}

void destroyCppQuickFixes()
{
    for (int i = g_cppQuickFixFactories.size(); --i >= 0; )
        delete g_cppQuickFixFactories.at(i);
}

} // namespace Internal
} // namespace CppEditor
