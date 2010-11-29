
#line 392 "./glsl.g"

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
#include "glslengine.h"
#include <iostream>
#include <cstdio>
#include <cassert>

using namespace GLSL;

Parser::Parser(Engine *engine, const char *source, unsigned size, int variant)
    : _engine(engine), _tos(-1), _index(0), yyloc(-1)
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

    _index = 1;
}

Parser::~Parser()
{
}

TranslationUnitAST *Parser::parse()
{
    int action = 0;
    int yytoken = -1;
    yyloc = -1;
    void *yyval = 0; // value of the current token.

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
            yyval = _tokens.at(yyloc).ptr;
        }

        if (unsigned(++_tos) == _stateStack.size()) {
            _stateStack.resize(_tos * 2);
            _locationStack.resize(_tos * 2);
            _symStack.resize(_tos * 2);
        }

        _stateStack[_tos] = action;
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
        }
    } while (action);

    const int line = _tokens[yyloc].line + 1;
    QString message = QLatin1String("Syntax error");
    if (yytoken != -1) {
        const QLatin1String s(yytoken != -1 ? spell[yytoken] : "");
        message = QString("Unexpected token `%1'").arg(s);
    }

    error(line, message);

//    fprintf(stderr, "unexpected token `%s' at line %d\n", yytoken != -1 ? spell[yytoken] : "",
//        _tokens[yyloc].line + 1);

    return 0;
}

#line 563 "./glsl.g"

void Parser::reduce(int ruleno)
{
switch(ruleno) {

#line 572 "./glsl.g"

case 0: {
    ast(1) = makeAstNode<IdentifierExpressionAST>(string(1));
}   break;

#line 579 "./glsl.g"

case 1: {
    ast(1) = makeAstNode<LiteralExpressionAST>(string(1));
}   break;

#line 586 "./glsl.g"

case 2: {
    ast(1) = makeAstNode<LiteralExpressionAST>(_engine->identifier("true", 4));
}   break;

#line 593 "./glsl.g"

case 3: {
    ast(1) = makeAstNode<LiteralExpressionAST>(_engine->identifier("false", 5));
}   break;

#line 600 "./glsl.g"

case 4: {
    // nothing to do.
}   break;

#line 607 "./glsl.g"

case 5: {
    ast(1) = ast(2);
}   break;

#line 614 "./glsl.g"

case 6: {
    // nothing to do.
}   break;

#line 621 "./glsl.g"

case 7: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ArrayAccess, expression(1), expression(3));
}   break;

#line 628 "./glsl.g"

case 8: {
    // nothing to do.
}   break;

#line 635 "./glsl.g"

case 9: {
    ast(1) = makeAstNode<MemberAccessExpressionAST>(expression(1), string(3));
}   break;

#line 642 "./glsl.g"

case 10: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PostIncrement, expression(1));
}   break;

#line 649 "./glsl.g"

case 11: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PostDecrement, expression(1));
}   break;

#line 656 "./glsl.g"

case 12: {
    // nothing to do.
}   break;

#line 663 "./glsl.g"

case 13: {
    // nothing to do.
}   break;

#line 670 "./glsl.g"

case 14: {
    ast(1) = makeAstNode<FunctionCallExpressionAST>
        (sym(1).function.id, sym(1).function.arguments);
}   break;

#line 678 "./glsl.g"

case 15: {
    ast(1) = makeAstNode<FunctionCallExpressionAST>
        (expression(1), sym(3).function.id, sym(3).function.arguments);
}   break;

#line 686 "./glsl.g"

case 16: {
    // nothing to do.
}   break;

#line 693 "./glsl.g"

case 17: {
    // nothing to do.
}   break;

#line 700 "./glsl.g"

case 18: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments = 0;
}   break;

#line 708 "./glsl.g"

case 19: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments = 0;
}   break;

#line 716 "./glsl.g"

case 20: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments =
        makeAstNode< List<ExpressionAST *> >(expression(2));
}   break;

#line 725 "./glsl.g"

case 21: {
    sym(1).function.arguments =
        makeAstNode< List<ExpressionAST *> >
            (sym(1).function.arguments, expression(3));
}   break;

#line 734 "./glsl.g"

case 22: {
    // nothing to do.
}   break;

#line 741 "./glsl.g"

case 23: {
    ast(1) = makeAstNode<FunctionIdentifierAST>(type(1));
}   break;

#line 748 "./glsl.g"

case 24: {
    ast(1) = makeAstNode<FunctionIdentifierAST>(string(1));
}   break;

#line 755 "./glsl.g"

case 25: {
    // nothing to do.
}   break;

#line 762 "./glsl.g"

case 26: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PreIncrement, expression(2));
}   break;

#line 769 "./glsl.g"

case 27: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PreDecrement, expression(2));
}   break;

#line 776 "./glsl.g"

case 28: {
    ast(1) = makeAstNode<UnaryExpressionAST>(sym(1).kind, expression(2));
}   break;

#line 783 "./glsl.g"

case 29: {
    sym(1).kind = AST::Kind_UnaryPlus;
}   break;

#line 790 "./glsl.g"

case 30: {
    sym(1).kind = AST::Kind_UnaryMinus;
}   break;

#line 797 "./glsl.g"

case 31: {
    sym(1).kind = AST::Kind_LogicalNot;
}   break;

#line 804 "./glsl.g"

case 32: {
    sym(1).kind = AST::Kind_BitwiseNot;
}   break;

#line 811 "./glsl.g"

case 33: {
    // nothing to do.
}   break;

#line 818 "./glsl.g"

case 34: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Multiply, expression(1), expression(3));
}   break;

#line 825 "./glsl.g"

case 35: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Divide, expression(1), expression(3));
}   break;

#line 832 "./glsl.g"

case 36: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Modulus, expression(1), expression(3));
}   break;

#line 839 "./glsl.g"

case 37: {
    // nothing to do.
}   break;

#line 846 "./glsl.g"

case 38: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Plus, expression(1), expression(3));
}   break;

#line 853 "./glsl.g"

case 39: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Minus, expression(1), expression(3));
}   break;

#line 860 "./glsl.g"

case 40: {
    // nothing to do.
}   break;

#line 867 "./glsl.g"

case 41: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ShiftLeft, expression(1), expression(3));
}   break;

#line 874 "./glsl.g"

case 42: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ShiftRight, expression(1), expression(3));
}   break;

#line 881 "./glsl.g"

case 43: {
    // nothing to do.
}   break;

#line 888 "./glsl.g"

case 44: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LessThan, expression(1), expression(3));
}   break;

#line 895 "./glsl.g"

case 45: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_GreaterThan, expression(1), expression(3));
}   break;

#line 902 "./glsl.g"

case 46: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LessEqual, expression(1), expression(3));
}   break;

#line 909 "./glsl.g"

case 47: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_GreaterEqual, expression(1), expression(3));
}   break;

#line 916 "./glsl.g"

case 48: {
    // nothing to do.
}   break;

#line 923 "./glsl.g"

case 49: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Equal, expression(1), expression(3));
}   break;

#line 930 "./glsl.g"

case 50: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_NotEqual, expression(1), expression(3));
}   break;

#line 937 "./glsl.g"

case 51: {
    // nothing to do.
}   break;

#line 944 "./glsl.g"

case 52: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseAnd, expression(1), expression(3));
}   break;

#line 951 "./glsl.g"

case 53: {
    // nothing to do.
}   break;

#line 958 "./glsl.g"

case 54: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseXor, expression(1), expression(3));
}   break;

#line 965 "./glsl.g"

case 55: {
    // nothing to do.
}   break;

#line 972 "./glsl.g"

case 56: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseOr, expression(1), expression(3));
}   break;

#line 979 "./glsl.g"

case 57: {
    // nothing to do.
}   break;

#line 986 "./glsl.g"

case 58: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalAnd, expression(1), expression(3));
}   break;

#line 993 "./glsl.g"

case 59: {
    // nothing to do.
}   break;

#line 1000 "./glsl.g"

case 60: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalXor, expression(1), expression(3));
}   break;

#line 1007 "./glsl.g"

case 61: {
    // nothing to do.
}   break;

#line 1014 "./glsl.g"

case 62: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalOr, expression(1), expression(3));
}   break;

#line 1021 "./glsl.g"

case 63: {
    // nothing to do.
}   break;

#line 1028 "./glsl.g"

case 64: {
    ast(1) = makeAstNode<TernaryExpressionAST>(AST::Kind_Conditional, expression(1), expression(3), expression(5));
}   break;

#line 1035 "./glsl.g"

case 65: {
    // nothing to do.
}   break;

#line 1042 "./glsl.g"

case 66: {
    ast(1) = makeAstNode<AssignmentExpressionAST>(sym(2).kind, expression(1), expression(3));
}   break;

#line 1049 "./glsl.g"

case 67: {
    sym(1).kind = AST::Kind_Assign;
}   break;

#line 1056 "./glsl.g"

case 68: {
    sym(1).kind = AST::Kind_AssignMultiply;
}   break;

#line 1063 "./glsl.g"

case 69: {
    sym(1).kind = AST::Kind_AssignDivide;
}   break;

#line 1070 "./glsl.g"

case 70: {
    sym(1).kind = AST::Kind_AssignModulus;
}   break;

#line 1077 "./glsl.g"

case 71: {
    sym(1).kind = AST::Kind_AssignPlus;
}   break;

#line 1084 "./glsl.g"

case 72: {
    sym(1).kind = AST::Kind_AssignMinus;
}   break;

#line 1091 "./glsl.g"

case 73: {
    sym(1).kind = AST::Kind_AssignShiftLeft;
}   break;

#line 1098 "./glsl.g"

case 74: {
    sym(1).kind = AST::Kind_AssignShiftRight;
}   break;

#line 1105 "./glsl.g"

case 75: {
    sym(1).kind = AST::Kind_AssignAnd;
}   break;

#line 1112 "./glsl.g"

case 76: {
    sym(1).kind = AST::Kind_AssignXor;
}   break;

#line 1119 "./glsl.g"

case 77: {
    sym(1).kind = AST::Kind_AssignOr;
}   break;

#line 1126 "./glsl.g"

case 78: {
    // nothing to do.
}   break;

#line 1133 "./glsl.g"

case 79: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Comma, expression(1), expression(3));
}   break;

#line 1140 "./glsl.g"

case 80: {
    // nothing to do.
}   break;

#line 1147 "./glsl.g"

case 81: {
    // nothing to do.
}   break;

#line 1154 "./glsl.g"

case 82: {
    ast(1) = makeAstNode<InitDeclarationAST>(sym(1).declaration_list);
}   break;

#line 1161 "./glsl.g"

case 83: {
    ast(1) = makeAstNode<PrecisionDeclarationAST>(sym(2).precision, type(3));
}   break;

#line 1168 "./glsl.g"

case 84: {
    if (sym(1).type_qualifier.qualifier != QualifiedTypeAST::Struct) {
        // TODO: issue an error if the qualifier is not "struct".
    }
    TypeAST *type = makeAstNode<StructTypeAST>(string(2), sym(4).field_list);
    ast(1) = makeAstNode<TypeDeclarationAST>(type);
}   break;

#line 1179 "./glsl.g"

case 85: {
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

#line 1198 "./glsl.g"

case 86: {
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

#line 1218 "./glsl.g"

case 87: {
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

#line 1238 "./glsl.g"

case 88: {
    TypeAST *type = makeAstNode<QualifiedTypeAST>
        (sym(1).type_qualifier.qualifier, (TypeAST *)0,
         sym(1).type_qualifier.layout_list);
    ast(1) = makeAstNode<TypeDeclarationAST>(type);
}   break;

#line 1248 "./glsl.g"

case 89: {
    function(1)->finishParams();
}   break;

#line 1255 "./glsl.g"

case 90: {
    // nothing to do.
}   break;

#line 1262 "./glsl.g"

case 91: {
    // nothing to do.
}   break;

#line 1269 "./glsl.g"

case 92: {
    function(1)->params = makeAstNode< List<ParameterDeclarationAST *> >
        (sym(2).param_declaration);
}   break;

#line 1277 "./glsl.g"

case 93: {
    function(1)->params = makeAstNode< List<ParameterDeclarationAST *> >
        (function(1)->params, sym(3).param_declaration);
}   break;

#line 1285 "./glsl.g"

case 94: {
    function(1) = makeAstNode<FunctionDeclarationAST>(type(1), string(2));
}   break;

#line 1292 "./glsl.g"

case 95: {
    sym(1).param_declarator.type = type(1);
    sym(1).param_declarator.name = string(2);
}   break;

#line 1300 "./glsl.g"

case 96: {
    sym(1).param_declarator.type = makeAstNode<ArrayTypeAST>(type(1), expression(4));
    sym(1).param_declarator.name = string(2);
}   break;

#line 1308 "./glsl.g"

case 97: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (makeAstNode<QualifiedTypeAST>
            (sym(1).qualifier, sym(3).param_declarator.type,
             (List<LayoutQualifier *> *)0),
         ParameterDeclarationAST::Qualifier(sym(2).qualifier),
         sym(3).param_declarator.name);
}   break;

#line 1320 "./glsl.g"

case 98: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (sym(2).param_declarator.type,
         ParameterDeclarationAST::Qualifier(sym(1).qualifier),
         sym(2).param_declarator.name);
}   break;

#line 1330 "./glsl.g"

case 99: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (makeAstNode<QualifiedTypeAST>
            (sym(1).qualifier, type(3), (List<LayoutQualifier *> *)0),
         ParameterDeclarationAST::Qualifier(sym(2).qualifier),
         (const QString *)0);
}   break;

#line 1341 "./glsl.g"

case 100: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (type(2), ParameterDeclarationAST::Qualifier(sym(1).qualifier),
         (const QString *)0);
}   break;

#line 1350 "./glsl.g"

case 101: {
    sym(1).qualifier = ParameterDeclarationAST::In;
}   break;

#line 1357 "./glsl.g"

case 102: {
    sym(1).qualifier = ParameterDeclarationAST::In;
}   break;

#line 1364 "./glsl.g"

case 103: {
    sym(1).qualifier = ParameterDeclarationAST::Out;
}   break;

#line 1371 "./glsl.g"

case 104: {
    sym(1).qualifier = ParameterDeclarationAST::InOut;
}   break;

#line 1378 "./glsl.g"

case 105: {
    // nothing to do.
}   break;

#line 1385 "./glsl.g"

case 106: {
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
        (sym(1).declaration);
}   break;

#line 1393 "./glsl.g"

case 107: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1403 "./glsl.g"

case 108: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1414 "./glsl.g"

case 109: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type, expression(5));
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1425 "./glsl.g"

case 110: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>
            (type, string(3), expression(7));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1437 "./glsl.g"

case 111: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type, expression(5));
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>
            (type, string(3), expression(8));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1449 "./glsl.g"

case 112: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>
            (type, string(3), expression(5));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1460 "./glsl.g"

case 113: {
    ast(1) = makeAstNode<TypeDeclarationAST>(type(1));
}   break;

#line 1467 "./glsl.g"

case 114: {
    ast(1) = makeAstNode<VariableDeclarationAST>(type(1), string(2));
}   break;

#line 1474 "./glsl.g"

case 115: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1)), string(2));
}   break;

#line 1482 "./glsl.g"

case 116: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1), expression(4)), string(2));
}   break;

#line 1490 "./glsl.g"

case 117: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1)), string(2), expression(6));
}   break;

#line 1498 "./glsl.g"

case 118: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1), expression(4)),
         string(2), expression(7));
}   break;

#line 1507 "./glsl.g"

case 119: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (type(1), string(2), expression(4));
}   break;

#line 1515 "./glsl.g"

case 120: {
    ast(1) = makeAstNode<InvariantDeclarationAST>(string(2));
}   break;

#line 1522 "./glsl.g"

case 121: {
    ast(1) = makeAstNode<QualifiedTypeAST>(0, type(1), (List<LayoutQualifier *> *)0);
}   break;

#line 1529 "./glsl.g"

case 122: {
    ast(1) = makeAstNode<QualifiedTypeAST>
        (sym(1).type_qualifier.qualifier, type(2),
         sym(1).type_qualifier.layout_list);
}   break;

#line 1538 "./glsl.g"

case 123: {
    sym(1).qualifier = QualifiedTypeAST::Invariant;
}   break;

#line 1545 "./glsl.g"

case 124: {
    sym(1).qualifier = QualifiedTypeAST::Smooth;
}   break;

#line 1552 "./glsl.g"

case 125: {
    sym(1).qualifier = QualifiedTypeAST::Flat;
}   break;

#line 1559 "./glsl.g"

case 126: {
    sym(1).qualifier = QualifiedTypeAST::NoPerspective;
}   break;

#line 1566 "./glsl.g"

case 127: {
    sym(1) = sym(3);
}   break;

#line 1573 "./glsl.g"

case 128: {
    sym(1).layout_list = makeAstNode< List<LayoutQualifier *> >(sym(1).layout);
}   break;

#line 1580 "./glsl.g"

case 129: {
    sym(1).layout_list = makeAstNode< List<LayoutQualifier *> >(sym(1).layout_list, sym(3).layout);
}   break;

#line 1587 "./glsl.g"

case 130: {
    sym(1).layout = makeAstNode<LayoutQualifier>(string(1), (const QString *)0);
}   break;

#line 1594 "./glsl.g"

case 131: {
    sym(1).layout = makeAstNode<LayoutQualifier>(string(1), string(3));
}   break;

#line 1601 "./glsl.g"

case 132: {
    sym(1).qualifier = QualifiedTypeAST::Const;
}   break;

#line 1608 "./glsl.g"

case 133: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1616 "./glsl.g"

case 134: {
    sym(1).type_qualifier.layout_list = sym(1).layout_list;
    sym(1).type_qualifier.qualifier = 0;
}   break;

#line 1624 "./glsl.g"

case 135: {
    sym(1).type_qualifier.layout_list = sym(1).layout_list;
    sym(1).type_qualifier.qualifier = sym(2).qualifier;
}   break;

#line 1632 "./glsl.g"

case 136: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1640 "./glsl.g"

case 137: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1648 "./glsl.g"

case 138: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1656 "./glsl.g"

case 139: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier | sym(3).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1664 "./glsl.g"

case 140: {
    sym(1).type_qualifier.qualifier = QualifiedTypeAST::Invariant;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1672 "./glsl.g"

case 141: {
    sym(1).qualifier = QualifiedTypeAST::Const;
}   break;

#line 1679 "./glsl.g"

case 142: {
    sym(1).qualifier = QualifiedTypeAST::Attribute;
}   break;

#line 1686 "./glsl.g"

case 143: {
    sym(1).qualifier = QualifiedTypeAST::Varying;
}   break;

#line 1693 "./glsl.g"

case 144: {
    sym(1).qualifier = QualifiedTypeAST::CentroidVarying;
}   break;

#line 1700 "./glsl.g"

case 145: {
    sym(1).qualifier = QualifiedTypeAST::In;
}   break;

#line 1707 "./glsl.g"

case 146: {
    sym(1).qualifier = QualifiedTypeAST::Out;
}   break;

#line 1714 "./glsl.g"

case 147: {
    sym(1).qualifier = QualifiedTypeAST::CentroidIn;
}   break;

#line 1721 "./glsl.g"

case 148: {
    sym(1).qualifier = QualifiedTypeAST::CentroidOut;
}   break;

#line 1728 "./glsl.g"

case 149: {
    sym(1).qualifier = QualifiedTypeAST::PatchIn;
}   break;

#line 1735 "./glsl.g"

case 150: {
    sym(1).qualifier = QualifiedTypeAST::PatchOut;
}   break;

#line 1742 "./glsl.g"

case 151: {
    sym(1).qualifier = QualifiedTypeAST::SampleIn;
}   break;

#line 1749 "./glsl.g"

case 152: {
    sym(1).qualifier = QualifiedTypeAST::SampleOut;
}   break;

#line 1756 "./glsl.g"

case 153: {
    sym(1).qualifier = QualifiedTypeAST::Uniform;
}   break;

#line 1763 "./glsl.g"

case 154: {
    // nothing to do.
}   break;

#line 1770 "./glsl.g"

case 155: {
    if (!type(2)->setPrecision(sym(1).precision)) {
        // TODO: issue an error about precision not allowed on this type.
    }
    ast(1) = type(2);
}   break;

#line 1780 "./glsl.g"

case 156: {
    // nothing to do.
}   break;

#line 1787 "./glsl.g"

case 157: {
    ast(1) = makeAstNode<ArrayTypeAST>(type(1));
}   break;

#line 1794 "./glsl.g"

case 158: {
    ast(1) = makeAstNode<ArrayTypeAST>(type(1), expression(3));
}   break;

#line 1801 "./glsl.g"

case 159: {
    ast(1) = makeBasicType(T_VOID);
}   break;

#line 1808 "./glsl.g"

case 160: {
    ast(1) = makeBasicType(T_FLOAT);
}   break;

#line 1815 "./glsl.g"

case 161: {
    ast(1) = makeBasicType(T_DOUBLE);
}   break;

#line 1822 "./glsl.g"

case 162: {
    ast(1) = makeBasicType(T_INT);
}   break;

#line 1829 "./glsl.g"

case 163: {
    ast(1) = makeBasicType(T_UINT);
}   break;

#line 1836 "./glsl.g"

case 164: {
    ast(1) = makeBasicType(T_BOOL);
}   break;

#line 1843 "./glsl.g"

case 165: {
    ast(1) = makeBasicType(T_VEC2);
}   break;

#line 1850 "./glsl.g"

case 166: {
    ast(1) = makeBasicType(T_VEC3);
}   break;

#line 1857 "./glsl.g"

case 167: {
    ast(1) = makeBasicType(T_VEC4);
}   break;

#line 1864 "./glsl.g"

case 168: {
    ast(1) = makeBasicType(T_DVEC2);
}   break;

#line 1871 "./glsl.g"

case 169: {
    ast(1) = makeBasicType(T_DVEC3);
}   break;

#line 1878 "./glsl.g"

case 170: {
    ast(1) = makeBasicType(T_DVEC4);
}   break;

#line 1885 "./glsl.g"

case 171: {
    ast(1) = makeBasicType(T_BVEC2);
}   break;

#line 1892 "./glsl.g"

case 172: {
    ast(1) = makeBasicType(T_BVEC3);
}   break;

#line 1899 "./glsl.g"

case 173: {
    ast(1) = makeBasicType(T_BVEC4);
}   break;

#line 1906 "./glsl.g"

case 174: {
    ast(1) = makeBasicType(T_IVEC2);
}   break;

#line 1913 "./glsl.g"

case 175: {
    ast(1) = makeBasicType(T_IVEC3);
}   break;

#line 1920 "./glsl.g"

case 176: {
    ast(1) = makeBasicType(T_IVEC4);
}   break;

#line 1927 "./glsl.g"

case 177: {
    ast(1) = makeBasicType(T_UVEC2);
}   break;

#line 1934 "./glsl.g"

case 178: {
    ast(1) = makeBasicType(T_UVEC3);
}   break;

#line 1941 "./glsl.g"

case 179: {
    ast(1) = makeBasicType(T_UVEC4);
}   break;

#line 1948 "./glsl.g"

case 180: {
    ast(1) = makeBasicType(T_MAT2);
}   break;

#line 1955 "./glsl.g"

case 181: {
    ast(1) = makeBasicType(T_MAT3);
}   break;

#line 1962 "./glsl.g"

case 182: {
    ast(1) = makeBasicType(T_MAT4);
}   break;

#line 1969 "./glsl.g"

case 183: {
    ast(1) = makeBasicType(T_MAT2);
}   break;

#line 1976 "./glsl.g"

case 184: {
    ast(1) = makeBasicType(T_MAT2X3);
}   break;

#line 1983 "./glsl.g"

case 185: {
    ast(1) = makeBasicType(T_MAT2X4);
}   break;

#line 1990 "./glsl.g"

case 186: {
    ast(1) = makeBasicType(T_MAT3X2);
}   break;

#line 1997 "./glsl.g"

case 187: {
    ast(1) = makeBasicType(T_MAT3);
}   break;

#line 2004 "./glsl.g"

case 188: {
    ast(1) = makeBasicType(T_MAT3X4);
}   break;

#line 2011 "./glsl.g"

case 189: {
    ast(1) = makeBasicType(T_MAT4X2);
}   break;

#line 2018 "./glsl.g"

case 190: {
    ast(1) = makeBasicType(T_MAT4X3);
}   break;

#line 2025 "./glsl.g"

case 191: {
    ast(1) = makeBasicType(T_MAT4);
}   break;

#line 2032 "./glsl.g"

case 192: {
    ast(1) = makeBasicType(T_DMAT2);
}   break;

#line 2039 "./glsl.g"

case 193: {
    ast(1) = makeBasicType(T_DMAT3);
}   break;

#line 2046 "./glsl.g"

case 194: {
    ast(1) = makeBasicType(T_DMAT4);
}   break;

#line 2053 "./glsl.g"

case 195: {
    ast(1) = makeBasicType(T_DMAT2);
}   break;

#line 2060 "./glsl.g"

case 196: {
    ast(1) = makeBasicType(T_DMAT2X3);
}   break;

#line 2067 "./glsl.g"

case 197: {
    ast(1) = makeBasicType(T_DMAT2X4);
}   break;

#line 2074 "./glsl.g"

case 198: {
    ast(1) = makeBasicType(T_DMAT3X2);
}   break;

#line 2081 "./glsl.g"

case 199: {
    ast(1) = makeBasicType(T_DMAT3);
}   break;

#line 2088 "./glsl.g"

case 200: {
    ast(1) = makeBasicType(T_DMAT3X4);
}   break;

#line 2095 "./glsl.g"

case 201: {
    ast(1) = makeBasicType(T_DMAT4X2);
}   break;

#line 2102 "./glsl.g"

case 202: {
    ast(1) = makeBasicType(T_DMAT4X3);
}   break;

#line 2109 "./glsl.g"

case 203: {
    ast(1) = makeBasicType(T_DMAT4);
}   break;

#line 2116 "./glsl.g"

case 204: {
    ast(1) = makeBasicType(T_SAMPLER1D);
}   break;

#line 2123 "./glsl.g"

case 205: {
    ast(1) = makeBasicType(T_SAMPLER2D);
}   break;

#line 2130 "./glsl.g"

case 206: {
    ast(1) = makeBasicType(T_SAMPLER3D);
}   break;

#line 2137 "./glsl.g"

case 207: {
    ast(1) = makeBasicType(T_SAMPLERCUBE);
}   break;

#line 2144 "./glsl.g"

case 208: {
    ast(1) = makeBasicType(T_SAMPLER1DSHADOW);
}   break;

#line 2151 "./glsl.g"

case 209: {
    ast(1) = makeBasicType(T_SAMPLER2DSHADOW);
}   break;

#line 2158 "./glsl.g"

case 210: {
    ast(1) = makeBasicType(T_SAMPLERCUBESHADOW);
}   break;

#line 2165 "./glsl.g"

case 211: {
    ast(1) = makeBasicType(T_SAMPLER1DARRAY);
}   break;

#line 2172 "./glsl.g"

case 212: {
    ast(1) = makeBasicType(T_SAMPLER2DARRAY);
}   break;

#line 2179 "./glsl.g"

case 213: {
    ast(1) = makeBasicType(T_SAMPLER1DARRAYSHADOW);
}   break;

#line 2186 "./glsl.g"

case 214: {
    ast(1) = makeBasicType(T_SAMPLER2DARRAYSHADOW);
}   break;

#line 2193 "./glsl.g"

case 215: {
    ast(1) = makeBasicType(T_SAMPLERCUBEARRAY);
}   break;

#line 2200 "./glsl.g"

case 216: {
    ast(1) = makeBasicType(T_SAMPLERCUBEARRAYSHADOW);
}   break;

#line 2207 "./glsl.g"

case 217: {
    ast(1) = makeBasicType(T_ISAMPLER1D);
}   break;

#line 2214 "./glsl.g"

case 218: {
    ast(1) = makeBasicType(T_ISAMPLER2D);
}   break;

#line 2221 "./glsl.g"

case 219: {
    ast(1) = makeBasicType(T_ISAMPLER3D);
}   break;

#line 2228 "./glsl.g"

case 220: {
    ast(1) = makeBasicType(T_ISAMPLERCUBE);
}   break;

#line 2235 "./glsl.g"

case 221: {
    ast(1) = makeBasicType(T_ISAMPLER1DARRAY);
}   break;

#line 2242 "./glsl.g"

case 222: {
    ast(1) = makeBasicType(T_ISAMPLER2DARRAY);
}   break;

#line 2249 "./glsl.g"

case 223: {
    ast(1) = makeBasicType(T_ISAMPLERCUBEARRAY);
}   break;

#line 2256 "./glsl.g"

case 224: {
    ast(1) = makeBasicType(T_USAMPLER1D);
}   break;

#line 2263 "./glsl.g"

case 225: {
    ast(1) = makeBasicType(T_USAMPLER2D);
}   break;

#line 2270 "./glsl.g"

case 226: {
    ast(1) = makeBasicType(T_USAMPLER3D);
}   break;

#line 2277 "./glsl.g"

case 227: {
    ast(1) = makeBasicType(T_USAMPLERCUBE);
}   break;

#line 2284 "./glsl.g"

case 228: {
    ast(1) = makeBasicType(T_USAMPLER1DARRAY);
}   break;

#line 2291 "./glsl.g"

case 229: {
    ast(1) = makeBasicType(T_USAMPLER2DARRAY);
}   break;

#line 2298 "./glsl.g"

case 230: {
    ast(1) = makeBasicType(T_USAMPLERCUBEARRAY);
}   break;

#line 2305 "./glsl.g"

case 231: {
    ast(1) = makeBasicType(T_SAMPLER2DRECT);
}   break;

#line 2312 "./glsl.g"

case 232: {
    ast(1) = makeBasicType(T_SAMPLER2DRECTSHADOW);
}   break;

#line 2319 "./glsl.g"

case 233: {
    ast(1) = makeBasicType(T_ISAMPLER2DRECT);
}   break;

#line 2326 "./glsl.g"

case 234: {
    ast(1) = makeBasicType(T_USAMPLER2DRECT);
}   break;

#line 2333 "./glsl.g"

case 235: {
    ast(1) = makeBasicType(T_SAMPLERBUFFER);
}   break;

#line 2340 "./glsl.g"

case 236: {
    ast(1) = makeBasicType(T_ISAMPLERBUFFER);
}   break;

#line 2347 "./glsl.g"

case 237: {
    ast(1) = makeBasicType(T_USAMPLERBUFFER);
}   break;

#line 2354 "./glsl.g"

case 238: {
    ast(1) = makeBasicType(T_SAMPLER2DMS);
}   break;

#line 2361 "./glsl.g"

case 239: {
    ast(1) = makeBasicType(T_ISAMPLER2DMS);
}   break;

#line 2368 "./glsl.g"

case 240: {
    ast(1) = makeBasicType(T_USAMPLER2DMS);
}   break;

#line 2375 "./glsl.g"

case 241: {
    ast(1) = makeBasicType(T_SAMPLER2DMSARRAY);
}   break;

#line 2382 "./glsl.g"

case 242: {
    ast(1) = makeBasicType(T_ISAMPLER2DMSARRAY);
}   break;

#line 2389 "./glsl.g"

case 243: {
    ast(1) = makeBasicType(T_USAMPLER2DMSARRAY);
}   break;

#line 2396 "./glsl.g"

case 244: {
    // nothing to do.
}   break;

#line 2403 "./glsl.g"

case 245: {
    ast(1) = makeAstNode<NamedTypeAST>(string(1));
}   break;

#line 2410 "./glsl.g"

case 246: {
    sym(1).precision = TypeAST::Highp;
}   break;

#line 2417 "./glsl.g"

case 247: {
    sym(1).precision = TypeAST::Mediump;
}   break;

#line 2424 "./glsl.g"

case 248: {
    sym(1).precision = TypeAST::Lowp;
}   break;

#line 2431 "./glsl.g"

case 249: {
    ast(1) = makeAstNode<StructTypeAST>(string(2), sym(4).field_list);
}   break;

#line 2438 "./glsl.g"

case 250: {
    ast(1) = makeAstNode<StructTypeAST>(sym(3).field_list);
}   break;

#line 2445 "./glsl.g"

case 251: {
    // nothing to do.
}   break;

#line 2452 "./glsl.g"

case 252: {
    sym(1).field_list = appendLists(sym(1).field_list, sym(2).field_list);
}   break;

#line 2459 "./glsl.g"

case 253: {
    sym(1).field_list = StructTypeAST::fixInnerTypes(type(1), sym(2).field_list);
}   break;

#line 2466 "./glsl.g"

case 254: {
    sym(1).field_list = StructTypeAST::fixInnerTypes
        (makeAstNode<QualifiedTypeAST>
            (sym(1).type_qualifier.qualifier, type(2),
             sym(1).type_qualifier.layout_list), sym(3).field_list);
}   break;

#line 2476 "./glsl.g"

case 255: {
    // nothing to do.
    sym(1).field_list = makeAstNode< List<StructTypeAST::Field *> >(sym(1).field);
}   break;

#line 2484 "./glsl.g"

case 256: {
    sym(1).field_list = makeAstNode< List<StructTypeAST::Field *> >(sym(1).field_list, sym(3).field);
}   break;

#line 2491 "./glsl.g"

case 257: {
    sym(1).field = makeAstNode<StructTypeAST::Field>(string(1));
}   break;

#line 2498 "./glsl.g"

case 258: {
    sym(1).field = makeAstNode<StructTypeAST::Field>
        (string(1), makeAstNode<ArrayTypeAST>((TypeAST *)0));
}   break;

#line 2506 "./glsl.g"

case 259: {
    sym(1).field = makeAstNode<StructTypeAST::Field>
        (string(1), makeAstNode<ArrayTypeAST>((TypeAST *)0, expression(3)));
}   break;

#line 2514 "./glsl.g"

case 260: {
    // nothing to do.
}   break;

#line 2521 "./glsl.g"

case 261: {
    ast(1) = makeAstNode<DeclarationStatementAST>(sym(1).declaration);
}   break;

#line 2528 "./glsl.g"

case 262: {
    // nothing to do.
}   break;

#line 2535 "./glsl.g"

case 263: {
    // nothing to do.
}   break;

#line 2542 "./glsl.g"

case 264: {
    // nothing to do.
}   break;

#line 2549 "./glsl.g"

case 265: {
    // nothing to do.
}   break;

#line 2556 "./glsl.g"

case 266: {
    // nothing to do.
}   break;

#line 2563 "./glsl.g"

case 267: {
    // nothing to do.
}   break;

#line 2570 "./glsl.g"

case 268: {
    // nothing to do.
}   break;

#line 2577 "./glsl.g"

case 269: {
    // nothing to do.
}   break;

#line 2584 "./glsl.g"

case 270: {
    // nothing to do.
}   break;

#line 2591 "./glsl.g"

case 271: {
    ast(1) = makeAstNode<CompoundStatementAST>();
}   break;

#line 2598 "./glsl.g"

case 272: {
    ast(1) = makeAstNode<CompoundStatementAST>(sym(2).statement_list);
}   break;

#line 2605 "./glsl.g"

case 273: {
    // nothing to do.
}   break;

#line 2612 "./glsl.g"

case 274: {
    // nothing to do.
}   break;

#line 2619 "./glsl.g"

case 275: {
    ast(1) = makeAstNode<CompoundStatementAST>();
}   break;

#line 2626 "./glsl.g"

case 276: {
    ast(1) = makeAstNode<CompoundStatementAST>(sym(2).statement_list);
}   break;

#line 2633 "./glsl.g"

case 277: {
    sym(1).statement_list = makeAstNode< List<StatementAST *> >(sym(1).statement);
}   break;

#line 2640 "./glsl.g"

case 278: {
    sym(1).statement_list = makeAstNode< List<StatementAST *> >(sym(1).statement_list, sym(2).statement);
}   break;

#line 2647 "./glsl.g"

case 279: {
    ast(1) = makeAstNode<CompoundStatementAST>();  // Empty statement
}   break;

#line 2654 "./glsl.g"

case 280: {
    ast(1) = makeAstNode<ExpressionStatementAST>(expression(1));
}   break;

#line 2661 "./glsl.g"

case 281: {
    ast(1) = makeAstNode<IfStatementAST>(expression(3), sym(5).ifstmt.thenClause, sym(5).ifstmt.elseClause);
}   break;

#line 2668 "./glsl.g"

case 282: {
    sym(1).ifstmt.thenClause = statement(1);
    sym(1).ifstmt.elseClause = statement(3);
}   break;

#line 2676 "./glsl.g"

case 283: {
    sym(1).ifstmt.thenClause = statement(1);
    sym(1).ifstmt.elseClause = 0;
}   break;

#line 2684 "./glsl.g"

case 284: {
    // nothing to do.
}   break;

#line 2691 "./glsl.g"

case 285: {
    ast(1) = makeAstNode<DeclarationExpressionAST>
        (type(1), string(2), expression(4));
}   break;

#line 2699 "./glsl.g"

case 286: {
    ast(1) = makeAstNode<SwitchStatementAST>(expression(3), statement(6));
}   break;

#line 2706 "./glsl.g"

case 287: {
    ast(1) = makeAstNode<CompoundStatementAST>();
}   break;

#line 2713 "./glsl.g"

case 288: {
    ast(1) = makeAstNode<CompoundStatementAST>(sym(1).statement_list);
}   break;

#line 2720 "./glsl.g"

case 289: {
    ast(1) = makeAstNode<CaseLabelStatementAST>(expression(2));
}   break;

#line 2727 "./glsl.g"

case 290: {
    ast(1) = makeAstNode<CaseLabelStatementAST>();
}   break;

#line 2734 "./glsl.g"

case 291: {
    ast(1) = makeAstNode<WhileStatementAST>(expression(3), statement(5));
}   break;

#line 2741 "./glsl.g"

case 292: {
    ast(1) = makeAstNode<DoStatementAST>(statement(2), expression(5));
}   break;

#line 2748 "./glsl.g"

case 293: {
    ast(1) = makeAstNode<ForStatementAST>(statement(3), sym(4).forstmt.condition, sym(4).forstmt.increment, statement(6));
}   break;

#line 2755 "./glsl.g"

case 294: {
    // nothing to do.
}   break;

#line 2762 "./glsl.g"

case 295: {
    // nothing to do.
}   break;

#line 2769 "./glsl.g"

case 296: {
    // nothing to do.
}   break;

#line 2776 "./glsl.g"

case 297: {
    // nothing to do.
}   break;

#line 2783 "./glsl.g"

case 298: {
    sym(1).forstmt.condition = expression(1);
    sym(1).forstmt.increment = 0;
}   break;

#line 2791 "./glsl.g"

case 299: {
    sym(1).forstmt.condition = expression(1);
    sym(1).forstmt.increment = expression(3);
}   break;

#line 2799 "./glsl.g"

case 300: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Continue);
}   break;

#line 2806 "./glsl.g"

case 301: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Break);
}   break;

#line 2813 "./glsl.g"

case 302: {
    ast(1) = makeAstNode<ReturnStatementAST>();
}   break;

#line 2820 "./glsl.g"

case 303: {
    ast(1) = makeAstNode<ReturnStatementAST>(expression(2));
}   break;

#line 2827 "./glsl.g"

case 304: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Discard);
}   break;

#line 2834 "./glsl.g"

case 305: {
    ast(1) = makeAstNode<TranslationUnitAST>(sym(1).declaration_list);
}   break;

#line 2841 "./glsl.g"

case 306: {
    if (sym(1).declaration) {
        sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration);
    } else {
        sym(1).declaration_list = 0;
    }
}   break;

#line 2853 "./glsl.g"

case 307: {
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

#line 2870 "./glsl.g"

case 308: {
    // nothing to do.
}   break;

#line 2877 "./glsl.g"

case 309: {
    // nothing to do.
}   break;

#line 2884 "./glsl.g"

case 310: {
    ast(1) = 0;
}   break;

#line 2891 "./glsl.g"

case 311: {
    function(1)->body = statement(2);
}   break;

#line 2898 "./glsl.g"

case 312: {
    ast(1) = 0;
}   break;

#line 2906 "./glsl.g"

} // end switch
} // end Parser::reduce()
