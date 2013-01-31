
#line 423 "./glsl.g"

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

#line 641 "./glsl.g"

void Parser::reduce(int ruleno)
{
switch(ruleno) {

#line 650 "./glsl.g"

case 0: {
    ast(1) = makeAstNode<IdentifierExpressionAST>(string(1));
}   break;

#line 657 "./glsl.g"

case 1: {
    ast(1) = makeAstNode<LiteralExpressionAST>(string(1));
}   break;

#line 664 "./glsl.g"

case 2: {
    ast(1) = makeAstNode<LiteralExpressionAST>(_engine->identifier("true", 4));
}   break;

#line 671 "./glsl.g"

case 3: {
    ast(1) = makeAstNode<LiteralExpressionAST>(_engine->identifier("false", 5));
}   break;

#line 678 "./glsl.g"

case 4: {
    // nothing to do.
}   break;

#line 685 "./glsl.g"

case 5: {
    ast(1) = ast(2);
}   break;

#line 692 "./glsl.g"

case 6: {
    // nothing to do.
}   break;

#line 699 "./glsl.g"

case 7: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ArrayAccess, expression(1), expression(3));
}   break;

#line 706 "./glsl.g"

case 8: {
    // nothing to do.
}   break;

#line 713 "./glsl.g"

case 9: {
    ast(1) = makeAstNode<MemberAccessExpressionAST>(expression(1), string(3));
}   break;

#line 720 "./glsl.g"

case 10: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PostIncrement, expression(1));
}   break;

#line 727 "./glsl.g"

case 11: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PostDecrement, expression(1));
}   break;

#line 734 "./glsl.g"

case 12: {
    // nothing to do.
}   break;

#line 741 "./glsl.g"

case 13: {
    // nothing to do.
}   break;

#line 748 "./glsl.g"

case 14: {
    ast(1) = makeAstNode<FunctionCallExpressionAST>
        (sym(1).function.id, sym(1).function.arguments);
}   break;

#line 756 "./glsl.g"

case 15: {
    ast(1) = makeAstNode<FunctionCallExpressionAST>
        (expression(1), sym(3).function.id, sym(3).function.arguments);
}   break;

#line 764 "./glsl.g"

case 16: {
    // nothing to do.
}   break;

#line 771 "./glsl.g"

case 17: {
    // nothing to do.
}   break;

#line 778 "./glsl.g"

case 18: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments = 0;
}   break;

#line 786 "./glsl.g"

case 19: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments = 0;
}   break;

#line 794 "./glsl.g"

case 20: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments =
        makeAstNode< List<ExpressionAST *> >(expression(2));
}   break;

#line 803 "./glsl.g"

case 21: {
    sym(1).function.arguments =
        makeAstNode< List<ExpressionAST *> >
            (sym(1).function.arguments, expression(3));
}   break;

#line 812 "./glsl.g"

case 22: {
    // nothing to do.
}   break;

#line 819 "./glsl.g"

case 23: {
    ast(1) = makeAstNode<FunctionIdentifierAST>(type(1));
}   break;

#line 826 "./glsl.g"

case 24: {
    ast(1) = makeAstNode<FunctionIdentifierAST>(string(1));
}   break;

#line 833 "./glsl.g"

case 25: {
    // nothing to do.
}   break;

#line 840 "./glsl.g"

case 26: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PreIncrement, expression(2));
}   break;

#line 847 "./glsl.g"

case 27: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PreDecrement, expression(2));
}   break;

#line 854 "./glsl.g"

case 28: {
    ast(1) = makeAstNode<UnaryExpressionAST>(sym(1).kind, expression(2));
}   break;

#line 861 "./glsl.g"

case 29: {
    sym(1).kind = AST::Kind_UnaryPlus;
}   break;

#line 868 "./glsl.g"

case 30: {
    sym(1).kind = AST::Kind_UnaryMinus;
}   break;

#line 875 "./glsl.g"

case 31: {
    sym(1).kind = AST::Kind_LogicalNot;
}   break;

#line 882 "./glsl.g"

case 32: {
    sym(1).kind = AST::Kind_BitwiseNot;
}   break;

#line 889 "./glsl.g"

case 33: {
    // nothing to do.
}   break;

#line 896 "./glsl.g"

case 34: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Multiply, expression(1), expression(3));
}   break;

#line 903 "./glsl.g"

case 35: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Divide, expression(1), expression(3));
}   break;

#line 910 "./glsl.g"

case 36: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Modulus, expression(1), expression(3));
}   break;

#line 917 "./glsl.g"

case 37: {
    // nothing to do.
}   break;

#line 924 "./glsl.g"

case 38: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Plus, expression(1), expression(3));
}   break;

#line 931 "./glsl.g"

case 39: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Minus, expression(1), expression(3));
}   break;

#line 938 "./glsl.g"

case 40: {
    // nothing to do.
}   break;

#line 945 "./glsl.g"

case 41: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ShiftLeft, expression(1), expression(3));
}   break;

#line 952 "./glsl.g"

case 42: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ShiftRight, expression(1), expression(3));
}   break;

#line 959 "./glsl.g"

case 43: {
    // nothing to do.
}   break;

#line 966 "./glsl.g"

case 44: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LessThan, expression(1), expression(3));
}   break;

#line 973 "./glsl.g"

case 45: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_GreaterThan, expression(1), expression(3));
}   break;

#line 980 "./glsl.g"

case 46: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LessEqual, expression(1), expression(3));
}   break;

#line 987 "./glsl.g"

case 47: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_GreaterEqual, expression(1), expression(3));
}   break;

#line 994 "./glsl.g"

case 48: {
    // nothing to do.
}   break;

#line 1001 "./glsl.g"

case 49: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Equal, expression(1), expression(3));
}   break;

#line 1008 "./glsl.g"

case 50: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_NotEqual, expression(1), expression(3));
}   break;

#line 1015 "./glsl.g"

case 51: {
    // nothing to do.
}   break;

#line 1022 "./glsl.g"

case 52: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseAnd, expression(1), expression(3));
}   break;

#line 1029 "./glsl.g"

case 53: {
    // nothing to do.
}   break;

#line 1036 "./glsl.g"

case 54: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseXor, expression(1), expression(3));
}   break;

#line 1043 "./glsl.g"

case 55: {
    // nothing to do.
}   break;

#line 1050 "./glsl.g"

case 56: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseOr, expression(1), expression(3));
}   break;

#line 1057 "./glsl.g"

case 57: {
    // nothing to do.
}   break;

#line 1064 "./glsl.g"

case 58: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalAnd, expression(1), expression(3));
}   break;

#line 1071 "./glsl.g"

case 59: {
    // nothing to do.
}   break;

#line 1078 "./glsl.g"

case 60: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalXor, expression(1), expression(3));
}   break;

#line 1085 "./glsl.g"

case 61: {
    // nothing to do.
}   break;

#line 1092 "./glsl.g"

case 62: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalOr, expression(1), expression(3));
}   break;

#line 1099 "./glsl.g"

case 63: {
    // nothing to do.
}   break;

#line 1106 "./glsl.g"

case 64: {
    ast(1) = makeAstNode<TernaryExpressionAST>(AST::Kind_Conditional, expression(1), expression(3), expression(5));
}   break;

#line 1113 "./glsl.g"

case 65: {
    // nothing to do.
}   break;

#line 1120 "./glsl.g"

case 66: {
    ast(1) = makeAstNode<AssignmentExpressionAST>(sym(2).kind, expression(1), expression(3));
}   break;

#line 1127 "./glsl.g"

case 67: {
    sym(1).kind = AST::Kind_Assign;
}   break;

#line 1134 "./glsl.g"

case 68: {
    sym(1).kind = AST::Kind_AssignMultiply;
}   break;

#line 1141 "./glsl.g"

case 69: {
    sym(1).kind = AST::Kind_AssignDivide;
}   break;

#line 1148 "./glsl.g"

case 70: {
    sym(1).kind = AST::Kind_AssignModulus;
}   break;

#line 1155 "./glsl.g"

case 71: {
    sym(1).kind = AST::Kind_AssignPlus;
}   break;

#line 1162 "./glsl.g"

case 72: {
    sym(1).kind = AST::Kind_AssignMinus;
}   break;

#line 1169 "./glsl.g"

case 73: {
    sym(1).kind = AST::Kind_AssignShiftLeft;
}   break;

#line 1176 "./glsl.g"

case 74: {
    sym(1).kind = AST::Kind_AssignShiftRight;
}   break;

#line 1183 "./glsl.g"

case 75: {
    sym(1).kind = AST::Kind_AssignAnd;
}   break;

#line 1190 "./glsl.g"

case 76: {
    sym(1).kind = AST::Kind_AssignXor;
}   break;

#line 1197 "./glsl.g"

case 77: {
    sym(1).kind = AST::Kind_AssignOr;
}   break;

#line 1204 "./glsl.g"

case 78: {
    // nothing to do.
}   break;

#line 1211 "./glsl.g"

case 79: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Comma, expression(1), expression(3));
}   break;

#line 1218 "./glsl.g"

case 80: {
    // nothing to do.
}   break;

#line 1225 "./glsl.g"

case 81: {
    // nothing to do.
}   break;

#line 1232 "./glsl.g"

case 82: {
    ast(1) = makeAstNode<InitDeclarationAST>(sym(1).declaration_list);
}   break;

#line 1239 "./glsl.g"

case 83: {
    ast(1) = makeAstNode<PrecisionDeclarationAST>(sym(2).precision, type(3));
}   break;

#line 1246 "./glsl.g"

case 84: {
    if (sym(1).type_qualifier.qualifier != QualifiedTypeAST::Struct) {
        // TODO: issue an error if the qualifier is not "struct".
    }
    TypeAST *type = makeAstNode<StructTypeAST>(string(2), sym(4).field_list);
    ast(1) = makeAstNode<TypeDeclarationAST>(type);
}   break;

#line 1257 "./glsl.g"

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

#line 1276 "./glsl.g"

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

#line 1296 "./glsl.g"

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

#line 1316 "./glsl.g"

case 88: {
    TypeAST *type = makeAstNode<QualifiedTypeAST>
        (sym(1).type_qualifier.qualifier, (TypeAST *)0,
         sym(1).type_qualifier.layout_list);
    ast(1) = makeAstNode<TypeDeclarationAST>(type);
}   break;

#line 1326 "./glsl.g"

case 89: {
    function(1)->finishParams();
}   break;

#line 1333 "./glsl.g"

case 90: {
    // nothing to do.
}   break;

#line 1340 "./glsl.g"

case 91: {
    // nothing to do.
}   break;

#line 1347 "./glsl.g"

case 92: {
    function(1)->params = makeAstNode< List<ParameterDeclarationAST *> >
        (sym(2).param_declaration);
}   break;

#line 1355 "./glsl.g"

case 93: {
    function(1)->params = makeAstNode< List<ParameterDeclarationAST *> >
        (function(1)->params, sym(3).param_declaration);
}   break;

#line 1363 "./glsl.g"

case 94: {
    function(1) = makeAstNode<FunctionDeclarationAST>(type(1), string(2));
}   break;

#line 1370 "./glsl.g"

case 95: {
    sym(1).param_declarator.type = type(1);
    sym(1).param_declarator.name = string(2);
}   break;

#line 1378 "./glsl.g"

case 96: {
    sym(1).param_declarator.type = makeAstNode<ArrayTypeAST>(type(1), expression(4));
    sym(1).param_declarator.name = string(2);
}   break;

#line 1386 "./glsl.g"

case 97: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (makeAstNode<QualifiedTypeAST>
            (sym(1).qualifier, sym(3).param_declarator.type,
             (List<LayoutQualifierAST *> *)0),
         ParameterDeclarationAST::Qualifier(sym(2).qualifier),
         sym(3).param_declarator.name);
}   break;

#line 1398 "./glsl.g"

case 98: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (sym(2).param_declarator.type,
         ParameterDeclarationAST::Qualifier(sym(1).qualifier),
         sym(2).param_declarator.name);
}   break;

#line 1408 "./glsl.g"

case 99: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (makeAstNode<QualifiedTypeAST>
            (sym(1).qualifier, type(3), (List<LayoutQualifierAST *> *)0),
         ParameterDeclarationAST::Qualifier(sym(2).qualifier),
         (const QString *)0);
}   break;

#line 1419 "./glsl.g"

case 100: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (type(2), ParameterDeclarationAST::Qualifier(sym(1).qualifier),
         (const QString *)0);
}   break;

#line 1428 "./glsl.g"

case 101: {
    sym(1).qualifier = ParameterDeclarationAST::In;
}   break;

#line 1435 "./glsl.g"

case 102: {
    sym(1).qualifier = ParameterDeclarationAST::In;
}   break;

#line 1442 "./glsl.g"

case 103: {
    sym(1).qualifier = ParameterDeclarationAST::Out;
}   break;

#line 1449 "./glsl.g"

case 104: {
    sym(1).qualifier = ParameterDeclarationAST::InOut;
}   break;

#line 1456 "./glsl.g"

case 105: {
    // nothing to do.
}   break;

#line 1463 "./glsl.g"

case 106: {
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
        (sym(1).declaration);
}   break;

#line 1471 "./glsl.g"

case 107: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1481 "./glsl.g"

case 108: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1492 "./glsl.g"

case 109: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type, expression(5));
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1503 "./glsl.g"

case 110: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>
            (type, string(3), expression(7));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1515 "./glsl.g"

case 111: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type, expression(5));
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>
            (type, string(3), expression(8));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1527 "./glsl.g"

case 112: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>
            (type, string(3), expression(5));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1538 "./glsl.g"

case 113: {
    ast(1) = makeAstNode<TypeDeclarationAST>(type(1));
}   break;

#line 1545 "./glsl.g"

case 114: {
    ast(1) = makeAstNode<VariableDeclarationAST>(type(1), string(2));
}   break;

#line 1552 "./glsl.g"

case 115: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1)), string(2));
}   break;

#line 1560 "./glsl.g"

case 116: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1), expression(4)), string(2));
}   break;

#line 1568 "./glsl.g"

case 117: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1)), string(2), expression(6));
}   break;

#line 1576 "./glsl.g"

case 118: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1), expression(4)),
         string(2), expression(7));
}   break;

#line 1585 "./glsl.g"

case 119: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (type(1), string(2), expression(4));
}   break;

#line 1593 "./glsl.g"

case 120: {
    ast(1) = makeAstNode<InvariantDeclarationAST>(string(2));
}   break;

#line 1600 "./glsl.g"

case 121: {
    ast(1) = makeAstNode<QualifiedTypeAST>(0, type(1), (List<LayoutQualifierAST *> *)0);
}   break;

#line 1607 "./glsl.g"

case 122: {
    ast(1) = makeAstNode<QualifiedTypeAST>
        (sym(1).type_qualifier.qualifier, type(2),
         sym(1).type_qualifier.layout_list);
}   break;

#line 1616 "./glsl.g"

case 123: {
    sym(1).qualifier = QualifiedTypeAST::Invariant;
}   break;

#line 1623 "./glsl.g"

case 124: {
    sym(1).qualifier = QualifiedTypeAST::Smooth;
}   break;

#line 1630 "./glsl.g"

case 125: {
    sym(1).qualifier = QualifiedTypeAST::Flat;
}   break;

#line 1637 "./glsl.g"

case 126: {
    sym(1).qualifier = QualifiedTypeAST::NoPerspective;
}   break;

#line 1644 "./glsl.g"

case 127: {
    sym(1) = sym(3);
}   break;

#line 1651 "./glsl.g"

case 128: {
    sym(1).layout_list = makeAstNode< List<LayoutQualifierAST *> >(sym(1).layout);
}   break;

#line 1658 "./glsl.g"

case 129: {
    sym(1).layout_list = makeAstNode< List<LayoutQualifierAST *> >(sym(1).layout_list, sym(3).layout);
}   break;

#line 1665 "./glsl.g"

case 130: {
    sym(1).layout = makeAstNode<LayoutQualifierAST>(string(1), (const QString *)0);
}   break;

#line 1672 "./glsl.g"

case 131: {
    sym(1).layout = makeAstNode<LayoutQualifierAST>(string(1), string(3));
}   break;

#line 1679 "./glsl.g"

case 132: {
    sym(1).qualifier = QualifiedTypeAST::Const;
}   break;

#line 1686 "./glsl.g"

case 133: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1694 "./glsl.g"

case 134: {
    sym(1).type_qualifier.layout_list = sym(1).layout_list;
    sym(1).type_qualifier.qualifier = 0;
}   break;

#line 1702 "./glsl.g"

case 135: {
    sym(1).type_qualifier.layout_list = sym(1).layout_list;
    sym(1).type_qualifier.qualifier = sym(2).qualifier;
}   break;

#line 1710 "./glsl.g"

case 136: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1718 "./glsl.g"

case 137: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1726 "./glsl.g"

case 138: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1734 "./glsl.g"

case 139: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier | sym(3).qualifier;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1742 "./glsl.g"

case 140: {
    sym(1).type_qualifier.qualifier = QualifiedTypeAST::Invariant;
    sym(1).type_qualifier.layout_list = 0;
}   break;

#line 1750 "./glsl.g"

case 141: {
    sym(1).qualifier = QualifiedTypeAST::Const;
}   break;

#line 1757 "./glsl.g"

case 142: {
    sym(1).qualifier = QualifiedTypeAST::Attribute;
}   break;

#line 1764 "./glsl.g"

case 143: {
    sym(1).qualifier = QualifiedTypeAST::Varying;
}   break;

#line 1771 "./glsl.g"

case 144: {
    sym(1).qualifier = QualifiedTypeAST::CentroidVarying;
}   break;

#line 1778 "./glsl.g"

case 145: {
    sym(1).qualifier = QualifiedTypeAST::In;
}   break;

#line 1785 "./glsl.g"

case 146: {
    sym(1).qualifier = QualifiedTypeAST::Out;
}   break;

#line 1792 "./glsl.g"

case 147: {
    sym(1).qualifier = QualifiedTypeAST::CentroidIn;
}   break;

#line 1799 "./glsl.g"

case 148: {
    sym(1).qualifier = QualifiedTypeAST::CentroidOut;
}   break;

#line 1806 "./glsl.g"

case 149: {
    sym(1).qualifier = QualifiedTypeAST::PatchIn;
}   break;

#line 1813 "./glsl.g"

case 150: {
    sym(1).qualifier = QualifiedTypeAST::PatchOut;
}   break;

#line 1820 "./glsl.g"

case 151: {
    sym(1).qualifier = QualifiedTypeAST::SampleIn;
}   break;

#line 1827 "./glsl.g"

case 152: {
    sym(1).qualifier = QualifiedTypeAST::SampleOut;
}   break;

#line 1834 "./glsl.g"

case 153: {
    sym(1).qualifier = QualifiedTypeAST::Uniform;
}   break;

#line 1841 "./glsl.g"

case 154: {
    // nothing to do.
}   break;

#line 1848 "./glsl.g"

case 155: {
    if (!type(2)->setPrecision(sym(1).precision)) {
        // TODO: issue an error about precision not allowed on this type.
    }
    ast(1) = type(2);
}   break;

#line 1858 "./glsl.g"

case 156: {
    // nothing to do.
}   break;

#line 1865 "./glsl.g"

case 157: {
    ast(1) = makeAstNode<ArrayTypeAST>(type(1));
}   break;

#line 1872 "./glsl.g"

case 158: {
    ast(1) = makeAstNode<ArrayTypeAST>(type(1), expression(3));
}   break;

#line 1879 "./glsl.g"

case 159: {
    ast(1) = makeBasicType(T_VOID);
}   break;

#line 1886 "./glsl.g"

case 160: {
    ast(1) = makeBasicType(T_FLOAT);
}   break;

#line 1893 "./glsl.g"

case 161: {
    ast(1) = makeBasicType(T_DOUBLE);
}   break;

#line 1900 "./glsl.g"

case 162: {
    ast(1) = makeBasicType(T_INT);
}   break;

#line 1907 "./glsl.g"

case 163: {
    ast(1) = makeBasicType(T_UINT);
}   break;

#line 1914 "./glsl.g"

case 164: {
    ast(1) = makeBasicType(T_BOOL);
}   break;

#line 1921 "./glsl.g"

case 165: {
    ast(1) = makeBasicType(T_VEC2);
}   break;

#line 1928 "./glsl.g"

case 166: {
    ast(1) = makeBasicType(T_VEC3);
}   break;

#line 1935 "./glsl.g"

case 167: {
    ast(1) = makeBasicType(T_VEC4);
}   break;

#line 1942 "./glsl.g"

case 168: {
    ast(1) = makeBasicType(T_DVEC2);
}   break;

#line 1949 "./glsl.g"

case 169: {
    ast(1) = makeBasicType(T_DVEC3);
}   break;

#line 1956 "./glsl.g"

case 170: {
    ast(1) = makeBasicType(T_DVEC4);
}   break;

#line 1963 "./glsl.g"

case 171: {
    ast(1) = makeBasicType(T_BVEC2);
}   break;

#line 1970 "./glsl.g"

case 172: {
    ast(1) = makeBasicType(T_BVEC3);
}   break;

#line 1977 "./glsl.g"

case 173: {
    ast(1) = makeBasicType(T_BVEC4);
}   break;

#line 1984 "./glsl.g"

case 174: {
    ast(1) = makeBasicType(T_IVEC2);
}   break;

#line 1991 "./glsl.g"

case 175: {
    ast(1) = makeBasicType(T_IVEC3);
}   break;

#line 1998 "./glsl.g"

case 176: {
    ast(1) = makeBasicType(T_IVEC4);
}   break;

#line 2005 "./glsl.g"

case 177: {
    ast(1) = makeBasicType(T_UVEC2);
}   break;

#line 2012 "./glsl.g"

case 178: {
    ast(1) = makeBasicType(T_UVEC3);
}   break;

#line 2019 "./glsl.g"

case 179: {
    ast(1) = makeBasicType(T_UVEC4);
}   break;

#line 2026 "./glsl.g"

case 180: {
    ast(1) = makeBasicType(T_MAT2);
}   break;

#line 2033 "./glsl.g"

case 181: {
    ast(1) = makeBasicType(T_MAT3);
}   break;

#line 2040 "./glsl.g"

case 182: {
    ast(1) = makeBasicType(T_MAT4);
}   break;

#line 2047 "./glsl.g"

case 183: {
    ast(1) = makeBasicType(T_MAT2);
}   break;

#line 2054 "./glsl.g"

case 184: {
    ast(1) = makeBasicType(T_MAT2X3);
}   break;

#line 2061 "./glsl.g"

case 185: {
    ast(1) = makeBasicType(T_MAT2X4);
}   break;

#line 2068 "./glsl.g"

case 186: {
    ast(1) = makeBasicType(T_MAT3X2);
}   break;

#line 2075 "./glsl.g"

case 187: {
    ast(1) = makeBasicType(T_MAT3);
}   break;

#line 2082 "./glsl.g"

case 188: {
    ast(1) = makeBasicType(T_MAT3X4);
}   break;

#line 2089 "./glsl.g"

case 189: {
    ast(1) = makeBasicType(T_MAT4X2);
}   break;

#line 2096 "./glsl.g"

case 190: {
    ast(1) = makeBasicType(T_MAT4X3);
}   break;

#line 2103 "./glsl.g"

case 191: {
    ast(1) = makeBasicType(T_MAT4);
}   break;

#line 2110 "./glsl.g"

case 192: {
    ast(1) = makeBasicType(T_DMAT2);
}   break;

#line 2117 "./glsl.g"

case 193: {
    ast(1) = makeBasicType(T_DMAT3);
}   break;

#line 2124 "./glsl.g"

case 194: {
    ast(1) = makeBasicType(T_DMAT4);
}   break;

#line 2131 "./glsl.g"

case 195: {
    ast(1) = makeBasicType(T_DMAT2);
}   break;

#line 2138 "./glsl.g"

case 196: {
    ast(1) = makeBasicType(T_DMAT2X3);
}   break;

#line 2145 "./glsl.g"

case 197: {
    ast(1) = makeBasicType(T_DMAT2X4);
}   break;

#line 2152 "./glsl.g"

case 198: {
    ast(1) = makeBasicType(T_DMAT3X2);
}   break;

#line 2159 "./glsl.g"

case 199: {
    ast(1) = makeBasicType(T_DMAT3);
}   break;

#line 2166 "./glsl.g"

case 200: {
    ast(1) = makeBasicType(T_DMAT3X4);
}   break;

#line 2173 "./glsl.g"

case 201: {
    ast(1) = makeBasicType(T_DMAT4X2);
}   break;

#line 2180 "./glsl.g"

case 202: {
    ast(1) = makeBasicType(T_DMAT4X3);
}   break;

#line 2187 "./glsl.g"

case 203: {
    ast(1) = makeBasicType(T_DMAT4);
}   break;

#line 2194 "./glsl.g"

case 204: {
    ast(1) = makeBasicType(T_SAMPLER1D);
}   break;

#line 2201 "./glsl.g"

case 205: {
    ast(1) = makeBasicType(T_SAMPLER2D);
}   break;

#line 2208 "./glsl.g"

case 206: {
    ast(1) = makeBasicType(T_SAMPLER3D);
}   break;

#line 2215 "./glsl.g"

case 207: {
    ast(1) = makeBasicType(T_SAMPLERCUBE);
}   break;

#line 2222 "./glsl.g"

case 208: {
    ast(1) = makeBasicType(T_SAMPLER1DSHADOW);
}   break;

#line 2229 "./glsl.g"

case 209: {
    ast(1) = makeBasicType(T_SAMPLER2DSHADOW);
}   break;

#line 2236 "./glsl.g"

case 210: {
    ast(1) = makeBasicType(T_SAMPLERCUBESHADOW);
}   break;

#line 2243 "./glsl.g"

case 211: {
    ast(1) = makeBasicType(T_SAMPLER1DARRAY);
}   break;

#line 2250 "./glsl.g"

case 212: {
    ast(1) = makeBasicType(T_SAMPLER2DARRAY);
}   break;

#line 2257 "./glsl.g"

case 213: {
    ast(1) = makeBasicType(T_SAMPLER1DARRAYSHADOW);
}   break;

#line 2264 "./glsl.g"

case 214: {
    ast(1) = makeBasicType(T_SAMPLER2DARRAYSHADOW);
}   break;

#line 2271 "./glsl.g"

case 215: {
    ast(1) = makeBasicType(T_SAMPLERCUBEARRAY);
}   break;

#line 2278 "./glsl.g"

case 216: {
    ast(1) = makeBasicType(T_SAMPLERCUBEARRAYSHADOW);
}   break;

#line 2285 "./glsl.g"

case 217: {
    ast(1) = makeBasicType(T_ISAMPLER1D);
}   break;

#line 2292 "./glsl.g"

case 218: {
    ast(1) = makeBasicType(T_ISAMPLER2D);
}   break;

#line 2299 "./glsl.g"

case 219: {
    ast(1) = makeBasicType(T_ISAMPLER3D);
}   break;

#line 2306 "./glsl.g"

case 220: {
    ast(1) = makeBasicType(T_ISAMPLERCUBE);
}   break;

#line 2313 "./glsl.g"

case 221: {
    ast(1) = makeBasicType(T_ISAMPLER1DARRAY);
}   break;

#line 2320 "./glsl.g"

case 222: {
    ast(1) = makeBasicType(T_ISAMPLER2DARRAY);
}   break;

#line 2327 "./glsl.g"

case 223: {
    ast(1) = makeBasicType(T_ISAMPLERCUBEARRAY);
}   break;

#line 2334 "./glsl.g"

case 224: {
    ast(1) = makeBasicType(T_USAMPLER1D);
}   break;

#line 2341 "./glsl.g"

case 225: {
    ast(1) = makeBasicType(T_USAMPLER2D);
}   break;

#line 2348 "./glsl.g"

case 226: {
    ast(1) = makeBasicType(T_USAMPLER3D);
}   break;

#line 2355 "./glsl.g"

case 227: {
    ast(1) = makeBasicType(T_USAMPLERCUBE);
}   break;

#line 2362 "./glsl.g"

case 228: {
    ast(1) = makeBasicType(T_USAMPLER1DARRAY);
}   break;

#line 2369 "./glsl.g"

case 229: {
    ast(1) = makeBasicType(T_USAMPLER2DARRAY);
}   break;

#line 2376 "./glsl.g"

case 230: {
    ast(1) = makeBasicType(T_USAMPLERCUBEARRAY);
}   break;

#line 2383 "./glsl.g"

case 231: {
    ast(1) = makeBasicType(T_SAMPLER2DRECT);
}   break;

#line 2390 "./glsl.g"

case 232: {
    ast(1) = makeBasicType(T_SAMPLER2DRECTSHADOW);
}   break;

#line 2397 "./glsl.g"

case 233: {
    ast(1) = makeBasicType(T_ISAMPLER2DRECT);
}   break;

#line 2404 "./glsl.g"

case 234: {
    ast(1) = makeBasicType(T_USAMPLER2DRECT);
}   break;

#line 2411 "./glsl.g"

case 235: {
    ast(1) = makeBasicType(T_SAMPLERBUFFER);
}   break;

#line 2418 "./glsl.g"

case 236: {
    ast(1) = makeBasicType(T_ISAMPLERBUFFER);
}   break;

#line 2425 "./glsl.g"

case 237: {
    ast(1) = makeBasicType(T_USAMPLERBUFFER);
}   break;

#line 2432 "./glsl.g"

case 238: {
    ast(1) = makeBasicType(T_SAMPLER2DMS);
}   break;

#line 2439 "./glsl.g"

case 239: {
    ast(1) = makeBasicType(T_ISAMPLER2DMS);
}   break;

#line 2446 "./glsl.g"

case 240: {
    ast(1) = makeBasicType(T_USAMPLER2DMS);
}   break;

#line 2453 "./glsl.g"

case 241: {
    ast(1) = makeBasicType(T_SAMPLER2DMSARRAY);
}   break;

#line 2460 "./glsl.g"

case 242: {
    ast(1) = makeBasicType(T_ISAMPLER2DMSARRAY);
}   break;

#line 2467 "./glsl.g"

case 243: {
    ast(1) = makeBasicType(T_USAMPLER2DMSARRAY);
}   break;

#line 2474 "./glsl.g"

case 244: {
    // nothing to do.
}   break;

#line 2481 "./glsl.g"

case 245: {
    ast(1) = makeAstNode<NamedTypeAST>(string(1));
}   break;

#line 2488 "./glsl.g"

case 246: {
    sym(1).precision = TypeAST::Highp;
}   break;

#line 2495 "./glsl.g"

case 247: {
    sym(1).precision = TypeAST::Mediump;
}   break;

#line 2502 "./glsl.g"

case 248: {
    sym(1).precision = TypeAST::Lowp;
}   break;

#line 2509 "./glsl.g"

case 249: {
    ast(1) = makeAstNode<StructTypeAST>(string(2), sym(4).field_list);
}   break;

#line 2516 "./glsl.g"

case 250: {
    ast(1) = makeAstNode<StructTypeAST>(sym(3).field_list);
}   break;

#line 2523 "./glsl.g"

case 251: {
    // nothing to do.
}   break;

#line 2530 "./glsl.g"

case 252: {
    sym(1).field_list = appendLists(sym(1).field_list, sym(2).field_list);
}   break;

#line 2537 "./glsl.g"

case 253: {
    sym(1).field_list = StructTypeAST::fixInnerTypes(type(1), sym(2).field_list);
}   break;

#line 2544 "./glsl.g"

case 254: {
    sym(1).field_list = StructTypeAST::fixInnerTypes
        (makeAstNode<QualifiedTypeAST>
            (sym(1).type_qualifier.qualifier, type(2),
             sym(1).type_qualifier.layout_list), sym(3).field_list);
}   break;

#line 2554 "./glsl.g"

case 255: {
    // nothing to do.
    sym(1).field_list = makeAstNode< List<StructTypeAST::Field *> >(sym(1).field);
}   break;

#line 2562 "./glsl.g"

case 256: {
    sym(1).field_list = makeAstNode< List<StructTypeAST::Field *> >(sym(1).field_list, sym(3).field);
}   break;

#line 2569 "./glsl.g"

case 257: {
    sym(1).field = makeAstNode<StructTypeAST::Field>(string(1));
}   break;

#line 2576 "./glsl.g"

case 258: {
    sym(1).field = makeAstNode<StructTypeAST::Field>
        (string(1), makeAstNode<ArrayTypeAST>((TypeAST *)0));
}   break;

#line 2584 "./glsl.g"

case 259: {
    sym(1).field = makeAstNode<StructTypeAST::Field>
        (string(1), makeAstNode<ArrayTypeAST>((TypeAST *)0, expression(3)));
}   break;

#line 2592 "./glsl.g"

case 260: {
    // nothing to do.
}   break;

#line 2599 "./glsl.g"

case 261: {
    ast(1) = makeAstNode<DeclarationStatementAST>(sym(1).declaration);
}   break;

#line 2606 "./glsl.g"

case 262: {
    // nothing to do.
}   break;

#line 2613 "./glsl.g"

case 263: {
    // nothing to do.
}   break;

#line 2620 "./glsl.g"

case 264: {
    // nothing to do.
}   break;

#line 2627 "./glsl.g"

case 265: {
    // nothing to do.
}   break;

#line 2634 "./glsl.g"

case 266: {
    // nothing to do.
}   break;

#line 2641 "./glsl.g"

case 267: {
    // nothing to do.
}   break;

#line 2648 "./glsl.g"

case 268: {
    // nothing to do.
}   break;

#line 2655 "./glsl.g"

case 269: {
    // nothing to do.
}   break;

#line 2662 "./glsl.g"

case 270: {
    // nothing to do.
}   break;

#line 2669 "./glsl.g"

case 271: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>();
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(2)).end();
    ast(1) = stmt;
}   break;

#line 2679 "./glsl.g"

case 272: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>(sym(2).statement_list);
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(3)).end();
    ast(1) = stmt;
}   break;

#line 2689 "./glsl.g"

case 273: {
    // nothing to do.
}   break;

#line 2696 "./glsl.g"

case 274: {
    // nothing to do.
}   break;

#line 2703 "./glsl.g"

case 275: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>();
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(2)).end();
    ast(1) = stmt;
}   break;

#line 2713 "./glsl.g"

case 276: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>(sym(2).statement_list);
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(3)).end();
    ast(1) = stmt;
}   break;

#line 2723 "./glsl.g"

case 277: {
    sym(1).statement_list = makeAstNode< List<StatementAST *> >(sym(1).statement);
}   break;

#line 2730 "./glsl.g"

case 278: {
    sym(1).statement_list = makeAstNode< List<StatementAST *> >(sym(1).statement_list, sym(2).statement);
}   break;

#line 2737 "./glsl.g"

case 279: {
    ast(1) = makeAstNode<CompoundStatementAST>();  // Empty statement
}   break;

#line 2744 "./glsl.g"

case 280: {
    ast(1) = makeAstNode<ExpressionStatementAST>(expression(1));
}   break;

#line 2751 "./glsl.g"

case 281: {
    ast(1) = makeAstNode<IfStatementAST>(expression(3), sym(5).ifstmt.thenClause, sym(5).ifstmt.elseClause);
}   break;

#line 2758 "./glsl.g"

case 282: {
    sym(1).ifstmt.thenClause = statement(1);
    sym(1).ifstmt.elseClause = statement(3);
}   break;

#line 2766 "./glsl.g"

case 283: {
    sym(1).ifstmt.thenClause = statement(1);
    sym(1).ifstmt.elseClause = 0;
}   break;

#line 2774 "./glsl.g"

case 284: {
    // nothing to do.
}   break;

#line 2781 "./glsl.g"

case 285: {
    ast(1) = makeAstNode<DeclarationExpressionAST>
        (type(1), string(2), expression(4));
}   break;

#line 2789 "./glsl.g"

case 286: {
    ast(1) = makeAstNode<SwitchStatementAST>(expression(3), statement(6));
}   break;

#line 2796 "./glsl.g"

case 287: {
    ast(1) = makeAstNode<CompoundStatementAST>();
}   break;

#line 2803 "./glsl.g"

case 288: {
    ast(1) = makeAstNode<CompoundStatementAST>(sym(1).statement_list);
}   break;

#line 2810 "./glsl.g"

case 289: {
    ast(1) = makeAstNode<CaseLabelStatementAST>(expression(2));
}   break;

#line 2817 "./glsl.g"

case 290: {
    ast(1) = makeAstNode<CaseLabelStatementAST>();
}   break;

#line 2824 "./glsl.g"

case 291: {
    ast(1) = makeAstNode<WhileStatementAST>(expression(3), statement(5));
}   break;

#line 2831 "./glsl.g"

case 292: {
    ast(1) = makeAstNode<DoStatementAST>(statement(2), expression(5));
}   break;

#line 2838 "./glsl.g"

case 293: {
    ast(1) = makeAstNode<ForStatementAST>(statement(3), sym(4).forstmt.condition, sym(4).forstmt.increment, statement(6));
}   break;

#line 2845 "./glsl.g"

case 294: {
    // nothing to do.
}   break;

#line 2852 "./glsl.g"

case 295: {
    // nothing to do.
}   break;

#line 2859 "./glsl.g"

case 296: {
    // nothing to do.
}   break;

#line 2866 "./glsl.g"

case 297: {
    // nothing to do.
}   break;

#line 2873 "./glsl.g"

case 298: {
    sym(1).forstmt.condition = expression(1);
    sym(1).forstmt.increment = 0;
}   break;

#line 2881 "./glsl.g"

case 299: {
    sym(1).forstmt.condition = expression(1);
    sym(1).forstmt.increment = expression(3);
}   break;

#line 2889 "./glsl.g"

case 300: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Continue);
}   break;

#line 2896 "./glsl.g"

case 301: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Break);
}   break;

#line 2903 "./glsl.g"

case 302: {
    ast(1) = makeAstNode<ReturnStatementAST>();
}   break;

#line 2910 "./glsl.g"

case 303: {
    ast(1) = makeAstNode<ReturnStatementAST>(expression(2));
}   break;

#line 2917 "./glsl.g"

case 304: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Discard);
}   break;

#line 2924 "./glsl.g"

case 305: {
    ast(1) = makeAstNode<TranslationUnitAST>(sym(1).declaration_list);
}   break;

#line 2931 "./glsl.g"

case 306: {
    if (sym(1).declaration) {
        sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration);
    } else {
        sym(1).declaration_list = 0;
    }
}   break;

#line 2943 "./glsl.g"

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

#line 2960 "./glsl.g"

case 308: {
    // nothing to do.
}   break;

#line 2967 "./glsl.g"

case 309: {
    // nothing to do.
}   break;

#line 2974 "./glsl.g"

case 310: {
    ast(1) = 0;
}   break;

#line 2981 "./glsl.g"

case 311: {
    function(1)->body = statement(2);
}   break;

#line 2988 "./glsl.g"

case 312: {
    ast(1) = 0;
}   break;

#line 2996 "./glsl.g"

case 313: {
    ast(1) = ast(2);
}   break;

#line 3003 "./glsl.g"

case 314: {
    ast(1) = ast(2);
}   break;

#line 3009 "./glsl.g"

} // end switch
} // end Parser::reduce()
