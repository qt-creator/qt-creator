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

#ifndef GLSLASTVISITOR_H
#define GLSLASTVISITOR_H

#include "glslast.h"

namespace GLSL {

class GLSL_EXPORT Visitor
{
public:
    Visitor();
    virtual ~Visitor();

    void accept(AST *ast);

    virtual bool preVisit(AST *) { return true; }
    virtual void postVisit(AST *) {}

    virtual bool visit(TranslationUnitAST *) { return true; }
    virtual void endVisit(TranslationUnitAST *) {}

    virtual bool visit(IdentifierExpressionAST *) { return true; }
    virtual void endVisit(IdentifierExpressionAST *) {}

    virtual bool visit(LiteralExpressionAST *) { return true; }
    virtual void endVisit(LiteralExpressionAST *) {}

    virtual bool visit(BinaryExpressionAST *) { return true; }
    virtual void endVisit(BinaryExpressionAST *) {}

    virtual bool visit(UnaryExpressionAST *) { return true; }
    virtual void endVisit(UnaryExpressionAST *) {}

    virtual bool visit(TernaryExpressionAST *) { return true; }
    virtual void endVisit(TernaryExpressionAST *) {}

    virtual bool visit(AssignmentExpressionAST *) { return true; }
    virtual void endVisit(AssignmentExpressionAST *) {}

    virtual bool visit(MemberAccessExpressionAST *) { return true; }
    virtual void endVisit(MemberAccessExpressionAST *) {}

    virtual bool visit(FunctionCallExpressionAST *) { return true; }
    virtual void endVisit(FunctionCallExpressionAST *) {}

    virtual bool visit(FunctionIdentifierAST *) { return true; }
    virtual void endVisit(FunctionIdentifierAST *) {}

    virtual bool visit(DeclarationExpressionAST *) { return true; }
    virtual void endVisit(DeclarationExpressionAST *) {}

    virtual bool visit(ExpressionStatementAST *) { return true; }
    virtual void endVisit(ExpressionStatementAST *) {}

    virtual bool visit(CompoundStatementAST *) { return true; }
    virtual void endVisit(CompoundStatementAST *) {}

    virtual bool visit(IfStatementAST *) { return true; }
    virtual void endVisit(IfStatementAST *) {}

    virtual bool visit(WhileStatementAST *) { return true; }
    virtual void endVisit(WhileStatementAST *) {}

    virtual bool visit(DoStatementAST *) { return true; }
    virtual void endVisit(DoStatementAST *) {}

    virtual bool visit(ForStatementAST *) { return true; }
    virtual void endVisit(ForStatementAST *) {}

    virtual bool visit(JumpStatementAST *) { return true; }
    virtual void endVisit(JumpStatementAST *) {}

    virtual bool visit(ReturnStatementAST *) { return true; }
    virtual void endVisit(ReturnStatementAST *) {}

    virtual bool visit(SwitchStatementAST *) { return true; }
    virtual void endVisit(SwitchStatementAST *) {}

    virtual bool visit(CaseLabelStatementAST *) { return true; }
    virtual void endVisit(CaseLabelStatementAST *) {}

    virtual bool visit(DeclarationStatementAST *) { return true; }
    virtual void endVisit(DeclarationStatementAST *) {}

    virtual bool visit(BasicTypeAST *) { return true; }
    virtual void endVisit(BasicTypeAST *) {}

    virtual bool visit(NamedTypeAST *) { return true; }
    virtual void endVisit(NamedTypeAST *) {}

    virtual bool visit(ArrayTypeAST *) { return true; }
    virtual void endVisit(ArrayTypeAST *) {}

    virtual bool visit(StructTypeAST *) { return true; }
    virtual void endVisit(StructTypeAST *) {}

    virtual bool visit(StructTypeAST::Field *) { return true; }
    virtual void endVisit(StructTypeAST::Field *) {}

    virtual bool visit(LayoutQualifierAST *) { return true; }
    virtual void endVisit(LayoutQualifierAST *) {}

    virtual bool visit(QualifiedTypeAST *) { return true; }
    virtual void endVisit(QualifiedTypeAST *) {}

    virtual bool visit(PrecisionDeclarationAST *) { return true; }
    virtual void endVisit(PrecisionDeclarationAST *) {}

    virtual bool visit(ParameterDeclarationAST *) { return true; }
    virtual void endVisit(ParameterDeclarationAST *) {}

    virtual bool visit(VariableDeclarationAST *) { return true; }
    virtual void endVisit(VariableDeclarationAST *) {}

    virtual bool visit(TypeDeclarationAST *) { return true; }
    virtual void endVisit(TypeDeclarationAST *) {}

    virtual bool visit(TypeAndVariableDeclarationAST *) { return true; }
    virtual void endVisit(TypeAndVariableDeclarationAST *) {}

    virtual bool visit(InvariantDeclarationAST *) { return true; }
    virtual void endVisit(InvariantDeclarationAST *) {}

    virtual bool visit(InitDeclarationAST *) { return true; }
    virtual void endVisit(InitDeclarationAST *) {}

    virtual bool visit(FunctionDeclarationAST *) { return true; }
    virtual void endVisit(FunctionDeclarationAST *) {}
};

} // namespace GLSL

#endif // GLSLASTVISITOR_H
