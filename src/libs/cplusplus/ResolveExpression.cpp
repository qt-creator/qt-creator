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

#include "ResolveExpression.h"
#include "LookupContext.h"
#include "Overview.h"

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

typedef QList< QPair<Name *, FullySpecifiedType> > Substitution;

class Instantiation: protected TypeVisitor, protected NameVisitor
{
    Control *_control;
    FullySpecifiedType _type;
    const Substitution _substitution;

public:
    Instantiation(Control *control, const Substitution &substitution)
        : _control(control),
          _substitution(substitution)
    { }

    FullySpecifiedType operator()(const FullySpecifiedType &ty)
    { return subst(ty); }

protected:
    FullySpecifiedType subst(Name *name)
    {
        for (int i = 0; i < _substitution.size(); ++i) {
            const QPair<Name *, FullySpecifiedType> s = _substitution.at(i);
            if (name->isEqualTo(s.first))
                return s.second;
        }

        return _control->namedType(name);
    }

    FullySpecifiedType subst(const FullySpecifiedType &ty)
    {
        FullySpecifiedType previousType = switchType(ty);
        TypeVisitor::accept(ty.type());
        return switchType(previousType);
    }

    FullySpecifiedType switchType(const FullySpecifiedType &type)
    {
        FullySpecifiedType previousType = _type;
        _type = type;
        return previousType;
    }

    // types
    virtual void visit(PointerToMemberType * /*ty*/)
    {
        Q_ASSERT(false);
    }

    virtual void visit(PointerType *ty)
    {
        FullySpecifiedType elementType = subst(ty->elementType());
        _type.setType(_control->pointerType(elementType));
    }

    virtual void visit(ReferenceType *ty)
    {
        FullySpecifiedType elementType = subst(ty->elementType());
        _type.setType(_control->referenceType(elementType));
    }

    virtual void visit(ArrayType *ty)
    {
        FullySpecifiedType elementType = subst(ty->elementType());
        _type.setType(_control->arrayType(elementType, ty->size()));
    }

    virtual void visit(NamedType *ty)
    { _type.setType(subst(ty->name()).type()); } // ### merge the specifiers

    virtual void visit(Function *ty)
    {
        Name *name = ty->name();
        FullySpecifiedType returnType = subst(ty->returnType());

        Function *fun = _control->newFunction(0, name);
        fun->setScope(ty->scope());
        fun->setReturnType(returnType);
        for (unsigned i = 0; i < ty->argumentCount(); ++i) {
            Symbol *arg = ty->argumentAt(i);
            FullySpecifiedType argTy = subst(arg->type());
            Argument *newArg = _control->newArgument(0, arg->name());
            newArg->setType(argTy);
            fun->arguments()->enterSymbol(newArg);
        }
        _type.setType(fun);
    }

    virtual void visit(VoidType *)
    { /* nothing to do*/ }

    virtual void visit(IntegerType *)
    { /* nothing to do*/ }

    virtual void visit(FloatType *)
    { /* nothing to do*/ }

    virtual void visit(Namespace *)
    { Q_ASSERT(false); }

    virtual void visit(Class *)
    { Q_ASSERT(false); }

    virtual void visit(Enum *)
    { Q_ASSERT(false); }

    // names
    virtual void visit(NameId *)
    { Q_ASSERT(false); }

    virtual void visit(TemplateNameId *)
    { Q_ASSERT(false); }

    virtual void visit(DestructorNameId *)
    { Q_ASSERT(false); }

    virtual void visit(OperatorNameId *)
    { Q_ASSERT(false); }

    virtual void visit(ConversionNameId *)
    { Q_ASSERT(false); }

    virtual void visit(QualifiedNameId *)
    { Q_ASSERT(false); }
};

} // end of anonymous namespace

/////////////////////////////////////////////////////////////////////
// ResolveExpression
/////////////////////////////////////////////////////////////////////
ResolveExpression::ResolveExpression(const LookupContext &context)
    : ASTVisitor(context.expressionDocument()->control()),
      _context(context),
      sem(_context.control())
{ }

ResolveExpression::~ResolveExpression()
{ }

QList<ResolveExpression::Result> ResolveExpression::operator()(ExpressionAST *ast)
{
    const QList<Result> previousResults = switchResults(QList<Result>());
    accept(ast);
    return switchResults(previousResults);
}

QList<ResolveExpression::Result>
ResolveExpression::switchResults(const QList<ResolveExpression::Result> &results)
{
    const QList<Result> previousResults = _results;
    _results = results;
    return previousResults;
}

void ResolveExpression::addResults(const QList<Result> &results)
{
    foreach (const Result r, results)
        addResult(r);
}

void ResolveExpression::addResult(const FullySpecifiedType &ty, Symbol *symbol)
{ return addResult(Result(ty, symbol)); }

void ResolveExpression::addResult(const Result &r)
{
    Result p = r;
    if (! p.second)
        p.second = _context.symbol();

    if (! _results.contains(p))
        _results.append(p);
}

QList<Scope *> ResolveExpression::visibleScopes(const Result &result) const
{ return _context.visibleScopes(result); }

bool ResolveExpression::visit(ExpressionListAST *)
{
    // nothing to do.
    return false;
}

bool ResolveExpression::visit(BinaryExpressionAST *ast)
{
    accept(ast->left_expression);
    return false;
}

bool ResolveExpression::visit(CastExpressionAST *ast)
{
    Scope dummy;
    addResult(sem.check(ast->type_id, &dummy));
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

bool ResolveExpression::visit(CppCastExpressionAST *ast)
{
    Scope dummy;
    addResult(sem.check(ast->type_id, &dummy));
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
    Name *std_type_info[2];
    std_type_info[0] = control()->nameId(control()->findOrInsertIdentifier("std"));
    std_type_info[1] = control()->nameId(control()->findOrInsertIdentifier("type_info"));

    Name *q = control()->qualifiedNameId(std_type_info, 2, /*global=*/ true);
    FullySpecifiedType ty(control()->namedType(q));
    addResult(ty);

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
    addResult(ty);
    return false;
}

bool ResolveExpression::visit(NumericLiteralAST *)
{
    FullySpecifiedType ty(control()->integerType(IntegerType::Int));
    addResult(ty);
    return false;
}

bool ResolveExpression::visit(BoolLiteralAST *)
{
    FullySpecifiedType ty(control()->integerType(IntegerType::Bool));
    addResult(ty);
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
                addResult(ptrTy, fun);
                break;
            } else if (QualifiedNameId *q = fun->name()->asQualifiedNameId()) {
                Name *nestedNameSpecifier = 0;
                if (q->nameCount() == 1 && q->isGlobal())
                    nestedNameSpecifier = q->nameAt(0);
                else
                    nestedNameSpecifier = control()->qualifiedNameId(q->names(), q->nameCount() - 1);
                FullySpecifiedType classTy(control()->namedType(nestedNameSpecifier));
                FullySpecifiedType ptrTy(control()->pointerType(classTy));
                addResult(ptrTy, fun);
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
    addResult(ty);
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
        QMutableListIterator<Result > it(_results);
        while (it.hasNext()) {
            Result p = it.next();
            p.first.setType(control()->pointerType(p.first));
            it.setValue(p);
        }
    } else if (unaryOp == T_STAR) {
        QMutableListIterator<Result > it(_results);
        while (it.hasNext()) {
            Result p = it.next();
            if (PointerType *ptrTy = p.first->asPointerType()) {
                p.first = ptrTy->elementType();
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
    ResolveClass resolveClass;
    Scope dummy;
    Name *name = sem.check(ast, &dummy);

    QList<Symbol *> symbols = _context.resolve(name);
    foreach (Symbol *symbol, symbols) {
        if (symbol->isTypedef()) {
            if (NamedType *namedTy = symbol->type()->asNamedType()) {
                LookupContext symbolContext(symbol, _context);
                const Result r(namedTy, symbol);
                const QList<Symbol *> resolvedClasses =
                        resolveClass(r, _context);
                if (resolvedClasses.count()) {
                    foreach (Symbol *s, resolvedClasses) {
                        addResult(s->type(), s);
                    }
                    continue;
                }
            }
        }
        addResult(symbol->type(), symbol);
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
        addResult(symbol->type(), symbol);

    return false;
}

bool ResolveExpression::visit(DestructorNameAST *)
{
    FullySpecifiedType ty(control()->voidType());
    addResult(ty);
    return false;
}

bool ResolveExpression::visit(TemplateIdAST *ast)
{
    Scope dummy;
    Name *name = sem.check(ast, &dummy);

    QList<Symbol *> symbols = _context.resolve(name);
    foreach (Symbol *symbol, symbols)
        addResult(symbol->type(), symbol);

    return false;
}

bool ResolveExpression::visit(CallAST *ast)
{
    // Compute the types of the actual arguments.
    QList< QList<Result> > arguments;
    for (ExpressionListAST *exprIt = ast->expression_list; exprIt;
            exprIt = exprIt->next) {
        arguments.append(operator()(exprIt->expression));
    }

    QList<Result> baseResults = _results;
    _results.clear();

    foreach (Result p, baseResults) {
        if (Function *funTy = p.first->asFunction()) {
            unsigned minNumberArguments = 0;
            for (; minNumberArguments < funTy->argumentCount(); ++minNumberArguments) {
                Argument *arg = funTy->argumentAt(minNumberArguments)->asArgument();
                if (arg->hasInitializer())
                    break;
            }
            const unsigned actualArgumentCount = arguments.count();
            if (actualArgumentCount < minNumberArguments) {
                // not enough arguments.
            } else if (! funTy->isVariadic() && actualArgumentCount > funTy->argumentCount()) {
                // too many arguments.
            } else {
                p.first = funTy->returnType();
                addResult(p);
            }
        } else if (Class *classTy = p.first->asClass()) {
            // Constructor call
            p.first = control()->namedType(classTy->name());
            addResult(p);
        }
    }

    return false;
}

bool ResolveExpression::visit(ArrayAccessAST *ast)
{
    const QList<Result> baseResults = _results;
    _results.clear();

    const QList<Result> indexResults = operator()(ast->expression);
    ResolveClass symbolsForDotAcccess;

    foreach (Result p, baseResults) {
        FullySpecifiedType ty = p.first;
        Symbol *contextSymbol = p.second;

        if (ReferenceType *refTy = ty->asReferenceType())
            ty = refTy->elementType();

        if (PointerType *ptrTy = ty->asPointerType()) {
            addResult(ptrTy->elementType(), contextSymbol);
        } else if (ArrayType *arrTy = ty->asArrayType()) {
            addResult(arrTy->elementType(), contextSymbol);
        } else if (NamedType *namedTy = ty->asNamedType()) {
            const QList<Symbol *> classObjectCandidates =
                    symbolsForDotAcccess(p, _context);

            foreach (Symbol *classObject, classObjectCandidates) {
                const QList<Result> overloads =
                        resolveArrayOperator(p, namedTy, classObject->asClass());
                foreach (Result r, overloads) {
                    FullySpecifiedType ty = r.first;
                    Function *funTy = ty->asFunction();
                    if (! funTy)
                        continue;

                    ty = funTy->returnType();
                    addResult(ty, funTy);
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
    QList<Result> baseResults = _results;

    // Evaluate the expression-id that follows the access operator.
    Scope dummy;
    Name *memberName = sem.check(ast->member_name, &dummy);

    // Remember the access operator.
    const unsigned accessOp = tokenKind(ast->access_token);

    _results = resolveMemberExpression(baseResults, accessOp, memberName);

    return false;
}

QList<ResolveExpression::Result>
ResolveExpression::resolveMemberExpression(const QList<Result> &baseResults,
                                           unsigned accessOp,
                                           Name *memberName) const
{
    ResolveClass resolveClass;
    QList<Result> results;

    if (accessOp == T_ARROW) {
        foreach (Result p, baseResults) {
            FullySpecifiedType ty = p.first;

            if (ReferenceType *refTy = ty->asReferenceType())
                ty = refTy->elementType();

            if (NamedType *namedTy = ty->asNamedType()) {
                const QList<Symbol *> classObjectCandidates =
                        resolveClass(namedTy, p, _context);

                foreach (Symbol *classObject, classObjectCandidates) {
                    const QList<Result> overloads = resolveArrowOperator(p, namedTy,
                                                                         classObject->asClass());
                    foreach (Result r, overloads) {
                        FullySpecifiedType ty = r.first;
                        Function *funTy = ty->asFunction();
                        if (! funTy)
                            continue;

                        ty = funTy->returnType();

                        if (ReferenceType *refTy = ty->asReferenceType())
                            ty = refTy->elementType();

                        if (PointerType *ptrTy = ty->asPointerType()) {
                            if (NamedType *namedTy = ptrTy->elementType()->asNamedType())
                                results += resolveMember(r, memberName, namedTy);
                        }
                    }
                }
            } else if (PointerType *ptrTy = ty->asPointerType()) {
                if (NamedType *namedTy = ptrTy->elementType()->asNamedType())
                    results += resolveMember(p, memberName, namedTy);
            }
        }
    } else if (accessOp == T_DOT) {
        // The base expression shall be a "class object" of a complete type.
        foreach (Result p, baseResults) {
            FullySpecifiedType ty = p.first;

            if (ReferenceType *refTy = ty->asReferenceType())
                ty = refTy->elementType();

            if (NamedType *namedTy = ty->asNamedType())
                results += resolveMember(p, memberName, namedTy);
            else if (Function *fun = ty->asFunction()) {
                if (fun->scope()->isBlockScope() || fun->scope()->isNamespaceScope()) {
                    ty = fun->returnType();

                    if (ReferenceType *refTy = ty->asReferenceType())
                        ty = refTy->elementType();

                    if (NamedType *namedTy = ty->asNamedType())
                        results += resolveMember(p, memberName, namedTy);
                }
            }

        }
    }

    return results;
}

QList<ResolveExpression::Result>
ResolveExpression::resolveMember(const Result &p,
                                 Name *memberName,
                                 NamedType *namedTy) const
{
    ResolveClass resolveClass;

    const QList<Symbol *> classObjectCandidates =
            resolveClass(namedTy, p, _context);

    QList<Result> results;
    foreach (Symbol *classObject, classObjectCandidates) {
        results += resolveMember(p, memberName, namedTy,
                                 classObject->asClass());
    }
    return results;
}

QList<ResolveExpression::Result>
ResolveExpression::resolveMember(const Result &,
                                 Name *memberName,
                                 NamedType *namedTy,
                                 Class *klass) const
{
    QList<Scope *> scopes;
    _context.expand(klass->members(), _context.visibleScopes(), &scopes);
    QList<Result> results;

    QList<Symbol *> candidates = _context.resolve(memberName, scopes);
    foreach (Symbol *candidate, candidates) {
        FullySpecifiedType ty = candidate->type();
        Name *unqualifiedNameId = namedTy->name();
        if (QualifiedNameId *q = namedTy->name()->asQualifiedNameId())
            unqualifiedNameId = q->unqualifiedNameId();
        if (TemplateNameId *templId = unqualifiedNameId->asTemplateNameId()) {
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

        const Result result(ty, candidate);
        if (! results.contains(result))
            results.append(result);
    }

    return results;
}

QList<ResolveExpression::Result>
ResolveExpression::resolveArrowOperator(const Result &,
                                        NamedType *namedTy,
                                        Class *klass) const
{
    QList<Scope *> scopes;
    _context.expand(klass->members(), _context.visibleScopes(), &scopes);
    QList<Result> results;

    Name *memberName = control()->operatorNameId(OperatorNameId::ArrowOp);
    QList<Symbol *> candidates = _context.resolve(memberName, scopes);
    foreach (Symbol *candidate, candidates) {
        FullySpecifiedType ty = candidate->type();
        Name *unqualifiedNameId = namedTy->name();
        if (QualifiedNameId *q = namedTy->name()->asQualifiedNameId())
            unqualifiedNameId = q->unqualifiedNameId();
        if (TemplateNameId *templId = unqualifiedNameId->asTemplateNameId()) {
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

        const Result result(ty, candidate);
        if (! results.contains(result))
            results.append(result);
    }

    return results;
}

QList<ResolveExpression::Result>
ResolveExpression::resolveArrayOperator(const Result &,
                                        NamedType *namedTy,
                                        Class *klass) const
{
    // ### todo handle index expressions.

    QList<Scope *> scopes;
    _context.expand(klass->members(), _context.visibleScopes(), &scopes);
    QList<Result> results;

    Name *memberName = control()->operatorNameId(OperatorNameId::ArrayAccessOp);
    QList<Symbol *> candidates = _context.resolve(memberName, scopes);
    foreach (Symbol *candidate, candidates) {
        FullySpecifiedType ty = candidate->type();
        Name *unqualifiedNameId = namedTy->name();
        if (QualifiedNameId *q = namedTy->name()->asQualifiedNameId())
            unqualifiedNameId = q->unqualifiedNameId();
        if (TemplateNameId *templId = unqualifiedNameId->asTemplateNameId()) {
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

        const Result result(ty, candidate);
        if (! results.contains(result))
            results.append(result);
    }

    return results;
}

bool ResolveExpression::visit(PostIncrDecrAST *)
{
    return false;
}

////////////////////////////////////////////////////////////////////////////////
ResolveClass::ResolveClass()
{ }

QList<Symbol *> ResolveClass::operator()(NamedType *namedTy,
                                         ResolveExpression::Result p,
                                         const LookupContext &context)
{
    const QList<ResolveExpression::Result> previousBlackList = _blackList;
    const QList<Symbol *> symbols = resolveClass(namedTy, p, context);
    _blackList = previousBlackList;
    return symbols;
}

QList<Symbol *> ResolveClass::operator()(ResolveExpression::Result p,
                                         const LookupContext &context)
{
    const QList<ResolveExpression::Result> previousBlackList = _blackList;
    const QList<Symbol *> symbols = resolveClass(p, context);
    _blackList = previousBlackList;
    return symbols;
}

QList<Symbol *> ResolveClass::resolveClass(NamedType *namedTy,
                                           ResolveExpression::Result p,
                                           const LookupContext &context)
{
    QList<Symbol *> resolvedSymbols;

    if (_blackList.contains(p))
        return resolvedSymbols;

    _blackList.append(p);

    const QList<Symbol *> candidates =
            context.resolve(namedTy->name(), context.visibleScopes(p));

    foreach (Symbol *candidate, candidates) {
        if (Class *klass = candidate->asClass()) {
            if (resolvedSymbols.contains(klass))
                continue; // we already know about `klass'
            resolvedSymbols.append(klass);
        } else if (candidate->isTypedef()) {
            if (Declaration *decl = candidate->asDeclaration()) {
                if (Class *asClass = decl->type()->asClass()) {
                    // typedef struct { } Point;
                    // Point pt;
                    // pt.
                    resolvedSymbols.append(asClass);
                } else {
                    // typedef Point Boh;
                    // Boh b;
                    // b.
                    const ResolveExpression::Result r(decl->type(), decl);
                    resolvedSymbols += resolveClass(r, context);
                }
            }
        } else if (Declaration *decl = candidate->asDeclaration()) {
            if (Function *funTy = decl->type()->asFunction()) {
                // QString foo("ciao");
                // foo.
                if (funTy->scope()->isBlockScope() || funTy->scope()->isNamespaceScope()) {
                    const ResolveExpression::Result r(funTy->returnType(), decl);
                    resolvedSymbols += resolveClass(r, context);
                }
            }
        }
    }

    return resolvedSymbols;
}

QList<Symbol *> ResolveClass::resolveClass(ResolveExpression::Result p,
                                           const LookupContext &context)
{
    FullySpecifiedType ty = p.first;

    if (NamedType *namedTy = ty->asNamedType()) {
        return resolveClass(namedTy, p, context);
    } else if (ReferenceType *refTy = ty->asReferenceType()) {
        const ResolveExpression::Result e(refTy->elementType(), p.second);
        return resolveClass(e, context);
    }

    return QList<Symbol *>();
}

