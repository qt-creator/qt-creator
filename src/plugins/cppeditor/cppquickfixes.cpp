/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppquickfixes.h"

#include "cppeditor.h"
#include "cppeditordocument.h"
#include "cppfunctiondecldeflink.h"
#include "cppquickfixassistant.h"
#include "cppvirtualfunctionassistprovider.h"
#include "cppinsertvirtualmethods.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

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
#include <cplusplus/TypeOfExpression.h>
#include <cplusplus/TypePrettyPrinter.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/qtcassert.h>

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QInputDialog>
#include <QSharedPointer>
#include <QStack>
#include <QTextCursor>
#include <QTextCodec>

#include <cctype>

using namespace CPlusPlus;
using namespace CppTools;
using namespace TextEditor;
using Utils::ChangeSet;

namespace CppEditor {
namespace Internal {

void registerQuickFixes(ExtensionSystem::IPlugin *plugIn)
{
    plugIn->addAutoReleasedObject(new AddIncludeForUndefinedIdentifier);

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
    plugIn->addAutoReleasedObject(new ConvertQt4Connect);

    plugIn->addAutoReleasedObject(new ApplyDeclDefLinkChanges);
    plugIn->addAutoReleasedObject(new ConvertFromAndToPointer);
    plugIn->addAutoReleasedObject(new ExtractFunction);
    plugIn->addAutoReleasedObject(new ExtractLiteralAsParameter);
    plugIn->addAutoReleasedObject(new GenerateGetterSetter);
    plugIn->addAutoReleasedObject(new InsertDeclFromDef);
    plugIn->addAutoReleasedObject(new InsertDefFromDecl);

    plugIn->addAutoReleasedObject(new MoveFuncDefOutside);
    plugIn->addAutoReleasedObject(new MoveAllFuncDefOutside);
    plugIn->addAutoReleasedObject(new MoveFuncDefToDecl);

    plugIn->addAutoReleasedObject(new AssignToLocalVariable);

    plugIn->addAutoReleasedObject(new InsertVirtualMethods);

    plugIn->addAutoReleasedObject(new OptimizeForLoop);

    plugIn->addAutoReleasedObject(new EscapeStringLiteral);
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
        if (location.isValid() && location.fileName() == fileName)
            return location;
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
    while (!(enclosingScope->isNamespace() || enclosingScope->isClass()))
        enclosingScope = enclosingScope->enclosingScope();
    QTC_ASSERT(enclosingScope != 0, return 0);

    const Name *functionName = function->name();
    if (!functionName)
        return 0;

    if (!functionName->isQualifiedNameId())
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
        return 0;

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
    return name->isOperatorNameId()
        || (name->isQualifiedNameId() && name->asQualifiedNameId()->name()->isOperatorNameId());
}

QString memberBaseName(const QString &name)
{
    QString baseName = name;

    // Remove leading and trailing "_"
    while (baseName.startsWith(QLatin1Char('_')))
        baseName.remove(0, 1);
    while (baseName.endsWith(QLatin1Char('_')))
        baseName.chop(1);
    if (baseName != name)
        return baseName;

    // If no leading/trailing "_": remove "m_" and "m" prefix
    if (baseName.startsWith(QLatin1String("m_"))) {
        baseName.remove(0, 2);
    } else if (baseName.startsWith(QLatin1Char('m')) && baseName.length() > 1
               && baseName.at(1).isUpper()) {
        baseName.remove(0, 1);
        baseName[0] = baseName.at(0).toLower();
    }

    return baseName;
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
            nested = interface.path()[priority - 1]->asNestedExpression();

        // check for ! before parentheses
        if (nested && priority - 2 >= 0) {
            negation = interface.path()[priority - 2]->asUnaryExpression();
            if (negation
                    && !interface.currentFile()->tokenAt(negation->unary_op_token).is(T_EXCLAIM)) {
                negation = 0;
            }
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
    CppRefactoringFilePtr file = interface.currentFile();

    const QList<AST *> &path = interface.path();
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

    result.append(new InverseLogicalComparisonOp(interface, index, binary, invertToken));
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

    result.append(new FlipLogicalOperandsOp(interface, index, binary, replacement));
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
    if (!declaration->semicolon_token)
        return false;

    if (!declaration->decl_specifier_list)
        return false;

    for (SpecifierListAST *it = declaration->decl_specifier_list; it; it = it->next) {
        SpecifierAST *specifier = it->value;

        if (specifier->asEnumSpecifier() != 0)
            return false;

        else if (specifier->asClassSpecifier() != 0)
            return false;
    }

    if (!declaration->declarator_list)
        return false;

    else if (!declaration->declarator_list->next)
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
    const QList<AST *> &path = interface.path();
    CppRefactoringFilePtr file = interface.currentFile();
    const int cursorPosition = file->cursor().selectionStart();

    for (int index = path.size() - 1; index != -1; --index) {
        AST *node = path.at(index);

        if (CoreDeclaratorAST *coreDecl = node->asCoreDeclarator()) {
            core_declarator = coreDecl;
        } else if (SimpleDeclarationAST *simpleDecl = node->asSimpleDeclaration()) {
            if (checkDeclaration(simpleDecl)) {
                SimpleDeclarationAST *declaration = simpleDecl;

                const int startOfDeclSpecifier = file->startOf(declaration->decl_specifier_list->firstToken());
                const int endOfDeclSpecifier = file->endOf(declaration->decl_specifier_list->lastToken() - 1);

                if (cursorPosition >= startOfDeclSpecifier && cursorPosition <= endOfDeclSpecifier) {
                    // the AST node under cursor is a specifier.
                    result.append(new SplitSimpleDeclarationOp(interface, index, declaration));
                    return;
                }

                if (core_declarator && interface.isCursorOn(core_declarator)) {
                    // got a core-declarator under the text cursor.
                    result.append(new SplitSimpleDeclarationOp(interface, index, declaration));
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
    const QList<AST *> &path = interface.path();

    // show when we're on the 'if' of an if statement
    int index = path.size() - 1;
    IfStatementAST *ifStatement = path.at(index)->asIfStatement();
    if (ifStatement && interface.isCursorOn(ifStatement->if_token) && ifStatement->statement
        && !ifStatement->statement->asCompoundStatement()) {
        result.append(new AddBracesToIfOp(interface, index, ifStatement->statement));
        return;
    }

    // or if we're on the statement contained in the if
    // ### This may not be such a good idea, consider nested ifs...
    for (; index != -1; --index) {
        IfStatementAST *ifStatement = path.at(index)->asIfStatement();
        if (ifStatement && ifStatement->statement
            && interface.isCursorOn(ifStatement->statement)
            && !ifStatement->statement->asCompoundStatement()) {
            result.append(new AddBracesToIfOp(interface, index, ifStatement->statement));
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

        reset();
    }

    void reset()
    {
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
    const QList<AST *> &path = interface.path();
    typedef QSharedPointer<MoveDeclarationOutOfIfOp> Ptr;
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
        setDescription(QApplication::translate("CppTools::QuickFix",
                                               "Move Declaration out of Condition"));
        reset();
    }

    void reset()
    {
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
            result.append(new SplitIfStatementOp(interface, index, pattern, condition));
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
    Type type = TypeNone;
    QByteArray enclosingFunction;
    const QList<AST *> &path = interface.path();
    CppRefactoringFilePtr file = interface.currentFile();
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
        result.append(new WrapStringLiteralOp(interface, priority, actions, description, literal));
        if (NumericLiteralAST *charLiteral = literal->asNumericLiteral()) {
            const QByteArray contents(file->tokenAt(charLiteral->literal_token).identifier->chars());
            if (!charToStringEscapeSequences(contents).isEmpty()) {
                actions = DoubleQuoteAction | ConvertEscapeSequencesToStringAction;
                description = QApplication::translate("CppTools::QuickFix",
                              "Convert to String Literal");
                result.append(new WrapStringLiteralOp(interface, priority, actions,
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
                result.append(new WrapStringLiteralOp(interface, priority, actions,
                                                      description, literal));
                actions &= ~EncloseInQLatin1CharAction;
                description = QApplication::translate("CppTools::QuickFix",
                              "Convert to Character Literal");
                result.append(new WrapStringLiteralOp(interface, priority, actions,
                                                      description, literal));
            }
        }
        actions = EncloseInQLatin1StringAction | objectiveCActions;
        result.append(new WrapStringLiteralOp(interface, priority, actions,
                          msgQtStringLiteralDescription(replacement(actions), 4), literal));
        actions = EncloseInQStringLiteralAction | objectiveCActions;
        result.append(new WrapStringLiteralOp(interface, priority, actions,
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
    const QList<AST *> &path = interface.path();
    CppRefactoringFilePtr file = interface.currentFile();
    ExpressionAST *literal = WrapStringLiteral::analyze(path, file, &type, &enclosingFunction);
    if (!literal || type != WrapStringLiteral::TypeString
       || isQtStringLiteral(enclosingFunction) || isQtStringTranslation(enclosingFunction))
        return;

    QString trContext;

    QSharedPointer<Control> control = interface.context().bindings()->control();
    const Name *trName = control->identifier("tr");

    // Check whether we are in a function:
    const QString description = QApplication::translate("CppTools::QuickFix", "Mark as Translatable");
    for (int i = path.size() - 1; i >= 0; --i) {
        if (FunctionDefinitionAST *definition = path.at(i)->asFunctionDefinition()) {
            Function *function = definition->symbol;
            ClassOrNamespace *b = interface.context().lookupType(function);
            if (b) {
                // Do we have a tr function?
                foreach (const LookupItem &r, b->find(trName)) {
                    Symbol *s = r.declaration();
                    if (s->type()->isFunctionType()) {
                        // no context required for tr
                        result.append(new WrapStringLiteralOp(interface, path.size() - 1,
                                                              WrapStringLiteral::TranslateTrAction,
                                                              description, literal));
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
            result.append(new WrapStringLiteralOp(interface, path.size() - 1,
                                                  WrapStringLiteral::TranslateQCoreApplicationAction,
                                                  description, literal, trContext));
            return;
        }
    }

    // We need to use Q_TRANSLATE_NOOP
    result.append(new WrapStringLiteralOp(interface, path.size() - 1,
                                          WrapStringLiteral::TranslateNoopAction,
                                          description, literal, trContext));
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
    CppRefactoringFilePtr file = interface.currentFile();

    if (!interface.editor()->cppEditorDocument()->isObjCEnabled())
        return;

    WrapStringLiteral::Type type = WrapStringLiteral::TypeNone;
    QByteArray enclosingFunction;
    CallAST *qlatin1Call;
    const QList<AST *> &path = interface.path();
    ExpressionAST *literal = WrapStringLiteral::analyze(path, file, &type, &enclosingFunction,
                                                        &qlatin1Call);
    if (!literal || type != WrapStringLiteral::TypeString)
        return;
    if (!isQtStringLiteral(enclosingFunction))
        qlatin1Call = 0;

    result.append(new ConvertCStringToNSStringOp(interface, path.size() - 1, literal->asStringLiteral(),
                      qlatin1Call));
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
        auto op = new ConvertNumericLiteralOp(interface, start, start + numberLength, replacement);
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
            auto op = new ConvertNumericLiteralOp(interface, start, start + numberLength, replacement);
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
            auto op = new ConvertNumericLiteralOp(interface, start, start + numberLength, replacement);
            op->setDescription(QApplication::translate("CppTools::QuickFix", "Convert to Decimal"));
            op->setPriority(priority);
            result.append(op);
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
        typeOfExpression.init(semanticInfo().doc, snapshot(), context().bindings());
        Scope *scope = currentFile->scopeAt(binaryAST->firstToken());
        const QList<LookupItem> result =
                typeOfExpression(currentFile->textOf(binaryAST->right_expression).toUtf8(),
                                 scope,
                                 TypeOfExpression::Preprocess);

        if (!result.isEmpty()) {
            SubstitutionEnvironment env;
            env.setContext(context());
            env.switchScope(result.first().scope());
            ClassOrNamespace *con = typeOfExpression.context().lookupType(scope);
            if (!con)
                con = typeOfExpression.context().globalNamespace();
            UseMinimalNames q(con);
            env.enter(&q);

            Control *control = context().bindings()->control().data();
            FullySpecifiedType tn = rewriteType(result.first().type(), &env, control);

            Overview oo = CppCodeStyleSettings::currentProjectCodeStyleOverview();
            QString ty = oo.prettyType(tn, simpleNameAST->name);
            if (!ty.isEmpty()) {
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
    const QList<AST *> &path = interface.path();
    CppRefactoringFilePtr file = interface.currentFile();

    for (int index = path.size() - 1; index != -1; --index) {
        if (BinaryExpressionAST *binary = path.at(index)->asBinaryExpression()) {
            if (binary->left_expression && binary->right_expression
                    && file->tokenAt(binary->binary_op_token).is(T_EQUAL)) {
                IdExpressionAST *idExpr = binary->left_expression->asIdExpression();
                if (interface.isCursorOn(binary->left_expression) && idExpr
                        && idExpr->name->asSimpleName() != 0) {
                    SimpleNameAST *nameAST = idExpr->name->asSimpleName();
                    const QList<LookupItem> results = interface.context().lookup(nameAST->name, file->scopeAt(nameAST->firstToken()));
                    Declaration *decl = 0;
                    foreach (const LookupItem &r, results) {
                        if (!r.declaration())
                            continue;
                        if (Declaration *d = r.declaration()->asDeclaration()) {
                            if (!d->type()->isFunctionType()) {
                                decl = d;
                                break;
                            }
                        }
                    }

                    if (!decl) {
                        result.append(new AddLocalDeclarationOp(interface, index, binary, nameAST));
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
        editor()->renameUsages(m_name);
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
    const QList<AST *> &path = interface.path();

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
            result.append(new ConvertToCamelCaseOp(interface, path.size() - 1, newName));
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

    insertNewIncludeDirective(m_include, file, semanticInfo().doc);
}

namespace {

QString findShortestInclude(const QString currentDocumentFilePath,
                            const QString candidateFilePath,
                            const ProjectPart::HeaderPaths &headerPaths)
{
    QString result;

    const QFileInfo fileInfo(candidateFilePath);

    if (fileInfo.path() == QFileInfo(currentDocumentFilePath).path()) {
        result = QLatin1Char('"') + fileInfo.fileName() + QLatin1Char('"');
    } else {
        foreach (const ProjectPart::HeaderPath &headerPath, headerPaths) {
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

QString findQtIncludeWithSameName(const QString &className,
                                  const ProjectPart::HeaderPaths &headerPaths)
{
    QString result;

    // Check for a header file with the same name in the Qt include paths
    foreach (const ProjectPart::HeaderPath &headerPath, headerPaths) {
        if (!headerPath.path.contains(QLatin1String("/Qt"))) // "QtCore", "QtGui" etc...
            continue;

        const QString headerPathCandidate = headerPath.path + QLatin1Char('/') + className;
        const QFileInfo fileInfo(headerPathCandidate);
        if (fileInfo.exists() && fileInfo.isFile()) {
            result = QLatin1Char('<') + className + QLatin1Char('>');
            break;
        }
    }

    return result;
}

ProjectPart::HeaderPaths relevantHeaderPaths(const QString &filePath)
{
    ProjectPart::HeaderPaths headerPaths;

    CppModelManager *modelManager = CppModelManager::instance();
    const QList<ProjectPart::Ptr> projectParts = modelManager->projectPart(filePath);
    if (projectParts.isEmpty()) { // Not part of any project, better use all include paths than none
        headerPaths += modelManager->headerPaths();
    } else {
        foreach (const ProjectPart::Ptr &part, projectParts)
            headerPaths += part->headerPaths;
    }

    return headerPaths;
}

NameAST *nameUnderCursor(const QList<AST *> &path)
{
    if (path.isEmpty())
        return 0;

    NameAST *nameAst = 0;
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

bool canLookupDefinition(const CppQuickFixInterface &interface, const NameAST *nameAst)
{
    QTC_ASSERT(nameAst, return false);

    // Find the enclosing scope
    unsigned line, column;
    const Document::Ptr &doc = interface.semanticInfo().doc;
    doc->translationUnit()->getTokenStartPosition(nameAst->firstToken(), &line, &column);
    Scope *scope = doc->scopeAt(line, column);
    if (!scope)
        return false;

    // Try to find the class/template definition
    const Name *name = nameAst->name;
    const QList<LookupItem> results = interface.context().lookup(name, scope);
    foreach (const LookupItem &item, results) {
        if (Symbol *declaration = item.declaration()) {
            if (declaration->isClass())
                return true;
            if (Template *templ = declaration->asTemplate()) {
                Symbol *declaration = templ->declaration();
                if (declaration && declaration->isClass())
                    return true;
            }
        }
    }

    return false;
}

QString templateNameAsString(const TemplateNameId *templateName)
{
    const Identifier *id = templateName->identifier();
    return QString::fromUtf8(id->chars(), id->size());
}

// For templates, simply the name is returned, without '<...>'.
QString unqualifiedNameForLocator(const Name *name)
{
    QTC_ASSERT(name, return QString());

    const Overview oo;
    if (const QualifiedNameId *qualifiedName = name->asQualifiedNameId()) {
        const Name *name = qualifiedName->name();
        if (const TemplateNameId *templateName = name->asTemplateNameId())
            return templateNameAsString(templateName);
        return oo.prettyName(name);
    } else if (const TemplateNameId *templateName = name->asTemplateNameId()) {
        return templateNameAsString(templateName);
    } else {
        return oo.prettyName(name);
    }
}

Snapshot forwardingHeaders(const CppQuickFixInterface &interface)
{
    Snapshot result;

    foreach (Document::Ptr doc, interface.snapshot()) {
        if (doc->globalSymbolCount() == 0 && doc->resolvedIncludes().size() == 1)
            result.insert(doc);
    }

    return result;
}

bool looksLikeAQtClass(const QString &identifier)
{
    return identifier.size() > 2
        && identifier.at(0) == QLatin1Char('Q')
        && identifier.at(1).isUpper();
}

} // anonymous namespace

void AddIncludeForUndefinedIdentifier::match(const CppQuickFixInterface &interface,
                                             QuickFixOperations &result)
{
    const NameAST *nameAst = nameUnderCursor(interface.path());
    if (!nameAst)
        return;

    if (canLookupDefinition(interface, nameAst))
        return;

    const QString className = unqualifiedNameForLocator(nameAst->name);
    if (className.isEmpty())
        return;

    const QString currentDocumentFilePath = interface.semanticInfo().doc->fileName();
    const ProjectPart::HeaderPaths headerPaths = relevantHeaderPaths(currentDocumentFilePath);
    bool qtHeaderFileIncludeOffered = false;

    // Find an include file through the locator
    if (CppClassesFilter *classesFilter
            = ExtensionSystem::PluginManager::getObject<CppClassesFilter>()) {
        QFutureInterface<Core::LocatorFilterEntry> dummy;
        const QList<Core::LocatorFilterEntry> matches = classesFilter->matchesFor(dummy, className);

        const Snapshot forwardHeaders = forwardingHeaders(interface);
        foreach (const Core::LocatorFilterEntry &entry, matches) {
            IndexItem::Ptr info = entry.internalData.value<IndexItem::Ptr>();
            if (info->symbolName() != className)
                continue;

            Snapshot localForwardHeaders = forwardHeaders;
            localForwardHeaders.insert(interface.snapshot().document(info->fileName()));
            Utils::FileNameList headerAndItsForwardingHeaders;
            headerAndItsForwardingHeaders << Utils::FileName::fromString(info->fileName());
            headerAndItsForwardingHeaders += localForwardHeaders.filesDependingOn(info->fileName());

            foreach (const Utils::FileName &header, headerAndItsForwardingHeaders) {
                const QString include = findShortestInclude(currentDocumentFilePath,
                                                            header.toString(),
                                                            headerPaths);
                if (include.size() > 2) {
                    const QString headerFileName = Utils::FileName::fromString(info->fileName()).fileName();
                    QTC_ASSERT(!headerFileName.isEmpty(), break);

                    int priority = 0;
                    if (headerFileName == className)
                        priority = 2;
                    else if (headerFileName.at(1).isUpper())
                        priority = 1;

                    if (looksLikeAQtClass(include.mid(1, include.size() - 2)))
                        qtHeaderFileIncludeOffered = true;

                    result.append(new AddIncludeForUndefinedIdentifierOp(interface, priority,
                                                                         include));
                }
            }
        }
    }

    // The header file we are looking for might not be (yet) included in any file we have parsed.
    // As such, it will not be findable via locator. At least for Qt classes, check also for
    // headers with the same name.
    if (!qtHeaderFileIncludeOffered && looksLikeAQtClass(className)) {
        const QString include = findQtIncludeWithSameName(className, headerPaths);
        if (!include.isEmpty())
            result.append(new AddIncludeForUndefinedIdentifierOp(interface, 1, include));
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
    const QList<AST *> path = interface.path();

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
        result.append(new RearrangeParamDeclarationListOp(interface, paramListNode->value,
                          prevParamListNode->value, RearrangeParamDeclarationListOp::TargetPrevious));
    if (paramListNode->next)
        result.append(new RearrangeParamDeclarationListOp(interface, paramListNode->value,
                          paramListNode->next->value, RearrangeParamDeclarationListOp::TargetNext));
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
            result.append(new ReformatPointerDeclarationOp(interface, change));
    } else {
        const QList<AST *> suitableASTs
            = ReformatPointerDeclarationASTPathResultsFilter().filter(path);
        foreach (AST *ast, suitableASTs) {
            change = formatter.format(ast);
            if (!change.isEmpty()) {
                result.append(new ReformatPointerDeclarationOp(interface, change));
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
    CompleteSwitchCaseStatementOp(const CppQuickFixInterface &interface,
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
            if (ClassOrNamespace *con = ctxt.lookupType(namedType->name(), result.scope())) {
                const QList<Enum *> enums = con->unscopedEnums();
                const Name *referenceName = namedType->name();
                if (const QualifiedNameId *qualifiedName = referenceName->asQualifiedNameId())
                    referenceName = qualifiedName->name();
                foreach (Enum *e, enums) {
                    if (const Name *candidateName = e->name()) {
                        if (candidateName->match(referenceName))
                            return e;
                    }
                }
            }
        }
    }

    return 0;
}

Enum *conditionEnum(const CppQuickFixInterface &interface, SwitchStatementAST *statement)
{
    Block *block = statement->symbol;
    Scope *scope = interface.semanticInfo().doc->scopeAt(block->line(), block->column());
    TypeOfExpression typeOfExpression;
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
    const QList<AST *> &path = interface.path();

    if (path.isEmpty())
        return;

    // look for switch statement
    for (int depth = path.size() - 1; depth >= 0; --depth) {
        AST *ast = path.at(depth);
        SwitchStatementAST *switchStatement = ast->asSwitchStatement();
        if (switchStatement) {
            if (!interface.isCursorOn(switchStatement->switch_token) || !switchStatement->statement)
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
                CaseStatementCollector caseValues(interface.semanticInfo().doc, interface.snapshot(),
                                                  interface.semanticInfo().doc->scopeAt(block->line(), block->column()));
                QStringList usedValues = caseValues(switchStatement);
                // save the values that would be added
                foreach (const QString &usedValue, usedValues)
                    values.removeAll(usedValue);
                if (!values.isEmpty())
                    result.append(new CompleteSwitchCaseStatementOp(interface, depth,
                                                                    compoundStatement, values));
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
                        const QString &targetFileName, const Class *targetSymbol,
                        InsertionPointLocator::AccessSpec xsSpec, const QString &decl, int priority)
        : CppQuickFixOperation(interface, priority)
        , m_targetFileName(targetFileName)
        , m_targetSymbol(targetSymbol)
        , m_xsSpec(xsSpec)
        , m_decl(decl)
    {
        setDescription(QCoreApplication::translate("CppEditor::InsertDeclOperation",
                                                   "Add %1 Declaration")
                       .arg(InsertionPointLocator::accessSpecToString(xsSpec)));
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

    QuickFixOperation *operator()(InsertionPointLocator::AccessSpec xsSpec, int priority)
    {
        return new InsertDeclOperation(m_interface, m_fileName, m_matchingClass, xsSpec, m_decl, priority);
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
    const QList<AST *> &path = interface.path();
    CppRefactoringFilePtr file = interface.currentFile();

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
    if (Class *matchingClass = isMemberFunction(interface.context(), fun)) {
        const QualifiedNameId *qName = fun->name()->asQualifiedNameId();
        for (Symbol *s = matchingClass->find(qName->identifier()); s; s = s->next()) {
            if (!s->name()
                    || !qName->identifier()->match(s->identifier())
                    || !s->type()->isFunctionType())
                continue;

            if (s->type().match(fun->type())) {
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
    InsertDefOperation(const CppQuickFixInterface &interface,
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
            setPriority(2);
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
            setPriority(1);
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
            editor()->setTextCursor(c);
        } else {
            // make target lookup context
            Document::Ptr targetDoc = targetFile->cppDocument();
            Scope *targetScope = targetDoc->scopeAt(m_loc.line(), m_loc.column());
            LookupContext targetContext(targetDoc, snapshot());
            ClassOrNamespace *targetCoN = targetContext.lookupType(targetScope);
            if (!targetCoN)
                targetCoN = targetContext.globalNamespace();

            // setup rewriting to get minimally qualified names
            SubstitutionEnvironment env;
            env.setContext(context());
            env.switchScope(m_decl->enclosingScope());
            UseMinimalNames q(targetCoN);
            env.enter(&q);
            Control *control = context().bindings()->control().data();

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
                if (targetFile->editor())
                    targetFile->editor()->setTextCursor(c);
            } else {
                editor()->setTextCursor(c);
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
                            if (func->isSignal())
                                return;

                            // Check if there is already a definition
                            SymbolFinder symbolFinder;
                            if (symbolFinder.findMatchingDefinition(decl, interface.snapshot(),
                                                                    true)) {
                                return;
                            }

                            // Insert Position: Implementation File
                            DeclaratorAST *declAST = simpleDecl->declarator_list->value;
                            InsertDefOperation *op = 0;
                            ProjectFile::Kind kind = ProjectFile::classify(interface.fileName());
                            const bool isHeaderFile = ProjectFile::isHeader(kind);
                            if (isHeaderFile) {
                                CppRefactoringChanges refactoring(interface.snapshot());
                                InsertionPointLocator locator(refactoring);
                                // find appropriate implementation file, but do not use this
                                // location, because insertLocationForMethodDefinition() should
                                // be used in perform() to get consistent insert positions.
                                foreach (const InsertionLocation &location,
                                         locator.methodDefinition(decl, false, QString())) {
                                    if (!location.isValid())
                                        continue;

                                    const QString fileName = location.fileName();
                                    if (ProjectFile::isHeader(ProjectFile::classify(fileName))) {
                                        const QString source
                                                = CppTools::correspondingHeaderOrSource(fileName);
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
                                                                    fileName);
                                    }

                                    if (op)
                                        result.append(op);
                                    break;
                                }
                            }

                            // Determine if we are dealing with a free function
                            const bool isFreeFunction = func->enclosingClass() == 0;

                            // Insert Position: Outside Class
                            if (!isFreeFunction) {
                                result.append(new InsertDefOperation(interface, decl, declAST,
                                                                     InsertionLocation(),
                                                                     DefPosOutsideClass,
                                                                     interface.fileName()));
                            }

                            // Insert Position: Inside Class
                            // Determine insert location direct after the declaration.
                            unsigned line, column;
                            const CppRefactoringFilePtr file = interface.currentFile();
                            file->lineAndColumn(file->endOf(simpleDecl), &line, &column);
                            const InsertionLocation loc
                                    = InsertionLocation(interface.fileName(), QString(), QString(),
                                                        line, column);
                            result.append(new InsertDefOperation(interface, decl, declAST, loc,
                                                                 DefPosInsideClass, QString(),
                                                                 isFreeFunction));

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

bool hasClassMemberWithGetPrefix(const Class *klass)
{
    if (!klass)
        return false;

    for (unsigned i = 0; i < klass->memberCount(); ++i) {
        const Symbol *symbol = klass->memberAt(i);
        if (symbol->isFunction() || symbol->isDeclaration()) {
            if (const Name *symbolName = symbol->name()) {
                if (const Identifier *id = symbolName->identifier()) {
                    if (!strncmp(id->chars(), "get", 3))
                        return true;
                }
            }
        }
    }
    return false;
}

class GenerateGetterSetterOperation : public CppQuickFixOperation
{
public:
    enum OperationType {
        InvalidType,
        GetterSetterType,
        GetterType,
        SetterType
    };

    GenerateGetterSetterOperation(const CppQuickFixInterface &interface)
        : CppQuickFixOperation(interface)
        , m_type(InvalidType)
        , m_variableName(0)
        , m_declaratorId(0)
        , m_declarator(0)
        , m_variableDecl(0)
        , m_classSpecifier(0)
        , m_classDecl(0)
        , m_symbol(0)
        , m_offerQuickFix(true)
    {
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
        m_variableString = QString::fromUtf8(variableId->chars(), variableId->size());

        m_baseName = memberBaseName(m_variableString);
        if (m_baseName.isEmpty())
            m_baseName = QLatin1String("value");

        m_getterName = !(m_baseName == m_variableString
                         || hasClassMemberWithGetPrefix(m_classSpecifier->symbol))
            ? m_baseName
            : QString::fromLatin1("get%1%2")
                .arg(m_baseName.left(1).toUpper()).arg(m_baseName.mid(1));
        m_setterName = QString::fromLatin1("set%1%2")
            .arg(m_baseName.left(1).toUpper()).arg(m_baseName.mid(1));

        // Check if the class has already a getter and/or a setter.
        // This is only a simple check which should suffice not triggering the
        // same quick fix again. Limitations:
        //   1) It only checks in the current class, but not in base classes.
        //   2) It compares only names instead of types/signatures.
        //   3) Symbols in Qt property declarations are ignored.
        bool hasGetter = false;
        bool hasSetter = false;
        if (Class *klass = m_classSpecifier->symbol) {
            for (unsigned i = 0; i < klass->memberCount(); ++i) {
                Symbol *symbol = klass->memberAt(i);
                if (symbol->isQtPropertyDeclaration())
                    continue;
                if (const Name *symbolName = symbol->name()) {
                    if (const Identifier *id = symbolName->identifier()) {
                        const QString memberName = QString::fromUtf8(id->chars(), id->size());
                        if (memberName == m_getterName)
                            hasGetter = true;
                        if (memberName == m_setterName)
                            hasSetter = true;
                        if (hasGetter && hasSetter)
                            break;
                    }
                }
            } // for
        }
        if (hasGetter && hasSetter) {
            m_offerQuickFix = false;
            return;
        }

        // Find the right symbol in the simple declaration
        const List<Symbol *> *symbols = m_variableDecl->symbols;
        QTC_ASSERT(symbols, return);
        for (; symbols; symbols = symbols->next) {
            Symbol *s = symbols->value;
            if (const Name *name = s->name()) {
                if (const Identifier *id = name->identifier()) {
                    const QString symbolName = QString::fromUtf8(id->chars(), id->size());
                    if (symbolName == m_variableString) {
                        m_symbol = s;
                        break;
                    }
                }
            }
        }

        QTC_ASSERT(m_symbol, return);
        if (hasSetter) { // hasGetter is false in this case
            m_type = GetterType;
        } else { // no setter
            if (hasGetter) {
                if (m_symbol->type().isConst()) {
                    m_offerQuickFix = false;
                    return;
                } else {
                    m_type = SetterType;
                }
            } else {
                m_type = (m_symbol->type().isConst()) ? GetterType : GetterSetterType;
            }
        }
        updateDescriptionAndPriority();
    }

    // Clones "other" in order to prevent all the initial detection made in the ctor.
    GenerateGetterSetterOperation(const CppQuickFixInterface &interface,
                                  GenerateGetterSetterOperation *other, OperationType type)
        : CppQuickFixOperation(interface)
        , m_type(type)
        , m_variableName(other->m_variableName)
        , m_declaratorId(other->m_declaratorId)
        , m_declarator(other->m_declarator)
        , m_variableDecl(other->m_variableDecl)
        , m_classSpecifier(other->m_classSpecifier)
        , m_classDecl(other->m_classDecl)
        , m_symbol(other->m_symbol)
        , m_baseName(other->m_baseName)
        , m_getterName(other->m_getterName)
        , m_setterName(other->m_setterName)
        , m_variableString(other->m_variableString)
        , m_offerQuickFix(other->m_offerQuickFix)
    {
        QTC_ASSERT(isValid(), return);
        updateDescriptionAndPriority();
    }

    bool generateGetter() const
    {
        return (m_type == GetterSetterType || m_type == GetterType);
    }

    bool generateSetter() const
    {
        return (m_type == GetterSetterType || m_type == SetterType);
    }

    void updateDescriptionAndPriority()
    {
        switch (m_type) {
        case GetterSetterType:
            setPriority(5);
            setDescription(QuickFixFactory::tr("Create Getter and Setter Member Functions"));
            break;
        case GetterType:
            setPriority(4);
            setDescription(QuickFixFactory::tr("Create Getter Member Function"));
            break;
        case SetterType:
            setPriority(3);
            setDescription(QuickFixFactory::tr("Create Setter Member Function"));
            break;
        default:
            break;
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

        QTC_ASSERT(m_symbol, return);
        FullySpecifiedType fullySpecifiedType = m_symbol->type();
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
        QString classString = QString::fromUtf8(classId->chars(), classId->size());

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

        const bool isStatic = m_symbol->storage() == Symbol::Static;

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

        if (generateGetter())
            declaration += declarationGetter;
        if (generateSetter())
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
        QString implementation;
        if (generateGetter())
            implementation += implementationGetter;
        if (generateSetter())
            implementation += implementationSetter;

        // Create and apply changes
        ChangeSet currChanges;
        int declInsertPos = currentFile->position(qMax(1u, declLocation.line()),
                                                  declLocation.column());
        currChanges.insert(declInsertPos, declaration);

        if (sameFile) {
            InsertionLocation loc = insertLocationForMethodDefinition(m_symbol, false, refactoring,
                                                                      currentFile->fileName());
            currChanges.insert(currentFile->position(loc.line(), loc.column()), implementation);
        } else {
            CppRefactoringChanges implRef(snapshot());
            CppRefactoringFilePtr implFile = implRef.file(implFileName);
            ChangeSet implChanges;
            InsertionLocation loc = insertLocationForMethodDefinition(m_symbol, false,
                                                                      implRef, implFileName);
            const int implInsertPos = implFile->position(loc.line(), loc.column());
            implChanges.insert(implInsertPos, implementation);
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

    OperationType m_type;
    SimpleNameAST *m_variableName;
    DeclaratorIdAST *m_declaratorId;
    DeclaratorAST *m_declarator;
    SimpleDeclarationAST *m_variableDecl;
    ClassSpecifierAST *m_classSpecifier;
    SimpleDeclarationAST *m_classDecl;
    Symbol *m_symbol;

    QString m_baseName;
    QString m_getterName;
    QString m_setterName;
    QString m_variableString;
    bool m_offerQuickFix;
};

} // anonymous namespace

void GenerateGetterSetter::match(const CppQuickFixInterface &interface,
                                 QuickFixOperations &result)
{
    GenerateGetterSetterOperation *op = new GenerateGetterSetterOperation(interface);
    if (op->m_type != GenerateGetterSetterOperation::InvalidType) {
        result.append(op);
        if (op->m_type == GenerateGetterSetterOperation::GetterSetterType) {
            result.append(new GenerateGetterSetterOperation(
                              interface, op, GenerateGetterSetterOperation::GetterType));
            result.append(new GenerateGetterSetterOperation(
                              interface, op, GenerateGetterSetterOperation::SetterType));
        }
    } else {
        delete op;
    }
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
        setDescription(QCoreApplication::translate("QuickFix::ExtractFunction", "Extract Function"));
    }

    void perform()
    {
        QTC_ASSERT(!m_funcReturn || !m_relevantDecls.isEmpty(), return);
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());

        const QString &funcName = m_functionNameGetter ? m_functionNameGetter() : getFunctionName();
        if (funcName.isEmpty())
            return;

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
                        + QLatin1Char(';'));
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
            Core::AsynchronousMessageBox::critical(QCoreApplication::translate("QuickFix::ExtractFunction",
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

ExtractFunction::ExtractFunction(FunctionNameGetter functionNameGetter)
    : m_functionNameGetter(functionNameGetter)
{
}

void ExtractFunction::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    CppRefactoringFilePtr file = interface.currentFile();

    QTextCursor cursor = file->cursor();
    if (!cursor.hasSelection())
        return;

    const QList<AST *> &path = interface.path();
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
    Symbol *funcReturn = 0;
    QList<QPair<QString, QString> > relevantDecls;
    SemanticInfo::LocalUseIterator it(interface.semanticInfo().localUses);
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
    result.append(new ExtractFunctionOperation(interface,
                                               analyser.m_extractionStart,
                                               analyser.m_extractionEnd,
                                               refFuncDef,
                                               funcReturn,
                                               relevantDecls,
                                               m_functionNameGetter));
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
    bool visit(T *ast)
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
        setDescription(QApplication::translate("CppTools::QuickFix",
                                               "Extract Constant as Function Parameter"));
    }

    struct FoundDeclaration
    {
        FoundDeclaration()
            : ast(0)
        {}

        FunctionDeclaratorAST *ast;
        CppRefactoringFilePtr file;
    };

    FoundDeclaration findDeclaration(const CppRefactoringChanges &refactoring,
                                     FunctionDefinitionAST *ast)
    {
        FoundDeclaration result;
        Function *func = ast->symbol;
        QString declFileName;
        if (Class *matchingClass = isMemberFunction(context(), func)) {
            // Dealing with member functions
            const QualifiedNameId *qName = func->name()->asQualifiedNameId();
            for (Symbol *s = matchingClass->find(qName->identifier()); s; s = s->next()) {
                if (!s->name()
                        || !qName->identifier()->match(s->identifier())
                        || !s->type()->isFunctionType()
                        || !s->type().match(func->type())
                        || s->isFunction()) {
                    continue;
                }

                declFileName = QString::fromUtf8(matchingClass->fileName(),
                                                 matchingClass->fileNameLength());

                result.file = refactoring.file(declFileName);
                ASTPath astPath(result.file->cppDocument());
                const QList<AST *> path = astPath(s->line(), s->column());
                SimpleDeclarationAST *simpleDecl;
                for (int idx = 0; idx < path.size(); ++idx) {
                    AST *node = path.at(idx);
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
            declFileName = correspondingHeaderOrSource(fileName(), &isHeaderFile);
            if (!QFile::exists(declFileName))
                return FoundDeclaration();
            result.file = refactoring.file(declFileName);
            if (!result.file)
                return FoundDeclaration();
            const LookupContext lc(result.file->cppDocument(), snapshot());
            const QList<LookupItem> candidates = lc.lookup(func->name(), matchingNamespace);
            for (int i = 0; i < candidates.size(); ++i) {
                if (Symbol *s = candidates.at(i).declaration()) {
                    if (s->asDeclaration()) {
                        ASTPath astPath(result.file->cppDocument());
                        const QList<AST *> path = astPath(s->line(), s->column());
                        for (int idx = 0; idx < path.size(); ++idx) {
                            AST *node = path.at(idx);
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

    void perform()
    {
        FunctionDeclaratorAST *functionDeclaratorOfDefinition
                = functionDeclarator(m_functionDefinition);
        const CppRefactoringChanges refactoring(snapshot());
        const CppRefactoringFilePtr currentFile = refactoring.file(fileName());
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
            if (currentFile->fileName() != functionDeclaration.file->fileName()) {
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

    QString parameterDeclarationTextToInsert(FunctionDeclaratorAST *ast) const
    {
        QString str;
        if (hasParameters(ast))
            str = QLatin1String(", ");
        str += m_typeName;
        if (!m_typeName.endsWith(QLatin1Char('*')))
                str += QLatin1Char(' ');
        str += QLatin1String("newParameter");
        return str;
    }

    FunctionDeclaratorAST *functionDeclarator(SimpleDeclarationAST *ast) const
    {
        for (DeclaratorListAST *decls = ast->declarator_list; decls; decls = decls->next) {
            FunctionDeclaratorAST * const functionDeclaratorAST = functionDeclarator(decls->value);
            if (functionDeclaratorAST)
                return functionDeclaratorAST;
        }
        return 0;
    }

    FunctionDeclaratorAST *functionDeclarator(DeclaratorAST *ast) const
    {
        for (PostfixDeclaratorListAST *pds = ast->postfix_declarator_list; pds; pds = pds->next) {
            FunctionDeclaratorAST *funcdecl = pds->value->asFunctionDeclarator();
            if (funcdecl)
                return funcdecl;
        }
        return 0;
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
    result.append(new ExtractLiteralAsParameterOp(interface, priority, literal, function));
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
        , m_file(m_refactoring.file(fileName()))
        , m_document(interface.semanticInfo().doc)
    {
        setDescription(
                mode == FromPointer
                ? QuickFixFactory::tr("Convert to Stack Variable")
                : QuickFixFactory::tr("Convert to Pointer"));
    }

    void perform() Q_DECL_OVERRIDE
    {
        ChangeSet changes;

        switch (m_mode) {
        case FromPointer:
            removePointerOperator(changes);
            convertToStackVariable(changes);
            break;
        case FromReference:
            removeReferenceOperator(changes);
            // fallthrough intended
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
        if (newExprAST->new_initializer) {
            // remove 'new' keyword and type before initializer
            changes.remove(m_file->startOf(newExprAST->new_token),
                           m_file->startOf(newExprAST->new_initializer));

            // remove parenthesis around initializer
            if (ExpressionListParenAST *exprlist
                    = newExprAST->new_initializer->asExpressionListParen()) {
                int pos = m_file->startOf(exprlist->lparen_token);
                changes.remove(pos, pos + 1);
                pos = m_file->startOf(exprlist->rparen_token);
                changes.remove(pos, pos + 1);
            }
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
        foreach (const SemanticInfo::Use &use, semanticInfo().localUses.value(m_symbol)) {
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

    void insertNewExpression(ChangeSet &changes, CallAST *callAST) const
    {
        const QString typeName = typeNameOfDeclaration();
        if (typeName.isEmpty()) {
            changes.insert(m_file->startOf(callAST), QLatin1String("new "));
        } else {
            changes.insert(m_file->startOf(callAST),
                           QLatin1String("new ") + typeName + QLatin1Char('('));
            changes.insert(m_file->startOf(callAST->lastToken()), QLatin1String(")"));
        }
    }

    void insertNewExpression(ChangeSet &changes, ExpressionListParenAST *exprListAST) const
    {
        const QString typeName = typeNameOfDeclaration();
        if (typeName.isEmpty())
            return;
        changes.insert(m_file->startOf(exprListAST),
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
            } else if (ExpressionListParenAST *exprListAST
                     = m_declaratorAST->initializer->asExpressionListParen()) {
                insertNewExpression(changes, exprListAST);
            }
        }

        // Fix all occurrences of the identifier in this function.
        ASTPath astPath(m_document);
        foreach (const SemanticInfo::Use &use, semanticInfo().localUses.value(m_symbol)) {
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
    SimpleDeclarationAST *simpleDeclaration = 0;
    DeclaratorAST *declarator = 0;
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

    Symbol *symbol = 0;
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
        if (!result.isEmpty() && result.first().type()->isPointerType())
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
    result.append(new ConvertFromAndToPointerOp(interface, priority, mode, isAutoDeclaration,
                                                simpleDeclaration, declarator, identifier, symbol));
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

    InsertQtPropertyMembersOp(const CppQuickFixInterface &interface,
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
        setDescription(QuickFixFactory::tr("Generate Missing Q_PROPERTY Members..."));
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr file = refactoring.file(fileName());

        InsertionPointLocator locator(refactoring);
        ChangeSet declarations;

        const QString typeName = file->textOf(m_declaration->type_id);
        const QString propertyName = file->textOf(m_declaration->property_name);
        QString baseName = memberBaseName(m_storageName);
        if (baseName.isEmpty() || baseName == m_storageName)
            baseName = QStringLiteral("arg");

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
            setter << "void " << m_setterName << '(' << typeName << ' ' << baseName << ")\n{\n";
            if (m_signalName.isEmpty()) {
                setter << m_storageName <<  " = " << baseName << ";\n}\n";
            } else {
                setter << "if (" << m_storageName << " == " << baseName << ")\nreturn;\n\n"
                       << m_storageName << " = " << baseName << ";\nemit " << m_signalName
                       << '(' << baseName << ");\n}\n";
            }
            InsertionLocation setterLoc = locator.methodDeclarationInClass(file->fileName(), m_class, InsertionPointLocator::PublicSlot);
            QTC_ASSERT(setterLoc.isValid(), return);
            insertAndIndent(file, &declarations, setterLoc, setterDeclaration);
        }

        // signal declaration
        if (m_generateFlags & GenerateSignal) {
            const QString declaration = QLatin1String("void ") + m_signalName + QLatin1Char('(')
                                        + typeName + QLatin1Char(' ') + baseName
                                        + QLatin1String(");\n");
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
    const QList<AST *> &path = interface.path();

    if (path.isEmpty())
        return;

    AST * const ast = path.last();
    QtPropertyDeclarationAST *qtPropertyDeclaration = ast->asQtPropertyDeclaration();
    if (!qtPropertyDeclaration || !qtPropertyDeclaration->type_id)
        return;

    ClassSpecifierAST *klass = 0;
    for (int i = path.size() - 2; i >= 0; --i) {
        klass = path.at(i)->asClassSpecifier();
        if (klass)
            break;
    }
    if (!klass)
        return;

    CppRefactoringFilePtr file = interface.currentFile();
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

    result.append(new InsertQtPropertyMembersOp(interface, path.size() - 1, qtPropertyDeclaration, c,
                                                generateFlags, getterName, setterName,
                                                signalName, storageName));
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
    op->setDescription(FunctionDeclDefLink::tr("Apply Function Signature Changes"));
    result.append(op);
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
    const Name *name = func->name();
    if (name && nameIncludesOperatorName(name)) {
        CoreDeclaratorAST *coreDeclarator = functionDefinitionAST->declarator->core_declarator;
        const QString operatorNameText = baseFile->textOf(coreDeclarator);
        oo.includeWhiteSpaceInOperatorName = operatorNameText.contains(QLatin1Char(' '));
    }
    const QString nameText = oo.prettyName(LookupContext::minimalName(func, cppCoN, control));
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
                                 const QString &fromFile, const QString &toFile)
        : m_operation(operation), m_type(type), m_changes(m_operation->snapshot())
    {
        m_fromFile = m_changes.file(fromFile);
        m_toFile = (m_type == MoveOutside) ? m_fromFile : m_changes.file(toFile);
    }

    void performMove(FunctionDefinitionAST *funcAST)
    {
        // Determine file, insert position and scope
        InsertionLocation l = insertLocationForMethodDefinition(funcAST->symbol, false,
                                                                m_changes, m_toFile->fileName());
        const QString prefix = l.prefix();
        const QString suffix = l.suffix();
        const int insertPos = m_toFile->position(l.line(), l.column());
        Scope *scopeAtInsertPos = m_toFile->cppDocument()->scopeAt(l.line(), l.column());

        // construct definition
        const QString funcDec = definitionSignature(m_operation, funcAST, m_fromFile, m_toFile,
                                                    scopeAtInsertPos);
        QString funcDef = prefix + funcDec;
        const int startPosition = m_fromFile->endOf(funcAST->declarator);
        const int endPosition = m_fromFile->endOf(funcAST->function_body);
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
                         FunctionDefinitionAST *funcDef, const QString &cppFileName)
        : CppQuickFixOperation(interface, 0)
        , m_funcDef(funcDef)
        , m_type(type)
        , m_cppFileName(cppFileName)
        , m_headerFileName(QString::fromUtf8(funcDef->symbol->fileName(),
                                             funcDef->symbol->fileNameLength()))
    {
        if (m_type == MoveFuncDefRefactoringHelper::MoveOutside) {
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
        MoveFuncDefRefactoringHelper helper(this, m_type, m_headerFileName, m_cppFileName);
        helper.performMove(m_funcDef);
        helper.applyChanges();
    }

private:
    FunctionDefinitionAST *m_funcDef;
    MoveFuncDefRefactoringHelper::MoveType m_type;
    const QString m_cppFileName;
    const QString m_headerFileName;
};

} // anonymous namespace

void MoveFuncDefOutside::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();
    SimpleDeclarationAST *classAST = 0;
    FunctionDefinitionAST *funcAST = 0;
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
            }
            funcAST = 0;
        }
    }

    if (!funcAST)
        return;

    bool isHeaderFile = false;
    const QString cppFileName = correspondingHeaderOrSource(interface.fileName(), &isHeaderFile);

    if (isHeaderFile && !cppFileName.isEmpty()) {
        const MoveFuncDefRefactoringHelper::MoveType type = moveOutsideMemberDefinition
                ? MoveFuncDefRefactoringHelper::MoveOutsideMemberToCppFile
                : MoveFuncDefRefactoringHelper::MoveToCppFile;
        result.append(new MoveFuncDefOutsideOp(interface, type, funcAST, cppFileName));
    }

    if (classAST)
        result.append(new MoveFuncDefOutsideOp(interface, MoveFuncDefRefactoringHelper::MoveOutside,
                                               funcAST, QLatin1String("")));

    return;
}

namespace {

class MoveAllFuncDefOutsideOp : public CppQuickFixOperation
{
public:
    MoveAllFuncDefOutsideOp(const CppQuickFixInterface &interface,
                            MoveFuncDefRefactoringHelper::MoveType type,
                            ClassSpecifierAST *classDef, const QString &cppFileName)
        : CppQuickFixOperation(interface, 0)
        , m_type(type)
        , m_classDef(classDef)
        , m_cppFileName(cppFileName)
        , m_headerFileName(QString::fromUtf8(classDef->symbol->fileName(),
                                             classDef->symbol->fileNameLength()))
    {
        if (m_type == MoveFuncDefRefactoringHelper::MoveOutside) {
            setDescription(QCoreApplication::translate("CppEditor::QuickFix", "Move All Function "
                                                       "Definitions Outside Class"));
        } else {
            const QDir dir = QFileInfo(m_headerFileName).dir();
            setDescription(QCoreApplication::translate("CppEditor::QuickFix",
                                                       "Move All Function Definitions to %1")
                           .arg(dir.relativeFilePath(m_cppFileName)));
        }
    }

    void perform()
    {
        MoveFuncDefRefactoringHelper helper(this, m_type, m_headerFileName, m_cppFileName);
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
    const QString m_cppFileName;
    const QString m_headerFileName;
};

} // anonymous namespace

void MoveAllFuncDefOutside::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();
    const int pathSize = path.size();
    if (pathSize < 2)
        return;

    // Determine if cursor is on a class which is not a base class
    ClassSpecifierAST *classAST = Q_NULLPTR;
    if (SimpleNameAST *nameAST = path.at(pathSize - 1)->asSimpleName()) {
        if (!interface.isCursorOn(nameAST))
            return;
        classAST = path.at(pathSize - 2)->asClassSpecifier();
    }
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
    const QString cppFileName = correspondingHeaderOrSource(interface.fileName(), &isHeaderFile);
    if (isHeaderFile && !cppFileName.isEmpty()) {
        result.append(new MoveAllFuncDefOutsideOp(interface,
                                                  MoveFuncDefRefactoringHelper::MoveToCppFile,
                                                  classAST, cppFileName));
    }
    result.append(new MoveAllFuncDefOutsideOp(interface, MoveFuncDefRefactoringHelper::MoveOutside,
                                              classAST, QLatin1String("")));
}

namespace {

class MoveFuncDefToDeclOp : public CppQuickFixOperation
{
public:
    MoveFuncDefToDeclOp(const CppQuickFixInterface &interface,
                        const QString &fromFileName, const QString &toFileName,
                        FunctionDefinitionAST *funcDef, const QString &declText,
                        const ChangeSet::Range &toRange)
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
        ChangeSet toTarget;
        toTarget.replace(m_toRange, wholeFunctionText);
        if (m_toFileName == m_fromFileName)
            toTarget.remove(fromRange);
        toFile->setChangeSet(toTarget);
        toFile->appendIndentRange(m_toRange);
        toFile->setOpenEditor(true, m_toRange.start);
        toFile->apply();
        if (m_toFileName != m_fromFileName) {
            ChangeSet fromTarget;
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

} // anonymous namespace

void MoveFuncDefToDecl::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();
    FunctionDefinitionAST *funcAST = 0;

    const int pathSize = path.size();
    for (int idx = 1; idx < pathSize; ++idx) {
        if ((funcAST = path.at(idx)->asFunctionDefinition())) {
            if (path.at(idx - 1)->asClassSpecifier())
                return;

            // check cursor position
            if (idx != pathSize - 1  // Do not allow "void a() @ {..."
                    && funcAST->function_body
                    && !interface.isCursorOn(funcAST->function_body)) {
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
    if (Class *matchingClass = isMemberFunction(interface.context(), func)) {
        // Dealing with member functions
        const QualifiedNameId *qName = func->name()->asQualifiedNameId();
        for (Symbol *s = matchingClass->find(qName->identifier()); s; s = s->next()) {
            if (!s->name()
                    || !qName->identifier()->match(s->identifier())
                    || !s->type()->isFunctionType()
                    || !s->type().match(func->type())
                    || s->isFunction()) {
                continue;
            }

            declFileName = QString::fromUtf8(matchingClass->fileName(),
                                             matchingClass->fileNameLength());

            const CppRefactoringChanges refactoring(interface.snapshot());
            const CppRefactoringFilePtr declFile = refactoring.file(declFileName);
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
        declFileName = correspondingHeaderOrSource(interface.fileName(), &isHeaderFile);
        if (isHeaderFile)
            return;

        const CppRefactoringChanges refactoring(interface.snapshot());
        const CppRefactoringFilePtr declFile = refactoring.file(declFileName);
        const LookupContext lc(declFile->cppDocument(), interface.snapshot());
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
        result.append(new MoveFuncDefToDeclOp(interface,
                                              interface.fileName(),
                                              declFileName,
                                              funcAST, declText,
                                              declRange));
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
        CppRefactoringFilePtr file = refactoring.file(fileName());

        // Determine return type and new variable name
        TypeOfExpression typeOfExpression;
        typeOfExpression.init(semanticInfo().doc, snapshot(),
                              context().bindings());
        typeOfExpression.setExpandTemplates(true);
        Scope *scope = file->scopeAt(m_ast->firstToken());
        const QList<LookupItem> result = typeOfExpression(file->textOf(m_ast).toUtf8(),
                                                          scope, TypeOfExpression::Preprocess);

        if (!result.isEmpty()) {
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
                editor()->setTextCursor(c);
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
    const QList<AST *> &path = interface.path();
    AST *outerAST = 0;
    SimpleNameAST *nameAST = 0;

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

            const Name *name = nameAST->name;
            const int insertPos = interface.currentFile()->startOf(outerAST);
            result.append(new AssignToLocalVariableOperation(interface, insertPos, outerAST, name));
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
        setDescription(QApplication::translate("CppTools::QuickFix", "Optimize for-Loop"));
    }

    void perform()
    {
        QTC_ASSERT(m_forAst, return);

        const QString filename = currentFile()->fileName();
        const CppRefactoringChanges refactoring(snapshot());
        const CppRefactoringFilePtr file = refactoring.file(filename);
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

                renamePos = file->endOf(m_forAst->initializer) + 1 + varName.length();
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
    ForStatementAST *forAst = 0;
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
    ExpressionAST *conditionExpression = 0;
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
        result.append(new OptimizeForLoopOperation(interface, forAst, optimizePostcrement,
                                                   (optimizeCondition) ? conditionExpression : 0,
                                                   conditionType));
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
            setDescription(QApplication::translate("CppTools::QuickFix",
                                                   "Escape String Literal as UTF-8"));
        } else {
            setDescription(QApplication::translate("CppTools::QuickFix",
                                                   "Unescape String Literal as UTF-8"));
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

    static QByteArray escapeString(const QByteArray &contents)
    {
        QByteArray newContents;
        for (int i = 0; i < contents.length(); ++i) {
            quint8 c = contents.at(i);
            if (isascii(c) && isprint(c)) {
                newContents += c;
            } else {
                newContents += QByteArray("\\x") +
                        QByteArray::number(c, 16).rightJustified(2, '0');
            }
        }
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
    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());

        const int startPos = currentFile->startOf(m_literal);
        const int endPos = currentFile->endOf(m_literal);

        StringLiteralAST *stringLiteral = m_literal->asStringLiteral();
        QTC_ASSERT(stringLiteral, return);
        const QByteArray oldContents(currentFile->tokenAt(stringLiteral->literal_token).
                                     identifier->chars());
        QByteArray newContents;
        if (m_escape)
            newContents = escapeString(oldContents);
        else
            newContents = unescapeString(oldContents);

        if (oldContents != newContents) {
            // Check UTF-8 byte array is correct or not.
            QTextCodec *utf8codec = QTextCodec::codecForName("UTF-8");
            QScopedPointer<QTextDecoder> decoder(utf8codec->makeDecoder());
            const QString str = decoder->toUnicode(newContents);
            const QByteArray utf8buf = str.toUtf8();
            if (utf8codec->canEncode(str) && newContents == utf8buf) {
                ChangeSet changes;
                changes.replace(startPos + 1, endPos - 1, str);
                currentFile->setChangeSet(changes);
                currentFile->apply();
            }
        }
    }

private:
    ExpressionAST *m_literal;
    bool m_escape;
};

} // anonymous namespace

void EscapeStringLiteral::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface.path();

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
        result.append(new EscapeStringLiteralOperation(interface, literal, true));

    if (canUnescape)
        result.append(new EscapeStringLiteralOperation(interface, literal, false));
}


namespace {

class ConvertQt4ConnectOperation: public CppQuickFixOperation
{
public:
    ConvertQt4ConnectOperation(const CppQuickFixInterface &interface, const ChangeSet &changes)
        : CppQuickFixOperation(interface, 1), m_changes(changes)
    {
        setDescription(QApplication::translate("CppTools::QuickFix",
                                               "Convert connect() to Qt 5 Style"));
    }

private:
    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());
        currentFile->setChangeSet(m_changes);
        currentFile->apply();
    }

    const ChangeSet m_changes;
};

Symbol *skipForwardDeclarations(const QList<Symbol *> &symbols)
{
    foreach (Symbol *symbol, symbols) {
        if (!symbol->type()->isForwardClassDeclarationType())
            return symbol;
    }

    return 0;
}

bool findRawAccessFunction(Class *klass, PointerType *pointerType, QString *objAccessFunction)
{
    QList<Function *> candidates;
    for (auto it = klass->memberBegin(), end = klass->memberEnd(); it != end; ++it) {
        if (Function *func = (*it)->asFunction()) {
            const Name *funcName = func->name();
            if (!funcName->isOperatorNameId()
                    && !funcName->isConversionNameId()
                    && func->returnType().type() == pointerType
                    && func->isConst()
                    && func->argumentCount() == 0) {
                candidates << func;
            }
        }
    }
    const Name *funcName = 0;
    switch (candidates.size()) {
    case 0:
        return false;
    case 1:
        funcName = candidates.first()->name();
        break;
    default:
        // Multiple candidates - prefer a function named data
        foreach (Function *func, candidates) {
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
        return 0;
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

    return 0;
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
    QTC_ASSERT(objectPointerExpressions.size() == 1, return 0);

    Type *objectPointerTypeBase = objectPointerExpressions.first().type().type();
    QTC_ASSERT(objectPointerTypeBase, return 0);

    PointerType *objectPointerType = objectPointerTypeBase->asPointerType();
    if (!objectPointerType) {
        objectPointerType = determineConvertedType(objectPointerTypeBase->asNamedType(), context,
                                               objectPointerScope, objAccessFunction);
    }
    QTC_ASSERT(objectPointerType, return 0);

    Type *objectTypeBase = objectPointerType->elementType().type(); // Dereference
    QTC_ASSERT(objectTypeBase, return 0);

    NamedType *objectType = objectTypeBase->asNamedType();
    QTC_ASSERT(objectType, return 0);

    ClassOrNamespace *objectClassCON = context.lookupType(objectType->name(), objectPointerScope);
    QTC_ASSERT(objectClassCON, return 0);
    QTC_ASSERT(!objectClassCON->symbols().isEmpty(), return 0);

    Symbol *objectClassSymbol = skipForwardDeclarations(objectClassCON->symbols());
    QTC_ASSERT(objectClassSymbol, return 0);

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
    if (!idExpr)
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
        *arg3 = 0; // Means 'this'
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

        result.append(new ConvertQt4ConnectOperation(interface, changes));
        return;
    }
}

} // namespace Internal
} // namespace CppEditor
