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

#include "ResolveExpression.h"
#include "LookupContext.h"
#include "Overview.h"
#include "DeprecatedGenTemplateInstance.h"

#include <Control.h>
#include <AST.h>
#include <Scope.h>
#include <Names.h>
#include <Symbols.h>
#include <Literals.h>
#include <CoreTypes.h>
#include <TypeVisitor.h>
#include <NameVisitor.h>

#include <QtCore/QList>
#include <QtCore/QtDebug>

using namespace CPlusPlus;

namespace {

const bool debug = ! qgetenv("CPLUSPLUS_DEBUG").isEmpty();

template <typename _Tp>
static QList<_Tp> removeDuplicates(const QList<_Tp> &results)
{
    QList<_Tp> uniqueList;
    QSet<_Tp> processed;
    foreach (const _Tp &r, results) {
        if (processed.contains(r))
            continue;

        processed.insert(r);
        uniqueList.append(r);
    }

    return uniqueList;
}

} // end of anonymous namespace

/////////////////////////////////////////////////////////////////////
// ResolveExpression
/////////////////////////////////////////////////////////////////////
ResolveExpression::ResolveExpression(const LookupContext &context)
    : ASTVisitor(context.expressionDocument()->translationUnit()),
      _scope(0),
      _context(context),
      sem(context.expressionDocument()->translationUnit())
{ }

ResolveExpression::~ResolveExpression()
{ }

QList<LookupItem> ResolveExpression::operator()(ExpressionAST *ast, Scope *scope)
{ return resolve(ast, scope); }

QList<LookupItem> ResolveExpression::resolve(ExpressionAST *ast, Scope *scope)
{
    Q_ASSERT(scope != 0);

    Scope *previousVisibleSymbol = _scope;
    _scope = scope;
    const QList<LookupItem> result = resolve(ast);
    _scope = previousVisibleSymbol;
    return result;
}

QList<LookupItem> ResolveExpression::resolve(ExpressionAST *ast)
{
    const QList<LookupItem> previousResults = switchResults(QList<LookupItem>());
    accept(ast);
    return removeDuplicates(switchResults(previousResults));
}

QList<LookupItem> ResolveExpression::switchResults(const QList<LookupItem> &results)
{
    const QList<LookupItem> previousResults = _results;
    _results = results;
    return previousResults;
}

void ResolveExpression::addResults(const QList<Symbol *> &symbols)
{
    foreach (Symbol *symbol, symbols) {
        LookupItem item;
        item.setType(symbol->type());
        item.setScope(symbol->scope());
        item.setDeclaration(symbol);
        _results.append(item);
    }
}

void ResolveExpression::addResult(const FullySpecifiedType &ty, Scope *scope)
{
    LookupItem item;
    item.setType(ty);
    item.setScope(scope);

    _results.append(item);
}

bool ResolveExpression::visit(BinaryExpressionAST *ast)
{
    if (tokenKind(ast->binary_op_token) == T_COMMA && ast->right_expression && ast->right_expression->asQtMethod() != 0) {

        if (ast->left_expression && ast->left_expression->asQtMethod() != 0)
            thisObject();
        else
            accept(ast->left_expression);

        QtMethodAST *qtMethod = ast->right_expression->asQtMethod();
        if (DeclaratorAST *d = qtMethod->declarator) {
            if (d->core_declarator) {
                if (DeclaratorIdAST *declaratorId = d->core_declarator->asDeclaratorId()) {
                    if (NameAST *nameAST = declaratorId->name) {
                        if (ClassOrNamespace *binding = baseExpression(_results, T_ARROW)) {
                            _results.clear();
                            addResults(binding->lookup(nameAST->name));
                        }
                    }
                }
            }
        }

        return false;
    }

    accept(ast->left_expression);
    return false;
}

bool ResolveExpression::visit(CastExpressionAST *ast)
{
    Scope *dummyScope = _context.expressionDocument()->globalSymbols();
    FullySpecifiedType ty = sem.check(ast->type_id, dummyScope);
    addResult(ty, _scope);
    return false;
}

bool ResolveExpression::visit(ConditionAST *)
{
    // nothing to do.
    return false;
}

bool ResolveExpression::visit(ConditionalExpressionAST *ast)
{
    if (ast->left_expression)
        accept(ast->left_expression);

    else if (ast->right_expression)
        accept(ast->right_expression);

    return false;
}

bool ResolveExpression::visit(CppCastExpressionAST *ast)
{
    Scope *dummyScope = _context.expressionDocument()->globalSymbols();
    FullySpecifiedType ty = sem.check(ast->type_id, dummyScope);
    addResult(ty, _scope);
    return false;
}

bool ResolveExpression::visit(DeleteExpressionAST *)
{
    FullySpecifiedType ty(control()->voidType());
    addResult(ty, _scope);
    return false;
}

bool ResolveExpression::visit(ArrayInitializerAST *)
{
    // nothing to do.
    return false;
}

bool ResolveExpression::visit(NewExpressionAST *ast)
{
    if (ast->new_type_id) {
        Scope *dummyScope = _context.expressionDocument()->globalSymbols();
        FullySpecifiedType ty = sem.check(ast->new_type_id->type_specifier_list, dummyScope);
        ty = sem.check(ast->new_type_id->ptr_operator_list, ty, dummyScope);
        FullySpecifiedType ptrTy(control()->pointerType(ty));
        addResult(ptrTy, _scope);
    }
    // nothing to do.
    return false;
}

bool ResolveExpression::visit(TypeidExpressionAST *)
{
    const Name *std_type_info[2];
    std_type_info[0] = control()->nameId(control()->findOrInsertIdentifier("std"));
    std_type_info[1] = control()->nameId(control()->findOrInsertIdentifier("type_info"));

    const Name *q = control()->qualifiedNameId(std_type_info, 2, /*global=*/ true);
    FullySpecifiedType ty(control()->namedType(q));
    addResult(ty, _scope);

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

    for (PostfixListAST *it = ast->postfix_expression_list; it; it = it->next)
        accept(it->value);

    return false;
}

bool ResolveExpression::visit(SizeofExpressionAST *)
{
    FullySpecifiedType ty(control()->integerType(IntegerType::Int));
    ty.setUnsigned(true);
    addResult(ty, _scope);
    return false;
}

bool ResolveExpression::visit(NumericLiteralAST *ast)
{
    Type *type = 0;
    const NumericLiteral *literal = numericLiteral(ast->literal_token);

    if (literal->isChar())
        type = control()->integerType(IntegerType::Char);
    else if (literal->isWideChar())
        type = control()->integerType(IntegerType::WideChar);
    else if (literal->isInt())
        type = control()->integerType(IntegerType::Int);
    else if (literal->isLong())
        type = control()->integerType(IntegerType::Long);
    else if (literal->isLongLong())
        type = control()->integerType(IntegerType::LongLong);
    else if (literal->isFloat())
        type = control()->floatType(FloatType::Float);
    else if (literal->isDouble())
        type = control()->floatType(FloatType::Double);
    else if (literal->isLongDouble())
        type = control()->floatType(FloatType::LongDouble);
    else
        type = control()->integerType(IntegerType::Int);

    FullySpecifiedType ty(type);
    if (literal->isUnsigned())
        ty.setUnsigned(true);

    addResult(ty, _scope);
    return false;
}

bool ResolveExpression::visit(BoolLiteralAST *)
{
    FullySpecifiedType ty(control()->integerType(IntegerType::Bool));
    addResult(ty, _scope);
    return false;
}

bool ResolveExpression::visit(ThisExpressionAST *)
{
    thisObject();
    return false;
}

void ResolveExpression::thisObject()
{
    Scope *scope = _scope;
    for (; scope; scope = scope->enclosingScope()) {
        if (scope->isFunctionScope()) {
            Function *fun = scope->owner()->asFunction();
            if (Scope *cscope = scope->enclosingClassScope()) {
                Class *klass = cscope->owner()->asClass();
                FullySpecifiedType classTy(control()->namedType(klass->name()));
                FullySpecifiedType ptrTy(control()->pointerType(classTy));
                addResult(ptrTy, fun->scope());
                break;
            } else if (const QualifiedNameId *q = fun->name()->asQualifiedNameId()) {
                const Name *nestedNameSpecifier = 0;
                if (q->nameCount() == 1 && q->isGlobal())
                    nestedNameSpecifier = q->nameAt(0);
                else
                    nestedNameSpecifier = control()->qualifiedNameId(q->names(), q->nameCount() - 1);
                FullySpecifiedType classTy(control()->namedType(nestedNameSpecifier));
                FullySpecifiedType ptrTy(control()->pointerType(classTy));
                addResult(ptrTy, fun->scope());
                break;
            }
        }
    }
}

bool ResolveExpression::visit(CompoundExpressionAST *ast)
{
    CompoundStatementAST *cStmt = ast->statement;
    if (cStmt && cStmt->statement_list) {
        accept(cStmt->statement_list->lastValue());
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
    addResult(ty, _scope);
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
        QMutableListIterator<LookupItem > it(_results);
        while (it.hasNext()) {
            LookupItem p = it.next();
            FullySpecifiedType ty = p.type();
            ty.setType(control()->pointerType(ty));
            p.setType(ty);
            it.setValue(p);
        }
    } else if (unaryOp == T_STAR) {
        QMutableListIterator<LookupItem > it(_results);
        while (it.hasNext()) {
            LookupItem p = it.next();
            if (PointerType *ptrTy = p.type()->asPointerType()) {
                p.setType(ptrTy->elementType());
                it.setValue(p);
            } else {
                it.remove();
            }
        }
    }
    return false;
}

bool ResolveExpression::visit(CompoundLiteralAST *ast)
{
    accept(ast->type_id);
    return false;
}

bool ResolveExpression::visit(QualifiedNameAST *ast)
{
    if (const Name *name = ast->name) {
        const QList<Symbol *> candidates = _context.lookup(name, _scope);
        addResults(candidates);
    }

    return false;
}

bool ResolveExpression::visit(SimpleNameAST *ast)
{
    const QList<Symbol *> candidates = _context.lookup(ast->name, _scope);
    addResults(candidates);
    return false;
}

bool ResolveExpression::visit(TemplateIdAST *ast)
{
    const QList<Symbol *> candidates = _context.lookup(ast->name, _scope);
    addResults(candidates);
    return false;
}

bool ResolveExpression::visit(DestructorNameAST *)
{
    FullySpecifiedType ty(control()->voidType());
    addResult(ty, _scope);
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

bool ResolveExpression::maybeValidPrototype(Function *funTy, unsigned actualArgumentCount) const
{
    unsigned minNumberArguments = 0;

    for (; minNumberArguments < funTy->argumentCount(); ++minNumberArguments) {
        Argument *arg = funTy->argumentAt(minNumberArguments)->asArgument();

        if (arg->hasInitializer())
            break;
    }

    if (actualArgumentCount < minNumberArguments) {
        // not enough arguments.
        return false;

    } else if (! funTy->isVariadic() && actualArgumentCount > funTy->argumentCount()) {
        // too many arguments.
        return false;
    }

    return true;
}

bool ResolveExpression::visit(CallAST *ast)
{
    const QList<LookupItem> baseResults = _results;
    _results.clear();

    // Compute the types of the actual arguments.
    int actualArgumentCount = 0;

    //QList< QList<Result> > arguments;
    for (ExpressionListAST *exprIt = ast->expression_list; exprIt; exprIt = exprIt->next) {
        //arguments.append(resolve(exprIt->expression));
        ++actualArgumentCount;
    }

    const Name *functionCallOp = control()->operatorNameId(OperatorNameId::FunctionCallOp);

    foreach (const LookupItem &result, baseResults) {
        FullySpecifiedType ty = result.type().simplified();
        Scope *scope = result.scope();

        if (NamedType *namedTy = ty->asNamedType()) {
            if (ClassOrNamespace *b = _context.lookupType(namedTy->name(), scope)) {
                foreach (Symbol *overload, b->find(functionCallOp)) {
                    if (Function *funTy = overload->type()->asFunctionType()) {
                        if (maybeValidPrototype(funTy, actualArgumentCount)) {
                            if (Function *proto = instantiate(namedTy->name(), funTy)->asFunctionType())
                                addResult(proto->returnType().simplified(), scope);
                        }
                    }
                }
            }

        } else if (Function *funTy = ty->asFunctionType()) {
            if (maybeValidPrototype(funTy, actualArgumentCount))
                addResult(funTy->returnType().simplified(), scope);

        } else if (Class *classTy = ty->asClassType()) {
            // Constructor call
            FullySpecifiedType ctorTy = control()->namedType(classTy->name());
            addResult(ctorTy, scope);
        }
    }

    return false;
}

bool ResolveExpression::visit(ArrayAccessAST *ast)
{
    const QList<LookupItem> baseResults = _results;
    _results.clear();

    const QList<LookupItem> indexResults = resolve(ast->expression);

    const Name *arrayAccessOp = control()->operatorNameId(OperatorNameId::ArrayAccessOp);

    foreach (const LookupItem &result, baseResults) {
        FullySpecifiedType ty = result.type().simplified();
        Scope *scope = result.scope();

        if (PointerType *ptrTy = ty->asPointerType()) {
            addResult(ptrTy->elementType().simplified(), scope);

        } else if (ArrayType *arrTy = ty->asArrayType()) {
            addResult(arrTy->elementType().simplified(), scope);

        } else if (NamedType *namedTy = ty->asNamedType()) {
            if (ClassOrNamespace *b = _context.lookupType(namedTy->name(), scope)) {
                foreach (Symbol *overload, b->find(arrayAccessOp)) {
                    if (Function *funTy = overload->type()->asFunctionType()) {
                        if (Function *proto = instantiate(namedTy->name(), funTy)->asFunctionType())
                            // ### TODO: check the actual arguments
                            addResult(proto->returnType().simplified(), scope);
                    }
                }

            }
        }
    }
    return false;
}

bool ResolveExpression::visit(MemberAccessAST *ast)
{
    // The candidate types for the base expression are stored in
    // _results.
    const QList<LookupItem> baseResults = _results;
    _results.clear();

    // Evaluate the expression-id that follows the access operator.
    const Name *memberName = 0;
    if (ast->member_name)
        memberName = ast->member_name->name;

    // Remember the access operator.
    const int accessOp = tokenKind(ast->access_token);

    if (ClassOrNamespace *binding = baseExpression(baseResults, accessOp))
        addResults(binding->lookup(memberName));

    return false;
}

ClassOrNamespace *ResolveExpression::findClass(const FullySpecifiedType &originalTy, Scope *scope) const
{
    FullySpecifiedType ty = originalTy.simplified();
    ClassOrNamespace *binding = 0;

    if (Class *klass = ty->asClassType())
        binding = _context.lookupType(klass);

    else if (NamedType *namedTy = ty->asNamedType())
        binding = _context.lookupType(namedTy->name(), scope);

    else if (Function *funTy = ty->asFunctionType())
        return findClass(funTy->returnType(), scope);

    return binding;
}

ClassOrNamespace *ResolveExpression::baseExpression(const QList<LookupItem> &baseResults,
                                                    int accessOp,
                                                    bool *replacedDotOperator) const
{
    foreach (const LookupItem &r, baseResults) {
        FullySpecifiedType ty = r.type().simplified();
        Scope *scope = r.scope();

        if (accessOp == T_ARROW) {
            if (PointerType *ptrTy = ty->asPointerType()) {
                if (ClassOrNamespace *binding = findClass(ptrTy->elementType(), scope))
                    return binding;

            } else if (ClassOrNamespace *binding = findClass(ty, scope)) {
                // lookup for overloads of operator->
                const OperatorNameId *arrowOp = control()->operatorNameId(OperatorNameId::ArrowOp);

                foreach (Symbol *overload, binding->find(arrowOp)) {
                    if (overload->type()->isFunctionType()) {
                        FullySpecifiedType overloadTy = instantiate(binding->templateId(), overload);
                        Function *instantiatedFunction = overloadTy->asFunctionType();
                        Q_ASSERT(instantiatedFunction != 0);

                        FullySpecifiedType retTy = instantiatedFunction->returnType().simplified();

                        if (PointerType *ptrTy = retTy->asPointerType()) {
                            if (ClassOrNamespace *retBinding = findClass(ptrTy->elementType(), overload->scope()))
                                return retBinding;

                            else if (debug) {
                                Overview oo;
                                qDebug() << "no class for:" << oo(ptrTy->elementType());
                            }
                        }
                    }
                }
            }
        } else if (accessOp == T_DOT) {
            if (replacedDotOperator) {
                if (PointerType *ptrTy = ty->asPointerType()) {
                    // replace . with ->
                    ty = ptrTy->elementType();
                    *replacedDotOperator = true;
                }
            }

            if (ClassOrNamespace *binding = findClass(ty, scope))
                return binding;
        }
    }

    return 0;
}

FullySpecifiedType ResolveExpression::instantiate(const Name *className, Symbol *candidate) const
{
    return DeprecatedGenTemplateInstance::instantiate(className, candidate, _context.control());
}

bool ResolveExpression::visit(PostIncrDecrAST *)
{
    return false;
}

bool ResolveExpression::visit(ObjCMessageExpressionAST *ast)
{
    const QList<LookupItem> receiverResults = resolve(ast->receiver_expression);

    foreach (const LookupItem &result, receiverResults) {
        FullySpecifiedType ty = result.type().simplified();
        ClassOrNamespace *binding = 0;

        if (ObjCClass *clazz = ty->asObjCClassType()) {
            // static access, e.g.:
            //   [NSObject description];
            binding = _context.lookupType(clazz);
        } else if (PointerType *ptrTy = ty->asPointerType()) {
            if (NamedType *namedTy = ptrTy->elementType()->asNamedType()) {
                // dynamic access, e.g.:
                //   NSObject *obj = ...; [obj release];
                binding = _context.lookupType(namedTy->name(), result.scope());
            }
        }

        if (binding) {
            foreach (Symbol *s, binding->lookup(ast->selector->name))
                if (ObjCMethod *m = s->asObjCMethod())
                    addResult(m->returnType(), result.scope());
        }
    }

    return false;
}

