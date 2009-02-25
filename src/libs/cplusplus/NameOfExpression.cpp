/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "NameOfExpression.h"
#include "LookupContext.h"

#include <cplusplus/Overview.h>
#include <Control.h>
#include <AST.h>
#include <Scope.h>
#include <Names.h>
#include <Symbols.h>
#include <Literals.h>
#include <CoreTypes.h>
#include <TypeVisitor.h>
#include <NameVisitor.h>

#include <QList>
#include <QtDebug>

using namespace CPlusPlus;

NameOfExpression::NameOfExpression(const LookupContext &context)
    : ASTVisitor(context.expressionDocument()->control()),
      _context(context),
      sem(_context.control())
{ }

NameOfExpression::~NameOfExpression()
{ }

QList<FullySpecifiedType> ResolveExpression::operator()(ExpressionAST *ast)
{
    QList<FullySpecifiedType> previousResolvedSymbols = switchResolvedSymbols(QList<FullySpecifiedType>());
    accept(ast);
    return switchResolvedSymbols(previousResolvedSymbols);
}

QList<FullySpecifiedType> ResolveExpression::switchResolvedSymbols(const QList<FullySpecifiedType> &symbols)
{
    QList<FullySpecifiedType> previousResolvedSymbols = _resolvedSymbols;
    _resolvedSymbols = symbols;
    return previousResolvedSymbols;
}

bool ResolveExpression::visit(ExpressionListAST *)
{
    // nothing to do.
    return false;
}

bool ResolveExpression::visit(BinaryExpressionAST *)
{
    // nothing to do.
    return false;
}

bool ResolveExpression::visit(CastExpressionAST *)
{
    // nothing to do.
    return false;
}

bool ResolveExpression::visit(ConditionAST *)
{
    // nothing to do.
    return false;
}

bool ResolveExpression::visit(ConditionalExpressionAST *)
{
    // nothing to do.
    return false;
}

bool ResolveExpression::visit(CppCastExpressionAST *)
{
    // ### resolve ast->type_id
    return false;
}

bool ResolveExpression::visit(DeleteExpressionAST *)
{
    // nothing to do.
    return false;
}

bool ResolveExpression::visit(ArrayInitializerAST *)
{
    // nothing to do.
    return false;
}

bool ResolveExpression::visit(NewExpressionAST *)
{
    // nothing to do.
    return false;
}

bool ResolveExpression::visit(TypeidExpressionAST *)
{
    // nothing to do.
    return false;
}

bool ResolveExpression::visit(TypenameCallExpressionAST *)
{
    // nothing to do
    return false;
}

bool ResolveExpression::visit(TypeConstructorCallAST *)
{
    // nothing to do.
    return false;
}

bool ResolveExpression::visit(PostfixExpressionAST *ast)
{
    accept(ast->base_expression);

    for (PostfixAST *fx = ast->postfix_expressions; fx; fx = fx->next) {
        accept(fx);
    }

    return false;
}

bool ResolveExpression::visit(SizeofExpressionAST *)
{
    FullySpecifiedType ty(control()->integerType(IntegerType::Int));
    ty.setUnsigned(true);
    _resolvedSymbols.append(ty);
    return false;
}

bool ResolveExpression::visit(NumericLiteralAST *)
{
    _resolvedSymbols.append(control()->integerType(IntegerType::Int)); // ### handle short, long, floats, ...
    return false;
}

bool ResolveExpression::visit(BoolLiteralAST *)
{
    _resolvedSymbols.append(control()->integerType(IntegerType::Bool));
    return false;
}

bool ResolveExpression::visit(ThisExpressionAST *)
{
    if (! _context.symbol())
        return false;

    Scope *scope = _context.symbol()->scope();
    for (; scope; scope = scope->enclosingScope()) {
        if (scope->isFunctionScope()) {
            Function *fun = scope->owner()->asFunction();
            if (Scope *cscope = scope->enclosingClassScope()) {
                Class *klass = cscope->owner()->asClass();
                FullySpecifiedType classTy(control()->namedType(klass->name()));
                FullySpecifiedType ptrTy(control()->pointerType(classTy));
                _resolvedSymbols.append(ptrTy);
                break;
            } else if (QualifiedNameId *q = fun->name()->asQualifiedNameId()) {
                Name *nestedNameSpecifier = 0;
                if (q->nameCount() == 2)
                    nestedNameSpecifier = q->nameAt(0);
                else
                    nestedNameSpecifier = control()->qualifiedNameId(&q->names()[0], q->nameCount() - 1);
                FullySpecifiedType classTy(control()->namedType(nestedNameSpecifier));
                FullySpecifiedType ptrTy(control()->pointerType(classTy));
                _resolvedSymbols.append(ptrTy);
                break;
            }
        }
    }
    return false;
}

bool ResolveExpression::visit(NestedExpressionAST *ast)
{
    accept(ast->expression);
    return false;
}

bool ResolveExpression::visit(StringLiteralAST *)
{
    FullySpecifiedType charTy = control()->integerType(IntegerType::Char);
    charTy.setConst(true);
    FullySpecifiedType ty(control()->pointerType(charTy));
    _resolvedSymbols.append(ty);
    return false;
}

bool ResolveExpression::visit(ThrowExpressionAST *)
{
    return false;
}

bool ResolveExpression::visit(TypeIdAST *)
{
    return false;
}

bool ResolveExpression::visit(UnaryExpressionAST *ast)
{
    accept(ast->expression);
    unsigned unaryOp = tokenKind(ast->unary_op_token);
    if (unaryOp == T_AMPER) {
        QMutableListIterator<FullySpecifiedType> it(_resolvedSymbols);
        while (it.hasNext()) {
            FullySpecifiedType ty = it.next();
            ty.setType(control()->pointerType(ty));
            it.setValue(ty);
        }
    } else if (unaryOp == T_STAR) {
        QMutableListIterator<FullySpecifiedType> it(_resolvedSymbols);
        while (it.hasNext()) {
            FullySpecifiedType ty = it.next();
            if (PointerType *ptrTy = ty->asPointerType()) {
                it.setValue(ptrTy->elementType());
            } else {
                it.remove();
            }
        }
    }
    return false;
}

bool ResolveExpression::visit(QualifiedNameAST *ast)
{
    Scope dummy;
    Name *name = sem.check(ast, &dummy);

    QList<Symbol *> symbols = _context.resolve(name);
    foreach (Symbol *symbol, symbols) {
        if (symbol->isTypedef()) {
            if (NamedType *namedTy = symbol->type()->asNamedType()) {
                LookupContext symbolContext(symbol, _context);
                QList<Symbol *> resolvedClasses = symbolContext.resolveClass(namedTy->name());
                if (resolvedClasses.count()) {
                    foreach (Symbol *s, resolvedClasses) {
                        _resolvedSymbols.append(s->type());
                    }
                    continue;
                }
            }
        }
        _resolvedSymbols.append(symbol->type());
    }
    return false;
}

bool ResolveExpression::visit(OperatorFunctionIdAST *)
{
    return false;
}

bool ResolveExpression::visit(ConversionFunctionIdAST *)
{
    return false;
}

bool ResolveExpression::visit(SimpleNameAST *ast)
{
    Scope dummy;
    Name *name = sem.check(ast, &dummy);

    QList<Symbol *> symbols = _context.resolve(name);
    foreach (Symbol *symbol, symbols)
        _resolvedSymbols.append(symbol->type());

    return false;
}

bool ResolveExpression::visit(DestructorNameAST *)
{
    FullySpecifiedType ty(control()->voidType());
    _resolvedSymbols.append(ty);
    return false;
}

bool ResolveExpression::visit(TemplateIdAST *ast)
{
    Scope dummy;
    Name *name = sem.check(ast, &dummy);

    QList<Symbol *> symbols = _context.resolve(name);
    foreach (Symbol *symbol, symbols)
        _resolvedSymbols.append(symbol->type());

    return false;
}

bool ResolveExpression::visit(CallAST *)
{
    QMutableListIterator<FullySpecifiedType> it(_resolvedSymbols);
    while (it.hasNext()) {
        FullySpecifiedType ty = it.next();
        if (Function *funTy = ty->asFunction()) {
            it.setValue(funTy->returnType());
        } else {
            it.remove();
        }
    }
    return false;
}

bool ResolveExpression::visit(ArrayAccessAST * /* ast */)
{
    QMutableListIterator<FullySpecifiedType> it(_resolvedSymbols);
    while (it.hasNext()) {
        FullySpecifiedType ty = it.next();
        if (PointerType *ptrTy = ty->asPointerType()) {
            it.setValue(ptrTy->elementType());
        } else {
            it.remove();
        }
    }
    return false;
}

bool ResolveExpression::visit(MemberAccessAST *ast)
{
    Scope dummy;
    Name *memberName = sem.check(ast->member_name, &dummy);
    unsigned accessOp = tokenKind(ast->access_token);

    Overview overview;

    QList<FullySpecifiedType> candidates = _resolvedSymbols;
    _resolvedSymbols.clear();

    foreach (FullySpecifiedType ty, candidates) {
        NamedType *namedTy = 0;

        if (accessOp == T_ARROW) {
            if (PointerType *ptrTy = ty->asPointerType())
                namedTy = ptrTy->elementType()->asNamedType();
        } else if (accessOp == T_DOT) {
            if (ReferenceType *refTy = ty->asReferenceType())
                ty = refTy->elementType();
            namedTy = ty->asNamedType();
            if (! namedTy) {
                Function *fun = ty->asFunction();
                if (fun && (fun->scope()->isBlockScope() || fun->scope()->isNamespaceScope()))
                    namedTy = fun->returnType()->asNamedType();
            }
        }

        if (namedTy) {
            QList<Symbol *> symbols = _context.resolveClass(namedTy->name());
            if (symbols.isEmpty())
                return false;

            Class *klass = symbols.first()->asClass();
            QList<Scope *> allScopes;
            QSet<Class *> processed;
            QList<Class *> todo;
            todo.append(klass);

            while (! todo.isEmpty()) {
                Class *klass = todo.last();
                todo.removeLast();

                if (processed.contains(klass))
                    continue;

                processed.insert(klass);
                allScopes.append(klass->members());

                for (unsigned i = 0; i < klass->baseClassCount(); ++i) {
                    BaseClass *baseClass = klass->baseClassAt(i);
                    Name *baseClassName = baseClass->name();
                    QList<Symbol *> baseClasses = _context.resolveClass(baseClassName/*, allScopes*/);
                    if (baseClasses.isEmpty())
                        qWarning() << "unresolved base class:" << overview.prettyName(baseClassName);
                    foreach (Symbol *symbol, baseClasses) {
                        todo.append(symbol->asClass());
                    }
                }
            }

            QList<Symbol *> candidates = _context.resolve(memberName, allScopes);
            foreach (Symbol *candidate, candidates) {
                FullySpecifiedType ty = candidate->type();
                if (TemplateNameId *templId = namedTy->name()->asTemplateNameId()) {
                    Substitution subst;
                    for (unsigned i = 0; i < templId->templateArgumentCount(); ++i) {
                        FullySpecifiedType templArgTy = templId->templateArgumentAt(i);
                        if (i < klass->templateParameterCount()) {
                            subst.append(qMakePair(klass->templateParameterAt(i)->name(),
                                                   templArgTy));
                        }
                    }
                    Instantiation inst(control(), subst);
                    ty = inst(ty);
                }
                _resolvedSymbols.append(ty);
            }
        }
    }
    return false;
}

bool ResolveExpression::visit(PostIncrDecrAST *)
{
    return false;
}
