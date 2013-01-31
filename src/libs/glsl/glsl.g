-----------------------------------------------------------------------------
--
-- Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
-- Contact: http://www.qt-project.org/legal
--
-- This file is part of Qt Creator.
--
-- Commercial License Usage
-- Licensees holding valid commercial Qt licenses may use this file in
-- accordance with the commercial license agreement provided with the
-- Software or, alternatively, in accordance with the terms contained in
-- a written agreement between you and Digia.  For licensing terms and
-- conditions see http://qt.digia.com/licensing.  For further information
-- use the contact form at http://qt.digia.com/contact-us.
--
-- GNU Lesser General Public License Usage
-- Alternatively, this file may be used under the terms of the GNU Lesser
-- General Public License version 2.1 as published by the Free Software
-- Foundation and appearing in the file LICENSE.LGPL included in the
-- packaging of this file.  Please review the following information to
-- ensure the GNU Lesser General Public License version 2.1 requirements
-- will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
--
-- In addition, as a special exception, Digia gives you certain additional
-- rights.  These rights are described in the Digia Qt LGPL Exception
-- version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
--
-----------------------------------------------------------------------------

%decl glslparser.h
%impl glslparser.cpp
%parser GLSLParserTable
%token_prefix T_
%expect 1

%token FEED_GLSL "feed GLSL"
%token FEED_EXPRESSION "feed expression"

%token ADD_ASSIGN "+="
%token AMPERSAND "&"
%token AND_ASSIGN "&="
%token AND_OP "&&"
%token ATTRIBUTE "attribute"
%token BANG "!"
%token BOOL "bool"
%token BREAK "break"
%token BVEC2 "bvec2"
%token BVEC3 "bvec3"
%token BVEC4 "bvec4"
%token CARET "^"
%token CASE "case"
%token CENTROID "centroid"
%token COLON ":"
%token COMMA ","
%token CONST "const"
%token CONTINUE "continue"
%token DASH "-"
%token DEC_OP "--"
%token DEFAULT "default"
%token DISCARD "discard"
%token DIV_ASSIGN "/="
%token DMAT2 "dmat2"
%token DMAT2X2 "dmat2x2"
%token DMAT2X3 "dmat2x3"
%token DMAT2X4 "dmat2x4"
%token DMAT3 "dmat3"
%token DMAT3X2 "dmat3x2"
%token DMAT3X3 "dmat3x3"
%token DMAT3X4 "dmat3x4"
%token DMAT4 "dmat4"
%token DMAT4X2 "dmat4x2"
%token DMAT4X3 "dmat4x3"
%token DMAT4X4 "dmat4x4"
%token DO "do"
%token DOT "."
%token DOUBLE "double"
%token DVEC2 "dvec2"
%token DVEC3 "dvec3"
%token DVEC4 "dvec4"
%token ELSE "else"
%token EQUAL "="
%token EQ_OP "=="
%token FLAT "flat"
%token FLOAT "float"
%token FOR "for"
%token GE_OP ">="
%token HIGHP "highp"
%token IDENTIFIER "identifier"
%token IF "if"
%token IN "in"
%token INC_OP "++"
%token INOUT "inout"
%token INT "int"
%token INVARIANT "invariant"
%token ISAMPLER1D "isampler1D"
%token ISAMPLER1DARRAY "isampler1DArray"
%token ISAMPLER2D "isampler2D"
%token ISAMPLER2DARRAY "isampler2DArray"
%token ISAMPLER2DMS "isampler2DMS"
%token ISAMPLER2DMSARRAY "isampler2DMSArray"
%token ISAMPLER2DRECT "isampler2DRect"
%token ISAMPLER3D "isampler3D"
%token ISAMPLERBUFFER "isamplerBuffer"
%token ISAMPLERCUBE "isamplerCube"
%token ISAMPLERCUBEARRAY "isamplerCubeArray"
%token IVEC2 "ivec2"
%token IVEC3 "ivec3"
%token IVEC4 "ivec4"
%token LAYOUT "layout"
%token LEFT_ANGLE "<"
%token LEFT_ASSIGN "<<="
%token LEFT_BRACE "{"
%token LEFT_BRACKET "["
%token LEFT_OP "<<"
%token LEFT_PAREN "("
%token LE_OP "<="
%token LOWP "lowp"
%token MAT2 "mat2"
%token MAT2X2 "mat2x2"
%token MAT2X3 "mat2x3"
%token MAT2X4 "mat2x4"
%token MAT3 "mat3"
%token MAT3X2 "mat3x2"
%token MAT3X3 "mat3x3"
%token MAT3X4 "mat3x4"
%token MAT4 "mat4"
%token MAT4X2 "mat4x2"
%token MAT4X3 "mat4x3"
%token MAT4X4 "mat4x4"
%token MEDIUMP "mediump"
%token MOD_ASSIGN "%="
%token MUL_ASSIGN "*="
%token NE_OP "!="
%token NOPERSPECTIVE "noperspective"
%token NUMBER "number constant"
%token OR_ASSIGN "|="
%token OR_OP "||"
%token OUT "out"
%token PATCH "patch"
%token PERCENT "%"
%token PLUS "plus"
%token PRECISION "precision"
%token QUESTION "?"
%token RETURN "return"
%token RIGHT_ANGLE ">"
%token RIGHT_ASSIGN ">>="
%token RIGHT_BRACE "}"
%token RIGHT_BRACKET "]"
%token RIGHT_OP ">>"
%token RIGHT_PAREN ")"
%token SAMPLE "sample"
%token SAMPLER1D "sampler1D"
%token SAMPLER1DARRAY "sampler1DArray"
%token SAMPLER1DARRAYSHADOW "sampler1DArrayShadow"
%token SAMPLER1DSHADOW "sampler1DShadow"
%token SAMPLER2D "sampler2D"
%token SAMPLER2DARRAY "sampler2DArray"
%token SAMPLER2DARRAYSHADOW "sampler2DArrayShadow"
%token SAMPLER2DMS "sampler2DMS"
%token SAMPLER2DMSARRAY "sampler2DMSArray"
%token SAMPLER2DRECT "sampler2DRect"
%token SAMPLER2DRECTSHADOW "sampler2DRectShadow"
%token SAMPLER2DSHADOW "sampler2DShadow"
%token SAMPLER3D "sampler3D"
%token SAMPLERBUFFER "samplerBuffer"
%token SAMPLERCUBE "samplerCube"
%token SAMPLERCUBEARRAY "samplerCubeArray"
%token SAMPLERCUBEARRAYSHADOW "samplerCubeArrayShadow"
%token SAMPLERCUBESHADOW "samplerCubeShadow"
%token SEMICOLON ";"
%token SLASH "/"
%token SMOOTH "smooth"
%token STAR "*"
%token STRUCT "struct"
%token SUBROUTINE "subroutine"
%token SUB_ASSIGN "-="
%token SWITCH "switch"
%token TILDE "~"
%token TYPE_NAME "type_name"
%token UINT "uint"
%token UNIFORM "uniform"
%token USAMPLER1D "usampler1D"
%token USAMPLER1DARRAY "usampler1DArray"
%token USAMPLER2D "usampler2D"
%token USAMPLER2DARRAY "usampler2DArray"
%token USAMPLER2DMS "usampler2DMS"
%token USAMPLER2DMSARRAY "usampler2DMSarray"
%token USAMPLER2DRECT "usampler2DRect"
%token USAMPLER3D "usampler3D"
%token USAMPLERBUFFER "usamplerBuffer"
%token USAMPLERCUBE "usamplerCube"
%token USAMPLERCUBEARRAY "usamplerCubeArray"
%token UVEC2 "uvec2"
%token UVEC3 "uvec3"
%token UVEC4 "uvec4"
%token VARYING "varying"
%token VEC2 "vec2"
%token VEC3 "vec3"
%token VEC4 "vec4"
%token VERTICAL_BAR "|"
%token VOID "void"
%token WHILE "while"
%token XOR_ASSIGN "^="
%token XOR_OP "^^"
%token TRUE "true"
%token FALSE "false"
%token PREPROC "preprocessor directive"
%token COMMENT "comment"
%token ERROR "error"
%token RESERVED "reserved word"

%start toplevel

/:
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

#include "$header"
#include "glsllexer.h"
#include "glslast.h"
#include "glslengine.h"
#include <vector>
#include <stack>

namespace GLSL {

class GLSL_EXPORT Parser: public $table
{
public:
    union Value {
        void *ptr;
        const QString *string;
        AST *ast;
        List<AST *> *ast_list;
        DeclarationAST *declaration;
        List<DeclarationAST *> *declaration_list;
        ExpressionAST *expression;
        List<ExpressionAST *> *expression_list;
        StatementAST *statement;
        List<StatementAST *> *statement_list;
        TypeAST *type;
        StructTypeAST::Field *field;
        List<StructTypeAST::Field *> *field_list;
        TranslationUnitAST *translation_unit;
        FunctionIdentifierAST *function_identifier;
        AST::Kind kind;
        TypeAST::Precision precision;
        struct {
            StatementAST *thenClause;
            StatementAST *elseClause;
        } ifstmt;
        struct {
            ExpressionAST *condition;
            ExpressionAST *increment;
        } forstmt;
        struct {
            FunctionIdentifierAST *id;
            List<ExpressionAST *> *arguments;
        } function;
        int qualifier;
        LayoutQualifierAST *layout;
        List<LayoutQualifierAST *> *layout_list;
        struct {
            int qualifier;
            List<LayoutQualifierAST *> *layout_list;
        } type_qualifier;
        struct {
            TypeAST *type;
            const QString *name;
        } param_declarator;
        ParameterDeclarationAST *param_declaration;
        FunctionDeclarationAST *function_declaration;
    };

    Parser(Engine *engine, const char *source, unsigned size, int variant);
    ~Parser();

    TranslationUnitAST *parse() {
        if (AST *u = parse(T_FEED_GLSL))
            return u->asTranslationUnit();
        return 0;
    }

    ExpressionAST *parseExpression() {
        if (AST *u = parse(T_FEED_EXPRESSION))
            return u->asExpression();
        return 0;
    }

    AST *parse(int startToken);

private:
    // 1-based
    int &location(int n) { return _locationStack[_tos + n - 1]; }
    Value &sym(int n) { return _symStack[_tos + n - 1]; }
    AST *&ast(int n) { return _symStack[_tos + n - 1].ast; }
    const QString *&string(int n) { return _symStack[_tos + n - 1].string; }
    ExpressionAST *&expression(int n) { return _symStack[_tos + n - 1].expression; }
    StatementAST *&statement(int n) { return _symStack[_tos + n - 1].statement; }
    TypeAST *&type(int n) { return _symStack[_tos + n - 1].type; }
    FunctionDeclarationAST *&function(int n) { return _symStack[_tos + n - 1].function_declaration; }

    inline int consumeToken() {
        if (_index < int(_tokens.size()))
            return _index++;
        return _tokens.size() - 1;
    }
    inline const Token &tokenAt(int index) const {
        if (index == 0)
            return _startToken;
        return _tokens.at(index);
    }
    inline int tokenKind(int index) const {
        if (index == 0)
            return _startToken.kind;
        return _tokens.at(index).kind;
    }
    void reduce(int ruleno);

    void warning(int line, const QString &message)
    {
        _engine->warning(line, message);
    }

    void error(int line, const QString &message)
    {
        _engine->error(line, message);
    }

    template <typename T>
    T *makeAstNode()
    {
        T *node = new (_engine->pool()) T ();
        node->lineno = yyloc >= 0 ? (_tokens[yyloc].line + 1) : 0;
        return node;
    }

    template <typename T, typename A1>
    T *makeAstNode(A1 a1)
    {
        T *node = new (_engine->pool()) T (a1);
        node->lineno = yyloc >= 0 ? (_tokens[yyloc].line + 1) : 0;
        return node;
    }

    template <typename T, typename A1, typename A2>
    T *makeAstNode(A1 a1, A2 a2)
    {
        T *node = new (_engine->pool()) T (a1, a2);
        node->lineno = yyloc >= 0 ? (_tokens[yyloc].line + 1) : 0;
        return node;
    }

    template <typename T, typename A1, typename A2, typename A3>
    T *makeAstNode(A1 a1, A2 a2, A3 a3)
    {
        T *node = new (_engine->pool()) T (a1, a2, a3);
        node->lineno = yyloc >= 0 ? (_tokens[yyloc].line + 1) : 0;
        return node;
    }

    template <typename T, typename A1, typename A2, typename A3, typename A4>
    T *makeAstNode(A1 a1, A2 a2, A3 a3, A4 a4)
    {
        T *node = new (_engine->pool()) T (a1, a2, a3, a4);
        node->lineno = yyloc >= 0 ? (_tokens[yyloc].line + 1) : 0;
        return node;
    }

    TypeAST *makeBasicType(int token)
    {
        TypeAST *type = new (_engine->pool()) BasicTypeAST(token, spell[token]);
        type->lineno = yyloc >= 0 ? (_tokens[yyloc].line + 1) : 0;
        return type;
    }

private:
    Engine *_engine;
    int _tos;
    int _index;
    int yyloc;
    int yytoken;
    int yyrecovering;
    bool _recovered;
    Token _startToken;
    std::vector<int> _stateStack;
    std::vector<int> _locationStack;
    std::vector<Value> _symStack;
    std::vector<Token> _tokens;
};

} // namespace GLSL
:/

/.
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

#include "glslparser.h"
#include "glslengine.h"
#include <iostream>
#include <cstdio>
#include <cassert>
#include <QDebug>

using namespace GLSL;

Parser::Parser(Engine *engine, const char *source, unsigned size, int variant)
    : _engine(engine), _tos(-1), _index(0), yyloc(-1), yytoken(-1), yyrecovering(0), _recovered(false)
{
    _tokens.reserve(1024);

    _stateStack.resize(128);
    _locationStack.resize(128);
    _symStack.resize(128);

    _tokens.push_back(Token()); // invalid token

    std::stack<int> parenStack;
    std::stack<int> bracketStack;
    std::stack<int> braceStack;

    Lexer lexer(engine, source, size);
    lexer.setVariant(variant);
    Token tk;
    do {
        lexer.yylex(&tk);

        switch (tk.kind) {
        case T_LEFT_PAREN:
            parenStack.push(_tokens.size());
            break;
        case T_LEFT_BRACKET:
            bracketStack.push(_tokens.size());
            break;
        case T_LEFT_BRACE:
            braceStack.push(_tokens.size());
            break;

        case T_RIGHT_PAREN:
            if (! parenStack.empty()) {
                _tokens[parenStack.top()].matchingBrace = _tokens.size();
                parenStack.pop();
            }
            break;
        case T_RIGHT_BRACKET:
            if (! bracketStack.empty()) {
                _tokens[bracketStack.top()].matchingBrace = _tokens.size();
                bracketStack.pop();
            }
            break;
        case T_RIGHT_BRACE:
            if (! braceStack.empty()) {
                _tokens[braceStack.top()].matchingBrace = _tokens.size();
                braceStack.pop();
            }
            break;
        default:
            break;
        }

        _tokens.push_back(tk);
    } while (tk.isNot(EOF_SYMBOL));

    _index = 0;
}

Parser::~Parser()
{
}

AST *Parser::parse(int startToken)
{
    int action = 0;
    yytoken = -1;
    yyloc = -1;
    void *yyval = 0; // value of the current token.

    _recovered = false;
    _tos = -1;
    _startToken.kind = startToken;

    do {
    again:
        if (unsigned(++_tos) == _stateStack.size()) {
            _stateStack.resize(_tos * 2);
            _locationStack.resize(_tos * 2);
            _symStack.resize(_tos * 2);
        }

        _stateStack[_tos] = action;

        if (yytoken == -1 && -TERMINAL_COUNT != action_index[action]) {
            yyloc = consumeToken();
            yytoken = tokenKind(yyloc);
            if (yyrecovering)
                --yyrecovering;
            if (yytoken == T_IDENTIFIER && t_action(action, T_TYPE_NAME) != 0) {
                const Token &la = tokenAt(_index);

                if (la.is(T_IDENTIFIER)) {
                    yytoken = T_TYPE_NAME;
                } else if (la.is(T_LEFT_BRACKET) && la.matchingBrace != 0 &&
                           tokenAt(la.matchingBrace + 1).is(T_IDENTIFIER)) {
                    yytoken = T_TYPE_NAME;
                }
            }
            yyval = _tokens.at(yyloc).ptr;
        }

        action = t_action(action, yytoken);
        if (action > 0) {
            if (action == ACCEPT_STATE) {
                --_tos;
                return _symStack[0].translation_unit;
            }
            _symStack[_tos].ptr = yyval;
            _locationStack[_tos] = yyloc;
            yytoken = -1;
        } else if (action < 0) {
            const int ruleno = -action - 1;
            const int N = rhs[ruleno];
            _tos -= N;
            reduce(ruleno);
            action = nt_action(_stateStack[_tos], lhs[ruleno] - TERMINAL_COUNT);
        } else if (action == 0) {
            const int line = _tokens[yyloc].line + 1;
            QString message = QLatin1String("Syntax error");
            if (yytoken != -1) {
                const QLatin1String s(spell[yytoken]);
                message = QString::fromLatin1("Unexpected token `%1'").arg(s);
            }

            for (; _tos; --_tos) {
                const int state = _stateStack[_tos];

                static int tks[] = {
                    T_RIGHT_BRACE, T_RIGHT_PAREN, T_RIGHT_BRACKET,
                    T_SEMICOLON, T_COLON, T_COMMA,
                    T_NUMBER, T_TYPE_NAME, T_IDENTIFIER,
                    T_LEFT_BRACE, T_LEFT_PAREN, T_LEFT_BRACKET,
                    T_WHILE,
                    0
                };

                for (int *tptr = tks; *tptr; ++tptr) {
                    const int next = t_action(state, *tptr);
                    if (next > 0) {
                        if (! yyrecovering && ! _recovered) {
                            _recovered = true;
                            error(line, QString::fromLatin1("Expected `%1'").arg(QLatin1String(spell[*tptr])));
                        }

                        yyrecovering = 3;
                        if (*tptr == T_IDENTIFIER)
                            yyval = (void *) _engine->identifier(QLatin1String("$identifier"));
                        else if (*tptr == T_NUMBER || *tptr == T_TYPE_NAME)
                            yyval = (void *) _engine->identifier(QLatin1String("$0"));
                        else
                            yyval = 0;

                        _symStack[_tos].ptr = yyval;
                        _locationStack[_tos] = yyloc;
                        yytoken = -1;

                        action = next;
                        goto again;
                    }
                }
            }

            if (! _recovered) {
                _recovered = true;
                error(line, message);
            }
        }

    } while (action);

    return 0;
}
./



/.
void Parser::reduce(int ruleno)
{
switch(ruleno) {
./



variable_identifier ::= IDENTIFIER ;
/.
case $rule_number: {
    ast(1) = makeAstNode<IdentifierExpressionAST>(string(1));
}   break;
./

primary_expression ::= NUMBER ;
/.
case $rule_number: {
    ast(1) = makeAstNode<LiteralExpressionAST>(string(1));
}   break;
./

primary_expression ::= TRUE ;
/.
case $rule_number: {
    ast(1) = makeAstNode<LiteralExpressionAST>(_engine->identifier("true", 4));
}   break;
./

primary_expression ::= FALSE ;
/.
case $rule_number: {
    ast(1) = makeAstNode<LiteralExpressionAST>(_engine->identifier("false", 5));
}   break;
./

primary_expression ::= variable_identifier ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

primary_expression ::= LEFT_PAREN expression RIGHT_PAREN ;
/.
case $rule_number: {
    ast(1) = ast(2);
}   break;
./

postfix_expression ::= primary_expression ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

postfix_expression ::= postfix_expression LEFT_BRACKET integer_expression RIGHT_BRACKET ;
/.
case $rule_number: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ArrayAccess, expression(1), expression(3));
}   break;
./

postfix_expression ::= function_call ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

postfix_expression ::= postfix_expression DOT IDENTIFIER ;
/.
case $rule_number: {
    ast(1) = makeAstNode<MemberAccessExpressionAST>(expression(1), string(3));
}   break;
./

postfix_expression ::= postfix_expression INC_OP ;
/.
case $rule_number: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PostIncrement, expression(1));
}   break;
./

postfix_expression ::= postfix_expression DEC_OP ;
/.
case $rule_number: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PostDecrement, expression(1));
}   break;
./

integer_expression ::= expression ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

function_call ::= function_call_or_method ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

function_call_or_method ::= function_call_generic ;
/.
case $rule_number: {
    ast(1) = makeAstNode<FunctionCallExpressionAST>
        (sym(1).function.id, sym(1).function.arguments);
}   break;
./

function_call_or_method ::= postfix_expression DOT function_call_generic ;
/.
case $rule_number: {
    ast(1) = makeAstNode<FunctionCallExpressionAST>
        (expression(1), sym(3).function.id, sym(3).function.arguments);
}   break;
./

function_call_generic ::= function_call_header_with_parameters RIGHT_PAREN ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

function_call_generic ::= function_call_header_no_parameters RIGHT_PAREN ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

function_call_header_no_parameters ::= function_call_header VOID ;
/.
case $rule_number: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments = 0;
}   break;
./

function_call_header_no_parameters ::= function_call_header ;
/.
case $rule_number: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments = 0;
}   break;
./

function_call_header_with_parameters ::= function_call_header assignment_expression ;
/.
case $rule_number: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments =
        makeAstNode< List<ExpressionAST *> >(expression(2));
}   break;
./

function_call_header_with_parameters ::= function_call_header_with_parameters COMMA assignment_expression ;
/.
case $rule_number: {
    sym(1).function.arguments =
        makeAstNode< List<ExpressionAST *> >
            (sym(1).function.arguments, expression(3));
}   break;
./

function_call_header ::= function_identifier LEFT_PAREN ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

function_identifier ::= type_specifier ;
/.
case $rule_number: {
    ast(1) = makeAstNode<FunctionIdentifierAST>(type(1));
}   break;
./

function_identifier ::= IDENTIFIER ;
/.
case $rule_number: {
    ast(1) = makeAstNode<FunctionIdentifierAST>(string(1));
}   break;
./

unary_expression ::= postfix_expression ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

unary_expression ::= INC_OP unary_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PreIncrement, expression(2));
}   break;
./

unary_expression ::= DEC_OP unary_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PreDecrement, expression(2));
}   break;
./

unary_expression ::= unary_operator unary_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<UnaryExpressionAST>(sym(1).kind, expression(2));
}   break;
./

unary_operator ::= PLUS ;
/.
case $rule_number: {
    sym(1).kind = AST::Kind_UnaryPlus;
}   break;
./

unary_operator ::= DASH ;
/.
case $rule_number: {
    sym(1).kind = AST::Kind_UnaryMinus;
}   break;
./

unary_operator ::= BANG ;
/.
case $rule_number: {
    sym(1).kind = AST::Kind_LogicalNot;
}   break;
./

unary_operator ::= TILDE ;
/.
case $rule_number: {
    sym(1).kind = AST::Kind_BitwiseNot;
}   break;
./

multiplicative_expression ::= unary_expression ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

multiplicative_expression ::= multiplicative_expression STAR unary_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Multiply, expression(1), expression(3));
}   break;
./

multiplicative_expression ::= multiplicative_expression SLASH unary_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Divide, expression(1), expression(3));
}   break;
./

multiplicative_expression ::= multiplicative_expression PERCENT unary_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Modulus, expression(1), expression(3));
}   break;
./

additive_expression ::= multiplicative_expression ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

additive_expression ::= additive_expression PLUS multiplicative_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Plus, expression(1), expression(3));
}   break;
./

additive_expression ::= additive_expression DASH multiplicative_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Minus, expression(1), expression(3));
}   break;
./

shift_expression ::= additive_expression ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

shift_expression ::= shift_expression LEFT_OP additive_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ShiftLeft, expression(1), expression(3));
}   break;
./

shift_expression ::= shift_expression RIGHT_OP additive_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ShiftRight, expression(1), expression(3));
}   break;
./

relational_expression ::= shift_expression ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

relational_expression ::= relational_expression LEFT_ANGLE shift_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LessThan, expression(1), expression(3));
}   break;
./

relational_expression ::= relational_expression RIGHT_ANGLE shift_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_GreaterThan, expression(1), expression(3));
}   break;
./

relational_expression ::= relational_expression LE_OP shift_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LessEqual, expression(1), expression(3));
}   break;
./

relational_expression ::= relational_expression GE_OP shift_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_GreaterEqual, expression(1), expression(3));
}   break;
./

equality_expression ::= relational_expression ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

equality_expression ::= equality_expression EQ_OP relational_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Equal, expression(1), expression(3));
}   break;
./

equality_expression ::= equality_expression NE_OP relational_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_NotEqual, expression(1), expression(3));
}   break;
./

and_expression ::= equality_expression ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

and_expression ::= and_expression AMPERSAND equality_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseAnd, expression(1), expression(3));
}   break;
./

exclusive_or_expression ::= and_expression ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

exclusive_or_expression ::= exclusive_or_expression CARET and_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseXor, expression(1), expression(3));
}   break;
./

inclusive_or_expression ::= exclusive_or_expression ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

inclusive_or_expression ::= inclusive_or_expression VERTICAL_BAR exclusive_or_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseOr, expression(1), expression(3));
}   break;
./

logical_and_expression ::= inclusive_or_expression ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

logical_and_expression ::= logical_and_expression AND_OP inclusive_or_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalAnd, expression(1), expression(3));
}   break;
./

logical_xor_expression ::= logical_and_expression ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

logical_xor_expression ::= logical_xor_expression XOR_OP logical_and_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalXor, expression(1), expression(3));
}   break;
./

logical_or_expression ::= logical_xor_expression ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

logical_or_expression ::= logical_or_expression OR_OP logical_xor_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalOr, expression(1), expression(3));
}   break;
./

conditional_expression ::= logical_or_expression ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

conditional_expression ::= logical_or_expression QUESTION expression COLON assignment_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<TernaryExpressionAST>(AST::Kind_Conditional, expression(1), expression(3), expression(5));
}   break;
./

assignment_expression ::= conditional_expression ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

assignment_expression ::= unary_expression assignment_operator assignment_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<AssignmentExpressionAST>(sym(2).kind, expression(1), expression(3));
}   break;
./

assignment_operator ::= EQUAL ;
/.
case $rule_number: {
    sym(1).kind = AST::Kind_Assign;
}   break;
./

assignment_operator ::= MUL_ASSIGN ;
/.
case $rule_number: {
    sym(1).kind = AST::Kind_AssignMultiply;
}   break;
./

assignment_operator ::= DIV_ASSIGN ;
/.
case $rule_number: {
    sym(1).kind = AST::Kind_AssignDivide;
}   break;
./

assignment_operator ::= MOD_ASSIGN ;
/.
case $rule_number: {
    sym(1).kind = AST::Kind_AssignModulus;
}   break;
./

assignment_operator ::= ADD_ASSIGN ;
/.
case $rule_number: {
    sym(1).kind = AST::Kind_AssignPlus;
}   break;
./

assignment_operator ::= SUB_ASSIGN ;
/.
case $rule_number: {
    sym(1).kind = AST::Kind_AssignMinus;
}   break;
./

assignment_operator ::= LEFT_ASSIGN ;
/.
case $rule_number: {
    sym(1).kind = AST::Kind_AssignShiftLeft;
}   break;
./

assignment_operator ::= RIGHT_ASSIGN ;
/.
case $rule_number: {
    sym(1).kind = AST::Kind_AssignShiftRight;
}   break;
./

assignment_operator ::= AND_ASSIGN ;
/.
case $rule_number: {
    sym(1).kind = AST::Kind_AssignAnd;
}   break;
./

assignment_operator ::= XOR_ASSIGN ;
/.
case $rule_number: {
    sym(1).kind = AST::Kind_AssignXor;
}   break;
./

assignment_operator ::= OR_ASSIGN ;
/.
case $rule_number: {
    sym(1).kind = AST::Kind_AssignOr;
}   break;
./

expression ::= assignment_expression ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

expression ::= expression COMMA assignment_expression ;
/.
case $rule_number: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Comma, expression(1), expression(3));
}   break;
./

constant_expression ::= conditional_expression ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

declaration ::= function_prototype SEMICOLON ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

declaration ::= init_declarator_list SEMICOLON ;
/.
case $rule_number: {
    ast(1) = makeAstNode<InitDeclarationAST>(sym(1).declaration_list);
}   break;
./

declaration ::= PRECISION precision_qualifier type_specifier_no_prec SEMICOLON ;
/.
case $rule_number: {
    ast(1) = makeAstNode<PrecisionDeclarationAST>(sym(2).precision, type(3));
}   break;
./

declaration ::= type_qualifier IDENTIFIER LEFT_BRACE struct_declaration_list RIGHT_BRACE SEMICOLON ;
/.
case $rule_number: {
    if (sym(1).type_qualifier.qualifier != QualifiedTypeAST::Struct) {
        // TODO: issue an error if the qualifier is not "struct".
    }
    TypeAST *type = makeAstNode<StructTypeAST>(string(2), sym(4).field_list);
    ast(1) = makeAstNode<TypeDeclarationAST>(type);
}   break;
./

declaration ::= type_qualifier IDENTIFIER LEFT_BRACE struct_declaration_list RIGHT_BRACE IDENTIFIER SEMICOLON ;
/.
case $rule_number: {
    if ((sym(1).type_qualifier.qualifier & QualifiedTypeAST::Struct) == 0) {
        // TODO: issue an error if the qualifier does not contain "struct".
    }
    TypeAST *type = makeAstNode<StructTypeAST>(string(2), sym(4).field_list);
    TypeAST *qualtype = type;
    if (sym(1).type_qualifier.qualifier != QualifiedTypeAST::Struct) {
        qualtype = makeAstNode<QualifiedTypeAST>
            (sym(1).type_qualifier.qualifier & ~QualifiedTypeAST::Struct, qualtype,
             sym(1).type_qualifier.layout_list);
    }
    ast(1) = makeAstNode<TypeAndVariableDeclarationAST>
        (makeAstNode<TypeDeclarationAST>(type),
         makeAstNode<VariableDeclarationAST>(qualtype, string(6)));
}   break;
./

declaration ::= type_qualifier IDENTIFIER LEFT_BRACE struct_declaration_list RIGHT_BRACE IDENTIFIER LEFT_BRACKET RIGHT_BRACKET SEMICOLON ;
/.
case $rule_number: {
    if ((sym(1).type_qualifier.qualifier & QualifiedTypeAST::Struct) == 0) {
        // TODO: issue an error if the qualifier does not contain "struct".
    }
    TypeAST *type = makeAstNode<StructTypeAST>(string(2), sym(4).field_list);
    TypeAST *qualtype = type;
    if (sym(1).type_qualifier.qualifier != QualifiedTypeAST::Struct) {
        qualtype = makeAstNode<QualifiedTypeAST>
            (sym(1).type_qualifier.qualifier & ~QualifiedTypeAST::Struct, qualtype,
             sym(1).type_qualifier.layout_list);
    }
    ast(1) = makeAstNode<TypeAndVariableDeclarationAST>
        (makeAstNode<TypeDeclarationAST>(type),
         makeAstNode<VariableDeclarationAST>
            (makeAstNode<ArrayTypeAST>(qualtype), string(6)));
}   break;
./

declaration ::= type_qualifier IDENTIFIER LEFT_BRACE struct_declaration_list RIGHT_BRACE IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET SEMICOLON ;
/.
case $rule_number: {
    if ((sym(1).type_qualifier.qualifier & QualifiedTypeAST::Struct) == 0) {
        // TODO: issue an error if the qualifier does not contain "struct".
    }
    TypeAST *type = makeAstNode<StructTypeAST>(string(2), sym(4).field_list);
    TypeAST *qualtype = type;
    if (sym(1).type_qualifier.qualifier != QualifiedTypeAST::Struct) {
        qualtype = makeAstNode<QualifiedTypeAST>
            (sym(1).type_qualifier.qualifier & ~QualifiedTypeAST::Struct, qualtype,
             sym(1).type_qualifier.layout_list);
    }
    ast(1) = makeAstNode<TypeAndVariableDeclarationAST>
        (makeAstNode<TypeDeclarationAST>(type),
         makeAstNode<VariableDeclarationAST>
            (makeAstNode<ArrayTypeAST>(qualtype, expression(8)), string(6)));
}   break;
./

declaration ::= type_qualifier SEMICOLON ;
/.
case $rule_number: {
    TypeAST *type = makeAstNode<QualifiedTypeAST>
        (sym(1).type_qualifier.qualifier, (TypeAST *)0,
         sym(1).type_qualifier.layout_list);
    ast(1) = makeAstNode<TypeDeclarationAST>(type);
}   break;
./

function_prototype ::= function_declarator RIGHT_PAREN ;
/.
case $rule_number: {
    function(1)->finishParams();
}   break;
./

function_declarator ::= function_header ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

function_declarator ::= function_header_with_parameters ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

function_header_with_parameters ::= function_header parameter_declaration ;
/.
case $rule_number: {
    function(1)->params = makeAstNode< List<ParameterDeclarationAST *> >
        (sym(2).param_declaration);
}   break;
./

function_header_with_parameters ::= function_header_with_parameters COMMA parameter_declaration ;
/.
case $rule_number: {
    function(1)->params = makeAstNode< List<ParameterDeclarationAST *> >
        (function(1)->params, sym(3).param_declaration);
}   break;
./

function_header ::= fully_specified_type IDENTIFIER LEFT_PAREN ;
/.
case $rule_number: {
    function(1) = makeAstNode<FunctionDeclarationAST>(type(1), string(2));
}   break;
./

parameter_declarator ::= type_specifier IDENTIFIER ;
/.
case $rule_number: {
    sym(1).param_declarator.type = type(1);
    sym(1).param_declarator.name = string(2);
}   break;
./

parameter_declarator ::= type_specifier IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET ;
/.
case $rule_number: {
    sym(1).param_declarator.type = makeAstNode<ArrayTypeAST>(type(1), expression(4));
    sym(1).param_declarator.name = string(2);
}   break;
./

parameter_declaration ::= parameter_type_qualifier parameter_qualifier parameter_declarator ;
/.
case $rule_number: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (makeAstNode<QualifiedTypeAST>
            (sym(1).qualifier, sym(3).param_declarator.type,
             (List<LayoutQualifierAST *> *)0),
         ParameterDeclarationAST::Qualifier(sym(2).qualifier),
         sym(3).param_declarator.name);
}   break;
./

parameter_declaration ::= parameter_qualifier parameter_declarator ;
/.
case $rule_number: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (sym(2).param_declarator.type,
         ParameterDeclarationAST::Qualifier(sym(1).qualifier),
         sym(2).param_declarator.name);
}   break;
./

parameter_declaration ::= parameter_type_qualifier parameter_qualifier parameter_type_specifier ;
/.
case $rule_number: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (makeAstNode<QualifiedTypeAST>
            (sym(1).qualifier, type(3), (List<LayoutQualifierAST *> *)0),
         ParameterDeclarationAST::Qualifier(sym(2).qualifier),
         (const QString *)0);
}   break;
./

parameter_declaration ::= parameter_qualifier parameter_type_specifier ;
/.
case $rule_number: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (type(2), ParameterDeclarationAST::Qualifier(sym(1).qualifier),
         (const QString *)0);
}   break;
./

parameter_qualifier ::= empty ;
/.
case $rule_number: {
    sym(1).qualifier = ParameterDeclarationAST::In;
}   break;
./

parameter_qualifier ::= IN ;
/.
case $rule_number: {
    sym(1).qualifier = ParameterDeclarationAST::In;
}   break;
./

parameter_qualifier ::= OUT ;
/.
case $rule_number: {
    sym(1).qualifier = ParameterDeclarationAST::Out;
}   break;
./

parameter_qualifier ::= INOUT ;
/.
case $rule_number: {
    sym(1).qualifier = ParameterDeclarationAST::InOut;
}   break;
./

parameter_type_specifier ::= type_specifier ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

init_declarator_list ::= single_declaration ;
/.
case $rule_number: {
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
        (sym(1).declaration);
}   break;
./

init_declarator_list ::= init_declarator_list COMMA IDENTIFIER ;
/.
case $rule_number: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;
./

init_declarator_list ::= init_declarator_list COMMA IDENTIFIER LEFT_BRACKET RIGHT_BRACKET ;
/.
case $rule_number: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;
./

init_declarator_list ::= init_declarator_list COMMA IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET ;
/.
case $rule_number: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type, expression(5));
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;
./

init_declarator_list ::= init_declarator_list COMMA IDENTIFIER LEFT_BRACKET RIGHT_BRACKET EQUAL initializer ;
/.
case $rule_number: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>
            (type, string(3), expression(7));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;
./

init_declarator_list ::= init_declarator_list COMMA IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET EQUAL initializer ;
/.
case $rule_number: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type, expression(5));
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>
            (type, string(3), expression(8));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;
./

init_declarator_list ::= init_declarator_list COMMA IDENTIFIER EQUAL initializer ;
/.
case $rule_number: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>
            (type, string(3), expression(5));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;
./

single_declaration ::= fully_specified_type ;
/.
case $rule_number: {
    ast(1) = makeAstNode<TypeDeclarationAST>(type(1));
}   break;
./

single_declaration ::= fully_specified_type IDENTIFIER ;
/.
case $rule_number: {
    ast(1) = makeAstNode<VariableDeclarationAST>(type(1), string(2));
}   break;
./

single_declaration ::= fully_specified_type IDENTIFIER LEFT_BRACKET RIGHT_BRACKET ;
/.
case $rule_number: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1)), string(2));
}   break;
./

single_declaration ::= fully_specified_type IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET ;
/.
case $rule_number: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1), expression(4)), string(2));
}   break;
./

single_declaration ::= fully_specified_type IDENTIFIER LEFT_BRACKET RIGHT_BRACKET EQUAL initializer ;
/.
case $rule_number: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1)), string(2), expression(6));
}   break;
./

single_declaration ::= fully_specified_type IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET EQUAL initializer ;
/.
case $rule_number: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1), expression(4)),
         string(2), expression(7));
}   break;
./

single_declaration ::= fully_specified_type IDENTIFIER EQUAL initializer ;
/.
case $rule_number: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (type(1), string(2), expression(4));
}   break;
./

single_declaration ::= INVARIANT IDENTIFIER ;
/.
case $rule_number: {
    ast(1) = makeAstNode<InvariantDeclarationAST>(string(2));
}   break;
./

fully_specified_type ::= type_specifier ;
/.
case $rule_number: {
    ast(1) = makeAstNode<QualifiedTypeAST>(0, type(1), (List<LayoutQualifierAST *> *)0);
}   break;
./

fully_specified_type ::= type_qualifier type_specifier ;
/.
case $rule_number: {
    ast(1) = makeAstNode<QualifiedTypeAST>
        (sym(1).type_qualifier.qualifier, type(2),
         sym(1).type_qualifier.layout_list);
}   break;
./

invariant_qualifier ::= INVARIANT ;
/.
case $rule_number: {
    sym(1).qualifier = QualifiedTypeAST::Invariant;
}   break;
./

interpolation_qualifier ::= SMOOTH ;
/.
case $rule_number: {
    sym(1).qualifier = QualifiedTypeAST::Smooth;
}   break;
./

interpolation_qualifier ::= FLAT ;
/.
case $rule_number: {
    sym(1).qualifier = QualifiedTypeAST::Flat;
}   break;
./

interpolation_qualifier ::= NOPERSPECTIVE ;
/.
case $rule_number: {
    sym(1).qualifier = QualifiedTypeAST::NoPerspective;
}   break;
./

layout_qualifier ::= LAYOUT LEFT_PAREN layout_qualifier_id_list RIGHT_PAREN ;
/.
case $rule_number: {
    sym(1) = sym(3);
}   break;
./

layout_qualifier_id_list ::= layout_qualifier_id ;
/.
case $rule_number: {
    sym(1).layout_list = makeAstNode< List<LayoutQualifierAST *> >(sym(1).layout);
}   break;
./

layout_qualifier_id_list ::= layout_qualifier_id_list COMMA layout_qualifier_id ;
/.
case $rule_number: {
    sym(1).layout_list = makeAstNode< List<LayoutQualifierAST *> >(sym(1).layout_list, sym(3).layout);
}   break;
./

layout_qualifier_id ::= IDENTIFIER ;
/.
case $rule_number: {
    sym(1).layout = makeAstNode<LayoutQualifierAST>(string(1), (const QString *)0);
}   break;
./

layout_qualifier_id ::= IDENTIFIER EQUAL NUMBER ;
/.
case $rule_number: {
    sym(1).layout = makeAstNode<LayoutQualifierAST>(string(1), string(3));
}   break;
./

parameter_type_qualifier ::= CONST ;
/.
case $rule_number: {
    sym(1).qualifier = QualifiedTypeAST::Const;
}   break;
./

type_qualifier ::= storage_qualifier ;
/.
case $rule_number: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;
./

type_qualifier ::= layout_qualifier ;
/.
case $rule_number: {
    sym(1).type_qualifier.layout_list = sym(1).layout_list;
    sym(1).type_qualifier.qualifier = 0;
}   break;
./

type_qualifier ::= layout_qualifier storage_qualifier ;
/.
case $rule_number: {
    sym(1).type_qualifier.layout_list = sym(1).layout_list;
    sym(1).type_qualifier.qualifier = sym(2).qualifier;
}   break;
./

type_qualifier ::= interpolation_qualifier storage_qualifier ;
/.
case $rule_number: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;
./

type_qualifier ::= interpolation_qualifier ;
/.
case $rule_number: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;
./

type_qualifier ::= invariant_qualifier storage_qualifier ;
/.
case $rule_number: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;
./

type_qualifier ::= invariant_qualifier interpolation_qualifier storage_qualifier ;
/.
case $rule_number: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier | sym(3).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;
./

type_qualifier ::= INVARIANT ;
/.
case $rule_number: {
    sym(1).type_qualifier.qualifier = QualifiedTypeAST::Invariant;
    sym(1).type_qualifier.layout_list = 0;
}   break;
./

storage_qualifier ::= CONST ;
/.
case $rule_number: {
    sym(1).qualifier = QualifiedTypeAST::Const;
}   break;
./

storage_qualifier ::= ATTRIBUTE ;
/.
case $rule_number: {
    sym(1).qualifier = QualifiedTypeAST::Attribute;
}   break;
./

storage_qualifier ::= VARYING ;
/.
case $rule_number: {
    sym(1).qualifier = QualifiedTypeAST::Varying;
}   break;
./

storage_qualifier ::= CENTROID VARYING ;
/.
case $rule_number: {
    sym(1).qualifier = QualifiedTypeAST::CentroidVarying;
}   break;
./

storage_qualifier ::= IN ;
/.
case $rule_number: {
    sym(1).qualifier = QualifiedTypeAST::In;
}   break;
./

storage_qualifier ::= OUT ;
/.
case $rule_number: {
    sym(1).qualifier = QualifiedTypeAST::Out;
}   break;
./

storage_qualifier ::= CENTROID IN ;
/.
case $rule_number: {
    sym(1).qualifier = QualifiedTypeAST::CentroidIn;
}   break;
./

storage_qualifier ::= CENTROID OUT ;
/.
case $rule_number: {
    sym(1).qualifier = QualifiedTypeAST::CentroidOut;
}   break;
./

storage_qualifier ::= PATCH IN ;
/.
case $rule_number: {
    sym(1).qualifier = QualifiedTypeAST::PatchIn;
}   break;
./

storage_qualifier ::= PATCH OUT ;
/.
case $rule_number: {
    sym(1).qualifier = QualifiedTypeAST::PatchOut;
}   break;
./

storage_qualifier ::= SAMPLE IN ;
/.
case $rule_number: {
    sym(1).qualifier = QualifiedTypeAST::SampleIn;
}   break;
./

storage_qualifier ::= SAMPLE OUT ;
/.
case $rule_number: {
    sym(1).qualifier = QualifiedTypeAST::SampleOut;
}   break;
./

storage_qualifier ::= UNIFORM ;
/.
case $rule_number: {
    sym(1).qualifier = QualifiedTypeAST::Uniform;
}   break;
./

type_specifier ::= type_specifier_no_prec ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

type_specifier ::= precision_qualifier type_specifier_no_prec ;
/.
case $rule_number: {
    if (!type(2)->setPrecision(sym(1).precision)) {
        // TODO: issue an error about precision not allowed on this type.
    }
    ast(1) = type(2);
}   break;
./

type_specifier_no_prec ::= type_specifier_nonarray ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

type_specifier_no_prec ::= type_specifier_nonarray LEFT_BRACKET RIGHT_BRACKET ;
/.
case $rule_number: {
    ast(1) = makeAstNode<ArrayTypeAST>(type(1));
}   break;
./

type_specifier_no_prec ::= type_specifier_nonarray LEFT_BRACKET constant_expression RIGHT_BRACKET ;
/.
case $rule_number: {
    ast(1) = makeAstNode<ArrayTypeAST>(type(1), expression(3));
}   break;
./

type_specifier_nonarray ::= VOID ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_VOID);
}   break;
./

type_specifier_nonarray ::= FLOAT ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_FLOAT);
}   break;
./

type_specifier_nonarray ::= DOUBLE ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_DOUBLE);
}   break;
./

type_specifier_nonarray ::= INT ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_INT);
}   break;
./

type_specifier_nonarray ::= UINT ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_UINT);
}   break;
./

type_specifier_nonarray ::= BOOL ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_BOOL);
}   break;
./

type_specifier_nonarray ::= VEC2 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_VEC2);
}   break;
./

type_specifier_nonarray ::= VEC3 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_VEC3);
}   break;
./

type_specifier_nonarray ::= VEC4 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_VEC4);
}   break;
./

type_specifier_nonarray ::= DVEC2 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_DVEC2);
}   break;
./

type_specifier_nonarray ::= DVEC3 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_DVEC3);
}   break;
./

type_specifier_nonarray ::= DVEC4 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_DVEC4);
}   break;
./

type_specifier_nonarray ::= BVEC2 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_BVEC2);
}   break;
./

type_specifier_nonarray ::= BVEC3 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_BVEC3);
}   break;
./

type_specifier_nonarray ::= BVEC4 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_BVEC4);
}   break;
./

type_specifier_nonarray ::= IVEC2 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_IVEC2);
}   break;
./

type_specifier_nonarray ::= IVEC3 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_IVEC3);
}   break;
./

type_specifier_nonarray ::= IVEC4 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_IVEC4);
}   break;
./

type_specifier_nonarray ::= UVEC2 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_UVEC2);
}   break;
./

type_specifier_nonarray ::= UVEC3 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_UVEC3);
}   break;
./

type_specifier_nonarray ::= UVEC4 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_UVEC4);
}   break;
./

type_specifier_nonarray ::= MAT2 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_MAT2);
}   break;
./

type_specifier_nonarray ::= MAT3 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_MAT3);
}   break;
./

type_specifier_nonarray ::= MAT4 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_MAT4);
}   break;
./

type_specifier_nonarray ::= MAT2X2 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_MAT2);
}   break;
./

type_specifier_nonarray ::= MAT2X3 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_MAT2X3);
}   break;
./

type_specifier_nonarray ::= MAT2X4 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_MAT2X4);
}   break;
./

type_specifier_nonarray ::= MAT3X2 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_MAT3X2);
}   break;
./

type_specifier_nonarray ::= MAT3X3 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_MAT3);
}   break;
./

type_specifier_nonarray ::= MAT3X4 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_MAT3X4);
}   break;
./

type_specifier_nonarray ::= MAT4X2 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_MAT4X2);
}   break;
./

type_specifier_nonarray ::= MAT4X3 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_MAT4X3);
}   break;
./

type_specifier_nonarray ::= MAT4X4 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_MAT4);
}   break;
./

type_specifier_nonarray ::= DMAT2 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_DMAT2);
}   break;
./

type_specifier_nonarray ::= DMAT3 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_DMAT3);
}   break;
./

type_specifier_nonarray ::= DMAT4 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_DMAT4);
}   break;
./

type_specifier_nonarray ::= DMAT2X2 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_DMAT2);
}   break;
./

type_specifier_nonarray ::= DMAT2X3 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_DMAT2X3);
}   break;
./

type_specifier_nonarray ::= DMAT2X4 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_DMAT2X4);
}   break;
./

type_specifier_nonarray ::= DMAT3X2 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_DMAT3X2);
}   break;
./

type_specifier_nonarray ::= DMAT3X3 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_DMAT3);
}   break;
./

type_specifier_nonarray ::= DMAT3X4 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_DMAT3X4);
}   break;
./

type_specifier_nonarray ::= DMAT4X2 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_DMAT4X2);
}   break;
./

type_specifier_nonarray ::= DMAT4X3 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_DMAT4X3);
}   break;
./

type_specifier_nonarray ::= DMAT4X4 ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_DMAT4);
}   break;
./

type_specifier_nonarray ::= SAMPLER1D ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_SAMPLER1D);
}   break;
./

type_specifier_nonarray ::= SAMPLER2D ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_SAMPLER2D);
}   break;
./

type_specifier_nonarray ::= SAMPLER3D ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_SAMPLER3D);
}   break;
./

type_specifier_nonarray ::= SAMPLERCUBE ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_SAMPLERCUBE);
}   break;
./

type_specifier_nonarray ::= SAMPLER1DSHADOW ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_SAMPLER1DSHADOW);
}   break;
./

type_specifier_nonarray ::= SAMPLER2DSHADOW ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_SAMPLER2DSHADOW);
}   break;
./

type_specifier_nonarray ::= SAMPLERCUBESHADOW ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_SAMPLERCUBESHADOW);
}   break;
./

type_specifier_nonarray ::= SAMPLER1DARRAY ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_SAMPLER1DARRAY);
}   break;
./

type_specifier_nonarray ::= SAMPLER2DARRAY ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_SAMPLER2DARRAY);
}   break;
./

type_specifier_nonarray ::= SAMPLER1DARRAYSHADOW ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_SAMPLER1DARRAYSHADOW);
}   break;
./

type_specifier_nonarray ::= SAMPLER2DARRAYSHADOW ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_SAMPLER2DARRAYSHADOW);
}   break;
./

type_specifier_nonarray ::= SAMPLERCUBEARRAY ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_SAMPLERCUBEARRAY);
}   break;
./

type_specifier_nonarray ::= SAMPLERCUBEARRAYSHADOW ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_SAMPLERCUBEARRAYSHADOW);
}   break;
./

type_specifier_nonarray ::= ISAMPLER1D ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_ISAMPLER1D);
}   break;
./

type_specifier_nonarray ::= ISAMPLER2D ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_ISAMPLER2D);
}   break;
./

type_specifier_nonarray ::= ISAMPLER3D ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_ISAMPLER3D);
}   break;
./

type_specifier_nonarray ::= ISAMPLERCUBE ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_ISAMPLERCUBE);
}   break;
./

type_specifier_nonarray ::= ISAMPLER1DARRAY ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_ISAMPLER1DARRAY);
}   break;
./

type_specifier_nonarray ::= ISAMPLER2DARRAY ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_ISAMPLER2DARRAY);
}   break;
./

type_specifier_nonarray ::= ISAMPLERCUBEARRAY ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_ISAMPLERCUBEARRAY);
}   break;
./

type_specifier_nonarray ::= USAMPLER1D ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_USAMPLER1D);
}   break;
./

type_specifier_nonarray ::= USAMPLER2D ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_USAMPLER2D);
}   break;
./

type_specifier_nonarray ::= USAMPLER3D ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_USAMPLER3D);
}   break;
./

type_specifier_nonarray ::= USAMPLERCUBE ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_USAMPLERCUBE);
}   break;
./

type_specifier_nonarray ::= USAMPLER1DARRAY ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_USAMPLER1DARRAY);
}   break;
./

type_specifier_nonarray ::= USAMPLER2DARRAY ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_USAMPLER2DARRAY);
}   break;
./

type_specifier_nonarray ::= USAMPLERCUBEARRAY ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_USAMPLERCUBEARRAY);
}   break;
./

type_specifier_nonarray ::= SAMPLER2DRECT ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_SAMPLER2DRECT);
}   break;
./

type_specifier_nonarray ::= SAMPLER2DRECTSHADOW ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_SAMPLER2DRECTSHADOW);
}   break;
./

type_specifier_nonarray ::= ISAMPLER2DRECT ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_ISAMPLER2DRECT);
}   break;
./

type_specifier_nonarray ::= USAMPLER2DRECT ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_USAMPLER2DRECT);
}   break;
./

type_specifier_nonarray ::= SAMPLERBUFFER ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_SAMPLERBUFFER);
}   break;
./

type_specifier_nonarray ::= ISAMPLERBUFFER ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_ISAMPLERBUFFER);
}   break;
./

type_specifier_nonarray ::= USAMPLERBUFFER ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_USAMPLERBUFFER);
}   break;
./

type_specifier_nonarray ::= SAMPLER2DMS ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_SAMPLER2DMS);
}   break;
./

type_specifier_nonarray ::= ISAMPLER2DMS ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_ISAMPLER2DMS);
}   break;
./

type_specifier_nonarray ::= USAMPLER2DMS ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_USAMPLER2DMS);
}   break;
./

type_specifier_nonarray ::= SAMPLER2DMSARRAY ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_SAMPLER2DMSARRAY);
}   break;
./

type_specifier_nonarray ::= ISAMPLER2DMSARRAY ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_ISAMPLER2DMSARRAY);
}   break;
./

type_specifier_nonarray ::= USAMPLER2DMSARRAY ;
/.
case $rule_number: {
    ast(1) = makeBasicType(T_USAMPLER2DMSARRAY);
}   break;
./

type_specifier_nonarray ::= struct_specifier ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

type_specifier_nonarray ::= TYPE_NAME ;
/.
case $rule_number: {
    ast(1) = makeAstNode<NamedTypeAST>(string(1));
}   break;
./

precision_qualifier ::= HIGHP ;
/.
case $rule_number: {
    sym(1).precision = TypeAST::Highp;
}   break;
./

precision_qualifier ::= MEDIUMP ;
/.
case $rule_number: {
    sym(1).precision = TypeAST::Mediump;
}   break;
./

precision_qualifier ::= LOWP ;
/.
case $rule_number: {
    sym(1).precision = TypeAST::Lowp;
}   break;
./

struct_specifier ::= STRUCT IDENTIFIER LEFT_BRACE struct_declaration_list RIGHT_BRACE ;
/.
case $rule_number: {
    ast(1) = makeAstNode<StructTypeAST>(string(2), sym(4).field_list);
}   break;
./

struct_specifier ::= STRUCT LEFT_BRACE struct_declaration_list RIGHT_BRACE ;
/.
case $rule_number: {
    ast(1) = makeAstNode<StructTypeAST>(sym(3).field_list);
}   break;
./

struct_declaration_list ::= struct_declaration ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

struct_declaration_list ::= struct_declaration_list struct_declaration ;
/.
case $rule_number: {
    sym(1).field_list = appendLists(sym(1).field_list, sym(2).field_list);
}   break;
./

struct_declaration ::= type_specifier struct_declarator_list SEMICOLON ;
/.
case $rule_number: {
    sym(1).field_list = StructTypeAST::fixInnerTypes(type(1), sym(2).field_list);
}   break;
./

struct_declaration ::= type_qualifier type_specifier struct_declarator_list SEMICOLON ;
/.
case $rule_number: {
    sym(1).field_list = StructTypeAST::fixInnerTypes
        (makeAstNode<QualifiedTypeAST>
            (sym(1).type_qualifier.qualifier, type(2),
             sym(1).type_qualifier.layout_list), sym(3).field_list);
}   break;
./

struct_declarator_list ::= struct_declarator ;
/.
case $rule_number: {
    // nothing to do.
    sym(1).field_list = makeAstNode< List<StructTypeAST::Field *> >(sym(1).field);
}   break;
./

struct_declarator_list ::= struct_declarator_list COMMA struct_declarator ;
/.
case $rule_number: {
    sym(1).field_list = makeAstNode< List<StructTypeAST::Field *> >(sym(1).field_list, sym(3).field);
}   break;
./

struct_declarator ::= IDENTIFIER ;
/.
case $rule_number: {
    sym(1).field = makeAstNode<StructTypeAST::Field>(string(1));
}   break;
./

struct_declarator ::= IDENTIFIER LEFT_BRACKET RIGHT_BRACKET ;
/.
case $rule_number: {
    sym(1).field = makeAstNode<StructTypeAST::Field>
        (string(1), makeAstNode<ArrayTypeAST>((TypeAST *)0));
}   break;
./

struct_declarator ::= IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET ;
/.
case $rule_number: {
    sym(1).field = makeAstNode<StructTypeAST::Field>
        (string(1), makeAstNode<ArrayTypeAST>((TypeAST *)0, expression(3)));
}   break;
./

initializer ::= assignment_expression ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

declaration_statement ::= declaration ;
/.
case $rule_number: {
    ast(1) = makeAstNode<DeclarationStatementAST>(sym(1).declaration);
}   break;
./

statement ::= compound_statement ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

statement ::= simple_statement ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

simple_statement ::= declaration_statement ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

simple_statement ::= expression_statement ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

simple_statement ::= selection_statement ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

simple_statement ::= switch_statement ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

simple_statement ::= case_label ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

simple_statement ::= iteration_statement ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

simple_statement ::= jump_statement ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

compound_statement ::= LEFT_BRACE RIGHT_BRACE ;
/.
case $rule_number: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>();
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(2)).end();
    ast(1) = stmt;
}   break;
./

compound_statement ::= LEFT_BRACE statement_list RIGHT_BRACE ;
/.
case $rule_number: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>(sym(2).statement_list);
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(3)).end();
    ast(1) = stmt;
}   break;
./

statement_no_new_scope ::= compound_statement_no_new_scope ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

statement_no_new_scope ::= simple_statement ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

compound_statement_no_new_scope ::= LEFT_BRACE RIGHT_BRACE ;
/.
case $rule_number: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>();
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(2)).end();
    ast(1) = stmt;
}   break;
./

compound_statement_no_new_scope ::= LEFT_BRACE statement_list RIGHT_BRACE ;
/.
case $rule_number: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>(sym(2).statement_list);
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(3)).end();
    ast(1) = stmt;
}   break;
./

statement_list ::= statement ;
/.
case $rule_number: {
    sym(1).statement_list = makeAstNode< List<StatementAST *> >(sym(1).statement);
}   break;
./

statement_list ::= statement_list statement ;
/.
case $rule_number: {
    sym(1).statement_list = makeAstNode< List<StatementAST *> >(sym(1).statement_list, sym(2).statement);
}   break;
./

expression_statement ::= SEMICOLON ;
/.
case $rule_number: {
    ast(1) = makeAstNode<CompoundStatementAST>();  // Empty statement
}   break;
./

expression_statement ::= expression SEMICOLON ;
/.
case $rule_number: {
    ast(1) = makeAstNode<ExpressionStatementAST>(expression(1));
}   break;
./

selection_statement ::= IF LEFT_PAREN expression RIGHT_PAREN selection_rest_statement ;
/.
case $rule_number: {
    ast(1) = makeAstNode<IfStatementAST>(expression(3), sym(5).ifstmt.thenClause, sym(5).ifstmt.elseClause);
}   break;
./

selection_rest_statement ::= statement ELSE statement ;
/.
case $rule_number: {
    sym(1).ifstmt.thenClause = statement(1);
    sym(1).ifstmt.elseClause = statement(3);
}   break;
./

selection_rest_statement ::= statement ;
/.
case $rule_number: {
    sym(1).ifstmt.thenClause = statement(1);
    sym(1).ifstmt.elseClause = 0;
}   break;
./

condition ::= expression ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

condition ::= fully_specified_type IDENTIFIER EQUAL initializer ;
/.
case $rule_number: {
    ast(1) = makeAstNode<DeclarationExpressionAST>
        (type(1), string(2), expression(4));
}   break;
./

switch_statement ::= SWITCH LEFT_PAREN expression RIGHT_PAREN LEFT_BRACE switch_statement_list RIGHT_BRACE ;
/.
case $rule_number: {
    ast(1) = makeAstNode<SwitchStatementAST>(expression(3), statement(6));
}   break;
./

switch_statement_list ::= empty ;
/.
case $rule_number: {
    ast(1) = makeAstNode<CompoundStatementAST>();
}   break;
./

switch_statement_list ::= statement_list ;
/.
case $rule_number: {
    ast(1) = makeAstNode<CompoundStatementAST>(sym(1).statement_list);
}   break;
./

case_label ::= CASE expression COLON ;
/.
case $rule_number: {
    ast(1) = makeAstNode<CaseLabelStatementAST>(expression(2));
}   break;
./

case_label ::= DEFAULT COLON ;
/.
case $rule_number: {
    ast(1) = makeAstNode<CaseLabelStatementAST>();
}   break;
./

iteration_statement ::= WHILE LEFT_PAREN condition RIGHT_PAREN statement_no_new_scope ;
/.
case $rule_number: {
    ast(1) = makeAstNode<WhileStatementAST>(expression(3), statement(5));
}   break;
./

iteration_statement ::= DO statement WHILE LEFT_PAREN expression RIGHT_PAREN SEMICOLON ;
/.
case $rule_number: {
    ast(1) = makeAstNode<DoStatementAST>(statement(2), expression(5));
}   break;
./

iteration_statement ::= FOR LEFT_PAREN for_init_statement for_rest_statement RIGHT_PAREN statement_no_new_scope ;
/.
case $rule_number: {
    ast(1) = makeAstNode<ForStatementAST>(statement(3), sym(4).forstmt.condition, sym(4).forstmt.increment, statement(6));
}   break;
./

for_init_statement ::= expression_statement ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

for_init_statement ::= declaration_statement ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

conditionopt ::= empty ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

conditionopt ::= condition ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

for_rest_statement ::= conditionopt SEMICOLON ;
/.
case $rule_number: {
    sym(1).forstmt.condition = expression(1);
    sym(1).forstmt.increment = 0;
}   break;
./

for_rest_statement ::= conditionopt SEMICOLON expression ;
/.
case $rule_number: {
    sym(1).forstmt.condition = expression(1);
    sym(1).forstmt.increment = expression(3);
}   break;
./

jump_statement ::= CONTINUE SEMICOLON ;
/.
case $rule_number: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Continue);
}   break;
./

jump_statement ::= BREAK SEMICOLON ;
/.
case $rule_number: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Break);
}   break;
./

jump_statement ::= RETURN SEMICOLON ;
/.
case $rule_number: {
    ast(1) = makeAstNode<ReturnStatementAST>();
}   break;
./

jump_statement ::= RETURN expression SEMICOLON ;
/.
case $rule_number: {
    ast(1) = makeAstNode<ReturnStatementAST>(expression(2));
}   break;
./

jump_statement ::= DISCARD SEMICOLON ;
/.
case $rule_number: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Discard);
}   break;
./

translation_unit ::= external_declaration_list ;
/.
case $rule_number: {
    ast(1) = makeAstNode<TranslationUnitAST>(sym(1).declaration_list);
}   break;
./

external_declaration_list ::= external_declaration ;
/.
case $rule_number: {
    if (sym(1).declaration) {
        sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration);
    } else {
        sym(1).declaration_list = 0;
    }
}   break;
./

external_declaration_list ::= external_declaration_list external_declaration ;
/.
case $rule_number: {
    if (sym(1).declaration_list && sym(2).declaration) {
        sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, sym(2).declaration);
    } else if (!sym(1).declaration_list) {
        if (sym(2).declaration) {
            sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
                (sym(2).declaration);
        } else {
            sym(1).declaration_list = 0;
        }
    }
}   break;
./

external_declaration ::= function_definition ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

external_declaration ::= declaration ;
/.
case $rule_number: {
    // nothing to do.
}   break;
./

external_declaration ::= SEMICOLON ;
/.
case $rule_number: {
    ast(1) = 0;
}   break;
./

function_definition ::= function_prototype compound_statement_no_new_scope ;
/.
case $rule_number: {
    function(1)->body = statement(2);
}   break;
./

empty ::= ;
/.
case $rule_number: {
    ast(1) = 0;
}   break;
./


toplevel ::= FEED_GLSL translation_unit ;
/.
case $rule_number: {
    ast(1) = ast(2);
}   break;
./

toplevel ::= FEED_EXPRESSION expression ;
/.
case $rule_number: {
    ast(1) = ast(2);
}   break;
./

/.
} // end switch
} // end Parser::reduce()
./
