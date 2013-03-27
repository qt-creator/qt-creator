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

#include "ResolveExpression.h"

#include "LookupContext.h"
#include "Overview.h"
#include "DeprecatedGenTemplateInstance.h"
#include "CppRewriter.h"
#include "TypeOfExpression.h"

#include <cplusplus/Control.h>
#include <cplusplus/AST.h>
#include <cplusplus/Scope.h>
#include <cplusplus/Names.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/Literals.h>
#include <cplusplus/CoreTypes.h>
#include <cplusplus/TypeVisitor.h>
#include <cplusplus/NameVisitor.h>
#include <cplusplus/Templates.h>

#include <QList>
#include <QDebug>
#include <QSet>

#include <map>

using namespace CPlusPlus;

namespace {

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
      bind(context.expressionDocument()->translationUnit()),
      _reference(false)
{ }

ResolveExpression::~ResolveExpression()
{ }

QList<LookupItem> ResolveExpression::operator()(ExpressionAST *ast, Scope *scope)
{ return resolve(ast, scope); }

QList<LookupItem> ResolveExpression::reference(ExpressionAST *ast, Scope *scope)
{ return resolve(ast, scope, true); }

QList<LookupItem> ResolveExpression::resolve(ExpressionAST *ast, Scope *scope, bool ref)
{
    if (! scope)
        return QList<LookupItem>();

    std::swap(_scope, scope);
    std::swap(_reference, ref);

    const QList<LookupItem> result = expression(ast);

    std::swap(_reference, ref);
    std::swap(_scope, scope);

    return result;
}

QList<LookupItem> ResolveExpression::expression(ExpressionAST *ast)
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
        item.setScope(symbol->enclosingScope());
        item.setDeclaration(symbol);
        _results.append(item);
    }
}

void ResolveExpression::addResults(const QList<LookupItem> &items)
{
    _results += items;
}

void ResolveExpression::addResult(const FullySpecifiedType &ty, Scope *scope)
{
    LookupItem item;
    item.setType(ty);
    item.setScope(scope);

    _results.append(item);
}

bool ResolveExpression::visit(IdExpressionAST *ast)
{
    accept(ast->name);
    return false;
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
    Scope *dummyScope = _context.expressionDocument()->globalNamespace();
    FullySpecifiedType ty = bind(ast->type_id, dummyScope);
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
    Scope *dummyScope = _context.expressionDocument()->globalNamespace();
    FullySpecifiedType ty = bind(ast->type_id, dummyScope);
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
        Scope *dummyScope = _context.expressionDocument()->globalNamespace();
        FullySpecifiedType ty = bind(ast->new_type_id, dummyScope);
        FullySpecifiedType ptrTy(control()->pointerType(ty));
        addResult(ptrTy, _scope);
    }
    // nothing to do.
    return false;
}

bool ResolveExpression::visit(TypeidExpressionAST *)
{
    const Name *stdName = control()->identifier("std");
    const Name *tiName = control()->identifier("type_info");
    const Name *q = control()->qualifiedNameId(control()->qualifiedNameId(/* :: */ 0, stdName), tiName);

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

bool ResolveExpression::visit(SizeofExpressionAST *)
{
    FullySpecifiedType ty(control()->integerType(IntegerType::Int));
    ty.setUnsigned(true);
    addResult(ty, _scope);
    return false;
}

bool ResolveExpression::visit(PointerLiteralAST *)
{
    FullySpecifiedType ty(control()->integerType(IntegerType::Int)); // Handling as Int.
    addResult(ty, _scope);
    return false;
}

bool ResolveExpression::visit(NumericLiteralAST *ast)
{
    const Token &tk = tokenAt(ast->literal_token);

    Type *type = 0;
    bool isUnsigned = false;

    if (tk.is(T_CHAR_LITERAL)) {
        type = control()->integerType(IntegerType::Char);
    } else if (tk.is(T_WIDE_CHAR_LITERAL)) {
        type = control()->integerType(IntegerType::WideChar);
    } else if (tk.is(T_UTF16_CHAR_LITERAL)) {
        type = control()->integerType(IntegerType::Char16);
    } else if (tk.is(T_UTF32_CHAR_LITERAL)) {
        type = control()->integerType(IntegerType::Char32);
    } else if (const NumericLiteral *literal = numericLiteral(ast->literal_token)) {
        isUnsigned = literal->isUnsigned();
        if (literal->isInt())
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
    }

    FullySpecifiedType ty(type);
    ty.setUnsigned(isUnsigned);
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
        if (Function *fun = scope->asFunction()) {
            if (Class *klass = scope->enclosingClass()) {
                FullySpecifiedType classTy(control()->namedType(klass->name()));
                FullySpecifiedType ptrTy(control()->pointerType(classTy));
                addResult(ptrTy, fun->enclosingScope());
                break;
            } else if (const QualifiedNameId *q = fun->name()->asQualifiedNameId()) {
                if (q->base()) {
                    FullySpecifiedType classTy(control()->namedType(q->base()));
                    FullySpecifiedType ptrTy(control()->pointerType(classTy));
                    addResult(ptrTy, fun->enclosingScope());
                }
                break;
            }
        }
    }
}

bool ResolveExpression::visit(CompoundExpressionAST *ast)
{
    CompoundStatementAST *cStmt = ast->statement;
    if (cStmt && cStmt->statement_list)
        accept(cStmt->statement_list->lastValue());
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
            FullySpecifiedType ty = p.type();
            NamedType *namedTy = ty->asNamedType();
            if (namedTy != 0) {
                const QList<LookupItem> types = _context.lookup(namedTy->name(), p.scope());
                if (!types.empty())
                    ty = types.front().type();
            }
            bool added = false;
            if (PointerType *ptrTy = ty->asPointerType()) {
                p.setType(ptrTy->elementType());
                it.setValue(p);
                added = true;
            } else if (namedTy != 0) {
                const Name *starOp = control()->operatorNameId(OperatorNameId::StarOp);
                if (ClassOrNamespace *b = _context.lookupType(namedTy->name(), p.scope())) {
                    foreach (const LookupItem &r, b->find(starOp)) {
                        Symbol *overload = r.declaration();
                        if (Function *funTy = overload->type()->asFunctionType()) {
                            if (maybeValidPrototype(funTy, 0)) {
                                if (Function *proto = instantiate(b->templateId(), funTy)->asFunctionType()) {
                                    FullySpecifiedType retTy = proto->returnType().simplified();
                                    p.setType(retTy);
                                    p.setScope(proto->enclosingScope());
                                    it.setValue(p);
                                    added = true;
                                    break;
                                }
                            }
                        }
                    }
                }
                if (!added)
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
        const QList<LookupItem> candidates = _context.lookup(name, _scope);
        addResults(candidates);
    }

    return false;
}

namespace {

class DeduceAutoCheck : public ASTVisitor
{
public:
    DeduceAutoCheck(const Identifier *id, TranslationUnit *tu)
        : ASTVisitor(tu), _id(id), _block(false)
    {
        accept(tu->ast());
    }

    virtual bool preVisit(AST *)
    {
        if (_block)
            return false;

        return true;
    }

    virtual bool visit(SimpleNameAST *ast)
    {
        if (ast->name
                && ast->name->identifier()
                && strcmp(ast->name->identifier()->chars(), _id->chars()) == 0) {
            _block = true;
        }

        return false;
    }

    virtual bool visit(MemberAccessAST *ast)
    {
        accept(ast->base_expression);
        return false;
    }

    const Identifier *_id;
    bool _block;
};

} // namespace anonymous

bool ResolveExpression::visit(SimpleNameAST *ast)
{
    QList<LookupItem> candidates = _context.lookup(ast->name, _scope);
    QList<LookupItem> newCandidates;

    for (QList<LookupItem>::iterator it = candidates.begin(); it != candidates.end(); ++ it) {
        LookupItem& item = *it;
        if (!item.type()->isUndefinedType())
            continue;

        if (item.declaration() == 0)
            continue;

        if (item.type().isAuto()) {
            const Declaration *decl = item.declaration()->asDeclaration();
            if (!decl)
                continue;

            const StringLiteral *initializationString = decl->getInitializer();
            if (initializationString == 0)
                continue;

            const QByteArray &initializer =
                    QByteArray::fromRawData(initializationString->chars(),
                                            initializationString->size()).trimmed();

            // Skip lambda-function initializers
            if (initializer.length() > 0 && initializer[0] == '[')
                continue;

            TypeOfExpression exprTyper;
            Document::Ptr doc = _context.snapshot().document(QString::fromLocal8Bit(decl->fileName()));
            exprTyper.init(doc, _context.snapshot(), _context.bindings());

            Document::Ptr exprDoc =
                    documentForExpression(exprTyper.preprocessedExpression(initializer));
            exprDoc->check();

            DeduceAutoCheck deduceAuto(ast->name->identifier(), exprDoc->translationUnit());
            if (deduceAuto._block)
                continue;

            const QList<LookupItem> &typeItems =
                    exprTyper(extractExpressionAST(exprDoc), exprDoc, decl->enclosingScope());
            if (typeItems.empty())
                continue;

            CPlusPlus::Clone cloner(_context.bindings()->control().data());

            for (int n = 0; n < typeItems.size(); ++ n) {
                FullySpecifiedType newType = cloner.type(typeItems[n].type(), 0);
                if (n == 0) {
                    item.setType(newType);
                    item.setScope(typeItems[n].scope());
                }
                else {
                    LookupItem newItem(item);
                    newItem.setType(newType);
                    newItem.setScope(typeItems[n].scope());
                    newCandidates.push_back(newItem);
                }
            }
        }
        else {
            item.setType(item.declaration()->type());
            item.setScope(item.declaration()->enclosingScope());
        }
    }

    addResults(candidates);
    addResults(newCandidates);
    return false;
}

bool ResolveExpression::visit(TemplateIdAST *ast)
{
    const QList<LookupItem> candidates = _context.lookup(ast->name, _scope);
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

bool ResolveExpression::maybeValidPrototype(Function *funTy, unsigned actualArgumentCount)
{
    return funTy->maybeValidPrototype(actualArgumentCount);
}

bool ResolveExpression::implicitConversion(const FullySpecifiedType &sourceTy, const FullySpecifiedType &targetTy) const
{
    if (sourceTy.isEqualTo(targetTy))
        return true;
    else if (sourceTy.simplified().isEqualTo(targetTy.simplified()))
        return true;
    return false;
}

bool ResolveExpression::visit(CallAST *ast)
{
    const QList<LookupItem> baseResults = resolve(ast->base_expression, _scope);

    // Compute the types of the actual arguments.
    unsigned actualArgumentCount = 0;

    QList< QList<LookupItem> > arguments;
    for (ExpressionListAST *exprIt = ast->expression_list; exprIt; exprIt = exprIt->next) {
        if (_reference)
            arguments.append(resolve(exprIt->value, _scope));

        ++actualArgumentCount;
    }

    if (_reference) {
        typedef std::multimap<int, LookupItem> LookupMap;
        LookupMap sortedResults;
        foreach (const LookupItem &base, baseResults) {
            if (Function *funTy = base.type()->asFunctionType()) {
                if (! maybeValidPrototype(funTy, actualArgumentCount))
                    continue;

                int score = 0;

                for (unsigned i = 0, argc = funTy->argumentCount(); i < argc; ++i) {
                    const FullySpecifiedType formalTy = funTy->argumentAt(i)->type();

                    FullySpecifiedType actualTy;
                    if (i < unsigned(arguments.size())) {
                        const QList<LookupItem> actual = arguments.at(i);
                        if (actual.isEmpty())
                            continue;

                        actualTy = actual.first().type();
                    } else
                        actualTy = formalTy;

                    if (implicitConversion(actualTy, formalTy))
                        ++score;
                }

                sortedResults.insert(LookupMap::value_type(-score, base));
            }
        }

        _results.clear();
        for (LookupMap::const_iterator it = sortedResults.begin(); it != sortedResults.end(); ++it)
            _results.append(it->second);
        if (_results.isEmpty())
            _results = baseResults;

        return false;
    }

    const Name *functionCallOp = control()->operatorNameId(OperatorNameId::FunctionCallOp);

    foreach (const LookupItem &result, baseResults) {
        FullySpecifiedType ty = result.type().simplified();
        Scope *scope = result.scope();

        if (NamedType *namedTy = ty->asNamedType()) {
            if (ClassOrNamespace *b = _context.lookupType(namedTy->name(), scope)) {
                foreach (const LookupItem &r, b->find(functionCallOp)) {
                    Symbol *overload = r.declaration();
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
    const QList<LookupItem> baseResults = resolve(ast->base_expression, _scope);
    const QList<LookupItem> indexResults = resolve(ast->expression, _scope);

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
                foreach (const LookupItem &r, b->find(arrayAccessOp)) {
                    Symbol *overload = r.declaration();
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

QList<LookupItem> ResolveExpression::getMembers(ClassOrNamespace *binding, const Name *memberName) const
{
    Q_UNUSED(binding);
    Q_UNUSED(memberName);

    // ### port me
    QList<LookupItem> members;
#if 0
    const QList<LookupItem> originalMembers = binding->find(memberName);

    foreach (const LookupItem &m, originalMembers) {
        if (! m.binding() || ! m.binding()->templateId()) {
            members.append(m);
            continue;
        }

        Symbol *decl = m.declaration();

        if (Class *klass = decl->scope()->asClass()) {
            if (klass->templateParameters() != 0) {
                SubstitutionMap map;

                const TemplateNameId *templateId = m.binding()->templateId();
                unsigned count = qMin(klass->templateParameterCount(), templateId->templateArgumentCount());

                for (unsigned i = 0; i < count; ++i) {
                    map.bind(klass->templateParameterAt(i)->name(), templateId->templateArgumentAt(i));
                }

                SubstitutionEnvironment env;
                if (m.scope())
                    env.switchScope(m.scope());
                env.setContext(_context);

                env.enter(&map);
                FullySpecifiedType instantiatedTy = rewriteType(decl->type(), &env, _context.control().data());

                Overview oo;
                oo.showReturnTypes = true;
                oo.showFunctionSignatures = true;

                qDebug() << "original:" << oo(decl->type(), decl->name()) << "inst:" << oo(instantiatedTy, decl->name());

                LookupItem newItem;
                newItem = m;
                newItem.setType(instantiatedTy);
                members.append(newItem);
            }
        }
    }
#endif

    return members;
}

bool ResolveExpression::visit(MemberAccessAST *ast)
{
    // The candidate types for the base expression are stored in
    // _results.
    const QList<LookupItem> baseResults = resolve(ast->base_expression, _scope);

    // Evaluate the expression-id that follows the access operator.
    const Name *memberName = 0;
    if (ast->member_name)
        memberName = ast->member_name->name;

    // Remember the access operator.
    const int accessOp = tokenKind(ast->access_token);

    if (ClassOrNamespace *binding = baseExpression(baseResults, accessOp))
        addResults(binding->find(memberName));

    return false;
}

ClassOrNamespace *ResolveExpression::findClass(const FullySpecifiedType &originalTy, Scope *scope,
                                               ClassOrNamespace* enclosingTemplateInstantiation) const
{
    FullySpecifiedType ty = originalTy.simplified();
    ClassOrNamespace *binding = 0;

    if (Class *klass = ty->asClassType())
        binding = _context.lookupType(klass, enclosingTemplateInstantiation);

    else if (NamedType *namedTy = ty->asNamedType())
        binding = _context.lookupType(namedTy->name(), scope, enclosingTemplateInstantiation);

    else if (Function *funTy = ty->asFunctionType())
        return findClass(funTy->returnType(), scope);

    return binding;
}

class TypedefsResolver
{
public:
    TypedefsResolver(const LookupContext &context) : _context(context) {}
    void resolve(FullySpecifiedType *type, Scope **scope, ClassOrNamespace *binding)
    {
        QSet<Symbol *> visited;
        _binding = binding;
        while (NamedType *namedTy = getNamedType(*type)) {
            QList<LookupItem> namedTypeItems = getNamedTypeItems(namedTy->name(), *scope, _binding);

#ifdef DEBUG_LOOKUP
            qDebug() << "-- we have" << namedTypeItems.size() << "candidates";
#endif // DEBUG_LOOKUP

            if (!findTypedef(namedTypeItems, type, scope, visited))
                break;
        }
    }

private:
    NamedType *getNamedType(FullySpecifiedType& type) const
    {
        NamedType *namedTy = type->asNamedType();
        if (! namedTy) {
            if (PointerType *pointerTy = type->asPointerType())
                namedTy = pointerTy->elementType()->asNamedType();
        }
        return namedTy;
    }

    QList<LookupItem> getNamedTypeItems(const Name *name, Scope *scope,
                                        ClassOrNamespace *binding) const
    {
        QList<LookupItem> namedTypeItems = typedefsFromScopeUpToFunctionScope(name, scope);
        if (namedTypeItems.isEmpty()) {
            if (binding)
                namedTypeItems = binding->lookup(name);
            if (ClassOrNamespace *scopeCon = _context.lookupType(scope))
                namedTypeItems += scopeCon->lookup(name);
        }

        return namedTypeItems;
    }

    /// Return all typedefs with given name from given scope up to function scope.
    static QList<LookupItem> typedefsFromScopeUpToFunctionScope(const Name *name, Scope *scope)
    {
        QList<LookupItem> results;
        Scope *enclosingBlockScope = 0;
        for (Block *block = scope->asBlock(); block;
             block = enclosingBlockScope ? enclosingBlockScope->asBlock() : 0) {
            const unsigned memberCount = block->memberCount();
            for (unsigned i = 0; i < memberCount; ++i) {
                Symbol *symbol = block->memberAt(i);
                if (Declaration *declaration = symbol->asDeclaration()) {
                    if (isTypedefWithName(declaration, name)) {
                        LookupItem item;
                        item.setDeclaration(declaration);
                        item.setScope(block);
                        item.setType(declaration->type());
                        results.append(item);
                    }
                }
            }
            enclosingBlockScope = block->enclosingScope();
        }
        return results;
    }

    static bool isTypedefWithName(const Declaration *declaration, const Name *name)
    {
        if (declaration->isTypedef()) {
            const Identifier *identifier = declaration->name()->identifier();
            if (name->identifier()->isEqualTo(identifier))
                return true;
        }
        return false;
    }

    bool findTypedef(const QList<LookupItem>& namedTypeItems, FullySpecifiedType *type,
                     Scope **scope, QSet<Symbol *>& visited)
    {
        bool foundTypedef = false;
        foreach (const LookupItem &it, namedTypeItems) {
            Symbol *declaration = it.declaration();
            if (declaration && declaration->isTypedef()) {
                if (visited.contains(declaration))
                    break;
                visited.insert(declaration);

                // continue working with the typedefed type and scope
                *type = declaration->type();
                *scope = it.scope();
                _binding = it.binding();
                foundTypedef = true;
                break;
            }
        }

        return foundTypedef;
    }

    const LookupContext &_context;
    // binding has to be remembered in case of resolving typedefs for templates
    ClassOrNamespace *_binding;
};

ClassOrNamespace *ResolveExpression::baseExpression(const QList<LookupItem> &baseResults,
                                                    int accessOp,
                                                    bool *replacedDotOperator) const
{
#ifdef DEBUG_LOOKUP
    qDebug() << "In ResolveExpression::baseExpression with" << baseResults.size() << "results...";
    int i = 0;
    Overview oo;
#endif // DEBUG_LOOKUP
    TypedefsResolver typedefsResolver(_context);

    foreach (const LookupItem &r, baseResults) {
        if (!r.type().type())
            continue;
        FullySpecifiedType ty = r.type().simplified();
        FullySpecifiedType originalType = ty;
        Scope *scope = r.scope();

#ifdef DEBUG_LOOKUP
        qDebug("trying result #%d", ++i);
        qDebug()<<"- before typedef resolving we have:"<<oo(ty);
#endif // DEBUG_LOOKUP

        typedefsResolver.resolve(&ty, &scope, r.binding());

#ifdef DEBUG_LOOKUP
        qDebug()<<"-  after typedef resolving:"<<oo(ty);
#endif // DEBUG_LOOKUP

        if (accessOp == T_ARROW) {
            if (PointerType *ptrTy = originalType->asPointerType()) {
                FullySpecifiedType type = ptrTy->elementType();
                if (! ty->isPointerType())
                    type = ty;
                if (ClassOrNamespace *binding = findClass(type, scope))
                    return binding;

            } else if (PointerType *ptrTy = ty->asPointerType()) {
                FullySpecifiedType type = ptrTy->elementType();
                if (ClassOrNamespace *binding = findClass(type, scope))
                    return binding;

            } else if (ClassOrNamespace *binding = findClass(ty, scope)) {
                // lookup for overloads of operator->

                const OperatorNameId *arrowOp = control()->operatorNameId(OperatorNameId::ArrowOp);
                foreach (const LookupItem &r, binding->find(arrowOp)) {
                    Symbol *overload = r.declaration();
                    if (! overload)
                        continue;
                    Scope *functionScope = overload->enclosingScope();

                    if (overload->type()->isFunctionType()) {
                        FullySpecifiedType overloadTy = instantiate(binding->templateId(), overload);
                        Function *instantiatedFunction = overloadTy->asFunctionType();
                        Q_ASSERT(instantiatedFunction != 0);

                        FullySpecifiedType retTy = instantiatedFunction->returnType().simplified();

                        typedefsResolver.resolve(&retTy, &functionScope, r.binding());

                        if (! retTy->isPointerType() && ! retTy->isNamedType())
                            continue;

                        if (PointerType *ptrTy = retTy->asPointerType())
                            retTy = ptrTy->elementType();

                        if (ClassOrNamespace *retBinding = findClass(retTy, functionScope))
                            return retBinding;

                        if (scope != functionScope) {
                            if (ClassOrNamespace *retBinding = findClass(retTy, scope))
                                return retBinding;
                        }

                        if (ClassOrNamespace *origin = binding->instantiationOrigin()) {
                            foreach (Symbol *originSymbol, origin->symbols()) {
                                Scope *originScope = originSymbol->asScope();
                                if (originScope && originScope != scope && originScope != functionScope) {
                                    if (ClassOrNamespace *retBinding = findClass(retTy, originScope))
                                        return retBinding;
                                }
                            }
                        }
                    }
                }
            }
        } else if (accessOp == T_DOT) {
            if (replacedDotOperator) {
                *replacedDotOperator = originalType->isPointerType() || ty->isPointerType();
                // replace . with ->
                if (PointerType *ptrTy = originalType->asPointerType()) {
                    // case when original type is a pointer and
                    // typedef is for type
                    // e.g.:
                    // typedef S SType;
                    // SType *p;
                    ty = ptrTy->elementType();
                }
                else if (PointerType *ptrTy = ty->asPointerType()) {
                    // case when original type is a type and
                    // typedef is for pointer of type
                    // e.g.:
                    // typedef S* SPTR;
                    // SPTR p;
                    ty = ptrTy->elementType();
                }
            }

            ClassOrNamespace *enclosingTemplateInstantiation = 0;
            if (ClassOrNamespace *binding = r.binding()) {
                if (binding->instantiationOrigin())
                    enclosingTemplateInstantiation = binding;
            }

            if (ClassOrNamespace *binding = findClass(ty, scope, enclosingTemplateInstantiation))
                return binding;
        }
    }

    return 0;
}

FullySpecifiedType ResolveExpression::instantiate(const Name *className, Symbol *candidate) const
{
    return DeprecatedGenTemplateInstance::instantiate(className, candidate,
                                                      _context.bindings()->control());
}

bool ResolveExpression::visit(PostIncrDecrAST *ast)
{
    const QList<LookupItem> baseResults = resolve(ast->base_expression, _scope);
    _results = baseResults;
    return false;
}

bool ResolveExpression::visit(ObjCMessageExpressionAST *ast)
{
    const QList<LookupItem> receiverResults = resolve(ast->receiver_expression, _scope);

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
            foreach (const LookupItem &r, binding->lookup(ast->selector->name)) {
                Symbol *s = r.declaration();
                if (ObjCMethod *m = s->asObjCMethod())
                    addResult(m->returnType(), result.scope());
            }
        }
    }

    return false;
}

const LookupContext &ResolveExpression::context() const
{
    return _context;
}
