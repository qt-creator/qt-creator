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
#ifndef GLSLSEMANTIC_H
#define GLSLSEMANTIC_H

#include "glslastvisitor.h"

namespace GLSL {

class GLSL_EXPORT Semantic: protected Visitor
{
public:
    Semantic();
    virtual ~Semantic();

    void expression(Expression *ast);
    void statement(Statement *ast);
    void type(Type *ast);
    void declaration(Declaration *ast);
    void translationUnit(TranslationUnit *ast);
    void functionIdentifier(FunctionIdentifier *ast);
    void field(StructType::Field *ast);

protected:
    virtual bool visit(TranslationUnit *ast);
    virtual bool visit(FunctionIdentifier *ast);
    virtual bool visit(StructType::Field *ast);

    // expressions
    virtual bool visit(IdentifierExpression *ast);
    virtual bool visit(LiteralExpression *ast);
    virtual bool visit(BinaryExpression *ast);
    virtual bool visit(UnaryExpression *ast);
    virtual bool visit(TernaryExpression *ast);
    virtual bool visit(AssignmentExpression *ast);
    virtual bool visit(MemberAccessExpression *ast);
    virtual bool visit(FunctionCallExpression *ast);
    virtual bool visit(DeclarationExpression *ast);

    // statements
    virtual bool visit(ExpressionStatement *ast);
    virtual bool visit(CompoundStatement *ast);
    virtual bool visit(IfStatement *ast);
    virtual bool visit(WhileStatement *ast);
    virtual bool visit(DoStatement *ast);
    virtual bool visit(ForStatement *ast);
    virtual bool visit(JumpStatement *ast);
    virtual bool visit(ReturnStatement *ast);
    virtual bool visit(SwitchStatement *ast);
    virtual bool visit(CaseLabelStatement *ast);
    virtual bool visit(DeclarationStatement *ast);

    // types
    virtual bool visit(BasicType *ast);
    virtual bool visit(NamedType *ast);
    virtual bool visit(ArrayType *ast);
    virtual bool visit(StructType *ast);
    virtual bool visit(QualifiedType *ast);

    // declarations
    virtual bool visit(PrecisionDeclaration *ast);
    virtual bool visit(ParameterDeclaration *ast);
    virtual bool visit(VariableDeclaration *ast);
    virtual bool visit(TypeDeclaration *ast);
    virtual bool visit(TypeAndVariableDeclaration *ast);
    virtual bool visit(InvariantDeclaration *ast);
    virtual bool visit(InitDeclaration *ast);
    virtual bool visit(FunctionDeclaration *ast);
};

} // namespace GLSL

#endif // GLSLSEMANTIC_H
