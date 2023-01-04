// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "LookupContext.h"

#include <cplusplus/ASTVisitor.h>
#include <cplusplus/FullySpecifiedType.h>
#include <cplusplus/Bind.h>

#include <set>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT ResolveExpression: protected ASTVisitor
{
public:
    ResolveExpression(const LookupContext &context,
                      const QSet<const Declaration *> &autoDeclarationsBeingResolved
                        = QSet<const Declaration *>());
    virtual ~ResolveExpression();

    QList<LookupItem> operator()(ExpressionAST *ast, Scope *scope);
    QList<LookupItem> resolve(ExpressionAST *ast, Scope *scope, bool ref = false);
    QList<LookupItem> reference(ExpressionAST *ast, Scope *scope);

    ClassOrNamespace *baseExpression(const QList<LookupItem> &baseResults,
                                     int accessOp,
                                     bool *replacedDotOperator = nullptr) const;

    const LookupContext &context() const;

protected:
    ClassOrNamespace *findClass(const FullySpecifiedType &ty, Scope *scope,
                                ClassOrNamespace *enclosingBinding = nullptr) const;

    QList<LookupItem> expression(ExpressionAST *ast);

    QList<LookupItem> switchResults(const QList<LookupItem> &symbols);
    FullySpecifiedType instantiate(const Name *className, Symbol *candidate) const;

    QList<LookupItem> getMembers(ClassOrNamespace *binding, const Name *memberName) const;

    void thisObject();

    void addResult(const FullySpecifiedType &ty, Scope *scope, ClassOrNamespace *binding = nullptr);
    void addResults(const QList<Symbol *> &symbols);
    void addResults(const QList<LookupItem> &items);

    static bool maybeValidPrototype(Function *funTy, unsigned actualArgumentCount);

    using ASTVisitor::visit;

    virtual bool visit(IdExpressionAST *ast);
    virtual bool visit(BinaryExpressionAST *ast);
    virtual bool visit(CastExpressionAST *ast);
    virtual bool visit(ConditionAST *ast);
    virtual bool visit(ConditionalExpressionAST *ast);
    virtual bool visit(CppCastExpressionAST *ast);
    virtual bool visit(DeleteExpressionAST *ast);
    virtual bool visit(ArrayInitializerAST *ast);
    virtual bool visit(NewExpressionAST *ast);
    virtual bool visit(TypeidExpressionAST *ast);
    virtual bool visit(TypenameCallExpressionAST *ast);
    virtual bool visit(TypeConstructorCallAST *ast);
    virtual bool visit(SizeofExpressionAST *ast);
    virtual bool visit(PointerLiteralAST *ast);
    virtual bool visit(NumericLiteralAST *ast);
    virtual bool visit(BoolLiteralAST *ast);
    virtual bool visit(ThisExpressionAST *ast);
    virtual bool visit(NestedExpressionAST *ast);
    virtual bool visit(StringLiteralAST *ast);
    virtual bool visit(ThrowExpressionAST *ast);
    virtual bool visit(TypeIdAST *ast);
    virtual bool visit(UnaryExpressionAST *ast);
    virtual bool visit(CompoundLiteralAST *ast);
    virtual bool visit(CompoundExpressionAST *ast);
    virtual bool visit(LambdaExpressionAST *ast);
    virtual bool visit(ReturnStatementAST *ast);

    //names
    virtual bool visit(QualifiedNameAST *ast);
    virtual bool visit(OperatorFunctionIdAST *ast);
    virtual bool visit(ConversionFunctionIdAST *ast);
    virtual bool visit(SimpleNameAST *ast);
    virtual bool visit(DestructorNameAST *ast);
    virtual bool visit(TemplateIdAST *ast);

    // postfix expressions
    virtual bool visit(CallAST *ast);
    virtual bool visit(ArrayAccessAST *ast);
    virtual bool visit(PostIncrDecrAST *ast);
    virtual bool visit(MemberAccessAST *ast);

    // Objective-C expressions
    virtual bool visit(ObjCMessageExpressionAST *ast);


private:
    ClassOrNamespace *findClassForTemplateParameterInExpressionScope(
            ClassOrNamespace *resultBinding,
            const FullySpecifiedType &ty) const;

    Scope *_scope;
    const LookupContext& _context;
    Bind bind;
    QList<LookupItem> _results;
    QSet<const Declaration *> _autoDeclarationsBeingResolved;
    bool _reference;
};

} // namespace CPlusPlus
