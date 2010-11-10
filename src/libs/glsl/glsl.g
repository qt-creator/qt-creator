---------------------------------------------------------------------------
--
-- This file is part of Qt Creator
--
-- Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
--
-- Contact: Nokia Corporation (qt-info@nokia.com)
--
-- Commercial Usage
--
-- Licensees holding valid Qt Commercial licenses may use this file in
-- accordance with the Qt Commercial License Agreement provided with the
-- Software or, alternatively, in accordance with the terms contained in
-- a written agreement between you and Nokia.
--
-- GNU Lesser General Public License Usage
--
-- Alternatively, this file may be used under the terms of the GNU Lesser
-- General Public License version 2.1 as published by the Free Software
-- Foundation and appearing in the file LICENSE.LGPL included in the
-- packaging of this file.  Please review the following information to
-- ensure the GNU Lesser General Public License version 2.1 requirements
-- will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
--
-- If you are unsure which license is appropriate for your use, please
-- contact the sales department at http://qt.nokia.com/contact.
---------------------------------------------------------------------------

--
-- todo:
--    spelling of XOR_OP and CARET

%decl glslparser.h
%impl glslparser.cpp
%parser GLSLParserTable
%token_prefix T_

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
%token XOR_OP "^"
%token TRUE "true"
%token FALSE "false"
%token PREPROC "preprocessor directive"
%token COMMENT "comment"
%token ERROR "error"

%start translation_unit

/:
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

#include "$header"
#include "glsllexer.h"
#include "glslast.h"
#include <vector>
#include <stack>

namespace GLSL {

class Parser: public $table
{
public:
    Parser(const char *source, unsigned size, int variant);
    ~Parser();

    bool parse();

private:
    inline int consumeToken() { return _index++; }
    inline const Token &tokenAt(int index) const { return _tokens.at(index); }
    inline int tokenKind(int index) const { return _tokens.at(index).kind; }
    void dump(AST *ast);

private:
    int _tos;
    int _index;
    std::vector<int> _stateStack;
    std::vector<int> _locationStack;
    std::vector<AST *> _astStack;
    std::vector<Token> _tokens;
};

} // end of namespace GLSL
:/

/.
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

#include "glslparser.h"
#include <iostream>

using namespace GLSL;

namespace GLSL {
void dumpAST(AST *);
void deleteAST(AST *ast);
}

Parser::Parser(const char *source, unsigned size, int variant)
    : _tos(-1), _index(0)
{
    _tokens.reserve(1024);

    _stateStack.resize(128);
    _locationStack.resize(128);
    _astStack.resize(128);

    _tokens.push_back(Token()); // invalid token

    std::stack<int> parenStack;
    std::stack<int> bracketStack;
    std::stack<int> braceStack;

    Lexer lexer(source, size);
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

    _index = 1;
}

Parser::~Parser()
{
}

void Parser::dump(AST *ast)
{
    dumpAST(ast);
}

bool Parser::parse()
{
    int action = 0;
    int yytoken = -1;
    int yyloc = -1;
    Operand *opd = 0;

    _tos = -1;

    do {
        if (yytoken == -1 && -TERMINAL_COUNT != action_index[action]) {
            yyloc = consumeToken();
            yytoken = tokenKind(yyloc);
            if (yytoken == T_IDENTIFIER && t_action(action, T_TYPE_NAME) != 0) {
                const Token &la = tokenAt(_index);

                if (la.is(T_IDENTIFIER)) {
                    yytoken = T_TYPE_NAME;
                } else if (la.is(T_LEFT_BRACKET) && la.matchingBrace != 0 &&
                           tokenAt(la.matchingBrace + 1).is(T_IDENTIFIER)) {
                    yytoken = T_TYPE_NAME;
                }
            }
            opd = new Operand(yyloc);
        }

        if (unsigned(++_tos) == _stateStack.size()) {
            _stateStack.resize(_tos * 2);
            _locationStack.resize(_tos * 2);
            _astStack.resize(_tos * 2);
        }

        _stateStack[_tos] = action;
        action = t_action(action, yytoken);
        if (action > 0) {
            if (action == ACCEPT_STATE) {
                --_tos;
                dump(_astStack[0]);
                deleteAST(_astStack[0]);
                return true;
            }
            _astStack[_tos] = opd;
            _locationStack[_tos] = yyloc;
            yytoken = -1;
        } else if (action < 0) {
            const int ruleno = -action - 1;
            const int N = rhs[ruleno];
            _tos -= N;
            if (N == 0)
                _astStack[_tos] = 0;
            else
                _astStack[_tos] = new Operator(ruleno, &_astStack[_tos], &_astStack[_tos + N]);
            action = nt_action(_stateStack[_tos], lhs[ruleno] - TERMINAL_COUNT);
        }
    } while (action);

    fprintf(stderr, "unexpected token `%s' at line %d\n", yytoken != -1 ? spell[yytoken] : "",
        _tokens[yyloc].line + 1);

    return false;
}
./
variable_identifier ::= IDENTIFIER ;
primary_expression ::= NUMBER ;
primary_expression ::= TRUE ;
primary_expression ::= FALSE ;
primary_expression ::= variable_identifier ;
primary_expression ::= LEFT_PAREN expression RIGHT_PAREN ;
postfix_expression ::= primary_expression ;
postfix_expression ::= postfix_expression LEFT_BRACKET integer_expression RIGHT_BRACKET ;
postfix_expression ::= function_call ;
postfix_expression ::= postfix_expression DOT IDENTIFIER ;
postfix_expression ::= postfix_expression INC_OP ;
postfix_expression ::= postfix_expression DEC_OP ;
integer_expression ::= expression ;
function_call ::= function_call_or_method ;
function_call_or_method ::= function_call_generic ;
function_call_or_method ::= postfix_expression DOT function_call_generic ;
function_call_generic ::= function_call_header_with_parameters RIGHT_PAREN ;
function_call_generic ::= function_call_header_no_parameters RIGHT_PAREN ;
function_call_header_no_parameters ::= function_call_header VOID ;
function_call_header_no_parameters ::= function_call_header ;
function_call_header_with_parameters ::= function_call_header assignment_expression ;
function_call_header_with_parameters ::= function_call_header_with_parameters COMMA assignment_expression ;
function_call_header ::= function_identifier LEFT_PAREN ;
function_identifier ::= type_specifier ;
function_identifier ::= IDENTIFIER ;
unary_expression ::= postfix_expression ;
unary_expression ::= INC_OP unary_expression ;
unary_expression ::= DEC_OP unary_expression ;
unary_expression ::= unary_operator unary_expression ;
unary_operator ::= PLUS ;
unary_operator ::= DASH ;
unary_operator ::= BANG ;
unary_operator ::= TILDE ;
multiplicative_expression ::= unary_expression ;
multiplicative_expression ::= multiplicative_expression STAR unary_expression ;
multiplicative_expression ::= multiplicative_expression SLASH unary_expression ;
multiplicative_expression ::= multiplicative_expression PERCENT unary_expression ;
additive_expression ::= multiplicative_expression ;
additive_expression ::= additive_expression PLUS multiplicative_expression ;
additive_expression ::= additive_expression DASH multiplicative_expression ;
shift_expression ::= additive_expression ;
shift_expression ::= shift_expression LEFT_OP additive_expression ;
shift_expression ::= shift_expression RIGHT_OP additive_expression ;
relational_expression ::= shift_expression ;
relational_expression ::= relational_expression LEFT_ANGLE shift_expression ;
relational_expression ::= relational_expression RIGHT_ANGLE shift_expression ;
relational_expression ::= relational_expression LE_OP shift_expression ;
relational_expression ::= relational_expression GE_OP shift_expression ;
equality_expression ::= relational_expression ;
equality_expression ::= equality_expression EQ_OP relational_expression ;
equality_expression ::= equality_expression NE_OP relational_expression ;
and_expression ::= equality_expression ;
and_expression ::= and_expression AMPERSAND equality_expression ;
exclusive_or_expression ::= and_expression ;
exclusive_or_expression ::= exclusive_or_expression CARET and_expression ;
inclusive_or_expression ::= exclusive_or_expression ;
inclusive_or_expression ::= inclusive_or_expression VERTICAL_BAR exclusive_or_expression ;
logical_and_expression ::= inclusive_or_expression ;
logical_and_expression ::= logical_and_expression AND_OP inclusive_or_expression ;
logical_xor_expression ::= logical_and_expression ;
logical_xor_expression ::= logical_xor_expression XOR_OP logical_and_expression ;
logical_or_expression ::= logical_xor_expression ;
logical_or_expression ::= logical_or_expression OR_OP logical_xor_expression ;
conditional_expression ::= logical_or_expression ;
conditional_expression ::= logical_or_expression QUESTION expression COLON assignment_expression ;
assignment_expression ::= conditional_expression ;
assignment_expression ::= unary_expression assignment_operator assignment_expression ;
assignment_operator ::= EQUAL ;
assignment_operator ::= MUL_ASSIGN ;
assignment_operator ::= DIV_ASSIGN ;
assignment_operator ::= MOD_ASSIGN ;
assignment_operator ::= ADD_ASSIGN ;
assignment_operator ::= SUB_ASSIGN ;
assignment_operator ::= LEFT_ASSIGN ;
assignment_operator ::= RIGHT_ASSIGN ;
assignment_operator ::= AND_ASSIGN ;
assignment_operator ::= XOR_ASSIGN ;
assignment_operator ::= OR_ASSIGN ;
expression ::= assignment_expression ;
expression ::= expression COMMA assignment_expression ;
constant_expression ::= conditional_expression ;
declaration ::= function_prototype SEMICOLON ;
declaration ::= init_declarator_list SEMICOLON ;
declaration ::= PRECISION precision_qualifier type_specifier_no_prec SEMICOLON ;
declaration ::= type_qualifier IDENTIFIER LEFT_BRACE struct_declaration_list RIGHT_BRACE SEMICOLON ;
declaration ::= type_qualifier IDENTIFIER LEFT_BRACE struct_declaration_list RIGHT_BRACE IDENTIFIER SEMICOLON ;
declaration ::= type_qualifier IDENTIFIER LEFT_BRACE struct_declaration_list RIGHT_BRACE IDENTIFIER LEFT_BRACKET RIGHT_BRACKET SEMICOLON ;
declaration ::= type_qualifier IDENTIFIER LEFT_BRACE struct_declaration_list RIGHT_BRACE IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET SEMICOLON ;
declaration ::= type_qualifier SEMICOLON ;
function_prototype ::= function_declarator RIGHT_PAREN ;
function_declarator ::= function_header ;
function_declarator ::= function_header_with_parameters ;
function_header_with_parameters ::= function_header parameter_declaration ;
function_header_with_parameters ::= function_header_with_parameters COMMA parameter_declaration ;
function_header ::= fully_specified_type IDENTIFIER LEFT_PAREN ;
parameter_declarator ::= type_specifier IDENTIFIER ;
parameter_declarator ::= type_specifier IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET ;
parameter_declaration ::= parameter_type_qualifier parameter_qualifier parameter_declarator ;
parameter_declaration ::= parameter_qualifier parameter_declarator ;
parameter_declaration ::= parameter_type_qualifier parameter_qualifier parameter_type_specifier ;
parameter_declaration ::= parameter_qualifier parameter_type_specifier ;
parameter_qualifier ::= ;
parameter_qualifier ::= IN ;
parameter_qualifier ::= OUT ;
parameter_qualifier ::= INOUT ;
parameter_type_specifier ::= type_specifier ;
init_declarator_list ::= single_declaration ;
init_declarator_list ::= init_declarator_list COMMA IDENTIFIER ;
init_declarator_list ::= init_declarator_list COMMA IDENTIFIER LEFT_BRACKET RIGHT_BRACKET ;
init_declarator_list ::= init_declarator_list COMMA IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET ;
init_declarator_list ::= init_declarator_list COMMA IDENTIFIER LEFT_BRACKET RIGHT_BRACKET EQUAL initializer ;
init_declarator_list ::= init_declarator_list COMMA IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET EQUAL initializer ;
init_declarator_list ::= init_declarator_list COMMA IDENTIFIER EQUAL initializer ;
single_declaration ::= fully_specified_type ;
single_declaration ::= fully_specified_type IDENTIFIER ;
single_declaration ::= fully_specified_type IDENTIFIER LEFT_BRACKET RIGHT_BRACKET ;
single_declaration ::= fully_specified_type IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET ;
single_declaration ::= fully_specified_type IDENTIFIER LEFT_BRACKET RIGHT_BRACKET EQUAL initializer ;
single_declaration ::= fully_specified_type IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET EQUAL initializer ;
single_declaration ::= fully_specified_type IDENTIFIER EQUAL initializer ;
single_declaration ::= INVARIANT IDENTIFIER ;
fully_specified_type ::= type_specifier ;
fully_specified_type ::= type_qualifier type_specifier ;
invariant_qualifier ::= INVARIANT ;
interpolation_qualifier ::= SMOOTH ;
interpolation_qualifier ::= FLAT ;
interpolation_qualifier ::= NOPERSPECTIVE ;
layout_qualifier ::= LAYOUT LEFT_PAREN layout_qualifier_id_list RIGHT_PAREN ;
layout_qualifier_id_list ::= layout_qualifier_id ;
layout_qualifier_id_list ::= layout_qualifier_id_list COMMA layout_qualifier_id ;
layout_qualifier_id ::= IDENTIFIER ;
layout_qualifier_id ::= IDENTIFIER EQUAL NUMBER ;
parameter_type_qualifier ::= CONST ;
type_qualifier ::= storage_qualifier ;
type_qualifier ::= layout_qualifier ;
type_qualifier ::= layout_qualifier storage_qualifier ;
type_qualifier ::= interpolation_qualifier storage_qualifier ;
type_qualifier ::= interpolation_qualifier ;
type_qualifier ::= invariant_qualifier storage_qualifier ;
type_qualifier ::= invariant_qualifier interpolation_qualifier storage_qualifier ;
type_qualifier ::= INVARIANT ;
storage_qualifier ::= CONST ;
storage_qualifier ::= ATTRIBUTE ;
storage_qualifier ::= VARYING ;
storage_qualifier ::= CENTROID VARYING ;
storage_qualifier ::= IN ;
storage_qualifier ::= OUT ;
storage_qualifier ::= CENTROID IN ;
storage_qualifier ::= CENTROID OUT ;
storage_qualifier ::= PATCH IN ;
storage_qualifier ::= PATCH OUT ;
storage_qualifier ::= SAMPLE IN ;
storage_qualifier ::= SAMPLE OUT ;
storage_qualifier ::= UNIFORM ;
type_specifier ::= type_specifier_no_prec ;
type_specifier ::= precision_qualifier type_specifier_no_prec ;
type_specifier_no_prec ::= type_specifier_nonarray ;
type_specifier_no_prec ::= type_specifier_nonarray LEFT_BRACKET RIGHT_BRACKET ;
type_specifier_no_prec ::= type_specifier_nonarray LEFT_BRACKET constant_expression RIGHT_BRACKET ;
type_specifier_nonarray ::= VOID ;
type_specifier_nonarray ::= FLOAT ;
type_specifier_nonarray ::= DOUBLE ;
type_specifier_nonarray ::= INT ;
type_specifier_nonarray ::= UINT ;
type_specifier_nonarray ::= BOOL ;
type_specifier_nonarray ::= VEC2 ;
type_specifier_nonarray ::= VEC3 ;
type_specifier_nonarray ::= VEC4 ;
type_specifier_nonarray ::= DVEC2 ;
type_specifier_nonarray ::= DVEC3 ;
type_specifier_nonarray ::= DVEC4 ;
type_specifier_nonarray ::= BVEC2 ;
type_specifier_nonarray ::= BVEC3 ;
type_specifier_nonarray ::= BVEC4 ;
type_specifier_nonarray ::= IVEC2 ;
type_specifier_nonarray ::= IVEC3 ;
type_specifier_nonarray ::= IVEC4 ;
type_specifier_nonarray ::= UVEC2 ;
type_specifier_nonarray ::= UVEC3 ;
type_specifier_nonarray ::= UVEC4 ;
type_specifier_nonarray ::= MAT2 ;
type_specifier_nonarray ::= MAT3 ;
type_specifier_nonarray ::= MAT4 ;
type_specifier_nonarray ::= MAT2X2 ;
type_specifier_nonarray ::= MAT2X3 ;
type_specifier_nonarray ::= MAT2X4 ;
type_specifier_nonarray ::= MAT3X2 ;
type_specifier_nonarray ::= MAT3X3 ;
type_specifier_nonarray ::= MAT3X4 ;
type_specifier_nonarray ::= MAT4X2 ;
type_specifier_nonarray ::= MAT4X3 ;
type_specifier_nonarray ::= MAT4X4 ;
type_specifier_nonarray ::= DMAT2 ;
type_specifier_nonarray ::= DMAT3 ;
type_specifier_nonarray ::= DMAT4 ;
type_specifier_nonarray ::= DMAT2X2 ;
type_specifier_nonarray ::= DMAT2X3 ;
type_specifier_nonarray ::= DMAT2X4 ;
type_specifier_nonarray ::= DMAT3X2 ;
type_specifier_nonarray ::= DMAT3X3 ;
type_specifier_nonarray ::= DMAT3X4 ;
type_specifier_nonarray ::= DMAT4X2 ;
type_specifier_nonarray ::= DMAT4X3 ;
type_specifier_nonarray ::= DMAT4X4 ;
type_specifier_nonarray ::= SAMPLER1D ;
type_specifier_nonarray ::= SAMPLER2D ;
type_specifier_nonarray ::= SAMPLER3D ;
type_specifier_nonarray ::= SAMPLERCUBE ;
type_specifier_nonarray ::= SAMPLER1DSHADOW ;
type_specifier_nonarray ::= SAMPLER2DSHADOW ;
type_specifier_nonarray ::= SAMPLERCUBESHADOW ;
type_specifier_nonarray ::= SAMPLER1DARRAY ;
type_specifier_nonarray ::= SAMPLER2DARRAY ;
type_specifier_nonarray ::= SAMPLER1DARRAYSHADOW ;
type_specifier_nonarray ::= SAMPLER2DARRAYSHADOW ;
type_specifier_nonarray ::= SAMPLERCUBEARRAY ;
type_specifier_nonarray ::= SAMPLERCUBEARRAYSHADOW ;
type_specifier_nonarray ::= ISAMPLER1D ;
type_specifier_nonarray ::= ISAMPLER2D ;
type_specifier_nonarray ::= ISAMPLER3D ;
type_specifier_nonarray ::= ISAMPLERCUBE ;
type_specifier_nonarray ::= ISAMPLER1DARRAY ;
type_specifier_nonarray ::= ISAMPLER2DARRAY ;
type_specifier_nonarray ::= ISAMPLERCUBEARRAY ;
type_specifier_nonarray ::= USAMPLER1D ;
type_specifier_nonarray ::= USAMPLER2D ;
type_specifier_nonarray ::= USAMPLER3D ;
type_specifier_nonarray ::= USAMPLERCUBE ;
type_specifier_nonarray ::= USAMPLER1DARRAY ;
type_specifier_nonarray ::= USAMPLER2DARRAY ;
type_specifier_nonarray ::= USAMPLERCUBEARRAY ;
type_specifier_nonarray ::= SAMPLER2DRECT ;
type_specifier_nonarray ::= SAMPLER2DRECTSHADOW ;
type_specifier_nonarray ::= ISAMPLER2DRECT ;
type_specifier_nonarray ::= USAMPLER2DRECT ;
type_specifier_nonarray ::= SAMPLERBUFFER ;
type_specifier_nonarray ::= ISAMPLERBUFFER ;
type_specifier_nonarray ::= USAMPLERBUFFER ;
type_specifier_nonarray ::= SAMPLER2DMS ;
type_specifier_nonarray ::= ISAMPLER2DMS ;
type_specifier_nonarray ::= USAMPLER2DMS ;
type_specifier_nonarray ::= SAMPLER2DMSARRAY ;
type_specifier_nonarray ::= ISAMPLER2DMSARRAY ;
type_specifier_nonarray ::= USAMPLER2DMSARRAY ;
type_specifier_nonarray ::= struct_specifier ;
type_specifier_nonarray ::= TYPE_NAME ;
precision_qualifier ::= HIGHP ;
precision_qualifier ::= MEDIUMP ;
precision_qualifier ::= LOWP ;
struct_specifier ::= STRUCT IDENTIFIER LEFT_BRACE struct_declaration_list RIGHT_BRACE ;
struct_specifier ::= STRUCT LEFT_BRACE struct_declaration_list RIGHT_BRACE ;
struct_declaration_list ::= struct_declaration ;
struct_declaration_list ::= struct_declaration_list struct_declaration ;
struct_declaration ::= type_specifier struct_declarator_list SEMICOLON ;
struct_declaration ::= type_qualifier type_specifier struct_declarator_list SEMICOLON ;
struct_declarator_list ::= struct_declarator ;
struct_declarator_list ::= struct_declarator_list COMMA struct_declarator ;
struct_declarator ::= IDENTIFIER ;
struct_declarator ::= IDENTIFIER LEFT_BRACKET RIGHT_BRACKET ;
struct_declarator ::= IDENTIFIER LEFT_BRACKET constant_expression RIGHT_BRACKET ;
initializer ::= assignment_expression ;
declaration_statement ::= declaration ;
statement ::= compound_statement ;
statement ::= simple_statement ;
simple_statement ::= declaration_statement ;
simple_statement ::= expression_statement ;
simple_statement ::= selection_statement ;
simple_statement ::= switch_statement ;
simple_statement ::= case_label ;
simple_statement ::= iteration_statement ;
simple_statement ::= jump_statement ;
compound_statement ::= LEFT_BRACE RIGHT_BRACE ;
compound_statement ::= LEFT_BRACE statement_list RIGHT_BRACE ;
statement_no_new_scope ::= compound_statement_no_new_scope ;
statement_no_new_scope ::= simple_statement ;
compound_statement_no_new_scope ::= LEFT_BRACE RIGHT_BRACE ;
compound_statement_no_new_scope ::= LEFT_BRACE statement_list RIGHT_BRACE ;
statement_list ::= statement ;
statement_list ::= statement_list statement ;
expression_statement ::= SEMICOLON ;
expression_statement ::= expression SEMICOLON ;
selection_statement ::= IF LEFT_PAREN expression RIGHT_PAREN selection_rest_statement ;
selection_rest_statement ::= statement ELSE statement ;
selection_rest_statement ::= statement ;
condition ::= expression ;
condition ::= fully_specified_type IDENTIFIER EQUAL initializer ;
switch_statement ::= SWITCH LEFT_PAREN expression RIGHT_PAREN LEFT_BRACE switch_statement_list RIGHT_BRACE ;
switch_statement_list ::= ;
switch_statement_list ::= statement_list ;
case_label ::= CASE expression COLON ;
case_label ::= DEFAULT COLON ;
iteration_statement ::= WHILE LEFT_PAREN condition RIGHT_PAREN statement_no_new_scope ;
iteration_statement ::= DO statement WHILE LEFT_PAREN expression RIGHT_PAREN SEMICOLON ;
iteration_statement ::= FOR LEFT_PAREN for_init_statement for_rest_statement RIGHT_PAREN statement_no_new_scope ;
for_init_statement ::= expression_statement ;
for_init_statement ::= declaration_statement ;
conditionopt ::= ;
conditionopt ::= condition ;
for_rest_statement ::= conditionopt SEMICOLON ;
for_rest_statement ::= conditionopt SEMICOLON expression ;
jump_statement ::= CONTINUE SEMICOLON ;
jump_statement ::= BREAK SEMICOLON ;
jump_statement ::= RETURN SEMICOLON ;
jump_statement ::= RETURN expression SEMICOLON ;
jump_statement ::= DISCARD SEMICOLON ;
translation_unit ::= external_declaration ;
translation_unit ::= translation_unit external_declaration ;
external_declaration ::= function_definition ;
external_declaration ::= declaration ;
external_declaration ::= SEMICOLON ;
function_definition ::= function_prototype compound_statement_no_new_scope ;
