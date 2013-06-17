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

#include "cppeditor.h"
#include "cppfunctiondecldeflink.h"
#include "cppquickfixassistant.h"

#include <coreplugin/icore.h>

#include <cpptools/cppclassesfilter.h>
#include <cpptools/cppcodestylesettings.h>
#include <cpptools/cpppointerdeclarationformatter.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/includeutils.h>
#include <cpptools/insertionpointlocator.h>
#include <cpptools/symbolfinder.h>

#include <cplusplus/ASTPath.h>
#include <cplusplus/CPlusPlusForwardDeclarations.h>
#include <cplusplus/CppRewriter.h>
#include <cplusplus/DependencyTable.h>
#include <cplusplus/TypeOfExpression.h>

#include <extensionsystem/pluginmanager.h>

#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <utils/qtcassert.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QItemDelegate>
#include <QLabel>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QQueue>
#include <QSharedPointer>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTextBlock>
#include <QTextCursor>
#include <QTreeView>
#include <QVBoxLayout>

#include <cctype>

using namespace CPlusPlus;
using namespace CppEditor;
using namespace CppEditor::Internal;
using namespace CppTools;
using namespace TextEditor;
using namespace Utils;

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

    plugIn->addAutoReleasedObject(new MoveFuncDefOutside);
    plugIn->addAutoReleasedObject(new MoveFuncDefToDecl);

    plugIn->addAutoReleasedObject(new AssignToLocalVariable);

    plugIn->addAutoReleasedObject(new InsertVirtualMethods);
}

// In the following anonymous namespace all functions are collected, which could be of interest for
// different quick fixes.
namespace {

enum DefPos {
    DefPosInsideClass,
    DefPosOutsideClass,
    DefPosImplementationFile
};

InsertionLocation insertLocationForMethodDefinition(Symbol *symbol, const bool useSymbolFinder,
                                                    CppRefactoringChanges& refactoring,
                                                    const QString& fileName)
{
    QTC_ASSERT(symbol, return InsertionLocation());

    // Try to find optimal location
    const InsertionPointLocator locator(refactoring);
    const QList<InsertionLocation> list
            = locator.methodDefinition(symbol, useSymbolFinder, fileName);
    for (int i = 0; i < list.count(); ++i) {
        InsertionLocation location = list.at(i);
        if (location.isValid() && location.fileName() == fileName) {
            return location;
            break;
        }
    }

    // ...failed,
    // if class member try to get position right after class
    CppRefactoringFilePtr file = refactoring.file(fileName);
    unsigned line = 0, column = 0;
    if (Class *clazz = symbol->enclosingClass()) {
        if (symbol->fileName() == fileName.toUtf8()) {
            file->cppDocument()->translationUnit()->getPosition(clazz->endOffset(), &line, &column);
            if (line != 0) {
                ++column; // Skipping the ";"
                return InsertionLocation(fileName, QLatin1String("\n\n"), QLatin1String(""),
                                         line, column);
            }
        }
    }

    // fall through: position at end of file
    const QTextDocument *doc = file->document();
    int pos = qMax(0, doc->characterCount() - 1);

    //TODO watch for matching namespace
    //TODO watch for moc-includes

    file->lineAndColumn(pos, &line, &column);
    return InsertionLocation(fileName, QLatin1String("\n\n"), QLatin1String("\n"), line, column);
}

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
    QTC_ASSERT(function, return 0);

    Scope *enclosingScope = function->enclosingScope();
    while (! (enclosingScope->isNamespace() || enclosingScope->isClass()))
        enclosingScope = enclosingScope->enclosingScope();
    QTC_ASSERT(enclosingScope != 0, return 0);

    const Name *functionName = function->name();
    if (! functionName)
        return 0; // anonymous function names are not valid c++

    if (! functionName->isQualifiedNameId())
        return 0; // trying to add a declaration for a global function

    const QualifiedNameId *q = functionName->asQualifiedNameId();
    if (!q->base())
        return 0;

    if (ClassOrNamespace *binding = context.lookupType(q->base(), enclosingScope)) {
        foreach (Symbol *s, binding->symbols()) {
            if (Class *matchingClass = s->asClass())
                return matchingClass;
        }
    }

    return 0;
}

// Given include is e.g. "afile.h" or <afile.h> (quotes/angle brackets included!).
void insertNewIncludeDirective(const QString &include, CppRefactoringFilePtr file)
{
    // Find optimal position
    using namespace IncludeUtils;
    LineForNewIncludeDirective finder(file->document(), file->cppDocument()->includes(),
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
    return name->isOperatorNameId()
        || (name->isQualifiedNameId() && name->asQualifiedNameId()->name()->isOperatorNameId());
}

} // anonymous namespace

namespace {

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

} // anonymous namespace

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
        changes.flip(currentFile->range(binary->left_expression),
                     currentFile->range(binary->right_expression));
        if (! replacement.isEmpty())
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

} // anonymous namespace

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
        op->setDescription(QApplication::translate("CppTools::QuickFix",
                                                   "Rewrite Condition Using ||"));
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

namespace {

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

} // anonymous namespace

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

namespace {

class AddBracesToIfOp: public CppQuickFixOperation
{
public:
    AddBracesToIfOp(const CppQuickFixInterface &interface, int priority, StatementAST *statement)
        : CppQuickFixOperation(interface, priority)
        , _statement(statement)
    {
        setDescription(QApplication::translate("CppTools::QuickFix", "Add Curly Braces"));
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
        currentFile->appendIndentRange(ChangeSet::Range(start, end));
        currentFile->apply();
    }

private:
    StatementAST *_statement;
};

} // anonymous namespace

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

namespace {

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
    ConditionAST *condition;
    IfStatementAST *pattern;
    CoreDeclaratorAST *core;
};

} // anonymous namespace

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

namespace {

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
    ConditionAST *condition;
    WhileStatementAST *pattern;
    CoreDeclaratorAST *core;
};

} // anonymous namespace

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

} // anonymous namespace

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

/* Analze a string/character literal like "x", QLatin1String("x") and return the literal
 * (StringLiteral or NumericLiteral for characters) and its type
 * and the enclosing function (QLatin1String, tr...) */
ExpressionAST *WrapStringLiteral::analyze(const QList<AST *> &path,
                                          const CppRefactoringFilePtr &file, Type *type,
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

namespace {

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
            const QString newQuote((m_actions & Factory::SingleQuoteAction)
                                   ? QLatin1Char('\'') : QLatin1Char('"'));
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
            if (m_actions
                    & (Factory::TranslateQCoreApplicationAction | Factory::TranslateNoopAction)) {
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
                result << OperationPtr(new WrapStringLiteralOp(interface, priority, actions,
                                                               description, literal));
                actions &= ~EncloseInQLatin1CharAction;
                description = QApplication::translate("CppTools::QuickFix",
                              "Convert to Character Literal");
                result << OperationPtr(new WrapStringLiteralOp(interface, priority, actions,
                                                               description, literal));
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

inline QString WrapStringLiteral::msgQtStringLiteralDescription(const QString &replacement,
                                                                       int qtVersion)
{
    return QApplication::translate("CppTools::QuickFix", "Enclose in %1(...) (Qt %2)")
           .arg(replacement).arg(qtVersion);
}

inline QString WrapStringLiteral::msgQtStringLiteralDescription(const QString &replacement)
{
    return QApplication::translate("CppTools::QuickFix", "Enclose in %1(...)").arg(replacement);
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

    QSharedPointer<Control> control = interface->context().bindings()->control();
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
        setDescription(QApplication::translate("CppTools::QuickFix",
                                               "Convert to Objective-C String Literal"));
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());

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
    CppRefactoringFilePtr file = interface->currentFile();

    if (!interface->editor()->isObjCEnabled())
        return;

    WrapStringLiteral::Type type = WrapStringLiteral::TypeNone;
    QByteArray enclosingFunction;
    CallAST *qlatin1Call;
    const QList<AST *> &path = interface->path();
    ExpressionAST *literal = WrapStringLiteral::analyze(path, file, &type, &enclosingFunction,
                                                        &qlatin1Call);
    if (!literal || type != WrapStringLiteral::TypeString)
        return;
    if (!isQtStringLiteral(enclosingFunction))
        qlatin1Call = 0;

    result.append(QuickFixOperation::Ptr(
        new ConvertCStringToNSStringOp(interface, path.size() - 1, literal->asStringLiteral(),
            qlatin1Call)));
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

} // anonymous namespace

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

namespace {

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

        SymbolFinder symbolFinder;
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

            const QString include = QString::fromLatin1("<%1>").arg(QFileInfo(best).fileName());
            insertNewIncludeDirective(include, currentFile);
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
                         interface->context().lookup(name, interface->semanticInfo().doc->scopeAt(line, column))) {
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

} // anonymous namespace

void AddIncludeForForwardDeclaration::match(const CppQuickFixInterface &interface,
                                            QuickFixOperations &result)
{
    const QList<AST *> &path = interface->path();

    for (int index = path.size() - 1; index != -1; --index) {
        AST *ast = path.at(index);
        if (NamedTypeSpecifierAST *namedTy = ast->asNamedTypeSpecifier()) {
            if (Symbol *fwdClass = AddIncludeForForwardDeclarationOp::checkName(interface,
                                                                                namedTy->name)) {
                result.append(QuickFixOperation::Ptr(
                    new AddIncludeForForwardDeclarationOp(interface, index, fwdClass)));
                return;
            }
        } else if (ElaboratedTypeSpecifierAST *eTy = ast->asElaboratedTypeSpecifier()) {
            if (Symbol *fwdClass = AddIncludeForForwardDeclarationOp::checkName(interface,
                                                                                eTy->name)) {
                result.append(QuickFixOperation::Ptr(
                    new AddIncludeForForwardDeclarationOp(interface, index, fwdClass)));
                return;
            }
        }
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

            Control *control = assistInterface()->context().bindings()->control().data();
            FullySpecifiedType tn = rewriteType(result.first().type(), &env, control);

            Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
            QString ty = oo.prettyType(tn, simpleNameAST->name);
            if (! ty.isEmpty()) {
                ChangeSet changes;
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

} // anonymous namespace

void AddLocalDeclaration::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface->path();
    CppRefactoringFilePtr file = interface->currentFile();

    for (int index = path.size() - 1; index != -1; --index) {
        if (BinaryExpressionAST *binary = path.at(index)->asBinaryExpression()) {
            if (binary->left_expression && binary->right_expression
                    && file->tokenAt(binary->binary_op_token).is(T_EQUAL)) {
                IdExpressionAST *idExpr = binary->left_expression->asIdExpression();
                if (interface->isCursorOn(binary->left_expression) && idExpr
                        && idExpr->name->asSimpleName() != 0) {
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

namespace {

class ConvertToCamelCaseOp: public CppQuickFixOperation
{
public:
    ConvertToCamelCaseOp(const CppQuickFixInterface &interface, int priority,
                         const QString &newName)
        : CppQuickFixOperation(interface, priority)
        , m_name(newName)
    {
        setDescription(QApplication::translate("CppTools::QuickFix", "Convert to Camel Case"));
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
        static_cast<CPPEditorWidget*>(assistInterface()->editor())->renameUsagesNow(m_name);
    }

    static bool isConvertibleUnderscore(const QString &name, int pos)
    {
        return name.at(pos) == QLatin1Char('_') && name.at(pos+1).isLetter()
                && !(pos == 1 && name.at(0) == QLatin1Char('m'));
    }

private:
    QString m_name;
};

} // anonymous namespace

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

AddIncludeForUndefinedIdentifierOp::AddIncludeForUndefinedIdentifierOp(
        const CppQuickFixInterface &interface, int priority, const QString &include)
    : CppQuickFixOperation(interface, priority)
    , m_include(include)
{
    setDescription(QApplication::translate("CppTools::QuickFix", "Add #include %1").arg(m_include));
}

void AddIncludeForUndefinedIdentifierOp::perform()
{
    CppRefactoringChanges refactoring(snapshot());
    CppRefactoringFilePtr file = refactoring.file(fileName());

    insertNewIncludeDirective(m_include, file);
}

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
        includePaths = modelManager->includePaths();
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

    const bool isProbablyAQtClass = className.size() > 2
            && className.at(0) == QLatin1Char('Q')
            && className.at(1).isUpper();

    if (!isProbablyAQtClass)
        return;

    // for QSomething, propose a <QSomething> include -- if such a class was in the locator
    if (classExists) {
        const QString include = QLatin1Char('<') + className + QLatin1Char('>');
        result += CppQuickFixOperation::Ptr(
                    new AddIncludeForUndefinedIdentifierOp(interface, 1, include));

    // otherwise, check for a header file with the same name in the Qt include paths
    } else {
        foreach (const QString &includePath, includePaths) {
            if (!includePath.contains(QLatin1String("/Qt"))) // "QtCore", "QtGui" etc...
                continue;

            const QString headerPathCandidate = includePath + QLatin1Char('/') + className;
            const QFileInfo fileInfo(headerPathCandidate);
            if (fileInfo.exists() && fileInfo.isFile()) {
                const QString include = QLatin1Char('<') + className + QLatin1Char('>');
                result += CppQuickFixOperation::Ptr(
                            new AddIncludeForUndefinedIdentifierOp(interface, 1, include));
                break;
            }
        }
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

} // anonymous namespace

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
    ChangeSet m_change;
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

} // anonymous namespace

void ReformatPointerDeclaration::match(const CppQuickFixInterface &interface,
                                       QuickFixOperations &result)
{
    const QList<AST *> &path = interface->path();
    CppRefactoringFilePtr file = interface->currentFile();

    Overview overview = CppCodeStyleSettings::currentProjectCodeStyleOverview();
    overview.showArgumentNames = true;
    overview.showReturnTypes = true;

    const QTextCursor cursor = file->cursor();
    ChangeSet change;
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

    bool preVisit(AST *ast) {
        if (CaseStatementAST *cs = ast->asCaseStatement()) {
            foundCaseStatementLevel = true;
            if (ExpressionAST *expression = cs->expression->asIdExpression()) {
                QList<LookupItem> candidates = typeOfExpression(expression, document, scope);
                if (!candidates .isEmpty() && candidates.first().declaration()) {
                    Symbol *decl = candidates.first().declaration();
                    values << prettyPrint.prettyName(LookupContext::fullyQualifiedName(decl));
                }
            }
            return true;
        } else if (foundCaseStatementLevel) {
            return false;
        }
        return true;
    }

    Overview prettyPrint;
    bool foundCaseStatementLevel;
    QStringList values;
    TypeOfExpression typeOfExpression;
    Document::Ptr document;
    Scope *scope;
};

class CompleteSwitchCaseStatementOp: public CppQuickFixOperation
{
public:
    CompleteSwitchCaseStatementOp(const QSharedPointer<const CppQuickFixAssistInterface> &interface,
              int priority, CompoundStatementAST *compoundStatement, const QStringList &values)
        : CppQuickFixOperation(interface, priority)
        , compoundStatement(compoundStatement)
        , values(values)
    {
        setDescription(QApplication::translate("CppTools::QuickFix",
                                               "Complete Switch Statement"));
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());

        ChangeSet changes;
        int start = currentFile->endOf(compoundStatement->lbrace_token);
        changes.insert(start, QLatin1String("\ncase ")
                       + values.join(QLatin1String(":\nbreak;\ncase "))
                       + QLatin1String(":\nbreak;"));
        currentFile->setChangeSet(changes);
        currentFile->appendIndentRange(currentFile->range(compoundStatement));
        currentFile->apply();
    }

    CompoundStatementAST *compoundStatement;
    QStringList values;
};

Enum *findEnum(const QList<LookupItem> &results, const LookupContext &ctxt)
{
    foreach (const LookupItem &result, results) {
        const FullySpecifiedType fst = result.type();

        Type *type = result.declaration() ? result.declaration()->type().type()
                                          : fst.type();

        if (!type)
            continue;
        if (Enum *e = type->asEnumType())
            return e;
        if (const NamedType *namedType = type->asNamedType()) {
            const QList<LookupItem> candidates =
                    ctxt.lookup(namedType->name(), result.scope());
            return findEnum(candidates, ctxt);
        }
    }

    return 0;
}

Enum *conditionEnum(const CppQuickFixInterface &interface, SwitchStatementAST *statement)
{
    Block *block = statement->symbol;
    Scope *scope = interface->semanticInfo().doc->scopeAt(block->line(), block->column());
    TypeOfExpression typeOfExpression;
    typeOfExpression.init(interface->semanticInfo().doc, interface->snapshot());
    const QList<LookupItem> results = typeOfExpression(statement->condition,
                                                       interface->semanticInfo().doc,
                                                       scope);

    return findEnum(results, typeOfExpression.context());
}

} // anonymous namespace

void CompleteSwitchCaseStatement::match(const CppQuickFixInterface &interface,
                                        QuickFixOperations &result)
{
    const QList<AST *> &path = interface->path();

    if (path.isEmpty())
        return;

    // look for switch statement
    for (int depth = path.size() - 1; depth >= 0; --depth) {
        AST *ast = path.at(depth);
        SwitchStatementAST *switchStatement = ast->asSwitchStatement();
        if (switchStatement) {
            if (!interface->isCursorOn(switchStatement->switch_token) || !switchStatement->statement)
                return;
            CompoundStatementAST *compoundStatement = switchStatement->statement->asCompoundStatement();
            if (!compoundStatement) // we ignore pathologic case "switch (t) case A: ;"
                return;
            // look if the condition's type is an enum
            if (Enum *e = conditionEnum(interface, switchStatement)) {
                // check the possible enum values
                QStringList values;
                Overview prettyPrint;
                for (unsigned i = 0; i < e->memberCount(); ++i) {
                    if (Declaration *decl = e->memberAt(i)->asDeclaration())
                        values << prettyPrint.prettyName(LookupContext::fullyQualifiedName(decl));
                }
                // Get the used values
                Block *block = switchStatement->symbol;
                CaseStatementCollector caseValues(interface->semanticInfo().doc, interface->snapshot(),
                                                  interface->semanticInfo().doc->scopeAt(block->line(), block->column()));
                QStringList usedValues = caseValues(switchStatement);
                // save the values that would be added
                foreach (const QString &usedValue, usedValues)
                    values.removeAll(usedValue);
                if (!values.isEmpty())
                    result.append(CppQuickFixOperation::Ptr(new CompleteSwitchCaseStatementOp(interface, depth, compoundStatement, values)));
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
    InsertDeclOperation(const QSharedPointer<const CppQuickFixAssistInterface> &interface,
                        const QString &targetFileName, const Class *targetSymbol,
                        InsertionPointLocator::AccessSpec xsSpec, const QString &decl, int priority)
        : CppQuickFixOperation(interface, priority)
        , m_targetFileName(targetFileName)
        , m_targetSymbol(targetSymbol)
        , m_xsSpec(xsSpec)
        , m_decl(decl)
    {
        QString type;
        switch (xsSpec) {
        case InsertionPointLocator::Public: type = QLatin1String("public"); break;
        case InsertionPointLocator::Protected: type = QLatin1String("protected"); break;
        case InsertionPointLocator::Private: type = QLatin1String("private"); break;
        case InsertionPointLocator::PublicSlot: type = QLatin1String("public slot"); break;
        case InsertionPointLocator::ProtectedSlot: type = QLatin1String("protected slot"); break;
        case InsertionPointLocator::PrivateSlot: type = QLatin1String("private slot"); break;
        default: break;
        }

        setDescription(QCoreApplication::translate("CppEditor::InsertDeclOperation",
                                                   "Add %1 Declaration").arg(type));
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());

        InsertionPointLocator locator(refactoring);
        const InsertionLocation loc = locator.methodDeclarationInClass(
                    m_targetFileName, m_targetSymbol, m_xsSpec);
        QTC_ASSERT(loc.isValid(), return);

        CppRefactoringFilePtr targetFile = refactoring.file(m_targetFileName);
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
    QString m_targetFileName;
    const Class *m_targetSymbol;
    InsertionPointLocator::AccessSpec m_xsSpec;
    QString m_decl;
};

class DeclOperationFactory
{
public:
    DeclOperationFactory(const CppQuickFixInterface &interface, const QString &fileName,
                         const Class *matchingClass, const QString &decl)
        : m_interface(interface)
        , m_fileName(fileName)
        , m_matchingClass(matchingClass)
        , m_decl(decl)
    {}
    TextEditor::QuickFixOperation::Ptr
    operator()(InsertionPointLocator::AccessSpec xsSpec, int priority)
    {
        return TextEditor::QuickFixOperation::Ptr(
            new InsertDeclOperation(m_interface, m_fileName, m_matchingClass, xsSpec, m_decl,
                                    priority));
    }

private:
    const CppQuickFixInterface &m_interface;
    const QString &m_fileName;
    const Class *m_matchingClass;
    const QString &m_decl;
};

} // anonymous namespace

void InsertDeclFromDef::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface->path();
    CppRefactoringFilePtr file = interface->currentFile();

    FunctionDefinitionAST *funDef = 0;
    int idx = 0;
    for (; idx < path.size(); ++idx) {
        AST *node = path.at(idx);
        if (idx > 1) {
            if (DeclaratorIdAST *declId = node->asDeclaratorId()) {
                if (file->isCursorOn(declId)) {
                    if (FunctionDefinitionAST *candidate = path.at(idx - 2)->asFunctionDefinition()) {
                        if (funDef)
                            return;
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
    if (Class *matchingClass = isMemberFunction(interface->context(), fun)) {
        const QualifiedNameId *qName = fun->name()->asQualifiedNameId();
        for (Symbol *s = matchingClass->find(qName->identifier()); s; s = s->next()) {
            if (!s->name()
                    || !qName->identifier()->isEqualTo(s->identifier())
                    || !s->type()->isFunctionType())
                continue;

            if (s->type().isEqualTo(fun->type())) {
                // Declaration exists.
                return;
            }
        }
        QString fileName = QString::fromUtf8(matchingClass->fileName(),
                                             matchingClass->fileNameLength());
        const QString decl = InsertDeclOperation::generateDeclaration(fun);

        // Add several possible insertion locations for declaration
        DeclOperationFactory operation(interface, fileName, matchingClass, decl);

        result.append(operation(InsertionPointLocator::Public, 5));
        result.append(operation(InsertionPointLocator::PublicSlot, 4));
        result.append(operation(InsertionPointLocator::Protected, 3));
        result.append(operation(InsertionPointLocator::ProtectedSlot, 2));
        result.append(operation(InsertionPointLocator::Private, 1));
        result.append(operation(InsertionPointLocator::PrivateSlot, 0));
    }
}

QString InsertDeclOperation::generateDeclaration(const Function *function)
{
    Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
    oo.showFunctionSignatures = true;
    oo.showReturnTypes = true;
    oo.showArgumentNames = true;

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
    InsertDefOperation(const QSharedPointer<const CppQuickFixAssistInterface> &interface,
                       Declaration *decl, DeclaratorAST *declAST, const InsertionLocation &loc,
                       const DefPos defpos, const QString &targetFileName = QString(),
                       bool freeFunction = false)
        : CppQuickFixOperation(interface, 0)
        , m_decl(decl)
        , m_declAST(declAST)
        , m_loc(loc)
        , m_defpos(defpos)
        , m_targetFileName(targetFileName)
    {
        if (m_defpos == DefPosImplementationFile) {
            const QString declFile = QString::fromUtf8(decl->fileName(), decl->fileNameLength());
            const QDir dir = QFileInfo(declFile).dir();
            setDescription(QCoreApplication::translate("CppEditor::InsertDefOperation",
                                                       "Add Definition in %1")
                           .arg(dir.relativeFilePath(m_loc.isValid() ? m_loc.fileName()
                                                                     : m_targetFileName)));
        } else if (freeFunction) {
            setDescription(QCoreApplication::translate("CppEditor::InsertDefOperation",
                                                       "Add Definition Here"));
        } else if (m_defpos == DefPosInsideClass) {
            setDescription(QCoreApplication::translate("CppEditor::InsertDefOperation",
                                                       "Add Definition Inside Class"));
        } else if (m_defpos == DefPosOutsideClass) {
            setDescription(QCoreApplication::translate("CppEditor::InsertDefOperation",
                                                       "Add Definition Outside Class"));
        }
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        if (!m_loc.isValid())
            m_loc = insertLocationForMethodDefinition(m_decl, true, refactoring, m_targetFileName);
        QTC_ASSERT(m_loc.isValid(), return);

        CppRefactoringFilePtr targetFile = refactoring.file(m_loc.fileName());
        Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
        oo.showFunctionSignatures = true;
        oo.showReturnTypes = true;
        oo.showArgumentNames = true;

        if (m_defpos == DefPosInsideClass) {
            const int targetPos = targetFile->position(m_loc.line(), m_loc.column());
            ChangeSet target;
            target.replace(targetPos - 1, targetPos, QLatin1String("\n {\n\n}")); // replace ';'
            targetFile->setChangeSet(target);
            targetFile->appendIndentRange(ChangeSet::Range(targetPos, targetPos + 4));
            targetFile->setOpenEditor(true, targetPos);
            targetFile->apply();

            // Move cursor inside definition
            QTextCursor c = targetFile->cursor();
            c.setPosition(targetPos);
            c.movePosition(QTextCursor::Down);
            c.movePosition(QTextCursor::EndOfLine);
            assistInterface()->editor()->setTextCursor(c);
        } else {
            // make target lookup context
            Document::Ptr targetDoc = targetFile->cppDocument();
            Scope *targetScope = targetDoc->scopeAt(m_loc.line(), m_loc.column());
            LookupContext targetContext(targetDoc, assistInterface()->snapshot());
            ClassOrNamespace *targetCoN = targetContext.lookupType(targetScope);
            if (!targetCoN)
                targetCoN = targetContext.globalNamespace();

            // setup rewriting to get minimally qualified names
            SubstitutionEnvironment env;
            env.setContext(assistInterface()->context());
            env.switchScope(m_decl->enclosingScope());
            UseMinimalNames q(targetCoN);
            env.enter(&q);
            Control *control = assistInterface()->context().bindings()->control().data();

            // rewrite the function type
            const FullySpecifiedType tn = rewriteType(m_decl->type(), &env, control);

            // rewrite the function name
            if (nameIncludesOperatorName(m_decl->name())) {
                CppRefactoringFilePtr file = refactoring.file(fileName());
                const QString operatorNameText = file->textOf(m_declAST->core_declarator);
                oo.includeWhiteSpaceInOperatorName = operatorNameText.contains(QLatin1Char(' '));
            }
            const QString name = oo.prettyName(LookupContext::minimalName(m_decl, targetCoN,
                                                                          control));

            const QString defText = oo.prettyType(tn, name) + QLatin1String("\n{\n\n}");

            const int targetPos = targetFile->position(m_loc.line(), m_loc.column());
            const int targetPos2 = qMax(0, targetFile->position(m_loc.line(), 1) - 1);

            ChangeSet target;
            target.insert(targetPos,  m_loc.prefix() + defText + m_loc.suffix());
            targetFile->setChangeSet(target);
            targetFile->appendIndentRange(ChangeSet::Range(targetPos2, targetPos));
            targetFile->setOpenEditor(true, targetPos);
            targetFile->apply();

            // Move cursor inside definition
            QTextCursor c = targetFile->cursor();
            c.setPosition(targetPos);
            c.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor,
                           m_loc.prefix().count(QLatin1String("\n")) + 2);
            c.movePosition(QTextCursor::EndOfLine);
            if (m_defpos == DefPosImplementationFile) {
                if (BaseTextEditorWidget *editor = refactoring.editorForFile(m_loc.fileName()))
                    editor->setTextCursor(c);
            } else {
                assistInterface()->editor()->setTextCursor(c);
            }
        }
    }

private:
    Declaration *m_decl;
    DeclaratorAST *m_declAST;
    InsertionLocation m_loc;
    const DefPos m_defpos;
    const QString m_targetFileName;
};

} // anonymous namespace

void InsertDefFromDecl::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface->path();

    int idx = path.size() - 1;
    for (; idx >= 0; --idx) {
        AST *node = path.at(idx);
        if (SimpleDeclarationAST *simpleDecl = node->asSimpleDeclaration()) {
            if (idx > 0 && path.at(idx - 1)->asStatement())
                return;
            if (simpleDecl->symbols && ! simpleDecl->symbols->next) {
                if (Symbol *symbol = simpleDecl->symbols->value) {
                    if (Declaration *decl = symbol->asDeclaration()) {
                        if (Function *func = decl->type()->asFunctionType()) {
                            if (func->isSignal())
                                return;

                            // Check if there is already a definition
                            CppTools::SymbolFinder symbolFinder;
                            if (symbolFinder.findMatchingDefinition(decl, interface->snapshot(),
                                                                    true)) {
                                return;
                            }

                            // Insert Position: Implementation File
                            DeclaratorAST *declAST = simpleDecl->declarator_list->value;
                            InsertDefOperation *op = 0;
                            ProjectFile::Kind kind = ProjectFile::classify(interface->fileName());
                            const bool isHeaderFile = ProjectFile::isHeader(kind);
                            if (isHeaderFile) {
                                CppRefactoringChanges refactoring(interface->snapshot());
                                InsertionPointLocator locator(refactoring);
                                // find appropriate implementation file, but do not use this
                                // location, because insertLocationForMethodDefinition() should
                                // be used in perform() to get consistent insert positions.
                                foreach (const InsertionLocation &location,
                                         locator.methodDefinition(decl, false, QString())) {
                                    if (location.isValid()) {
                                        op = new InsertDefOperation(interface, decl, declAST,
                                                                    InsertionLocation(),
                                                                    DefPosImplementationFile,
                                                                    location.fileName());
                                        result.append(CppQuickFixOperation::Ptr(op));
                                        break;
                                    }
                                }
                            }

                            // Determine if we are dealing with a free function
                            const bool isFreeFunction = func->enclosingClass() == 0;

                            // Insert Position: Outside Class
                            if (!isFreeFunction) {
                                op = new InsertDefOperation(interface, decl, declAST,
                                                            InsertionLocation(), DefPosOutsideClass,
                                                            interface->fileName());
                                result.append(CppQuickFixOperation::Ptr(op));
                            }

                            // Insert Position: Inside Class
                            // Determine insert location direct after the declaration.
                            unsigned line, column;
                            const CppRefactoringFilePtr file = interface->currentFile();
                            file->lineAndColumn(file->endOf(simpleDecl), &line, &column);
                            const InsertionLocation loc
                                    = InsertionLocation(interface->fileName(), QString(), QString(),
                                                        line, column);
                            op = new InsertDefOperation(interface, decl, declAST, loc,
                                                        DefPosInsideClass, QString(),
                                                        isFreeFunction);
                            result.append(CppQuickFixOperation::Ptr(op));

                            return;
                        }
                    }
                }
            }
            break;
        }
    }
}

namespace {

class GenerateGetterSetterOperation : public CppQuickFixOperation
{
public:
    GenerateGetterSetterOperation(const QSharedPointer<const CppQuickFixAssistInterface> &interface)
        : CppQuickFixOperation(interface)
        , m_variableName(0)
        , m_declaratorId(0)
        , m_declarator(0)
        , m_variableDecl(0)
        , m_classSpecifier(0)
        , m_classDecl(0)
        , m_offerQuickFix(true)
    {
        setDescription(TextEditor::QuickFixFactory::tr("Create Getter and Setter Member Functions"));

        const QList<AST *> &path = interface->path();
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
        m_variableName = path.at(n - i++)->asSimpleName();
        m_declaratorId = path.at(n - i++)->asDeclaratorId();
        // DeclaratorAST might be preceded by PointerAST, e.g. for the case
        // "class C { char *@s; };", where '@' denotes the text cursor position.
        if (!(m_declarator = path.at(n - i++)->asDeclarator())) {
            --i;
            if (path.at(n - i++)->asPointer()) {
                if (n < 7)
                    return;
                m_declarator = path.at(n - i++)->asDeclarator();
            }
        }
        m_variableDecl = path.at(n - i++)->asSimpleDeclaration();
        m_classSpecifier = path.at(n - i++)->asClassSpecifier();
        m_classDecl = path.at(n - i++)->asSimpleDeclaration();

        if (!isValid())
            return;

        // Do not get triggered on member functions and arrays
        if (m_declarator->postfix_declarator_list) {
            m_offerQuickFix = false;
            return;
        }

        // Construct getter and setter names
        const Name *variableName = m_variableName->name;
        if (!variableName) {
            m_offerQuickFix = false;
            return;
        }
        const Identifier *variableId = variableName->identifier();
        if (!variableId) {
            m_offerQuickFix = false;
            return;
        }
        m_variableString = QString::fromLatin1(variableId->chars(), variableId->size());

        m_baseName = m_variableString;
        if (m_baseName.startsWith(QLatin1String("m_")))
            m_baseName.remove(0, 2);
        else if (m_baseName.startsWith(QLatin1Char('_')))
            m_baseName.remove(0, 1);
        else if (m_baseName.endsWith(QLatin1Char('_')))
            m_baseName.chop(1);

        m_getterName = m_baseName != m_variableString
            ? QString::fromLatin1("%1").arg(m_baseName)
            : QString::fromLatin1("get%1%2")
                .arg(m_baseName.left(1).toUpper()).arg(m_baseName.mid(1));
        m_setterName = QString::fromLatin1("set%1%2")
            .arg(m_baseName.left(1).toUpper()).arg(m_baseName.mid(1));

        // Check if the class has already a getter or setter.
        // This is only a simple check which should suffice not triggering the
        // same quick fix again. Limitations:
        //   1) It only checks in the current class, but not in base classes.
        //   2) It compares only names instead of types/signatures.
        if (Class *klass = m_classSpecifier->symbol) {
            for (unsigned i = 0; i < klass->memberCount(); ++i) {
                Symbol *symbol = klass->memberAt(i);
                if (const Name *symbolName = symbol->name()) {
                    if (const Identifier *id = symbolName->identifier()) {
                        const QString memberName = QString::fromLatin1(id->chars(), id->size());
                        if (memberName == m_getterName || memberName == m_setterName) {
                            m_offerQuickFix = false;
                            return;
                        }
                    }
                }
            } // for
        }
    }

    bool isValid() const
    {
        return m_variableName
            && m_declaratorId
            && m_declarator
            && m_variableDecl
            && m_classSpecifier
            && m_classDecl
            && m_offerQuickFix;
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());

        const List<Symbol *> *symbols = m_variableDecl->symbols;
        QTC_ASSERT(symbols, return);

        // Find the right symbol in the simple declaration
        Symbol *symbol = 0;
        for (; symbols; symbols = symbols->next) {
            Symbol *s = symbols->value;
            if (const Name *name = s->name()) {
                if (const Identifier *id = name->identifier()) {
                    const QString symbolName = QString::fromLatin1(id->chars(), id->size());
                    if (symbolName == m_variableString) {
                        symbol = s;
                        break;
                    }
                }
            }
        }

        QTC_ASSERT(symbol, return);
        FullySpecifiedType fullySpecifiedType = symbol->type();
        Type *type = fullySpecifiedType.type();
        QTC_ASSERT(type, return);
        Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
        oo.showFunctionSignatures = true;
        oo.showReturnTypes = true;
        oo.showArgumentNames = true;
        const QString typeString = oo.prettyType(fullySpecifiedType);

        const NameAST *classNameAST = m_classSpecifier->name;
        QTC_ASSERT(classNameAST, return);
        const Name *className = classNameAST->name;
        QTC_ASSERT(className, return);
        const Identifier *classId = className->identifier();
        QTC_ASSERT(classId, return);
        QString classString = QString::fromLatin1(classId->chars(), classId->size());

        bool wasHeader = true;
        QString declFileName = currentFile->fileName();
        QString implFileName = correspondingHeaderOrSource(declFileName, &wasHeader);
        const bool sameFile = !wasHeader || !QFile::exists(implFileName);
        if (sameFile)
            implFileName = declFileName;

        InsertionPointLocator locator(refactoring);
        InsertionLocation declLocation = locator.methodDeclarationInClass
            (declFileName, m_classSpecifier->symbol->asClass(), InsertionPointLocator::Public);

        const bool passByValue = type->isIntegerType() || type->isFloatType()
                || type->isPointerType() || type->isEnumType();
        const QString paramName = m_baseName != m_variableString
            ? m_baseName : QLatin1String("value");
        QString paramString;
        if (passByValue) {
            paramString = oo.prettyType(fullySpecifiedType, paramName);
        } else {
            FullySpecifiedType constParamType(fullySpecifiedType);
            constParamType.setConst(true);
            QScopedPointer<ReferenceType> referenceType(new ReferenceType(constParamType, false));
            FullySpecifiedType referenceToConstParamType(referenceType.data());
            paramString = oo.prettyType(referenceToConstParamType, paramName);
        }

        const bool isStatic = symbol->storage() == Symbol::Static;

        // Construct declaration strings
        QString declaration = declLocation.prefix();
        QString getterTypeString = typeString;
        FullySpecifiedType getterType(fullySpecifiedType);
        if (fullySpecifiedType.isConst()) {
            getterType.setConst(false);
            getterTypeString = oo.prettyType(getterType);
        }

        const QString declarationGetterTypeAndNameString = oo.prettyType(getterType, m_getterName);
        const QString declarationGetter = QString::fromLatin1("%1%2()%3;\n")
                .arg(isStatic ? QLatin1String("static ") : QString())
            .arg(declarationGetterTypeAndNameString)
            .arg(isStatic ? QString() : QLatin1String(" const"));
        const QString declarationSetter = QString::fromLatin1("%1void %2(%3);\n")
            .arg(isStatic ? QLatin1String("static ") : QString())
            .arg(m_setterName)
            .arg(paramString);

        declaration += declarationGetter;
        if (!fullySpecifiedType.isConst())
            declaration += declarationSetter;
        declaration += declLocation.suffix();

        // Construct implementation strings
        const QString implementationGetterTypeAndNameString = oo.prettyType(
            getterType, QString::fromLatin1("%1::%2").arg(classString, m_getterName));
        const QString implementationGetter = QString::fromLatin1(
                    "\n%1()%2\n"
                    "{\n"
                    "return %3;\n"
                    "}\n")
                .arg(implementationGetterTypeAndNameString)
                .arg(isStatic ? QString() : QLatin1String(" const"))
                .arg(m_variableString);
        const QString implementationSetter = QString::fromLatin1(
                    "\nvoid %1::%2(%3)\n"
                    "{\n"
                    "%4 = %5;\n"
                    "}\n")
                .arg(classString).arg(m_setterName)
                .arg(paramString).arg(m_variableString)
                .arg(paramName);
        QString implementation = implementationGetter;
        if (!fullySpecifiedType.isConst())
            implementation += implementationSetter;

        // Create and apply changes
        ChangeSet currChanges;
        int declInsertPos = currentFile->position(qMax(1u, declLocation.line()),
                                                  declLocation.column());
        currChanges.insert(declInsertPos, declaration);

        if (sameFile) {
            InsertionLocation loc = insertLocationForMethodDefinition(symbol, false, refactoring,
                                                                      currentFile->fileName());
            currChanges.insert(currentFile->position(loc.line(), loc.column()), implementation);
        } else {
            CppRefactoringChanges implRef(snapshot());
            CppRefactoringFilePtr implFile = implRef.file(implFileName);
            ChangeSet implChanges;
            InsertionLocation loc = insertLocationForMethodDefinition(symbol, false,
                                                                      implRef, implFileName);
            const int implInsertPos = implFile->position(loc.line(), loc.column());
            implChanges.insert(implFile->position(loc.line(), loc.column()), implementation);
            implFile->setChangeSet(implChanges);
            implFile->appendIndentRange(
                ChangeSet::Range(implInsertPos, implInsertPos + implementation.size()));
            implFile->apply();
        }
        currentFile->setChangeSet(currChanges);
        currentFile->appendIndentRange(
            ChangeSet::Range(declInsertPos, declInsertPos + declaration.size()));
        currentFile->apply();
    }

    SimpleNameAST *m_variableName;
    DeclaratorIdAST *m_declaratorId;
    DeclaratorAST *m_declarator;
    SimpleDeclarationAST *m_variableDecl;
    ClassSpecifierAST *m_classSpecifier;
    SimpleDeclarationAST *m_classDecl;

    QString m_baseName;
    QString m_getterName;
    QString m_setterName;
    QString m_variableString;
    bool m_offerQuickFix;
};

} // anonymous namespace

void GenerateGetterSetter::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    GenerateGetterSetterOperation *op = new GenerateGetterSetterOperation(interface);
    if (op->isValid())
        result.append(CppQuickFixOperation::Ptr(op));
    else
        delete op;
}

namespace {

class ExtractFunctionOperation : public CppQuickFixOperation
{
public:
    ExtractFunctionOperation(const CppQuickFixInterface &interface,
                    int extractionStart,
                    int extractionEnd,
                    FunctionDefinitionAST *refFuncDef,
                    Symbol *funcReturn,
                    QList<QPair<QString, QString> > relevantDecls)
        : CppQuickFixOperation(interface)
        , m_extractionStart(extractionStart)
        , m_extractionEnd(extractionEnd)
        , m_refFuncDef(refFuncDef)
        , m_funcReturn(funcReturn)
        , m_relevantDecls(relevantDecls)
    {
        setDescription(QCoreApplication::translate("QuickFix::ExtractFunction", "Extract Function"));
    }

    void perform()
    {
        QTC_ASSERT(!m_funcReturn || !m_relevantDecls.isEmpty(), return);
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());

        const QString &funcName = getFunctionName();
        if (funcName.isEmpty())
            return;

        Function *refFunc = m_refFuncDef->symbol;

        // We don't need to rewrite the type for declarations made inside the reference function,
        // since their scope will remain the same. Then we preserve the original spelling style.
        // However, we must do so for the return type in the definition.
        SubstitutionEnvironment env;
        env.setContext(assistInterface()->context());
        env.switchScope(refFunc);
        ClassOrNamespace *targetCoN =
                assistInterface()->context().lookupType(refFunc->enclosingScope());
        if (!targetCoN)
            targetCoN = assistInterface()->context().globalNamespace();
        UseMinimalNames subs(targetCoN);
        env.enter(&subs);

        Overview printer = CppCodeStyleSettings::currentProjectCodeStyleOverview();
        Control *control = assistInterface()->context().bindings()->control().data();
        QString funcDef;
        QString funcDecl; // We generate a declaration only in the case of a member function.
        QString funcCall;

        Class *matchingClass = isMemberFunction(assistInterface()->context(), refFunc);

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
            const Name *name = rewriteName(matchingClass->name(), &env, control);
            funcDef.append(printer.prettyName(name));
            funcDef.append(QLatin1String("::"));
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
        if (matchingClass)
            funcDecl.append(QLatin1String(";\n"));
        if (m_funcReturn) {
            funcDef.append(QLatin1String("\nreturn ")
                        + m_relevantDecls.at(0).first
                        + QLatin1String(";"));
            funcCall.prepend(m_relevantDecls.at(0).second + QLatin1String(" = "));
        }
        funcDef.append(QLatin1String("\n}\n\n"));
        funcDef.replace(QChar::ParagraphSeparator, QLatin1String("\n"));
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
        change.insert(position, funcDef);
        change.replace(m_extractionStart, m_extractionEnd, funcCall);
        currentFile->setChangeSet(change);
        currentFile->appendIndentRange(ChangeSet::Range(position, position + 1));
        currentFile->apply();

        QTextCursor tc = currentFile->document()->find(QLatin1String("{"), position);
        QTC_ASSERT(tc.hasSelection(), return);
        position = tc.selectionEnd() + 1;
        change.clear();
        change.insert(position, extract);
        currentFile->setChangeSet(change);
        currentFile->appendReindentRange(ChangeSet::Range(position, position + 1));
        currentFile->apply();

        // Write declaration, if necessary.
        if (matchingClass) {
            InsertionPointLocator locator(refactoring);
            const QString fileName = QLatin1String(matchingClass->fileName());
            const InsertionLocation &location =
                    locator.methodDeclarationInClass(fileName, matchingClass,
                                                     InsertionPointLocator::Public);
            CppRefactoringFilePtr declFile = refactoring.file(fileName);
            change.clear();
            position = declFile->position(location.line(), location.column());
            change.insert(position, funcDecl);
            declFile->setChangeSet(change);
            declFile->appendIndentRange(ChangeSet::Range(position, position + 1));
            declFile->apply();
        }
    }

    QString getFunctionName() const
    {
        bool ok;
        QString name =
                QInputDialog::getText(0,
                                      QCoreApplication::translate("QuickFix::ExtractFunction",
                                                                  "Extract Function Refactoring"),
                                      QCoreApplication::translate("QuickFix::ExtractFunction",
                                                                  "Enter function name"),
                                      QLineEdit::Normal,
                                      QString(),
                                      &ok);
        name = name.trimmed();
        if (!ok || name.isEmpty())
            return QString();

        if (!isValidIdentifier(name)) {
            QMessageBox::critical(0,
                                  QCoreApplication::translate("QuickFix::ExtractFunction",
                                                              "Extract Function Refactoring"),
                                  QCoreApplication::translate("QuickFix::ExtractFunction",
                                                              "Invalid function name"));
            return QString();
        }

        return name;
    }

    int m_extractionStart;
    int m_extractionEnd;
    FunctionDefinitionAST *m_refFuncDef;
    Symbol *m_funcReturn;
    QList<QPair<QString, QString> > m_relevantDecls;
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
            return qMakePair(name, completeDecl);
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

    bool preVisit(AST *)
    {
        if (m_done)
            return false;
        return true;
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

    bool visit(CaseStatementAST *stmt)
    {
        statement(stmt->statement);
        return false;
    }

    bool visit(CompoundStatementAST *stmt)
    {
        for (StatementListAST *it = stmt->statement_list; it; it = it->next) {
            statement(it->value);
            if (m_done)
                break;
        }
        return false;
    }

    bool visit(DoStatementAST *stmt)
    {
        statement(stmt->statement);
        return false;
    }

    bool visit(ForeachStatementAST *stmt)
    {
        statement(stmt->statement);
        return false;
    }

    bool visit(RangeBasedForStatementAST *stmt)
    {
        statement(stmt->statement);
        return false;
    }

    bool visit(ForStatementAST *stmt)
    {
        statement(stmt->initializer);
        if (!m_done)
            statement(stmt->statement);
        return false;
    }

    bool visit(IfStatementAST *stmt)
    {
        statement(stmt->statement);
        if (!m_done)
            statement(stmt->else_statement);
        return false;
    }

    bool visit(TryBlockStatementAST *stmt)
    {
        statement(stmt->statement);
        for (CatchClauseListAST *it = stmt->catch_clause_list; it; it = it->next) {
            statement(it->value);
            if (m_done)
                break;
        }
        return false;
    }

    bool visit(WhileStatementAST *stmt)
    {
        statement(stmt->statement);
        return false;
    }

    bool visit(DeclarationStatementAST *declStmt)
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

    bool visit(ReturnStatementAST *)
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

void ExtractFunction::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    CppRefactoringFilePtr file = interface->currentFile();

    QTextCursor cursor = file->cursor();
    if (!cursor.hasSelection())
        return;

    const QList<AST *> &path = interface->path();
    FunctionDefinitionAST *refFuncDef = 0; // The "reference" function, which we will extract from.
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
            || refFuncDef->symbol->enclosingScope()->isTemplate() /* TODO: Templates... */) {
        return;
    }

    // Adjust selection ends.
    int selStart = cursor.selectionStart();
    int selEnd = cursor.selectionEnd();
    if (selStart > selEnd)
        qSwap(selStart, selEnd);

    Overview printer;

    // Analyze the content to be extracted, which consists of determining the statements
    // which are complete and collecting the declarations seen.
    FunctionExtractionAnalyser analyser(interface->semanticInfo().doc->translationUnit(),
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
    Symbol *funcReturn = 0;
    QList<QPair<QString, QString> > relevantDecls;
    SemanticInfo::LocalUseIterator it(interface->semanticInfo().localUses);
    while (it.hasNext()) {
        it.next();

        bool usedBeforeExtraction = false;
        bool usedAfterExtraction = false;
        bool usedInsideExtraction = false;
        const QList<SemanticInfo::Use> &uses = it.value();
        foreach (const SemanticInfo::Use &use, uses) {
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
            relevantDecls.append(qMakePair(name, analyser.m_knownDecls.value(name)));
        }

        // We assume that the first use of a local corresponds to its declaration.
        if (usedInsideExtraction && usedAfterExtraction && !usedBeforeExtraction) {
            if (!funcReturn) {
                QTC_ASSERT(analyser.m_knownDecls.contains(name), return);
                // The return, if any, is stored as the first item in the list.
                relevantDecls.prepend(qMakePair(name, analyser.m_knownDecls.value(name)));
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
    result.append(CppQuickFixOperation::Ptr(new ExtractFunctionOperation(interface,
                                                     analyser.m_extractionStart,
                                                     analyser.m_extractionEnd,
                                                     refFuncDef,
                                                     funcReturn,
                                                     relevantDecls)));
}

namespace {

class InsertQtPropertyMembersOp: public CppQuickFixOperation
{
public:
    enum GenerateFlag {
        GenerateGetter = 1 << 0,
        GenerateSetter = 1 << 1,
        GenerateSignal = 1 << 2,
        GenerateStorage = 1 << 3
    };

    InsertQtPropertyMembersOp(const QSharedPointer<const CppQuickFixAssistInterface> &interface,
              int priority, QtPropertyDeclarationAST *declaration, Class *klass, int generateFlags,
              const QString &getterName, const QString &setterName, const QString &signalName,
              const QString &storageName)
        : CppQuickFixOperation(interface, priority)
        , m_declaration(declaration)
        , m_class(klass)
        , m_generateFlags(generateFlags)
        , m_getterName(getterName)
        , m_setterName(setterName)
        , m_signalName(signalName)
        , m_storageName(storageName)
    {
        setDescription(TextEditor::QuickFixFactory::tr("Generate Missing Q_PROPERTY Members..."));
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr file = refactoring.file(fileName());

        InsertionPointLocator locator(refactoring);
        ChangeSet declarations;

        const QString typeName = file->textOf(m_declaration->type_id);
        const QString propertyName = file->textOf(m_declaration->property_name);

        // getter declaration
        if (m_generateFlags & GenerateGetter) {
            const QString getterDeclaration = typeName + QLatin1Char(' ') + m_getterName +
                    QLatin1String("() const\n{\nreturn ") + m_storageName + QLatin1String(";\n}\n");
            InsertionLocation getterLoc = locator.methodDeclarationInClass(file->fileName(), m_class, InsertionPointLocator::Public);
            QTC_ASSERT(getterLoc.isValid(), return);
            insertAndIndent(file, &declarations, getterLoc, getterDeclaration);
        }

        // setter declaration
        if (m_generateFlags & GenerateSetter) {
            QString setterDeclaration;
            QTextStream setter(&setterDeclaration);
            setter << "void " << m_setterName << '(' << typeName << " arg)\n{\n";
            if (m_signalName.isEmpty()) {
                setter << m_storageName <<  " = arg;\n}\n";
            } else {
                setter << "if (" << m_storageName << " != arg) {\n" << m_storageName
                       << " = arg;\nemit " << m_signalName << "(arg);\n}\n}\n";
            }
            InsertionLocation setterLoc = locator.methodDeclarationInClass(file->fileName(), m_class, InsertionPointLocator::PublicSlot);
            QTC_ASSERT(setterLoc.isValid(), return);
            insertAndIndent(file, &declarations, setterLoc, setterDeclaration);
        }

        // signal declaration
        if (m_generateFlags & GenerateSignal) {
            const QString declaration = QLatin1String("void ") + m_signalName + QLatin1Char('(')
                                        + typeName + QLatin1String(" arg);\n");
            InsertionLocation loc = locator.methodDeclarationInClass(file->fileName(), m_class, InsertionPointLocator::Signals);
            QTC_ASSERT(loc.isValid(), return);
            insertAndIndent(file, &declarations, loc, declaration);
        }

        // storage
        if (m_generateFlags & GenerateStorage) {
            const QString storageDeclaration = typeName  + QLatin1String(" m_")
                                               + propertyName + QLatin1String(";\n");
            InsertionLocation storageLoc = locator.methodDeclarationInClass(file->fileName(), m_class, InsertionPointLocator::Private);
            QTC_ASSERT(storageLoc.isValid(), return);
            insertAndIndent(file, &declarations, storageLoc, storageDeclaration);
        }

        file->setChangeSet(declarations);
        file->apply();
    }

private:
    void insertAndIndent(const RefactoringFilePtr &file, ChangeSet *changeSet,
                         const InsertionLocation &loc, const QString &text)
    {
        int targetPosition1 = file->position(loc.line(), loc.column());
        int targetPosition2 = qMax(0, file->position(loc.line(), 1) - 1);
        changeSet->insert(targetPosition1, loc.prefix() + text + loc.suffix());
        file->appendIndentRange(ChangeSet::Range(targetPosition2, targetPosition1));
    }

    QtPropertyDeclarationAST *m_declaration;
    Class *m_class;
    int m_generateFlags;
    QString m_getterName;
    QString m_setterName;
    QString m_signalName;
    QString m_storageName;
};

} // anonymous namespace

void InsertQtPropertyMembers::match(const CppQuickFixInterface &interface,
    QuickFixOperations &result)
{
    const QList<AST *> &path = interface->path();

    if (path.isEmpty())
        return;

    AST * const ast = path.last();
    QtPropertyDeclarationAST *qtPropertyDeclaration = ast->asQtPropertyDeclaration();
    if (!qtPropertyDeclaration)
        return;

    ClassSpecifierAST *klass = 0;
    for (int i = path.size() - 2; i >= 0; --i) {
        klass = path.at(i)->asClassSpecifier();
        if (klass)
            break;
    }
    if (!klass)
        return;

    CppRefactoringFilePtr file = interface->currentFile();
    const QString propertyName = file->textOf(qtPropertyDeclaration->property_name);
    QString getterName;
    QString setterName;
    QString signalName;
    int generateFlags = 0;
    QtPropertyDeclarationItemListAST *it = qtPropertyDeclaration->property_declaration_item_list;
    for (; it; it = it->next) {
        const char *tokenString = file->tokenAt(it->value->item_name_token).spell();
        if (!qstrcmp(tokenString, "READ")) {
            getterName = file->textOf(it->value->expression);
            generateFlags |= InsertQtPropertyMembersOp::GenerateGetter;
        } else if (!qstrcmp(tokenString, "WRITE")) {
            setterName = file->textOf(it->value->expression);
            generateFlags |= InsertQtPropertyMembersOp::GenerateSetter;
        } else if (!qstrcmp(tokenString, "NOTIFY")) {
            signalName = file->textOf(it->value->expression);
            generateFlags |= InsertQtPropertyMembersOp::GenerateSignal;
        }
    }
    const QString storageName = QLatin1String("m_") +  propertyName;
    generateFlags |= InsertQtPropertyMembersOp::GenerateStorage;

    Class *c = klass->symbol;

    Overview overview;
    for (unsigned i = 0; i < c->memberCount(); ++i) {
        Symbol *member = c->memberAt(i);
        FullySpecifiedType type = member->type();
        if (member->asFunction() || (type.isValid() && type->asFunctionType())) {
            const QString name = overview.prettyName(member->name());
            if (name == getterName)
                generateFlags &= ~InsertQtPropertyMembersOp::GenerateGetter;
            else if (name == setterName)
                generateFlags &= ~InsertQtPropertyMembersOp::GenerateSetter;
            else if (name == signalName)
                generateFlags &= ~InsertQtPropertyMembersOp::GenerateSignal;
        } else if (member->asDeclaration()) {
            const QString name = overview.prettyName(member->name());
            if (name == storageName)
                generateFlags &= ~InsertQtPropertyMembersOp::GenerateStorage;
        }
    }

    if (getterName.isEmpty() && setterName.isEmpty() && signalName.isEmpty())
        return;

    result.append(QuickFixOperation::Ptr(
        new InsertQtPropertyMembersOp(interface, path.size() - 1, qtPropertyDeclaration, c,
            generateFlags, getterName, setterName, signalName, storageName)));
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

    void perform()
    {
        CPPEditorWidget *editor = assistInterface()->editor();
        QSharedPointer<FunctionDeclDefLink> link = editor->declDefLink();
        if (link == m_link)
            editor->applyDeclDefLinkChanges(/*don't jump*/false);
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
    QSharedPointer<FunctionDeclDefLink> link = interface->editor()->declDefLink();
    if (!link || !link->isMarkerVisible())
        return;

    QSharedPointer<ApplyDeclDefLinkOperation> op(new ApplyDeclDefLinkOperation(interface, link));
    op->setDescription(FunctionDeclDefLink::tr("Apply Function Signature Changes"));
    result += op;
}

namespace {

QString definitionSignature(const CppQuickFixAssistInterface *assist,
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
    const Name *name = func->name();
    if (nameIncludesOperatorName(name)) {
        CoreDeclaratorAST *coreDeclarator = functionDefinitionAST->declarator->core_declarator;
        const QString operatorNameText = baseFile->textOf(coreDeclarator);
        oo.includeWhiteSpaceInOperatorName = operatorNameText.contains(QLatin1Char(' '));
    }
    const QString nameText = oo.prettyName(LookupContext::minimalName(func, cppCoN, control));
    const FullySpecifiedType tn = rewriteType(func->type(), &env, control);

    return oo.prettyType(tn, nameText);
}

class MoveFuncDefOutsideOp : public CppQuickFixOperation
{
public:
    enum MoveType {
        MoveOutside,
        MoveToCppFile,
        MoveOutsideMemberToCppFile
    };

    MoveFuncDefOutsideOp(const QSharedPointer<const CppQuickFixAssistInterface> &interface,
                         MoveType type, FunctionDefinitionAST *funcDef, const QString cppFileName)
        : CppQuickFixOperation(interface, 0)
        , m_funcDef(funcDef)
        , m_type(type)
        , m_cppFileName(cppFileName)
        , m_func(funcDef->symbol)
        , m_headerFileName(QString::fromUtf8(m_func->fileName(), m_func->fileNameLength()))
    {
        if (m_type == MoveOutside) {
            setDescription(QCoreApplication::translate("CppEditor::QuickFix",
                                                       "Move Definition Outside Class"));
        } else {
            const QDir dir = QFileInfo(m_headerFileName).dir();
            setDescription(QCoreApplication::translate("CppEditor::QuickFix",
                                                       "Move Definition to %1")
                           .arg(dir.relativeFilePath(m_cppFileName)));
        }
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr fromFile = refactoring.file(m_headerFileName);
        CppRefactoringFilePtr toFile = (m_type == MoveOutside) ? fromFile
                                                               : refactoring.file(m_cppFileName);

        // Determine file, insert position and scope
        InsertionLocation l
                = insertLocationForMethodDefinition(m_func, false, refactoring, toFile->fileName());
        const QString prefix = l.prefix();
        const QString suffix = l.suffix();
        const int insertPos = toFile->position(l.line(), l.column());
        Scope *scopeAtInsertPos = toFile->cppDocument()->scopeAt(l.line(), l.column());

        // construct definition
        const QString funcDec = definitionSignature(assistInterface(), m_funcDef,
                                                    fromFile, toFile,
                                                    scopeAtInsertPos);
        QString funcDef = prefix + funcDec;
        const int startPosition = fromFile->endOf(m_funcDef->declarator);
        const int endPosition = fromFile->endOf(m_funcDef->function_body);
        funcDef += fromFile->textOf(startPosition, endPosition);
        funcDef += suffix;

        // insert definition at new position
        ChangeSet cppChanges;
        cppChanges.insert(insertPos, funcDef);
        toFile->setChangeSet(cppChanges);
        toFile->appendIndentRange(ChangeSet::Range(insertPos, insertPos + funcDef.size()));
        toFile->setOpenEditor(true, insertPos);
        toFile->apply();

        // remove definition from fromFile
        Utils::ChangeSet headerTarget;
        if (m_type == MoveOutsideMemberToCppFile) {
            headerTarget.remove(fromFile->range(m_funcDef));
        } else {
            QString textFuncDecl = fromFile->textOf(m_funcDef);
            textFuncDecl.truncate(startPosition - fromFile->startOf(m_funcDef));
            textFuncDecl = textFuncDecl.trimmed() + QLatin1String(";");
            headerTarget.replace(fromFile->range(m_funcDef), textFuncDecl);
        }
        fromFile->setChangeSet(headerTarget);
        fromFile->apply();
    }

private:
    FunctionDefinitionAST *m_funcDef;
    MoveType m_type;
    const QString m_cppFileName;
    Function *m_func;
    const QString m_headerFileName;
};

} // anonymous namespace

void MoveFuncDefOutside::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface->path();
    SimpleDeclarationAST *classAST = 0;
    FunctionDefinitionAST *funcAST = 0;
    bool moveOutsideMemberDefinition = false;

    const int pathSize = path.size();
    for (int idx = 1; idx < pathSize; ++idx) {
        if ((funcAST = path.at(idx)->asFunctionDefinition())) {
            // check cursor position
            if (idx != pathSize - 1  // Do not allow "void a() @ {..."
                    && funcAST->function_body
                    && !interface->isCursorOn(funcAST->function_body)) {
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
            }
            funcAST = 0;
        }
    }

    if (!funcAST)
        return;

    bool isHeaderFile = false;
    const QString cppFileName = correspondingHeaderOrSource(interface->fileName(), &isHeaderFile);

    if (isHeaderFile && !cppFileName.isEmpty())
        result.append(CppQuickFixOperation::Ptr(
                          new MoveFuncDefOutsideOp(interface, ((moveOutsideMemberDefinition) ?
                                                   MoveFuncDefOutsideOp::MoveOutsideMemberToCppFile
                                                   : MoveFuncDefOutsideOp::MoveToCppFile),
                                                   funcAST, cppFileName)));

    if (classAST)
        result.append(CppQuickFixOperation::Ptr(
                          new MoveFuncDefOutsideOp(interface, MoveFuncDefOutsideOp::MoveOutside,
                                                   funcAST, QLatin1String(""))));

    return;
}

namespace {

class MoveFuncDefToDeclOp : public CppQuickFixOperation
{
public:
    MoveFuncDefToDeclOp(const QSharedPointer<const CppQuickFixAssistInterface> &interface,
                        const QString fromFileName, const QString toFileName,
                        FunctionDefinitionAST *funcDef, const QString declText,
                        const ChangeSet::Range toRange)
        : CppQuickFixOperation(interface, 0)
        , m_fromFileName(fromFileName)
        , m_toFileName(toFileName)
        , m_funcAST(funcDef)
        , m_declarationText(declText)
        , m_toRange(toRange)
    {
        if (m_toFileName == m_fromFileName) {
            setDescription(QCoreApplication::translate("CppEditor::QuickFix",
                                                       "Move Definition to Class"));
        } else {
            const QDir dir = QFileInfo(m_fromFileName).dir();
            setDescription(QCoreApplication::translate("CppEditor::QuickFix",
                                                       "Move Definition to %1")
                           .arg(dir.relativeFilePath(m_toFileName)));
        }
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr fromFile = refactoring.file(m_fromFileName);
        CppRefactoringFilePtr toFile = refactoring.file(m_toFileName);
        ChangeSet::Range fromRange = fromFile->range(m_funcAST);

        const QString wholeFunctionText = m_declarationText
                + fromFile->textOf(fromFile->endOf(m_funcAST->declarator),
                                   fromFile->endOf(m_funcAST->function_body));

        // Replace declaration with function and delete old definition
        Utils::ChangeSet toTarget;
        toTarget.replace(m_toRange, wholeFunctionText);
        if (m_toFileName == m_fromFileName)
            toTarget.remove(fromRange);
        toFile->setChangeSet(toTarget);
        toFile->appendIndentRange(m_toRange);
        toFile->setOpenEditor(true, m_toRange.start);
        toFile->apply();
        if (m_toFileName != m_fromFileName) {
            Utils::ChangeSet fromTarget;
            fromTarget.remove(fromRange);
            fromFile->setChangeSet(fromTarget);
            fromFile->apply();
        }
    }

private:
    const QString m_fromFileName;
    const QString m_toFileName;
    FunctionDefinitionAST *m_funcAST;
    const QString m_declarationText;
    const ChangeSet::Range m_toRange;
};

Namespace *isNamespaceFunction(const LookupContext &context, Function *function)
{
    QTC_ASSERT(function, return 0);
    if (isMemberFunction(context, function))
        return 0;

    Scope *enclosingScope = function->enclosingScope();
    while (!(enclosingScope->isNamespace() || enclosingScope->isClass()))
        enclosingScope = enclosingScope->enclosingScope();
    QTC_ASSERT(enclosingScope != 0, return 0);

    const Name *functionName = function->name();
    if (!functionName)
        return 0; // anonymous function names are not valid c++

    // global namespace
    if (!functionName->isQualifiedNameId()) {
        foreach (Symbol *s, context.globalNamespace()->symbols()) {
            if (Namespace *matchingNamespace = s->asNamespace())
                return matchingNamespace;
        }
        return 0;
    }

    const QualifiedNameId *q = functionName->asQualifiedNameId();
    if (!q->base())
        return 0;

    if (ClassOrNamespace *binding = context.lookupType(q->base(), enclosingScope)) {
        foreach (Symbol *s, binding->symbols()) {
            if (Namespace *matchingNamespace = s->asNamespace())
                return matchingNamespace;
        }
    }

    return 0;
}

} // anonymous namespace

void MoveFuncDefToDecl::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface->path();
    FunctionDefinitionAST *funcAST = 0;

    const int pathSize = path.size();
    for (int idx = 1; idx < pathSize; ++idx) {
        if ((funcAST = path.at(idx)->asFunctionDefinition())) {
            if (path.at(idx - 1)->asClassSpecifier())
                return;

            // check cursor position
            if (idx != pathSize - 1  // Do not allow "void a() @ {..."
                    && funcAST->function_body
                    && !interface->isCursorOn(funcAST->function_body)) {
                break;
            }
            funcAST = 0;
        }
    }

    if (!funcAST || !funcAST->symbol)
        return;

    // Determine declaration (file, range, text);
    QString declFileName;
    ChangeSet::Range declRange;
    QString declText;

    Function *func = funcAST->symbol;
    if (Class *matchingClass = isMemberFunction(interface->context(), func)) {
        // Dealing with member functions
        const QualifiedNameId *qName = func->name()->asQualifiedNameId();
        for (Symbol *s = matchingClass->find(qName->identifier()); s; s = s->next()) {
            if (!s->name()
                    || !qName->identifier()->isEqualTo(s->identifier())
                    || !s->type()->isFunctionType()
                    || !s->type().isEqualTo(func->type())
                    || s->isFunction()) {
                continue;
            }

            declFileName = QString::fromUtf8(matchingClass->fileName(),
                                             matchingClass->fileNameLength());

            const CppRefactoringChanges refactoring(interface->snapshot());
            const CppRefactoringFilePtr declFile = refactoring.file(declFileName);
            ASTPath astPath(declFile->cppDocument());
            const QList<AST *> path = astPath(s->line(), s->column());
            for (int idx = 0; idx < path.size(); ++idx) {
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
    }
    else if (Namespace *matchingNamespace = isNamespaceFunction(interface->context(), func)) {
        // Dealing with free functions
        bool isHeaderFile = false;
        declFileName = correspondingHeaderOrSource(interface->fileName(), &isHeaderFile);
        if (isHeaderFile)
            return;

        const CppRefactoringChanges refactoring(interface->snapshot());
        const CppRefactoringFilePtr declFile = refactoring.file(declFileName);
        const LookupContext lc(declFile->cppDocument(), interface->snapshot());
        const QList<LookupItem> candidates = lc.lookup(func->name(), matchingNamespace);
        for (int i = 0; i < candidates.size(); ++i) {
            if (Symbol *s = candidates.at(i).declaration()) {
                if (s->asDeclaration()) {
                    ASTPath astPath(declFile->cppDocument());
                    const QList<AST *> path = astPath(s->line(), s->column());
                    for (int idx = 0; idx < path.size(); ++idx) {
                        AST *node = path.at(idx);
                        if (SimpleDeclarationAST *simpleDecl = node->asSimpleDeclaration()) {
                            declRange = declFile->range(simpleDecl);
                            declText = declFile->textOf(simpleDecl);
                            declText.remove(-1, 1); // remove ';' from declaration text
                            break;
                        }
                    }
                }
            }

            if (!declText.isEmpty())
                break;
        }
    }

    if (!declFileName.isEmpty() && !declText.isEmpty())
        result.append(QuickFixOperation::Ptr(new MoveFuncDefToDeclOp(interface,
                                                                     interface->fileName(),
                                                                     declFileName,
                                                                     funcAST, declText,
                                                                     declRange)));
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
    {
        setDescription(QApplication::translate("CppTools::QuickFix", "Assign to Local Variable"));
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr file = refactoring.file(assistInterface()->fileName());

        // Determine return type and new variable name
        TypeOfExpression typeOfExpression;
        typeOfExpression.init(assistInterface()->semanticInfo().doc, snapshot(),
                              assistInterface()->context().bindings());
        typeOfExpression.setExpandTemplates(true);
        Scope *scope = file->scopeAt(m_ast->firstToken());
        const QList<LookupItem> result = typeOfExpression(file->textOf(m_ast).toUtf8(),
                                                          scope, TypeOfExpression::Preprocess);

        if (!result.isEmpty()) {
            SubstitutionEnvironment env;
            env.setContext(assistInterface()->context());
            env.switchScope(result.first().scope());
            ClassOrNamespace *con = typeOfExpression.context().lookupType(scope);
            if (!con)
                con = typeOfExpression.context().globalNamespace();
            UseMinimalNames q(con);
            env.enter(&q);

            Control *control = assistInterface()->context().bindings()->control().data();
            FullySpecifiedType type = rewriteType(result.first().type(), &env, control);

            Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
            QString originalName = oo.prettyName(m_name);
            QString newName = originalName;
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

            const int nameLength = originalName.length();
            QString tempType = oo.prettyType(type, m_name);
            const QString insertString = tempType.replace(
                        tempType.length() - nameLength, nameLength, newName + QLatin1String(" = "));
            if (!tempType.isEmpty()) {
                ChangeSet changes;
                changes.insert(m_insertPos, insertString);
                file->setChangeSet(changes);
                file->apply();

                // move cursor to new variable name
                QTextCursor c = file->cursor();
                c.setPosition(m_insertPos + insertString.length() - newName.length() - 3);
                c.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
                assistInterface()->editor()->setTextCursor(c);
            }
        }
    }

private:
    const int m_insertPos;
    const AST *m_ast;
    const Name *m_name;
};

} // anonymous namespace

void AssignToLocalVariable::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface->path();
    AST *outerAST = 0;
    SimpleNameAST *nameAST = 0;
    SimpleNameAST *visibleNameAST = 0;

    for (int i = path.size() - 3; i >= 0; --i) {
        if (CallAST *callAST = path.at(i)->asCall()) {
            if (!interface->isCursorOn(callAST))
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
                if (path.at(a)->asBinaryExpression())
                    return;
                if (path.at(a)->asReturnStatement())
                    return;
                if (path.at(a)->asCall())
                    return;
            }

            if (MemberAccessAST *member = path.at(i + 1)->asMemberAccess()) { // member
                if (member->base_expression) {
                    if (IdExpressionAST *idex = member->base_expression->asIdExpression()) {
                        nameAST = idex->name->asSimpleName();
                        visibleNameAST = member->member_name->asSimpleName();
                    }
                }
            } else if (QualifiedNameAST *qname = path.at(i + 2)->asQualifiedName()) { // static or
                nameAST = qname->unqualified_name->asSimpleName();                    // func in ns
                visibleNameAST = nameAST;
            } else { // normal
                nameAST = path.at(i + 2)->asSimpleName();
                visibleNameAST = nameAST;
            }

            if (nameAST && visibleNameAST) {
                outerAST = callAST;
                break;
            }
        } else if (NewExpressionAST *newexp = path.at(i)->asNewExpression()) {
            if (!interface->isCursorOn(newexp))
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
                visibleNameAST = nameAST;
                outerAST = newexp;
                break;
            }
        }
    }

    if (outerAST && nameAST && visibleNameAST) {
        const CppRefactoringFilePtr file = interface->currentFile();
        QList<LookupItem> items;
        TypeOfExpression typeOfExpression;
        typeOfExpression.init(interface->semanticInfo().doc, interface->snapshot(),
                              interface->context().bindings());
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

        foreach (const LookupItem &item, items) {
            if (!item.declaration())
                continue;

            if (Function *func = item.declaration()->asFunction()) {
                if (func->isSignal() || func->returnType()->isVoidType())
                    return;
            } else if (Declaration *dec = item.declaration()->asDeclaration()) {
                if (Function *func = dec->type()->asFunctionType()) {
                    if (func->isSignal() || func->returnType()->isVoidType())
                      return;
                }
            }

            const Name *name = visibleNameAST->name;
            const int insertPos = interface->currentFile()->startOf(outerAST);
            result.append(CppQuickFixOperation::Ptr(
                              new AssignToLocalVariableOperation(interface, insertPos, outerAST,
                                                                 name)));
            return;
        }
    }
}

namespace {

class InsertVirtualMethodsOp : public CppQuickFixOperation
{
public:
    InsertVirtualMethodsOp(const QSharedPointer<const CppQuickFixAssistInterface> &interface,
                           InsertVirtualMethodsDialog *factory)
        : CppQuickFixOperation(interface, 0)
        , m_factory(factory)
        , m_classAST(0)
        , m_valid(false)
        , m_cppFileName(QString::null)
        , m_insertPosDecl(0)
        , m_insertPosOutside(0)
        , m_functionCount(0)
        , m_implementedFunctionCount(0)
    {
        setDescription(QCoreApplication::translate(
                           "CppEditor::QuickFix", "Insert Virtual Functions of Base Classes"));

        const QList<AST *> &path = interface->path();
        const int pathSize = path.size();
        if (pathSize < 2)
            return;

        // Determine if cursor is on a class or a base class
        if (SimpleNameAST *nameAST = path.at(pathSize - 1)->asSimpleName()) {
            if (!interface->isCursorOn(nameAST))
                return;

            if (!(m_classAST = path.at(pathSize - 2)->asClassSpecifier())) { // normal class
                int index = pathSize - 2;
                const BaseSpecifierAST *baseAST = path.at(index)->asBaseSpecifier();// simple bclass
                if (!baseAST) {
                    if (index > 0 && path.at(index)->asQualifiedName()) // namespaced base class
                        baseAST = path.at(--index)->asBaseSpecifier();
                }
                --index;
                if (baseAST && index >= 0)
                    m_classAST = path.at(index)->asClassSpecifier();
            }
        }
        if (!m_classAST || !m_classAST->base_clause_list)
            return;

        // Determine insert positions
        const int endOfClassAST = interface->currentFile()->endOf(m_classAST);
        m_insertPosDecl = endOfClassAST - 1; // Skip last "}"
        m_insertPosOutside = endOfClassAST + 1; // Step over ";"

        // Determine base classes
        QList<const Class *> baseClasses;
        QQueue<ClassOrNamespace *> baseClassQueue;
        QSet<ClassOrNamespace *> visitedBaseClasses;
        if (ClassOrNamespace *clazz = interface->context().lookupType(m_classAST->symbol))
            baseClassQueue.enqueue(clazz);
        while (!baseClassQueue.isEmpty()) {
            ClassOrNamespace *clazz = baseClassQueue.dequeue();
            visitedBaseClasses.insert(clazz);
            const QList<ClassOrNamespace *> bases = clazz->usings();
            foreach (ClassOrNamespace *baseClass, bases) {
                foreach (Symbol *symbol, baseClass->symbols()) {
                    Class *base = symbol->asClass();
                    if (base
                            && (clazz = interface->context().lookupType(symbol))
                            && !visitedBaseClasses.contains(clazz)
                            && !baseClasses.contains(base)) {
                        baseClasses << base;
                        baseClassQueue.enqueue(clazz);
                    }
                }
            }
        }

        // Determine virtual functions
        m_factory->classFunctionModel->clear();
        Overview printer = CppCodeStyleSettings::currentProjectCodeStyleOverview();
        printer.showFunctionSignatures = true;
        const TextEditor::FontSettings &fs =
                TextEditor::TextEditorSettings::instance()->fontSettings();
        const Format formatReimpFunc = fs.formatFor(C_DISABLED_CODE);
        foreach (const Class *clazz, baseClasses) {
            QStandardItem *itemBase = new QStandardItem(printer.prettyName(clazz->name()));
            itemBase->setData(false, InsertVirtualMethodsDialog::Implemented);
            itemBase->setData(qVariantFromValue((void *) clazz),
                              InsertVirtualMethodsDialog::ClassOrFunction);
            const QString baseClassName = printer.prettyName(clazz->name());
            const Qt::CheckState funcItemsCheckState = (baseClassName != QLatin1String("QObject")
                    && baseClassName != QLatin1String("QWidget")
                    && baseClassName != QLatin1String("QPaintDevice"))
                    ? Qt::Checked : Qt::Unchecked;
            for (Scope::iterator it = clazz->firstMember(); it != clazz->lastMember(); ++it) {
                if (const Function *func = (*it)->type()->asFunctionType()) {
                    if (!func->isVirtual())
                        continue;

                    // Filter virtual destructors
                    if (printer.prettyName(func->name()).startsWith(QLatin1Char('~')))
                        continue;

                    // Filter OQbject's
                    //   - virtual const QMetaObject *metaObject() const;
                    //   - virtual void *qt_metacast(const char *);
                    //   - virtual int qt_metacall(QMetaObject::Call, int, void **);
                    if (baseClassName == QLatin1String("QObject")) {
                        if (printer.prettyName(func->name()) == QLatin1String("metaObject"))
                            continue;
                        if (printer.prettyName(func->name()) == QLatin1String("qt_metacast"))
                            continue;
                        if (printer.prettyName(func->name()) == QLatin1String("qt_metacall"))
                            continue;
                    }

                    // Do not implement existing functions inside target class
                    bool funcExistsInClass = false;
                    const Name *funcName = func->name();
                    for (Symbol *symbol = m_classAST->symbol->find(funcName->identifier());
                         symbol; symbol = symbol->next()) {
                        if (!symbol->name()
                                || !funcName->identifier()->isEqualTo(symbol->identifier())) {
                            continue;
                        }
                        if (symbol->type().isEqualTo(func->type())) {
                            funcExistsInClass = true;
                            break;
                        }
                    }

                    // Do not show when reimplemented from an other class
                    bool funcReimplemented = false;
                    for (int i = baseClasses.count() - 1; i >= 0; --i) {
                        const Class *prevClass = baseClasses.at(i);
                        if (clazz == prevClass)
                            break; // reached current class

                        for (const Symbol *symbol = prevClass->find(funcName->identifier());
                             symbol; symbol = symbol->next()) {
                            if (!symbol->name()
                                    || !funcName->identifier()->isEqualTo(symbol->identifier())) {
                                continue;
                            }
                            if (symbol->type().isEqualTo(func->type())) {
                                funcReimplemented = true;
                                break;
                            }
                        }
                    }
                    if (funcReimplemented)
                        continue;

                    // Construct function item
                    const bool isPureVirtual = func->isPureVirtual();
                    QString itemName = printer.prettyType(func->type(), func->name());
                    if (isPureVirtual)
                        itemName += QLatin1String(" = 0");
                    const QString itemReturnTypeString = printer.prettyType(func->returnType());
                    QStandardItem *funcItem = new QStandardItem(
                                itemName + QLatin1String(" : ") + itemReturnTypeString);
                    if (!funcExistsInClass) {
                        funcItem->setCheckable(true);
                        funcItem->setCheckState(Qt::Checked);
                    } else {
                        funcItem->setEnabled(false);
                        funcItem->setData(formatReimpFunc.foreground(), Qt::ForegroundRole);
                        if (formatReimpFunc.background().isValid())
                            funcItem->setData(formatReimpFunc.background(), Qt::BackgroundRole);
                    }

                    funcItem->setData(qVariantFromValue((void *) func),
                                      InsertVirtualMethodsDialog::ClassOrFunction);
                    funcItem->setData(isPureVirtual, InsertVirtualMethodsDialog::PureVirtual);
                    funcItem->setData(acessSpec(*it), InsertVirtualMethodsDialog::AccessSpec);
                    funcItem->setData(funcExistsInClass, InsertVirtualMethodsDialog::Implemented);
                    funcItem->setCheckState(funcItemsCheckState);

                    itemBase->appendRow(funcItem);

                    // update internal counters
                    if (funcExistsInClass)
                        ++m_implementedFunctionCount;
                    else
                        ++m_functionCount;
                }
            }
            if (itemBase->hasChildren()) {
                for (int i = 0; i < itemBase->rowCount(); ++i) {
                    if (itemBase->child(i, 0)->isCheckable()) {
                        if (!itemBase->isCheckable()) {
                            itemBase->setCheckable(true);
                            itemBase->setTristate(true);
                            itemBase->setData(false, InsertVirtualMethodsDialog::Implemented);
                        }
                        if (itemBase->child(i, 0)->checkState() == Qt::Checked) {
                            itemBase->setCheckState(Qt::Checked);
                            break;
                        }
                    }
                }
                m_factory->classFunctionModel->invisibleRootItem()->appendRow(itemBase);
            }
        }
        if (!m_factory->classFunctionModel->invisibleRootItem()->hasChildren()
                || m_functionCount == 0) {
            return;
        }

        bool isHeaderFile = false;
        m_cppFileName = correspondingHeaderOrSource(interface->fileName(), &isHeaderFile);
        m_factory->setHasImplementationFile(isHeaderFile && !m_cppFileName.isEmpty());
        m_factory->setHasReimplementedFunctions(m_implementedFunctionCount != 0);

        m_valid = true;
    }

    bool isValid() const
    {
        return m_valid;
    }

    InsertionPointLocator::AccessSpec acessSpec(const Symbol *symbol)
    {
        const Function *func = symbol->type()->asFunctionType();
        if (!func)
            return InsertionPointLocator::Invalid;
        if (func->isSignal())
            return InsertionPointLocator::Signals;

        InsertionPointLocator::AccessSpec spec = InsertionPointLocator::Invalid;
        if (symbol->isPrivate())
            spec = InsertionPointLocator::Private;
        else if (symbol->isProtected())
            spec = InsertionPointLocator::Protected;
        else if (symbol->isPublic())
            spec = InsertionPointLocator::Public;
        else
            return InsertionPointLocator::Invalid;

        if (func->isSlot()) {
            switch (spec) {
            case InsertionPointLocator::Private:
                return InsertionPointLocator::PrivateSlot;
                break;
            case InsertionPointLocator::Protected:
                return InsertionPointLocator::ProtectedSlot;
                break;
            case InsertionPointLocator::Public:
                return InsertionPointLocator::PublicSlot;
                break;
            default:
                return spec;
                break;
            }
        }
        return spec;
    }

    void perform()
    {
        if (!m_factory->gather())
            return;

        Core::ICore::settings()->setValue(
                    QLatin1String("QuickFix/InsertVirtualMethods/insertKeywordVirtual"),
                    m_factory->insertKeywordVirtual());
        Core::ICore::settings()->setValue(
                    QLatin1String("QuickFix/InsertVirtualMethods/implementationMode"),
                    m_factory->implementationMode());
        Core::ICore::settings()->setValue(
                    QLatin1String("QuickFix/InsertVirtualMethods/hideReimplementedFunctions"),
                    m_factory->hideReimplementedFunctions());

        // Insert declarations (and definition if Inside-/OutsideClass)
        Overview printer = CppCodeStyleSettings::currentProjectCodeStyleOverview();
        printer.showFunctionSignatures = true;
        printer.showReturnTypes = true;
        printer.showArgumentNames = true;
        ChangeSet headerChangeSet;
        const CppRefactoringChanges refactoring(assistInterface()->snapshot());
        const QString filename = assistInterface()->currentFile()->fileName();
        const CppRefactoringFilePtr headerFile = refactoring.file(filename);
        const LookupContext targetContext(headerFile->cppDocument(), assistInterface()->snapshot());

        const Class *targetClass = m_classAST->symbol;
        ClassOrNamespace *targetCoN = targetContext.lookupType(targetClass->scope());
        if (!targetCoN)
            targetCoN = targetContext.globalNamespace();
        UseMinimalNames useMinimalNames(targetCoN);
        Control *control = assistInterface()->context().bindings()->control().data();
        for (int i = 0; i < m_factory->classFunctionModel->rowCount(); ++i) {
            const QStandardItem *parent =
                    m_factory->classFunctionModel->invisibleRootItem()->child(i, 0);
            if (!parent->isCheckable() || parent->checkState() == Qt::Unchecked)
                continue;
            const Class *clazz = (const Class *)
                    parent->data(InsertVirtualMethodsDialog::ClassOrFunction).value<void *>();

            // Add comment
            const QString comment = QLatin1String("\n// ") + printer.prettyName(clazz->name()) +
                    QLatin1String(" interface\n");
            headerChangeSet.insert(m_insertPosDecl, comment);

            // Insert Declarations (+ definitions)
            QString lastAccessSpecString;
            for (int j = 0; j < parent->rowCount(); ++j) {
                const QStandardItem *item = parent->child(j, 0);
                if (!item->isCheckable() || item->checkState() == Qt::Unchecked)
                    continue;
                const Function *func = (const Function *)
                        item->data(InsertVirtualMethodsDialog::ClassOrFunction).value<void *>();

                // Construct declaration
                // setup rewriting to get minimally qualified names
                SubstitutionEnvironment env;
                env.setContext(assistInterface()->context());
                env.switchScope(clazz->enclosingScope());
                env.enter(&useMinimalNames);

                QString declaration;
                const FullySpecifiedType tn = rewriteType(func->type(), &env, control);
                declaration += printer.prettyType(tn, func->unqualifiedName());

                if (m_factory->insertKeywordVirtual())
                    declaration = QLatin1String("virtual ") + declaration;
                if (m_factory->implementationMode() & InsertVirtualMethodsDialog::ModeInsideClass)
                    declaration += QLatin1String("\n{\n}\n");
                else
                    declaration += QLatin1String(";\n");

                const InsertionPointLocator::AccessSpec spec =
                        static_cast<InsertionPointLocator::AccessSpec>(
                            item->data(InsertVirtualMethodsDialog::AccessSpec).toInt());
                const QString accessSpecString = InsertionPointLocator::accessSpecToString(spec);
                if (accessSpecString != lastAccessSpecString) {
                    declaration = accessSpecString + declaration;
                    if (!lastAccessSpecString.isEmpty()) // separate if not direct after the comment
                        declaration = QLatin1String("\n") + declaration;
                    lastAccessSpecString = accessSpecString;
                }
                headerChangeSet.insert(m_insertPosDecl, declaration);

                // Insert definition outside class
                if (m_factory->implementationMode() & InsertVirtualMethodsDialog::ModeOutsideClass) {
                    const QString name = printer.prettyName(targetClass->name()) +
                            QLatin1String("::") + printer.prettyName(func->name());
                    const QString defText = printer.prettyType(tn, name) + QLatin1String("\n{\n}");
                    headerChangeSet.insert(m_insertPosOutside,  QLatin1String("\n\n") + defText);
                }
            }
        }

        // Write header file
        headerFile->setChangeSet(headerChangeSet);
        headerFile->appendIndentRange(ChangeSet::Range(m_insertPosDecl, m_insertPosDecl + 1));
        headerFile->setOpenEditor(true, m_insertPosDecl);
        headerFile->apply();

        // Insert in implementation file
        if (m_factory->implementationMode() & InsertVirtualMethodsDialog::ModeImplementationFile) {
            const Symbol *symbol = headerFile->cppDocument()->lastVisibleSymbolAt(
                        targetClass->line(), targetClass->column());
            if (!symbol)
                return;
            const Class *clazz = symbol->asClass();
            if (!clazz)
                return;

            CppRefactoringFilePtr implementationFile = refactoring.file(m_cppFileName);
            ChangeSet implementationChangeSet;
            const int insertPos = qMax(0, implementationFile->document()->characterCount() - 1);

            // make target lookup context
            Document::Ptr implementationDoc = implementationFile->cppDocument();
            unsigned line, column;
            implementationDoc->translationUnit()->getPosition(insertPos, &line, &column);
            Scope *targetScope = implementationDoc->scopeAt(line, column);
            const LookupContext targetContext(implementationDoc, assistInterface()->snapshot());
            ClassOrNamespace *targetCoN = targetContext.lookupType(targetScope);
            if (!targetCoN)
                targetCoN = targetContext.globalNamespace();

            // Loop through inserted declarations
            for (unsigned i = targetClass->memberCount(); i < clazz->memberCount(); ++i) {
                Declaration *decl = clazz->memberAt(i)->asDeclaration();
                if (!decl)
                    continue;

                // setup rewriting to get minimally qualified names
                SubstitutionEnvironment env;
                env.setContext(assistInterface()->context());
                env.switchScope(decl->enclosingScope());
                UseMinimalNames q(targetCoN);
                env.enter(&q);
                Control *control = assistInterface()->context().bindings()->control().data();

                // rewrite the function type and name + create definition
                const FullySpecifiedType type = rewriteType(decl->type(), &env, control);
                const QString name = printer.prettyName(
                            LookupContext::minimalName(decl, targetCoN, control));
                const QString defText = printer.prettyType(type, name) + QLatin1String("\n{\n}");

                implementationChangeSet.insert(insertPos,  QLatin1String("\n\n") + defText);
            }

            implementationFile->setChangeSet(implementationChangeSet);
            implementationFile->appendIndentRange(ChangeSet::Range(insertPos, insertPos + 1));
            implementationFile->apply();
        }
    }

private:
    InsertVirtualMethodsDialog *m_factory;
    const ClassSpecifierAST *m_classAST;
    bool m_valid;
    QString m_cppFileName;
    int m_insertPosDecl;
    int m_insertPosOutside;
    unsigned m_functionCount;
    unsigned m_implementedFunctionCount;
};

class InsertVirtualMethodsFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    InsertVirtualMethodsFilterModel(QObject *parent = 0)
        : QSortFilterProxyModel(parent)
        , m_hideReimplemented(false)
    {}

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
    {
        QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

        // Handle base class
        if (!sourceParent.isValid()) {
            // check if any child is valid
            if (!sourceModel()->hasChildren(index))
                return false;
            if (!m_hideReimplemented)
                return true;

            for (int i = 0; i < sourceModel()->rowCount(index); ++i) {
                const QModelIndex child = sourceModel()->index(i, 0, index);
                if (!child.data(InsertVirtualMethodsDialog::Implemented).toBool())
                    return true;
            }
            return false;
        }

        if (m_hideReimplemented)
            return !index.data(InsertVirtualMethodsDialog::Implemented).toBool();
        return true;
    }

    bool hideReimplemented() const
    {
        return m_hideReimplemented;
    }

    void setHideReimplementedFunctions(bool show)
    {
        m_hideReimplemented = show;
        invalidateFilter();
    }

private:
    bool m_hideReimplemented;
};

} // anonymous namespace

InsertVirtualMethodsDialog::InsertVirtualMethodsDialog(QWidget *parent)
    : QDialog(parent)
    , m_view(0)
    , m_hideReimplementedFunctions(0)
    , m_insertMode(0)
    , m_virtualKeyword(0)
    , m_buttons(0)
    , m_hasImplementationFile(false)
    , m_hasReimplementedFunctions(false)
    , m_implementationMode(ModeOnlyDeclarations)
    , m_insertKeywordVirtual(false)
    , classFunctionModel(new QStandardItemModel(this))
    , classFunctionFilterModel(new InsertVirtualMethodsFilterModel(this))
{
    classFunctionFilterModel->setSourceModel(classFunctionModel);
}

void InsertVirtualMethodsDialog::initGui()
{
    if (m_view)
        return;

    setWindowTitle(tr("Insert Virtual Functions"));
    QVBoxLayout *globalVerticalLayout = new QVBoxLayout;

    // View
    QGroupBox *groupBoxView = new QGroupBox(tr("&Functions to insert:"), this);
    QVBoxLayout *groupBoxViewLayout = new QVBoxLayout(groupBoxView);
    m_view = new QTreeView(this);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setHeaderHidden(true);
    groupBoxViewLayout->addWidget(m_view);
    m_hideReimplementedFunctions =
            new QCheckBox(tr("&Hide already implemented functions of current class"), this);
    groupBoxViewLayout->addWidget(m_hideReimplementedFunctions);

    // Insertion options
    QGroupBox *groupBoxImplementation = new QGroupBox(tr("&Insertion options:"), this);
    QVBoxLayout *groupBoxImplementationLayout = new QVBoxLayout(groupBoxImplementation);
    m_insertMode = new QComboBox(this);
    m_insertMode->addItem(tr("Insert only declarations"), ModeOnlyDeclarations);
    m_insertMode->addItem(tr("Insert definitions inside class"), ModeInsideClass);
    m_insertMode->addItem(tr("Insert definitions outside class"), ModeOutsideClass);
    m_insertMode->addItem(tr("Insert definitions in implementation file"), ModeImplementationFile);
    m_virtualKeyword = new QCheckBox(tr("&Add keyword 'virtual' to function declaration"), this);
    groupBoxImplementationLayout->addWidget(m_insertMode);
    groupBoxImplementationLayout->addWidget(m_virtualKeyword);
    groupBoxImplementationLayout->addStretch(99);

    // Bottom button box
    m_buttons = new QDialogButtonBox(this);
    m_buttons->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(m_buttons, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttons, SIGNAL(rejected()), this, SLOT(reject()));

    globalVerticalLayout->addWidget(groupBoxView, 9);
    globalVerticalLayout->addWidget(groupBoxImplementation, 0);
    globalVerticalLayout->addWidget(m_buttons, 0);
    setLayout(globalVerticalLayout);

    connect(classFunctionModel, SIGNAL(itemChanged(QStandardItem*)),
            this, SLOT(updateCheckBoxes(QStandardItem*)));
    connect(m_hideReimplementedFunctions, SIGNAL(toggled(bool)),
            this, SLOT(setHideReimplementedFunctions(bool)));
}

void InsertVirtualMethodsDialog::initData()
{
    m_insertKeywordVirtual = Core::ICore::settings()->value(
                QLatin1String("QuickFix/InsertVirtualMethods/insertKeywordVirtual"),
                false).toBool();
    m_implementationMode = static_cast<InsertVirtualMethodsDialog::ImplementationMode>(
                Core::ICore::settings()->value(
                    QLatin1String("QuickFix/InsertVirtualMethods/implementationMode"), 1).toInt());
    m_hideReimplementedFunctions->setChecked(
                Core::ICore::settings()->value(
                    QLatin1String("QuickFix/InsertVirtualMethods/hideReimplementedFunctions"),
                    false).toBool());

    m_view->setModel(classFunctionFilterModel);
    m_expansionStateNormal.clear();
    m_expansionStateReimp.clear();
    m_hideReimplementedFunctions->setVisible(m_hasReimplementedFunctions);
    m_virtualKeyword->setChecked(m_insertKeywordVirtual);
    m_insertMode->setCurrentIndex(m_insertMode->findData(m_implementationMode));

    setHideReimplementedFunctions(m_hideReimplementedFunctions->isChecked());

    if (m_hasImplementationFile) {
        if (m_insertMode->count() == 3) {
            m_insertMode->addItem(tr("Insert definitions in implementation file"),
                                  ModeImplementationFile);
        }
    } else {
        if (m_insertMode->count() == 4)
            m_insertMode->removeItem(3);
    }
}

bool InsertVirtualMethodsDialog::gather()
{
    initGui();
    initData();

    // Expand the dialog a little bit
    adjustSize();
    resize(size() * 1.5);

    QPointer<InsertVirtualMethodsDialog> that(this);
    const int ret = exec();
    if (!that)
        return false;

    m_implementationMode = implementationMode();
    m_insertKeywordVirtual = insertKeywordVirtual();
    return (ret == QDialog::Accepted);
}

InsertVirtualMethodsDialog::ImplementationMode
InsertVirtualMethodsDialog::implementationMode() const
{
    return static_cast<InsertVirtualMethodsDialog::ImplementationMode>(
                m_insertMode->itemData(m_insertMode->currentIndex()).toInt());
}

void InsertVirtualMethodsDialog::setImplementationsMode(InsertVirtualMethodsDialog::ImplementationMode mode)
{
    m_implementationMode = mode;
}

bool InsertVirtualMethodsDialog::insertKeywordVirtual() const
{
    return m_virtualKeyword->isChecked();
}

void InsertVirtualMethodsDialog::setInsertKeywordVirtual(bool insert)
{
    m_insertKeywordVirtual = insert;
}

void InsertVirtualMethodsDialog::setHasImplementationFile(bool file)
{
    m_hasImplementationFile = file;
}

void InsertVirtualMethodsDialog::setHasReimplementedFunctions(bool functions)
{
    m_hasReimplementedFunctions = functions;
}

bool InsertVirtualMethodsDialog::hideReimplementedFunctions() const
{
    // Safty check necessary because of testing class
    return (m_hideReimplementedFunctions && m_hideReimplementedFunctions->isChecked());
}

void InsertVirtualMethodsDialog::updateCheckBoxes(QStandardItem *item)
{
    if (item->hasChildren()) {
        const Qt::CheckState state = item->checkState();
        if (!item->isCheckable() || state == Qt::PartiallyChecked)
            return;
        for (int i = 0; i < item->rowCount(); ++i) {
            if (item->child(i, 0)->isCheckable())
                item->child(i, 0)->setCheckState(state);
        }
    } else {
        QStandardItem *parent = item->parent();
        if (!parent->isCheckable())
            return;
        const Qt::CheckState state = item->checkState();
        for (int i = 0; i < parent->rowCount(); ++i) {
            if (state != parent->child(i, 0)->checkState()) {
                parent->setCheckState(Qt::PartiallyChecked);
                return;
            }
        }
        parent->setCheckState(state);
    }
}

void InsertVirtualMethodsDialog::setHideReimplementedFunctions(bool hide)
{
    InsertVirtualMethodsFilterModel *model =
            qobject_cast<InsertVirtualMethodsFilterModel *>(classFunctionFilterModel);

    if (m_expansionStateNormal.isEmpty() && m_expansionStateReimp.isEmpty()) {
        model->setHideReimplementedFunctions(hide);
        m_view->expandAll();
        saveExpansionState();
        return;
    }

    if (model->hideReimplemented() == hide)
        return;

    saveExpansionState();
    model->setHideReimplementedFunctions(hide);
    restoreExpansionState();
}

void InsertVirtualMethodsDialog::saveExpansionState()
{
    InsertVirtualMethodsFilterModel *model =
            qobject_cast<InsertVirtualMethodsFilterModel *>(classFunctionFilterModel);

    QList<bool> &state = model->hideReimplemented() ? m_expansionStateReimp
                                                    : m_expansionStateNormal;
    state.clear();
    for (int i = 0; i < model->rowCount(); ++i)
        state << m_view->isExpanded(model->index(i, 0));
}

void InsertVirtualMethodsDialog::restoreExpansionState()
{
    InsertVirtualMethodsFilterModel *model =
            qobject_cast<InsertVirtualMethodsFilterModel *>(classFunctionFilterModel);

    const QList<bool> &state = model->hideReimplemented() ? m_expansionStateReimp
                                                          : m_expansionStateNormal;
    const int stateCount = state.count();
    for (int i = 0; i < model->rowCount(); ++i) {
        if (i < stateCount && !state.at(i)) {
            m_view->collapse(model->index(i, 0));
            continue;
        }
        m_view->expand(model->index(i, 0));
    }
}

InsertVirtualMethods::InsertVirtualMethods(InsertVirtualMethodsDialog *dialog)
    : m_dialog(dialog)
{}

InsertVirtualMethods::~InsertVirtualMethods()
{
    if (m_dialog)
        m_dialog->deleteLater();
}

void InsertVirtualMethods::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    InsertVirtualMethodsOp *op = new InsertVirtualMethodsOp(interface, m_dialog);
    if (op->isValid())
        result.append(QuickFixOperation::Ptr(op));
    else
        delete op;
}

#include "cppquickfixes.moc"
