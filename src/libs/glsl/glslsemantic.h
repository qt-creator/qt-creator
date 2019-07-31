/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "glslastvisitor.h"
#include "glsltype.h"

namespace GLSL {

class GLSL_EXPORT Semantic: protected Visitor
{
public:
    Semantic();
    ~Semantic() override;

    struct ExprResult {
        ExprResult(const Type *type = nullptr, bool isConstant = false)
            : type(type), isConstant(isConstant) {}

        ~ExprResult() { }

        bool isValid() const {
            if (! type)
                return false;
            else if (type->asUndefinedType() != nullptr)
                return false;
            return true;
        }

        explicit operator bool() const { return isValid(); }

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

    bool visit(TranslationUnitAST *ast) override;
    bool visit(FunctionIdentifierAST *ast) override;
    bool visit(StructTypeAST::Field *ast) override;

    // expressions
    bool visit(IdentifierExpressionAST *ast) override;
    bool visit(LiteralExpressionAST *ast) override;
    bool visit(BinaryExpressionAST *ast) override;
    bool visit(UnaryExpressionAST *ast) override;
    bool visit(TernaryExpressionAST *ast) override;
    bool visit(AssignmentExpressionAST *ast) override;
    bool visit(MemberAccessExpressionAST *ast) override;
    bool visit(FunctionCallExpressionAST *ast) override;
    bool visit(DeclarationExpressionAST *ast) override;

    // statements
    bool visit(ExpressionStatementAST *ast) override;
    bool visit(CompoundStatementAST *ast) override;
    bool visit(IfStatementAST *ast) override;
    bool visit(WhileStatementAST *ast) override;
    bool visit(DoStatementAST *ast) override;
    bool visit(ForStatementAST *ast) override;
    bool visit(JumpStatementAST *ast) override;
    bool visit(ReturnStatementAST *ast) override;
    bool visit(SwitchStatementAST *ast) override;
    bool visit(CaseLabelStatementAST *ast) override;
    bool visit(DeclarationStatementAST *ast) override;

    // types
    bool visit(BasicTypeAST *ast) override;
    bool visit(NamedTypeAST *ast) override;
    bool visit(ArrayTypeAST *ast) override;
    bool visit(StructTypeAST *ast) override;
    bool visit(QualifiedTypeAST *ast) override;

    // declarations
    bool visit(PrecisionDeclarationAST *ast) override;
    bool visit(ParameterDeclarationAST *ast) override;
    bool visit(VariableDeclarationAST *ast) override;
    bool visit(TypeDeclarationAST *ast) override;
    bool visit(TypeAndVariableDeclarationAST *ast) override;
    bool visit(InvariantDeclarationAST *ast) override;
    bool visit(InitDeclarationAST *ast) override;
    bool visit(FunctionDeclarationAST *ast) override;

private:
    Engine *_engine;
    Scope *_scope;
    const Type *_type;
    ExprResult _expr;
};

} // namespace GLSL
