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

#include "PrettyPrinter.h"
#include "AST.h"
#include "Token.h"
#include <iostream>
#include <string>
#include <cassert>

CPLUSPLUS_USE_NAMESPACE

PrettyPrinter::PrettyPrinter(Control *control, std::ostream &out)
    : ASTVisitor(control),
      out(out),
      depth(0)
{ }

void PrettyPrinter::operator()(AST *ast)
{ accept(ast); }

void PrettyPrinter::indent()
{ ++depth; }

void PrettyPrinter::deindent()
{ --depth; }

void PrettyPrinter::newline()
{ out << '\n' << std::string(depth * 4, ' '); }

bool PrettyPrinter::visit(AccessDeclarationAST *ast)
{
    deindent();
    newline();
    out << spell(ast->access_specifier_token);
    if (ast->slots_token)
        out << ' ' << spell(ast->slots_token);
    out << ':' << std::endl;
    indent();
    return false;
}

bool PrettyPrinter::visit(ArrayAccessAST *ast)
{
    out << '[';
    accept(ast->expression);
    out << ']';
    return false;
}

bool PrettyPrinter::visit(ArrayDeclaratorAST *ast)
{
    out << '[';
    accept(ast->expression);
    out << ']';
    return false;
}

bool PrettyPrinter::visit(ArrayInitializerAST *ast)
{
    out << '{';
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        accept(it->expression);
        if (it->next)
            out << ", ";
    }
    out << '}';
    return false;
}

bool PrettyPrinter::visit(AsmDefinitionAST *ast)
{
    out << spell(ast->asm_token);
    for (SpecifierAST *it = ast->cv_qualifier_seq; it; it = it->next) {
        out << ' ';
        accept(it);
    }
    out << '(';
    out << "/* ### implement me */";
    out << ");";
    return false;
}

bool PrettyPrinter::visit(AttributeSpecifierAST *ast)
{
    out << "attribute((";
    for (AttributeAST *it = ast->attributes; it; it = it->next) {
        accept(it);
        if (it->next)
            out << ", ";
    }
    out << "))";
    return false;
}

bool PrettyPrinter::visit(AttributeAST *ast)
{
    out << spell(ast->identifier_token);
    if (ast->lparen_token) {
        out << '(';
        out << spell(ast->tag_token);
        if (ast->expression_list) {
            out << '(';
            for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
                accept(it->expression);
                if (it->next)
                    out << ", ";
            }
            out << ')';
        }
        out << ')';
    }
    return false;
}

bool PrettyPrinter::visit(BaseSpecifierAST *ast)
{
    if (ast->token_virtual && ast->token_access_specifier) {
        out << "virtual";
        out << ' ';
        out << spell(ast->token_access_specifier);
        out << ' ';
    } else if (ast->token_virtual) {
        out << "virtual";
        out << ' ';
    } else if (ast->token_access_specifier) {
        out << spell(ast->token_access_specifier);
        out << ' ';
    }
    accept(ast->name);
    return false;
}

bool PrettyPrinter::visit(BinaryExpressionAST *ast)
{
    accept(ast->left_expression);
    out << ' '  << spell(ast->binary_op_token) << ' ';
    accept(ast->right_expression);
    return false;
}

bool PrettyPrinter::visit(BoolLiteralAST *ast)
{
    out << spell(ast->token);
    return false;
}

bool PrettyPrinter::visit(BreakStatementAST *)
{
    out << "break;";
    return false;
}

bool PrettyPrinter::visit(CallAST *ast)
{
    out << '(';
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        accept(it->expression);
        if (it->next)
            out << ", ";
    }
    out << ')';
    return false;
}

bool PrettyPrinter::visit(CaseStatementAST *ast)
{
    out << "case ";
    accept(ast->expression);
    out << ':';
    if (! ast->statement) {
        newline();
        return false;
    }

    if (ast->statement->asCompoundStatement()) {
        out << ' ';
        accept(ast->statement);
    } else if (ast->statement->asCaseStatement() || ast->statement->asLabeledStatement()) {
        newline();
        accept(ast->statement);
    } else {
        indent();
        newline();
        accept(ast->statement);
        deindent();
        newline();
    }
    return false;
}

bool PrettyPrinter::visit(CastExpressionAST *ast)
{
    out << '(';
    accept(ast->type_id);
    out << ')';
    accept(ast->expression);
    return false;
}

bool PrettyPrinter::visit(CatchClauseAST *ast)
{
    out << "catch";
    out << '(';
    accept(ast->exception_declaration);
    out << ')';
    accept(ast->statement);
    return false;
}

bool PrettyPrinter::visit(ClassSpecifierAST *ast)
{
    out << spell(ast->classkey_token);
    out << ' ';
    if (ast->attributes) {
        accept(ast->attributes);
        out << ' ';
    }
    accept(ast->name);
    if (ast->colon_token) {
        out << ':';
        out << ' ';
        for (BaseSpecifierAST *it = ast->base_clause; it; it = it->next) {
            accept(it);
            if (it->next)
                out << ", ";
        }
    }
    newline();
    out << '{';
    if (ast->member_specifiers) {
        indent();
        newline();
        if (ast->member_specifiers) {
            for (DeclarationAST *it = ast->member_specifiers; it; it = it->next) {
                accept(it);
                if (it->next)
                    newline();
            }
        }
        deindent();
        newline();
    }
    out << '}';
    return false;
}

bool PrettyPrinter::visit(CompoundStatementAST *ast)
{
    out << '{';
    if (ast->statements) {
        indent();
        newline();
        for (StatementAST *it = ast->statements; it; it = it->next) {
            accept(it);
            if (it->next)
                newline();
        }
        deindent();
        newline();
    }
    out << '}';
    return false;
}

bool PrettyPrinter::visit(ConditionAST *ast)
{
    for (SpecifierAST *it = ast->type_specifier; it; it = it->next) {
        accept(it);
        if (it->next)
            out << ' ';
    }
    if (ast->declarator) {
        if (ast->type_specifier)
            out << ' ';

        accept(ast->declarator);
    }
    return false;
}

bool PrettyPrinter::visit(ConditionalExpressionAST *ast)
{
    accept(ast->condition);
    out << '?';
    accept(ast->left_expression);
    out << ':';
    accept(ast->right_expression);
    return false;
}

bool PrettyPrinter::visit(ContinueStatementAST *)
{
    out << "continue";
    out << ';';
    return false;
}

bool PrettyPrinter::visit(ConversionFunctionIdAST *ast)
{
    out << "operator";
    out << ' ';
    for (SpecifierAST *it = ast->type_specifier; it; it = it->next) {
        accept(it);
        if (it->next)
            out << ' ';
    }
    for (PtrOperatorAST *it = ast->ptr_operators; it; it = it->next) {
        accept(it);
    }
    return false;
}

bool PrettyPrinter::visit(CppCastExpressionAST *ast)
{
    out << spell(ast->cast_token);
    out << '<';
    out << ' ';
    accept(ast->type_id);
    out << ' ';
    out << '>';
    out << '(';
    accept(ast->expression);
    out << ')';
    return false;
}

bool PrettyPrinter::visit(CtorInitializerAST *ast)
{
    out << ':';
    out << ' ';
    for (MemInitializerAST *it = ast->member_initializers; it; it = it->next) {
        accept(it->name);
        out << '(';
        accept(it->expression);
        out << ')';
        if (it->next)
            out << ", ";
    }
    return false;
}

bool PrettyPrinter::visit(DeclaratorAST *ast)
{
    for (PtrOperatorAST *it = ast->ptr_operators; it; it = it->next) {
        accept(it);
    }
    if (ast->core_declarator) {
        if (ast->ptr_operators)
            out << ' ';
        accept(ast->core_declarator);
    }
    for (PostfixDeclaratorAST *it = ast->postfix_declarators; it; it = it->next) {
        accept(it);
    }
    for (SpecifierAST *it = ast->attributes; it; it = it->next) {
        accept(it);
        if (it->next)
            out << ' ';
    }
    if (ast->initializer) {
        out << ' ';
        out << '=';
        out << ' ';
        accept(ast->initializer);
    }
    return false;
}

bool PrettyPrinter::visit(DeclarationStatementAST *ast)
{
    accept(ast->declaration);
    return false;
}

bool PrettyPrinter::visit(DeclaratorIdAST *ast)
{
    accept(ast->name);
    return false;
}

bool PrettyPrinter::visit(DeclaratorListAST *ast)
{
    for (DeclaratorListAST *it = ast; it; it = it->next) {
        accept(it->declarator);
        if (it->next)
            out << ", ";
    }
    return false;
}

bool PrettyPrinter::visit(DeleteExpressionAST *ast)
{
    if (ast->scope_token)
        out << "::";
    out << "delete";
    if (ast->expression) {
        out << ' ';
        accept(ast->expression);
    }
    return false;
}

bool PrettyPrinter::visit(DestructorNameAST *ast)
{
    out << '~';
    out << spell(ast->identifier_token);
    return false;
}

bool PrettyPrinter::visit(DoStatementAST *ast)
{
    out << "do";
    if (ast->statement) {
        out << ' ';
        accept(ast->statement);
    }
    out << "while";
    out << '(';
    accept(ast->expression);
    out << ')';
    out << ';';
    return false;
}

bool PrettyPrinter::visit(ElaboratedTypeSpecifierAST *ast)
{
    out << spell(ast->classkey_token);
    if (ast->name) {
        out << ' ';
        accept(ast->name);
    }
    return false;
}

bool PrettyPrinter::visit(EmptyDeclarationAST *)
{
    out << ';';
    return false;
}

bool PrettyPrinter::visit(EnumSpecifierAST *ast)
{
    out << "enum";
    if (ast->name) {
        out << ' ';
        accept(ast->name);
    }
    out << ' ';
    out << '{';
    if (ast->enumerators) {
        indent();
        newline();
        for (EnumeratorAST *it = ast->enumerators; it; it = it->next) {
            accept(it);
            if (it->next) {
                out << ", ";
                newline();
            }
        }
        deindent();
        newline();
    }
    out << '}';
    return false;
}

bool PrettyPrinter::visit(EnumeratorAST *ast)
{
    out << spell(ast->identifier_token);
    if (ast->equal_token) {
        out << ' ';
        out << '=';
        out << ' ';
        accept(ast->expression);
    }
    return false;
}

bool PrettyPrinter::visit(ExceptionDeclarationAST *ast)
{
    for (SpecifierAST *it = ast->type_specifier; it; it = it->next) {
        accept(it);
        if (it->next)
            out << ' ';
    }
    if (ast->declarator) {
        if (ast->type_specifier)
            out << ' ';
        accept(ast->declarator);
    }
    if (ast->dot_dot_dot_token)
        out << "...";
    return false;
}

bool PrettyPrinter::visit(ExceptionSpecificationAST *ast)
{
    out << "throw";
    out << '(';
    if (ast->dot_dot_dot_token)
        out << "...";
    else {
        for (ExpressionListAST *it = ast->type_ids; it; it = it->next) {
            accept(it->expression);
            if (it->next)
                out << ", ";
        }
    }
    out << ')';
    return false;
}

bool PrettyPrinter::visit(ExpressionListAST *ast)
{
    for (ExpressionListAST *it = ast; it; it = it->next) {
        accept(it->expression);
        if (it->next)
            out << ", ";
    }
    return false;
}

bool PrettyPrinter::visit(ExpressionOrDeclarationStatementAST *ast)
{
    accept(ast->declaration);
    return false;
}

bool PrettyPrinter::visit(ExpressionStatementAST *ast)
{
    accept(ast->expression);
    out << ';';
    return false;
}

bool PrettyPrinter::visit(ForStatementAST *ast)
{
    out << "for";
    out << ' ';
    out << '(';
    accept(ast->initializer);
    accept(ast->condition);
    out << ';';
    accept(ast->expression);
    out << ')';
    accept(ast->statement);
    return false;
}

bool PrettyPrinter::visit(FunctionDeclaratorAST *ast)
{
    out << '(';
    accept(ast->parameters);
    out << ')';
    for (SpecifierAST *it = ast->cv_qualifier_seq; it; it = it->next) {
        out << ' ';
        accept(it);
    }
    if (ast->exception_specification) {
        out << ' ';
        accept(ast->exception_specification);
    }
    return false;
}

bool PrettyPrinter::visit(FunctionDefinitionAST *ast)
{
    for (SpecifierAST *it = ast->decl_specifier_seq; it; it = it->next) {
        accept(it);
        if (it->next)
            out << ' ';
    }
    if (ast->declarator) {
        if (ast->decl_specifier_seq)
            out << ' ';
        accept(ast->declarator);
    }
    accept(ast->ctor_initializer);
    newline();
    accept(ast->function_body);
    if (ast->next)
        newline(); // one extra line after the function definiton.
    return false;
}

bool PrettyPrinter::visit(GotoStatementAST *ast)
{
    out << "goto";
    out << ' ';
    out << spell(ast->identifier_token);
    out << ';';
    return false;
}

bool PrettyPrinter::visit(IfStatementAST *ast)
{
    out << "if";
    out << ' ';
    out << '(';
    accept(ast->condition);
    out << ')';
    if (ast->statement->asCompoundStatement()) {
        out << ' ';
        accept(ast->statement);
    } else {
        indent();
        newline();
        accept(ast->statement);
        deindent();
        newline();
    }
    if (ast->else_token) {
        out << "else";
        out << ' ';
        accept(ast->else_statement);
    }
    return false;
}

bool PrettyPrinter::visit(LabeledStatementAST *ast)
{
    out << spell(ast->label_token);
    out << ':';
    accept(ast->statement);
    return false;
}

bool PrettyPrinter::visit(LinkageBodyAST *ast)
{
    out << '{';
    if (ast->declarations) {
        indent();
        newline();
        for (DeclarationAST *it = ast->declarations; it; it = it->next) {
            accept(it);
            if (it->next)
                newline();
        }
        deindent();
        newline();
    }
    out << '}';
    return false;
}

bool PrettyPrinter::visit(LinkageSpecificationAST *ast)
{
    out << "extern";
    out << ' ';
    if (ast->extern_type) {
        out << '"' << spell(ast->extern_type) << '"';
        out << ' ';
    }

    accept(ast->declaration);
    return false;
}

bool PrettyPrinter::visit(MemInitializerAST *ast)
{
    accept(ast->name);
    out << '(';
    accept(ast->expression);
    out << ')';
    return false;
}

bool PrettyPrinter::visit(MemberAccessAST *ast)
{
    out << spell(ast->access_token);
    if (ast->template_token) {
        out << "template";
        out << ' ';
    }
    accept(ast->member_name);
    return false;
}

bool PrettyPrinter::visit(NamedTypeSpecifierAST *ast)
{
    accept(ast->name);
    return false;
}

bool PrettyPrinter::visit(NamespaceAST *ast)
{
    out << "namespace";
    if (ast->identifier_token) {
        out << ' ';
        out << spell(ast->identifier_token);
    }
    for (SpecifierAST *it = ast->attributes; it; it = it->next) {
        out << ' ';
        accept(it);
    }
    out << ' ';
    accept(ast->linkage_body);
    return false;
}

bool PrettyPrinter::visit(NamespaceAliasDefinitionAST *ast)
{
    out << "namespace";
    out << ' ';
    out << spell(ast->namespace_name);
    out << ' ';
    out << '=';
    out << ' ';
    accept(ast->name);
    out << ';';
    return false;
}

bool PrettyPrinter::visit(NestedDeclaratorAST *ast)
{
    out << '(';
    accept(ast->declarator);
    out << ')';
    return false;
}

bool PrettyPrinter::visit(NestedExpressionAST *ast)
{
    out << '(';
    accept(ast->expression);
    out << ')';
    return false;
}

bool PrettyPrinter::visit(NestedNameSpecifierAST *ast)
{
    accept(ast->class_or_namespace_name);
    if (ast->scope_token)
        out << "::";
    return false;
}

bool PrettyPrinter::visit(NewDeclaratorAST *ast)
{
    for (PtrOperatorAST *it = ast->ptr_operators; it; it = it->next) {
        accept(it);
        if (it->next)
            out << ' ';
    }
    if (ast->declarator)
        accept(ast->declarator);
    return false;
}

bool PrettyPrinter::visit(NewExpressionAST *ast)
{
    if (ast->scope_token)
        out << "::";
    out << "new";
    out << ' ';
    if (ast->expression) {
        accept(ast->expression);
        if (ast->type_id)
            out << ' ';
    }
    if (ast->type_id) {
        accept(ast->type_id);
        if (ast->new_type_id)
            out << ' ';
    }
    if (ast->new_type_id) {
        accept(ast->new_type_id);
        if (ast->new_initializer)
            out << ' ';
    }
    accept(ast->new_initializer);
    return false;
}

bool PrettyPrinter::visit(NewInitializerAST *ast)
{
    out << '(';
    accept(ast->expression);
    out << ')';
    return false;
}

bool PrettyPrinter::visit(NewTypeIdAST *ast)
{
    for (SpecifierAST *it = ast->type_specifier; it; it = it->next) {
        accept(it);
        if (it->next)
            out << ' ';
    }
    if (ast->type_specifier)
        out << ' ';
    if (ast->new_initializer) {
        accept(ast->new_initializer);
        if (ast->new_declarator)
            out << ' ';
    }
    accept(ast->new_declarator);
    return false;
}

bool PrettyPrinter::visit(NumericLiteralAST *ast)
{
    switch (tokenKind(ast->token)) {
    case T_CHAR_LITERAL:
        out << '\'' << spell(ast->token) << '\'';
        break;
    case T_WIDE_CHAR_LITERAL:
        out << "L\'" << spell(ast->token) << '\'';
        break;

    default:
        out << spell(ast->token);
    }
    return false;
}

bool PrettyPrinter::visit(OperatorAST *ast)
{
    out << spell(ast->op_token);
    if (ast->open_token) {
        out << spell(ast->open_token);
        out << spell(ast->close_token);
    }
    return false;
}

bool PrettyPrinter::visit(OperatorFunctionIdAST *ast)
{
    out << "operator";
    out << ' ';
    accept(ast->op);
    return false;
}

bool PrettyPrinter::visit(ParameterDeclarationAST *ast)
{
    for (SpecifierAST *it = ast->type_specifier; it; it = it->next) {
        accept(it);
        if (it->next)
            out << ' ';
    }
    if (ast->declarator) {
        out << ' ';
        accept(ast->declarator);
    }
    if (ast->equal_token) {
        out << ' ';
        out << '=';
        out << ' ';
    }
    accept(ast->expression);
    return false;
}

bool PrettyPrinter::visit(ParameterDeclarationClauseAST *ast)
{
    for (DeclarationAST *it = ast->parameter_declarations; it; it = it->next) {
        accept(it);
        if (it->next)
            out << ", ";
    }
    return false;
}

bool PrettyPrinter::visit(PointerAST *ast)
{
    out << '*';
    for (SpecifierAST *it = ast->cv_qualifier_seq; it; it = it->next) {
        accept(it);
        if (it->next)
            out << ' ';
    }
    return false;
}

bool PrettyPrinter::visit(PointerToMemberAST *ast)
{
    if (ast->global_scope_token)
        out << "::";
    for (NestedNameSpecifierAST *it = ast->nested_name_specifier; it; it = it->next) {
        accept(it);
    }
    out << '*';
    for (SpecifierAST *it = ast->cv_qualifier_seq; it; it = it->next) {
        accept(it);
        if (it->next)
            out << ' ';
    }
    return false;
}

bool PrettyPrinter::visit(PostIncrDecrAST *ast)
{
    out << spell(ast->incr_decr_token);
    return false;
}

bool PrettyPrinter::visit(PostfixExpressionAST *ast)
{
    accept(ast->base_expression);
    for (PostfixAST *it = ast->postfix_expressions; it; it = it->next) {
        accept(it);
    }
    return false;
}

bool PrettyPrinter::visit(QualifiedNameAST *ast)
{
    if (ast->global_scope_token)
        out << "::";
    for (NestedNameSpecifierAST *it = ast->nested_name_specifier; it; it = it->next) {
        accept(it);
    }
    accept(ast->unqualified_name);
    return false;
}

bool PrettyPrinter::visit(ReferenceAST *)
{
    out << '&';
    return false;
}

bool PrettyPrinter::visit(ReturnStatementAST *ast)
{
    out << "return";
    if (ast->expression) {
        out << ' ';
        accept(ast->expression);
    }
    out << ';';
    return false;
}

bool PrettyPrinter::visit(SimpleDeclarationAST *ast)
{
    for (SpecifierAST *it = ast->decl_specifier_seq; it; it = it->next) {
        accept(it);
        if (it->next)
            out << ' ';
    }
    if (ast->declarators) {
        if (ast->decl_specifier_seq)
            out << ' ';

        for (DeclaratorListAST *it = ast->declarators; it; it = it->next) {
            accept(it->declarator);
            if (it->next)
                out << ", ";
        }
    }
    out << ';';
    return false;
}

bool PrettyPrinter::visit(SimpleNameAST *ast)
{
    out << spell(ast->identifier_token);
    return false;
}

bool PrettyPrinter::visit(SimpleSpecifierAST *ast)
{
    out << spell(ast->specifier_token);
    return false;
}

bool PrettyPrinter::visit(SizeofExpressionAST *ast)
{
    out << "sizeof";
    out << ' ';
    accept(ast->expression);
    return false;
}

bool PrettyPrinter::visit(StringLiteralAST *ast)
{
    for (StringLiteralAST *it = ast; it; it = it->next) {
        if (tokenKind(ast->token) == T_STRING_LITERAL)
            out << '"' << spell(ast->token) << '"';
        else
            out << "L\"" << spell(ast->token) << '"';
        if (it->next)
            out << ' ';
    }
    return false;
}

bool PrettyPrinter::visit(SwitchStatementAST *ast)
{
    out << "switch";
    out << ' ';
    out << '(';
    accept(ast->condition);
    out << ')';
    accept(ast->statement);
    return false;
}

bool PrettyPrinter::visit(TemplateArgumentListAST *ast)
{
    for (TemplateArgumentListAST *it = ast; it; it = it->next) {
        accept(it->template_argument);
        if (it->next)
            out << ", ";
    }
    return false;
}

bool PrettyPrinter::visit(TemplateDeclarationAST *ast)
{
    if (ast->export_token) {
        out << "export";
        out << ' ';
    }
    out << "template";
    out << ' ';
    out << '<';
    if (ast->template_parameters) {
        out << ' ';
        for (DeclarationAST *it = ast->template_parameters; it; it = it->next) {
            accept(it);
            if (it->next)
                out << ", ";
        }
        out << ' ';
    }
    out << '>';
    newline();
    accept(ast->declaration);
    return false;
}

bool PrettyPrinter::visit(TemplateIdAST *ast)
{
    out << spell(ast->identifier_token);
    out << '<';
    if (ast->template_arguments) {
        out << ' ';
        for (TemplateArgumentListAST *it = ast->template_arguments; it; it = it->next) {
            accept(it->template_argument);
            if (it->next)
                out << ", ";
        }
        out << ' ';
    }
    out << '>';
    return false;
}

bool PrettyPrinter::visit(TemplateTypeParameterAST *ast)
{
    out << "template";
    out << ' ';
    out << '<';
    if (ast->template_parameters) {
        out << ' ';
        for (DeclarationAST *it = ast->template_parameters; it; it = it->next) {
            accept(it);
            if (it->next)
                out << ", ";
        }
        out << ' ';
    }
    out << '>';
    out << ' ';
    out << "class";
    out << ' ';
    accept(ast->name);
    if (ast->equal_token) {
        out << ' ';
        out << '=';
        out << ' ';
        accept(ast->type_id);
    }
    return false;
}

bool PrettyPrinter::visit(ThisExpressionAST *)
{
    out << "this";
    return false;
}

bool PrettyPrinter::visit(ThrowExpressionAST *ast)
{
    out << "throw";
    out << ' ';
    accept(ast->expression);
    return false;
}

bool PrettyPrinter::visit(TranslationUnitAST *ast)
{
    for (DeclarationAST *it = ast->declarations; it; it = it->next) {
        accept(it);
        if (it->next)
            newline();
    }
    return false;
}

bool PrettyPrinter::visit(TryBlockStatementAST *ast)
{
    out << "try";
    out << ' ';
    accept(ast->statement);
    for (CatchClauseAST *it = ast->catch_clause_seq; it; it = it->next) {
        accept(it);
    }
    return false;
}

bool PrettyPrinter::visit(TypeConstructorCallAST *ast)
{
    for (SpecifierAST *it = ast->type_specifier; it; it = it->next) {
        accept(it);
        if (it->next)
            out << ' ';
    }
    out << '(';
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        accept(it->expression);
        if (it->next)
            out << ", ";
    }
    out << ')';
    return false;
}

bool PrettyPrinter::visit(TypeIdAST *ast)
{
    for (SpecifierAST *it = ast->type_specifier; it; it = it->next) {
        accept(it);
        if (it->next)
            out << ' ';
    }
    if (ast->type_specifier && ast->declarator) {
        out << ' ';
        accept(ast->declarator);
    }
    return false;
}

bool PrettyPrinter::visit(TypeidExpressionAST *ast)
{
    out << "typeid";
    out << '(';
    accept(ast->expression);
    out << ')';
    return false;
}

bool PrettyPrinter::visit(TypeofSpecifierAST *ast)
{
    out << "typeof";
    out << '(';
    accept(ast->expression);
    out << ')';
    return false;
}

bool PrettyPrinter::visit(TypenameCallExpressionAST *ast)
{
    out << "typename";
    out << ' ';
    accept(ast->name);
    out << '(';
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        accept(it->expression);
        if (it->next)
            out << ", ";
    }
    out << ')';
    return false;
}

bool PrettyPrinter::visit(TypenameTypeParameterAST *ast)
{
    out << spell(ast->classkey_token);
    if (ast->name) {
        out << ' ';
        accept(ast->name);
    }
    if (ast->equal_token) {
        out << ' ';
        out << '=';
        out << ' ';
        accept(ast->type_id);
    }
    return false;
}

bool PrettyPrinter::visit(UnaryExpressionAST *ast)
{
    out << spell(ast->unary_op_token);
    accept(ast->expression);
    return false;
}

bool PrettyPrinter::visit(UsingAST *ast)
{
    out << "using";
    out << ' ';
    if (ast->typename_token) {
        out << "typename";
        out << ' ';
    }
    accept(ast->name);
    out << ';';
    return false;
}

bool PrettyPrinter::visit(UsingDirectiveAST *ast)
{
    out << "using";
    out << ' ';
    out << "namespace";
    out << ' ';
    accept(ast->name);
    out << ';';
    return false;
}

bool PrettyPrinter::visit(WhileStatementAST *ast)
{
    out << "while";
    out << ' ';
    out << '(';
    accept(ast->condition);
    out << ')';
    out << ' ';
    if (ast->statement && ast->statement->asCompoundStatement())
        accept(ast->statement);
    else {
        indent();
        newline();
        accept(ast->statement);
        deindent();
        newline();
    }
    return false;
}

bool PrettyPrinter::visit(QtMethodAST *ast)
{
    out << ast->method_token;
    out << '(';
    accept(ast->declarator);
    out << ')';
    return false;
}

bool PrettyPrinter::visit(CompoundLiteralAST *ast)
{
    out << '(';
    accept(ast->type_id);
    out << ')';
    out << ' ';
    accept(ast->initializer);
    return false;
}
