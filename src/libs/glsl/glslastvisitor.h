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
#ifndef GLSLASTVISITOR_H
#define GLSLASTVISITOR_H

#include "glslast.h"

namespace GLSL {

class GLSL_EXPORT Visitor
{
public:
    Visitor();
    virtual ~Visitor();

    virtual bool preVisit(AST *) { return true; }
    virtual void postVisit(AST *) {}

    virtual bool visit(TranslationUnit *) { return true; }
    virtual void endVisit(TranslationUnit *) {}

    virtual bool visit(IdentifierExpression *) { return true; }
    virtual void endVisit(IdentifierExpression *) {}

    virtual bool visit(LiteralExpression *) { return true; }
    virtual void endVisit(LiteralExpression *) {}

    virtual bool visit(BinaryExpression *) { return true; }
    virtual void endVisit(BinaryExpression *) {}

    virtual bool visit(UnaryExpression *) { return true; }
    virtual void endVisit(UnaryExpression *) {}

    virtual bool visit(TernaryExpression *) { return true; }
    virtual void endVisit(TernaryExpression *) {}

    virtual bool visit(AssignmentExpression *) { return true; }
    virtual void endVisit(AssignmentExpression *) {}

    virtual bool visit(MemberAccessExpression *) { return true; }
    virtual void endVisit(MemberAccessExpression *) {}

    virtual bool visit(FunctionCallExpression *) { return true; }
    virtual void endVisit(FunctionCallExpression *) {}

    virtual bool visit(FunctionIdentifier *) { return true; }
    virtual void endVisit(FunctionIdentifier *) {}

    virtual bool visit(ExpressionStatement *) { return true; }
    virtual void endVisit(ExpressionStatement *) {}

    virtual bool visit(CompoundStatement *) { return true; }
    virtual void endVisit(CompoundStatement *) {}

    virtual bool visit(IfStatement *) { return true; }
    virtual void endVisit(IfStatement *) {}

    virtual bool visit(WhileStatement *) { return true; }
    virtual void endVisit(WhileStatement *) {}

    virtual bool visit(DoStatement *) { return true; }
    virtual void endVisit(DoStatement *) {}

    virtual bool visit(ForStatement *) { return true; }
    virtual void endVisit(ForStatement *) {}

    virtual bool visit(JumpStatement *) { return true; }
    virtual void endVisit(JumpStatement *) {}

    virtual bool visit(ReturnStatement *) { return true; }
    virtual void endVisit(ReturnStatement *) {}

    virtual bool visit(SwitchStatement *) { return true; }
    virtual void endVisit(SwitchStatement *) {}

    virtual bool visit(CaseLabelStatement *) { return true; }
    virtual void endVisit(CaseLabelStatement *) {}

    virtual bool visit(BasicType *) { return true; }
    virtual void endVisit(BasicType *) {}

    virtual bool visit(NamedType *) { return true; }
    virtual void endVisit(NamedType *) {}

    virtual bool visit(ArrayType *) { return true; }
    virtual void endVisit(ArrayType *) {}

    virtual bool visit(StructType *) { return true; }
    virtual void endVisit(StructType *) {}

    virtual bool visit(StructType::Field *) { return true; }
    virtual void endVisit(StructType::Field *) {}

    virtual bool visit(QualifiedType *) { return true; }
    virtual void endVisit(QualifiedType *) {}

    virtual bool visit(PrecisionDeclaration *) { return true; }
    virtual void endVisit(PrecisionDeclaration *) {}
};

} // namespace GLSL

#endif // GLSLASTVISITOR_H
