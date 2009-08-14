/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "PrettyPrinter.h"
#include "AST.h"
#include "Token.h"

#include <iostream>
#include <string>
#include <sstream>
#include <cassert>

#include <QString>

CPLUSPLUS_USE_NAMESPACE

PrettyPrinter::PrettyPrinter(Control *control, std::ostream &out)
    : ASTVisitor(control),
      _out(out),
      _depth(0),
      _lastToken(0)
{ }

void PrettyPrinter::operator()(AST *ast, const QByteArray &contents)
{
    _contents = contents;
    accept(ast);

    if (_lastToken + 1 < tokenCount())
        outToken(_lastToken + 1);
}

void PrettyPrinter::indent()
{ ++_depth; }

void PrettyPrinter::deindent()
{ --_depth; }

void PrettyPrinter::newline()
{
    _out << '\n' << std::string(_depth * 4, ' ');
}

void PrettyPrinter::outToken(unsigned token)
{
    if (!token)
        return;

    const Token &t = tokenAt(token);
    const unsigned start = _lastToken ? tokenAt(_lastToken).end() : 0;
    const unsigned end = t.begin();
    _lastToken = token;

    std::ostringstream oss;

    // Preserve non-AST text
    QByteArray ba(_contents.constData() + start, end - start);
    oss << ba.constData();

    // Print the token itself
    QByteArray tt(_contents.constData() + t.begin(), t.f.length);
    oss << tt.constData();

    QString stuff = QString::fromUtf8(oss.str().c_str());
    QString indent = QString(_depth * 4, QLatin1Char(' '));

    int from = 0;
    int index = 0;
    while ((index = stuff.indexOf(QLatin1Char('\n'), from)) != -1) {
        from = index + 1;
        int firstNonWhitespace = from;

        while (firstNonWhitespace < stuff.length()) {
            const QChar c = stuff.at(firstNonWhitespace);
            if (c.isSpace() && c != QLatin1Char('\n'))
                ++firstNonWhitespace;
            else
                break;
        }

        if (firstNonWhitespace != from)
            stuff.replace(from, firstNonWhitespace - from, indent);
    }

    _out << stuff.toUtf8().constData();
}

bool PrettyPrinter::visit(AccessDeclarationAST *ast)
{
    deindent();
    outToken(ast->access_specifier_token);
    outToken(ast->slots_token);
    outToken(ast->colon_token);
    indent();
    return false;
}

bool PrettyPrinter::visit(ArrayAccessAST *ast)
{
    outToken(ast->lbracket_token);
    accept(ast->expression);
    outToken(ast->rbracket_token);
    return false;
}

bool PrettyPrinter::visit(ArrayDeclaratorAST *ast)
{
    outToken(ast->lbracket_token);
    accept(ast->expression);
    outToken(ast->rbracket_token);
    return false;
}

bool PrettyPrinter::visit(ArrayInitializerAST *ast)
{
    outToken(ast->lbrace_token);
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        outToken(it->comma_token);
        accept(it->expression);
    }
    outToken(ast->rbrace_token);
    return false;
}

bool PrettyPrinter::visit(AsmDefinitionAST *ast)
{
    outToken(ast->asm_token);
    outToken(ast->volatile_token);
    outToken(ast->lparen_token);
    /* ### implement me */
    outToken(ast->rparen_token);
    outToken(ast->semicolon_token);
    return false;
}

bool PrettyPrinter::visit(AttributeSpecifierAST *ast)
{
    outToken(ast->attribute_token);
    outToken(ast->first_lparen_token);
    outToken(ast->second_lparen_token);
    for (AttributeAST *it = ast->attributes; it; it = it->next) {
        outToken(it->comma_token);
        accept(it);
    }
    outToken(ast->first_rparen_token);
    outToken(ast->second_rparen_token);
    return false;
}

bool PrettyPrinter::visit(AttributeAST *ast)
{
    outToken(ast->identifier_token);
    if (ast->lparen_token) {
        outToken(ast->lparen_token);
        outToken(ast->tag_token);
        if (ast->expression_list) {
            for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
                outToken(ast->expression_list->comma_token);
                accept(it->expression);
            }
        }
        outToken(ast->rparen_token);
    }
    return false;
}

bool PrettyPrinter::visit(BaseSpecifierAST *ast)
{
    outToken(ast->virtual_token);
    outToken(ast->access_specifier_token);
    accept(ast->name);
    return false;
}

bool PrettyPrinter::visit(BinaryExpressionAST *ast)
{
    accept(ast->left_expression);
    outToken(ast->binary_op_token);
    accept(ast->right_expression);
    return false;
}

bool PrettyPrinter::visit(BoolLiteralAST *ast)
{
    outToken(ast->literal_token);
    return false;
}

bool PrettyPrinter::visit(BreakStatementAST *ast)
{
    outToken(ast->break_token);
    outToken(ast->semicolon_token);
    return false;
}

bool PrettyPrinter::visit(CallAST *ast)
{
    outToken(ast->lparen_token);
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        outToken(it->comma_token);
        accept(it->expression);
    }
    outToken(ast->rparen_token);
    return false;
}

bool PrettyPrinter::visit(CaseStatementAST *ast)
{
    outToken(ast->case_token);
    accept(ast->expression);
    outToken(ast->colon_token);
    if (! ast->statement)
        return false;

    if (ast->statement->asCompoundStatement()) {
        accept(ast->statement);
    } else if (ast->statement->asCaseStatement() || ast->statement->asLabeledStatement()) {
        accept(ast->statement);
    } else {
        indent();
        accept(ast->statement);
        deindent();
    }
    return false;
}

bool PrettyPrinter::visit(CastExpressionAST *ast)
{
    outToken(ast->lparen_token);
    accept(ast->type_id);
    outToken(ast->rparen_token);
    accept(ast->expression);
    return false;
}

bool PrettyPrinter::visit(CatchClauseAST *ast)
{
    outToken(ast->catch_token);
    outToken(ast->lparen_token);
    accept(ast->exception_declaration);
    outToken(ast->rparen_token);
    accept(ast->statement);
    return false;
}

bool PrettyPrinter::visit(ClassSpecifierAST *ast)
{
    outToken(ast->classkey_token);
    if (ast->attributes) {
        accept(ast->attributes);
    }
    accept(ast->name);
    if (ast->colon_token) {
        outToken(ast->colon_token);
        for (BaseSpecifierAST *it = ast->base_clause; it; it = it->next) {
            outToken(it->comma_token);
            accept(it);
        }
    }
    outToken(ast->lbrace_token);
    if (ast->member_specifiers) {
        indent();
        for (DeclarationListAST *it = ast->member_specifiers; it; it = it->next) {
            accept(it->declaration);
        }
        deindent();
    }
    outToken(ast->rbrace_token);
    return false;
}

bool PrettyPrinter::visit(CompoundStatementAST *ast)
{
    outToken(ast->lbrace_token);
    if (ast->statements) {
        indent();
        for (StatementListAST *it = ast->statements; it; it = it->next) {
            accept(it);
        }
        deindent();
    }
    outToken(ast->rbrace_token);
    return false;
}

bool PrettyPrinter::visit(ConditionAST *ast)
{
    for (SpecifierAST *it = ast->type_specifier; it; it = it->next) {
        accept(it);
    }
    if (ast->declarator) {
        accept(ast->declarator);
    }
    return false;
}

bool PrettyPrinter::visit(ConditionalExpressionAST *ast)
{
    accept(ast->condition);
    outToken(ast->question_token);
    accept(ast->left_expression);
    outToken(ast->colon_token);
    accept(ast->right_expression);
    return false;
}

bool PrettyPrinter::visit(ContinueStatementAST *ast)
{
    outToken(ast->continue_token);
    outToken(ast->semicolon_token);
    return false;
}

bool PrettyPrinter::visit(ConversionFunctionIdAST *ast)
{
    outToken(ast->operator_token);
    for (SpecifierAST *it = ast->type_specifier; it; it = it->next) {
        accept(it);
    }
    for (PtrOperatorAST *it = ast->ptr_operators; it; it = it->next) {
        accept(it);
    }
    return false;
}

bool PrettyPrinter::visit(CppCastExpressionAST *ast)
{
    outToken(ast->cast_token);
    outToken(ast->less_token);
    accept(ast->type_id);
    outToken(ast->greater_token);
    outToken(ast->lparen_token);
    accept(ast->expression);
    outToken(ast->rparen_token);
    return false;
}

bool PrettyPrinter::visit(CtorInitializerAST *ast)
{
    outToken(ast->colon_token);
    for (MemInitializerAST *it = ast->member_initializers; it; it = it->next) {
        outToken(it->comma_token);
        accept(it->name);
        outToken(it->lparen_token);
        accept(it->expression);
        outToken(it->rparen_token);
    }
    return false;
}

bool PrettyPrinter::visit(DeclaratorAST *ast)
{
    for (PtrOperatorAST *it = ast->ptr_operators; it; it = it->next) {
        accept(it);
    }
    if (ast->core_declarator) {
        accept(ast->core_declarator);
    }
    for (PostfixDeclaratorAST *it = ast->postfix_declarators; it; it = it->next) {
        accept(it);
    }
    for (SpecifierAST *it = ast->attributes; it; it = it->next) {
        accept(it);
    }
    if (ast->initializer) {
        outToken(ast->equals_token);
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
        outToken(ast->comma_token);
        accept(it->declarator);
    }
    return false;
}

bool PrettyPrinter::visit(DeleteExpressionAST *ast)
{
    outToken(ast->scope_token);
    outToken(ast->delete_token);
    if (ast->expression) {
        accept(ast->expression);
    }
    return false;
}

bool PrettyPrinter::visit(DestructorNameAST *ast)
{
    outToken(ast->tilde_token);
    outToken(ast->identifier_token);
    return false;
}

bool PrettyPrinter::visit(DoStatementAST *ast)
{
    outToken(ast->do_token);
    if (ast->statement) {
        accept(ast->statement);
    }
    outToken(ast->while_token);
    outToken(ast->lparen_token);
    accept(ast->expression);
    outToken(ast->rparen_token);
    outToken(ast->semicolon_token);
    return false;
}

bool PrettyPrinter::visit(ElaboratedTypeSpecifierAST *ast)
{
    outToken(ast->classkey_token);
    if (ast->name) {
        accept(ast->name);
    }
    return false;
}

bool PrettyPrinter::visit(EmptyDeclarationAST *ast)
{
    outToken(ast->semicolon_token);
    return false;
}

bool PrettyPrinter::visit(EnumSpecifierAST *ast)
{
    outToken(ast->enum_token);
    if (ast->name) {
        accept(ast->name);
    }
    outToken(ast->lbrace_token);
    if (ast->enumerators) {
        indent();
        for (EnumeratorAST *it = ast->enumerators; it; it = it->next) {
            outToken(it->comma_token);
            accept(it);
        }
        deindent();
    }
    outToken(ast->rbrace_token);
    return false;
}

bool PrettyPrinter::visit(EnumeratorAST *ast)
{
    outToken(ast->identifier_token);
    if (ast->equal_token) {
        outToken(ast->equal_token);
        accept(ast->expression);
    }
    return false;
}

bool PrettyPrinter::visit(ExceptionDeclarationAST *ast)
{
    for (SpecifierAST *it = ast->type_specifier; it; it = it->next) {
        accept(it);
    }
    if (ast->declarator) {
        accept(ast->declarator);
    }
    outToken(ast->dot_dot_dot_token);
    return false;
}

bool PrettyPrinter::visit(ExceptionSpecificationAST *ast)
{
    outToken(ast->throw_token);
    outToken(ast->lparen_token);
    if (ast->dot_dot_dot_token)
        outToken(ast->dot_dot_dot_token);
    else {
        for (ExpressionListAST *it = ast->type_ids; it; it = it->next) {
            outToken(it->comma_token);
            accept(it->expression);
        }
    }
    outToken(ast->rparen_token);
    return false;
}

bool PrettyPrinter::visit(ExpressionListAST *ast)
{
    for (ExpressionListAST *it = ast; it; it = it->next) {
        outToken(it->comma_token);
        accept(it->expression);
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
    outToken(ast->semicolon_token);
    return false;
}

bool PrettyPrinter::visit(ForStatementAST *ast)
{
    outToken(ast->for_token);
    outToken(ast->lparen_token);
    accept(ast->initializer);
    accept(ast->condition);
    outToken(ast->semicolon_token);
    accept(ast->expression);
    outToken(ast->rparen_token);
    accept(ast->statement);
    return false;
}

bool PrettyPrinter::visit(FunctionDeclaratorAST *ast)
{
    outToken(ast->lparen_token);
    accept(ast->parameters);
    outToken(ast->rparen_token);
    for (SpecifierAST *it = ast->cv_qualifier_seq; it; it = it->next) {
        accept(it);
    }
    if (ast->exception_specification) {
        accept(ast->exception_specification);
    }
    return false;
}

bool PrettyPrinter::visit(FunctionDefinitionAST *ast)
{
    for (SpecifierAST *it = ast->decl_specifier_seq; it; it = it->next) {
        accept(it);
    }
    if (ast->declarator) {
        accept(ast->declarator);
    }
    accept(ast->ctor_initializer);
    accept(ast->function_body);
    return false;
}

bool PrettyPrinter::visit(GotoStatementAST *ast)
{
    outToken(ast->goto_token);
    outToken(ast->identifier_token);
    outToken(ast->semicolon_token);
    return false;
}

bool PrettyPrinter::visit(IfStatementAST *ast)
{
    outToken(ast->if_token);
    outToken(ast->lparen_token);
    accept(ast->condition);
    outToken(ast->rparen_token);
    if (ast->statement->asCompoundStatement()) {
        accept(ast->statement);
    } else {
        indent();
        accept(ast->statement);
        deindent();
    }
    if (ast->else_token) {
        outToken(ast->else_token);
        accept(ast->else_statement);
    }
    return false;
}

bool PrettyPrinter::visit(LabeledStatementAST *ast)
{
    outToken(ast->label_token);
    outToken(ast->colon_token);
    accept(ast->statement);
    return false;
}

bool PrettyPrinter::visit(LinkageBodyAST *ast)
{
    outToken(ast->lbrace_token);
    if (ast->declarations) {
        indent();
        for (DeclarationListAST *it = ast->declarations; it; it = it->next) {
            accept(it->declaration);
        }
        deindent();
    }
    outToken(ast->rbrace_token);
    return false;
}

bool PrettyPrinter::visit(LinkageSpecificationAST *ast)
{
    outToken(ast->extern_token);
    if (ast->extern_type_token) {
        outToken(ast->extern_type_token);
    }

    accept(ast->declaration);
    return false;
}

bool PrettyPrinter::visit(MemInitializerAST *ast)
{
    accept(ast->name);
    outToken(ast->lparen_token);
    accept(ast->expression);
    outToken(ast->rparen_token);
    return false;
}

bool PrettyPrinter::visit(MemberAccessAST *ast)
{
    outToken(ast->access_token);
    outToken(ast->template_token);
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
    outToken(ast->namespace_token);
    outToken(ast->identifier_token);
    for (SpecifierAST *it = ast->attributes; it; it = it->next) {
        accept(it);
    }
    accept(ast->linkage_body);
    return false;
}

bool PrettyPrinter::visit(NamespaceAliasDefinitionAST *ast)
{
    outToken(ast->namespace_token);
    outToken(ast->namespace_name_token);
    outToken(ast->equal_token);
    accept(ast->name);
    outToken(ast->semicolon_token);
    return false;
}

bool PrettyPrinter::visit(NestedDeclaratorAST *ast)
{
    outToken(ast->lparen_token);
    accept(ast->declarator);
    outToken(ast->rparen_token);
    return false;
}

bool PrettyPrinter::visit(NestedExpressionAST *ast)
{
    outToken(ast->lparen_token);
    accept(ast->expression);
    outToken(ast->rparen_token);
    return false;
}

bool PrettyPrinter::visit(NestedNameSpecifierAST *ast)
{
    accept(ast->class_or_namespace_name);
    outToken(ast->scope_token);
    return false;
}

bool PrettyPrinter::visit(NewArrayDeclaratorAST *ast)
{
    outToken(ast->lbracket_token);
    accept(ast->expression);
    outToken(ast->rbracket_token);
    return false;
}

bool PrettyPrinter::visit(NewExpressionAST *ast)
{
    outToken(ast->scope_token);
    outToken(ast->new_token);
    accept(ast->new_placement);
    if (ast->lparen_token) {
        outToken(ast->lparen_token);
        accept(ast->type_id);
        outToken(ast->rparen_token);
    } else {
        accept(ast->new_type_id);
    }
    accept(ast->new_initializer);
    return false;
}

bool PrettyPrinter::visit(NewPlacementAST *ast)
{
    outToken(ast->lparen_token);
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        outToken(it->comma_token);
        accept(it->expression);
    }
    outToken(ast->rparen_token);
    return false;
}

bool PrettyPrinter::visit(NewInitializerAST *ast)
{
    outToken(ast->lparen_token);
    accept(ast->expression);
    outToken(ast->rparen_token);
    return false;
}

bool PrettyPrinter::visit(NewTypeIdAST *ast)
{
    for (SpecifierAST *it = ast->type_specifier; it; it = it->next) {
        accept(it);
    }
    for (PtrOperatorAST *it = ast->ptr_operators; it; it = it->next) {
        accept(it);
    }
    for (NewArrayDeclaratorAST *it = ast->new_array_declarators; it; it = it->next) {
        accept(it);
    }
    return false;
}

bool PrettyPrinter::visit(NumericLiteralAST *ast)
{
    outToken(ast->literal_token);
    return false;
}

bool PrettyPrinter::visit(OperatorAST *ast)
{
    outToken(ast->op_token);
    if (ast->open_token) {
        outToken(ast->open_token);
        outToken(ast->close_token);
    }
    return false;
}

bool PrettyPrinter::visit(OperatorFunctionIdAST *ast)
{
    outToken(ast->operator_token);
    accept(ast->op);
    return false;
}

bool PrettyPrinter::visit(ParameterDeclarationAST *ast)
{
    for (SpecifierAST *it = ast->type_specifier; it; it = it->next) {
        accept(it);
    }
    if (ast->declarator) {
        accept(ast->declarator);
    }
    outToken(ast->equal_token);
    accept(ast->expression);
    return false;
}

bool PrettyPrinter::visit(ParameterDeclarationClauseAST *ast)
{
    for (DeclarationListAST *it = ast->parameter_declarations; it; it = it->next) {
        // XXX handle the comma tokens correctly
        accept(it->declaration);
    }
    return false;
}

bool PrettyPrinter::visit(PointerAST *ast)
{
    outToken(ast->star_token);
    for (SpecifierAST *it = ast->cv_qualifier_seq; it; it = it->next) {
        accept(it);
    }
    return false;
}

bool PrettyPrinter::visit(PointerToMemberAST *ast)
{
    outToken(ast->global_scope_token);
    for (NestedNameSpecifierAST *it = ast->nested_name_specifier; it; it = it->next) {
        accept(it);
    }
    outToken(ast->star_token);
    for (SpecifierAST *it = ast->cv_qualifier_seq; it; it = it->next) {
        accept(it);
    }
    return false;
}

bool PrettyPrinter::visit(PostIncrDecrAST *ast)
{
    outToken(ast->incr_decr_token);
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
    outToken(ast->global_scope_token);
    for (NestedNameSpecifierAST *it = ast->nested_name_specifier; it; it = it->next) {
        accept(it);
    }
    accept(ast->unqualified_name);
    return false;
}

bool PrettyPrinter::visit(ReferenceAST *ast)
{
    outToken(ast->amp_token);
    return false;
}

bool PrettyPrinter::visit(ReturnStatementAST *ast)
{
    outToken(ast->return_token);
    if (ast->expression) {
        accept(ast->expression);
    }
    outToken(ast->semicolon_token);
    return false;
}

bool PrettyPrinter::visit(SimpleDeclarationAST *ast)
{
    for (SpecifierAST *it = ast->decl_specifier_seq; it; it = it->next) {
        accept(it);
    }
    if (ast->declarators) {
        for (DeclaratorListAST *it = ast->declarators; it; it = it->next) {
            outToken(it->comma_token);
            accept(it->declarator);
        }
    }
    outToken(ast->semicolon_token);
    return false;
}

bool PrettyPrinter::visit(SimpleNameAST *ast)
{
    outToken(ast->identifier_token);
    return false;
}

bool PrettyPrinter::visit(SimpleSpecifierAST *ast)
{
    outToken(ast->specifier_token);
    return false;
}

bool PrettyPrinter::visit(SizeofExpressionAST *ast)
{
    outToken(ast->sizeof_token);
    accept(ast->expression);
    return false;
}

bool PrettyPrinter::visit(StringLiteralAST *ast)
{
    for (StringLiteralAST *it = ast; it; it = it->next) {
        outToken(it->literal_token);
    }
    return false;
}

bool PrettyPrinter::visit(SwitchStatementAST *ast)
{
    outToken(ast->switch_token);
    outToken(ast->lparen_token);
    accept(ast->condition);
    outToken(ast->rparen_token);
    accept(ast->statement);
    return false;
}

bool PrettyPrinter::visit(TemplateArgumentListAST *ast)
{
    for (TemplateArgumentListAST *it = ast; it; it = it->next) {
        outToken(it->comma_token);
        accept(it->template_argument);
    }
    return false;
}

bool PrettyPrinter::visit(TemplateDeclarationAST *ast)
{
    outToken(ast->export_token);
    outToken(ast->template_token);
    outToken(ast->less_token);
    if (ast->template_parameters) {
        for (DeclarationListAST *it = ast->template_parameters; it; it = it->next) {
            // XXX handle the comma tokens correctly
            accept(it->declaration);
        }
    }
    outToken(ast->greater_token);
    accept(ast->declaration);
    return false;
}

bool PrettyPrinter::visit(TemplateIdAST *ast)
{
    outToken(ast->identifier_token);
    outToken(ast->less_token);
    if (ast->template_arguments) {
        for (TemplateArgumentListAST *it = ast->template_arguments; it; it = it->next) {
            outToken(it->comma_token);
            accept(it->template_argument);
        }
    }
    outToken(ast->greater_token);
    return false;
}

bool PrettyPrinter::visit(TemplateTypeParameterAST *ast)
{
    outToken(ast->template_token);
    outToken(ast->less_token);
    if (ast->template_parameters) {
        for (DeclarationListAST *it = ast->template_parameters; it; it = it->next) {
            // XXX handle the comma tokens correctly
            accept(it->declaration);
        }
    }
    outToken(ast->greater_token);
    outToken(ast->class_token);
    accept(ast->name);
    if (ast->equal_token) {
        outToken(ast->equal_token);
        accept(ast->type_id);
    }
    return false;
}

bool PrettyPrinter::visit(ThisExpressionAST *ast)
{
    outToken(ast->this_token);
    return false;
}

bool PrettyPrinter::visit(ThrowExpressionAST *ast)
{
    outToken(ast->throw_token);
    accept(ast->expression);
    return false;
}

bool PrettyPrinter::visit(TranslationUnitAST *ast)
{
    for (DeclarationListAST *it = ast->declarations; it; it = it->next) {
        accept(it->declaration);
    }
    return false;
}

bool PrettyPrinter::visit(TryBlockStatementAST *ast)
{
    outToken(ast->try_token);
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
    }
    outToken(ast->lparen_token);
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        outToken(it->comma_token);
        accept(it->expression);
    }
    outToken(ast->rparen_token);
    return false;
}

bool PrettyPrinter::visit(TypeIdAST *ast)
{
    for (SpecifierAST *it = ast->type_specifier; it; it = it->next) {
        accept(it);
    }
    if (ast->type_specifier && ast->declarator) {
        accept(ast->declarator);
    }
    return false;
}

bool PrettyPrinter::visit(TypeidExpressionAST *ast)
{
    outToken(ast->typeid_token);
    outToken(ast->lparen_token);
    accept(ast->expression);
    outToken(ast->rparen_token);
    return false;
}

bool PrettyPrinter::visit(TypeofSpecifierAST *ast)
{
    outToken(ast->typeof_token);
    outToken(ast->lparen_token);
    accept(ast->expression);
    outToken(ast->rparen_token);
    return false;
}

bool PrettyPrinter::visit(TypenameCallExpressionAST *ast)
{
    outToken(ast->typename_token);
    accept(ast->name);
    outToken(ast->lparen_token);
    for (ExpressionListAST *it = ast->expression_list; it; it = it->next) {
        outToken(it->comma_token);
        accept(it->expression);
    }
    outToken(ast->rparen_token);
    return false;
}

bool PrettyPrinter::visit(TypenameTypeParameterAST *ast)
{
    outToken(ast->classkey_token);
    if (ast->name) {
        accept(ast->name);
    }
    if (ast->equal_token) {
        outToken(ast->equal_token);
        accept(ast->type_id);
    }
    return false;
}

bool PrettyPrinter::visit(UnaryExpressionAST *ast)
{
    outToken(ast->unary_op_token);
    accept(ast->expression);
    return false;
}

bool PrettyPrinter::visit(UsingAST *ast)
{
    outToken(ast->using_token);
    outToken(ast->typename_token);
    accept(ast->name);
    outToken(ast->semicolon_token);
    return false;
}

bool PrettyPrinter::visit(UsingDirectiveAST *ast)
{
    outToken(ast->using_token);
    outToken(ast->namespace_token);
    accept(ast->name);
    outToken(ast->semicolon_token);
    return false;
}

bool PrettyPrinter::visit(WhileStatementAST *ast)
{
    outToken(ast->while_token);
    outToken(ast->lparen_token);
    accept(ast->condition);
    outToken(ast->rparen_token);
    if (ast->statement && ast->statement->asCompoundStatement())
        accept(ast->statement);
    else {
        indent();
        accept(ast->statement);
        deindent();
    }
    return false;
}

bool PrettyPrinter::visit(QtMethodAST *ast)
{
    outToken(ast->method_token);
    outToken(ast->lparen_token);
    accept(ast->declarator);
    outToken(ast->rparen_token);
    return false;
}

bool PrettyPrinter::visit(CompoundLiteralAST *ast)
{
    outToken(ast->lparen_token);
    accept(ast->type_id);
    outToken(ast->rparen_token);
    accept(ast->initializer);
    return false;
}
