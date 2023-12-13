// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppquickfixes.h"

#include "baseeditordocumentprocessor.h"
#include "cppcodestylesettings.h"
#include "cppeditordocument.h"
#include "cppeditortr.h"
#include "cppeditorwidget.h"
#include "cppfunctiondecldeflink.h"
#include "cppinsertvirtualmethods.h"
#include "cpplocatordata.h"
#include "cpppointerdeclarationformatter.h"
#include "cppquickfixassistant.h"
#include "cppquickfixprojectsettings.h"
#include "cpprefactoringchanges.h"
#include "cpptoolsreuse.h"
#include "includeutils.h"
#include "indexitem.h"
#include "insertionpointlocator.h"
#include "symbolfinder.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <cplusplus/ASTPath.h>
#include <cplusplus/CPlusPlusForwardDeclarations.h>
#include <cplusplus/CppRewriter.h>
#include <cplusplus/declarationcomments.h>
#include <cplusplus/NamePrettyPrinter.h>
#include <cplusplus/TypeOfExpression.h>
#include <cplusplus/TypePrettyPrinter.h>

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/editorconfiguration.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/projectmanager.h>

#include <texteditor/tabsettings.h>

#include <utils/algorithm.h>
#include <utils/basetreeview.h>
#include <utils/layoutbuilder.h>
#include <utils/fancylineedit.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>

#ifdef WITH_TESTS
#include <QAbstractItemModelTester>
#endif
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

const QList<CppQuickFixFactory *> &CppQuickFixFactory::cppQuickFixFactories()
{
    return g_cppQuickFixFactories;
}

namespace Internal {

QString inlinePrefix(const FilePath &targetFile, const std::function<bool()> &extraCondition = {})
{
    if (ProjectFile::isHeader(ProjectFile::classify(targetFile.path()))
            && (!extraCondition || extraCondition())) {
        return "inline ";
    }
    return {};
}

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
    return id == "QLatin1String" || id == "QLatin1Literal" || id == "QStringLiteral";
}

inline bool isQtStringTranslation(const QByteArray &id)
{
    return id == "tr" || id == "trUtf8" || id == "translate" || id == "QT_TRANSLATE_NOOP";
}

Class *isMemberFunction(const LookupContext &context, Function *function)
{
    QTC_ASSERT(function, return nullptr);

    Scope *enclosingScope = function->enclosingScope();
    while (!(enclosingScope->asNamespace() || enclosingScope->asClass()))
        enclosingScope = enclosingScope->enclosingScope();
    QTC_ASSERT(enclosingScope != nullptr, return nullptr);

    const Name *functionName = function->name();
    if (!functionName)
        return nullptr;

    if (!functionName->asQualifiedNameId())
        return nullptr; // trying to add a declaration for a global function

    const QualifiedNameId *q = functionName->asQualifiedNameId();
    if (!q->base())
        return nullptr;

    if (ClassOrNamespace *binding = context.lookupType(q->base(), enclosingScope)) {
        const QList<Symbol *> symbols = binding->symbols();
        for (Symbol *s : symbols) {
            if (Class *matchingClass = s->asClass())
                return matchingClass;
        }
    }

    return nullptr;
}

Namespace *isNamespaceFunction(const LookupContext &context, Function *function)
{
    QTC_ASSERT(function, return nullptr);
    if (isMemberFunction(context, function))
        return nullptr;

    Scope *enclosingScope = function->enclosingScope();
    while (!(enclosingScope->asNamespace() || enclosingScope->asClass()))
        enclosingScope = enclosingScope->enclosingScope();
    QTC_ASSERT(enclosingScope != nullptr, return nullptr);

    const Name *functionName = function->name();
    if (!functionName)
        return nullptr;

    // global namespace
    if (!functionName->asQualifiedNameId()) {
        const QList<Symbol *> symbols = context.globalNamespace()->symbols();
        for (Symbol *s : symbols) {
            if (Namespace *matchingNamespace = s->asNamespace())
                return matchingNamespace;
        }
        return nullptr;
    }

    const QualifiedNameId *q = functionName->asQualifiedNameId();
    if (!q->base())
        return nullptr;

    if (ClassOrNamespace *binding = context.lookupType(q->base(), enclosingScope)) {
        const QList<Symbol *> symbols = binding->symbols();
        for (Symbol *s : symbols) {
            if (Namespace *matchingNamespace = s->asNamespace())
                return matchingNamespace;
        }
    }

    return nullptr;
}

// Given include is e.g. "afile.h" or <afile.h> (quotes/angle brackets included!).
void insertNewIncludeDirective(const QString &include, CppRefactoringFilePtr file,
                               const Document::Ptr &cppDocument)
{
    // Find optimal position
    using namespace IncludeUtils;
    LineForNewIncludeDirective finder(file->document(), cppDocument,
                                      LineForNewIncludeDirective::IgnoreMocIncludes,
                                      LineForNewIncludeDirective::AutoDetect);
    unsigned newLinesToPrepend = 0;
    unsigned newLinesToAppend = 0;
    const int insertLine = finder(include, &newLinesToPrepend, &newLinesToAppend);
    QTC_ASSERT(insertLine >= 1, return);
    const int insertPosition = file->position(insertLine, 1);
    QTC_ASSERT(insertPosition >= 0, return);

    // Construct text to insert
    const QString includeLine = QLatin1String("#include ") + include + QLatin1Char('\n');
    QString prependedNewLines, appendedNewLines;
    while (newLinesToAppend--)
        appendedNewLines += QLatin1String("\n");
    while (newLinesToPrepend--)
        prependedNewLines += QLatin1String("\n");
    const QString textToInsert = prependedNewLines + includeLine + appendedNewLines;

    // Insert
    ChangeSet changes;
    changes.insert(insertPosition, textToInsert);
    file->setChangeSet(changes);
    file->apply();
}

bool nameIncludesOperatorName(const Name *name)
{
    return name->asOperatorNameId()
        || (name->asQualifiedNameId() && name->asQualifiedNameId()->name()->asOperatorNameId());
}

QString memberBaseName(const QString &name)
{
    const auto validName = [](const QString &name) {
        return !name.isEmpty() && !name.at(0).isDigit();
    };
    QString baseName = name;

    CppQuickFixSettings *settings = CppQuickFixProjectsSettings::getQuickFixSettings(
        ProjectExplorer::ProjectTree::currentProject());
    const QString &nameTemplate = settings->memberVariableNameTemplate;
    const QString prefix = nameTemplate.left(nameTemplate.indexOf('<'));
    const QString postfix = nameTemplate.mid(nameTemplate.lastIndexOf('>') + 1);
    if (name.startsWith(prefix) && name.endsWith(postfix)) {
        const QString base = name.mid(prefix.length(), name.length() - postfix.length());
        if (validName(base))
            return base;
    }

    // Remove leading and trailing "_"
    while (baseName.startsWith(QLatin1Char('_')))
        baseName.remove(0, 1);
    while (baseName.endsWith(QLatin1Char('_')))
        baseName.chop(1);
    if (baseName != name && validName(baseName))
        return baseName;

    // If no leading/trailing "_": remove "m_" and "m" prefix
    if (baseName.startsWith(QLatin1String("m_"))) {
        baseName.remove(0, 2);
    } else if (baseName.startsWith(QLatin1Char('m')) && baseName.length() > 1
               && baseName.at(1).isUpper()) {
        baseName.remove(0, 1);
        baseName[0] = baseName.at(0).toLower();
    }

    return validName(baseName) ? baseName : name;
}

// Returns a non-null value if and only if the cursor is on the name of a (proper) class
// declaration or at some place inside the body of a class declaration that does not
// correspond to an AST of its own, i.e. on "empty space".
ClassSpecifierAST *astForClassOperations(const CppQuickFixInterface &interface)
{
    const QList<AST *> &path = interface.path();
    if (path.isEmpty())
        return nullptr;
    if (const auto classSpec = path.last()->asClassSpecifier()) // Cursor inside class decl?
        return classSpec;

    // Cursor on a class name?
    if (path.size() < 2)
        return nullptr;
    const SimpleNameAST * const nameAST = path.at(path.size() - 1)->asSimpleName();
    if (!nameAST || !interface.isCursorOn(nameAST))
       return nullptr;
    if (const auto classSpec = path.at(path.size() - 2)->asClassSpecifier())
        return classSpec;
    return nullptr;
}

QString nameString(const NameAST *name)
{
    return CppCodeStyleSettings::currentProjectCodeStyleOverview().prettyName(name->name);
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

        Control *control = context.bindings()->control().data();
        return rewriteType(result.first().type(), &env, control);
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
        CppRefactoringFilePtr currentFile = refactoring.file(filePath());

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

void InverseLogicalComparison::match(const CppQuickFixInterface &interface,
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
        CppRefactoringFilePtr currentFile = refactoring.file(filePath());

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

void FlipLogicalOperands::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
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

    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(filePath());

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

} // anonymous namespace

void RewriteLogicalAnd::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
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
        CppRefactoringFilePtr currentFile = refactoring.file(filePath());

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

} // anonymous namespace

void SplitSimpleDeclaration::match(const CppQuickFixInterface &interface,
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

class AddBracesToIfOp: public CppQuickFixOperation
{
public:
    AddBracesToIfOp(const CppQuickFixInterface &interface, int priority,
                    const IfStatementAST *statement)
        : CppQuickFixOperation(interface, priority)
        , _statement(statement)
    {
        setDescription(Tr::tr("Add Curly Braces"));
    }

    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(filePath());

        ChangeSet changes;

        const int start = currentFile->endOf(_statement->rparen_token);
        changes.insert(start, QLatin1String(" {"));

        const int end = currentFile->endOf(_statement->statement->lastToken() - 1);
        changes.insert(end, QLatin1String("\n}"));

        currentFile->setChangeSet(changes);
        currentFile->appendIndentRange(ChangeSet::Range(start, end));
        currentFile->apply();
    }

private:
    const IfStatementAST * const _statement;
};

} // anonymous namespace

void AddBracesToIf::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();
    if (path.isEmpty())
        return;

    // show when we're on the 'if' of an if statement
    int index = path.size() - 1;
    IfStatementAST *ifStatement = path.at(index)->asIfStatement();
    if (ifStatement && interface.isCursorOn(ifStatement->if_token) && ifStatement->statement
        && !ifStatement->statement->asCompoundStatement()) {
        result << new AddBracesToIfOp(interface, index, ifStatement);
        return;
    }

    // or if we're on the statement contained in the if
    // ### This may not be such a good idea, consider nested ifs...
    for (; index != -1; --index) {
        IfStatementAST *ifStatement = path.at(index)->asIfStatement();
        if (ifStatement && ifStatement->statement
            && interface.isCursorOn(ifStatement->statement)
            && !ifStatement->statement->asCompoundStatement()) {
            result << new AddBracesToIfOp(interface, index, ifStatement);
            return;
        }
    }

    // ### This could very well be extended to the else branch
    // and other nodes entirely.
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
        CppRefactoringFilePtr currentFile = refactoring.file(filePath());

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
    ConditionAST *condition = nullptr;
    IfStatementAST *pattern = nullptr;
    CoreDeclaratorAST *core = nullptr;
};

} // anonymous namespace

void MoveDeclarationOutOfIf::match(const CppQuickFixInterface &interface,
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
        CppRefactoringFilePtr currentFile = refactoring.file(filePath());

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
    ConditionAST *condition = nullptr;
    WhileStatementAST *pattern = nullptr;
    CoreDeclaratorAST *core = nullptr;
};

} // anonymous namespace

void MoveDeclarationOutOfWhile::match(const CppQuickFixInterface &interface,
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
        CppRefactoringFilePtr currentFile = refactoring.file(filePath());

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

} // anonymous namespace

void SplitIfStatement::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
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
    EncloseActionMask = EncloseInQLatin1CharAction
    | EncloseInQLatin1StringAction | EncloseInQStringLiteralAction,
    TranslateTrAction = 0x8,
    TranslateQCoreApplicationAction = 0x10,
    TranslateNoopAction = 0x20,
    TranslationMask = TranslateTrAction
    | TranslateQCoreApplicationAction | TranslateNoopAction,
    RemoveObjectiveCAction = 0x40,
    ConvertEscapeSequencesToCharAction = 0x100,
    ConvertEscapeSequencesToStringAction = 0x200,
    SingleQuoteAction = 0x400,
    DoubleQuoteAction = 0x800
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
        CppRefactoringFilePtr currentFile = refactoring.file(filePath());

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

void WrapStringLiteral::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
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
    }
}

void TranslateStringLiteral::match(const CppQuickFixInterface &interface,
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

    QSharedPointer<Control> control = interface.context().bindings()->control();
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
        CppRefactoringFilePtr currentFile = refactoring.file(filePath());

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

void ConvertCStringToNSString::match(const CppQuickFixInterface &interface,
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
        CppRefactoringFilePtr currentFile = refactoring.file(filePath());

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

void ConvertNumericLiteral::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
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
        CppRefactoringFilePtr currentFile = refactoring.file(filePath());
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
        CppRefactoringFilePtr currentFile = refactoring.file(filePath());
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
        CppRefactoringFilePtr currentFile = refactoring.file(filePath());

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

void ConvertToCamelCase::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
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
    CppRefactoringFilePtr file = refactoring.file(filePath());

    insertNewIncludeDirective(m_include, file, semanticInfo().doc);
}

AddForwardDeclForUndefinedIdentifierOp::AddForwardDeclForUndefinedIdentifierOp(
        const CppQuickFixInterface &interface,
        int priority,
        const QString &fqClassName,
        int symbolPos)
    : CppQuickFixOperation(interface, priority), m_className(fqClassName), m_symbolPos(symbolPos)
{
    setDescription(Tr::tr("Add forward declaration for %1").arg(m_className));
}

void AddForwardDeclForUndefinedIdentifierOp::perform()
{
    const QStringList parts = m_className.split("::");
    QTC_ASSERT(!parts.isEmpty(), return);
    const QStringList namespaces = parts.mid(0, parts.length() - 1);

    CppRefactoringChanges refactoring(snapshot());
    CppRefactoringFilePtr file = refactoring.file(filePath());

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

void AddIncludeForUndefinedIdentifier::match(const CppQuickFixInterface &interface,
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
        CppRefactoringFilePtr currentFile = refactoring.file(filePath());

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

void RearrangeParamDeclarationList::match(const CppQuickFixInterface &interface,
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
                "Reformat to \"%1\"").arg(m_change.operationList().constFirst().text);
        } else { // > 1
            description = Tr::tr("Reformat Pointers or References");
        }
        setDescription(description);
    }

    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(filePath());
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

void ReformatPointerDeclaration::match(const CppQuickFixInterface &interface,
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
        CppRefactoringFilePtr currentFile = refactoring.file(filePath());

        ChangeSet changes;
        int start = currentFile->endOf(compoundStatement->lbrace_token);
        changes.insert(start, QLatin1String("\ncase ")
                       + values.join(QLatin1String(":\nbreak;\ncase "))
                       + QLatin1String(":\nbreak;"));
        currentFile->setChangeSet(changes);
        currentFile->appendIndentRange(ChangeSet::Range(start, start + 1));
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

void CompleteSwitchCaseStatement::match(const CppQuickFixInterface &interface,
                                        QuickFixOperations &result)
{
    if (CppModelManager::usesClangd(interface.currentFile()->editor()->textDocument()))
        return;

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

        CppRefactoringFilePtr targetFile = refactoring.file(m_targetFilePath);
        int targetPosition1 = targetFile->position(loc.line(), loc.column());
        int targetPosition2 = qMax(0, targetFile->position(loc.line(), 1) - 1);

        ChangeSet target;
        target.insert(targetPosition1, loc.prefix() + m_decl);
        targetFile->setChangeSet(target);
        targetFile->appendIndentRange(ChangeSet::Range(targetPosition2, targetPosition1));
        targetFile->setOpenEditor(true, targetPosition1);
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

void InsertDeclFromDef::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
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
            ChangeSet *changeSet = nullptr,
            QList<ChangeSet::Range> *indentRanges = nullptr)
    {
        CppRefactoringChanges refactoring(op->snapshot());
        if (!loc.isValid())
            loc = insertLocationForMethodDefinition(decl, true, NamespaceHandling::Ignore,
                                                      refactoring, targetFilePath);
        QTC_ASSERT(loc.isValid(), return);

        CppRefactoringFilePtr targetFile = refactoring.file(loc.filePath());
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
            const ChangeSet::Range indentRange(targetPos, targetPos + 4);
            if (indentRanges)
                indentRanges->append(indentRange);
            else
                targetFile->appendIndentRange(indentRange);

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
            Control *control = op->context().bindings()->control().data();

            // rewrite the function type
            const FullySpecifiedType tn = rewriteType(decl->type(), &env, control);

            // rewrite the function name
            if (nameIncludesOperatorName(decl->name())) {
                CppRefactoringFilePtr file = refactoring.file(op->filePath());
                const QString operatorNameText = file->textOf(declAST->core_declarator);
                oo.includeWhiteSpaceInOperatorName = operatorNameText.contains(QLatin1Char(' '));
            }
            const QString name = oo.prettyName(LookupContext::minimalName(decl, targetCoN,
                                                                          control));
            const QString defText = inlinePrefix(
                        targetFilePath, [defPos] { return defPos == DefPosOutsideClass; })
                    + oo.prettyType(tn, name)
                    + QLatin1String("\n{\n\n}");

            const int targetPos = targetFile->position(loc.line(), loc.column());
            const int targetPos2 = qMax(0, targetFile->position(loc.line(), 1) - 1);

            ChangeSet localChangeSet;
            ChangeSet * const target = changeSet ? changeSet : &localChangeSet;
            target->insert(targetPos,  loc.prefix() + defText + loc.suffix());
            const ChangeSet::Range indentRange(targetPos2, targetPos);
            if (indentRanges)
                indentRanges->append(indentRange);
            else
                targetFile->appendIndentRange(indentRange);

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

void InsertDefFromDecl::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
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
                            if (!isFreeFunction) {
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

        CppRefactoringFilePtr targetFile = refactoring.file(filePath);
        const int targetPosition1 = targetFile->position(loc.line(), loc.column());
        const int targetPosition2 = qMax(0, targetFile->position(loc.line(), 1) - 1);
        ChangeSet target;
        target.insert(targetPosition1, loc.prefix() + decl + ";\n");
        targetFile->setChangeSet(target);
        targetFile->appendIndentRange(ChangeSet::Range(targetPosition2, targetPosition1));
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

void AddDeclarationForUndeclaredIdentifier::match(const CppQuickFixInterface &interface,
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

        QHash<FilePath, QPair<ChangeSet, QList<ChangeSet::Range>>> changeSets;
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
            auto &changeSet = changeSets[targetFilePath];
            InsertDefOperation::insertDefinition(
                        this, loc, setting.defPos, finder.decl()->declarator_list->value,
                        setting.func->asDeclaration(),targetFilePath,
                        &changeSet.first, &changeSet.second);
        }
        for (auto it = changeSets.cbegin(); it != changeSets.cend(); ++it) {
            const CppRefactoringFilePtr file = refactoring.file(it.key());
            for (const ChangeSet::Range &r : it.value().second)
                file->appendIndentRange(r);
            file->setChangeSet(it.value().first);
            file->apply();
        }
    }

    ClassSpecifierAST *m_classAST = nullptr;
    InsertDefsFromDecls::Mode m_mode;
    QList<Symbol *> m_declarations;
};


void InsertDefsFromDecls::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const auto op = QSharedPointer<InsertDefsOperation>::create(interface);
    op->setMode(m_mode);
    if (op->isApplicable())
        result << op;
}

namespace {

std::optional<FullySpecifiedType> getFirstTemplateParameter(const Name *name)
{
    if (const QualifiedNameId *qualifiedName = name->asQualifiedNameId())
        return getFirstTemplateParameter(qualifiedName->name());

    if (const TemplateNameId *templateName = name->asTemplateNameId()) {
        if (templateName->templateArgumentCount() > 0)
            return templateName->templateArgumentAt(0).type();
    }
    return {};
}

std::optional<FullySpecifiedType> getFirstTemplateParameter(Type *type)
{
    if (NamedType *namedType = type->asNamedType())
        return getFirstTemplateParameter(namedType->name());

    return {};
}

std::optional<FullySpecifiedType> getFirstTemplateParameter(FullySpecifiedType type)
{
    return getFirstTemplateParameter(type.type());
}

QString symbolAtDifferentLocation(const CppQuickFixInterface &interface,
                                  Symbol *symbol,
                                  const CppRefactoringFilePtr &targetFile,
                                  InsertionLocation targetLocation)
{
    QTC_ASSERT(symbol, return QString());
    Scope *scopeAtInsertPos = targetFile->cppDocument()->scopeAt(targetLocation.line(),
                                                                 targetLocation.column());

    LookupContext cppContext(targetFile->cppDocument(), interface.snapshot());
    ClassOrNamespace *cppCoN = cppContext.lookupType(scopeAtInsertPos);
    if (!cppCoN)
        cppCoN = cppContext.globalNamespace();
    SubstitutionEnvironment env;
    env.setContext(interface.context());
    env.switchScope(symbol->enclosingScope());
    UseMinimalNames q(cppCoN);
    env.enter(&q);
    Control *control = interface.context().bindings()->control().data();
    Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
    return oo.prettyName(LookupContext::minimalName(symbol, cppCoN, control));
}

FullySpecifiedType typeAtDifferentLocation(const CppQuickFixInterface &interface,
                                           FullySpecifiedType type,
                                           Scope *originalScope,
                                           const CppRefactoringFilePtr &targetFile,
                                           InsertionLocation targetLocation,
                                           const QStringList &newNamespaceNamesAtLoc = {})
{
    Scope *scopeAtInsertPos = targetFile->cppDocument()->scopeAt(targetLocation.line(),
                                                                 targetLocation.column());
    for (const QString &nsName : newNamespaceNamesAtLoc) {
        const QByteArray utf8Name = nsName.toUtf8();
        Control *control = targetFile->cppDocument()->control();
        const Name *name = control->identifier(utf8Name.data(), utf8Name.size());
        Namespace *ns = control->newNamespace(0, name);
        ns->setEnclosingScope(scopeAtInsertPos);
        scopeAtInsertPos = ns;
    }
    LookupContext cppContext(targetFile->cppDocument(), interface.snapshot());
    ClassOrNamespace *cppCoN = cppContext.lookupType(scopeAtInsertPos);
    if (!cppCoN)
        cppCoN = cppContext.globalNamespace();
    SubstitutionEnvironment env;
    env.setContext(interface.context());
    env.switchScope(originalScope);
    UseMinimalNames q(cppCoN);
    env.enter(&q);
    Control *control = interface.context().bindings()->control().data();
    return rewriteType(type, &env, control);
}

struct ExistingGetterSetterData
{
    Class *clazz = nullptr;
    Declaration *declarationSymbol = nullptr;
    QString getterName;
    QString setterName;
    QString resetName;
    QString signalName;
    QString qPropertyName;
    QString memberVariableName;
    Document::Ptr doc;

    int computePossibleFlags() const;
};

class GetterSetterRefactoringHelper
{
public:
    GetterSetterRefactoringHelper(CppQuickFixOperation *operation,
                                  const FilePath &filePath,
                                  Class *clazz)
        : m_operation(operation)
        , m_changes(m_operation->snapshot())
        , m_locator(m_changes)
        , m_headerFile(m_changes.file(filePath))
        , m_sourceFile([&] {
            FilePath cppFilePath = correspondingHeaderOrSource(filePath, &m_isHeaderHeaderFile);
            if (!m_isHeaderHeaderFile || !cppFilePath.exists()) {
                // there is no "source" file
                return m_headerFile;
            } else {
                return m_changes.file(cppFilePath);
            }
        }())
        , m_class(clazz)
    {}

    void performGeneration(ExistingGetterSetterData data, int generationFlags);

    void applyChanges()
    {
        const auto classLayout = {
            InsertionPointLocator::Public,
            InsertionPointLocator::PublicSlot,
            InsertionPointLocator::Signals,
            InsertionPointLocator::Protected,
            InsertionPointLocator::ProtectedSlot,
            InsertionPointLocator::PrivateSlot,
            InsertionPointLocator::Private,
        };
        for (auto spec : classLayout) {
            const auto iter = m_headerFileCode.find(spec);
            if (iter != m_headerFileCode.end()) {
                const InsertionLocation loc = headerLocationFor(spec);
                m_headerFile->setOpenEditor(true, m_headerFile->position(loc.line(), loc.column()));
                insertAndIndent(m_headerFile, loc, *iter);
            }
        }
        if (!m_sourceFileCode.isEmpty() && m_sourceFileInsertionPoint.isValid()) {
            m_sourceFile->setOpenEditor(true, m_sourceFile->position(
                                            m_sourceFileInsertionPoint.line(),
                                            m_sourceFileInsertionPoint.column()));
            insertAndIndent(m_sourceFile, m_sourceFileInsertionPoint, m_sourceFileCode);
        }

        if (!m_headerFileChangeSet.isEmpty()) {
            m_headerFile->setChangeSet(m_headerFileChangeSet);
            m_headerFile->apply();
        }
        if (!m_sourceFileChangeSet.isEmpty()) {
            m_sourceFile->setChangeSet(m_sourceFileChangeSet);
            m_sourceFile->apply();
        }
    }

    bool hasSourceFile() const { return m_headerFile != m_sourceFile; }

protected:
    void insertAndIndent(const RefactoringFilePtr &file,
                         const InsertionLocation &loc,
                         const QString &text)
    {
        int targetPosition1 = file->position(loc.line(), loc.column());
        int targetPosition2 = qMax(0, file->position(loc.line(), 1) - 1);
        ChangeSet &changeSet = file == m_headerFile ? m_headerFileChangeSet : m_sourceFileChangeSet;
        changeSet.insert(targetPosition1, loc.prefix() + text + loc.suffix());
        file->appendIndentRange(ChangeSet::Range(targetPosition2, targetPosition1));
    }

    FullySpecifiedType makeConstRef(FullySpecifiedType type)
    {
        type.setConst(true);
        return m_operation->currentFile()->cppDocument()->control()->referenceType(type, false);
    }

    FullySpecifiedType addConstToReference(FullySpecifiedType type)
    {
        if (ReferenceType *ref = type.type()->asReferenceType()) {
            FullySpecifiedType elemType = ref->elementType();
            if (elemType.isConst())
                return type;
            elemType.setConst(true);
            return m_operation->currentFile()->cppDocument()->control()->referenceType(elemType,
                                                                                       false);
        }
        return type;
    }

    QString symbolAt(Symbol *symbol,
                     const CppRefactoringFilePtr &targetFile,
                     InsertionLocation targetLocation)
    {
        return symbolAtDifferentLocation(*m_operation, symbol, targetFile, targetLocation);
    }

    FullySpecifiedType typeAt(FullySpecifiedType type,
                              Scope *originalScope,
                              const CppRefactoringFilePtr &targetFile,
                              InsertionLocation targetLocation,
                              const QStringList &newNamespaceNamesAtLoc = {})
    {
        return typeAtDifferentLocation(*m_operation,
                                       type,
                                       originalScope,
                                       targetFile,
                                       targetLocation,
                                       newNamespaceNamesAtLoc);
    }

    /**
     * @brief checks if the type in the enclosing scope in the header is a value type
     * @param type a type in the m_headerFile
     * @param enclosingScope the enclosing scope
     * @param customValueType if not nullptr set to true when value type comes
     * from CppQuickFixSettings::isValueType
     * @return true if it is a pointer, enum, integer, floating point, reference, custom value type
     */
    bool isValueType(FullySpecifiedType type, Scope *enclosingScope, bool *customValueType = nullptr)
    {
        if (customValueType)
            *customValueType = false;
        // a type is a value type if it is one of the following
        const auto isTypeValueType = [](const FullySpecifiedType &t) {
            return t->asPointerType() || t->asEnumType() || t->asIntegerType() || t->asFloatType()
                   || t->asReferenceType();
        };
        if (type->asNamedType()) {
            // we need a recursive search and a lookup context
            LookupContext context(m_headerFile->cppDocument(), m_changes.snapshot());
            auto isValueType = [settings = m_settings,
                                &customValueType,
                                &context,
                                &isTypeValueType](const Name *name,
                                                  Scope *scope,
                                                  auto &isValueType) mutable -> bool {
                // maybe the type is a custom value type by name
                if (const Identifier *id = name->identifier()) {
                    if (settings->isValueType(QString::fromUtf8(id->chars(), id->size()))) {
                        if (customValueType)
                            *customValueType = true;
                        return true;
                    }
                }
                // search for the type declaration
                QList<LookupItem> localLookup = context.lookup(name, scope);
                for (auto &&i : localLookup) {
                    if (isTypeValueType(i.type()))
                        return true;
                    if (i.type()->asNamedType()) { // check if we have to search recursively
                        const Name *newName = i.type()->asNamedType()->name();
                        Scope *newScope = i.declaration()->enclosingScope();
                        if (Matcher::match(newName, name)
                                && Matcher::match(newScope->name(), scope->name())) {
                            continue; // we have found the start location of the search
                        }
                        return isValueType(newName, newScope, isValueType);
                    }
                    return false;
                }
                return false;
            };
            // start recursion
            return isValueType(type->asNamedType()->name(), enclosingScope, isValueType);
        }
        return isTypeValueType(type);
    }

    bool isValueType(Symbol *symbol, bool *customValueType = nullptr)
    {
        return isValueType(symbol->type(), symbol->enclosingScope(), customValueType);
    }

    void addHeaderCode(InsertionPointLocator::AccessSpec spec, QString code)
    {
        QString &existing = m_headerFileCode[spec];
        existing += code;
        if (!existing.endsWith('\n'))
            existing += '\n';
    }

    InsertionLocation headerLocationFor(InsertionPointLocator::AccessSpec spec)
    {
        const auto insertionPoint = m_headerInsertionPoints.find(spec);
        if (insertionPoint != m_headerInsertionPoints.end())
            return *insertionPoint;
        const InsertionLocation loc = m_locator.methodDeclarationInClass(
                    m_headerFile->filePath(), m_class, spec,
                    InsertionPointLocator::ForceAccessSpec::Yes);
        m_headerInsertionPoints.insert(spec, loc);
        return loc;
    }

    InsertionLocation sourceLocationFor(Symbol *symbol, QStringList *insertedNamespaces = nullptr)
    {
        if (m_sourceFileInsertionPoint.isValid())
            return m_sourceFileInsertionPoint;
        m_sourceFileInsertionPoint
            = insertLocationForMethodDefinition(symbol,
                                                false,
                                                m_settings->createMissingNamespacesinCppFile()
                                                    ? NamespaceHandling::CreateMissing
                                                    : NamespaceHandling::Ignore,
                                                m_changes,
                                                m_sourceFile->filePath(),
                                                insertedNamespaces);
        if (m_settings->addUsingNamespaceinCppFile()) {
            // check if we have to insert a using namespace ...
            auto requiredNamespaces = getNamespaceNames(
                symbol->asClass() ? symbol : symbol->enclosingClass());
            NSCheckerVisitor visitor(m_sourceFile.get(),
                                     requiredNamespaces,
                                     m_sourceFile->position(m_sourceFileInsertionPoint.line(),
                                                            m_sourceFileInsertionPoint.column()));
            visitor.accept(m_sourceFile->cppDocument()->translationUnit()->ast());
            if (insertedNamespaces)
                insertedNamespaces->clear();
            if (auto rns = visitor.remainingNamespaces(); !rns.empty()) {
                QString ns = "using namespace ";
                for (auto &n : rns) {
                    if (!n.isEmpty()) { // we have to ignore unnamed namespaces
                        ns += n;
                        ns += "::";
                        if (insertedNamespaces)
                            insertedNamespaces->append(n);
                    }
                }
                ns.resize(ns.size() - 2); // remove last '::'
                ns += ";\n";
                const auto &loc = m_sourceFileInsertionPoint;
                m_sourceFileInsertionPoint = InsertionLocation(loc.filePath(),
                                                               loc.prefix() + ns,
                                                               loc.suffix(),
                                                               loc.line(),
                                                               loc.column());
            }
        }
        return m_sourceFileInsertionPoint;
    }

    void addSourceFileCode(QString code)
    {
        while (!m_sourceFileCode.isEmpty() && !m_sourceFileCode.endsWith("\n\n"))
            m_sourceFileCode += '\n';
        m_sourceFileCode += code;
    }

protected:
    CppQuickFixOperation *const m_operation;
    const CppRefactoringChanges m_changes;
    const InsertionPointLocator m_locator;
    const CppRefactoringFilePtr m_headerFile;
    bool m_isHeaderHeaderFile = false; // the "header" (where the class is defined) can be a source file
    const CppRefactoringFilePtr m_sourceFile;
    CppQuickFixSettings *const m_settings = CppQuickFixProjectsSettings::getQuickFixSettings(
        ProjectExplorer::ProjectTree::currentProject());
    Class *const m_class;

private:
    ChangeSet m_headerFileChangeSet;
    ChangeSet m_sourceFileChangeSet;
    QMap<InsertionPointLocator::AccessSpec, InsertionLocation> m_headerInsertionPoints;
    InsertionLocation m_sourceFileInsertionPoint;
    QString m_sourceFileCode;
    QMap<InsertionPointLocator::AccessSpec, QString> m_headerFileCode;
};

class GenerateGetterSetterOp : public CppQuickFixOperation
{
public:
    enum GenerateFlag {
        GenerateGetter = 1 << 0,
        GenerateSetter = 1 << 1,
        GenerateSignal = 1 << 2,
        GenerateMemberVariable = 1 << 3,
        GenerateReset = 1 << 4,
        GenerateProperty = 1 << 5,
        GenerateConstantProperty = 1 << 6,
        HaveExistingQProperty = 1 << 7,
        Invalid = -1,
    };

    GenerateGetterSetterOp(const CppQuickFixInterface &interface,
                           ExistingGetterSetterData data,
                           int generateFlags,
                           int priority,
                           const QString &description)
        : CppQuickFixOperation(interface)
        , m_generateFlags(generateFlags)
        , m_data(data)
    {
        setDescription(description);
        setPriority(priority);
    }

    static void generateQuickFixes(QuickFixOperations &results,
                                   const CppQuickFixInterface &interface,
                                   const ExistingGetterSetterData &data,
                                   const int possibleFlags)
    {
        // flags can have the value HaveExistingQProperty or a combination of all other values
        // of the enum 'GenerateFlag'
        int p = 0;
        if (possibleFlags & HaveExistingQProperty) {
            const QString desc = Tr::tr("Generate Missing Q_PROPERTY Members");
            results << new GenerateGetterSetterOp(interface, data, possibleFlags, ++p, desc);
        } else {
            if (possibleFlags & GenerateSetter) {
                const QString desc = Tr::tr("Generate Setter");
                results << new GenerateGetterSetterOp(interface, data, GenerateSetter, ++p, desc);
            }
            if (possibleFlags & GenerateGetter) {
                const QString desc = Tr::tr("Generate Getter");
                results << new GenerateGetterSetterOp(interface, data, GenerateGetter, ++p, desc);
            }
            if (possibleFlags & GenerateGetter && possibleFlags & GenerateSetter) {
                const QString desc = Tr::tr("Generate Getter and Setter");
                const int flags = GenerateGetter | GenerateSetter;
                results << new GenerateGetterSetterOp(interface, data, flags, ++p, desc);
            }

            if (possibleFlags & GenerateConstantProperty) {
                const QString desc = Tr::tr("Generate Constant Q_PROPERTY and Missing Members");
                const int flags = possibleFlags & ~(GenerateSetter | GenerateSignal | GenerateReset);
                results << new GenerateGetterSetterOp(interface, data, flags, ++p, desc);
            }
            if (possibleFlags & GenerateProperty) {
                if (possibleFlags & GenerateReset) {
                    const QString desc = Tr::tr(
                        "Generate Q_PROPERTY and Missing Members with Reset Function");
                    const int flags = possibleFlags & ~GenerateConstantProperty;
                    results << new GenerateGetterSetterOp(interface, data, flags, ++p, desc);
                }
                const QString desc = Tr::tr("Generate Q_PROPERTY and Missing Members");
                const int flags = possibleFlags & ~GenerateConstantProperty & ~GenerateReset;
                results << new GenerateGetterSetterOp(interface, data, flags, ++p, desc);
            }
        }
    }

    void perform() override
    {
        GetterSetterRefactoringHelper helper(this, currentFile()->filePath(), m_data.clazz);
        helper.performGeneration(m_data, m_generateFlags);
        helper.applyChanges();
    }

private:
    int m_generateFlags;
    ExistingGetterSetterData m_data;
};

int ExistingGetterSetterData::computePossibleFlags() const
{
    const bool isConst = declarationSymbol->type().isConst();
    const bool isStatic = declarationSymbol->type().isStatic();
    using Flag = GenerateGetterSetterOp::GenerateFlag;
    int generateFlags = 0;
    if (getterName.isEmpty())
        generateFlags |= Flag::GenerateGetter;
    if (!isConst) {
        if (resetName.isEmpty())
            generateFlags |= Flag::GenerateReset;
        if (!isStatic && signalName.isEmpty() && setterName.isEmpty())
            generateFlags |= Flag::GenerateSignal;
        if (setterName.isEmpty())
            generateFlags |= Flag::GenerateSetter;
    }
    if (!isStatic) {
        const bool hasSignal = !signalName.isEmpty() || generateFlags & Flag::GenerateSignal;
        if (!isConst && hasSignal)
            generateFlags |= Flag::GenerateProperty;
    }
    if (setterName.isEmpty() && signalName.isEmpty())
        generateFlags |= Flag::GenerateConstantProperty;
    return generateFlags;
}

void GetterSetterRefactoringHelper::performGeneration(ExistingGetterSetterData data, int generateFlags)
{
    using Flag = GenerateGetterSetterOp::GenerateFlag;

    if (generateFlags & Flag::GenerateGetter && data.getterName.isEmpty()) {
        data.getterName = m_settings->getGetterName(data.qPropertyName);
        if (data.getterName == data.memberVariableName) {
            data.getterName = "get" + data.memberVariableName.left(1).toUpper()
                              + data.memberVariableName.mid(1);
        }
    }
    if (generateFlags & Flag::GenerateSetter && data.setterName.isEmpty())
        data.setterName = m_settings->getSetterName(data.qPropertyName);
    if (generateFlags & Flag::GenerateSignal && data.signalName.isEmpty())
        data.signalName = m_settings->getSignalName(data.qPropertyName);
    if (generateFlags & Flag::GenerateReset && data.resetName.isEmpty())
        data.resetName = m_settings->getResetName(data.qPropertyName);

    FullySpecifiedType memberVariableType = data.declarationSymbol->type();
    memberVariableType.setConst(false);
    const bool isMemberVariableStatic = memberVariableType.isStatic();
    memberVariableType.setStatic(false);
    Overview overview = CppCodeStyleSettings::currentProjectCodeStyleOverview();
    overview.showTemplateParameters = false;
    // TODO does not work with using. e.g. 'using foo = std::unique_ptr<int>'
    // TODO must be fully qualified
    auto getSetTemplate = m_settings->findGetterSetterTemplate(overview.prettyType(memberVariableType));
    overview.showTemplateParameters = true;

    // Ok... - If a type is a Named type we have to search recusive for the real type
    const bool isValueType = this->isValueType(memberVariableType,
                                               data.declarationSymbol->enclosingScope());
    const FullySpecifiedType parameterType = isValueType ? memberVariableType
                                                         : makeConstRef(memberVariableType);

    QString baseName = memberBaseName(data.memberVariableName);
    if (baseName.isEmpty())
        baseName = data.memberVariableName;

    const QString parameterName = m_settings->getSetterParameterName(baseName);
    if (parameterName == data.memberVariableName)
        data.memberVariableName = "this->" + data.memberVariableName;

    getSetTemplate.replacePlaceholders(data.memberVariableName, parameterName);

    using Pattern = CppQuickFixSettings::GetterSetterTemplate;
    std::optional<FullySpecifiedType> returnTypeTemplateParameter;
    if (getSetTemplate.returnTypeTemplate.has_value()) {
        QString returnTypeTemplate = getSetTemplate.returnTypeTemplate.value();
        if (returnTypeTemplate.contains(Pattern::TEMPLATE_PARAMETER_PATTERN)) {
            returnTypeTemplateParameter = getFirstTemplateParameter(data.declarationSymbol->type());
            if (!returnTypeTemplateParameter.has_value())
                return; // Maybe report error to the user
        }
    }
    const FullySpecifiedType returnTypeHeader = [&] {
        if (!getSetTemplate.returnTypeTemplate.has_value())
            return m_settings->returnByConstRef ? parameterType : memberVariableType;
        QString typeTemplate = getSetTemplate.returnTypeTemplate.value();
        if (returnTypeTemplateParameter.has_value())
            typeTemplate.replace(Pattern::TEMPLATE_PARAMETER_PATTERN,
                                 overview.prettyType(returnTypeTemplateParameter.value()));
        if (typeTemplate.contains(Pattern::TYPE_PATTERN))
            typeTemplate.replace(Pattern::TYPE_PATTERN,
                                 overview.prettyType(data.declarationSymbol->type()));
        Control *control = m_operation->currentFile()->cppDocument()->control();
        std::string utf8TypeName = typeTemplate.toUtf8().toStdString();
        return FullySpecifiedType(control->namedType(control->identifier(utf8TypeName.c_str())));
    }();

    // getter declaration
    if (generateFlags & Flag::GenerateGetter) {
        // maybe we added 'this->' to memberVariableName because of a collision with parameterName
        // but here the 'this->' is not needed
        const QString returnExpression = QString{getSetTemplate.returnExpression}.replace("this->",
                                                                                          "");
        QString getterInClassDeclaration = overview.prettyType(returnTypeHeader, data.getterName)
                                           + QLatin1String("()");
        if (isMemberVariableStatic)
            getterInClassDeclaration.prepend(QLatin1String("static "));
        else
            getterInClassDeclaration += QLatin1String(" const");
        getterInClassDeclaration.prepend(m_settings->getterAttributes + QLatin1Char(' '));

        auto getterLocation = m_settings->determineGetterLocation(1);
        // if we have an anonymous class we must add code inside the class
        if (data.clazz->name()->asAnonymousNameId())
            getterLocation = CppQuickFixSettings::FunctionLocation::InsideClass;

        if (getterLocation == CppQuickFixSettings::FunctionLocation::InsideClass) {
            getterInClassDeclaration += QLatin1String("\n{\nreturn ") + returnExpression
                                        + QLatin1String(";\n}\n");
        } else {
            getterInClassDeclaration += QLatin1String(";\n");
        }
        addHeaderCode(InsertionPointLocator::Public, getterInClassDeclaration);
        if (getterLocation == CppQuickFixSettings::FunctionLocation::CppFile && !hasSourceFile())
            getterLocation = CppQuickFixSettings::FunctionLocation::OutsideClass;

        if (getterLocation != CppQuickFixSettings::FunctionLocation::InsideClass) {
            const auto getReturnTypeAt = [&](CppRefactoringFilePtr targetFile,
                                             InsertionLocation targetLoc) {
                if (getSetTemplate.returnTypeTemplate.has_value()) {
                    QString returnType = getSetTemplate.returnTypeTemplate.value();
                    if (returnTypeTemplateParameter.has_value()) {
                        const QString templateTypeName = overview.prettyType(typeAt(
                            returnTypeTemplateParameter.value(), data.clazz, targetFile, targetLoc));
                        returnType.replace(Pattern::TEMPLATE_PARAMETER_PATTERN, templateTypeName);
                    }
                    if (returnType.contains(Pattern::TYPE_PATTERN)) {
                        const QString declarationType = overview.prettyType(
                            typeAt(memberVariableType, data.clazz, targetFile, targetLoc));
                        returnType.replace(Pattern::TYPE_PATTERN, declarationType);
                    }
                    Control *control = m_operation->currentFile()->cppDocument()->control();
                    std::string utf8String = returnType.toUtf8().toStdString();
                    return FullySpecifiedType(
                        control->namedType(control->identifier(utf8String.c_str())));
                } else {
                    FullySpecifiedType returnType = typeAt(memberVariableType,
                                                           data.clazz,
                                                           targetFile,
                                                           targetLoc);
                    if (m_settings->returnByConstRef && !isValueType)
                        return makeConstRef(returnType);
                    return returnType;
                }
            };
            const QString constSpec = isMemberVariableStatic ? QLatin1String("")
                                                             : QLatin1String(" const");
            if (getterLocation == CppQuickFixSettings::FunctionLocation::CppFile) {
                InsertionLocation loc = sourceLocationFor(data.declarationSymbol);
                FullySpecifiedType returnType;
                QString clazz;
                if (m_settings->rewriteTypesinCppFile()) {
                    returnType = getReturnTypeAt(m_sourceFile, loc);
                    clazz = symbolAt(data.clazz, m_sourceFile, loc);
                } else {
                    returnType = returnTypeHeader;
                    const Identifier *identifier = data.clazz->name()->identifier();
                    clazz = QString::fromUtf8(identifier->chars(), identifier->size());
                }
                const QString code = overview.prettyType(returnType, clazz + "::" + data.getterName)
                                     + "()" + constSpec + "\n{\nreturn " + returnExpression + ";\n}";
                addSourceFileCode(code);
            } else if (getterLocation == CppQuickFixSettings::FunctionLocation::OutsideClass) {
                InsertionLocation loc
                    = insertLocationForMethodDefinition(data.declarationSymbol,
                                                        false,
                                                        NamespaceHandling::Ignore,
                                                        m_changes,
                                                        m_headerFile->filePath());
                const FullySpecifiedType returnType = getReturnTypeAt(m_headerFile, loc);
                const QString clazz = symbolAt(data.clazz, m_headerFile, loc);
                QString code = overview.prettyType(returnType, clazz + "::" + data.getterName)
                               + "()" + constSpec + "\n{\nreturn " + returnExpression + ";\n}";
                if (m_isHeaderHeaderFile)
                    code.prepend("inline ");
                insertAndIndent(m_headerFile, loc, code);
            }
        }
    }

    // setter declaration
    InsertionPointLocator::AccessSpec setterAccessSpec = InsertionPointLocator::Public;
    if (m_settings->setterAsSlot) {
        const QByteArray connectName = "connect";
        const Identifier connectId(connectName.data(), connectName.size());
        const QList<LookupItem> items = m_operation->context().lookup(&connectId, data.clazz);
        for (const LookupItem &item : items) {
            if (item.declaration() && item.declaration()->enclosingClass()
                    && overview.prettyName(item.declaration()->enclosingClass()->name())
                    == "QObject") {
                setterAccessSpec = InsertionPointLocator::PublicSlot;
                break;
            }
        }
    }
    const auto createSetterBodyWithSignal = [this, &getSetTemplate, &data] {
        QString body;
        QTextStream setter(&body);
        setter << "if (" << getSetTemplate.equalComparison << ")\nreturn;\n";

        setter << getSetTemplate.assignment << ";\n";
        if (m_settings->signalWithNewValue)
            setter << "emit " << data.signalName << "(" << getSetTemplate.returnExpression << ");\n";
        else
            setter << "emit " << data.signalName << "();\n";

        return body;
    };
    if (generateFlags & Flag::GenerateSetter) {
        QString headerDeclaration = "void " + data.setterName + '('
                                    + overview.prettyType(addConstToReference(parameterType),
                                                          parameterName)
                                    + ")";
        if (isMemberVariableStatic)
            headerDeclaration.prepend("static ");
        QString body = "\n{\n";
        if (data.signalName.isEmpty())
            body += getSetTemplate.assignment + ";\n";
        else
            body += createSetterBodyWithSignal();

        body += "}";

        auto setterLocation = m_settings->determineSetterLocation(body.count('\n') - 2);
        // if we have an anonymous class we must add code inside the class
        if (data.clazz->name()->asAnonymousNameId())
            setterLocation = CppQuickFixSettings::FunctionLocation::InsideClass;

        if (setterLocation == CppQuickFixSettings::FunctionLocation::CppFile && !hasSourceFile())
            setterLocation = CppQuickFixSettings::FunctionLocation::OutsideClass;

        if (setterLocation == CppQuickFixSettings::FunctionLocation::InsideClass) {
            headerDeclaration += body;
        } else {
            headerDeclaration += ";\n";
            if (setterLocation == CppQuickFixSettings::FunctionLocation::CppFile) {
                InsertionLocation loc = sourceLocationFor(data.declarationSymbol);
                QString clazz;
                FullySpecifiedType newParameterType = parameterType;
                if (m_settings->rewriteTypesinCppFile()) {
                    newParameterType = typeAt(memberVariableType, data.clazz, m_sourceFile, loc);
                    if (!isValueType)
                        newParameterType = makeConstRef(newParameterType);
                    clazz = symbolAt(data.clazz, m_sourceFile, loc);
                } else {
                    const Identifier *identifier = data.clazz->name()->identifier();
                    clazz = QString::fromUtf8(identifier->chars(), identifier->size());
                }
                newParameterType = addConstToReference(newParameterType);
                const QString code = "void " + clazz + "::" + data.setterName + '('
                                     + overview.prettyType(newParameterType, parameterName) + ')'
                                     + body;
                addSourceFileCode(code);
            } else if (setterLocation == CppQuickFixSettings::FunctionLocation::OutsideClass) {
                InsertionLocation loc
                    = insertLocationForMethodDefinition(data.declarationSymbol,
                                                        false,
                                                        NamespaceHandling::Ignore,
                                                        m_changes,
                                                        m_headerFile->filePath());

                FullySpecifiedType newParameterType = typeAt(data.declarationSymbol->type(),
                                                             data.clazz,
                                                             m_headerFile,
                                                             loc);
                if (!isValueType)
                    newParameterType = makeConstRef(newParameterType);
                newParameterType = addConstToReference(newParameterType);
                QString clazz = symbolAt(data.clazz, m_headerFile, loc);

                QString code = "void " + clazz + "::" + data.setterName + '('
                               + overview.prettyType(newParameterType, parameterName) + ')' + body;
                if (m_isHeaderHeaderFile)
                    code.prepend("inline ");
                insertAndIndent(m_headerFile, loc, code);
            }
        }
        addHeaderCode(setterAccessSpec, headerDeclaration);
    }

    // reset declaration
    if (generateFlags & Flag::GenerateReset) {
        QString headerDeclaration = "void " + data.resetName + "()";
        if (isMemberVariableStatic)
            headerDeclaration.prepend("static ");
        QString body = "\n{\n";
        if (!data.setterName.isEmpty()) {
            body += data.setterName + "({}); // TODO: Adapt to use your actual default value\n";
        } else {
            body += "static $TYPE defaultValue{}; "
                    "// TODO: Adapt to use your actual default value\n";
            if (data.signalName.isEmpty())
                body += getSetTemplate.assignment + ";\n";
            else
                body += createSetterBodyWithSignal();
        }
        body += "}";

        // the template use <parameterName> as new value name, but we want to use 'defaultValue'
        body.replace(QRegularExpression("\\b" + parameterName + "\\b"), "defaultValue");
        // body.count('\n') - 2 : do not count the 2 at start
        auto resetLocation = m_settings->determineSetterLocation(body.count('\n') - 2);
        // if we have an anonymous class we must add code inside the class
        if (data.clazz->name()->asAnonymousNameId())
            resetLocation = CppQuickFixSettings::FunctionLocation::InsideClass;

        if (resetLocation == CppQuickFixSettings::FunctionLocation::CppFile && !hasSourceFile())
            resetLocation = CppQuickFixSettings::FunctionLocation::OutsideClass;

        if (resetLocation == CppQuickFixSettings::FunctionLocation::InsideClass) {
            headerDeclaration += body.replace("$TYPE", overview.prettyType(memberVariableType));
        } else {
            headerDeclaration += ";\n";
            if (resetLocation == CppQuickFixSettings::FunctionLocation::CppFile) {
                const InsertionLocation loc = sourceLocationFor(data.declarationSymbol);
                QString clazz;
                FullySpecifiedType type = memberVariableType;
                if (m_settings->rewriteTypesinCppFile()) {
                    type = typeAt(memberVariableType, data.clazz, m_sourceFile, loc);
                    clazz = symbolAt(data.clazz, m_sourceFile, loc);
                } else {
                    const Identifier *identifier = data.clazz->name()->identifier();
                    clazz = QString::fromUtf8(identifier->chars(), identifier->size());
                }
                const QString code = "void " + clazz + "::" + data.resetName + "()"
                                     + body.replace("$TYPE", overview.prettyType(type));
                addSourceFileCode(code);
            } else if (resetLocation == CppQuickFixSettings::FunctionLocation::OutsideClass) {
                const InsertionLocation loc = insertLocationForMethodDefinition(
                    data.declarationSymbol,
                    false,
                    NamespaceHandling::Ignore,
                    m_changes,
                    m_headerFile->filePath());
                const FullySpecifiedType type = typeAt(data.declarationSymbol->type(),
                                                       data.clazz,
                                                       m_headerFile,
                                                       loc);
                const QString clazz = symbolAt(data.clazz, m_headerFile, loc);
                QString code = "void " + clazz + "::" + data.resetName + "()"
                               + body.replace("$TYPE", overview.prettyType(type));
                if (m_isHeaderHeaderFile)
                    code.prepend("inline ");
                insertAndIndent(m_headerFile, loc, code);
            }
        }
        addHeaderCode(setterAccessSpec, headerDeclaration);
    }

    // signal declaration
    if (generateFlags & Flag::GenerateSignal) {
        const auto &parameter = overview.prettyType(returnTypeHeader, data.qPropertyName);
        const QString newValue = m_settings->signalWithNewValue ? parameter : QString();
        const QString declaration = QString("void %1(%2);\n").arg(data.signalName, newValue);
        addHeaderCode(InsertionPointLocator::Signals, declaration);
    }

    // member variable
    if (generateFlags & Flag::GenerateMemberVariable) {
        QString storageDeclaration = overview.prettyType(memberVariableType, data.memberVariableName);
        if (memberVariableType->asPointerType()
            && m_operation->semanticInfo().doc->translationUnit()->languageFeatures().cxx11Enabled) {
            storageDeclaration.append(" = nullptr");
        }
        storageDeclaration.append(";\n");
        addHeaderCode(InsertionPointLocator::Private, storageDeclaration);
    }

    // Q_PROPERTY
    if (generateFlags & Flag::GenerateProperty || generateFlags & Flag::GenerateConstantProperty) {
        // Use the returnTypeHeader as base because of custom types in getSetTemplates.
        // Remove const reference from type.
        FullySpecifiedType type = returnTypeHeader;
        if (ReferenceType *ref = type.type()->asReferenceType())
            type = ref->elementType();
        type.setConst(false);

        QString propertyDeclaration = QLatin1String("Q_PROPERTY(")
                                      + overview.prettyType(type,
                                                            memberBaseName(data.memberVariableName));
        bool needMember = false;
        if (data.getterName.isEmpty())
            needMember = true;
        else
            propertyDeclaration += QLatin1String(" READ ") + data.getterName;
        if (generateFlags & Flag::GenerateConstantProperty) {
            if (needMember)
                propertyDeclaration += QLatin1String(" MEMBER ") + data.memberVariableName;
            propertyDeclaration.append(QLatin1String(" CONSTANT"));
        } else {
            if (data.setterName.isEmpty()) {
                needMember = true;
            } else if (!getSetTemplate.returnTypeTemplate.has_value()) {
                // if the return type of the getter and then Q_PROPERTY is different than
                // the setter type, we should not add WRITE to the Q_PROPERTY
                propertyDeclaration.append(QLatin1String(" WRITE ")).append(data.setterName);
            }
            if (needMember)
                propertyDeclaration += QLatin1String(" MEMBER ") + data.memberVariableName;
            if (!data.resetName.isEmpty())
                propertyDeclaration += QLatin1String(" RESET ") + data.resetName;
            propertyDeclaration.append(QLatin1String(" NOTIFY ")).append(data.signalName);
        }

        propertyDeclaration.append(QLatin1String(" FINAL)\n"));
        addHeaderCode(InsertionPointLocator::Private, propertyDeclaration);
    }
}

QStringList toStringList(const QList<Symbol *> names)
{
    QStringList list;
    list.reserve(names.size());
    for (const auto symbol : names) {
        const Identifier *const id = symbol->identifier();
        list << QString::fromUtf8(id->chars(), id->size());
    }
    return list;
}

void findExistingFunctions(ExistingGetterSetterData &existing, QStringList memberFunctionNames)
{
    const CppQuickFixSettings *settings = CppQuickFixProjectsSettings::getQuickFixSettings(
        ProjectExplorer::ProjectTree::currentProject());
    const QString lowerBaseName = memberBaseName(existing.memberVariableName).toLower();
    const QStringList getterNames{lowerBaseName,
                                  "get_" + lowerBaseName,
                                  "get" + lowerBaseName,
                                  "is_" + lowerBaseName,
                                  "is" + lowerBaseName,
                                  settings->getGetterName(lowerBaseName)};
    const QStringList setterNames{"set_" + lowerBaseName,
                                  "set" + lowerBaseName,
                                  settings->getSetterName(lowerBaseName)};
    const QStringList resetNames{"reset_" + lowerBaseName,
                                 "reset" + lowerBaseName,
                                 settings->getResetName(lowerBaseName)};
    const QStringList signalNames{lowerBaseName + "_changed",
                                  lowerBaseName + "changed",
                                  settings->getSignalName(lowerBaseName)};
    for (const auto &memberFunctionName : memberFunctionNames) {
        const QString lowerName = memberFunctionName.toLower();
        if (getterNames.contains(lowerName))
            existing.getterName = memberFunctionName;
        else if (setterNames.contains(lowerName))
            existing.setterName = memberFunctionName;
        else if (resetNames.contains(lowerName))
            existing.resetName = memberFunctionName;
        else if (signalNames.contains(lowerName))
            existing.signalName = memberFunctionName;
    }
}

QList<Symbol *> getMemberFunctions(const Class *clazz)
{
    QList<Symbol *> memberFunctions;
    for (auto it = clazz->memberBegin(); it != clazz->memberEnd(); ++it) {
        Symbol *const s = *it;
        if (!s->identifier() || !s->type())
            continue;
        if ((s->asDeclaration() && s->type()->asFunctionType()) || s->asFunction())
            memberFunctions << s;
    }
    return memberFunctions;
}

} // anonymous namespace

void GenerateGetterSetter::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    ExistingGetterSetterData existing;

    const QList<AST *> &path = interface.path();
    // We expect something like
    // [0] TranslationUnitAST
    // [1] NamespaceAST
    // [2] LinkageBodyAST
    // [3] SimpleDeclarationAST
    // [4] ClassSpecifierAST
    // [5] SimpleDeclarationAST
    // [6] DeclaratorAST
    // [7] DeclaratorIdAST
    // [8] SimpleNameAST

    const int n = path.size();
    if (n < 6)
        return;

    int i = 1;
    const auto variableNameAST = path.at(n - i++)->asSimpleName();
    const auto declaratorId = path.at(n - i++)->asDeclaratorId();
    // DeclaratorAST might be preceded by PointerAST, e.g. for the case
    // "class C { char *@s; };", where '@' denotes the text cursor position.
    auto declarator = path.at(n - i++)->asDeclarator();
    if (!declarator) {
        --i;
        if (path.at(n - i++)->asPointer()) {
            if (n < 7)
                return;
            declarator = path.at(n - i++)->asDeclarator();
        }
        if (!declarator)
            return;
    }
    const auto variableDecl = path.at(n - i++)->asSimpleDeclaration();
    const auto classSpecifier = path.at(n - i++)->asClassSpecifier();
    const auto classDecl = path.at(n - i++)->asSimpleDeclaration();

    if (!(variableNameAST && declaratorId && variableDecl && classSpecifier && classDecl))
        return;

    // Do not get triggered on member functconstions and arrays
    if (declarator->postfix_declarator_list) {
        return;
    }

    // Construct getter and setter names
    const Name *variableName = variableNameAST->name;
    if (!variableName) {
        return;
    }
    const Identifier *variableId = variableName->identifier();
    if (!variableId) {
        return;
    }
    existing.memberVariableName = QString::fromUtf8(variableId->chars(), variableId->size());

    // Find the right symbol (for typeName) in the simple declaration
    Symbol *symbol = nullptr;
    const List<Symbol *> *symbols = variableDecl->symbols;
    QTC_ASSERT(symbols, return );
    for (; symbols; symbols = symbols->next) {
        Symbol *s = symbols->value;
        if (const Name *name = s->name()) {
            if (const Identifier *id = name->identifier()) {
                const QString symbolName = QString::fromUtf8(id->chars(), id->size());
                if (symbolName == existing.memberVariableName) {
                    symbol = s;
                    break;
                }
            }
        }
    }
    if (!symbol) {
        // no type can be determined
        return;
    }
    if (!symbol->asDeclaration()) {
        return;
    }
    existing.declarationSymbol = symbol->asDeclaration();

    existing.clazz = classSpecifier->symbol;
    if (!existing.clazz)
        return;

    auto file = interface.currentFile();
    // check if a Q_PROPERTY exist
    const QString baseName = memberBaseName(existing.memberVariableName);
    // eg: we have 'int m_test' and now 'Q_PROPERTY(int foo WRITE setTest MEMBER m_test NOTIFY tChanged)'
    for (auto it = classSpecifier->member_specifier_list; it; it = it->next) {
        if (it->value->asQtPropertyDeclaration()) {
            auto propDecl = it->value->asQtPropertyDeclaration();
            // iterator over 'READ ...', ...
            auto p = propDecl->property_declaration_item_list;
            // first check, if we have a MEMBER and the member is equal to the baseName
            for (; p; p = p->next) {
                const char *tokenString = file->tokenAt(p->value->item_name_token).spell();
                if (!qstrcmp(tokenString, "MEMBER")) {
                    if (baseName == file->textOf(p->value->expression))
                        return;
                }
            }
            // no MEMBER, but maybe the property name is the same
            const QString propertyName = file->textOf(propDecl->property_name);
            // we compare the baseName. e.g. 'test' instead of 'm_test'
            if (propertyName == baseName)
                return; // TODO Maybe offer quick fix "Add missing Q_PROPERTY Members"
        }
    }

    findExistingFunctions(existing, toStringList(getMemberFunctions(existing.clazz)));
    existing.qPropertyName = memberBaseName(existing.memberVariableName);

    const int possibleFlags = existing.computePossibleFlags();
    GenerateGetterSetterOp::generateQuickFixes(result, interface, existing, possibleFlags);
}

class MemberInfo
{
public:
    MemberInfo(ExistingGetterSetterData data, int possibleFlags)
        : data(data)
        , possibleFlags(possibleFlags)
    {}

    ExistingGetterSetterData data;
    int possibleFlags;
    int requestedFlags = 0;
};
using GetterSetterCandidates = std::vector<MemberInfo>;

class CandidateTreeItem : public TreeItem
{
public:
    enum Column {
        NameColumn,
        GetterColumn,
        SetterColumn,
        SignalColumn,
        ResetColumn,
        QPropertyColumn,
        ConstantQPropertyColumn
    };
    using Flag = GenerateGetterSetterOp::GenerateFlag;
    constexpr static Flag ColumnFlag[] = {
        Flag::Invalid,
        Flag::GenerateGetter,
        Flag::GenerateSetter,
        Flag::GenerateSignal,
        Flag::GenerateReset,
        Flag::GenerateProperty,
        Flag::GenerateConstantProperty,
    };

    CandidateTreeItem(MemberInfo *memberInfo)
        : m_memberInfo(memberInfo)
    {}

private:
    QVariant data(int column, int role) const override
    {
        if (role == Qt::DisplayRole && column == NameColumn)
            return m_memberInfo->data.memberVariableName;
        if (role == Qt::CheckStateRole && column > 0
            && column <= static_cast<int>(std::size(ColumnFlag))) {
            return m_memberInfo->requestedFlags & ColumnFlag[column] ? Qt::Checked : Qt::Unchecked;
        }
        return {};
    }

    bool setData(int column, const QVariant &data, int role) override
    {
        if (column < 1 || column > static_cast<int>(std::size(ColumnFlag)))
            return false;
        if (role != Qt::CheckStateRole)
            return false;
        if (!(m_memberInfo->possibleFlags & ColumnFlag[column]))
            return false;
        const bool nowChecked = data.toInt() == Qt::Checked;
        if (nowChecked)
            m_memberInfo->requestedFlags |= ColumnFlag[column];
        else
            m_memberInfo->requestedFlags &= ~ColumnFlag[column];

        if (nowChecked) {
            if (column == QPropertyColumn) {
                m_memberInfo->requestedFlags |= Flag::GenerateGetter;
                m_memberInfo->requestedFlags |= Flag::GenerateSetter;
                m_memberInfo->requestedFlags |= Flag::GenerateSignal;
                m_memberInfo->requestedFlags &= ~Flag::GenerateConstantProperty;
            } else if (column == ConstantQPropertyColumn) {
                m_memberInfo->requestedFlags |= Flag::GenerateGetter;
                m_memberInfo->requestedFlags &= ~Flag::GenerateSetter;
                m_memberInfo->requestedFlags &= ~Flag::GenerateSignal;
                m_memberInfo->requestedFlags &= ~Flag::GenerateReset;
                m_memberInfo->requestedFlags &= ~Flag::GenerateProperty;
            } else if (column == SetterColumn || column == SignalColumn || column == ResetColumn) {
                m_memberInfo->requestedFlags &= ~Flag::GenerateConstantProperty;
            }
        } else {
            if (column == SignalColumn)
                m_memberInfo->requestedFlags &= ~Flag::GenerateProperty;
        }
        for (int i = 0; i < 16; ++i) {
            const bool allowed = m_memberInfo->possibleFlags & (1 << i);
            if (!allowed)
                m_memberInfo->requestedFlags &= ~(1 << i); // clear bit
        }
        update();
        return true;
    }

    Qt::ItemFlags flags(int column) const override
    {
        if (column == NameColumn)
            return Qt::ItemIsEnabled;
        if (column < 1 || column > static_cast<int>(std::size(ColumnFlag)))
            return {};
        if (m_memberInfo->possibleFlags & ColumnFlag[column])
            return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
        return {};
    }

    MemberInfo *const m_memberInfo;
};

class GenerateGettersSettersDialog : public QDialog
{
    static constexpr CandidateTreeItem::Column CheckBoxColumn[4]
        = {CandidateTreeItem::Column::GetterColumn,
           CandidateTreeItem::Column::SetterColumn,
           CandidateTreeItem::Column::SignalColumn,
           CandidateTreeItem::Column::QPropertyColumn};

public:
    GenerateGettersSettersDialog(const GetterSetterCandidates &candidates)
        : QDialog()
        , m_candidates(candidates)
    {
        using Flags = GenerateGetterSetterOp::GenerateFlag;
        setWindowTitle(Tr::tr("Getters and Setters"));
        const auto model = new TreeModel<TreeItem, CandidateTreeItem>(this);
        model->setHeader(QStringList({
            Tr::tr("Member"),
            Tr::tr("Getter"),
            Tr::tr("Setter"),
            Tr::tr("Signal"),
            Tr::tr("Reset"),
            Tr::tr("QProperty"),
            Tr::tr("Constant QProperty"),
        }));
        for (MemberInfo &candidate : m_candidates)
            model->rootItem()->appendChild(new CandidateTreeItem(&candidate));
        const auto view = new BaseTreeView(this);
        view->setModel(model);
        int optimalWidth = 0;
        for (int i = 0; i < model->columnCount(QModelIndex{}); ++i) {
            view->resizeColumnToContents(i);
            optimalWidth += view->columnWidth(i);
        }

        const auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        const auto setCheckStateForAll = [model](int column, int checkState) {
            for (int i = 0; i < model->rowCount(); ++i)
                model->setData(model->index(i, column), checkState, Qt::CheckStateRole);
        };
        const auto preventPartiallyChecked = [](QCheckBox *checkbox) {
            if (checkbox->checkState() == Qt::PartiallyChecked)
                checkbox->setCheckState(Qt::Checked);
        };
        using Column = CandidateTreeItem::Column;
        const auto createConnections = [=](QCheckBox *checkbox, Column column) {
            connect(checkbox, &QCheckBox::stateChanged, [setCheckStateForAll, column](int state) {
                if (state != Qt::PartiallyChecked)
                    setCheckStateForAll(column, state);
            });
            connect(checkbox, &QCheckBox::clicked, this, [checkbox, preventPartiallyChecked] {
                preventPartiallyChecked(checkbox);
            });
        };
        std::array<QCheckBox *, 4> checkBoxes = {};

        static_assert(std::size(CheckBoxColumn) == checkBoxes.size(),
                      "Must contain the same number of elements");
        for (std::size_t i = 0; i < checkBoxes.size(); ++i) {
            if (Utils::anyOf(candidates, [i](const MemberInfo &mi) {
                    return mi.possibleFlags & CandidateTreeItem::ColumnFlag[CheckBoxColumn[i]];
                })) {
                const Column column = CheckBoxColumn[i];
                if (column == Column::GetterColumn)
                    checkBoxes[i] = new QCheckBox(Tr::tr("Create getters for all members"));
                else if (column == Column::SetterColumn)
                    checkBoxes[i] = new QCheckBox(Tr::tr("Create setters for all members"));
                else if (column == Column::SignalColumn)
                    checkBoxes[i] = new QCheckBox(Tr::tr("Create signals for all members"));
                else if (column == Column::QPropertyColumn)
                    checkBoxes[i] = new QCheckBox(Tr::tr("Create Q_PROPERTY for all members"));

                createConnections(checkBoxes[i], column);
            }
        }
        connect(model, &QAbstractItemModel::dataChanged, this, [this, checkBoxes] {
            const auto countExisting = [this](Flags flag) {
                return Utils::count(m_candidates, [flag](const MemberInfo &mi) {
                    return !(mi.possibleFlags & flag);
                });
            };
            const auto countRequested = [this](Flags flag) {
                return Utils::count(m_candidates, [flag](const MemberInfo &mi) {
                    return mi.requestedFlags & flag;
                });
            };
            const auto countToState = [this](int requestedCount, int alreadyExistsCount) {
                if (requestedCount == 0)
                    return Qt::Unchecked;
                if (int(m_candidates.size()) - requestedCount == alreadyExistsCount)
                    return Qt::Checked;
                return Qt::PartiallyChecked;
            };
            for (std::size_t i = 0; i < checkBoxes.size(); ++i) {
                if (checkBoxes[i]) {
                    const Flags flag = CandidateTreeItem::ColumnFlag[CheckBoxColumn[i]];
                    checkBoxes[i]->setCheckState(
                        countToState(countRequested(flag), countExisting(flag)));
                }
            }
        });

        const auto mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(new QLabel(Tr::tr("Select the getters and setters "
                                                "to be created.")));
        for (auto checkBox : checkBoxes) {
            if (checkBox)
                mainLayout->addWidget(checkBox);
        }
        mainLayout->addWidget(view);
        mainLayout->addWidget(buttonBox);
        int left, right;
        mainLayout->getContentsMargins(&left, nullptr, &right, nullptr);
        optimalWidth += left + right;
        resize(optimalWidth, mainLayout->sizeHint().height());
    }

    GetterSetterCandidates candidates() const { return m_candidates; }

private:
    GetterSetterCandidates m_candidates;
};

class GenerateGettersSettersOperation : public CppQuickFixOperation
{
public:
    GenerateGettersSettersOperation(const CppQuickFixInterface &interface)
        : CppQuickFixOperation(interface)
    {
        setDescription(Tr::tr("Create Getter and Setter Member Functions"));

        m_classAST = astForClassOperations(interface);
        if (!m_classAST)
            return;
        Class * const theClass = m_classAST->symbol;
        if (!theClass)
            return;

        // Go through all data members and try to find out whether they have getters and/or setters.
        QList<Symbol *> dataMembers;
        QList<Symbol *> memberFunctions;
        for (auto it = theClass->memberBegin(); it != theClass->memberEnd(); ++it) {
            Symbol *const s = *it;
            if (!s->identifier() || !s->type() || s->type().isTypedef())
                continue;
            if ((s->asDeclaration() && s->type()->asFunctionType()) || s->asFunction())
                memberFunctions << s;
            else if (s->asDeclaration() && (s->isPrivate() || s->isProtected()))
                dataMembers << s;
        }

        auto file = interface.currentFile();
        QStringList qPropertyNames; // name after MEMBER or name of the property
        for (auto it = m_classAST->member_specifier_list; it; it = it->next) {
            if (it->value->asQtPropertyDeclaration()) {
                auto propDecl = it->value->asQtPropertyDeclaration();
                // iterator over 'READ ...', ... and check if we have a MEMBER
                for (auto p = propDecl->property_declaration_item_list; p; p = p->next) {
                    const char *tokenString = file->tokenAt(p->value->item_name_token).spell();
                    if (!qstrcmp(tokenString, "MEMBER"))
                        qPropertyNames << file->textOf(p->value->expression);
                }
                // no MEMBER, but maybe the property name is the same
                qPropertyNames << file->textOf(propDecl->property_name);
            }
        }
        const QStringList memberFunctionsAsStrings = toStringList(memberFunctions);

        for (Symbol *const member : std::as_const(dataMembers)) {
            ExistingGetterSetterData existing;
            existing.memberVariableName = QString::fromUtf8(member->identifier()->chars(),
                                                            member->identifier()->size());
            existing.declarationSymbol = member->asDeclaration();
            existing.clazz = theClass;

            // check if a Q_PROPERTY exist
            const QString baseName = memberBaseName(existing.memberVariableName);
            if (qPropertyNames.contains(baseName)
                || qPropertyNames.contains(existing.memberVariableName))
                continue;

            findExistingFunctions(existing, memberFunctionsAsStrings);
            existing.qPropertyName = baseName;

            int possibleFlags = existing.computePossibleFlags();
            if (possibleFlags == 0)
                continue;
            m_candidates.emplace_back(existing, possibleFlags);
        }
    }

    GetterSetterCandidates candidates() const { return m_candidates; }
    bool isApplicable() const { return !m_candidates.empty(); }

    void setGetterSetterData(const GetterSetterCandidates &data)
    {
        m_candidates = data;
        m_hasData = true;
    }

private:
    void perform() override
    {
        if (!m_hasData) {
            GenerateGettersSettersDialog dlg(m_candidates);
            if (dlg.exec() == QDialog::Rejected)
                return;
            m_candidates = dlg.candidates();
        }
        if (m_candidates.empty())
            return;
        GetterSetterRefactoringHelper helper(this,
                                             currentFile()->filePath(),
                                             m_candidates.front().data.clazz);
        for (MemberInfo &mi : m_candidates) {
            if (mi.requestedFlags != 0) {
                helper.performGeneration(mi.data, mi.requestedFlags);
            }
        }
        helper.applyChanges();
    }

    GetterSetterCandidates m_candidates;
    const ClassSpecifierAST *m_classAST = nullptr;
    bool m_hasData = false;
};

void GenerateGettersSettersForClass::match(const CppQuickFixInterface &interface,
                                           QuickFixOperations &result)
{
    const auto op = QSharedPointer<GenerateGettersSettersOperation>::create(interface);
    if (!op->isApplicable())
        return;
    if (m_test) {
        GetterSetterCandidates candidates = op->candidates();
        for (MemberInfo &mi : candidates) {
            mi.requestedFlags = mi.possibleFlags;
            using Flag = GenerateGetterSetterOp::GenerateFlag;
            mi.requestedFlags &= ~Flag::GenerateConstantProperty;
        }
        op->setGetterSetterData(candidates);
    }
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
        CppRefactoringFilePtr currentFile = refactoring.file(filePath());

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
        Control *control = context().bindings()->control().data();
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
        funcDef.append(QLatin1String("\n{"));
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

        // Get starting indentation from original code.
        int indentedExtractionStart = m_extractionStart;
        QChar current = currentFile->document()->characterAt(indentedExtractionStart - 1);
        while (current == QLatin1Char(' ') || current == QLatin1Char('\t')) {
            --indentedExtractionStart;
            current = currentFile->document()->characterAt(indentedExtractionStart - 1);
        }
        QString extract = currentFile->textOf(indentedExtractionStart, m_extractionEnd);
        extract.replace(QChar::ParagraphSeparator, QLatin1String("\n"));
        if (!extract.endsWith(QLatin1Char('\n')) && m_funcReturn)
            extract.append(QLatin1Char('\n'));

        // Since we need an indent range and a nested reindent range (based on the original
        // formatting) it's simpler to have two different change sets.
        ChangeSet change;
        int position = currentFile->startOf(m_refFuncDef);

        // Do not insert right between the function and an associated comment.
        const QList<Token> functionDoc = commentsForDeclaration(
            m_refFuncDef->symbol, m_refFuncDef, *currentFile->document(),
            currentFile->cppDocument());
        if (!functionDoc.isEmpty()) {
            position = currentFile->cppDocument()->translationUnit()->getTokenPositionInDocument(
                functionDoc.first(), currentFile->document());
        }

        change.insert(position, funcDef);
        change.replace(m_extractionStart, m_extractionEnd, funcCall);
        currentFile->setChangeSet(change);
        currentFile->appendIndentRange(ChangeSet::Range(position, position + 1));
        currentFile->apply();

        QTextCursor tc = currentFile->document()->find(QLatin1String("{"), position);
        QTC_ASSERT(tc.hasSelection(), return);
        position = tc.selectionEnd() + 1;
        change.clear();
        change.insert(position, extract + '\n');
        currentFile->setChangeSet(change);
        currentFile->appendReindentRange(ChangeSet::Range(position, position + 1));
        currentFile->apply();

        // Write declaration, if necessary.
        if (matchingClass) {
            InsertionPointLocator locator(refactoring);
            const FilePath filePath = FilePath::fromUtf8(matchingClass->fileName());
            const InsertionLocation &location =
                    locator.methodDeclarationInClass(filePath, matchingClass, options.access);
            CppRefactoringFilePtr declFile = refactoring.file(filePath);
            change.clear();
            position = declFile->position(location.line(), location.column());
            change.insert(position, location.prefix() + funcDecl + location.suffix());
            declFile->setChangeSet(change);
            declFile->appendIndentRange(ChangeSet::Range(position, position + 1));
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

void ExtractFunction::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
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
                result.file = refactoring.file(declFilePath);
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
            result.file = refactoring.file(declFilePath);
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
        const CppRefactoringFilePtr currentFile = refactoring.file(filePath());
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

void ExtractLiteralAsParameter::match(const CppQuickFixInterface &interface,
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
        , m_file(m_refactoring.file(filePath()))
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

void ConvertFromAndToPointer::match(const CppQuickFixInterface &interface,
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

void extractNames(const CppRefactoringFilePtr &file,
                  QtPropertyDeclarationAST *qtPropertyDeclaration,
                  ExistingGetterSetterData &data)
{
    QtPropertyDeclarationItemListAST *it = qtPropertyDeclaration->property_declaration_item_list;
    for (; it; it = it->next) {
        const char *tokenString = file->tokenAt(it->value->item_name_token).spell();
        if (!qstrcmp(tokenString, "READ")) {
            data.getterName = file->textOf(it->value->expression);
        } else if (!qstrcmp(tokenString, "WRITE")) {
            data.setterName = file->textOf(it->value->expression);
        } else if (!qstrcmp(tokenString, "RESET")) {
            data.resetName = file->textOf(it->value->expression);
        } else if (!qstrcmp(tokenString, "NOTIFY")) {
            data.signalName = file->textOf(it->value->expression);
        } else if (!qstrcmp(tokenString, "MEMBER")) {
            data.memberVariableName = file->textOf(it->value->expression);
        }
    }
}

} // anonymous namespace

void InsertQtPropertyMembers::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    using Flag = GenerateGetterSetterOp::GenerateFlag;
    ExistingGetterSetterData existing;
    // check for Q_PROPERTY

    const QList<AST *> &path = interface.path();
    if (path.isEmpty())
        return;

    AST *const ast = path.last();
    QtPropertyDeclarationAST *qtPropertyDeclaration = ast->asQtPropertyDeclaration();
    if (!qtPropertyDeclaration || !qtPropertyDeclaration->type_id)
        return;

    ClassSpecifierAST *klass = nullptr;
    for (int i = path.size() - 2; i >= 0; --i) {
        klass = path.at(i)->asClassSpecifier();
        if (klass)
            break;
    }
    if (!klass)
        return;
    existing.clazz = klass->symbol;

    CppRefactoringFilePtr file = interface.currentFile();
    const QString propertyName = file->textOf(qtPropertyDeclaration->property_name);
    existing.qPropertyName = propertyName;
    extractNames(file, qtPropertyDeclaration, existing);

    Control *control = interface.currentFile()->cppDocument()->control();

    existing.declarationSymbol = control->newDeclaration(ast->firstToken(),
                                                         qtPropertyDeclaration->property_name->name);
    existing.declarationSymbol->setVisibility(Symbol::Private);
    existing.declarationSymbol->setEnclosingScope(existing.clazz);

    {
        // create a 'right' Type Object
        // if we have Q_PROPERTY(int test ...) then we only get a NamedType for 'int', but we want
        // a IntegerType. So create a new dummy file with a dummy declaration to get the right
        // object
        QByteArray type = file->textOf(qtPropertyDeclaration->type_id).toUtf8();
        QByteArray newSource = file->document()
                                   ->toPlainText()
                                   .insert(file->startOf(qtPropertyDeclaration),
                                           QString::fromUtf8(type + " __dummy;\n"))
                                   .toUtf8();

        Document::Ptr doc = interface.snapshot().preprocessedDocument(newSource, "___quickfix.h");
        if (!doc->parse(Document::ParseTranlationUnit))
            return;
        doc->check();
        class TypeFinder : public ASTVisitor
        {
        public:
            FullySpecifiedType type;
            TypeFinder(TranslationUnit *u)
                : ASTVisitor(u)
            {}
            bool visit(SimpleDeclarationAST *ast) override
            {
                if (ast->symbols && !ast->symbols->next) {
                    const Name *name = ast->symbols->value->name();
                    if (name && name->asNameId() && name->asNameId()->identifier()) {
                        const Identifier *id = name->asNameId()->identifier();
                        if (QString::fromUtf8(id->chars(), id->size()) == "__dummy")
                            type = ast->symbols->value->type();
                    }
                }
                return true;
            }
        };
        TypeFinder finder(doc->translationUnit());
        finder.accept(doc->translationUnit()->ast());
        if (finder.type.type()->isUndefinedType())
            return;
        existing.declarationSymbol->setType(finder.type);
        existing.doc = doc; // to hold type
    }
    // check which methods are already there
    const bool haveFixMemberVariableName = !existing.memberVariableName.isEmpty();
    int generateFlags = Flag::GenerateMemberVariable;
    if (!existing.resetName.isEmpty())
        generateFlags |= Flag::GenerateReset;
    if (!existing.setterName.isEmpty())
        generateFlags |= Flag::GenerateSetter;
    if (!existing.getterName.isEmpty())
        generateFlags |= Flag::GenerateGetter;
    if (!existing.signalName.isEmpty())
        generateFlags |= Flag::GenerateSignal;
    Overview overview;
    for (int i = 0; i < existing.clazz->memberCount(); ++i) {
        Symbol *member = existing.clazz->memberAt(i);
        FullySpecifiedType type = member->type();
        if (member->asFunction() || (type.isValid() && type->asFunctionType())) {
            const QString name = overview.prettyName(member->name());
            if (name == existing.getterName)
                generateFlags &= ~Flag::GenerateGetter;
            else if (name == existing.setterName)
                generateFlags &= ~Flag::GenerateSetter;
            else if (name == existing.resetName)
                generateFlags &= ~Flag::GenerateReset;
            else if (name == existing.signalName)
                generateFlags &= ~Flag::GenerateSignal;
        } else if (member->asDeclaration()) {
            const QString name = overview.prettyName(member->name());
            if (haveFixMemberVariableName) {
                if (name == existing.memberVariableName) {
                    generateFlags &= ~Flag::GenerateMemberVariable;
                }
            } else {
                const QString baseName = memberBaseName(name);
                if (existing.qPropertyName == baseName) {
                    existing.memberVariableName = name;
                    generateFlags &= ~Flag::GenerateMemberVariable;
                }
            }
        }
    }
    if (generateFlags & Flag::GenerateMemberVariable) {
        CppQuickFixSettings *settings = CppQuickFixProjectsSettings::getQuickFixSettings(
            ProjectExplorer::ProjectTree::currentProject());
        existing.memberVariableName = settings->getMemberVariableName(existing.qPropertyName);
    }
    if (generateFlags == 0) {
        // everything is already there
        return;
    }
    generateFlags |= Flag::HaveExistingQProperty;
    GenerateGetterSetterOp::generateQuickFixes(result, interface, existing, generateFlags);
}

namespace {

class ApplyDeclDefLinkOperation : public CppQuickFixOperation
{
public:
    explicit ApplyDeclDefLinkOperation(const CppQuickFixInterface &interface,
            const QSharedPointer<FunctionDeclDefLink> &link)
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
    QSharedPointer<FunctionDeclDefLink> m_link;
};

} // anonymous namespace

void ApplyDeclDefLinkChanges::match(const CppQuickFixInterface &interface,
                                    QuickFixOperations &result)
{
    QSharedPointer<FunctionDeclDefLink> link = interface.editor()->declDefLink();
    if (!link || !link->isMarkerVisible())
        return;

    auto op = new ApplyDeclDefLinkOperation(interface, link);
    op->setDescription(Tr::tr("Apply Function Signature Changes"));
    result << op;
}

namespace {

QString definitionSignature(const CppQuickFixInterface *assist,
                            FunctionDefinitionAST *functionDefinitionAST,
                            CppRefactoringFilePtr &baseFile,
                            CppRefactoringFilePtr &targetFile,
                            Scope *scope)
{
    QTC_ASSERT(assist, return QString());
    QTC_ASSERT(functionDefinitionAST, return QString());
    QTC_ASSERT(scope, return QString());
    Function *func = functionDefinitionAST->symbol;
    QTC_ASSERT(func, return QString());

    LookupContext cppContext(targetFile->cppDocument(), assist->snapshot());
    ClassOrNamespace *cppCoN = cppContext.lookupType(scope);
    if (!cppCoN)
        cppCoN = cppContext.globalNamespace();
    SubstitutionEnvironment env;
    env.setContext(assist->context());
    env.switchScope(func->enclosingScope());
    UseMinimalNames q(cppCoN);
    env.enter(&q);
    Control *control = assist->context().bindings()->control().data();
    Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
    oo.showFunctionSignatures = true;
    oo.showReturnTypes = true;
    oo.showArgumentNames = true;
    oo.showEnclosingTemplate = true;
    oo.showTemplateParameters = true;
    oo.trailingReturnType = functionDefinitionAST->declarator
            && functionDefinitionAST->declarator->postfix_declarator_list
            && functionDefinitionAST->declarator->postfix_declarator_list->value
            && functionDefinitionAST->declarator->postfix_declarator_list
                    ->value->asFunctionDeclarator()
            && functionDefinitionAST->declarator->postfix_declarator_list
                    ->value->asFunctionDeclarator()->trailing_return_type;
    const Name *name = func->name();
    if (name && nameIncludesOperatorName(name)) {
        CoreDeclaratorAST *coreDeclarator = functionDefinitionAST->declarator->core_declarator;
        const QString operatorNameText = baseFile->textOf(coreDeclarator);
        oo.includeWhiteSpaceInOperatorName = operatorNameText.contains(QLatin1Char(' '));
    }
    const QString nameText = oo.prettyName(LookupContext::minimalName(func, cppCoN, control));
    oo.showTemplateParameters = false;
    const FullySpecifiedType tn = rewriteType(func->type(), &env, control);

    return oo.prettyType(tn, nameText);
}

class MoveFuncDefRefactoringHelper
{
public:
    enum MoveType {
        MoveOutside,
        MoveToCppFile,
        MoveOutsideMemberToCppFile
    };

    MoveFuncDefRefactoringHelper(CppQuickFixOperation *operation, MoveType type,
                                 const FilePath &fromFile, const FilePath &toFile)
        : m_operation(operation), m_type(type), m_changes(m_operation->snapshot())
    {
        m_fromFile = m_changes.file(fromFile);
        m_toFile = (m_type == MoveOutside) ? m_fromFile : m_changes.file(toFile);
    }

    void performMove(FunctionDefinitionAST *funcAST)
    {
        // Determine file, insert position and scope
        InsertionLocation l = insertLocationForMethodDefinition(
                    funcAST->symbol, false, NamespaceHandling::Ignore,
                    m_changes, m_toFile->filePath());
        const QString prefix = l.prefix();
        const QString suffix = l.suffix();
        const int insertPos = m_toFile->position(l.line(), l.column());
        Scope *scopeAtInsertPos = m_toFile->cppDocument()->scopeAt(l.line(), l.column());

        // construct definition
        const QString funcDec = inlinePrefix(m_toFile->filePath(), [this] { return m_type == MoveOutside; })
                + definitionSignature(m_operation, funcAST, m_fromFile, m_toFile,
                                      scopeAtInsertPos);
        QString funcDef = prefix + funcDec;
        const int startPosition = m_fromFile->endOf(funcAST->declarator);
        const int endPosition = m_fromFile->endOf(funcAST);
        funcDef += m_fromFile->textOf(startPosition, endPosition);
        funcDef += suffix;

        // insert definition at new position
        m_toFileChangeSet.insert(insertPos, funcDef);
        m_toFile->appendIndentRange(ChangeSet::Range(insertPos, insertPos + funcDef.size()));
        m_toFile->setOpenEditor(true, insertPos);

        // remove definition from fromFile
        if (m_type == MoveOutsideMemberToCppFile) {
            m_fromFileChangeSet.remove(m_fromFile->range(funcAST));
        } else {
            QString textFuncDecl = m_fromFile->textOf(funcAST);
            textFuncDecl.truncate(startPosition - m_fromFile->startOf(funcAST));
            if (textFuncDecl.left(7) == QLatin1String("inline "))
                textFuncDecl = textFuncDecl.mid(7);
            else
                textFuncDecl.replace(" inline ", QLatin1String(" "));
            textFuncDecl = textFuncDecl.trimmed() + QLatin1Char(';');
            m_fromFileChangeSet.replace(m_fromFile->range(funcAST), textFuncDecl);
        }
    }

    void applyChanges()
    {
        if (!m_toFileChangeSet.isEmpty()) {
            m_toFile->setChangeSet(m_toFileChangeSet);
            m_toFile->apply();
        }
        if (!m_fromFileChangeSet.isEmpty()) {
            m_fromFile->setChangeSet(m_fromFileChangeSet);
            m_fromFile->apply();
        }
    }

private:
    CppQuickFixOperation *m_operation;
    MoveType m_type;
    CppRefactoringChanges m_changes;
    CppRefactoringFilePtr m_fromFile;
    CppRefactoringFilePtr m_toFile;
    ChangeSet m_fromFileChangeSet;
    ChangeSet m_toFileChangeSet;
};

class MoveFuncDefOutsideOp : public CppQuickFixOperation
{
public:
    MoveFuncDefOutsideOp(const CppQuickFixInterface &interface,
                         MoveFuncDefRefactoringHelper::MoveType type,
                         FunctionDefinitionAST *funcDef, const FilePath &cppFilePath)
        : CppQuickFixOperation(interface, 0)
        , m_funcDef(funcDef)
        , m_type(type)
        , m_cppFilePath(cppFilePath)
        , m_headerFilePath(funcDef->symbol->filePath())
    {
        if (m_type == MoveFuncDefRefactoringHelper::MoveOutside) {
            setDescription(Tr::tr("Move Definition Outside Class"));
        } else {
            const FilePath resolved = m_cppFilePath.relativePathFrom(m_headerFilePath.parentDir());
            setDescription(Tr::tr("Move Definition to %1").arg(resolved.displayName()));
        }
    }

    void perform() override
    {
        MoveFuncDefRefactoringHelper helper(this, m_type, m_headerFilePath, m_cppFilePath);
        helper.performMove(m_funcDef);
        helper.applyChanges();
    }

private:
    FunctionDefinitionAST *m_funcDef;
    MoveFuncDefRefactoringHelper::MoveType m_type;
    const FilePath m_cppFilePath;
    const FilePath m_headerFilePath;
};

} // anonymous namespace

void MoveFuncDefOutside::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();
    SimpleDeclarationAST *classAST = nullptr;
    FunctionDefinitionAST *funcAST = nullptr;
    bool moveOutsideMemberDefinition = false;

    const int pathSize = path.size();
    for (int idx = 1; idx < pathSize; ++idx) {
        if ((funcAST = path.at(idx)->asFunctionDefinition())) {
            // check cursor position
            if (idx != pathSize - 1  // Do not allow "void a() @ {..."
                    && funcAST->function_body
                    && !interface.isCursorOn(funcAST->function_body)) {
                if (path.at(idx - 1)->asTranslationUnit()) { // normal function
                    if (idx + 3 < pathSize && path.at(idx + 3)->asQualifiedName()) // Outside member
                        moveOutsideMemberDefinition = true;                        // definition
                    break;
                }

                if (idx > 1) {
                    if ((classAST = path.at(idx - 2)->asSimpleDeclaration())) // member function
                        break;
                    if (path.at(idx - 2)->asNamespace())  // normal function in namespace
                        break;
                }
                if (idx > 2 && path.at(idx - 1)->asTemplateDeclaration()) {
                    if ((classAST = path.at(idx - 3)->asSimpleDeclaration())) // member template
                        break;
                }
            }
            funcAST = nullptr;
        }
    }

    if (!funcAST || !funcAST->symbol)
        return;

    bool isHeaderFile = false;
    const FilePath cppFileName = correspondingHeaderOrSource(interface.filePath(), &isHeaderFile);

    if (isHeaderFile && !cppFileName.isEmpty()) {
        const MoveFuncDefRefactoringHelper::MoveType type = moveOutsideMemberDefinition
                ? MoveFuncDefRefactoringHelper::MoveOutsideMemberToCppFile
                : MoveFuncDefRefactoringHelper::MoveToCppFile;
        result << new MoveFuncDefOutsideOp(interface, type, funcAST, cppFileName);
    }

    if (classAST)
        result << new MoveFuncDefOutsideOp(interface, MoveFuncDefRefactoringHelper::MoveOutside,
                                           funcAST, FilePath());

    return;
}

namespace {

class MoveAllFuncDefOutsideOp : public CppQuickFixOperation
{
public:
    MoveAllFuncDefOutsideOp(const CppQuickFixInterface &interface,
                            MoveFuncDefRefactoringHelper::MoveType type,
                            ClassSpecifierAST *classDef, const FilePath &cppFileName)
        : CppQuickFixOperation(interface, 0)
        , m_type(type)
        , m_classDef(classDef)
        , m_cppFilePath(cppFileName)
        , m_headerFilePath(classDef->symbol->filePath())
    {
        if (m_type == MoveFuncDefRefactoringHelper::MoveOutside) {
            setDescription(Tr::tr("Definitions Outside Class"));
        } else {
            const FilePath resolved = m_cppFilePath.relativePathFrom(m_headerFilePath.parentDir());
            setDescription(Tr::tr("Move All Function Definitions to %1")
                           .arg(resolved.displayName()));
        }
    }

    void perform() override
    {
        MoveFuncDefRefactoringHelper helper(this, m_type, m_headerFilePath, m_cppFilePath);
        for (DeclarationListAST *it = m_classDef->member_specifier_list; it; it = it->next) {
            if (FunctionDefinitionAST *funcAST = it->value->asFunctionDefinition()) {
                if (funcAST->symbol && !funcAST->symbol->isGenerated())
                    helper.performMove(funcAST);
            }
        }
        helper.applyChanges();
    }

private:
    MoveFuncDefRefactoringHelper::MoveType m_type;
    ClassSpecifierAST *m_classDef;
    const FilePath m_cppFilePath;
    const FilePath m_headerFilePath;
};

} // anonymous namespace

void MoveAllFuncDefOutside::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    ClassSpecifierAST * const classAST = astForClassOperations(interface);
    if (!classAST)
        return;

    // Determine if the class has at least one function definition
    bool classContainsFunctions = false;
    for (DeclarationListAST *it = classAST->member_specifier_list; it; it = it->next) {
        if (FunctionDefinitionAST *funcAST = it->value->asFunctionDefinition()) {
            if (funcAST->symbol && !funcAST->symbol->isGenerated()) {
                classContainsFunctions = true;
                break;
            }
        }
    }
    if (!classContainsFunctions)
        return;

    bool isHeaderFile = false;
    const FilePath cppFileName = correspondingHeaderOrSource(interface.filePath(), &isHeaderFile);
    if (isHeaderFile && !cppFileName.isEmpty()) {
        result << new MoveAllFuncDefOutsideOp(interface,
                                              MoveFuncDefRefactoringHelper::MoveToCppFile,
                                              classAST, cppFileName);
    }
    result << new MoveAllFuncDefOutsideOp(interface, MoveFuncDefRefactoringHelper::MoveOutside,
                                          classAST, FilePath());
}

namespace {

class MoveFuncDefToDeclOp : public CppQuickFixOperation
{
public:
    MoveFuncDefToDeclOp(const CppQuickFixInterface &interface,
                        const FilePath &fromFilePath, const FilePath &toFilePath,
                        FunctionDefinitionAST *funcDef, const QString &declText,
                        const ChangeSet::Range &fromRange,
                        const ChangeSet::Range &toRange)
        : CppQuickFixOperation(interface, 0)
        , m_fromFilePath(fromFilePath)
        , m_toFilePath(toFilePath)
        , m_funcAST(funcDef)
        , m_declarationText(declText)
        , m_fromRange(fromRange)
        , m_toRange(toRange)
    {
        if (m_toFilePath == m_fromFilePath) {
            setDescription(Tr::tr("Move Definition to Class"));
        } else {
            const FilePath resolved = m_toFilePath.relativePathFrom(m_fromFilePath.parentDir());
            setDescription(Tr::tr("Move Definition to %1").arg(resolved.displayName()));
        }
    }

    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr fromFile = refactoring.file(m_fromFilePath);
        CppRefactoringFilePtr toFile = refactoring.file(m_toFilePath);

        const QString wholeFunctionText = m_declarationText
                + fromFile->textOf(fromFile->endOf(m_funcAST->declarator),
                                   fromFile->endOf(m_funcAST->function_body));

        // Replace declaration with function and delete old definition
        ChangeSet toTarget;
        toTarget.replace(m_toRange, wholeFunctionText);
        if (m_toFilePath == m_fromFilePath)
            toTarget.remove(m_fromRange);
        toFile->setChangeSet(toTarget);
        toFile->appendIndentRange(m_toRange);
        toFile->setOpenEditor(true, m_toRange.start);
        toFile->apply();
        if (m_toFilePath != m_fromFilePath) {
            ChangeSet fromTarget;
            fromTarget.remove(m_fromRange);
            fromFile->setChangeSet(fromTarget);
            fromFile->apply();
        }
    }

private:
    const FilePath m_fromFilePath;
    const FilePath m_toFilePath;
    FunctionDefinitionAST *m_funcAST;
    const QString m_declarationText;
    const ChangeSet::Range m_fromRange;
    const ChangeSet::Range m_toRange;
};

} // anonymous namespace

void MoveFuncDefToDecl::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();
    AST *completeDefAST = nullptr;
    FunctionDefinitionAST *funcAST = nullptr;

    const int pathSize = path.size();
    for (int idx = 1; idx < pathSize; ++idx) {
        if ((funcAST = path.at(idx)->asFunctionDefinition())) {
            AST *enclosingAST = path.at(idx - 1);
            if (enclosingAST->asClassSpecifier())
                return;

            // check cursor position
            if (idx != pathSize - 1  // Do not allow "void a() @ {..."
                    && funcAST->function_body
                    && !interface.isCursorOn(funcAST->function_body)) {
                completeDefAST = enclosingAST->asTemplateDeclaration() ? enclosingAST : funcAST;
                break;
            }
            funcAST = nullptr;
        }
    }

    if (!funcAST || !funcAST->symbol)
        return;

    const CppRefactoringChanges refactoring(interface.snapshot());
    const CppRefactoringFilePtr defFile = refactoring.file(interface.filePath());
    const ChangeSet::Range defRange = defFile->range(completeDefAST);

    // Determine declaration (file, range, text);
    ChangeSet::Range declRange;
    QString declText;
    FilePath declFilePath;

    Function *func = funcAST->symbol;
    if (Class *matchingClass = isMemberFunction(interface.context(), func)) {
        // Dealing with member functions
        const QualifiedNameId *qName = func->name()->asQualifiedNameId();
        for (Symbol *symbol = matchingClass->find(qName->identifier());
             symbol; symbol = symbol->next()) {
            Symbol *s = symbol;
            if (func->enclosingScope()->asTemplate()) {
                if (const Template *templ = s->type()->asTemplateType()) {
                    if (Symbol *decl = templ->declaration()) {
                        if (decl->type()->asFunctionType())
                                s = decl;
                    }
                }
            }
            if (!s->name()
                    || !qName->identifier()->match(s->identifier())
                    || !s->type()->asFunctionType()
                    || !s->type().match(func->type())
                    || s->asFunction()) {
                continue;
            }

            declFilePath = matchingClass->filePath();
            const CppRefactoringFilePtr declFile = refactoring.file(declFilePath);
            ASTPath astPath(declFile->cppDocument());
            const QList<AST *> path = astPath(s->line(), s->column());
            for (int idx = path.size() - 1; idx > 0; --idx) {
                AST *node = path.at(idx);
                if (SimpleDeclarationAST *simpleDecl = node->asSimpleDeclaration()) {
                    if (simpleDecl->symbols && !simpleDecl->symbols->next) {
                        declRange = declFile->range(simpleDecl);
                        declText = declFile->textOf(simpleDecl);
                        declText.remove(-1, 1); // remove ';' from declaration text
                        break;
                    }
                }
            }

            if (!declText.isEmpty())
                break;
        }
    } else if (Namespace *matchingNamespace = isNamespaceFunction(interface.context(), func)) {
        // Dealing with free functions
        bool isHeaderFile = false;
        declFilePath = correspondingHeaderOrSource(interface.filePath(), &isHeaderFile);
        if (isHeaderFile)
            return;

        const CppRefactoringFilePtr declFile = refactoring.file(declFilePath);
        const LookupContext lc(declFile->cppDocument(), interface.snapshot());
        const QList<LookupItem> candidates = lc.lookup(func->name(), matchingNamespace);
        for (const LookupItem &candidate : candidates) {
            if (Symbol *s = candidate.declaration()) {
                if (s->asDeclaration()) {
                    ASTPath astPath(declFile->cppDocument());
                    const QList<AST *> path = astPath(s->line(), s->column());
                    for (AST *node : path) {
                        if (SimpleDeclarationAST *simpleDecl = node->asSimpleDeclaration()) {
                            declRange = declFile->range(simpleDecl);
                            declText = declFile->textOf(simpleDecl);
                            declText.remove(-1, 1); // remove ';' from declaration text
                            break;
                        }
                    }
                }
            }

            if (!declText.isEmpty()) {
                declText.prepend(inlinePrefix(declFilePath));
                break;
            }
        }
    }

    if (!declFilePath.isEmpty() && !declText.isEmpty())
        result << new MoveFuncDefToDeclOp(interface,
                                          interface.filePath(),
                                          declFilePath,
                                          funcAST, declText,
                                          defRange, declRange);
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
        , m_file(CppRefactoringChanges(snapshot()).file(filePath()))
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

        Control *control = context().bindings()->control().data();
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

void AssignToLocalVariable::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
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
        const CppRefactoringFilePtr file = refactoring.file(filePath);
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

void OptimizeForLoop::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
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
        CppRefactoringFilePtr currentFile = refactoring.file(filePath());

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

void EscapeStringLiteral::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
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


namespace {

class ConvertQt4ConnectOperation: public CppQuickFixOperation
{
public:
    ConvertQt4ConnectOperation(const CppQuickFixInterface &interface, const ChangeSet &changes)
        : CppQuickFixOperation(interface, 1), m_changes(changes)
    {
        setDescription(Tr::tr("Convert connect() to Qt 5 Style"));
    }

private:
    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(filePath());
        currentFile->setChangeSet(m_changes);
        currentFile->apply();
    }

    const ChangeSet m_changes;
};

Symbol *skipForwardDeclarations(const QList<Symbol *> &symbols)
{
    for (Symbol *symbol : symbols) {
        if (!symbol->type()->asForwardClassDeclarationType())
            return symbol;
    }

    return nullptr;
}

bool findRawAccessFunction(Class *klass, PointerType *pointerType, QString *objAccessFunction)
{
    QList<Function *> candidates;
    for (auto it = klass->memberBegin(), end = klass->memberEnd(); it != end; ++it) {
        if (Function *func = (*it)->asFunction()) {
            const Name *funcName = func->name();
            if (!funcName->asOperatorNameId()
                    && !funcName->asConversionNameId()
                    && func->returnType().type() == pointerType
                    && func->isConst()
                    && func->argumentCount() == 0) {
                candidates << func;
            }
        }
    }
    const Name *funcName = nullptr;
    switch (candidates.size()) {
    case 0:
        return false;
    case 1:
        funcName = candidates.first()->name();
        break;
    default:
        // Multiple candidates - prefer a function named data
        for (Function *func : std::as_const(candidates)) {
            if (!strcmp(func->name()->identifier()->chars(), "data")) {
                funcName = func->name();
                break;
            }
        }
        if (!funcName)
            funcName = candidates.first()->name();
    }
    const Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
    *objAccessFunction = QLatin1Char('.') + oo.prettyName(funcName) + QLatin1String("()");
    return true;
}

PointerType *determineConvertedType(NamedType *namedType, const LookupContext &context,
                                    Scope *scope, QString *objAccessFunction)
{
    if (!namedType)
        return nullptr;
    if (ClassOrNamespace *binding = context.lookupType(namedType->name(), scope)) {
        if (Symbol *objectClassSymbol = skipForwardDeclarations(binding->symbols())) {
            if (Class *klass = objectClassSymbol->asClass()) {
                for (auto it = klass->memberBegin(), end = klass->memberEnd(); it != end; ++it) {
                    if (Function *func = (*it)->asFunction()) {
                        if (const ConversionNameId *conversionName =
                                func->name()->asConversionNameId()) {
                            if (PointerType *type = conversionName->type()->asPointerType()) {
                                if (findRawAccessFunction(klass, type, objAccessFunction))
                                    return type;
                            }
                        }
                    }
                }
            }
        }
    }

    return nullptr;
}

Class *senderOrReceiverClass(const CppQuickFixInterface &interface,
                             const CppRefactoringFilePtr &file,
                             const ExpressionAST *objectPointerAST,
                             Scope *objectPointerScope,
                             QString *objAccessFunction)
{
    const LookupContext &context = interface.context();

    QByteArray objectPointerExpression;
    if (objectPointerAST)
        objectPointerExpression = file->textOf(objectPointerAST).toUtf8();
    else
        objectPointerExpression = "this";

    TypeOfExpression toe;
    toe.setExpandTemplates(true);
    toe.init(interface.semanticInfo().doc, interface.snapshot(), context.bindings());
    const QList<LookupItem> objectPointerExpressions = toe(objectPointerExpression,
                                                           objectPointerScope, TypeOfExpression::Preprocess);
    QTC_ASSERT(!objectPointerExpressions.isEmpty(), return nullptr);

    Type *objectPointerTypeBase = objectPointerExpressions.first().type().type();
    QTC_ASSERT(objectPointerTypeBase, return nullptr);

    PointerType *objectPointerType = objectPointerTypeBase->asPointerType();
    if (!objectPointerType) {
        objectPointerType = determineConvertedType(objectPointerTypeBase->asNamedType(), context,
                                               objectPointerScope, objAccessFunction);
    }
    QTC_ASSERT(objectPointerType, return nullptr);

    Type *objectTypeBase = objectPointerType->elementType().type(); // Dereference
    QTC_ASSERT(objectTypeBase, return nullptr);

    NamedType *objectType = objectTypeBase->asNamedType();
    QTC_ASSERT(objectType, return nullptr);

    ClassOrNamespace *objectClassCON = context.lookupType(objectType->name(), objectPointerScope);
    if (!objectClassCON) {
        objectClassCON = objectPointerExpressions.first().binding();
        QTC_ASSERT(objectClassCON, return nullptr);
    }
    QTC_ASSERT(!objectClassCON->symbols().isEmpty(), return nullptr);

    Symbol *objectClassSymbol = skipForwardDeclarations(objectClassCON->symbols());
    QTC_ASSERT(objectClassSymbol, return nullptr);

    return objectClassSymbol->asClass();
}

bool findConnectReplacement(const CppQuickFixInterface &interface,
                            const ExpressionAST *objectPointerAST,
                            const QtMethodAST *methodAST,
                            const CppRefactoringFilePtr &file,
                            QString *replacement,
                            QString *objAccessFunction)
{
    // Get name of method
    if (!methodAST->declarator || !methodAST->declarator->core_declarator)
        return false;

    DeclaratorIdAST *methodDeclIdAST = methodAST->declarator->core_declarator->asDeclaratorId();
    if (!methodDeclIdAST)
        return false;

    NameAST *methodNameAST = methodDeclIdAST->name;
    if (!methodNameAST)
        return false;

    // Lookup object pointer type
    Scope *scope = file->scopeAt(methodAST->firstToken());
    Class *objectClass = senderOrReceiverClass(interface, file, objectPointerAST, scope,
                                               objAccessFunction);
    QTC_ASSERT(objectClass, return false);

    // Look up member function in call, including base class members.
    const LookupContext &context = interface.context();
    const QList<LookupItem> methodResults = context.lookup(methodNameAST->name, objectClass);
    if (methodResults.isEmpty())
        return false; // Maybe mis-spelled signal/slot name

    Scope *baseClassScope = methodResults.at(0).scope(); // FIXME: Handle overloads
    QTC_ASSERT(baseClassScope, return false);

    Class *classOfMethod = baseClassScope->asClass(); // Declaration point of signal/slot
    QTC_ASSERT(classOfMethod, return false);

    Symbol *method = methodResults.at(0).declaration();
    QTC_ASSERT(method, return false);

    // Minimize qualification
    Control *control = context.bindings()->control().data();
    ClassOrNamespace *functionCON = context.lookupParent(scope);
    const Name *shortName = LookupContext::minimalName(method, functionCON, control);
    if (!shortName->asQualifiedNameId())
        shortName = control->qualifiedNameId(classOfMethod->name(), shortName);

    const Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
    *replacement = QLatin1Char('&') + oo.prettyName(shortName);
    return true;
}

bool onConnectOrDisconnectCall(AST *ast, const ExpressionListAST **arguments)
{
    if (!ast)
        return false;

    CallAST *call = ast->asCall();
    if (!call)
        return false;

    if (!call->base_expression)
        return false;

    const IdExpressionAST *idExpr = call->base_expression->asIdExpression();
    if (!idExpr || !idExpr->name || !idExpr->name->name)
        return false;

    const ExpressionListAST *args = call->expression_list;
    if (!arguments)
        return false;

    const Identifier *id = idExpr->name->name->identifier();
    if (!id)
        return false;

    const QByteArray name(id->chars(), id->size());
    if (name != "connect" && name != "disconnect")
        return false;

    if (arguments)
        *arguments = args;
    return true;
}

// Might modify arg* output arguments even if false is returned.
bool collectConnectArguments(const ExpressionListAST *arguments,
                             const ExpressionAST **arg1, const QtMethodAST **arg2,
                             const ExpressionAST **arg3, const QtMethodAST **arg4)
{
    if (!arguments || !arg1 || !arg2 || !arg3 || !arg4)
        return false;

    *arg1 = arguments->value;
    arguments = arguments->next;
    if (!arg1 || !arguments)
        return false;

    *arg2 = arguments->value->asQtMethod();
    arguments = arguments->next;
    if (!*arg2 || !arguments)
        return false;

    *arg3 = arguments->value;
    if (!*arg3)
        return false;

    // Take care of three-arg version, with 'this' receiver.
    if (QtMethodAST *receiverMethod = arguments->value->asQtMethod()) {
        *arg3 = nullptr; // Means 'this'
        *arg4 = receiverMethod;
        return true;
    }

    arguments = arguments->next;
    if (!arguments)
        return false;

    *arg4 = arguments->value->asQtMethod();
    if (!*arg4)
        return false;

    return true;
}

} // anonynomous namespace

void ConvertQt4Connect::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();

    for (int i = path.size(); --i >= 0; ) {
        const ExpressionListAST *arguments;
        if (!onConnectOrDisconnectCall(path.at(i), &arguments))
            continue;

        const ExpressionAST *arg1, *arg3;
        const QtMethodAST *arg2, *arg4;
        if (!collectConnectArguments(arguments, &arg1, &arg2, &arg3, &arg4))
            continue;

        const CppRefactoringFilePtr file = interface.currentFile();

        QString newSignal;
        QString senderAccessFunc;
        if (!findConnectReplacement(interface, arg1, arg2, file, &newSignal, &senderAccessFunc))
            continue;

        QString newMethod;
        QString receiverAccessFunc;
        if (!findConnectReplacement(interface, arg3, arg4, file, &newMethod, &receiverAccessFunc))
            continue;

        ChangeSet changes;
        changes.replace(file->endOf(arg1), file->endOf(arg1), senderAccessFunc);
        changes.replace(file->startOf(arg2), file->endOf(arg2), newSignal);
        if (!arg3)
            newMethod.prepend(QLatin1String("this, "));
        else
            changes.replace(file->endOf(arg3), file->endOf(arg3), receiverAccessFunc);
        changes.replace(file->startOf(arg4), file->endOf(arg4), newMethod);

        result << new ConvertQt4ConnectOperation(interface, changes);
        return;
    }
}

void ExtraRefactoringOperations::match(const CppQuickFixInterface &interface,
                                       QuickFixOperations &result)
{
    const auto processor = CppModelManager::cppEditorDocumentProcessor(interface.filePath());
    if (processor) {
        const auto clangFixItOperations = processor->extraRefactoringOperations(interface);
        result.append(clangFixItOperations);
    }
}

namespace {

/**
 * @brief The NameCounter class counts the parts of a name. E.g. 2 for std::vector or 1 for variant
 */
class NameCounter : private NameVisitor
{
public:
    int count(const Name *name)
    {
        counter = 0;
        accept(name);
        return counter;
    }

private:
    void visit(const Identifier *) override { ++counter; }
    void visit(const DestructorNameId *) override { ++counter; }
    void visit(const TemplateNameId *) override { ++counter; }
    void visit(const QualifiedNameId *name) override
    {
        if (name->base())
            accept(name->base());
        accept(name->name());
    }
    int counter;
};

/**
 * @brief getBaseName returns the base name of a qualified name or nullptr.
 * E.g.: foo::bar => foo; bar => bar
 * @param name The Name, maybe qualified
 * @return The base name of the qualified name or nullptr
 */
const Identifier *getBaseName(const Name *name)
{
    class GetBaseName : public NameVisitor
    {
        void visit(const Identifier *name) override { baseName = name; }
        void visit(const QualifiedNameId *name) override
        {
            if (name->base())
                accept(name->base());
            else
                accept(name->name());
        }

    public:
        const Identifier *baseName = nullptr;
    };
    GetBaseName getter;
    getter.accept(name);
    return getter.baseName;
}

/**
 * @brief countNames counts the parts of the Name.
 * E.g. if the name is std::vector, the function returns 2, if the name is variant, returns 1
 * @param name The name that should be counted
 * @return the number of parts of the name
 */
int countNames(const Name *name)
{
    return NameCounter{}.count(name);
}

/**
 * @brief removeLine removes the whole line in which the ast node is located if there are otherwise only whitespaces
 * @param file The file in which the AST node is located
 * @param ast The ast node
 * @param changeSet The ChangeSet of the file
 */
void removeLine(const CppRefactoringFile *file, AST *ast, ChangeSet &changeSet)
{
    RefactoringFile::Range range = file->range(ast);
    --range.start;
    while (range.start >= 0) {
        QChar current = file->charAt(range.start);
        if (!current.isSpace()) {
            ++range.start;
            break;
        }
        if (current == QChar::ParagraphSeparator)
            break;
        --range.start;
    }
    range.start = std::max(0, range.start);
    while (range.end < file->document()->characterCount()) {
        QChar current = file->charAt(range.end);
        if (!current.isSpace())
            break;
        if (current == QChar::ParagraphSeparator)
            break;
        ++range.end;
    }
    range.end = std::min(file->document()->characterCount(), range.end);
    const bool newLineStart = file->charAt(range.start) == QChar::ParagraphSeparator;
    const bool newLineEnd = file->charAt(range.end) == QChar::ParagraphSeparator;
    if (!newLineEnd && newLineStart)
        ++range.start;
    changeSet.remove(range);
}

/**
 * @brief The RemoveNamespaceVisitor class removes a using namespace and rewrites all types that
 * are in the namespace if needed
 */
class RemoveNamespaceVisitor : public ASTVisitor
{
public:
    constexpr static int SearchGlobalUsingDirectivePos = std::numeric_limits<int>::max();
    RemoveNamespaceVisitor(const CppRefactoringFile *file,
                           const Snapshot &snapshot,
                           const Name *namespace_,
                           int symbolPos,
                           bool removeAllAtGlobalScope)
        : ASTVisitor(file->cppDocument()->translationUnit())
        , m_file(file)
        , m_snapshot(snapshot)
        , m_namespace(namespace_)
        , m_missingNamespace(toString(namespace_) + "::")
        , m_context(m_file->cppDocument(), m_snapshot)
        , m_symbolPos(symbolPos)
        , m_removeAllAtGlobalScope(removeAllAtGlobalScope)

    {}

    const ChangeSet &getChanges() { return m_changeSet; }

    /**
     * @brief isGlobalUsingNamespace return true if the using namespace that should be removed
     * is not scoped and other files that include this file will also use the using namespace
     * @return true if using namespace statement is global and not scoped, false otherwise
     */
    bool isGlobalUsingNamespace() const { return m_parentNode == nullptr; }

    /**
     * @brief foundGlobalUsingNamespace return true if removeAllAtGlobalScope is false and
     * another using namespace is found at the global scope, so that other files that include this
     * file don't have to be processed
     * @return true if there was a 'global' second using namespace in this file and
     * removeAllAtGlobalScope is false
     */
    bool foundGlobalUsingNamespace() const { return m_foundNamespace; }

private:
    bool preVisit(AST *ast) override
    {
        if (!m_start) {
            if (ast->asTranslationUnit())
                return true;
            if (UsingDirectiveAST *usingDirective = ast->asUsingDirective()) {
                if (nameEqual(usingDirective->name->name, m_namespace)) {
                    if (m_symbolPos == SearchGlobalUsingDirectivePos) {
                        // we have found a global using directive, so lets start
                        m_start = true;
                        removeLine(m_file, ast, m_changeSet);
                        return false;
                    }
                    // ignore the using namespace that should be removed
                    if (m_file->endOf(ast) != m_symbolPos) {
                        if (m_removeAllAtGlobalScope)
                            removeLine(m_file, ast, m_changeSet);
                        else
                            m_done = true;
                    }
                }
            }
            // if the end of the ast is before we should start, we are not interested in the node
            if (m_file->endOf(ast) <= m_symbolPos)
                return false;

            if (m_file->startOf(ast) > m_symbolPos)
                m_start = true;
        }
        return !m_foundNamespace && !m_done;
    }

    bool visit(NamespaceAST *ast) override
    {
        if (m_start && nameEqual(m_namespace, ast->symbol->name()))
            return false;

        return m_start;
    }

    // scopes for using namespace statements:
    bool visit(LinkageBodyAST *ast) override { return visitNamespaceScope(ast); }
    bool visit(CompoundStatementAST *ast) override { return visitNamespaceScope(ast); }
    bool visitNamespaceScope(AST *ast)
    {
        ++m_namespaceScopeCounter;
        if (!m_start)
            m_parentNode = ast;
        return true;
    }

    void endVisit(LinkageBodyAST *ast) override { endVisitNamespaceScope(ast); }
    void endVisit(CompoundStatementAST *ast) override { endVisitNamespaceScope(ast); }
    void endVisitNamespaceScope(AST *ast)
    {
        --m_namespaceScopeCounter;
        m_foundNamespace = false;
        // if we exit the scope of the using namespace we are done
        if (ast == m_parentNode)
            m_done = true;
    }

    bool visit(UsingDirectiveAST *ast) override
    {
        if (nameEqual(ast->name->name, m_namespace)) {
            if (m_removeAllAtGlobalScope && m_namespaceScopeCounter == 0)
                removeLine(m_file, ast, m_changeSet);
            else
                m_foundNamespace = true;
            return false;
        }
        return handleAstWithLongestName(ast);
    }

    bool visit(DeclaratorIdAST *ast) override
    {
        // e.g. we have the following code and get the following Lookup items:
        // namespace test {
        //   struct foo { // 1. item with test::foo
        //     foo();     // 2. item with test::foo::foo
        //   };
        // }
        // using namespace foo;
        // foo::foo() { ... } // 3. item with foo::foo
        // Our current name is foo::foo so we have to match with the 2. item / longest name
        return handleAstWithLongestName(ast);
    }

    template<typename AST>
    bool handleAstWithLongestName(AST *ast)
    {
        if (m_start) {
            Scope *scope = m_file->scopeAt(ast->firstToken());
            const QList<LookupItem> localLookup = m_context.lookup(ast->name->name, scope);
            QList<const Name *> longestName;
            for (const LookupItem &item : localLookup) {
                QList<const Name *> names
                    = m_context.fullyQualifiedName(item.declaration(),
                                                   LookupContext::HideInlineNamespaces);
                if (names.length() > longestName.length())
                    longestName = names;
            }
            const int currentNameCount = countNames(ast->name->name);
            const bool needNew = needMissingNamespaces(std::move(longestName), currentNameCount);
            if (needNew)
                insertMissingNamespace(ast);
        }
        return false;
    }

    bool visit(NamedTypeSpecifierAST *ast) override { return handleAstWithName(ast); }

    bool visit(IdExpressionAST *ast) override { return handleAstWithName(ast); }

    template<typename AST>
    bool handleAstWithName(AST *ast)
    {
        if (m_start) {
            Scope *scope = m_file->scopeAt(ast->firstToken());
            const Name *wantToLookup = ast->name->name;
            // first check if the base name is a typedef. Consider the following example:
            // using namespace std;
            // using vec = std::vector<int>;
            // vec::iterator it; // we have to lookup 'vec' and not iterator (would result in
            //   std::vector<int>::iterator => std::vec::iterator, which is wrong)
            const Name *baseName = getBaseName(wantToLookup);
            QList<LookupItem> typedefCandidates = m_context.lookup(baseName, scope);
            if (!typedefCandidates.isEmpty()) {
                if (typedefCandidates.front().declaration()->isTypedef())
                    wantToLookup = baseName;
            }

            const QList<LookupItem> lookups = m_context.lookup(wantToLookup, scope);
            if (!lookups.empty()) {
                QList<const Name *> fullName
                    = m_context.fullyQualifiedName(lookups.first().declaration(),
                                                   LookupContext::HideInlineNamespaces);
                const int currentNameCount = countNames(wantToLookup);
                const bool needNamespace = needMissingNamespaces(std::move(fullName),
                                                                 currentNameCount);
                if (needNamespace)
                    insertMissingNamespace(ast);
            }
        }
        return true;
    }

    template<typename AST>
    void insertMissingNamespace(AST *ast)
    {
        DestructorNameAST *destructorName = ast->name->asDestructorName();
        if (destructorName)
            m_changeSet.insert(m_file->startOf(destructorName->unqualified_name), m_missingNamespace);
        else
            m_changeSet.insert(m_file->startOf(ast->name), m_missingNamespace);
    }

    bool needMissingNamespaces(QList<const Name *> &&fullName, int currentNameCount)
    {
        if (currentNameCount > fullName.length())
            return false;

        // eg. fullName = std::vector, currentName = vector => result should be std
        fullName.erase(fullName.end() - currentNameCount, fullName.end());
        if (fullName.empty())
            return false;
        return nameEqual(m_namespace, fullName.last());
    }

    static bool nameEqual(const Name *name1, const Name *name2)
    {
        return Matcher::match(name1, name2);
    }

    QString toString(const Name *id)
    {
        const Identifier *identifier = id->asNameId();
        QTC_ASSERT(identifier, return {});
        return QString::fromUtf8(identifier->chars(), identifier->size());
    }

    const CppRefactoringFile *const m_file;
    const Snapshot &m_snapshot;

    const Name *m_namespace;          // the name of the namespace that should be removed
    const QString m_missingNamespace; // that should be added if a type was using the namespace
    LookupContext m_context;
    ChangeSet m_changeSet;
    const int m_symbolPos; // the end position of the start symbol
    bool m_done = false;
    bool m_start = false;
    // true if a using namespace was found at a scope and the scope should be left
    bool m_foundNamespace = false;
    bool m_removeAllAtGlobalScope;
    // the scope where the using namespace that should be removed is valid
    AST *m_parentNode = nullptr;
    int m_namespaceScopeCounter = 0;
};

class RemoveUsingNamespaceOperation : public CppQuickFixOperation
{
    struct Node
    {
        Document::Ptr document;
        bool hasGlobalUsingDirective = false;
        int unprocessedParents;
        std::vector<std::reference_wrapper<Node>> includes;
        std::vector<std::reference_wrapper<Node>> includedBy;
        Node() = default;
        Node(const Node &) = delete;
        Node(Node &&) = delete;
    };

public:
    RemoveUsingNamespaceOperation(const CppQuickFixInterface &interface,
                                  UsingDirectiveAST *usingDirective,
                                  bool removeAllAtGlobalScope)
        : CppQuickFixOperation(interface, 1)
        , m_usingDirective(usingDirective)
        , m_removeAllAtGlobalScope(removeAllAtGlobalScope)
    {
        const QString name = Overview{}.prettyName(usingDirective->name->name);
        if (m_removeAllAtGlobalScope) {
            setDescription(Tr::tr(
                               "Remove All Occurrences of \"using namespace %1\" in Global Scope "
                               "and Adjust Type Names Accordingly")
                               .arg(name));
        } else {
            setDescription(Tr::tr("Remove \"using namespace %1\" and "
                                  "Adjust Type Names Accordingly")
                               .arg(name));
        }
    }

private:
    std::map<Utils::FilePath, Node> buildIncludeGraph(CppRefactoringChanges &refactoring)
    {
        using namespace ProjectExplorer;
        using namespace Utils;

        const Snapshot &s = refactoring.snapshot();
        std::map<Utils::FilePath, Node> includeGraph;

        auto handleFile = [&](const FilePath &filePath, Document::Ptr doc, auto shouldHandle) {
            Node &node = includeGraph[filePath];
            node.document = doc;
            for (const Document::Include &include : doc->resolvedIncludes()) {
                const FilePath filePath = include.resolvedFileName();
                if (shouldHandle(filePath)) {
                    Node &includedNode = includeGraph[filePath];
                    includedNode.includedBy.push_back(node);
                    node.includes.push_back(includedNode);
                }
            }
        };

        if (const Project *project = ProjectManager::projectForFile(filePath())) {
            const FilePaths files = project->files(ProjectExplorer::Project::SourceFiles);
            QSet<FilePath> projectFiles(files.begin(), files.end());
            for (const auto &file : files) {
                const Document::Ptr doc = s.document(file);
                if (!doc)
                    continue;
                handleFile(file, doc, [&](const FilePath &file) {
                    return projectFiles.contains(file);
                });
            }
        } else {
            for (auto i = s.begin(); i != s.end(); ++i) {
                if (ProjectFile::classify(i.key().toString()) != ProjectFile::Unsupported) {
                    handleFile(i.key(), i.value(), [](const FilePath &file) {
                        return ProjectFile::classify(file.toString()) != ProjectFile::Unsupported;
                    });
                }
            }
        }
        for (auto &[_, node] : includeGraph) {
            Q_UNUSED(_)
            node.unprocessedParents = static_cast<int>(node.includes.size());
        }
        return includeGraph;
    }

    void removeAllUsingsAtGlobalScope(CppRefactoringChanges &refactoring)
    {
        auto includeGraph = buildIncludeGraph(refactoring);
        std::vector<std::reference_wrapper<Node>> nodesWithProcessedParents;
        for (auto &[_, node] : includeGraph) {
            Q_UNUSED(_)
            if (!node.unprocessedParents)
                nodesWithProcessedParents.push_back(node);
        }
        while (!nodesWithProcessedParents.empty()) {
            Node &node = nodesWithProcessedParents.back();
            nodesWithProcessedParents.pop_back();
            CppRefactoringFilePtr file = refactoring.file(node.document->filePath());
            const bool parentHasUsing = Utils::anyOf(node.includes, &Node::hasGlobalUsingDirective);
            const int startPos = parentHasUsing
                                     ? 0
                                     : RemoveNamespaceVisitor::SearchGlobalUsingDirectivePos;
            const bool noGlobalUsing = refactorFile(file, refactoring.snapshot(), startPos);
            node.hasGlobalUsingDirective = !noGlobalUsing || parentHasUsing;

            for (Node &subNode : node.includedBy) {
                --subNode.unprocessedParents;
                if (subNode.unprocessedParents == 0)
                    nodesWithProcessedParents.push_back(subNode);
            }
        }
    }

    void perform() override
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(filePath());
        if (m_removeAllAtGlobalScope) {
            removeAllUsingsAtGlobalScope(refactoring);
        } else if (refactorFile(currentFile,
                                refactoring.snapshot(),
                                currentFile->endOf(m_usingDirective),
                                true)) {
            processIncludes(refactoring, filePath());
        }

        for (auto &file : std::as_const(m_changes))
            file->apply();
    }

    /**
     * @brief refactorFile remove using namespace xyz in the given file and rewrite types
     * @param file The file that should be processed
     * @param snapshot The snapshot to work on
     * @param startSymbol start processing after this index
     * @param removeUsing if the using directive is in this file, remove it
     * @return true if the using statement is global and there is no other global using namespace
     */
    bool refactorFile(CppRefactoringFilePtr &file,
                      const Snapshot &snapshot,
                      int startSymbol,
                      bool removeUsing = false)
    {
        RemoveNamespaceVisitor visitor(file.get(),
                                       snapshot,
                                       m_usingDirective->name->name,
                                       startSymbol,
                                       m_removeAllAtGlobalScope);
        visitor.accept(file->cppDocument()->translationUnit()->ast());
        Utils::ChangeSet changes = visitor.getChanges();
        if (removeUsing)
            removeLine(file.get(), m_usingDirective, changes);
        if (!changes.isEmpty()) {
            file->setChangeSet(changes);
            // apply changes at the end, otherwise the symbol finder will fail to resolve symbols if
            // the using namespace is missing
            m_changes.insert(file);
        }
        return visitor.isGlobalUsingNamespace() && !visitor.foundGlobalUsingNamespace();
    }

    void processIncludes(CppRefactoringChanges &refactoring, const FilePath &filePath)
    {
        QList<Snapshot::IncludeLocation>
            includeLocationsOfDocument = refactoring.snapshot().includeLocationsOfDocument(filePath);
        for (Snapshot::IncludeLocation &loc : includeLocationsOfDocument) {
            if (!Utils::insert(m_processed, loc.first))
                continue;

            CppRefactoringFilePtr file = refactoring.file(loc.first->filePath());
            const bool noGlobalUsing = refactorFile(file,
                                                    refactoring.snapshot(),
                                                    file->position(loc.second, 1));
            if (noGlobalUsing)
                processIncludes(refactoring, loc.first->filePath());
        }
    }

    QSet<Document::Ptr> m_processed;
    QSet<CppRefactoringFilePtr> m_changes;

    UsingDirectiveAST *m_usingDirective;
    bool m_removeAllAtGlobalScope;
};
} // namespace

void RemoveUsingNamespace::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    if (CppModelManager::usesClangd(interface.currentFile()->editor()->textDocument()))
        return;

    const QList<AST *> &path = interface.path();
    // We expect something like
    // [0] TranslationUnitAST
    // ...
    // [] UsingDirectiveAST : if activated at 'using namespace'
    // [] NameAST (optional): if activated at the name e.g. 'std'
    int n = path.size() - 1;
    if (n <= 0)
        return;
    if (path.last()->asName())
        --n;
    UsingDirectiveAST *usingDirective = path.at(n)->asUsingDirective();
    if (usingDirective && usingDirective->name->name->asNameId()) {
        result << new RemoveUsingNamespaceOperation(interface, usingDirective, false);
        const bool isHeader = ProjectFile::isHeader(ProjectFile::classify(interface.filePath().toString()));
        if (isHeader && path.at(n - 1)->asTranslationUnit()) // using namespace at global scope
            result << new RemoveUsingNamespaceOperation(interface, usingDirective, true);
    }
}

namespace {

struct ParentClassConstructorInfo;

class ConstructorMemberInfo
{
public:
    ConstructorMemberInfo(const QString &name, Symbol *symbol, int numberOfMember)
        : memberVariableName(name)
        , parameterName(memberBaseName(name))
        , symbol(symbol)
        , type(symbol->type())
        , numberOfMember(numberOfMember)
    {}
    ConstructorMemberInfo(const QString &memberName,
                          const QString &paramName,
                          const QString &defaultValue,
                          Symbol *symbol,
                          const ParentClassConstructorInfo *parentClassConstructor)
        : parentClassConstructor(parentClassConstructor)
        , memberVariableName(memberName)
        , parameterName(paramName)
        , defaultValue(defaultValue)
        , init(defaultValue.isEmpty())
        , symbol(symbol)
        , type(symbol->type())
    {}
    const ParentClassConstructorInfo *parentClassConstructor = nullptr;
    QString memberVariableName;
    QString parameterName;
    QString defaultValue;
    bool init = true;
    bool customValueType; // for the generation later
    Symbol *symbol; // for the right type later
    FullySpecifiedType type;
    int numberOfMember; // first member, second member, ...
};

class ConstructorParams : public QAbstractTableModel
{
    Q_OBJECT
    std::list<ConstructorMemberInfo> candidates;
    std::vector<ConstructorMemberInfo *> infos;

    void validateOrder()
    {
        // parameters with default values must be at the end
        bool foundWithDefault = false;
        for (auto info : infos) {
            if (info->init) {
                if (foundWithDefault && info->defaultValue.isEmpty()) {
                    emit validOrder(false);
                    return;
                }
                foundWithDefault |= !info->defaultValue.isEmpty();
            }
        }
        emit validOrder(true);
    }

public:
    enum Column { ShouldInitColumn, MemberNameColumn, ParameterNameColumn, DefaultValueColumn };
    template<typename... _Args>
    void emplaceBackParameter(_Args &&...__args)
    {
        candidates.emplace_back(std::forward<_Args>(__args)...);
        infos.push_back(&candidates.back());
    }
    const std::vector<ConstructorMemberInfo *> &getInfos() const { return infos; }
    void addRow(ConstructorMemberInfo *info)
    {
        beginInsertRows({}, rowCount(), rowCount());
        infos.push_back(info);
        endInsertRows();
        validateOrder();
    }
    void removeRow(ConstructorMemberInfo *info)
    {
        for (auto iter = infos.begin(); iter != infos.end(); ++iter) {
            if (*iter == info) {
                const auto index = iter - infos.begin();
                beginRemoveRows({}, index, index);
                infos.erase(iter);
                endRemoveRows();
                validateOrder();
                return;
            }
        }
    }

    int selectedCount() const
    {
        return Utils::count(infos, [](const ConstructorMemberInfo *mi) {
            return mi->init && !mi->parentClassConstructor;
        });
    }
    int memberCount() const
    {
        return Utils::count(infos, [](const ConstructorMemberInfo *mi) {
            return !mi->parentClassConstructor;
        });
    }

    int rowCount(const QModelIndex & /*parent*/ = {}) const override { return int(infos.size()); }
    int columnCount(const QModelIndex & /*parent*/ = {}) const override { return 4; }
    QVariant data(const QModelIndex &index, int role) const override
    {
        if (index.row() < 0 || index.row() >= rowCount())
            return {};
        if (role == Qt::CheckStateRole && index.column() == ShouldInitColumn
            && !infos[index.row()]->parentClassConstructor)
            return infos[index.row()]->init ? Qt::Checked : Qt::Unchecked;
        if (role == Qt::DisplayRole && index.column() == MemberNameColumn)
            return infos[index.row()]->memberVariableName;
        if ((role == Qt::DisplayRole || role == Qt::EditRole)
            && index.column() == ParameterNameColumn)
            return infos[index.row()]->parameterName;
        if ((role == Qt::DisplayRole || role == Qt::EditRole)
            && index.column() == DefaultValueColumn)
            return infos[index.row()]->defaultValue;
        if ((role == Qt::ToolTipRole) && index.column() > 0)
            return Overview{}.prettyType(infos[index.row()]->symbol->type());
        return {};
    }
    bool setData(const QModelIndex &index, const QVariant &value, int role) override
    {
        if (index.column() == ShouldInitColumn && role == Qt::CheckStateRole) {
            if (infos[index.row()]->parentClassConstructor)
                return false;
            infos[index.row()]->init = value.toInt() == Qt::Checked;
            emit dataChanged(this->index(index.row(), 0), this->index(index.row(), columnCount()));
            validateOrder();
            return true;
        }
        if (index.column() == ParameterNameColumn && role == Qt::EditRole) {
            infos[index.row()]->parameterName = value.toString();
            return true;
        }
        if (index.column() == DefaultValueColumn && role == Qt::EditRole) {
            infos[index.row()]->defaultValue = value.toString();
            validateOrder();
            return true;
        }
        return false;
    }
    Qt::DropActions supportedDropActions() const override { return Qt::MoveAction; }
    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        if (!index.isValid())
            return Qt::ItemIsSelectable | Qt::ItemIsDropEnabled;

        Qt::ItemFlags f{};
        if (infos[index.row()]->init) {
            f |= Qt::ItemIsDragEnabled;
            f |= Qt::ItemIsSelectable;
        }

        if (index.column() == ShouldInitColumn && !infos[index.row()]->parentClassConstructor)
            return f | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
        if (!infos[index.row()]->init)
            return f;
        if (index.column() == MemberNameColumn)
            return f | Qt::ItemIsEnabled;
        if (index.column() == ParameterNameColumn || index.column() == DefaultValueColumn)
            return f | Qt::ItemIsEnabled | Qt::ItemIsEditable;
        return {};
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            switch (section) {
            case ShouldInitColumn:
                return Tr::tr("Initialize in Constructor");
            case MemberNameColumn:
                return Tr::tr("Member Name");
            case ParameterNameColumn:
                return Tr::tr("Parameter Name");
            case DefaultValueColumn:
                return Tr::tr("Default Value");
            }
        }
        return {};
    }
    bool dropMimeData(const QMimeData *data,
                      Qt::DropAction /*action*/,
                      int row,
                      int /*column*/,
                      const QModelIndex & /*parent*/) override
    {
        if (row == -1)
            row = rowCount();
        bool ok;
        int sourceRow = data->data("application/x-qabstractitemmodeldatalist").toInt(&ok);
        if (ok) {
            if (sourceRow == row || row == sourceRow + 1)
                return false;
            beginMoveRows({}, sourceRow, sourceRow, {}, row);
            infos.insert(infos.begin() + row, infos.at(sourceRow));
            if (row < sourceRow)
                ++sourceRow;
            infos.erase(infos.begin() + sourceRow);
            validateOrder();
            return true;
        }
        return false;
    }

    QMimeData *mimeData(const QModelIndexList &indexes) const override
    {
        for (const auto &i : indexes) {
            if (!i.isValid())
                continue;
            auto data = new QMimeData();
            data->setData("application/x-qabstractitemmodeldatalist",
                          QString::number(i.row()).toLatin1());
            return data;
        }
        return nullptr;
    }

    class TableViewStyle : public QProxyStyle
    {
    public:
        TableViewStyle(QStyle *style)
            : QProxyStyle(style)
        {}

        void drawPrimitive(PrimitiveElement element,
                           const QStyleOption *option,
                           QPainter *painter,
                           const QWidget *widget) const override
        {
            if (element == QStyle::PE_IndicatorItemViewItemDrop && !option->rect.isNull()) {
                QStyleOption opt(*option);
                opt.rect.setLeft(0);
                if (widget)
                    opt.rect.setRight(widget->width());
                QProxyStyle::drawPrimitive(element, &opt, painter, widget);
                return;
            }
            QProxyStyle::drawPrimitive(element, option, painter, widget);
        }
    };
signals:
    void validOrder(bool valid);
};

class TopMarginDelegate : public QStyledItemDelegate
{
public:
    TopMarginDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent)
    {}

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        Q_ASSERT(index.isValid());
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        const QWidget *widget = option.widget;
        QStyle *style = widget ? widget->style() : QApplication::style();
        if (opt.rect.height() > 20)
            opt.rect.adjust(0, 5, 0, 0);
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
    }
};

struct ParentClassConstructorParameter : public ConstructorMemberInfo
{
    QString originalDefaultValue;
    QString declaration; // displayed in the treeView
    ParentClassConstructorParameter(const QString &name,
                                    const QString &defaultValue,
                                    Symbol *symbol,
                                    const ParentClassConstructorInfo *parentClassConstructor);

    ParentClassConstructorParameter(const ParentClassConstructorParameter &) = delete;
    ParentClassConstructorParameter(ParentClassConstructorParameter &&) = default;
};

struct ParentClassConstructorInfo
{
    ParentClassConstructorInfo(const QString &name, ConstructorParams &model)
        : className(name)
        , model(model)
    {}
    bool useInConstructor = false;
    const QString className;
    QString declaration;
    std::vector<ParentClassConstructorParameter> parameters;
    ConstructorParams &model;

    ParentClassConstructorInfo(const ParentClassConstructorInfo &) = delete;
    ParentClassConstructorInfo(ParentClassConstructorInfo &&) = default;

    void addParameter(ParentClassConstructorParameter &param) { model.addRow(&param); }
    void removeParameter(ParentClassConstructorParameter &param) { model.removeRow(&param); }
    void removeAllParameters()
    {
        for (auto &param : parameters)
            model.removeRow(&param);
    }
};

ParentClassConstructorParameter::ParentClassConstructorParameter(
    const QString &name,
    const QString &defaultValue,
    Symbol *symbol,
    const ParentClassConstructorInfo *parentClassConstructor)
    : ConstructorMemberInfo(parentClassConstructor->className + "::" + name,
                            name,
                            defaultValue,
                            symbol,
                            parentClassConstructor)
    , originalDefaultValue(defaultValue)
    , declaration(Overview{}.prettyType(symbol->type(), name)
                  + (defaultValue.isEmpty() ? QString{} : " = " + defaultValue))
{}

using ParentClassConstructors = std::vector<ParentClassConstructorInfo>;

class ParentClassesModel : public QAbstractItemModel
{
    ParentClassConstructors &constructors;

public:
    ParentClassesModel(QObject *parent, ParentClassConstructors &constructors)
        : QAbstractItemModel(parent)
        , constructors(constructors)
    {}
    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override
    {
        if (!parent.isValid())
            return createIndex(row, column, nullptr);
        if (parent.internalPointer())
            return {};
        auto index = createIndex(row, column, &constructors.at(parent.row()));
        return index;
    }
    QModelIndex parent(const QModelIndex &index) const override
    {
        if (!index.isValid())
            return {};
        auto *parent = static_cast<ParentClassConstructorInfo *>(index.internalPointer());
        if (!parent)
            return {};
        int i = 0;
        for (const auto &info : constructors) {
            if (&info == parent)
                return createIndex(i, 0, nullptr);
            ++i;
        }
        return {};
    }
    int rowCount(const QModelIndex &parent = {}) const override
    {
        if (!parent.isValid())
            return static_cast<int>(constructors.size());
        auto info = static_cast<ParentClassConstructorInfo *>(parent.internalPointer());
        if (!info)
            return static_cast<int>(constructors.at(parent.row()).parameters.size());
        return 0;
    }
    int columnCount(const QModelIndex & /*parent*/ = {}) const override { return 1; }
    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid())
            return {};
        auto info = static_cast<ParentClassConstructorInfo *>(index.internalPointer());

        if (info) {
            const auto &parameter = info->parameters.at(index.row());
            if (role == Qt::CheckStateRole)
                return parameter.init ? Qt::Checked : Qt::Unchecked;
            if (role == Qt::DisplayRole)
                return parameter.declaration;
            return {};
        }
        const auto &constructor = constructors.at(index.row());
        if (role == Qt::CheckStateRole)
            return constructor.useInConstructor ? Qt::PartiallyChecked : Qt::Unchecked;
        if (role == Qt::DisplayRole)
            return constructor.declaration;

        // Highlight the selected items
        if (role == Qt::FontRole && constructor.useInConstructor) {
            QFont font = QApplication::font();
            font.setBold(true);
            return font;
        }
        // Create a margin between sets of constructors for base classes
        if (role == Qt::SizeHintRole && index.row() > 0
            && constructor.className != constructors.at(index.row() - 1).className) {
            return QSize(-1, 25);
        }
        return {};
    }
    bool setData(const QModelIndex &index, const QVariant &value, int /*role*/) override
    {
        if (index.isValid() && index.column() == 0) {
            auto info = static_cast<ParentClassConstructorInfo *>(index.internalPointer());
            if (info) {
                const bool nowUse = value.toBool();
                auto &param = info->parameters.at(index.row());
                param.init = nowUse;
                if (nowUse)
                    info->addParameter(param);
                else
                    info->removeParameter(param);
                return true;
            }
            auto &newConstructor = constructors.at(index.row());
            // You have to select a base class constructor
            if (newConstructor.useInConstructor)
                return false;
            auto c = std::find_if(constructors.begin(), constructors.end(), [&](const auto &c) {
                return c.className == newConstructor.className && c.useInConstructor;
            });
            QTC_ASSERT(c == constructors.end(), return false;);
            c->useInConstructor = false;
            newConstructor.useInConstructor = true;
            emit dataChanged(this->index(index.row(), 0), this->index(index.row(), columnCount()));
            auto parentIndex = this->index(index.row(), 0);
            emit dataChanged(this->index(0, 0, parentIndex),
                             this->index(rowCount(parentIndex), columnCount()));
            const int oldIndex = c - constructors.begin();
            emit dataChanged(this->index(oldIndex, 0), this->index(oldIndex, columnCount()));
            parentIndex = this->index(oldIndex, 0);
            emit dataChanged(this->index(0, 0, parentIndex),
                             this->index(rowCount(parentIndex), columnCount()));
            // update other table
            c->removeAllParameters();
            for (auto &p : newConstructor.parameters)
                if (p.init)
                    newConstructor.addParameter(p);
            return true;
        }
        return false;
    }
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            switch (section) {
            case 0:
                return Tr::tr("Base Class Constructors");
            }
        }
        return {};
    }
    Qt::ItemFlags flags(const QModelIndex &index) const override
    {
        if (index.isValid()) {
            Qt::ItemFlags f;
            auto info = static_cast<ParentClassConstructorInfo *>(index.internalPointer());
            if (!info || info->useInConstructor) {
                f |= Qt::ItemIsEnabled;
            }
            f |= Qt::ItemIsUserCheckable;

            return f;
        }
        return {};
    }
};

class GenerateConstructorDialog : public QDialog
{
public:
    GenerateConstructorDialog(ConstructorParams *constructorParamsModel,
                              ParentClassConstructors &constructors)
    {
        setWindowTitle(Tr::tr("Constructor"));

        const auto treeModel = new ParentClassesModel(this, constructors);
        const auto treeView = new QTreeView(this);
        treeView->setModel(treeModel);
        treeView->setItemDelegate(new TopMarginDelegate(this));
        treeView->expandAll();

        const auto view = new QTableView(this);
        view->setModel(constructorParamsModel);
        int optimalWidth = 0;
        for (int i = 0; i < constructorParamsModel->columnCount(QModelIndex{}); ++i) {
            view->resizeColumnToContents(i);
            optimalWidth += view->columnWidth(i);
        }
        view->resizeRowsToContents();
        view->verticalHeader()->setDefaultSectionSize(view->rowHeight(0));
        view->setSelectionBehavior(QAbstractItemView::SelectRows);
        view->setSelectionMode(QAbstractItemView::SingleSelection);
        view->setDragEnabled(true);
        view->setDropIndicatorShown(true);
        view->setDefaultDropAction(Qt::MoveAction);
        view->setDragDropMode(QAbstractItemView::InternalMove);
        view->setDragDropOverwriteMode(false);
        view->horizontalHeader()->setStretchLastSection(true);
        view->setStyle(new ConstructorParams::TableViewStyle(view->style()));

        const auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        const auto errorLabel = new QLabel(
            Tr::tr("Parameters without default value must come before parameters with default value."));
        errorLabel->setStyleSheet("color: #ff0000");
        errorLabel->setVisible(false);
        QSizePolicy labelSizePolicy = errorLabel->sizePolicy();
        labelSizePolicy.setRetainSizeWhenHidden(true);
        errorLabel->setSizePolicy(labelSizePolicy);
        connect(constructorParamsModel, &ConstructorParams::validOrder, this,
                [errorLabel, button = buttonBox->button(QDialogButtonBox::Ok)](bool valid) {
                    button->setEnabled(valid);
                    errorLabel->setVisible(!valid);
                });

        // setup select all/none checkbox
        QCheckBox *const checkBox = new QCheckBox(Tr::tr("Initialize all members"));
        checkBox->setChecked(true);
        connect(checkBox, &QCheckBox::stateChanged, this,
                [model = constructorParamsModel](int state) {
            if (state != Qt::PartiallyChecked) {
                for (int i = 0; i < model->rowCount(); ++i)
                    model->setData(model->index(i, ConstructorParams::ShouldInitColumn),
                                   state,
                                   Qt::CheckStateRole);
            }
        });
        connect(checkBox, &QCheckBox::clicked, this, [checkBox] {
            if (checkBox->checkState() == Qt::PartiallyChecked)
                checkBox->setCheckState(Qt::Checked);
        });
        connect(constructorParamsModel,
                &QAbstractItemModel::dataChanged,
                this,
                [model = constructorParamsModel, checkBox] {
                    const auto state = [model, selectedCount = model->selectedCount()]() {
                        if (selectedCount == 0)
                            return Qt::Unchecked;
                        if (static_cast<int>(model->memberCount() == selectedCount))
                            return Qt::Checked;
                        return Qt::PartiallyChecked;
                    }();
                    checkBox->setCheckState(state);
                });

        using A = InsertionPointLocator::AccessSpec;
        auto accessCombo = new QComboBox;
        connect(accessCombo, &QComboBox::currentIndexChanged, this, [this, accessCombo] {
            const auto data = accessCombo->currentData();
            m_accessSpec = static_cast<A>(data.toInt());
        });
        for (auto a : {A::Public, A::Protected, A::Private})
            accessCombo->addItem(InsertionPointLocator::accessSpecToString(a), a);
        const auto row = new QHBoxLayout();
        row->addWidget(new QLabel(Tr::tr("Access") + ":"));
        row->addWidget(accessCombo);
        row->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum));

        const auto mainLayout = new QVBoxLayout(this);
        mainLayout->addWidget(
            new QLabel(Tr::tr("Select the members to be initialized in the constructor.\n"
                              "Use drag and drop to change the order of the parameters.")));
        mainLayout->addLayout(row);
        mainLayout->addWidget(checkBox);
        mainLayout->addWidget(view);
        mainLayout->addWidget(treeView);
        mainLayout->addWidget(errorLabel);
        mainLayout->addWidget(buttonBox);
        int left, right;
        mainLayout->getContentsMargins(&left, nullptr, &right, nullptr);
        optimalWidth += left + right;
        resize(optimalWidth, mainLayout->sizeHint().height());
    }

    InsertionPointLocator::AccessSpec accessSpec() const { return m_accessSpec; }

private:
    InsertionPointLocator::AccessSpec m_accessSpec;
};

class GenerateConstructorOperation : public CppQuickFixOperation
{
public:
    GenerateConstructorOperation(const CppQuickFixInterface &interface)
        : CppQuickFixOperation(interface)
    {
        setDescription(Tr::tr("Generate Constructor"));

        m_classAST = astForClassOperations(interface);
        if (!m_classAST)
            return;
        Class *const theClass = m_classAST->symbol;
        if (!theClass)
            return;

        // Go through all members and find member variable declarations
        int memberCounter = 0;
        for (auto it = theClass->memberBegin(); it != theClass->memberEnd(); ++it) {
            Symbol *const s = *it;
            if (!s->identifier() || !s->type() || s->type().isTypedef())
                continue;
            if ((s->asDeclaration() && s->type()->asFunctionType()) || s->asFunction())
                continue;
            if (s->asDeclaration() && (s->isPrivate() || s->isProtected()) && !s->isStatic()) {
                const auto name = QString::fromUtf8(s->identifier()->chars(),
                                                    s->identifier()->size());
                parameterModel.emplaceBackParameter(name, s, memberCounter++);
            }
        }
        Overview o = CppCodeStyleSettings::currentProjectCodeStyleOverview();
        o.showArgumentNames = true;
        o.showReturnTypes = true;
        o.showDefaultArguments = true;
        o.showTemplateParameters = true;
        o.showFunctionSignatures = true;
        LookupContext context(currentFile()->cppDocument(), interface.snapshot());
        for (BaseClass *bc : theClass->baseClasses()) {
            const QString className = o.prettyName(bc->name());

            ClassOrNamespace *localLookupType = context.lookupType(bc);
            QList<LookupItem> localLookup = localLookupType->lookup(bc->name());
            for (auto &li : localLookup) {
                Symbol *d = li.declaration();
                if (!d->asClass())
                    continue;
                for (auto i = d->asClass()->memberBegin(); i != d->asClass()->memberEnd(); ++i) {
                    Symbol *s = *i;
                    if (s->isProtected() || s->isPublic()) {
                        if (s->name()->match(d->name())) {
                            // we have found a constructor
                            Function *func = s->type().type()->asFunctionType();
                            if (!func)
                                continue;
                            const bool isFirst = parentClassConstructors.empty()
                                                 || parentClassConstructors.back().className
                                                        != className;
                            parentClassConstructors.emplace_back(className, parameterModel);
                            ParentClassConstructorInfo &constructor = parentClassConstructors.back();
                            constructor.declaration = className + o.prettyType(func->type());
                            constructor.declaration.replace("std::__1::__get_nullptr_t()",
                                                            "nullptr");
                            constructor.useInConstructor = isFirst;
                            for (auto arg = func->memberBegin(); arg != func->memberEnd(); ++arg) {
                                Symbol *param = *arg;
                                Argument *argument = param->asArgument();
                                if (!argument) // can also be a block
                                    continue;
                                const QString name = o.prettyName(param->name());
                                const StringLiteral *ini = argument->initializer();
                                QString defaultValue;
                                if (ini)
                                    defaultValue = QString::fromUtf8(ini->chars(), ini->size())
                                                       .replace("std::__1::__get_nullptr_t()",
                                                                "nullptr");
                                constructor.parameters.emplace_back(name,
                                                                    defaultValue,
                                                                    param,
                                                                    &constructor);
                                // do not show constructors like QObject(QObjectPrivate & dd, ...)
                                ReferenceType *ref = param->type()->asReferenceType();
                                if (ref && name == "dd") {
                                    auto type = o.prettyType(ref->elementType());
                                    if (type.startsWith("Q") && type.endsWith("Private")) {
                                        parentClassConstructors.pop_back();
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // add params to parameter lists
        for (auto &c : parentClassConstructors)
            if (c.useInConstructor)
                for (auto &p : c.parameters)
                    if (p.init)
                        c.addParameter(p);
    }

    bool isApplicable() const
    {
        return parameterModel.rowCount() > 0
               || Utils::anyOf(parentClassConstructors,
                               [](const auto &parent) { return !parent.parameters.empty(); });
    }

    void setTest(bool isTest = true) { m_test = isTest; }

private:
    void perform() override
    {
        auto infos = parameterModel.getInfos();

        InsertionPointLocator::AccessSpec accessSpec = InsertionPointLocator::Public;
        if (!m_test) {
            GenerateConstructorDialog dlg(&parameterModel, parentClassConstructors);
            if (dlg.exec() == QDialog::Rejected)
                return;
            accessSpec = dlg.accessSpec();
            infos = parameterModel.getInfos();
        } else {
#ifdef WITH_TESTS
            ParentClassesModel model(nullptr, parentClassConstructors);
            QAbstractItemModelTester tester(&model);
#endif
            if (infos.size() >= 3) {
                // if we are testing and have 3 or more members => change the order
                // move first element to the back
                infos.push_back(infos[0]);
                infos.erase(infos.begin());
            }
            for (auto info : infos) {
                if (info->memberVariableName.startsWith("di_"))
                    info->defaultValue = "42";
            }
            for (auto &c : parentClassConstructors) {
                if (c.useInConstructor) {
                    for (auto &p : c.parameters) {
                        if (!p.init && p.parameterName.startsWith("use_")) {
                            infos.push_back(&p);
                            p.init = true;
                        }
                    }
                }
            }
        }
        if (infos.empty())
            return;
        struct GenerateConstructorRefactoringHelper : public GetterSetterRefactoringHelper
        {
            const ClassSpecifierAST *m_classAST;
            InsertionPointLocator::AccessSpec m_accessSpec;
            GenerateConstructorRefactoringHelper(CppQuickFixOperation *operation,
                                                 const FilePath &filePath,
                                                 Class *clazz,
                                                 const ClassSpecifierAST *classAST,
                                                 InsertionPointLocator::AccessSpec accessSpec)
                : GetterSetterRefactoringHelper(operation, filePath, clazz)
                , m_classAST(classAST)
                , m_accessSpec(accessSpec)
            {}
            void generateConstructor(std::vector<ConstructorMemberInfo *> members,
                                     const ParentClassConstructors &parentClassConstructors)
            {
                auto constructorLocation = m_settings->determineSetterLocation(int(members.size()));
                if (constructorLocation == CppQuickFixSettings::FunctionLocation::CppFile
                    && !hasSourceFile())
                    constructorLocation = CppQuickFixSettings::FunctionLocation::OutsideClass;

                Overview overview = CppCodeStyleSettings::currentProjectCodeStyleOverview();
                overview.showTemplateParameters = true;

                InsertionLocation implLoc;
                QString implCode;
                CppRefactoringFilePtr implFile;
                QString className = overview.prettyName(m_class->name());
                QStringList insertedNamespaces;
                if (constructorLocation == CppQuickFixSettings::FunctionLocation::CppFile) {
                    implLoc = sourceLocationFor(m_class, &insertedNamespaces);
                    implFile = m_sourceFile;
                    if (m_settings->rewriteTypesinCppFile())
                        implCode = symbolAt(m_class, m_sourceFile, implLoc);
                    else
                        implCode = className;
                    implCode += "::" + className + "(";
                } else if (constructorLocation
                           == CppQuickFixSettings::FunctionLocation::OutsideClass) {
                    implLoc = insertLocationForMethodDefinition(m_class,
                                                                false,
                                                                NamespaceHandling::Ignore,
                                                                m_changes,
                                                                m_headerFile->filePath(),
                                                                &insertedNamespaces);
                    implFile = m_headerFile;
                    implCode = symbolAt(m_class, m_headerFile, implLoc);
                    implCode += "::" + className + "(";
                }

                QString inClassDeclaration = overview.prettyName(m_class->name()) + "(";
                QString constructorBody = members.empty() ? QString(") {}") : QString(") : ");
                for (auto &member : members) {
                    if (isValueType(member->symbol, &member->customValueType))
                        member->type.setConst(false);
                    else
                        member->type = makeConstRef(member->type);

                    inClassDeclaration += overview.prettyType(member->type, member->parameterName);
                    if (!member->defaultValue.isEmpty())
                        inClassDeclaration += " = " + member->defaultValue;
                    inClassDeclaration += ", ";
                    if (implFile) {
                        FullySpecifiedType type = typeAt(member->type,
                                                         m_class,
                                                         implFile,
                                                         implLoc,
                                                         insertedNamespaces);
                        implCode += overview.prettyType(type, member->parameterName) + ", ";
                    }
                }
                Utils::sort(members, &ConstructorMemberInfo::numberOfMember);
                // first, do the base classes
                for (const auto &parent : parentClassConstructors) {
                    if (!parent.useInConstructor)
                        continue;
                    // Check if we really need a constructor
                    if (Utils::anyOf(parent.parameters, [](const auto &param) {
                            return param.init || param.originalDefaultValue.isEmpty();
                        })) {
                        int defaultAtEndCount = 0;
                        for (auto i = parent.parameters.crbegin(); i != parent.parameters.crend();
                             ++i) {
                            if (i->init || i->originalDefaultValue.isEmpty())
                                break;
                            ++defaultAtEndCount;
                        }
                        const int numberOfParameters = static_cast<int>(parent.parameters.size())
                                                       - defaultAtEndCount;
                        constructorBody += parent.className + "(";
                        int counter = 0;
                        for (const auto &param : parent.parameters) {
                            if (++counter > numberOfParameters)
                                break;
                            if (param.init) {
                                if (param.customValueType)
                                    constructorBody += "std::move(" + param.parameterName + ')';
                                else
                                    constructorBody += param.parameterName;
                            } else if (!param.originalDefaultValue.isEmpty())
                                constructorBody += param.originalDefaultValue;
                            else
                                constructorBody += "/* insert value */";
                            constructorBody += ", ";
                        }
                        constructorBody.resize(constructorBody.length() - 2);
                        constructorBody += "),\n";
                    }
                }
                for (auto &member : members) {
                    if (member->parentClassConstructor)
                        continue;
                    QString param = member->parameterName;
                    if (member->customValueType)
                        param = "std::move(" + member->parameterName + ')';
                    constructorBody += member->memberVariableName + '(' + param + "),\n";
                }
                if (!members.empty()) {
                    inClassDeclaration.resize(inClassDeclaration.length() - 2);
                    constructorBody.remove(constructorBody.length() - 2, 1); // ..),\n => ..)\n
                    constructorBody += "{}";
                    if (!implCode.isEmpty())
                        implCode.resize(implCode.length() - 2);
                }
                implCode += constructorBody;

                if (constructorLocation == CppQuickFixSettings::FunctionLocation::InsideClass)
                    inClassDeclaration += constructorBody;
                else
                    inClassDeclaration += QLatin1String(");");

                TranslationUnit *tu = m_headerFile->cppDocument()->translationUnit();
                insertAndIndent(m_headerFile,
                                m_locator.constructorDeclarationInClass(tu,
                                                                        m_classAST,
                                                                        m_accessSpec,
                                                                        int(members.size())),
                                inClassDeclaration);

                if (constructorLocation == CppQuickFixSettings::FunctionLocation::CppFile) {
                    addSourceFileCode(implCode);
                } else if (constructorLocation
                           == CppQuickFixSettings::FunctionLocation::OutsideClass) {
                    if (m_isHeaderHeaderFile)
                        implCode.prepend("inline ");
                    insertAndIndent(m_headerFile, implLoc, implCode);
                }
            }
        };
        GenerateConstructorRefactoringHelper helper(this,
                                                    currentFile()->filePath(),
                                                    m_classAST->symbol,
                                                    m_classAST,
                                                    accessSpec);

        auto members = Utils::filtered(infos, [](const auto mi) {
            return mi->init || mi->parentClassConstructor;
        });
        helper.generateConstructor(std::move(members), parentClassConstructors);
        helper.applyChanges();
    }

    ConstructorParams parameterModel;
    ParentClassConstructors parentClassConstructors;
    const ClassSpecifierAST *m_classAST = nullptr;
    bool m_test = false;
};
} // namespace
void GenerateConstructor::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const auto op = QSharedPointer<GenerateConstructorOperation>::create(interface);
    if (!op->isApplicable())
        return;
    op->setTest(m_test);
    result << op;
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

void ConvertCommentStyle::match(const CppQuickFixInterface &interface,
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
        CppModelManager::followSymbol(cursorInEditor, callback, true, false);
    }

    static void moveComments(const Link &targetLoc, const Link &symbolLoc,
                             const QList<Token> &comments)
    {
        if (!targetLoc.hasValidTarget() || targetLoc.hasSameLocation(symbolLoc))
            return;

        CppRefactoringChanges changes(CppModelManager::snapshot());
        const CppRefactoringFilePtr sourceFile = changes.file(symbolLoc.targetFilePath);
        const CppRefactoringFilePtr targetFile
            = targetLoc.targetFilePath == symbolLoc.targetFilePath
                  ? sourceFile
                  : changes.file(targetLoc.targetFilePath);
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

void MoveFunctionComments::match(const CppQuickFixInterface &interface,
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

    new AddBracesToIf;
    new RearrangeParamDeclarationList;
    new ReformatPointerDeclaration;

    new CompleteSwitchCaseStatement;
    new InsertQtPropertyMembers;
    new ConvertQt4Connect;

    new ApplyDeclDefLinkChanges;
    new ConvertFromAndToPointer;
    new ExtractFunction;
    new ExtractLiteralAsParameter;
    new GenerateGetterSetter;
    new GenerateGettersSettersForClass;
    new InsertDeclFromDef;
    new InsertDefFromDecl;
    new AddDeclarationForUndeclaredIdentifier;
    new InsertDefsFromDecls;

    new MoveFuncDefOutside;
    new MoveAllFuncDefOutside;
    new MoveFuncDefToDecl;

    new AssignToLocalVariable;

    new InsertVirtualMethods;

    new OptimizeForLoop;

    new EscapeStringLiteral;

    new ExtraRefactoringOperations;

    new RemoveUsingNamespace;
    new GenerateConstructor;
    new ConvertCommentStyle;
    new MoveFunctionComments;
}

void destroyCppQuickFixes()
{
    for (int i = g_cppQuickFixFactories.size(); --i >= 0; )
        delete g_cppQuickFixFactories.at(i);
}

} // namespace Internal
} // namespace CppEditor

#include "cppquickfixes.moc"
