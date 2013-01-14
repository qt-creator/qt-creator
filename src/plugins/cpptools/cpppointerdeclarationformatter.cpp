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


#include "cpppointerdeclarationformatter.h"

#include <AST.h>

#include <QTextCursor>

#define DEBUG_OUTPUT 0
#define CHECK_RV(cond, err, r) if (!(cond)) { if (DEBUG_OUTPUT) qDebug() << (err); return r; }
#define CHECK_R(cond, err) if (!(cond)) { if (DEBUG_OUTPUT) qDebug() << (err); return; }
#define CHECK_C(cond, err) if (!(cond)) { if (DEBUG_OUTPUT) qDebug() << (err); continue; }

using namespace CppTools;

/*!
   \brief Skip not type relevant specifiers and return the index of the
          first specifier token which is not followed by __attribute__
          ((T___ATTRIBUTE__)).

   This is used to get 'correct' start of the activation range in
   simple declarations.

   Consider these cases:

        static char *s = 0;
        typedef char *s cp;
        __attribute__((visibility("default"))) char *f();

   For all cases we want to skip all the not type relevant specifer
   (since these are not part of the type and thus are not rewritten).

   \param list The specifier list to iterate
   \param translationUnit The TranslationUnit
   \param endToken Do not check further than this token
   \param found Output parameter, must not be 0.
 */
static unsigned firstTypeSpecifierWithoutFollowingAttribute(
    SpecifierListAST *list, TranslationUnit *translationUnit, unsigned endToken, bool *found)
{
    *found = false;
    if (! list || ! translationUnit || ! endToken)
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
        const CppRefactoringFilePtr refactoringFile,
        const Overview &overview,
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
        if (! isFirstDeclarator) {
            const int startAST = m_cppRefactoringFile->startOf(ast);
            const int startFirstDeclarator = m_cppRefactoringFile->startOf(firstDeclarator);
            CHECK_RV(startAST < startFirstDeclarator, "No specifier", true);
            charactersToRemove = startFirstDeclarator - startAST;
        }

        // Specify activation range
        int lastActivationToken = 0;
        Range range;
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
            if (! foundBegin) {
                CHECK_RV(! isFirstDeclarator, "Declaration without attributes not supported", true);
                firstActivationToken = declarator->firstToken();
            }

            range.start = m_cppRefactoringFile->startOf(firstActivationToken);

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
                range.start = m_cppRefactoringFile->startOf(firstActivationToken);
            } else {
                range.start = m_cppRefactoringFile->startOf(declarator);
            }
            lastActivationToken = declarator->equal_token
                ? declarator->equal_token - 1
                : declarator->lastToken() - 1;
        }
        range.end = m_cppRefactoringFile->endOf(lastActivationToken);

        checkAndRewrite(symbol, range, charactersToRemove);
    }
    return true;
}

/*! Handle return types of function definitions */
bool PointerDeclarationFormatter::visit(FunctionDefinitionAST *ast)
{
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
    Range range(m_cppRefactoringFile->startOf(firstActivationToken),
                m_cppRefactoringFile->endOf(lastActivationToken));

    checkAndRewrite(symbol, range);
    return true;
}

/*! Handle parameters in function declarations and definitions */
bool PointerDeclarationFormatter::visit(ParameterDeclarationAST *ast)
{
    DeclaratorAST *declarator = ast->declarator;
    CHECK_RV(declarator, "No declarator", true);
    CHECK_RV(declarator->ptr_operator_list, "No Pointer or references", true);
    Symbol *symbol = ast->symbol;

    // Specify activation range
    const int lastActivationToken = ast->equal_token
        ? ast->equal_token - 1
        : ast->lastToken() - 1;
    Range range(m_cppRefactoringFile->startOf(ast),
                m_cppRefactoringFile->endOf(lastActivationToken));

    checkAndRewrite(symbol, range);
    return true;
}

/*! Handle declaration in foreach statement */
bool PointerDeclarationFormatter::visit(ForeachStatementAST *ast)
{
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
    Range range(m_cppRefactoringFile->startOf(firstSpecifier),
                m_cppRefactoringFile->endOf(lastActivationToken));

    checkAndRewrite(symbol, range);
    return true;
}

bool PointerDeclarationFormatter::visit(IfStatementAST *ast)
{
    processIfWhileForStatement(ast->condition, ast->symbol);
    return true;
}

bool PointerDeclarationFormatter::visit(WhileStatementAST *ast)
{
    processIfWhileForStatement(ast->condition, ast->symbol);
    return true;
}

bool PointerDeclarationFormatter::visit(ForStatementAST *ast)
{
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
    Scope::iterator it = block->lastMember() - 1;
    Symbol *symbol = *it;
    if (symbol && symbol->asScope()) { // True if there is a  "{ ... }" following.
        --it;
        symbol = *it;
    }

    // Specify activation range
    Range range(m_cppRefactoringFile->startOf(condition),
                m_cppRefactoringFile->endOf(declarator->equal_token - 1));

    checkAndRewrite(symbol, range);
}

/*!
   \brief Do some further checks and rewrite the symbol's type and
          name into the given range

   \param symbol the symbol to be rewritten
   \param range the substitution range in the file
 */
void PointerDeclarationFormatter::checkAndRewrite(Symbol *symbol, Range range,
                                                  unsigned charactersToRemove)
{
    CHECK_R(range.start >= 0 && range.end > 0, "Range invalid");
    CHECK_R(range.start < range.end, "Range invalid");
    CHECK_R(symbol, "No symbol");

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
    QString rewrittenDeclaration = rewriteDeclaration(type, symbol->name());
    rewrittenDeclaration.remove(0, charactersToRemove);
    CHECK_R(originalDeclaration != rewrittenDeclaration, "Rewritten is same as original");

    if (DEBUG_OUTPUT) {
        qDebug("Rewritten: \"%s\" --> \"%s\"", originalDeclaration.toLatin1().constData(),
               rewrittenDeclaration.toLatin1().constData());
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
    ChangeSet change(m_changeSet);
    if (change.replace(range, rewrittenDeclaration))
        m_changeSet = change;
    else if (DEBUG_OUTPUT)
        qDebug() << "Replacement operation failed";
}

/*! Rewrite/format the given type and name. */
QString PointerDeclarationFormatter::rewriteDeclaration(FullySpecifiedType type, const Name *name)
    const
{
    CHECK_RV(type.isValid(), "Invalid type", QString());

    const char *identifier = 0;
    if (const Name *declarationName = name) {
        if (const Identifier *id = declarationName->identifier())
            identifier = id->chars();
    }

    return m_overview.prettyType(type, QLatin1String(identifier));
}
