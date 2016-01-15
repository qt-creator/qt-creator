/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "cpppointerdeclarationformatter.h"

#include <cplusplus/Overview.h>

#include <QDebug>
#include <QTextCursor>

#define DEBUG_OUTPUT 0

#if DEBUG_OUTPUT
#  include <typeinfo>
#  ifdef __GNUC__
#    include <cxxabi.h>
#  endif
#endif

#define CHECK_RV(cond, err, r) \
    if (!(cond)) { if (DEBUG_OUTPUT) qDebug() << "Discarded:" << (err); return r; }
#define CHECK_R(cond, err) \
    if (!(cond)) { if (DEBUG_OUTPUT) qDebug() << "Discarded:" << (err); return; }
#define CHECK_C(cond, err) \
    if (!(cond)) { if (DEBUG_OUTPUT) qDebug() << "Discarded:" << (err); continue; }

using namespace CppTools;

/*!
   Skips specifiers that are not type relevant and returns the index of the
          first specifier token which is not followed by __attribute__
          ((T___ATTRIBUTE__)).

   This is used to get 'correct' start of the activation range in
   simple declarations.

   Consider these cases:

    \list
        \li \c {static char *s = 0;}
        \li \c {typedef char *s cp;}
        \li \c {__attribute__((visibility("default"))) char *f();}
    \endlist

   For all these cases we want to skip all the specifiers that are not type
   relevant
   (since these are not part of the type and thus are not rewritten).

   \a list is the specifier list to iterate and \a translationUnit is the
   translation unit.
   \a endToken is the last token to check.
   \a found is an output parameter that must not be 0.
 */
static unsigned firstTypeSpecifierWithoutFollowingAttribute(
    SpecifierListAST *list, TranslationUnit *translationUnit, unsigned endToken, bool *found)
{
    *found = false;
    if (!list || !translationUnit || !endToken)
        return 0;

    for (SpecifierListAST *it = list; it; it = it->next) {
        SpecifierAST *specifier = it->value;
        CHECK_RV(specifier, "No specifier", 0);
        const unsigned index = specifier->firstToken();
        CHECK_RV(index < endToken, "EndToken reached", 0);

        const int tokenKind = translationUnit->tokenKind(index);
        switch (tokenKind) {
        case T_VIRTUAL:
        case T_INLINE:
        case T_FRIEND:
        case T_REGISTER:
        case T_STATIC:
        case T_EXTERN:
        case T_MUTABLE:
        case T_TYPEDEF:
        case T_CONSTEXPR:
        case T___ATTRIBUTE__:
            continue;
        default:
            // Check if attributes follow
            for (unsigned i = index; i <= endToken; ++i) {
                const int tokenKind = translationUnit->tokenKind(i);
                if (tokenKind == T___ATTRIBUTE__)
                    return 0;
            }
            *found = true;
            return index;
        }
    }

    return 0;
}

PointerDeclarationFormatter::PointerDeclarationFormatter(
        const CppRefactoringFilePtr &refactoringFile,
        Overview &overview,
        CursorHandling cursorHandling)
    : ASTVisitor(refactoringFile->cppDocument()->translationUnit())
    , m_cppRefactoringFile(refactoringFile)
    , m_overview(overview)
    , m_cursorHandling(cursorHandling)
{}

/*!
    Handle
      (1) Simple declarations like in "char *s, *t, *int foo();"
      (2) Return types of function declarations.
 */
bool PointerDeclarationFormatter::visit(SimpleDeclarationAST *ast)
{
    CHECK_RV(ast, "Invalid AST", true);
    printCandidate(ast);

    const unsigned tokenKind = tokenAt(ast->firstToken()).kind();
    const bool astIsOk = tokenKind != T_CLASS && tokenKind != T_STRUCT && tokenKind != T_ENUM;
    CHECK_RV(astIsOk, "Nothing to do for class/struct/enum", true);

    DeclaratorListAST *declaratorList = ast->declarator_list;
    CHECK_RV(declaratorList, "No declarator list", true);
    DeclaratorAST *firstDeclarator = declaratorList->value;
    CHECK_RV(firstDeclarator, "No declarator", true);
    CHECK_RV(ast->symbols, "No Symbols", true);
    CHECK_RV(ast->symbols->value, "No Symbol", true);

    List<Symbol *> *sit = ast->symbols;
    DeclaratorListAST *dit = declaratorList;
    for (; sit && dit; sit = sit->next, dit = dit->next) {
        DeclaratorAST *declarator = dit->value;
        Symbol *symbol = sit->value;

        const bool isFirstDeclarator = declarator == firstDeclarator;

        // If were not handling the first declarator, we need to remove
        // characters from the beginning since our rewritten declaration
        // will contain all type specifiers.
        int charactersToRemove = 0;
        if (!isFirstDeclarator) {
            const int startAST = m_cppRefactoringFile->startOf(ast);
            const int startFirstDeclarator = m_cppRefactoringFile->startOf(firstDeclarator);
            CHECK_RV(startAST < startFirstDeclarator, "No specifier", true);
            charactersToRemove = startFirstDeclarator - startAST;
        }

        // Specify activation range
        int lastActivationToken = 0;
        TokenRange range;
        // (2) Handle function declaration's return type
        if (symbol->type()->asFunctionType()) {
            PostfixDeclaratorListAST *pfDeclaratorList = declarator->postfix_declarator_list;
            CHECK_RV(pfDeclaratorList, "No postfix declarator list", true);
            PostfixDeclaratorAST *pfDeclarator = pfDeclaratorList->value;
            CHECK_RV(pfDeclarator, "No postfix declarator", true);
            FunctionDeclaratorAST *functionDeclarator = pfDeclarator->asFunctionDeclarator();
            CHECK_RV(functionDeclarator, "No function declarator", true);
            // End the activation range before the '(' token.
            lastActivationToken = functionDeclarator->lparen_token - 1;

            SpecifierListAST *specifierList = isFirstDeclarator
                ? ast->decl_specifier_list
                : declarator->attribute_list;

            unsigned firstActivationToken = 0;
            bool foundBegin = false;
            firstActivationToken = firstTypeSpecifierWithoutFollowingAttribute(
                        specifierList,
                        m_cppRefactoringFile->cppDocument()->translationUnit(),
                        lastActivationToken,
                        &foundBegin);
            if (!foundBegin) {
                CHECK_RV(!isFirstDeclarator, "Declaration without attributes not supported", true);
                firstActivationToken = declarator->firstToken();
            }

            range.start = firstActivationToken;

        // (1) Handle 'normal' declarations.
        } else {
            if (isFirstDeclarator) {
                bool foundBegin = false;
                unsigned firstActivationToken = firstTypeSpecifierWithoutFollowingAttribute(
                            ast->decl_specifier_list,
                            m_cppRefactoringFile->cppDocument()->translationUnit(),
                            declarator->firstToken(),
                            &foundBegin);
                CHECK_RV(foundBegin, "Declaration without attributes not supported", true);
                range.start = firstActivationToken;
            } else {
                range.start = declarator->firstToken();
            }
            lastActivationToken = declarator->equal_token
                ? declarator->equal_token - 1
                : declarator->lastToken() - 1;
        }

        range.end = lastActivationToken;

        checkAndRewrite(declarator, symbol, range, charactersToRemove);
    }
    return true;
}

/*! Handle return types of function definitions */
bool PointerDeclarationFormatter::visit(FunctionDefinitionAST *ast)
{
    CHECK_RV(ast, "Invalid AST", true);
    printCandidate(ast);

    DeclaratorAST *declarator = ast->declarator;
    CHECK_RV(declarator, "No declarator", true);
    CHECK_RV(declarator->ptr_operator_list, "No Pointer or references", true);
    Symbol *symbol = ast->symbol;

    PostfixDeclaratorListAST *pfDeclaratorList = declarator->postfix_declarator_list;
    CHECK_RV(pfDeclaratorList, "No postfix declarator list", true);
    PostfixDeclaratorAST *pfDeclarator = pfDeclaratorList->value;
    CHECK_RV(pfDeclarator, "No postfix declarator", true);
    FunctionDeclaratorAST *functionDeclarator = pfDeclarator->asFunctionDeclarator();
    CHECK_RV(functionDeclarator, "No function declarator", true);

    // Specify activation range
    bool foundBegin = false;
    const unsigned lastActivationToken = functionDeclarator->lparen_token - 1;
    const unsigned firstActivationToken = firstTypeSpecifierWithoutFollowingAttribute(
        ast->decl_specifier_list,
        m_cppRefactoringFile->cppDocument()->translationUnit(),
        lastActivationToken,
        &foundBegin);
    CHECK_RV(foundBegin, "Declaration without attributes not supported", true);
    TokenRange range(firstActivationToken, lastActivationToken);

    checkAndRewrite(declarator, symbol, range);
    return true;
}

/*! Handle parameters in function declarations and definitions */
bool PointerDeclarationFormatter::visit(ParameterDeclarationAST *ast)
{
    CHECK_RV(ast, "Invalid AST", true);
    printCandidate(ast);

    DeclaratorAST *declarator = ast->declarator;
    CHECK_RV(declarator, "No declarator", true);
    CHECK_RV(declarator->ptr_operator_list, "No Pointer or references", true);
    Symbol *symbol = ast->symbol;

    // Specify activation range
    const int lastActivationToken = ast->equal_token
        ? ast->equal_token - 1
        : ast->lastToken() - 1;
    TokenRange range(ast->firstToken(), lastActivationToken);

    checkAndRewrite(declarator, symbol, range);
    return true;
}

/*! Handle declaration in foreach statement */
bool PointerDeclarationFormatter::visit(ForeachStatementAST *ast)
{
    CHECK_RV(ast, "Invalid AST", true);
    printCandidate(ast);

    DeclaratorAST *declarator = ast->declarator;
    CHECK_RV(declarator, "No declarator", true);
    CHECK_RV(declarator->ptr_operator_list, "No Pointer or references", true);
    CHECK_RV(ast->type_specifier_list, "No type specifier", true);
    SpecifierAST *firstSpecifier = ast->type_specifier_list->value;
    CHECK_RV(firstSpecifier, "No first type specifier", true);
    CHECK_RV(ast->symbol, "No symbol", true);
    Symbol *symbol = ast->symbol->memberAt(0);

    // Specify activation range
    const int lastActivationToken = declarator->equal_token
        ? declarator->equal_token - 1
        : declarator->lastToken() - 1;
    TokenRange range(firstSpecifier->firstToken(), lastActivationToken);

    checkAndRewrite(declarator, symbol, range);
    return true;
}

bool PointerDeclarationFormatter::visit(IfStatementAST *ast)
{
    CHECK_RV(ast, "Invalid AST", true);
    printCandidate(ast);
    processIfWhileForStatement(ast->condition, ast->symbol);
    return true;
}

bool PointerDeclarationFormatter::visit(WhileStatementAST *ast)
{
    CHECK_RV(ast, "Invalid AST", true);
    printCandidate(ast);
    processIfWhileForStatement(ast->condition, ast->symbol);
    return true;
}

bool PointerDeclarationFormatter::visit(ForStatementAST *ast)
{
    CHECK_RV(ast, "Invalid AST", true);
    printCandidate(ast);
    processIfWhileForStatement(ast->condition, ast->symbol);
    return true;
}

/*! Handle declaration in if, while and for statements */
void PointerDeclarationFormatter::processIfWhileForStatement(ExpressionAST *expression,
                                                             Symbol *statementSymbol)
{
    CHECK_R(expression, "No expression");
    CHECK_R(statementSymbol, "No symbol");

    ConditionAST *condition = expression->asCondition();
    CHECK_R(condition, "No condition");
    DeclaratorAST *declarator = condition->declarator;
    CHECK_R(declarator, "No declarator");
    CHECK_R(declarator->ptr_operator_list, "No Pointer or references");
    CHECK_R(declarator->equal_token, "No equal token");
    Block *block = statementSymbol->asBlock();
    CHECK_R(block, "No block");
    CHECK_R(block->memberCount() > 0, "No block members");

    // Get the right symbol
    //
    // This is especially important for e.g.
    //
    //    for (char *s = 0; char *t = 0;) {}
    //
    // The declaration for 's' will be handled in visit(SimpleDeclarationAST *ast),
    // so handle declaration for 't' here.
    Scope::iterator it = block->memberEnd() - 1;
    Symbol *symbol = *it;
    if (symbol && symbol->asScope()) { // True if there is a  "{ ... }" following.
        --it;
        symbol = *it;
    }

    // Specify activation range
    TokenRange range(condition->firstToken(), declarator->equal_token - 1);

    checkAndRewrite(declarator, symbol, range);
}

/*!
    Performs some further checks and rewrites the type and name of \a symbol
    into the substitution range in the file specified by \a tokenRange.
 */
void PointerDeclarationFormatter::checkAndRewrite(DeclaratorAST *declarator,
                                                  Symbol *symbol,
                                                  TokenRange tokenRange,
                                                  unsigned charactersToRemove)
{
    CHECK_R(tokenRange.end > 0, "TokenRange invalid1");
    CHECK_R(tokenRange.start < tokenRange.end, "TokenRange invalid2");
    CHECK_R(symbol, "No symbol");

    // Check for expanded tokens
    for (unsigned token = tokenRange.start; token <= tokenRange.end; ++token)
        CHECK_R(!tokenAt(token).expanded(), "Token is expanded");

    Utils::ChangeSet::Range range(m_cppRefactoringFile->startOf(tokenRange.start),
                                  m_cppRefactoringFile->endOf(tokenRange.end));

    CHECK_R(range.start >= 0 && range.end > 0, "ChangeRange invalid1");
    CHECK_R(range.start < range.end, "ChangeRange invalid2");

    // Check range with respect to cursor position / selection
    if (m_cursorHandling == RespectCursor) {
        const QTextCursor cursor = m_cppRefactoringFile->cursor();
        if (cursor.hasSelection()) {
            CHECK_R(cursor.selectionStart() <= range.start, "Change not in selection range");
            CHECK_R(range.end <= cursor.selectionEnd(), "Change not in selection range");
        } else {
            CHECK_R(range.start <= cursor.selectionStart(), "Cursor before activation range");
            CHECK_R(cursor.selectionEnd() <= range.end, "Cursor after activation range");
        }
    }

    FullySpecifiedType type = symbol->type();
    if (Function *function = type->asFunctionType())
        type = function->returnType();

    // Check if pointers or references are involved
    const QString originalDeclaration = m_cppRefactoringFile->textOf(range);
    CHECK_R(originalDeclaration.contains(QLatin1Char('&'))
            || originalDeclaration.contains(QLatin1Char('*')), "No pointer or references");

    // Does the rewritten declaration (part) differs from the original source (part)?
    QString rewrittenDeclaration;
    const Name *name = symbol->name();
    if (name) {
        if (name->isOperatorNameId()
                || (name->isQualifiedNameId()
                    && name->asQualifiedNameId()->name()->isOperatorNameId())) {
            const QString operatorText = m_cppRefactoringFile->textOf(declarator->core_declarator);
            m_overview.includeWhiteSpaceInOperatorName = operatorText.contains(QLatin1Char(' '));
        }
    }
    rewrittenDeclaration = m_overview.prettyType(type, name);
    rewrittenDeclaration.remove(0, charactersToRemove);

    CHECK_R(originalDeclaration != rewrittenDeclaration, "Rewritten is same as original");
    CHECK_R(rewrittenDeclaration.contains(QLatin1Char('&'))
            || rewrittenDeclaration.contains(QLatin1Char('*')),
            "No pointer or references in rewritten declaration");

    if (DEBUG_OUTPUT) {
        qDebug("==> Rewritten: \"%s\" --> \"%s\"", originalDeclaration.toUtf8().constData(),
               rewrittenDeclaration.toUtf8().constData());
    }

    // Creating the replacement in the changeset may fail due to operations
    // in the changeset that overlap with the current range.
    //
    // Consider this case:
    //
    //    void (*foo)(char * s) = 0;
    //
    // First visit(SimpleDeclarationAST *ast) will be called. It creates a
    // replacement that also includes the parameter.
    // Next visit(ParameterDeclarationAST *ast) is called with the
    // original source. It tries to create an replacement operation
    // at this position and fails due to overlapping ranges (the
    // simple declaration range includes parameter declaration range).
    Utils::ChangeSet change(m_changeSet);
    if (change.replace(range, rewrittenDeclaration))
        m_changeSet = change;
    else if (DEBUG_OUTPUT)
        qDebug() << "Replacement operation failed";
}

void PointerDeclarationFormatter::printCandidate(AST *ast)
{
#if DEBUG_OUTPUT
    QString tokens;
    for (unsigned token = ast->firstToken(); token < ast->lastToken(); token++)
        tokens += QString::fromLatin1(tokenAt(token).spell()) + QLatin1Char(' ');

#  ifdef __GNUC__
    QByteArray name = abi::__cxa_demangle(typeid(*ast).name(), 0, 0, 0) + 11;
    name.truncate(name.length() - 3);
#  else
    QByteArray name = typeid(*ast).name();
#  endif
    qDebug("--> Candidate: %s: %s", name.constData(), qPrintable(tokens));
#else
    Q_UNUSED(ast)
#endif // DEBUG_OUTPUT
}
