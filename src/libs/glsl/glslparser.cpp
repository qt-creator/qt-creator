
#line 399 "./glsl.g"

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

TranslationUnit *Parser::parse()
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

#line 570 "./glsl.g"

void Parser::reduce(int ruleno)
{
switch(ruleno) {

#line 579 "./glsl.g"

case 0: {
    ast(1) = makeAstNode<IdentifierExpression>(string(1));
}   break;

#line 586 "./glsl.g"

case 1: {
    ast(1) = makeAstNode<LiteralExpression>(string(1));
}   break;

#line 593 "./glsl.g"

case 2: {
    ast(1) = makeAstNode<LiteralExpression>(_engine->identifier("true", 4));
}   break;

#line 600 "./glsl.g"

case 3: {
    ast(1) = makeAstNode<LiteralExpression>(_engine->identifier("false", 5));
}   break;

#line 607 "./glsl.g"

case 4: {
    // nothing to do.
}   break;

#line 614 "./glsl.g"

case 5: {
    ast(1) = ast(2);
}   break;

#line 621 "./glsl.g"

case 6: {
    // nothing to do.
}   break;

#line 628 "./glsl.g"

case 7: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_ArrayAccess, expression(1), expression(3));
}   break;

#line 635 "./glsl.g"

case 8: {
    // nothing to do.
}   break;

#line 642 "./glsl.g"

case 9: {
    ast(1) = makeAstNode<MemberAccessExpression>(expression(1), string(3));
}   break;

#line 649 "./glsl.g"

case 10: {
    ast(1) = makeAstNode<UnaryExpression>(AST::Kind_PostIncrement, expression(1));
}   break;

#line 656 "./glsl.g"

case 11: {
    ast(1) = makeAstNode<UnaryExpression>(AST::Kind_PostDecrement, expression(1));
}   break;

#line 663 "./glsl.g"

case 12: {
    // nothing to do.
}   break;

#line 670 "./glsl.g"

case 13: {
    // nothing to do.
}   break;

#line 677 "./glsl.g"

case 14: {
    ast(1) = makeAstNode<FunctionCallExpression>
        (sym(1).function.id, sym(1).function.arguments);
}   break;

#line 685 "./glsl.g"

case 15: {
    ast(1) = makeAstNode<FunctionCallExpression>
        (expression(1), sym(3).function.id, sym(3).function.arguments);
}   break;

#line 693 "./glsl.g"

case 16: {
    // nothing to do.
}   break;

#line 700 "./glsl.g"

case 17: {
    // nothing to do.
}   break;

#line 707 "./glsl.g"

case 18: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments = 0;
}   break;

#line 715 "./glsl.g"

case 19: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments = 0;
}   break;

#line 723 "./glsl.g"

case 20: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments =
        makeAstNode< List<Expression *> >(expression(2));
}   break;

#line 732 "./glsl.g"

case 21: {
    sym(1).function.arguments =
        makeAstNode< List<Expression *> >
            (sym(1).function.arguments, expression(3));
}   break;

#line 741 "./glsl.g"

case 22: {
    // nothing to do.
}   break;

#line 748 "./glsl.g"

case 23: {
    ast(1) = makeAstNode<FunctionIdentifier>(type(1));
}   break;

#line 755 "./glsl.g"

case 24: {
    ast(1) = makeAstNode<FunctionIdentifier>(string(1));
}   break;

#line 762 "./glsl.g"

case 25: {
    // nothing to do.
}   break;

#line 769 "./glsl.g"

case 26: {
    ast(1) = makeAstNode<UnaryExpression>(AST::Kind_PreIncrement, expression(2));
}   break;

#line 776 "./glsl.g"

case 27: {
    ast(1) = makeAstNode<UnaryExpression>(AST::Kind_PreDecrement, expression(2));
}   break;

#line 783 "./glsl.g"

case 28: {
    ast(1) = makeAstNode<UnaryExpression>(sym(1).kind, expression(2));
}   break;

#line 790 "./glsl.g"

case 29: {
    sym(1).kind = AST::Kind_UnaryPlus;
}   break;

#line 797 "./glsl.g"

case 30: {
    sym(1).kind = AST::Kind_UnaryMinus;
}   break;

#line 804 "./glsl.g"

case 31: {
    sym(1).kind = AST::Kind_LogicalNot;
}   break;

#line 811 "./glsl.g"

case 32: {
    sym(1).kind = AST::Kind_BitwiseNot;
}   break;

#line 818 "./glsl.g"

case 33: {
    // nothing to do.
}   break;

#line 825 "./glsl.g"

case 34: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_Multiply, expression(1), expression(3));
}   break;

#line 832 "./glsl.g"

case 35: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_Divide, expression(1), expression(3));
}   break;

#line 839 "./glsl.g"

case 36: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_Modulus, expression(1), expression(3));
}   break;

#line 846 "./glsl.g"

case 37: {
    // nothing to do.
}   break;

#line 853 "./glsl.g"

case 38: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_Plus, expression(1), expression(3));
}   break;

#line 860 "./glsl.g"

case 39: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_Minus, expression(1), expression(3));
}   break;

#line 867 "./glsl.g"

case 40: {
    // nothing to do.
}   break;

#line 874 "./glsl.g"

case 41: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_ShiftLeft, expression(1), expression(3));
}   break;

#line 881 "./glsl.g"

case 42: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_ShiftRight, expression(1), expression(3));
}   break;

#line 888 "./glsl.g"

case 43: {
    // nothing to do.
}   break;

#line 895 "./glsl.g"

case 44: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_LessThan, expression(1), expression(3));
}   break;

#line 902 "./glsl.g"

case 45: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_GreaterThan, expression(1), expression(3));
}   break;

#line 909 "./glsl.g"

case 46: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_LessEqual, expression(1), expression(3));
}   break;

#line 916 "./glsl.g"

case 47: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_GreaterEqual, expression(1), expression(3));
}   break;

#line 923 "./glsl.g"

case 48: {
    // nothing to do.
}   break;

#line 930 "./glsl.g"

case 49: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_Equal, expression(1), expression(3));
}   break;

#line 937 "./glsl.g"

case 50: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_NotEqual, expression(1), expression(3));
}   break;

#line 944 "./glsl.g"

case 51: {
    // nothing to do.
}   break;

#line 951 "./glsl.g"

case 52: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_BitwiseAnd, expression(1), expression(3));
}   break;

#line 958 "./glsl.g"

case 53: {
    // nothing to do.
}   break;

#line 965 "./glsl.g"

case 54: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_BitwiseXor, expression(1), expression(3));
}   break;

#line 972 "./glsl.g"

case 55: {
    // nothing to do.
}   break;

#line 979 "./glsl.g"

case 56: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_BitwiseOr, expression(1), expression(3));
}   break;

#line 986 "./glsl.g"

case 57: {
    // nothing to do.
}   break;

#line 993 "./glsl.g"

case 58: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_LogicalAnd, expression(1), expression(3));
}   break;

#line 1000 "./glsl.g"

case 59: {
    // nothing to do.
}   break;

#line 1007 "./glsl.g"

case 60: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_LogicalXor, expression(1), expression(3));
}   break;

#line 1014 "./glsl.g"

case 61: {
    // nothing to do.
}   break;

#line 1021 "./glsl.g"

case 62: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_LogicalOr, expression(1), expression(3));
}   break;

#line 1028 "./glsl.g"

case 63: {
    // nothing to do.
}   break;

#line 1035 "./glsl.g"

case 64: {
    ast(1) = makeAstNode<TernaryExpression>(AST::Kind_Conditional, expression(1), expression(3), expression(5));
}   break;

#line 1042 "./glsl.g"

case 65: {
    // nothing to do.
}   break;

#line 1049 "./glsl.g"

case 66: {
    ast(1) = makeAstNode<AssignmentExpression>(sym(2).kind, expression(1), expression(3));
}   break;

#line 1056 "./glsl.g"

case 67: {
    sym(1).kind = AST::Kind_Assign;
}   break;

#line 1063 "./glsl.g"

case 68: {
    sym(1).kind = AST::Kind_AssignMultiply;
}   break;

#line 1070 "./glsl.g"

case 69: {
    sym(1).kind = AST::Kind_AssignDivide;
}   break;

#line 1077 "./glsl.g"

case 70: {
    sym(1).kind = AST::Kind_AssignModulus;
}   break;

#line 1084 "./glsl.g"

case 71: {
    sym(1).kind = AST::Kind_AssignPlus;
}   break;

#line 1091 "./glsl.g"

case 72: {
    sym(1).kind = AST::Kind_AssignMinus;
}   break;

#line 1098 "./glsl.g"

case 73: {
    sym(1).kind = AST::Kind_AssignShiftLeft;
}   break;

#line 1105 "./glsl.g"

case 74: {
    sym(1).kind = AST::Kind_AssignShiftRight;
}   break;

#line 1112 "./glsl.g"

case 75: {
    sym(1).kind = AST::Kind_AssignAnd;
}   break;

#line 1119 "./glsl.g"

case 76: {
    sym(1).kind = AST::Kind_AssignXor;
}   break;

#line 1126 "./glsl.g"

case 77: {
    sym(1).kind = AST::Kind_AssignOr;
}   break;

#line 1133 "./glsl.g"

case 78: {
    // nothing to do.
}   break;

#line 1140 "./glsl.g"

case 79: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_Comma, expression(1), expression(3));
}   break;

#line 1147 "./glsl.g"

case 80: {
    // nothing to do.
}   break;

#line 1154 "./glsl.g"

case 81: {
    // nothing to do.
}   break;

#line 1161 "./glsl.g"

case 82: {
    ast(1) = makeAstNode<InitDeclaration>(sym(1).declaration_list);
}   break;

#line 1168 "./glsl.g"

case 83: {
    ast(1) = makeAstNode<PrecisionDeclaration>(sym(2).precision, type(3));
}   break;

#line 1175 "./glsl.g"

case 84: {
    if (sym(1).type_qualifier.qualifier != QualifiedType::Struct) {
        // TODO: issue an error if the qualifier is not "struct".
    }
    Type *type = makeAstNode<StructType>(string(2), sym(4).field_list);
    ast(1) = makeAstNode<TypeDeclaration>(type);
}   break;

#line 1186 "./glsl.g"

case 85: {
    if ((sym(1).type_qualifier.qualifier & QualifiedType::Struct) == 0) {
        // TODO: issue an error if the qualifier does not contain "struct".
    }
    Type *type = makeAstNode<StructType>(string(2), sym(4).field_list);
    Type *qualtype = type;
    if (sym(1).type_qualifier.qualifier != QualifiedType::Struct) {
        qualtype = makeAstNode<QualifiedType>
            (sym(1).type_qualifier.qualifier & ~QualifiedType::Struct, qualtype,
             sym(1).type_qualifier.layout_list);
    }
    ast(1) = makeAstNode<TypeAndVariableDeclaration>
        (makeAstNode<TypeDeclaration>(type),
         makeAstNode<VariableDeclaration>(qualtype, string(6)));
}   break;

#line 1205 "./glsl.g"

case 86: {
    if ((sym(1).type_qualifier.qualifier & QualifiedType::Struct) == 0) {
        // TODO: issue an error if the qualifier does not contain "struct".
    }
    Type *type = makeAstNode<StructType>(string(2), sym(4).field_list);
    Type *qualtype = type;
    if (sym(1).type_qualifier.qualifier != QualifiedType::Struct) {
        qualtype = makeAstNode<QualifiedType>
            (sym(1).type_qualifier.qualifier & ~QualifiedType::Struct, qualtype,
             sym(1).type_qualifier.layout_list);
    }
    ast(1) = makeAstNode<TypeAndVariableDeclaration>
        (makeAstNode<TypeDeclaration>(type),
         makeAstNode<VariableDeclaration>
            (makeAstNode<ArrayType>(qualtype), string(6)));
}   break;

#line 1225 "./glsl.g"

case 87: {
    if ((sym(1).type_qualifier.qualifier & QualifiedType::Struct) == 0) {
        // TODO: issue an error if the qualifier does not contain "struct".
    }
    Type *type = makeAstNode<StructType>(string(2), sym(4).field_list);
    Type *qualtype = type;
    if (sym(1).type_qualifier.qualifier != QualifiedType::Struct) {
        qualtype = makeAstNode<QualifiedType>
            (sym(1).type_qualifier.qualifier & ~QualifiedType::Struct, qualtype,
             sym(1).type_qualifier.layout_list);
    }
    ast(1) = makeAstNode<TypeAndVariableDeclaration>
        (makeAstNode<TypeDeclaration>(type),
         makeAstNode<VariableDeclaration>
            (makeAstNode<ArrayType>(qualtype, expression(8)), string(6)));
}   break;

#line 1245 "./glsl.g"

case 88: {
    Type *type = makeAstNode<QualifiedType>
        (sym(1).type_qualifier.qualifier, (Type *)0,
         sym(1).type_qualifier.layout_list);
    ast(1) = makeAstNode<TypeDeclaration>(type);
}   break;

#line 1255 "./glsl.g"

case 89: {
    function(1)->finishParams();
}   break;

#line 1262 "./glsl.g"

case 90: {
    // nothing to do.
}   break;

#line 1269 "./glsl.g"

case 91: {
    // nothing to do.
}   break;

#line 1276 "./glsl.g"

case 92: {
    function(1)->params = makeAstNode< List<ParameterDeclaration *> >
        (sym(2).param_declaration);
}   break;

#line 1284 "./glsl.g"

case 93: {
    function(1)->params = makeAstNode< List<ParameterDeclaration *> >
        (function(1)->params, sym(3).param_declaration);
}   break;

#line 1292 "./glsl.g"

case 94: {
    function(1) = makeAstNode<FunctionDeclaration>(type(1), string(2));
}   break;

#line 1299 "./glsl.g"

case 95: {
    sym(1).param_declarator.type = type(1);
    sym(1).param_declarator.name = string(2);
}   break;

#line 1307 "./glsl.g"

case 96: {
    sym(1).param_declarator.type = makeAstNode<ArrayType>(type(1), expression(4));
    sym(1).param_declarator.name = string(2);
}   break;

#line 1315 "./glsl.g"

case 97: {
    ast(1) = makeAstNode<ParameterDeclaration>
        (makeAstNode<QualifiedType>
            (sym(1).qualifier, sym(3).param_declarator.type,
             (List<LayoutQualifier *> *)0),
         ParameterDeclaration::Qualifier(sym(2).qualifier),
         sym(3).param_declarator.name);
}   break;

#line 1327 "./glsl.g"

case 98: {
    ast(1) = makeAstNode<ParameterDeclaration>
        (sym(2).param_declarator.type,
         ParameterDeclaration::Qualifier(sym(1).qualifier),
         sym(2).param_declarator.name);
}   break;

#line 1337 "./glsl.g"

case 99: {
    ast(1) = makeAstNode<ParameterDeclaration>
        (makeAstNode<QualifiedType>
            (sym(1).qualifier, type(3), (List<LayoutQualifier *> *)0),
         ParameterDeclaration::Qualifier(sym(2).qualifier),
         (const QString *)0);
}   break;

#line 1348 "./glsl.g"

case 100: {
    ast(1) = makeAstNode<ParameterDeclaration>
        (type(2), ParameterDeclaration::Qualifier(sym(1).qualifier),
         (const QString *)0);
}   break;

#line 1357 "./glsl.g"

case 101: {
    sym(1).qualifier = ParameterDeclaration::In;
}   break;

#line 1364 "./glsl.g"

case 102: {
    sym(1).qualifier = ParameterDeclaration::In;
}   break;

#line 1371 "./glsl.g"

case 103: {
    sym(1).qualifier = ParameterDeclaration::Out;
}   break;

#line 1378 "./glsl.g"

case 104: {
    sym(1).qualifier = ParameterDeclaration::InOut;
}   break;

#line 1385 "./glsl.g"

case 105: {
    // nothing to do.
}   break;

#line 1392 "./glsl.g"

case 106: {
    sym(1).declaration_list = makeAstNode< List<Declaration *> >
        (sym(1).declaration);
}   break;

#line 1400 "./glsl.g"

case 107: {
    Type *type = VariableDeclaration::declarationType(sym(1).declaration_list);
    Declaration *decl = makeAstNode<VariableDeclaration>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<Declaration *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1410 "./glsl.g"

case 108: {
    Type *type = VariableDeclaration::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayType>(type);
    Declaration *decl = makeAstNode<VariableDeclaration>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<Declaration *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1421 "./glsl.g"

case 109: {
    Type *type = VariableDeclaration::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayType>(type, expression(5));
    Declaration *decl = makeAstNode<VariableDeclaration>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<Declaration *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1432 "./glsl.g"

case 110: {
    Type *type = VariableDeclaration::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayType>(type);
    Declaration *decl = makeAstNode<VariableDeclaration>
            (type, string(3), expression(7));
    sym(1).declaration_list = makeAstNode< List<Declaration *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1444 "./glsl.g"

case 111: {
    Type *type = VariableDeclaration::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayType>(type, expression(5));
    Declaration *decl = makeAstNode<VariableDeclaration>
            (type, string(3), expression(8));
    sym(1).declaration_list = makeAstNode< List<Declaration *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1456 "./glsl.g"

case 112: {
    Type *type = VariableDeclaration::declarationType(sym(1).declaration_list);
    Declaration *decl = makeAstNode<VariableDeclaration>
            (type, string(3), expression(5));
    sym(1).declaration_list = makeAstNode< List<Declaration *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1467 "./glsl.g"

case 113: {
    ast(1) = makeAstNode<TypeDeclaration>(type(1));
}   break;

#line 1474 "./glsl.g"

case 114: {
    ast(1) = makeAstNode<VariableDeclaration>(type(1), string(2));
}   break;

#line 1481 "./glsl.g"

case 115: {
    ast(1) = makeAstNode<VariableDeclaration>
        (makeAstNode<ArrayType>(type(1)), string(2));
}   break;

#line 1489 "./glsl.g"

case 116: {
    ast(1) = makeAstNode<VariableDeclaration>
        (makeAstNode<ArrayType>(type(1), expression(4)), string(2));
}   break;

#line 1497 "./glsl.g"

case 117: {
    ast(1) = makeAstNode<VariableDeclaration>
        (makeAstNode<ArrayType>(type(1)), string(2), expression(6));
}   break;

#line 1505 "./glsl.g"

case 118: {
    ast(1) = makeAstNode<VariableDeclaration>
        (makeAstNode<ArrayType>(type(1), expression(4)),
         string(2), expression(7));
}   break;

#line 1514 "./glsl.g"

case 119: {
    ast(1) = makeAstNode<VariableDeclaration>
        (type(1), string(2), expression(4));
}   break;

#line 1522 "./glsl.g"

case 120: {
    ast(1) = makeAstNode<InvariantDeclaration>(string(2));
}   break;

#line 1529 "./glsl.g"

case 121: {
    ast(1) = makeAstNode<QualifiedType>(0, type(1), (List<LayoutQualifier *> *)0);
}   break;

#line 1536 "./glsl.g"

case 122: {
    ast(1) = makeAstNode<QualifiedType>
        (sym(1).type_qualifier.qualifier, type(2),
         sym(1).type_qualifier.layout_list);
}   break;

#line 1545 "./glsl.g"

case 123: {
    sym(1).qualifier = QualifiedType::Invariant;
}   break;

#line 1552 "./glsl.g"

case 124: {
    sym(1).qualifier = QualifiedType::Smooth;
}   break;

#line 1559 "./glsl.g"

case 125: {
    sym(1).qualifier = QualifiedType::Flat;
}   break;

#line 1566 "./glsl.g"

case 126: {
    sym(1).qualifier = QualifiedType::NoPerspective;
}   break;

#line 1573 "./glsl.g"

case 127: {
    sym(1) = sym(3);
}   break;

#line 1580 "./glsl.g"

case 128: {
    sym(1).layout_list = makeAstNode< List<LayoutQualifier *> >(sym(1).layout);
}   break;

#line 1587 "./glsl.g"

case 129: {
    sym(1).layout_list = makeAstNode< List<LayoutQualifier *> >(sym(1).layout_list, sym(3).layout);
}   break;

#line 1594 "./glsl.g"

case 130: {
    sym(1).layout = makeAstNode<LayoutQualifier>(string(1), (const QString *)0);
}   break;

#line 1601 "./glsl.g"

case 131: {
    sym(1).layout = makeAstNode<LayoutQualifier>(string(1), string(3));
}   break;

#line 1608 "./glsl.g"

case 132: {
    sym(1).qualifier = QualifiedType::Const;
}   break;

#line 1615 "./glsl.g"

case 133: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1623 "./glsl.g"

case 134: {
    sym(1).type_qualifier.layout_list = sym(1).layout_list;
    sym(1).type_qualifier.qualifier = 0;
}   break;

#line 1631 "./glsl.g"

case 135: {
    sym(1).type_qualifier.layout_list = sym(1).layout_list;
    sym(1).type_qualifier.qualifier = sym(2).qualifier;
}   break;

#line 1639 "./glsl.g"

case 136: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1647 "./glsl.g"

case 137: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1655 "./glsl.g"

case 138: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1663 "./glsl.g"

case 139: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier | sym(3).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1671 "./glsl.g"

case 140: {
    sym(1).type_qualifier.qualifier = QualifiedType::Invariant;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1679 "./glsl.g"

case 141: {
    sym(1).qualifier = QualifiedType::Const;
}   break;

#line 1686 "./glsl.g"

case 142: {
    sym(1).qualifier = QualifiedType::Attribute;
}   break;

#line 1693 "./glsl.g"

case 143: {
    sym(1).qualifier = QualifiedType::Varying;
}   break;

#line 1700 "./glsl.g"

case 144: {
    sym(1).qualifier = QualifiedType::CentroidVarying;
}   break;

#line 1707 "./glsl.g"

case 145: {
    sym(1).qualifier = QualifiedType::In;
}   break;

#line 1714 "./glsl.g"

case 146: {
    sym(1).qualifier = QualifiedType::Out;
}   break;

#line 1721 "./glsl.g"

case 147: {
    sym(1).qualifier = QualifiedType::CentroidIn;
}   break;

#line 1728 "./glsl.g"

case 148: {
    sym(1).qualifier = QualifiedType::CentroidOut;
}   break;

#line 1735 "./glsl.g"

case 149: {
    sym(1).qualifier = QualifiedType::PatchIn;
}   break;

#line 1742 "./glsl.g"

case 150: {
    sym(1).qualifier = QualifiedType::PatchOut;
}   break;

#line 1749 "./glsl.g"

case 151: {
    sym(1).qualifier = QualifiedType::SampleIn;
}   break;

#line 1756 "./glsl.g"

case 152: {
    sym(1).qualifier = QualifiedType::SampleOut;
}   break;

#line 1763 "./glsl.g"

case 153: {
    sym(1).qualifier = QualifiedType::Uniform;
}   break;

#line 1770 "./glsl.g"

case 154: {
    // nothing to do.
}   break;

#line 1777 "./glsl.g"

case 155: {
    if (!type(2)->setPrecision(sym(1).precision)) {
        // TODO: issue an error about precision not allowed on this type.
    }
    ast(1) = type(2);
}   break;

#line 1787 "./glsl.g"

case 156: {
    // nothing to do.
}   break;

#line 1794 "./glsl.g"

case 157: {
    ast(1) = makeAstNode<ArrayType>(type(1));
}   break;

#line 1801 "./glsl.g"

case 158: {
    ast(1) = makeAstNode<ArrayType>(type(1), expression(3));
}   break;

#line 1808 "./glsl.g"

case 159: {
    ast(1) = makeBasicType(T_VOID, Type::Void);
}   break;

#line 1815 "./glsl.g"

case 160: {
    ast(1) = makeBasicType(T_FLOAT, Type::Primitive);
}   break;

#line 1822 "./glsl.g"

case 161: {
    ast(1) = makeBasicType(T_DOUBLE, Type::Primitive);
}   break;

#line 1829 "./glsl.g"

case 162: {
    ast(1) = makeBasicType(T_INT, Type::Primitive);
}   break;

#line 1836 "./glsl.g"

case 163: {
    ast(1) = makeBasicType(T_UINT, Type::Primitive);
}   break;

#line 1843 "./glsl.g"

case 164: {
    ast(1) = makeBasicType(T_BOOL, Type::Primitive);
}   break;

#line 1850 "./glsl.g"

case 165: {
    ast(1) = makeBasicType(T_VEC2, Type::Vector2);
}   break;

#line 1857 "./glsl.g"

case 166: {
    ast(1) = makeBasicType(T_VEC3, Type::Vector3);
}   break;

#line 1864 "./glsl.g"

case 167: {
    ast(1) = makeBasicType(T_VEC4, Type::Vector4);
}   break;

#line 1871 "./glsl.g"

case 168: {
    ast(1) = makeBasicType(T_DVEC2, Type::Vector2);
}   break;

#line 1878 "./glsl.g"

case 169: {
    ast(1) = makeBasicType(T_DVEC3, Type::Vector3);
}   break;

#line 1885 "./glsl.g"

case 170: {
    ast(1) = makeBasicType(T_DVEC4, Type::Vector4);
}   break;

#line 1892 "./glsl.g"

case 171: {
    ast(1) = makeBasicType(T_BVEC2, Type::Vector2);
}   break;

#line 1899 "./glsl.g"

case 172: {
    ast(1) = makeBasicType(T_BVEC3, Type::Vector3);
}   break;

#line 1906 "./glsl.g"

case 173: {
    ast(1) = makeBasicType(T_BVEC4, Type::Vector4);
}   break;

#line 1913 "./glsl.g"

case 174: {
    ast(1) = makeBasicType(T_IVEC2, Type::Vector2);
}   break;

#line 1920 "./glsl.g"

case 175: {
    ast(1) = makeBasicType(T_IVEC3, Type::Vector3);
}   break;

#line 1927 "./glsl.g"

case 176: {
    ast(1) = makeBasicType(T_IVEC4, Type::Vector4);
}   break;

#line 1934 "./glsl.g"

case 177: {
    ast(1) = makeBasicType(T_UVEC2, Type::Vector2);
}   break;

#line 1941 "./glsl.g"

case 178: {
    ast(1) = makeBasicType(T_UVEC3, Type::Vector3);
}   break;

#line 1948 "./glsl.g"

case 179: {
    ast(1) = makeBasicType(T_UVEC4, Type::Vector4);
}   break;

#line 1955 "./glsl.g"

case 180: {
    ast(1) = makeBasicType(T_MAT2, Type::Matrix);
}   break;

#line 1962 "./glsl.g"

case 181: {
    ast(1) = makeBasicType(T_MAT3, Type::Matrix);
}   break;

#line 1969 "./glsl.g"

case 182: {
    ast(1) = makeBasicType(T_MAT4, Type::Matrix);
}   break;

#line 1976 "./glsl.g"

case 183: {
    ast(1) = makeBasicType(T_MAT2, Type::Matrix);
}   break;

#line 1983 "./glsl.g"

case 184: {
    ast(1) = makeBasicType(T_MAT2X3, Type::Matrix);
}   break;

#line 1990 "./glsl.g"

case 185: {
    ast(1) = makeBasicType(T_MAT2X4, Type::Matrix);
}   break;

#line 1997 "./glsl.g"

case 186: {
    ast(1) = makeBasicType(T_MAT3X2, Type::Matrix);
}   break;

#line 2004 "./glsl.g"

case 187: {
    ast(1) = makeBasicType(T_MAT3, Type::Matrix);
}   break;

#line 2011 "./glsl.g"

case 188: {
    ast(1) = makeBasicType(T_MAT3X4, Type::Matrix);
}   break;

#line 2018 "./glsl.g"

case 189: {
    ast(1) = makeBasicType(T_MAT4X2, Type::Matrix);
}   break;

#line 2025 "./glsl.g"

case 190: {
    ast(1) = makeBasicType(T_MAT4X3, Type::Matrix);
}   break;

#line 2032 "./glsl.g"

case 191: {
    ast(1) = makeBasicType(T_MAT4, Type::Matrix);
}   break;

#line 2039 "./glsl.g"

case 192: {
    ast(1) = makeBasicType(T_DMAT2, Type::Matrix);
}   break;

#line 2046 "./glsl.g"

case 193: {
    ast(1) = makeBasicType(T_DMAT3, Type::Matrix);
}   break;

#line 2053 "./glsl.g"

case 194: {
    ast(1) = makeBasicType(T_DMAT4, Type::Matrix);
}   break;

#line 2060 "./glsl.g"

case 195: {
    ast(1) = makeBasicType(T_DMAT2, Type::Matrix);
}   break;

#line 2067 "./glsl.g"

case 196: {
    ast(1) = makeBasicType(T_DMAT2X3, Type::Matrix);
}   break;

#line 2074 "./glsl.g"

case 197: {
    ast(1) = makeBasicType(T_DMAT2X4, Type::Matrix);
}   break;

#line 2081 "./glsl.g"

case 198: {
    ast(1) = makeBasicType(T_DMAT3X2, Type::Matrix);
}   break;

#line 2088 "./glsl.g"

case 199: {
    ast(1) = makeBasicType(T_DMAT3, Type::Matrix);
}   break;

#line 2095 "./glsl.g"

case 200: {
    ast(1) = makeBasicType(T_DMAT3X4, Type::Matrix);
}   break;

#line 2102 "./glsl.g"

case 201: {
    ast(1) = makeBasicType(T_DMAT4X2, Type::Matrix);
}   break;

#line 2109 "./glsl.g"

case 202: {
    ast(1) = makeBasicType(T_DMAT4X3, Type::Matrix);
}   break;

#line 2116 "./glsl.g"

case 203: {
    ast(1) = makeBasicType(T_DMAT4, Type::Matrix);
}   break;

#line 2123 "./glsl.g"

case 204: {
    ast(1) = makeBasicType(T_SAMPLER1D, Type::Sampler1D);
}   break;

#line 2130 "./glsl.g"

case 205: {
    ast(1) = makeBasicType(T_SAMPLER2D, Type::Sampler2D);
}   break;

#line 2137 "./glsl.g"

case 206: {
    ast(1) = makeBasicType(T_SAMPLER3D, Type::Sampler3D);
}   break;

#line 2144 "./glsl.g"

case 207: {
    ast(1) = makeBasicType(T_SAMPLERCUBE, Type::SamplerCube);
}   break;

#line 2151 "./glsl.g"

case 208: {
    ast(1) = makeBasicType(T_SAMPLER1DSHADOW, Type::Sampler1DShadow);
}   break;

#line 2158 "./glsl.g"

case 209: {
    ast(1) = makeBasicType(T_SAMPLER2DSHADOW, Type::Sampler2DShadow);
}   break;

#line 2165 "./glsl.g"

case 210: {
    ast(1) = makeBasicType(T_SAMPLERCUBESHADOW, Type::SamplerCubeShadow);
}   break;

#line 2172 "./glsl.g"

case 211: {
    ast(1) = makeBasicType(T_SAMPLER1DARRAY, Type::Sampler1DArray);
}   break;

#line 2179 "./glsl.g"

case 212: {
    ast(1) = makeBasicType(T_SAMPLER2DARRAY, Type::Sampler2DArray);
}   break;

#line 2186 "./glsl.g"

case 213: {
    ast(1) = makeBasicType(T_SAMPLER1DARRAYSHADOW, Type::Sampler1DArrayShadow);
}   break;

#line 2193 "./glsl.g"

case 214: {
    ast(1) = makeBasicType(T_SAMPLER2DARRAYSHADOW, Type::Sampler2DArrayShadow);
}   break;

#line 2200 "./glsl.g"

case 215: {
    ast(1) = makeBasicType(T_SAMPLERCUBEARRAY, Type::SamplerCubeShadow);
}   break;

#line 2207 "./glsl.g"

case 216: {
    ast(1) = makeBasicType(T_SAMPLERCUBEARRAYSHADOW, Type::SamplerCubeArrayShadow);
}   break;

#line 2214 "./glsl.g"

case 217: {
    ast(1) = makeBasicType(T_ISAMPLER1D, Type::Sampler1D);
}   break;

#line 2221 "./glsl.g"

case 218: {
    ast(1) = makeBasicType(T_ISAMPLER2D, Type::Sampler2D);
}   break;

#line 2228 "./glsl.g"

case 219: {
    ast(1) = makeBasicType(T_ISAMPLER3D, Type::Sampler3D);
}   break;

#line 2235 "./glsl.g"

case 220: {
    ast(1) = makeBasicType(T_ISAMPLERCUBE, Type::SamplerCube);
}   break;

#line 2242 "./glsl.g"

case 221: {
    ast(1) = makeBasicType(T_ISAMPLER1DARRAY, Type::Sampler1DArray);
}   break;

#line 2249 "./glsl.g"

case 222: {
    ast(1) = makeBasicType(T_ISAMPLER2DARRAY, Type::Sampler2DArray);
}   break;

#line 2256 "./glsl.g"

case 223: {
    ast(1) = makeBasicType(T_ISAMPLERCUBEARRAY, Type::SamplerCubeArray);
}   break;

#line 2263 "./glsl.g"

case 224: {
    ast(1) = makeBasicType(T_USAMPLER1D, Type::Sampler1D);
}   break;

#line 2270 "./glsl.g"

case 225: {
    ast(1) = makeBasicType(T_USAMPLER2D, Type::Sampler2D);
}   break;

#line 2277 "./glsl.g"

case 226: {
    ast(1) = makeBasicType(T_USAMPLER3D, Type::Sampler3D);
}   break;

#line 2284 "./glsl.g"

case 227: {
    ast(1) = makeBasicType(T_USAMPLERCUBE, Type::SamplerCube);
}   break;

#line 2291 "./glsl.g"

case 228: {
    ast(1) = makeBasicType(T_USAMPLER1DARRAY, Type::Sampler1DArray);
}   break;

#line 2298 "./glsl.g"

case 229: {
    ast(1) = makeBasicType(T_USAMPLER2DARRAY, Type::Sampler2DArray);
}   break;

#line 2305 "./glsl.g"

case 230: {
    ast(1) = makeBasicType(T_USAMPLERCUBEARRAY, Type::SamplerCubeArray);
}   break;

#line 2312 "./glsl.g"

case 231: {
    ast(1) = makeBasicType(T_SAMPLER2DRECT, Type::Sampler2DRect);
}   break;

#line 2319 "./glsl.g"

case 232: {
    ast(1) = makeBasicType(T_SAMPLER2DRECTSHADOW, Type::Sampler2DRectShadow);
}   break;

#line 2326 "./glsl.g"

case 233: {
    ast(1) = makeBasicType(T_ISAMPLER2DRECT, Type::Sampler2DRect);
}   break;

#line 2333 "./glsl.g"

case 234: {
    ast(1) = makeBasicType(T_USAMPLER2DRECT, Type::Sampler2DRect);
}   break;

#line 2340 "./glsl.g"

case 235: {
    ast(1) = makeBasicType(T_SAMPLERBUFFER, Type::SamplerBuffer);
}   break;

#line 2347 "./glsl.g"

case 236: {
    ast(1) = makeBasicType(T_ISAMPLERBUFFER, Type::SamplerBuffer);
}   break;

#line 2354 "./glsl.g"

case 237: {
    ast(1) = makeBasicType(T_USAMPLERBUFFER, Type::SamplerBuffer);
}   break;

#line 2361 "./glsl.g"

case 238: {
    ast(1) = makeBasicType(T_SAMPLER2DMS, Type::Sampler2DMS);
}   break;

#line 2368 "./glsl.g"

case 239: {
    ast(1) = makeBasicType(T_ISAMPLER2DMS, Type::Sampler2DMS);
}   break;

#line 2375 "./glsl.g"

case 240: {
    ast(1) = makeBasicType(T_USAMPLER2DMS, Type::Sampler2DMS);
}   break;

#line 2382 "./glsl.g"

case 241: {
    ast(1) = makeBasicType(T_SAMPLER2DMSARRAY, Type::Sampler2DMSArray);
}   break;

#line 2389 "./glsl.g"

case 242: {
    ast(1) = makeBasicType(T_ISAMPLER2DMSARRAY, Type::Sampler2DMSArray);
}   break;

#line 2396 "./glsl.g"

case 243: {
    ast(1) = makeBasicType(T_USAMPLER2DMSARRAY, Type::Sampler2DMSArray);
}   break;

#line 2403 "./glsl.g"

case 244: {
    // nothing to do.
}   break;

#line 2410 "./glsl.g"

case 245: {
    ast(1) = makeAstNode<NamedType>(string(1));
}   break;

#line 2417 "./glsl.g"

case 246: {
    sym(1).precision = Type::Highp;
}   break;

#line 2424 "./glsl.g"

case 247: {
    sym(1).precision = Type::Mediump;
}   break;

#line 2431 "./glsl.g"

case 248: {
    sym(1).precision = Type::Lowp;
}   break;

#line 2438 "./glsl.g"

case 249: {
    ast(1) = makeAstNode<StructType>(string(2), sym(4).field_list);
}   break;

#line 2445 "./glsl.g"

case 250: {
    ast(1) = makeAstNode<StructType>(sym(3).field_list);
}   break;

#line 2452 "./glsl.g"

case 251: {
    // nothing to do.
}   break;

#line 2459 "./glsl.g"

case 252: {
    sym(1).field_list = appendLists(sym(1).field_list, sym(2).field_list);
}   break;

#line 2466 "./glsl.g"

case 253: {
    sym(1).field_list = StructType::fixInnerTypes(type(1), sym(2).field_list);
}   break;

#line 2473 "./glsl.g"

case 254: {
    sym(1).field_list = StructType::fixInnerTypes
        (makeAstNode<QualifiedType>
            (sym(1).type_qualifier.qualifier, type(2),
             sym(1).type_qualifier.layout_list), sym(3).field_list);
}   break;

#line 2483 "./glsl.g"

case 255: {
    // nothing to do.
    sym(1).field_list = makeAstNode< List<StructType::Field *> >(sym(1).field);
}   break;

#line 2491 "./glsl.g"

case 256: {
    sym(1).field_list = makeAstNode< List<StructType::Field *> >(sym(1).field_list, sym(3).field);
}   break;

#line 2498 "./glsl.g"

case 257: {
    sym(1).field = makeAstNode<StructType::Field>(string(1));
}   break;

#line 2505 "./glsl.g"

case 258: {
    sym(1).field = makeAstNode<StructType::Field>
        (string(1), makeAstNode<ArrayType>((Type *)0));
}   break;

#line 2513 "./glsl.g"

case 259: {
    sym(1).field = makeAstNode<StructType::Field>
        (string(1), makeAstNode<ArrayType>((Type *)0, expression(3)));
}   break;

#line 2521 "./glsl.g"

case 260: {
    // nothing to do.
}   break;

#line 2528 "./glsl.g"

case 261: {
    ast(1) = makeAstNode<DeclarationStatement>(sym(1).declaration_list);
}   break;

#line 2535 "./glsl.g"

case 262: {
    // nothing to do.
}   break;

#line 2542 "./glsl.g"

case 263: {
    // nothing to do.
}   break;

#line 2549 "./glsl.g"

case 264: {
    // nothing to do.
}   break;

#line 2556 "./glsl.g"

case 265: {
    // nothing to do.
}   break;

#line 2563 "./glsl.g"

case 266: {
    // nothing to do.
}   break;

#line 2570 "./glsl.g"

case 267: {
    // nothing to do.
}   break;

#line 2577 "./glsl.g"

case 268: {
    // nothing to do.
}   break;

#line 2584 "./glsl.g"

case 269: {
    // nothing to do.
}   break;

#line 2591 "./glsl.g"

case 270: {
    // nothing to do.
}   break;

#line 2598 "./glsl.g"

case 271: {
    ast(1) = makeAstNode<CompoundStatement>();
}   break;

#line 2605 "./glsl.g"

case 272: {
    ast(1) = makeAstNode<CompoundStatement>(sym(2).statement_list);
}   break;

#line 2612 "./glsl.g"

case 273: {
    // nothing to do.
}   break;

#line 2619 "./glsl.g"

case 274: {
    // nothing to do.
}   break;

#line 2626 "./glsl.g"

case 275: {
    ast(1) = makeAstNode<CompoundStatement>();
}   break;

#line 2633 "./glsl.g"

case 276: {
    ast(1) = makeAstNode<CompoundStatement>(sym(2).statement_list);
}   break;

#line 2640 "./glsl.g"

case 277: {
    sym(1).statement_list = makeAstNode< List<Statement *> >(sym(1).statement);
}   break;

#line 2647 "./glsl.g"

case 278: {
    sym(1).statement_list = makeAstNode< List<Statement *> >(sym(1).statement_list, sym(2).statement);
}   break;

#line 2654 "./glsl.g"

case 279: {
    ast(1) = makeAstNode<CompoundStatement>();  // Empty statement
}   break;

#line 2661 "./glsl.g"

case 280: {
    ast(1) = makeAstNode<ExpressionStatement>(expression(1));
}   break;

#line 2668 "./glsl.g"

case 281: {
    ast(1) = makeAstNode<IfStatement>(expression(3), sym(5).ifstmt.thenClause, sym(5).ifstmt.elseClause);
}   break;

#line 2675 "./glsl.g"

case 282: {
    sym(1).ifstmt.thenClause = statement(1);
    sym(1).ifstmt.elseClause = statement(3);
}   break;

#line 2683 "./glsl.g"

case 283: {
    sym(1).ifstmt.thenClause = statement(1);
    sym(1).ifstmt.elseClause = 0;
}   break;

#line 2691 "./glsl.g"

case 284: {
    // nothing to do.
}   break;

#line 2698 "./glsl.g"

case 285: {
    ast(1) = makeAstNode<DeclarationExpression>
        (type(1), string(2), expression(4));
}   break;

#line 2706 "./glsl.g"

case 286: {
    ast(1) = makeAstNode<SwitchStatement>(expression(3), statement(6));
}   break;

#line 2713 "./glsl.g"

case 287: {
    ast(1) = makeAstNode<CompoundStatement>();
}   break;

#line 2720 "./glsl.g"

case 288: {
    ast(1) = makeAstNode<CompoundStatement>(sym(1).statement_list);
}   break;

#line 2727 "./glsl.g"

case 289: {
    ast(1) = makeAstNode<CaseLabelStatement>(expression(2));
}   break;

#line 2734 "./glsl.g"

case 290: {
    ast(1) = makeAstNode<CaseLabelStatement>();
}   break;

#line 2741 "./glsl.g"

case 291: {
    ast(1) = makeAstNode<WhileStatement>(expression(3), statement(5));
}   break;

#line 2748 "./glsl.g"

case 292: {
    ast(1) = makeAstNode<DoStatement>(statement(2), expression(5));
}   break;

#line 2755 "./glsl.g"

case 293: {
    ast(1) = makeAstNode<ForStatement>(statement(3), sym(4).forstmt.condition, sym(4).forstmt.increment, statement(6));
}   break;

#line 2762 "./glsl.g"

case 294: {
    // nothing to do.
}   break;

#line 2769 "./glsl.g"

case 295: {
    // nothing to do.
}   break;

#line 2776 "./glsl.g"

case 296: {
    // nothing to do.
}   break;

#line 2783 "./glsl.g"

case 297: {
    // nothing to do.
}   break;

#line 2790 "./glsl.g"

case 298: {
    sym(1).forstmt.condition = expression(1);
    sym(1).forstmt.increment = 0;
}   break;

#line 2798 "./glsl.g"

case 299: {
    sym(1).forstmt.condition = expression(1);
    sym(1).forstmt.increment = expression(3);
}   break;

#line 2806 "./glsl.g"

case 300: {
    ast(1) = makeAstNode<JumpStatement>(AST::Kind_Continue);
}   break;

#line 2813 "./glsl.g"

case 301: {
    ast(1) = makeAstNode<JumpStatement>(AST::Kind_Break);
}   break;

#line 2820 "./glsl.g"

case 302: {
    ast(1) = makeAstNode<ReturnStatement>();
}   break;

#line 2827 "./glsl.g"

case 303: {
    ast(1) = makeAstNode<ReturnStatement>(expression(2));
}   break;

#line 2834 "./glsl.g"

case 304: {
    ast(1) = makeAstNode<JumpStatement>(AST::Kind_Discard);
}   break;

#line 2841 "./glsl.g"

case 305: {
    ast(1) = makeAstNode<TranslationUnit>(sym(1).declaration_list);
}   break;

#line 2848 "./glsl.g"

case 306: {
    if (sym(1).declaration) {
        sym(1).declaration_list = makeAstNode< List<Declaration *> >
            (sym(1).declaration);
    } else {
        sym(1).declaration_list = 0;
    }
}   break;

#line 2860 "./glsl.g"

case 307: {
    if (sym(1).declaration_list && sym(2).declaration) {
        sym(1).declaration_list = makeAstNode< List<Declaration *> >
            (sym(1).declaration_list, sym(2).declaration);
    } else if (!sym(1).declaration_list) {
        if (sym(2).declaration) {
            sym(1).declaration_list = makeAstNode< List<Declaration *> >
                (sym(2).declaration);
        } else {
            sym(1).declaration_list = 0;
        }
    }
}   break;

#line 2877 "./glsl.g"

case 308: {
    // nothing to do.
}   break;

#line 2884 "./glsl.g"

case 309: {
    // nothing to do.
}   break;

#line 2891 "./glsl.g"

case 310: {
    ast(1) = 0;
}   break;

#line 2898 "./glsl.g"

case 311: {
    function(1)->body = statement(2);
}   break;

#line 2905 "./glsl.g"

case 312: {
    ast(1) = 0;
}   break;

#line 2913 "./glsl.g"

} // end switch
} // end Parser::reduce()
