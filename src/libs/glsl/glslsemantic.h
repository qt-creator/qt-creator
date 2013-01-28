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

#ifndef GLSLSEMANTIC_H
#define GLSLSEMANTIC_H

#include "glslastvisitor.h"
#include "glsltype.h"

namespace GLSL {

class GLSL_EXPORT Semantic: protected Visitor
{
public:
    Semantic();
    virtual ~Semantic();

    struct ExprResult {
        ExprResult(const Type *type = 0, bool isConstant = false)
            : type(type), isConstant(isConstant) {}

        ~ExprResult() { }

        bool isValid() const {
            if (! type)
                return false;
            else if (type->asUndefinedType() != 0)
                return false;
            return true;
        }

        operator bool() const { return isValid(); }

        const Type *type;
        bool isConstant;
    };

    void translationUnit(TranslationUnitAST *ast, Scope *globalScope, Engine *engine);
    ExprResult expression(ExpressionAST *ast, Scope *scope, Engine *engine);

protected:
    Engine *switchEngine(Engine *engine);
    Scope *switchScope(Scope *scope);

    bool implicitCast(const Type *type, const Type *target) const;

    ExprResult expression(ExpressionAST *ast);
    void statement(StatementAST *ast);
    const Type *type(TypeAST *ast);
    void declaration(DeclarationAST *ast);
    ExprResult functionIdentifier(FunctionIdentifierAST *ast);
    Symbol *field(StructTypeAST::Field *ast);
    void parameterDeclaration(ParameterDeclarationAST *ast, Function *fun);

    virtual bool visit(TranslationUnitAST *ast);
    virtual bool visit(FunctionIdentifierAST *ast);
    virtual bool visit(StructTypeAST::Field *ast);

    // expressions
    virtual bool visit(IdentifierExpressionAST *ast);
    virtual bool visit(LiteralExpressionAST *ast);
    virtual bool visit(BinaryExpressionAST *ast);
    virtual bool visit(UnaryExpressionAST *ast);
    virtual bool visit(TernaryExpressionAST *ast);
    virtual bool visit(AssignmentExpressionAST *ast);
    virtual bool visit(MemberAccessExpressionAST *ast);
    virtual bool visit(FunctionCallExpressionAST *ast);
    virtual bool visit(DeclarationExpressionAST *ast);

    // statements
    virtual bool visit(ExpressionStatementAST *ast);
    virtual bool visit(CompoundStatementAST *ast);
    virtual bool visit(IfStatementAST *ast);
    virtual bool visit(WhileStatementAST *ast);
    virtual bool visit(DoStatementAST *ast);
    virtual bool visit(ForStatementAST *ast);
    virtual bool visit(JumpStatementAST *ast);
    virtual bool visit(ReturnStatementAST *ast);
    virtual bool visit(SwitchStatementAST *ast);
    virtual bool visit(CaseLabelStatementAST *ast);
    virtual bool visit(DeclarationStatementAST *ast);

    // types
    virtual bool visit(BasicTypeAST *ast);
    virtual bool visit(NamedTypeAST *ast);
    virtual bool visit(ArrayTypeAST *ast);
    virtual bool visit(StructTypeAST *ast);
    virtual bool visit(QualifiedTypeAST *ast);

    // declarations
    virtual bool visit(PrecisionDeclarationAST *ast);
    virtual bool visit(ParameterDeclarationAST *ast);
    virtual bool visit(VariableDeclarationAST *ast);
    virtual bool visit(TypeDeclarationAST *ast);
    virtual bool visit(TypeAndVariableDeclarationAST *ast);
    virtual bool visit(InvariantDeclarationAST *ast);
    virtual bool visit(InitDeclarationAST *ast);
    virtual bool visit(FunctionDeclarationAST *ast);

private:
    Engine *_engine;
    Scope *_scope;
    const Type *_type;
    ExprResult _expr;
};

} // namespace GLSL

#endif // GLSLSEMANTIC_H
