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

#include "cppinsertdecldef.h"

#include <AST.h>
#include <ASTVisitor.h>
#include <CoreTypes.h>
#include <Names.h>
#include <Symbols.h>
#include <TranslationUnit.h>
#include <cplusplus/ASTPath.h>
#include <cplusplus/InsertionPointLocator.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>

#include <QtCore/QCoreApplication>

using namespace CPlusPlus;
using namespace CppEditor;
using namespace CppEditor::Internal;
using namespace CppTools;

using CppEditor::CppRefactoringChanges;

namespace {

class InsertDeclOperation: public CppQuickFixOperation
{
public:
    InsertDeclOperation(const CppQuickFixState &state, int priority,
                        const QString &targetFileName, const Class *targetSymbol,
                        InsertionPointLocator::AccessSpec xsSpec,
                        const QString &decl)
        : CppQuickFixOperation(state, priority)
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

        setDescription(QCoreApplication::tr("Add %1 declaration",
                                            "CppEditor::DeclFromDef").arg(type));
    }

    void performChanges(CppRefactoringFile *, CppRefactoringChanges *refactoring)
    {
        InsertionPointLocator locator(state().snapshot());
        const InsertionLocation loc = locator.methodDeclarationInClass(
                    m_targetFileName, m_targetSymbol, m_xsSpec);
        Q_ASSERT(loc.isValid());

        CppRefactoringFile targetFile = refactoring->file(m_targetFileName);
        int targetPosition1 = targetFile.position(loc.line(), loc.column());
        int targetPosition2 = qMax(0, targetFile.position(loc.line(), 1) - 1);

        Utils::ChangeSet target;
        target.insert(targetPosition1, loc.prefix() + m_decl);
        targetFile.change(target);
        targetFile.indent(Utils::ChangeSet::Range(targetPosition2, targetPosition1));

        const int prefixLineCount = loc.prefix().count(QLatin1Char('\n'));
        refactoring->activateEditor(m_targetFileName, loc.line() + prefixLineCount,
                                    qMax(((int) loc.column()) - 1, 0));
    }

private:
    QString m_targetFileName;
    const Class *m_targetSymbol;
    InsertionPointLocator::AccessSpec m_xsSpec;
    QString m_decl;
};

} // anonymous namespace

QList<CppQuickFixOperation::Ptr> DeclFromDef::match(const CppQuickFixState &state)
{
    const QList<AST *> &path = state.path();
    const CppRefactoringFile &file = state.currentFile();

    FunctionDefinitionAST *funDef = 0;
    int idx = 0;
    for (; idx < path.size(); ++idx) {
        AST *node = path.at(idx);
        if (idx > 1) {
            if (DeclaratorIdAST *declId = node->asDeclaratorId()) {
                if (file.isCursorOn(declId)) {
                    if (FunctionDefinitionAST *candidate = path.at(idx - 2)->asFunctionDefinition()) {
                        if (funDef) {
                            return noResult();
                        } else {
                            funDef = candidate;
                            break;
                        }
                    }
                }
            }
        }

        if (node->asClassSpecifier()) {
            return noResult();
        }
    }

    if (!funDef || !funDef->symbol)
        return noResult();

    Function *method = funDef->symbol;

    if (ClassOrNamespace *targetBinding = state.context().lookupParent(method)) {
        foreach (Symbol *s, targetBinding->symbols()) {
            if (Class *clazz = s->asClass()) {
                QList<CppQuickFixOperation::Ptr> results;
                const QLatin1String fn(clazz->fileName());
                const QString decl = generateDeclaration(state,
                                                         method,
                                                         targetBinding);
                results.append(
                            singleResult(
                                new InsertDeclOperation(state, idx, fn, clazz,
                                                        InsertionPointLocator::Public,
                                                        decl)));
                results.append(
                            singleResult(
                                new InsertDeclOperation(state, idx, fn, clazz,
                                                        InsertionPointLocator::Protected,
                                                        decl)));
                results.append(
                            singleResult(
                                new InsertDeclOperation(state, idx, fn, clazz,
                                                        InsertionPointLocator::Private,
                                                        decl)));
                results.append(
                            singleResult(
                                new InsertDeclOperation(state, idx, fn, clazz,
                                                        InsertionPointLocator::PublicSlot,
                                                        decl)));
                results.append(
                            singleResult(
                                new InsertDeclOperation(state, idx, fn, clazz,
                                                        InsertionPointLocator::ProtectedSlot,
                                                        decl)));
                results.append(
                            singleResult(
                                new InsertDeclOperation(state, idx, fn, clazz,
                                                        InsertionPointLocator::PrivateSlot,
                                                        decl)));
                return results;
            } //! \todo support insertion of non-methods into namespaces
        }
    }

    return noResult();
}

QString DeclFromDef::generateDeclaration(const CppQuickFixState &,
                                         Function *method,
                                         ClassOrNamespace *targetBinding)
{
    Q_UNUSED(targetBinding);

    Overview oo;
    oo.setShowFunctionSignatures(true);
    oo.setShowReturnTypes(true);
    oo.setShowArgumentNames(true);

    QString decl;
    decl += oo(method->type(), method->unqualifiedName());
    decl += QLatin1String(";\n");

    return decl;
}

namespace {

static inline bool hasFunctionType(DeclarationAST *decl)
{
    if (decl->asFunctionDefinition())
        return true;

    if (SimpleDeclarationAST *ast = decl->asSimpleDeclaration())
        if (ast->symbols && ast->symbols->value && !ast->symbols->next)
            if (Declaration *decl = ast->symbols->value->asDeclaration())
                if (FullySpecifiedType ty = decl->type())
                    return ty->asFunctionType();

    return false;
}

static QPair<DeclarationAST *, DeclarationAST *> findSurroundingDeclarations(
    DeclarationListAST *members,
    DeclarationAST *decl)
{
    bool found = false;
    DeclarationAST *last = 0, *next = 0, *prev = 0;
    DeclarationListAST *iter = members;
    for (; iter; iter = iter->next) {
        DeclarationAST *value = iter->value;
        if (value == decl) {
            prev = last;
            found = true;
        } else if (hasFunctionType(value)) {
            if (found) {
                next = value;
                break;
            } else {
                last = value;
            }
        }
    }

    return qMakePair(prev, next);
}

} // anonymous namespace

QList<CppQuickFixOperation::Ptr> DefFromDecl::match(const CppQuickFixState &state)
{
#if 0
    qDebug() << Q_FUNC_INFO;

    const QList<AST *> &path = state.path();
    const CppRefactoringFile &file = state.currentFile();

    DeclaratorAST *declAST = 0;
    ClassSpecifierAST *classSpec = 0;
    int idx = path.size() - 1;
    for (; idx >= 0; --idx) {
        AST *node = path.at(idx);
        if (ClassSpecifierAST *clazz = node->asClassSpecifier()) {
            classSpec = clazz;
            continue;
        }

        if (idx <= 1) continue;
        DeclaratorIdAST *declId = node->asDeclaratorId();
        if (!declId) continue;

        if (!file.isCursorOn(declId)) continue;

        DeclaratorAST *candidate = path.at(idx - 1)->asDeclarator();
        if (!candidate) continue;
        if (!candidate->postfix_declarator_list) continue;
        if (!candidate->postfix_declarator_list->value) continue;
        if (candidate->postfix_declarator_list->next) continue;
        FunctionDeclaratorAST *funDecl = candidate->postfix_declarator_list->value->asFunctionDeclarator();
        if (!funDecl) continue;
        if (funDecl->symbol->asFunctionType())
            declAST = candidate;
    }

    if (!declAST || !classSpec || !classSpec->symbol)
        return noResult();

    if (!declAST->symbols || !declAST->symbols->value || declAST->symbols->next)
        return noResult();

    Declaration *decl = declAST->symbols->value->asDeclaration();
    if (!decl)
        return noResult();

    Function *funTy = decl->type()->asFunctionType();
    if (!funTy)
        return noResult();

    qDebug() << "-> Found funTy.";

    QPair<DeclarationAST *, DeclarationAST *> surroundingNodes =
            findSurroundingDeclarations(classSpec->member_specifier_list, declAST);
    qDebug() << "->("<<surroundingNodes.first<<","<<surroundingNodes.second<<")";
    if (surroundingNodes.first)
        if (SimpleDeclarationAST *sd = surroundingNodes.first->asSimpleDeclaration())
            qDebug()<<"-->prev@"<<sd->symbols->value->line()<<sd->symbols->value->column();
    if (surroundingNodes.second)
        if (SimpleDeclarationAST *sd=surroundingNodes.second->asSimpleDeclaration())
            qDebug()<<"-->next@"<<sd->symbols->value->line()<<sd->symbols->value->column();
#endif
//    if (ClassOrNamespace *targetBinding = state.context().lookupParent(method)) {
//        foreach (Symbol *s, targetBinding->symbols()) {
//            if (Class *clazz = s->asClass()) {
//                QList<CppQuickFixOperation::Ptr> results;
//                const QLatin1String fn(clazz->fileName());
//                const QString decl = generateDeclaration(state,
//                                                         method,
//                                                         targetBinding);
//                results.append(
//                            singleResult(
//                                new InsertDeclOperation(state, idx, fn, clazz,
//                                                        InsertionPointLocator::Public,
//                                                        decl)));
//                results.append(
//                            singleResult(
//                                new InsertDeclOperation(state, idx, fn, clazz,
//                                                        InsertionPointLocator::Protected,
//                                                        decl)));
//                results.append(
//                            singleResult(
//                                new InsertDeclOperation(state, idx, fn, clazz,
//                                                        InsertionPointLocator::Private,
//                                                        decl)));
//                results.append(
//                            singleResult(
//                                new InsertDeclOperation(state, idx, fn, clazz,
//                                                        InsertionPointLocator::PublicSlot,
//                                                        decl)));
//                results.append(
//                            singleResult(
//                                new InsertDeclOperation(state, idx, fn, clazz,
//                                                        InsertionPointLocator::ProtectedSlot,
//                                                        decl)));
//                results.append(
//                            singleResult(
//                                new InsertDeclOperation(state, idx, fn, clazz,
//                                                        InsertionPointLocator::PrivateSlot,
//                                                        decl)));
//                return results;
//            } //! \todo support insertion into namespaces
//        }
//    }

    return noResult();
}

QString DefFromDecl::generateDefinition(const CppQuickFixState &,
                                        Function *method,
                                        ClassOrNamespace *targetBinding)
{
    Q_UNUSED(targetBinding);

    Overview oo;
    oo.setShowFunctionSignatures(true);
    oo.setShowReturnTypes(true);
    oo.setShowArgumentNames(true);

    QString decl;
    decl += oo(method->type(), method->unqualifiedName());
    decl += QLatin1String(";\n");

    return decl;
}
