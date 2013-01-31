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

#ifndef CPLUSPLUS_RESOLVEEXPRESSION_H
#define CPLUSPLUS_RESOLVEEXPRESSION_H

#include "LookupContext.h"

#include <ASTVisitor.h>
#include <FullySpecifiedType.h>
#include <Bind.h>
#include <set>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT ResolveExpression: protected ASTVisitor
{
public:
    ResolveExpression(const LookupContext &context);
    virtual ~ResolveExpression();

    QList<LookupItem> operator()(ExpressionAST *ast, Scope *scope);
    QList<LookupItem> resolve(ExpressionAST *ast, Scope *scope, bool ref = false);
    QList<LookupItem> reference(ExpressionAST *ast, Scope *scope);

    ClassOrNamespace *baseExpression(const QList<LookupItem> &baseResults,
                                     int accessOp,
                                     bool *replacedDotOperator = 0) const;

    const LookupContext &context() const;

protected:
    ClassOrNamespace *findClass(const FullySpecifiedType &ty, Scope *scope,
                                ClassOrNamespace* enclosingTemplateInstantiation = 0) const;

    QList<LookupItem> expression(ExpressionAST *ast);

    QList<LookupItem> switchResults(const QList<LookupItem> &symbols);
    FullySpecifiedType instantiate(const Name *className, Symbol *candidate) const;

    QList<LookupItem> getMembers(ClassOrNamespace *binding, const Name *memberName) const;

    void thisObject();

    void addResult(const FullySpecifiedType &ty, Scope *scope);
    void addResults(const QList<Symbol *> &symbols);
    void addResults(const QList<LookupItem> &items);

    static bool maybeValidPrototype(Function *funTy, unsigned actualArgumentCount);
    bool implicitConversion(const FullySpecifiedType &sourceTy, const FullySpecifiedType &targetTy) const;

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
    Scope *_scope;
    LookupContext _context;
    Bind bind;
    QList<LookupItem> _results;
    bool _reference;
};

} // namespace CPlusPlus

#endif // CPLUSPLUS_RESOLVEEXPRESSION_H
