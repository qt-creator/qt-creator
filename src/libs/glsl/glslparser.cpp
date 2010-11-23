
#line 400 "./glsl.g"

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

#line 571 "./glsl.g"

void Parser::reduce(int ruleno)
{
switch(ruleno) {

#line 580 "./glsl.g"

case 0: {
    ast(1) = makeAstNode<IdentifierExpression>(string(1));
}   break;

#line 587 "./glsl.g"

case 1: {
    ast(1) = makeAstNode<LiteralExpression>(string(1));
}   break;

#line 594 "./glsl.g"

case 2: {
    ast(1) = makeAstNode<LiteralExpression>(_engine->identifier("true", 4));
}   break;

#line 601 "./glsl.g"

case 3: {
    ast(1) = makeAstNode<LiteralExpression>(_engine->identifier("false", 5));
}   break;

#line 608 "./glsl.g"

case 4: {
    // nothing to do.
}   break;

#line 615 "./glsl.g"

case 5: {
    ast(1) = ast(2);
}   break;

#line 622 "./glsl.g"

case 6: {
    // nothing to do.
}   break;

#line 629 "./glsl.g"

case 7: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_ArrayAccess, expression(1), expression(3));
}   break;

#line 636 "./glsl.g"

case 8: {
    // nothing to do.
}   break;

#line 643 "./glsl.g"

case 9: {
    ast(1) = makeAstNode<MemberAccessExpression>(expression(1), string(3));
}   break;

#line 650 "./glsl.g"

case 10: {
    ast(1) = makeAstNode<UnaryExpression>(AST::Kind_PostIncrement, expression(1));
}   break;

#line 657 "./glsl.g"

case 11: {
    ast(1) = makeAstNode<UnaryExpression>(AST::Kind_PostDecrement, expression(1));
}   break;

#line 664 "./glsl.g"

case 12: {
    // nothing to do.
}   break;

#line 671 "./glsl.g"

case 13: {
    // nothing to do.
}   break;

#line 678 "./glsl.g"

case 14: {
    ast(1) = makeAstNode<FunctionCallExpression>
        (sym(1).function.id, sym(1).function.arguments);
}   break;

#line 686 "./glsl.g"

case 15: {
    ast(1) = makeAstNode<FunctionCallExpression>
        (expression(1), sym(3).function.id, sym(3).function.arguments);
}   break;

#line 694 "./glsl.g"

case 16: {
    // nothing to do.
}   break;

#line 701 "./glsl.g"

case 17: {
    // nothing to do.
}   break;

#line 708 "./glsl.g"

case 18: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments = 0;
}   break;

#line 716 "./glsl.g"

case 19: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments = 0;
}   break;

#line 724 "./glsl.g"

case 20: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments =
        makeAstNode< List<Expression *> >(expression(2));
}   break;

#line 733 "./glsl.g"

case 21: {
    sym(1).function.arguments =
        makeAstNode< List<Expression *> >
            (sym(1).function.arguments, expression(3));
}   break;

#line 742 "./glsl.g"

case 22: {
    // nothing to do.
}   break;

#line 749 "./glsl.g"

case 23: {
    ast(1) = makeAstNode<FunctionIdentifier>(type(1));
}   break;

#line 756 "./glsl.g"

case 24: {
    ast(1) = makeAstNode<FunctionIdentifier>(string(1));
}   break;

#line 763 "./glsl.g"

case 25: {
    // nothing to do.
}   break;

#line 770 "./glsl.g"

case 26: {
    ast(1) = makeAstNode<UnaryExpression>(AST::Kind_PreIncrement, expression(2));
}   break;

#line 777 "./glsl.g"

case 27: {
    ast(1) = makeAstNode<UnaryExpression>(AST::Kind_PreDecrement, expression(2));
}   break;

#line 784 "./glsl.g"

case 28: {
    ast(1) = makeAstNode<UnaryExpression>(sym(1).kind, expression(2));
}   break;

#line 791 "./glsl.g"

case 29: {
    sym(1).kind = AST::Kind_UnaryPlus;
}   break;

#line 798 "./glsl.g"

case 30: {
    sym(1).kind = AST::Kind_UnaryMinus;
}   break;

#line 805 "./glsl.g"

case 31: {
    sym(1).kind = AST::Kind_LogicalNot;
}   break;

#line 812 "./glsl.g"

case 32: {
    sym(1).kind = AST::Kind_BitwiseNot;
}   break;

#line 819 "./glsl.g"

case 33: {
    // nothing to do.
}   break;

#line 826 "./glsl.g"

case 34: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_Multiply, expression(1), expression(3));
}   break;

#line 833 "./glsl.g"

case 35: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_Divide, expression(1), expression(3));
}   break;

#line 840 "./glsl.g"

case 36: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_Modulus, expression(1), expression(3));
}   break;

#line 847 "./glsl.g"

case 37: {
    // nothing to do.
}   break;

#line 854 "./glsl.g"

case 38: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_Plus, expression(1), expression(3));
}   break;

#line 861 "./glsl.g"

case 39: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_Minus, expression(1), expression(3));
}   break;

#line 868 "./glsl.g"

case 40: {
    // nothing to do.
}   break;

#line 875 "./glsl.g"

case 41: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_ShiftLeft, expression(1), expression(3));
}   break;

#line 882 "./glsl.g"

case 42: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_ShiftRight, expression(1), expression(3));
}   break;

#line 889 "./glsl.g"

case 43: {
    // nothing to do.
}   break;

#line 896 "./glsl.g"

case 44: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_LessThan, expression(1), expression(3));
}   break;

#line 903 "./glsl.g"

case 45: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_GreaterThan, expression(1), expression(3));
}   break;

#line 910 "./glsl.g"

case 46: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_LessEqual, expression(1), expression(3));
}   break;

#line 917 "./glsl.g"

case 47: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_GreaterEqual, expression(1), expression(3));
}   break;

#line 924 "./glsl.g"

case 48: {
    // nothing to do.
}   break;

#line 931 "./glsl.g"

case 49: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_Equal, expression(1), expression(3));
}   break;

#line 938 "./glsl.g"

case 50: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_NotEqual, expression(1), expression(3));
}   break;

#line 945 "./glsl.g"

case 51: {
    // nothing to do.
}   break;

#line 952 "./glsl.g"

case 52: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_BitwiseAnd, expression(1), expression(3));
}   break;

#line 959 "./glsl.g"

case 53: {
    // nothing to do.
}   break;

#line 966 "./glsl.g"

case 54: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_BitwiseXor, expression(1), expression(3));
}   break;

#line 973 "./glsl.g"

case 55: {
    // nothing to do.
}   break;

#line 980 "./glsl.g"

case 56: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_BitwiseOr, expression(1), expression(3));
}   break;

#line 987 "./glsl.g"

case 57: {
    // nothing to do.
}   break;

#line 994 "./glsl.g"

case 58: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_LogicalAnd, expression(1), expression(3));
}   break;

#line 1001 "./glsl.g"

case 59: {
    // nothing to do.
}   break;

#line 1008 "./glsl.g"

case 60: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_LogicalXor, expression(1), expression(3));
}   break;

#line 1015 "./glsl.g"

case 61: {
    // nothing to do.
}   break;

#line 1022 "./glsl.g"

case 62: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_LogicalOr, expression(1), expression(3));
}   break;

#line 1029 "./glsl.g"

case 63: {
    // nothing to do.
}   break;

#line 1036 "./glsl.g"

case 64: {
    ast(1) = makeAstNode<TernaryExpression>(AST::Kind_Conditional, expression(1), expression(3), expression(5));
}   break;

#line 1043 "./glsl.g"

case 65: {
    // nothing to do.
}   break;

#line 1050 "./glsl.g"

case 66: {
    ast(1) = makeAstNode<AssignmentExpression>(sym(2).kind, expression(1), expression(3));
}   break;

#line 1057 "./glsl.g"

case 67: {
    sym(1).kind = AST::Kind_Assign;
}   break;

#line 1064 "./glsl.g"

case 68: {
    sym(1).kind = AST::Kind_AssignMultiply;
}   break;

#line 1071 "./glsl.g"

case 69: {
    sym(1).kind = AST::Kind_AssignDivide;
}   break;

#line 1078 "./glsl.g"

case 70: {
    sym(1).kind = AST::Kind_AssignModulus;
}   break;

#line 1085 "./glsl.g"

case 71: {
    sym(1).kind = AST::Kind_AssignPlus;
}   break;

#line 1092 "./glsl.g"

case 72: {
    sym(1).kind = AST::Kind_AssignMinus;
}   break;

#line 1099 "./glsl.g"

case 73: {
    sym(1).kind = AST::Kind_AssignShiftLeft;
}   break;

#line 1106 "./glsl.g"

case 74: {
    sym(1).kind = AST::Kind_AssignShiftRight;
}   break;

#line 1113 "./glsl.g"

case 75: {
    sym(1).kind = AST::Kind_AssignAnd;
}   break;

#line 1120 "./glsl.g"

case 76: {
    sym(1).kind = AST::Kind_AssignXor;
}   break;

#line 1127 "./glsl.g"

case 77: {
    sym(1).kind = AST::Kind_AssignOr;
}   break;

#line 1134 "./glsl.g"

case 78: {
    // nothing to do.
}   break;

#line 1141 "./glsl.g"

case 79: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_Comma, expression(1), expression(3));
}   break;

#line 1148 "./glsl.g"

case 80: {
    // nothing to do.
}   break;

#line 1155 "./glsl.g"

case 81: {
    // nothing to do.
}   break;

#line 1162 "./glsl.g"

case 82: {
    ast(1) = makeAstNode<InitDeclaration>(sym(1).declaration_list);
}   break;

#line 1169 "./glsl.g"

case 83: {
    ast(1) = makeAstNode<PrecisionDeclaration>(sym(2).precision, type(3));
}   break;

#line 1176 "./glsl.g"

case 84: {
    if (sym(1).type_qualifier.qualifier != QualifiedType::Struct) {
        // TODO: issue an error if the qualifier is not "struct".
    }
    Type *type = makeAstNode<StructType>(string(2), sym(4).field_list);
    ast(1) = makeAstNode<TypeDeclaration>(type);
}   break;

#line 1187 "./glsl.g"

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

#line 1206 "./glsl.g"

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

#line 1226 "./glsl.g"

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

#line 1246 "./glsl.g"

case 88: {
    Type *type = makeAstNode<QualifiedType>
        (sym(1).type_qualifier.qualifier, (Type *)0,
         sym(1).type_qualifier.layout_list);
    ast(1) = makeAstNode<TypeDeclaration>(type);
}   break;

#line 1256 "./glsl.g"

case 89: {
    function(1)->finishParams();
}   break;

#line 1263 "./glsl.g"

case 90: {
    // nothing to do.
}   break;

#line 1270 "./glsl.g"

case 91: {
    // nothing to do.
}   break;

#line 1277 "./glsl.g"

case 92: {
    function(1)->params = makeAstNode< List<ParameterDeclaration *> >
        (sym(2).param_declaration);
}   break;

#line 1285 "./glsl.g"

case 93: {
    function(1)->params = makeAstNode< List<ParameterDeclaration *> >
        (function(1)->params, sym(3).param_declaration);
}   break;

#line 1293 "./glsl.g"

case 94: {
    function(1) = makeAstNode<FunctionDeclaration>(type(1), string(2));
}   break;

#line 1300 "./glsl.g"

case 95: {
    sym(1).param_declarator.type = type(1);
    sym(1).param_declarator.name = string(2);
}   break;

#line 1308 "./glsl.g"

case 96: {
    sym(1).param_declarator.type = makeAstNode<ArrayType>(type(1), expression(4));
    sym(1).param_declarator.name = string(2);
}   break;

#line 1316 "./glsl.g"

case 97: {
    ast(1) = makeAstNode<ParameterDeclaration>
        (makeAstNode<QualifiedType>
            (sym(1).qualifier, sym(3).param_declarator.type,
             (List<LayoutQualifier *> *)0),
         ParameterDeclaration::Qualifier(sym(2).qualifier),
         sym(3).param_declarator.name);
}   break;

#line 1328 "./glsl.g"

case 98: {
    ast(1) = makeAstNode<ParameterDeclaration>
        (sym(2).param_declarator.type,
         ParameterDeclaration::Qualifier(sym(1).qualifier),
         sym(2).param_declarator.name);
}   break;

#line 1338 "./glsl.g"

case 99: {
    ast(1) = makeAstNode<ParameterDeclaration>
        (makeAstNode<QualifiedType>
            (sym(1).qualifier, type(3), (List<LayoutQualifier *> *)0),
         ParameterDeclaration::Qualifier(sym(2).qualifier),
         (const QString *)0);
}   break;

#line 1349 "./glsl.g"

case 100: {
    ast(1) = makeAstNode<ParameterDeclaration>
        (type(2), ParameterDeclaration::Qualifier(sym(1).qualifier),
         (const QString *)0);
}   break;

#line 1358 "./glsl.g"

case 101: {
    sym(1).qualifier = ParameterDeclaration::In;
}   break;

#line 1365 "./glsl.g"

case 102: {
    sym(1).qualifier = ParameterDeclaration::In;
}   break;

#line 1372 "./glsl.g"

case 103: {
    sym(1).qualifier = ParameterDeclaration::Out;
}   break;

#line 1379 "./glsl.g"

case 104: {
    sym(1).qualifier = ParameterDeclaration::InOut;
}   break;

#line 1386 "./glsl.g"

case 105: {
    // nothing to do.
}   break;

#line 1393 "./glsl.g"

case 106: {
    sym(1).declaration_list = makeAstNode< List<Declaration *> >
        (sym(1).declaration);
}   break;

#line 1401 "./glsl.g"

case 107: {
    Type *type = VariableDeclaration::declarationType(sym(1).declaration_list);
    Declaration *decl = makeAstNode<VariableDeclaration>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<Declaration *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1411 "./glsl.g"

case 108: {
    Type *type = VariableDeclaration::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayType>(type);
    Declaration *decl = makeAstNode<VariableDeclaration>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<Declaration *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1422 "./glsl.g"

case 109: {
    Type *type = VariableDeclaration::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayType>(type, expression(5));
    Declaration *decl = makeAstNode<VariableDeclaration>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<Declaration *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1433 "./glsl.g"

case 110: {
    Type *type = VariableDeclaration::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayType>(type);
    Declaration *decl = makeAstNode<VariableDeclaration>
            (type, string(3), expression(7));
    sym(1).declaration_list = makeAstNode< List<Declaration *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1445 "./glsl.g"

case 111: {
    Type *type = VariableDeclaration::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayType>(type, expression(5));
    Declaration *decl = makeAstNode<VariableDeclaration>
            (type, string(3), expression(8));
    sym(1).declaration_list = makeAstNode< List<Declaration *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1457 "./glsl.g"

case 112: {
    Type *type = VariableDeclaration::declarationType(sym(1).declaration_list);
    Declaration *decl = makeAstNode<VariableDeclaration>
            (type, string(3), expression(5));
    sym(1).declaration_list = makeAstNode< List<Declaration *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1468 "./glsl.g"

case 113: {
    ast(1) = makeAstNode<TypeDeclaration>(type(1));
}   break;

#line 1475 "./glsl.g"

case 114: {
    ast(1) = makeAstNode<VariableDeclaration>(type(1), string(2));
}   break;

#line 1482 "./glsl.g"

case 115: {
    ast(1) = makeAstNode<VariableDeclaration>
        (makeAstNode<ArrayType>(type(1)), string(2));
}   break;

#line 1490 "./glsl.g"

case 116: {
    ast(1) = makeAstNode<VariableDeclaration>
        (makeAstNode<ArrayType>(type(1), expression(4)), string(2));
}   break;

#line 1498 "./glsl.g"

case 117: {
    ast(1) = makeAstNode<VariableDeclaration>
        (makeAstNode<ArrayType>(type(1)), string(2), expression(6));
}   break;

#line 1506 "./glsl.g"

case 118: {
    ast(1) = makeAstNode<VariableDeclaration>
        (makeAstNode<ArrayType>(type(1), expression(4)),
         string(2), expression(7));
}   break;

#line 1515 "./glsl.g"

case 119: {
    ast(1) = makeAstNode<VariableDeclaration>
        (type(1), string(2), expression(4));
}   break;

#line 1523 "./glsl.g"

case 120: {
    ast(1) = makeAstNode<InvariantDeclaration>(string(2));
}   break;

#line 1530 "./glsl.g"

case 121: {
    ast(1) = makeAstNode<QualifiedType>(0, type(1), (List<LayoutQualifier *> *)0);
}   break;

#line 1537 "./glsl.g"

case 122: {
    ast(1) = makeAstNode<QualifiedType>
        (sym(1).type_qualifier.qualifier, type(2),
         sym(1).type_qualifier.layout_list);
}   break;

#line 1546 "./glsl.g"

case 123: {
    sym(1).qualifier = QualifiedType::Invariant;
}   break;

#line 1553 "./glsl.g"

case 124: {
    sym(1).qualifier = QualifiedType::Smooth;
}   break;

#line 1560 "./glsl.g"

case 125: {
    sym(1).qualifier = QualifiedType::Flat;
}   break;

#line 1567 "./glsl.g"

case 126: {
    sym(1).qualifier = QualifiedType::NoPerspective;
}   break;

#line 1574 "./glsl.g"

case 127: {
    sym(1) = sym(3);
}   break;

#line 1581 "./glsl.g"

case 128: {
    sym(1).layout_list = makeAstNode< List<LayoutQualifier *> >(sym(1).layout);
}   break;

#line 1588 "./glsl.g"

case 129: {
    sym(1).layout_list = makeAstNode< List<LayoutQualifier *> >(sym(1).layout_list, sym(3).layout);
}   break;

#line 1595 "./glsl.g"

case 130: {
    sym(1).layout = makeAstNode<LayoutQualifier>(string(1), (const QString *)0);
}   break;

#line 1602 "./glsl.g"

case 131: {
    sym(1).layout = makeAstNode<LayoutQualifier>(string(1), string(3));
}   break;

#line 1609 "./glsl.g"

case 132: {
    sym(1).qualifier = QualifiedType::Const;
}   break;

#line 1616 "./glsl.g"

case 133: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1624 "./glsl.g"

case 134: {
    sym(1).type_qualifier.layout_list = sym(1).layout_list;
    sym(1).type_qualifier.qualifier = 0;
}   break;

#line 1632 "./glsl.g"

case 135: {
    sym(1).type_qualifier.layout_list = sym(1).layout_list;
    sym(1).type_qualifier.qualifier = sym(2).qualifier;
}   break;

#line 1640 "./glsl.g"

case 136: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1648 "./glsl.g"

case 137: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1656 "./glsl.g"

case 138: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1664 "./glsl.g"

case 139: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier | sym(3).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1672 "./glsl.g"

case 140: {
    sym(1).type_qualifier.qualifier = QualifiedType::Invariant;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1680 "./glsl.g"

case 141: {
    sym(1).qualifier = QualifiedType::Const;
}   break;

#line 1687 "./glsl.g"

case 142: {
    sym(1).qualifier = QualifiedType::Attribute;
}   break;

#line 1694 "./glsl.g"

case 143: {
    sym(1).qualifier = QualifiedType::Varying;
}   break;

#line 1701 "./glsl.g"

case 144: {
    sym(1).qualifier = QualifiedType::CentroidVarying;
}   break;

#line 1708 "./glsl.g"

case 145: {
    sym(1).qualifier = QualifiedType::In;
}   break;

#line 1715 "./glsl.g"

case 146: {
    sym(1).qualifier = QualifiedType::Out;
}   break;

#line 1722 "./glsl.g"

case 147: {
    sym(1).qualifier = QualifiedType::CentroidIn;
}   break;

#line 1729 "./glsl.g"

case 148: {
    sym(1).qualifier = QualifiedType::CentroidOut;
}   break;

#line 1736 "./glsl.g"

case 149: {
    sym(1).qualifier = QualifiedType::PatchIn;
}   break;

#line 1743 "./glsl.g"

case 150: {
    sym(1).qualifier = QualifiedType::PatchOut;
}   break;

#line 1750 "./glsl.g"

case 151: {
    sym(1).qualifier = QualifiedType::SampleIn;
}   break;

#line 1757 "./glsl.g"

case 152: {
    sym(1).qualifier = QualifiedType::SampleOut;
}   break;

#line 1764 "./glsl.g"

case 153: {
    sym(1).qualifier = QualifiedType::Uniform;
}   break;

#line 1771 "./glsl.g"

case 154: {
    // nothing to do.
}   break;

#line 1778 "./glsl.g"

case 155: {
    if (!type(2)->setPrecision(sym(1).precision)) {
        // TODO: issue an error about precision not allowed on this type.
    }
    ast(1) = type(2);
}   break;

#line 1788 "./glsl.g"

case 156: {
    // nothing to do.
}   break;

#line 1795 "./glsl.g"

case 157: {
    ast(1) = makeAstNode<ArrayType>(type(1));
}   break;

#line 1802 "./glsl.g"

case 158: {
    ast(1) = makeAstNode<ArrayType>(type(1), expression(3));
}   break;

#line 1809 "./glsl.g"

case 159: {
    ast(1) = makeBasicType(T_VOID, Type::Void);
}   break;

#line 1816 "./glsl.g"

case 160: {
    ast(1) = makeBasicType(T_FLOAT, Type::Primitive);
}   break;

#line 1823 "./glsl.g"

case 161: {
    ast(1) = makeBasicType(T_DOUBLE, Type::Primitive);
}   break;

#line 1830 "./glsl.g"

case 162: {
    ast(1) = makeBasicType(T_INT, Type::Primitive);
}   break;

#line 1837 "./glsl.g"

case 163: {
    ast(1) = makeBasicType(T_UINT, Type::Primitive);
}   break;

#line 1844 "./glsl.g"

case 164: {
    ast(1) = makeBasicType(T_BOOL, Type::Primitive);
}   break;

#line 1851 "./glsl.g"

case 165: {
    ast(1) = makeBasicType(T_VEC2, Type::Vector2);
}   break;

#line 1858 "./glsl.g"

case 166: {
    ast(1) = makeBasicType(T_VEC3, Type::Vector3);
}   break;

#line 1865 "./glsl.g"

case 167: {
    ast(1) = makeBasicType(T_VEC4, Type::Vector4);
}   break;

#line 1872 "./glsl.g"

case 168: {
    ast(1) = makeBasicType(T_DVEC2, Type::Vector2);
}   break;

#line 1879 "./glsl.g"

case 169: {
    ast(1) = makeBasicType(T_DVEC3, Type::Vector3);
}   break;

#line 1886 "./glsl.g"

case 170: {
    ast(1) = makeBasicType(T_DVEC4, Type::Vector4);
}   break;

#line 1893 "./glsl.g"

case 171: {
    ast(1) = makeBasicType(T_BVEC2, Type::Vector2);
}   break;

#line 1900 "./glsl.g"

case 172: {
    ast(1) = makeBasicType(T_BVEC3, Type::Vector3);
}   break;

#line 1907 "./glsl.g"

case 173: {
    ast(1) = makeBasicType(T_BVEC4, Type::Vector4);
}   break;

#line 1914 "./glsl.g"

case 174: {
    ast(1) = makeBasicType(T_IVEC2, Type::Vector2);
}   break;

#line 1921 "./glsl.g"

case 175: {
    ast(1) = makeBasicType(T_IVEC3, Type::Vector3);
}   break;

#line 1928 "./glsl.g"

case 176: {
    ast(1) = makeBasicType(T_IVEC4, Type::Vector4);
}   break;

#line 1935 "./glsl.g"

case 177: {
    ast(1) = makeBasicType(T_UVEC2, Type::Vector2);
}   break;

#line 1942 "./glsl.g"

case 178: {
    ast(1) = makeBasicType(T_UVEC3, Type::Vector3);
}   break;

#line 1949 "./glsl.g"

case 179: {
    ast(1) = makeBasicType(T_UVEC4, Type::Vector4);
}   break;

#line 1956 "./glsl.g"

case 180: {
    ast(1) = makeBasicType(T_MAT2, Type::Matrix);
}   break;

#line 1963 "./glsl.g"

case 181: {
    ast(1) = makeBasicType(T_MAT3, Type::Matrix);
}   break;

#line 1970 "./glsl.g"

case 182: {
    ast(1) = makeBasicType(T_MAT4, Type::Matrix);
}   break;

#line 1977 "./glsl.g"

case 183: {
    ast(1) = makeBasicType(T_MAT2, Type::Matrix);
}   break;

#line 1984 "./glsl.g"

case 184: {
    ast(1) = makeBasicType(T_MAT2X3, Type::Matrix);
}   break;

#line 1991 "./glsl.g"

case 185: {
    ast(1) = makeBasicType(T_MAT2X4, Type::Matrix);
}   break;

#line 1998 "./glsl.g"

case 186: {
    ast(1) = makeBasicType(T_MAT3X2, Type::Matrix);
}   break;

#line 2005 "./glsl.g"

case 187: {
    ast(1) = makeBasicType(T_MAT3, Type::Matrix);
}   break;

#line 2012 "./glsl.g"

case 188: {
    ast(1) = makeBasicType(T_MAT3X4, Type::Matrix);
}   break;

#line 2019 "./glsl.g"

case 189: {
    ast(1) = makeBasicType(T_MAT4X2, Type::Matrix);
}   break;

#line 2026 "./glsl.g"

case 190: {
    ast(1) = makeBasicType(T_MAT4X3, Type::Matrix);
}   break;

#line 2033 "./glsl.g"

case 191: {
    ast(1) = makeBasicType(T_MAT4, Type::Matrix);
}   break;

#line 2040 "./glsl.g"

case 192: {
    ast(1) = makeBasicType(T_DMAT2, Type::Matrix);
}   break;

#line 2047 "./glsl.g"

case 193: {
    ast(1) = makeBasicType(T_DMAT3, Type::Matrix);
}   break;

#line 2054 "./glsl.g"

case 194: {
    ast(1) = makeBasicType(T_DMAT4, Type::Matrix);
}   break;

#line 2061 "./glsl.g"

case 195: {
    ast(1) = makeBasicType(T_DMAT2, Type::Matrix);
}   break;

#line 2068 "./glsl.g"

case 196: {
    ast(1) = makeBasicType(T_DMAT2X3, Type::Matrix);
}   break;

#line 2075 "./glsl.g"

case 197: {
    ast(1) = makeBasicType(T_DMAT2X4, Type::Matrix);
}   break;

#line 2082 "./glsl.g"

case 198: {
    ast(1) = makeBasicType(T_DMAT3X2, Type::Matrix);
}   break;

#line 2089 "./glsl.g"

case 199: {
    ast(1) = makeBasicType(T_DMAT3, Type::Matrix);
}   break;

#line 2096 "./glsl.g"

case 200: {
    ast(1) = makeBasicType(T_DMAT3X4, Type::Matrix);
}   break;

#line 2103 "./glsl.g"

case 201: {
    ast(1) = makeBasicType(T_DMAT4X2, Type::Matrix);
}   break;

#line 2110 "./glsl.g"

case 202: {
    ast(1) = makeBasicType(T_DMAT4X3, Type::Matrix);
}   break;

#line 2117 "./glsl.g"

case 203: {
    ast(1) = makeBasicType(T_DMAT4, Type::Matrix);
}   break;

#line 2124 "./glsl.g"

case 204: {
    ast(1) = makeBasicType(T_SAMPLER1D, Type::Sampler1D);
}   break;

#line 2131 "./glsl.g"

case 205: {
    ast(1) = makeBasicType(T_SAMPLER2D, Type::Sampler2D);
}   break;

#line 2138 "./glsl.g"

case 206: {
    ast(1) = makeBasicType(T_SAMPLER3D, Type::Sampler3D);
}   break;

#line 2145 "./glsl.g"

case 207: {
    ast(1) = makeBasicType(T_SAMPLERCUBE, Type::SamplerCube);
}   break;

#line 2152 "./glsl.g"

case 208: {
    ast(1) = makeBasicType(T_SAMPLER1DSHADOW, Type::Sampler1DShadow);
}   break;

#line 2159 "./glsl.g"

case 209: {
    ast(1) = makeBasicType(T_SAMPLER2DSHADOW, Type::Sampler2DShadow);
}   break;

#line 2166 "./glsl.g"

case 210: {
    ast(1) = makeBasicType(T_SAMPLERCUBESHADOW, Type::SamplerCubeShadow);
}   break;

#line 2173 "./glsl.g"

case 211: {
    ast(1) = makeBasicType(T_SAMPLER1DARRAY, Type::Sampler1DArray);
}   break;

#line 2180 "./glsl.g"

case 212: {
    ast(1) = makeBasicType(T_SAMPLER2DARRAY, Type::Sampler2DArray);
}   break;

#line 2187 "./glsl.g"

case 213: {
    ast(1) = makeBasicType(T_SAMPLER1DARRAYSHADOW, Type::Sampler1DArrayShadow);
}   break;

#line 2194 "./glsl.g"

case 214: {
    ast(1) = makeBasicType(T_SAMPLER2DARRAYSHADOW, Type::Sampler2DArrayShadow);
}   break;

#line 2201 "./glsl.g"

case 215: {
    ast(1) = makeBasicType(T_SAMPLERCUBEARRAY, Type::SamplerCubeShadow);
}   break;

#line 2208 "./glsl.g"

case 216: {
    ast(1) = makeBasicType(T_SAMPLERCUBEARRAYSHADOW, Type::SamplerCubeArrayShadow);
}   break;

#line 2215 "./glsl.g"

case 217: {
    ast(1) = makeBasicType(T_ISAMPLER1D, Type::Sampler1D);
}   break;

#line 2222 "./glsl.g"

case 218: {
    ast(1) = makeBasicType(T_ISAMPLER2D, Type::Sampler2D);
}   break;

#line 2229 "./glsl.g"

case 219: {
    ast(1) = makeBasicType(T_ISAMPLER3D, Type::Sampler3D);
}   break;

#line 2236 "./glsl.g"

case 220: {
    ast(1) = makeBasicType(T_ISAMPLERCUBE, Type::SamplerCube);
}   break;

#line 2243 "./glsl.g"

case 221: {
    ast(1) = makeBasicType(T_ISAMPLER1DARRAY, Type::Sampler1DArray);
}   break;

#line 2250 "./glsl.g"

case 222: {
    ast(1) = makeBasicType(T_ISAMPLER2DARRAY, Type::Sampler2DArray);
}   break;

#line 2257 "./glsl.g"

case 223: {
    ast(1) = makeBasicType(T_ISAMPLERCUBEARRAY, Type::SamplerCubeArray);
}   break;

#line 2264 "./glsl.g"

case 224: {
    ast(1) = makeBasicType(T_USAMPLER1D, Type::Sampler1D);
}   break;

#line 2271 "./glsl.g"

case 225: {
    ast(1) = makeBasicType(T_USAMPLER2D, Type::Sampler2D);
}   break;

#line 2278 "./glsl.g"

case 226: {
    ast(1) = makeBasicType(T_USAMPLER3D, Type::Sampler3D);
}   break;

#line 2285 "./glsl.g"

case 227: {
    ast(1) = makeBasicType(T_USAMPLERCUBE, Type::SamplerCube);
}   break;

#line 2292 "./glsl.g"

case 228: {
    ast(1) = makeBasicType(T_USAMPLER1DARRAY, Type::Sampler1DArray);
}   break;

#line 2299 "./glsl.g"

case 229: {
    ast(1) = makeBasicType(T_USAMPLER2DARRAY, Type::Sampler2DArray);
}   break;

#line 2306 "./glsl.g"

case 230: {
    ast(1) = makeBasicType(T_USAMPLERCUBEARRAY, Type::SamplerCubeArray);
}   break;

#line 2313 "./glsl.g"

case 231: {
    ast(1) = makeBasicType(T_SAMPLER2DRECT, Type::Sampler2DRect);
}   break;

#line 2320 "./glsl.g"

case 232: {
    ast(1) = makeBasicType(T_SAMPLER2DRECTSHADOW, Type::Sampler2DRectShadow);
}   break;

#line 2327 "./glsl.g"

case 233: {
    ast(1) = makeBasicType(T_ISAMPLER2DRECT, Type::Sampler2DRect);
}   break;

#line 2334 "./glsl.g"

case 234: {
    ast(1) = makeBasicType(T_USAMPLER2DRECT, Type::Sampler2DRect);
}   break;

#line 2341 "./glsl.g"

case 235: {
    ast(1) = makeBasicType(T_SAMPLERBUFFER, Type::SamplerBuffer);
}   break;

#line 2348 "./glsl.g"

case 236: {
    ast(1) = makeBasicType(T_ISAMPLERBUFFER, Type::SamplerBuffer);
}   break;

#line 2355 "./glsl.g"

case 237: {
    ast(1) = makeBasicType(T_USAMPLERBUFFER, Type::SamplerBuffer);
}   break;

#line 2362 "./glsl.g"

case 238: {
    ast(1) = makeBasicType(T_SAMPLER2DMS, Type::Sampler2DMS);
}   break;

#line 2369 "./glsl.g"

case 239: {
    ast(1) = makeBasicType(T_ISAMPLER2DMS, Type::Sampler2DMS);
}   break;

#line 2376 "./glsl.g"

case 240: {
    ast(1) = makeBasicType(T_USAMPLER2DMS, Type::Sampler2DMS);
}   break;

#line 2383 "./glsl.g"

case 241: {
    ast(1) = makeBasicType(T_SAMPLER2DMSARRAY, Type::Sampler2DMSArray);
}   break;

#line 2390 "./glsl.g"

case 242: {
    ast(1) = makeBasicType(T_ISAMPLER2DMSARRAY, Type::Sampler2DMSArray);
}   break;

#line 2397 "./glsl.g"

case 243: {
    ast(1) = makeBasicType(T_USAMPLER2DMSARRAY, Type::Sampler2DMSArray);
}   break;

#line 2404 "./glsl.g"

case 244: {
    // nothing to do.
}   break;

#line 2411 "./glsl.g"

case 245: {
    ast(1) = makeAstNode<NamedType>(string(1));
}   break;

#line 2418 "./glsl.g"

case 246: {
    sym(1).precision = Type::Highp;
}   break;

#line 2425 "./glsl.g"

case 247: {
    sym(1).precision = Type::Mediump;
}   break;

#line 2432 "./glsl.g"

case 248: {
    sym(1).precision = Type::Lowp;
}   break;

#line 2439 "./glsl.g"

case 249: {
    ast(1) = makeAstNode<StructType>(string(2), sym(4).field_list);
}   break;

#line 2446 "./glsl.g"

case 250: {
    ast(1) = makeAstNode<StructType>(sym(3).field_list);
}   break;

#line 2453 "./glsl.g"

case 251: {
    // nothing to do.
}   break;

#line 2460 "./glsl.g"

case 252: {
    sym(1).field_list = appendLists(sym(1).field_list, sym(2).field_list);
}   break;

#line 2467 "./glsl.g"

case 253: {
    sym(1).field_list = StructType::fixInnerTypes(type(1), sym(2).field_list);
}   break;

#line 2474 "./glsl.g"

case 254: {
    sym(1).field_list = StructType::fixInnerTypes
        (makeAstNode<QualifiedType>
            (sym(1).type_qualifier.qualifier, type(2),
             sym(1).type_qualifier.layout_list), sym(3).field_list);
}   break;

#line 2484 "./glsl.g"

case 255: {
    // nothing to do.
    sym(1).field_list = makeAstNode< List<StructType::Field *> >(sym(1).field);
}   break;

#line 2492 "./glsl.g"

case 256: {
    sym(1).field_list = makeAstNode< List<StructType::Field *> >(sym(1).field_list, sym(3).field);
}   break;

#line 2499 "./glsl.g"

case 257: {
    sym(1).field = makeAstNode<StructType::Field>(string(1));
}   break;

#line 2506 "./glsl.g"

case 258: {
    sym(1).field = makeAstNode<StructType::Field>
        (string(1), makeAstNode<ArrayType>((Type *)0));
}   break;

#line 2514 "./glsl.g"

case 259: {
    sym(1).field = makeAstNode<StructType::Field>
        (string(1), makeAstNode<ArrayType>((Type *)0, expression(3)));
}   break;

#line 2522 "./glsl.g"

case 260: {
    // nothing to do.
}   break;

#line 2529 "./glsl.g"

case 261: {
    ast(1) = makeAstNode<DeclarationStatement>(sym(1).declaration_list);
}   break;

#line 2536 "./glsl.g"

case 262: {
    // nothing to do.
}   break;

#line 2543 "./glsl.g"

case 263: {
    // nothing to do.
}   break;

#line 2550 "./glsl.g"

case 264: {
    // nothing to do.
}   break;

#line 2557 "./glsl.g"

case 265: {
    // nothing to do.
}   break;

#line 2564 "./glsl.g"

case 266: {
    // nothing to do.
}   break;

#line 2571 "./glsl.g"

case 267: {
    // nothing to do.
}   break;

#line 2578 "./glsl.g"

case 268: {
    // nothing to do.
}   break;

#line 2585 "./glsl.g"

case 269: {
    // nothing to do.
}   break;

#line 2592 "./glsl.g"

case 270: {
    // nothing to do.
}   break;

#line 2599 "./glsl.g"

case 271: {
    ast(1) = makeAstNode<CompoundStatement>();
}   break;

#line 2606 "./glsl.g"

case 272: {
    ast(1) = makeAstNode<CompoundStatement>(sym(2).statement_list);
}   break;

#line 2613 "./glsl.g"

case 273: {
    // nothing to do.
}   break;

#line 2620 "./glsl.g"

case 274: {
    // nothing to do.
}   break;

#line 2627 "./glsl.g"

case 275: {
    ast(1) = makeAstNode<CompoundStatement>();
}   break;

#line 2634 "./glsl.g"

case 276: {
    ast(1) = makeAstNode<CompoundStatement>(sym(2).statement_list);
}   break;

#line 2641 "./glsl.g"

case 277: {
    sym(1).statement_list = makeAstNode< List<Statement *> >(sym(1).statement);
}   break;

#line 2648 "./glsl.g"

case 278: {
    sym(1).statement_list = makeAstNode< List<Statement *> >(sym(1).statement_list, sym(2).statement);
}   break;

#line 2655 "./glsl.g"

case 279: {
    ast(1) = makeAstNode<CompoundStatement>();  // Empty statement
}   break;

#line 2662 "./glsl.g"

case 280: {
    ast(1) = makeAstNode<ExpressionStatement>(expression(1));
}   break;

#line 2669 "./glsl.g"

case 281: {
    ast(1) = makeAstNode<IfStatement>(expression(3), sym(5).ifstmt.thenClause, sym(5).ifstmt.elseClause);
}   break;

#line 2676 "./glsl.g"

case 282: {
    sym(1).ifstmt.thenClause = statement(1);
    sym(1).ifstmt.elseClause = statement(3);
}   break;

#line 2684 "./glsl.g"

case 283: {
    sym(1).ifstmt.thenClause = statement(1);
    sym(1).ifstmt.elseClause = 0;
}   break;

#line 2692 "./glsl.g"

case 284: {
    // nothing to do.
}   break;

#line 2699 "./glsl.g"

case 285: {
    ast(1) = makeAstNode<DeclarationExpression>
        (type(1), string(2), expression(4));
}   break;

#line 2707 "./glsl.g"

case 286: {
    ast(1) = makeAstNode<SwitchStatement>(expression(3), statement(6));
}   break;

#line 2714 "./glsl.g"

case 287: {
    ast(1) = makeAstNode<CompoundStatement>();
}   break;

#line 2721 "./glsl.g"

case 288: {
    ast(1) = makeAstNode<CompoundStatement>(sym(1).statement_list);
}   break;

#line 2728 "./glsl.g"

case 289: {
    ast(1) = makeAstNode<CaseLabelStatement>(expression(2));
}   break;

#line 2735 "./glsl.g"

case 290: {
    ast(1) = makeAstNode<CaseLabelStatement>();
}   break;

#line 2742 "./glsl.g"

case 291: {
    ast(1) = makeAstNode<WhileStatement>(expression(3), statement(5));
}   break;

#line 2749 "./glsl.g"

case 292: {
    ast(1) = makeAstNode<DoStatement>(statement(2), expression(5));
}   break;

#line 2756 "./glsl.g"

case 293: {
    ast(1) = makeAstNode<ForStatement>(statement(3), sym(4).forstmt.condition, sym(4).forstmt.increment, statement(6));
}   break;

#line 2763 "./glsl.g"

case 294: {
    // nothing to do.
}   break;

#line 2770 "./glsl.g"

case 295: {
    // nothing to do.
}   break;

#line 2777 "./glsl.g"

case 296: {
    // nothing to do.
}   break;

#line 2784 "./glsl.g"

case 297: {
    // nothing to do.
}   break;

#line 2791 "./glsl.g"

case 298: {
    sym(1).forstmt.condition = expression(1);
    sym(1).forstmt.increment = 0;
}   break;

#line 2799 "./glsl.g"

case 299: {
    sym(1).forstmt.condition = expression(1);
    sym(1).forstmt.increment = expression(3);
}   break;

#line 2807 "./glsl.g"

case 300: {
    ast(1) = makeAstNode<JumpStatement>(AST::Kind_Continue);
}   break;

#line 2814 "./glsl.g"

case 301: {
    ast(1) = makeAstNode<JumpStatement>(AST::Kind_Break);
}   break;

#line 2821 "./glsl.g"

case 302: {
    ast(1) = makeAstNode<ReturnStatement>();
}   break;

#line 2828 "./glsl.g"

case 303: {
    ast(1) = makeAstNode<ReturnStatement>(expression(2));
}   break;

#line 2835 "./glsl.g"

case 304: {
    ast(1) = makeAstNode<JumpStatement>(AST::Kind_Discard);
}   break;

#line 2842 "./glsl.g"

case 305: {
    ast(1) = makeAstNode<TranslationUnit>(sym(1).declaration_list);
}   break;

#line 2849 "./glsl.g"

case 306: {
    if (sym(1).declaration) {
        sym(1).declaration_list = makeAstNode< List<Declaration *> >
            (sym(1).declaration);
    } else {
        sym(1).declaration_list = 0;
    }
}   break;

#line 2861 "./glsl.g"

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

#line 2878 "./glsl.g"

case 308: {
    // nothing to do.
}   break;

#line 2885 "./glsl.g"

case 309: {
    // nothing to do.
}   break;

#line 2892 "./glsl.g"

case 310: {
    ast(1) = 0;
}   break;

#line 2899 "./glsl.g"

case 311: {
    function(1)->body = statement(2);
}   break;

#line 2906 "./glsl.g"

case 312: {
    ast(1) = 0;
}   break;

#line 2914 "./glsl.g"

} // end switch
} // end Parser::reduce()
