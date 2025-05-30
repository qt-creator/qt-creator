
#line 421 "./glsl.g"

// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
            parenStack.push(static_cast<int>(_tokens.size()));
            break;
        case T_LEFT_BRACKET:
            bracketStack.push(static_cast<int>(_tokens.size()));
            break;
        case T_LEFT_BRACE:
            braceStack.push(static_cast<int>(_tokens.size()));
            break;

        case T_RIGHT_PAREN:
            if (! parenStack.empty()) {
                _tokens[parenStack.top()].matchingBrace = static_cast<int>(_tokens.size());
                parenStack.pop();
            }
            break;
        case T_RIGHT_BRACKET:
            if (! bracketStack.empty()) {
                _tokens[bracketStack.top()].matchingBrace = static_cast<int>(_tokens.size());
                bracketStack.pop();
            }
            break;
        case T_RIGHT_BRACE:
            if (! braceStack.empty()) {
                _tokens[braceStack.top()].matchingBrace = static_cast<int>(_tokens.size());
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
    void *yyval = nullptr; // value of the current token.

    _recovered = false;
    _tos = -1;
    _startToken.kind = startToken;
    int recoveryAttempts = 0;


    do {
        recoveryAttempts = 0;

    againAfterRecovery:
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
            ++recoveryAttempts;
            if (recoveryAttempts > 10)
               break;
            const int line = _tokens[yyloc].line + 1;
            QString message = QLatin1String("Syntax error");
            if (yytoken != -1) {
                const QLatin1String s(spell[yytoken]);
                message = QString::fromLatin1("Unexpected token `%1'").arg(s);
                if (yytoken == 0) // do not freeze on unexpected end of file
                    return nullptr;
            }

            for (; _tos; --_tos) {
                const int state = _stateStack[_tos];

                static int tks1[] = {
                    T_RIGHT_BRACE, T_RIGHT_PAREN, T_RIGHT_BRACKET,
                    T_SEMICOLON, T_COLON, T_COMMA,
                    T_NUMBER, T_TYPE_NAME, T_IDENTIFIER,
                    T_LEFT_BRACE, T_LEFT_PAREN, T_LEFT_BRACKET,
                    T_WHILE,
                    0
                };
                static int tks2[] = {
                    T_RIGHT_BRACE, T_RIGHT_PAREN, T_RIGHT_BRACKET,
                    T_SEMICOLON, T_COLON, T_COMMA,
                    0
                };
                int *tks;
                if (recoveryAttempts < 2)
                    tks = tks1;
                else
                    tks = tks2; // Avoid running into an endless loop for e.g.: for(int x=0; x y

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
                            yyval = nullptr;

                        _symStack[_tos].ptr = yyval;
                        _locationStack[_tos] = yyloc;
                        yytoken = -1;

                        action = next;
                        goto againAfterRecovery;
                    }
                }
            }

            if (! _recovered) {
                _recovered = true;
                error(line, message);
            }
        }

    } while (action);

    return nullptr;
}

#line 631 "./glsl.g"

void Parser::reduce(int ruleno)
{
switch(ruleno) {

#line 640 "./glsl.g"

case 0: {
    ast(1) = makeAstNode<IdentifierExpressionAST>(string(1));
}   break;

#line 647 "./glsl.g"

case 1: {
    ast(1) = makeAstNode<LiteralExpressionAST>(string(1));
}   break;

#line 654 "./glsl.g"

case 2: {
    ast(1) = makeAstNode<LiteralExpressionAST>(_engine->identifier("true", 4));
}   break;

#line 661 "./glsl.g"

case 3: {
    ast(1) = makeAstNode<LiteralExpressionAST>(_engine->identifier("false", 5));
}   break;

#line 668 "./glsl.g"

case 4: {
    // nothing to do.
}   break;

#line 675 "./glsl.g"

case 5: {
    ast(1) = ast(2);
}   break;

#line 682 "./glsl.g"

case 6: {
    // nothing to do.
}   break;

#line 689 "./glsl.g"

case 7: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ArrayAccess, expression(1), expression(3));
}   break;

#line 696 "./glsl.g"

case 8: {
    // nothing to do.
}   break;

#line 703 "./glsl.g"

case 9: {
    ast(1) = makeAstNode<MemberAccessExpressionAST>(expression(1), string(3));
}   break;

#line 710 "./glsl.g"

case 10: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PostIncrement, expression(1));
}   break;

#line 717 "./glsl.g"

case 11: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PostDecrement, expression(1));
}   break;

#line 724 "./glsl.g"

case 12: {
    // nothing to do.
}   break;

#line 731 "./glsl.g"

case 13: {
    // nothing to do.
}   break;

#line 738 "./glsl.g"

case 14: {
    ast(1) = makeAstNode<FunctionCallExpressionAST>
        (sym(1).function.id, sym(1).function.arguments);
}   break;

#line 746 "./glsl.g"

case 15: {
    ast(1) = makeAstNode<FunctionCallExpressionAST>
        (expression(1), sym(3).function.id, sym(3).function.arguments);
}   break;

#line 754 "./glsl.g"

case 16: {
    // nothing to do.
}   break;

#line 761 "./glsl.g"

case 17: {
    // nothing to do.
}   break;

#line 768 "./glsl.g"

case 18: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments = nullptr;
}   break;

#line 776 "./glsl.g"

case 19: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments = nullptr;
}   break;

#line 784 "./glsl.g"

case 20: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments =
        makeAstNode< List<ExpressionAST *> >(expression(2));
}   break;

#line 793 "./glsl.g"

case 21: {
    sym(1).function.arguments =
        makeAstNode< List<ExpressionAST *> >
            (sym(1).function.arguments, expression(3));
}   break;

#line 802 "./glsl.g"

case 22: {
    // nothing to do.
}   break;

#line 809 "./glsl.g"

case 23: {
    ast(1) = makeAstNode<FunctionIdentifierAST>(type(1));
}   break;

#line 816 "./glsl.g"

case 24: {
    ast(1) = makeAstNode<FunctionIdentifierAST>(string(1));
}   break;

#line 823 "./glsl.g"

case 25: {
    // nothing to do.
}   break;

#line 830 "./glsl.g"

case 26: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PreIncrement, expression(2));
}   break;

#line 837 "./glsl.g"

case 27: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PreDecrement, expression(2));
}   break;

#line 844 "./glsl.g"

case 28: {
    ast(1) = makeAstNode<UnaryExpressionAST>(sym(1).kind, expression(2));
}   break;

#line 851 "./glsl.g"

case 29: {
    sym(1).kind = AST::Kind_UnaryPlus;
}   break;

#line 858 "./glsl.g"

case 30: {
    sym(1).kind = AST::Kind_UnaryMinus;
}   break;

#line 865 "./glsl.g"

case 31: {
    sym(1).kind = AST::Kind_LogicalNot;
}   break;

#line 872 "./glsl.g"

case 32: {
    sym(1).kind = AST::Kind_BitwiseNot;
}   break;

#line 879 "./glsl.g"

case 33: {
    // nothing to do.
}   break;

#line 886 "./glsl.g"

case 34: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Multiply, expression(1), expression(3));
}   break;

#line 893 "./glsl.g"

case 35: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Divide, expression(1), expression(3));
}   break;

#line 900 "./glsl.g"

case 36: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Modulus, expression(1), expression(3));
}   break;

#line 907 "./glsl.g"

case 37: {
    // nothing to do.
}   break;

#line 914 "./glsl.g"

case 38: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Plus, expression(1), expression(3));
}   break;

#line 921 "./glsl.g"

case 39: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Minus, expression(1), expression(3));
}   break;

#line 928 "./glsl.g"

case 40: {
    // nothing to do.
}   break;

#line 935 "./glsl.g"

case 41: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ShiftLeft, expression(1), expression(3));
}   break;

#line 942 "./glsl.g"

case 42: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ShiftRight, expression(1), expression(3));
}   break;

#line 949 "./glsl.g"

case 43: {
    // nothing to do.
}   break;

#line 956 "./glsl.g"

case 44: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LessThan, expression(1), expression(3));
}   break;

#line 963 "./glsl.g"

case 45: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_GreaterThan, expression(1), expression(3));
}   break;

#line 970 "./glsl.g"

case 46: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LessEqual, expression(1), expression(3));
}   break;

#line 977 "./glsl.g"

case 47: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_GreaterEqual, expression(1), expression(3));
}   break;

#line 984 "./glsl.g"

case 48: {
    // nothing to do.
}   break;

#line 991 "./glsl.g"

case 49: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Equal, expression(1), expression(3));
}   break;

#line 998 "./glsl.g"

case 50: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_NotEqual, expression(1), expression(3));
}   break;

#line 1005 "./glsl.g"

case 51: {
    // nothing to do.
}   break;

#line 1012 "./glsl.g"

case 52: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseAnd, expression(1), expression(3));
}   break;

#line 1019 "./glsl.g"

case 53: {
    // nothing to do.
}   break;

#line 1026 "./glsl.g"

case 54: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseXor, expression(1), expression(3));
}   break;

#line 1033 "./glsl.g"

case 55: {
    // nothing to do.
}   break;

#line 1040 "./glsl.g"

case 56: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseOr, expression(1), expression(3));
}   break;

#line 1047 "./glsl.g"

case 57: {
    // nothing to do.
}   break;

#line 1054 "./glsl.g"

case 58: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalAnd, expression(1), expression(3));
}   break;

#line 1061 "./glsl.g"

case 59: {
    // nothing to do.
}   break;

#line 1068 "./glsl.g"

case 60: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalXor, expression(1), expression(3));
}   break;

#line 1075 "./glsl.g"

case 61: {
    // nothing to do.
}   break;

#line 1082 "./glsl.g"

case 62: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalOr, expression(1), expression(3));
}   break;

#line 1089 "./glsl.g"

case 63: {
    // nothing to do.
}   break;

#line 1096 "./glsl.g"

case 64: {
    ast(1) = makeAstNode<TernaryExpressionAST>(AST::Kind_Conditional, expression(1), expression(3), expression(5));
}   break;

#line 1103 "./glsl.g"

case 65: {
    // nothing to do.
}   break;

#line 1110 "./glsl.g"

case 66: {
    ast(1) = makeAstNode<AssignmentExpressionAST>(sym(2).kind, expression(1), expression(3));
}   break;

#line 1117 "./glsl.g"

case 67: {
    sym(1).kind = AST::Kind_Assign;
}   break;

#line 1124 "./glsl.g"

case 68: {
    sym(1).kind = AST::Kind_AssignMultiply;
}   break;

#line 1131 "./glsl.g"

case 69: {
    sym(1).kind = AST::Kind_AssignDivide;
}   break;

#line 1138 "./glsl.g"

case 70: {
    sym(1).kind = AST::Kind_AssignModulus;
}   break;

#line 1145 "./glsl.g"

case 71: {
    sym(1).kind = AST::Kind_AssignPlus;
}   break;

#line 1152 "./glsl.g"

case 72: {
    sym(1).kind = AST::Kind_AssignMinus;
}   break;

#line 1159 "./glsl.g"

case 73: {
    sym(1).kind = AST::Kind_AssignShiftLeft;
}   break;

#line 1166 "./glsl.g"

case 74: {
    sym(1).kind = AST::Kind_AssignShiftRight;
}   break;

#line 1173 "./glsl.g"

case 75: {
    sym(1).kind = AST::Kind_AssignAnd;
}   break;

#line 1180 "./glsl.g"

case 76: {
    sym(1).kind = AST::Kind_AssignXor;
}   break;

#line 1187 "./glsl.g"

case 77: {
    sym(1).kind = AST::Kind_AssignOr;
}   break;

#line 1194 "./glsl.g"

case 78: {
    // nothing to do.
}   break;

#line 1201 "./glsl.g"

case 79: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Comma, expression(1), expression(3));
}   break;

#line 1208 "./glsl.g"

case 80: {
    // nothing to do.
}   break;

#line 1215 "./glsl.g"

case 81: {
    // nothing to do.
}   break;

#line 1222 "./glsl.g"

case 82: {
    ast(1) = makeAstNode<InitDeclarationAST>(sym(1).declaration_list);
}   break;

#line 1229 "./glsl.g"

case 83: {
    ast(1) = makeAstNode<PrecisionDeclarationAST>(sym(2).precision, type(3));
}   break;

#line 1236 "./glsl.g"

case 84: {
    const int qualifier = sym(1).type_qualifier.qualifier;
    if ((qualifier & QualifiedTypeAST::Struct) == 0) {
        if (!isInterfaceBlockStorageIdentifier(qualifier)) {
            int loc = location(1);
            int lineno = loc >= 0 ? (_tokens[loc].line + 1) : 0;
            if ((qualifier & QualifiedTypeAST::StorageMask) == QualifiedTypeAST::NoStorage)
                error(lineno, "Missing storage qualifier.");
            else
                error(lineno, "Used storage qualifier not allowed for interface blocks.");
        }
        TypeAST *type = makeAstNode<InterfaceBlockAST>(string(2), sym(4).field_list);
        ast(1) = makeAstNode<TypeDeclarationAST>(type);
    } else {
        TypeAST *type = makeAstNode<StructTypeAST>(string(2), sym(4).field_list);
        ast(1) = makeAstNode<TypeDeclarationAST>(type);
    }
}   break;

#line 1258 "./glsl.g"

case 85: {
    const int qualifier = sym(1).type_qualifier.qualifier;
    if ((qualifier & QualifiedTypeAST::Struct) == 0) {
        if (!isInterfaceBlockStorageIdentifier(qualifier)) {
            int loc = location(1);
            int lineno = loc >= 0 ? (_tokens[loc].line + 1) : 0;
            if ((qualifier & QualifiedTypeAST::StorageMask) == QualifiedTypeAST::NoStorage)
                error(lineno, "Missing storage qualifier.");
            else
                error(lineno, "Used storage qualifier not allowed for interface blocks.");
        }
        TypeAST *type = makeAstNode<InterfaceBlockAST>(string(2), sym(4).field_list);
        ast(1) = makeAstNode<TypeAndVariableDeclarationAST>
            (makeAstNode<TypeDeclarationAST>(type),
             makeAstNode<VariableDeclarationAST>(type, string(6)));
    } else {
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
    }
}   break;

#line 1290 "./glsl.g"

case 86: {
    const int qualifier = sym(1).type_qualifier.qualifier;
    if ((qualifier & QualifiedTypeAST::Struct) == 0) {
        if (!isInterfaceBlockStorageIdentifier(qualifier)) {
            int loc = location(1);
            int lineno = loc >= 0 ? (_tokens[loc].line + 1) : 0;
            if ((qualifier & QualifiedTypeAST::StorageMask) == QualifiedTypeAST::NoStorage)
                error(lineno, "Missing storage qualifier.");
            else
                error(lineno, "Used storage qualifier not allowed for interface blocks.");
        }
        TypeAST *type = makeAstNode<InterfaceBlockAST>(string(2), sym(4).field_list);
        ast(1) = makeAstNode<TypeAndVariableDeclarationAST>
            (makeAstNode<TypeDeclarationAST>(type),
             makeAstNode<VariableDeclarationAST>
                (makeAstNode<ArrayTypeAST>(type), string(6)));
    } else {
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
    }
}   break;

#line 1324 "./glsl.g"

case 87: {
    const int qualifier = sym(1).type_qualifier.qualifier;
    if ((qualifier & QualifiedTypeAST::Struct) == 0) {
        if (!isInterfaceBlockStorageIdentifier(qualifier)) {
            int loc = location(1);
            int lineno = loc >= 0 ? (_tokens[loc].line + 1) : 0;
            if ((qualifier & QualifiedTypeAST::StorageMask) == QualifiedTypeAST::NoStorage)
                error(lineno, "Missing storage qualifier.");
            else
                error(lineno, "Used storage qualifier not allowed for interface blocks.");
        }
        TypeAST *type = makeAstNode<InterfaceBlockAST>(string(2), sym(4).field_list);
        ast(1) = makeAstNode<TypeAndVariableDeclarationAST>
            (makeAstNode<TypeDeclarationAST>(type),
             makeAstNode<VariableDeclarationAST>
                (makeAstNode<ArrayTypeAST>(type, expression(8)), string(6)));
    } else {
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
    }
}   break;

#line 1358 "./glsl.g"

case 88: {
    TypeAST *type = makeAstNode<QualifiedTypeAST>
        (sym(1).type_qualifier.qualifier, (TypeAST *)nullptr,
         sym(1).type_qualifier.layout_list);
    ast(1) = makeAstNode<TypeDeclarationAST>(type);
}   break;

#line 1368 "./glsl.g"

case 89: {
    function(1)->finishParams();
}   break;

#line 1375 "./glsl.g"

case 90: {
    // nothing to do.
}   break;

#line 1382 "./glsl.g"

case 91: {
    // nothing to do.
}   break;

#line 1389 "./glsl.g"

case 92: {
    function(1)->params = makeAstNode< List<ParameterDeclarationAST *> >
        (sym(2).param_declaration);
}   break;

#line 1397 "./glsl.g"

case 93: {
    function(1)->params = makeAstNode< List<ParameterDeclarationAST *> >
        (function(1)->params, sym(3).param_declaration);
}   break;

#line 1405 "./glsl.g"

case 94: {
    function(1) = makeAstNode<FunctionDeclarationAST>(type(1), string(2));
}   break;

#line 1412 "./glsl.g"

case 95: {
    sym(1).param_declarator.type = type(1);
    sym(1).param_declarator.name = string(2);
}   break;

#line 1420 "./glsl.g"

case 96: {
    sym(1).param_declarator.type = makeAstNode<ArrayTypeAST>(type(1), expression(4));
    sym(1).param_declarator.name = string(2);
}   break;

#line 1428 "./glsl.g"

case 97: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (makeAstNode<QualifiedTypeAST>
            (sym(1).qualifier, sym(3).param_declarator.type,
             (List<LayoutQualifierAST *> *)nullptr),
         ParameterDeclarationAST::Qualifier(sym(2).qualifier),
         sym(3).param_declarator.name);
}   break;

#line 1440 "./glsl.g"

case 98: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (sym(2).param_declarator.type,
         ParameterDeclarationAST::Qualifier(sym(1).qualifier),
         sym(2).param_declarator.name);
}   break;

#line 1450 "./glsl.g"

case 99: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (makeAstNode<QualifiedTypeAST>
            (sym(1).qualifier, type(3), (List<LayoutQualifierAST *> *)nullptr),
         ParameterDeclarationAST::Qualifier(sym(2).qualifier),
         (const QString *)nullptr);
}   break;

#line 1461 "./glsl.g"

case 100: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (type(2), ParameterDeclarationAST::Qualifier(sym(1).qualifier),
         (const QString *)nullptr);
}   break;

#line 1470 "./glsl.g"

case 101: {
    sym(1).qualifier = ParameterDeclarationAST::In;
}   break;

#line 1477 "./glsl.g"

case 102: {
    sym(1).qualifier = ParameterDeclarationAST::In;
}   break;

#line 1484 "./glsl.g"

case 103: {
    sym(1).qualifier = ParameterDeclarationAST::Out;
}   break;

#line 1491 "./glsl.g"

case 104: {
    sym(1).qualifier = ParameterDeclarationAST::InOut;
}   break;

#line 1498 "./glsl.g"

case 105: {
    // nothing to do.
}   break;

#line 1505 "./glsl.g"

case 106: {
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
        (sym(1).declaration);
}   break;

#line 1513 "./glsl.g"

case 107: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1523 "./glsl.g"

case 108: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1534 "./glsl.g"

case 109: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type, expression(5));
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1545 "./glsl.g"

case 110: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>
            (type, string(3), expression(7));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1557 "./glsl.g"

case 111: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type, expression(5));
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>
            (type, string(3), expression(8));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1569 "./glsl.g"

case 112: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>
            (type, string(3), expression(5));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1580 "./glsl.g"

case 113: {
    ast(1) = makeAstNode<TypeDeclarationAST>(type(1));
}   break;

#line 1587 "./glsl.g"

case 114: {
    ast(1) = makeAstNode<VariableDeclarationAST>(type(1), string(2));
}   break;

#line 1594 "./glsl.g"

case 115: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1)), string(2));
}   break;

#line 1602 "./glsl.g"

case 116: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1), expression(4)), string(2));
}   break;

#line 1610 "./glsl.g"

case 117: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1)), string(2), expression(6));
}   break;

#line 1618 "./glsl.g"

case 118: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1), expression(4)),
         string(2), expression(7));
}   break;

#line 1627 "./glsl.g"

case 119: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (type(1), string(2), expression(4));
}   break;

#line 1635 "./glsl.g"

case 120: {
    ast(1) = makeAstNode<InvariantDeclarationAST>(string(2));
}   break;

#line 1642 "./glsl.g"

case 121: {
    ast(1) = makeAstNode<QualifiedTypeAST>(0, type(1), (List<LayoutQualifierAST *> *)nullptr);
}   break;

#line 1649 "./glsl.g"

case 122: {
    ast(1) = makeAstNode<QualifiedTypeAST>
        (sym(1).type_qualifier.qualifier, type(2),
         sym(1).type_qualifier.layout_list);
}   break;

#line 1658 "./glsl.g"

case 123: {
    sym(1).qualifier = QualifiedTypeAST::Invariant;
}   break;

#line 1665 "./glsl.g"

case 124: {
    sym(1).qualifier = QualifiedTypeAST::Smooth;
}   break;

#line 1672 "./glsl.g"

case 125: {
    sym(1).qualifier = QualifiedTypeAST::Flat;
}   break;

#line 1679 "./glsl.g"

case 126: {
    sym(1).qualifier = QualifiedTypeAST::NoPerspective;
}   break;

#line 1686 "./glsl.g"

case 127: {
    sym(1) = sym(3);
}   break;

#line 1693 "./glsl.g"

case 128: {
    sym(1).layout_list = makeAstNode< List<LayoutQualifierAST *> >(sym(1).layout);
}   break;

#line 1700 "./glsl.g"

case 129: {
    sym(1).layout_list = makeAstNode< List<LayoutQualifierAST *> >(sym(1).layout_list, sym(3).layout);
}   break;

#line 1707 "./glsl.g"

case 130: {
    sym(1).layout = makeAstNode<LayoutQualifierAST>(string(1), (const QString *)nullptr);
}   break;

#line 1714 "./glsl.g"

case 131: {
    sym(1).layout = makeAstNode<LayoutQualifierAST>(string(1), string(3));
}   break;

#line 1721 "./glsl.g"

case 132: {
    sym(1).qualifier = QualifiedTypeAST::Const;
}   break;

#line 1728 "./glsl.g"

case 133: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier;
    sym(1).type_qualifier.layout_list = nullptr;
}   break;

#line 1736 "./glsl.g"

case 134: {
    sym(1).type_qualifier.layout_list = sym(1).layout_list;
    sym(1).type_qualifier.qualifier = 0;
}   break;

#line 1744 "./glsl.g"

case 135: {
    sym(1).type_qualifier.layout_list = sym(1).layout_list;
    sym(1).type_qualifier.qualifier = sym(2).qualifier;
}   break;

#line 1752 "./glsl.g"

case 136: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier;
    sym(1).type_qualifier.layout_list = nullptr;
}   break;

#line 1760 "./glsl.g"

case 137: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier;
    sym(1).type_qualifier.layout_list = nullptr;
}   break;

#line 1768 "./glsl.g"

case 138: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier;
    sym(1).type_qualifier.layout_list = nullptr;
}   break;

#line 1776 "./glsl.g"

case 139: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier | sym(3).qualifier;
    sym(1).type_qualifier.layout_list = nullptr;
}   break;

#line 1784 "./glsl.g"

case 140: {
    sym(1).type_qualifier.qualifier = QualifiedTypeAST::Invariant;
    sym(1).type_qualifier.layout_list = nullptr;
}   break;

#line 1792 "./glsl.g"

case 141: {
    sym(1).qualifier = QualifiedTypeAST::Const;
}   break;

#line 1799 "./glsl.g"

case 142: {
    sym(1).qualifier = QualifiedTypeAST::Attribute;
}   break;

#line 1806 "./glsl.g"

case 143: {
    sym(1).qualifier = QualifiedTypeAST::Varying;
}   break;

#line 1813 "./glsl.g"

case 144: {
    sym(1).qualifier = QualifiedTypeAST::CentroidVarying;
}   break;

#line 1820 "./glsl.g"

case 145: {
    sym(1).qualifier = QualifiedTypeAST::In;
}   break;

#line 1827 "./glsl.g"

case 146: {
    sym(1).qualifier = QualifiedTypeAST::Out;
}   break;

#line 1834 "./glsl.g"

case 147: {
    sym(1).qualifier = QualifiedTypeAST::CentroidIn;
}   break;

#line 1841 "./glsl.g"

case 148: {
    sym(1).qualifier = QualifiedTypeAST::CentroidOut;
}   break;

#line 1848 "./glsl.g"

case 149: {
    sym(1).qualifier = QualifiedTypeAST::PatchIn;
}   break;

#line 1855 "./glsl.g"

case 150: {
    sym(1).qualifier = QualifiedTypeAST::PatchOut;
}   break;

#line 1862 "./glsl.g"

case 151: {
    sym(1).qualifier = QualifiedTypeAST::SampleIn;
}   break;

#line 1869 "./glsl.g"

case 152: {
    sym(1).qualifier = QualifiedTypeAST::SampleOut;
}   break;

#line 1876 "./glsl.g"

case 153: {
    sym(1).qualifier = QualifiedTypeAST::Uniform;
}   break;

#line 1883 "./glsl.g"

case 154: {
    // nothing to do.
}   break;

#line 1890 "./glsl.g"

case 155: {
    if (!type(2)->setPrecision(sym(1).precision)) {
        // TODO: issue an error about precision not allowed on this type.
    }
    ast(1) = type(2);
}   break;

#line 1900 "./glsl.g"

case 156: {
    // nothing to do.
}   break;

#line 1907 "./glsl.g"

case 157: {
    ast(1) = makeAstNode<ArrayTypeAST>(type(1));
}   break;

#line 1914 "./glsl.g"

case 158: {
    ast(1) = makeAstNode<ArrayTypeAST>(type(1), expression(3));
}   break;

#line 1921 "./glsl.g"

case 159: {
    ast(1) = makeBasicType(T_VOID);
}   break;

#line 1928 "./glsl.g"

case 160: {
    ast(1) = makeBasicType(T_FLOAT);
}   break;

#line 1935 "./glsl.g"

case 161: {
    ast(1) = makeBasicType(T_DOUBLE);
}   break;

#line 1942 "./glsl.g"

case 162: {
    ast(1) = makeBasicType(T_INT);
}   break;

#line 1949 "./glsl.g"

case 163: {
    ast(1) = makeBasicType(T_UINT);
}   break;

#line 1956 "./glsl.g"

case 164: {
    ast(1) = makeBasicType(T_ATOMIC_UINT);
} break;

#line 1963 "./glsl.g"

case 165: {
    ast(1) = makeBasicType(T_BOOL);
}   break;

#line 1970 "./glsl.g"

case 166: {
    ast(1) = makeBasicType(T_VEC2);
}   break;

#line 1977 "./glsl.g"

case 167: {
    ast(1) = makeBasicType(T_VEC3);
}   break;

#line 1984 "./glsl.g"

case 168: {
    ast(1) = makeBasicType(T_VEC4);
}   break;

#line 1991 "./glsl.g"

case 169: {
    ast(1) = makeBasicType(T_DVEC2);
}   break;

#line 1998 "./glsl.g"

case 170: {
    ast(1) = makeBasicType(T_DVEC3);
}   break;

#line 2005 "./glsl.g"

case 171: {
    ast(1) = makeBasicType(T_DVEC4);
}   break;

#line 2012 "./glsl.g"

case 172: {
    ast(1) = makeBasicType(T_BVEC2);
}   break;

#line 2019 "./glsl.g"

case 173: {
    ast(1) = makeBasicType(T_BVEC3);
}   break;

#line 2026 "./glsl.g"

case 174: {
    ast(1) = makeBasicType(T_BVEC4);
}   break;

#line 2033 "./glsl.g"

case 175: {
    ast(1) = makeBasicType(T_IVEC2);
}   break;

#line 2040 "./glsl.g"

case 176: {
    ast(1) = makeBasicType(T_IVEC3);
}   break;

#line 2047 "./glsl.g"

case 177: {
    ast(1) = makeBasicType(T_IVEC4);
}   break;

#line 2054 "./glsl.g"

case 178: {
    ast(1) = makeBasicType(T_UVEC2);
}   break;

#line 2061 "./glsl.g"

case 179: {
    ast(1) = makeBasicType(T_UVEC3);
}   break;

#line 2068 "./glsl.g"

case 180: {
    ast(1) = makeBasicType(T_UVEC4);
}   break;

#line 2075 "./glsl.g"

case 181: {
    ast(1) = makeBasicType(T_MAT2);
}   break;

#line 2082 "./glsl.g"

case 182: {
    ast(1) = makeBasicType(T_MAT3);
}   break;

#line 2089 "./glsl.g"

case 183: {
    ast(1) = makeBasicType(T_MAT4);
}   break;

#line 2096 "./glsl.g"

case 184: {
    ast(1) = makeBasicType(T_MAT2);
}   break;

#line 2103 "./glsl.g"

case 185: {
    ast(1) = makeBasicType(T_MAT2X3);
}   break;

#line 2110 "./glsl.g"

case 186: {
    ast(1) = makeBasicType(T_MAT2X4);
}   break;

#line 2117 "./glsl.g"

case 187: {
    ast(1) = makeBasicType(T_MAT3X2);
}   break;

#line 2124 "./glsl.g"

case 188: {
    ast(1) = makeBasicType(T_MAT3);
}   break;

#line 2131 "./glsl.g"

case 189: {
    ast(1) = makeBasicType(T_MAT3X4);
}   break;

#line 2138 "./glsl.g"

case 190: {
    ast(1) = makeBasicType(T_MAT4X2);
}   break;

#line 2145 "./glsl.g"

case 191: {
    ast(1) = makeBasicType(T_MAT4X3);
}   break;

#line 2152 "./glsl.g"

case 192: {
    ast(1) = makeBasicType(T_MAT4);
}   break;

#line 2159 "./glsl.g"

case 193: {
    ast(1) = makeBasicType(T_DMAT2);
}   break;

#line 2166 "./glsl.g"

case 194: {
    ast(1) = makeBasicType(T_DMAT3);
}   break;

#line 2173 "./glsl.g"

case 195: {
    ast(1) = makeBasicType(T_DMAT4);
}   break;

#line 2180 "./glsl.g"

case 196: {
    ast(1) = makeBasicType(T_DMAT2);
}   break;

#line 2187 "./glsl.g"

case 197: {
    ast(1) = makeBasicType(T_DMAT2X3);
}   break;

#line 2194 "./glsl.g"

case 198: {
    ast(1) = makeBasicType(T_DMAT2X4);
}   break;

#line 2201 "./glsl.g"

case 199: {
    ast(1) = makeBasicType(T_DMAT3X2);
}   break;

#line 2208 "./glsl.g"

case 200: {
    ast(1) = makeBasicType(T_DMAT3);
}   break;

#line 2215 "./glsl.g"

case 201: {
    ast(1) = makeBasicType(T_DMAT3X4);
}   break;

#line 2222 "./glsl.g"

case 202: {
    ast(1) = makeBasicType(T_DMAT4X2);
}   break;

#line 2229 "./glsl.g"

case 203: {
    ast(1) = makeBasicType(T_DMAT4X3);
}   break;

#line 2236 "./glsl.g"

case 204: {
    ast(1) = makeBasicType(T_DMAT4);
}   break;

#line 2243 "./glsl.g"

case 205: {
    ast(1) = makeBasicType(T_SAMPLER1D);
}   break;

#line 2250 "./glsl.g"

case 206: {
    ast(1) = makeBasicType(T_SAMPLER2D);
}   break;

#line 2257 "./glsl.g"

case 207: {
    ast(1) = makeBasicType(T_SAMPLER3D);
}   break;

#line 2264 "./glsl.g"

case 208: {
    ast(1) = makeBasicType(T_SAMPLERCUBE);
}   break;

#line 2271 "./glsl.g"

case 209: {
    ast(1) = makeBasicType(T_SAMPLER1DSHADOW);
}   break;

#line 2278 "./glsl.g"

case 210: {
    ast(1) = makeBasicType(T_SAMPLER2DSHADOW);
}   break;

#line 2285 "./glsl.g"

case 211: {
    ast(1) = makeBasicType(T_SAMPLERCUBESHADOW);
}   break;

#line 2292 "./glsl.g"

case 212: {
    ast(1) = makeBasicType(T_SAMPLER1DARRAY);
}   break;

#line 2299 "./glsl.g"

case 213: {
    ast(1) = makeBasicType(T_SAMPLER2DARRAY);
}   break;

#line 2306 "./glsl.g"

case 214: {
    ast(1) = makeBasicType(T_SAMPLER1DARRAYSHADOW);
}   break;

#line 2313 "./glsl.g"

case 215: {
    ast(1) = makeBasicType(T_SAMPLER2DARRAYSHADOW);
}   break;

#line 2320 "./glsl.g"

case 216: {
    ast(1) = makeBasicType(T_SAMPLERCUBEARRAY);
}   break;

#line 2327 "./glsl.g"

case 217: {
    ast(1) = makeBasicType(T_SAMPLERCUBEARRAYSHADOW);
}   break;

#line 2334 "./glsl.g"

case 218: {
    ast(1) = makeBasicType(T_ISAMPLER1D);
}   break;

#line 2341 "./glsl.g"

case 219: {
    ast(1) = makeBasicType(T_ISAMPLER2D);
}   break;

#line 2348 "./glsl.g"

case 220: {
    ast(1) = makeBasicType(T_ISAMPLER3D);
}   break;

#line 2355 "./glsl.g"

case 221: {
    ast(1) = makeBasicType(T_ISAMPLERCUBE);
}   break;

#line 2362 "./glsl.g"

case 222: {
    ast(1) = makeBasicType(T_ISAMPLER1DARRAY);
}   break;

#line 2369 "./glsl.g"

case 223: {
    ast(1) = makeBasicType(T_ISAMPLER2DARRAY);
}   break;

#line 2376 "./glsl.g"

case 224: {
    ast(1) = makeBasicType(T_ISAMPLERCUBEARRAY);
}   break;

#line 2383 "./glsl.g"

case 225: {
    ast(1) = makeBasicType(T_USAMPLER1D);
}   break;

#line 2390 "./glsl.g"

case 226: {
    ast(1) = makeBasicType(T_USAMPLER2D);
}   break;

#line 2397 "./glsl.g"

case 227: {
    ast(1) = makeBasicType(T_USAMPLER3D);
}   break;

#line 2404 "./glsl.g"

case 228: {
    ast(1) = makeBasicType(T_USAMPLERCUBE);
}   break;

#line 2411 "./glsl.g"

case 229: {
    ast(1) = makeBasicType(T_USAMPLER1DARRAY);
}   break;

#line 2418 "./glsl.g"

case 230: {
    ast(1) = makeBasicType(T_USAMPLER2DARRAY);
}   break;

#line 2425 "./glsl.g"

case 231: {
    ast(1) = makeBasicType(T_USAMPLERCUBEARRAY);
}   break;

#line 2432 "./glsl.g"

case 232: {
    ast(1) = makeBasicType(T_SAMPLER2DRECT);
}   break;

#line 2439 "./glsl.g"

case 233: {
    ast(1) = makeBasicType(T_SAMPLER2DRECTSHADOW);
}   break;

#line 2446 "./glsl.g"

case 234: {
    ast(1) = makeBasicType(T_ISAMPLER2DRECT);
}   break;

#line 2453 "./glsl.g"

case 235: {
    ast(1) = makeBasicType(T_USAMPLER2DRECT);
}   break;

#line 2460 "./glsl.g"

case 236: {
    ast(1) = makeBasicType(T_SAMPLERBUFFER);
}   break;

#line 2467 "./glsl.g"

case 237: {
    ast(1) = makeBasicType(T_ISAMPLERBUFFER);
}   break;

#line 2474 "./glsl.g"

case 238: {
    ast(1) = makeBasicType(T_USAMPLERBUFFER);
}   break;

#line 2481 "./glsl.g"

case 239: {
    ast(1) = makeBasicType(T_SAMPLER2DMS);
}   break;

#line 2488 "./glsl.g"

case 240: {
    ast(1) = makeBasicType(T_ISAMPLER2DMS);
}   break;

#line 2495 "./glsl.g"

case 241: {
    ast(1) = makeBasicType(T_USAMPLER2DMS);
}   break;

#line 2502 "./glsl.g"

case 242: {
    ast(1) = makeBasicType(T_SAMPLER2DMSARRAY);
}   break;

#line 2509 "./glsl.g"

case 243: {
    ast(1) = makeBasicType(T_ISAMPLER2DMSARRAY);
}   break;

#line 2516 "./glsl.g"

case 244: {
    ast(1) = makeBasicType(T_USAMPLER2DMSARRAY);
}   break;

#line 2523 "./glsl.g"

case 245: {
    ast(1) = makeBasicType(T_IIMAGE1D);
} break;

#line 2530 "./glsl.g"

case 246: {
    ast(1) = makeBasicType(T_IIMAGE1DARRAY);
} break;

#line 2537 "./glsl.g"

case 247: {
    ast(1) = makeBasicType(T_IIMAGE2D);
} break;

#line 2544 "./glsl.g"

case 248: {
    ast(1) = makeBasicType(T_IIMAGE2DARRAY);
} break;

#line 2551 "./glsl.g"

case 249: {
    ast(1) = makeBasicType(T_IIMAGE2DMS);
} break;

#line 2558 "./glsl.g"

case 250: {
    ast(1) = makeBasicType(T_IIMAGE2DMSARRAY);
} break;

#line 2565 "./glsl.g"

case 251: {
    ast(1) = makeBasicType(T_IIMAGE2DRECT);
} break;

#line 2572 "./glsl.g"

case 252: {
    ast(1) = makeBasicType(T_IIMAGE3D);
} break;

#line 2579 "./glsl.g"

case 253: {
    ast(1) = makeBasicType(T_IIMAGEBUFFER);
} break;

#line 2586 "./glsl.g"

case 254: {
    ast(1) = makeBasicType(T_IIMAGECUBE);
} break;

#line 2593 "./glsl.g"

case 255: {
    ast(1) = makeBasicType(T_IIMAGECUBEARRAY);
} break;

#line 2600 "./glsl.g"

case 256: {
    ast(1) = makeBasicType(T_IMAGE1D);
} break;

#line 2607 "./glsl.g"

case 257: {
    ast(1) = makeBasicType(T_IMAGE1DARRAY);
} break;

#line 2614 "./glsl.g"

case 258: {
    ast(1) = makeBasicType(T_IMAGE2D);
} break;

#line 2621 "./glsl.g"

case 259: {
    ast(1) = makeBasicType(T_IMAGE2DARRAY);
} break;

#line 2628 "./glsl.g"

case 260: {
    ast(1) = makeBasicType(T_IMAGE2DMS);
} break;

#line 2635 "./glsl.g"

case 261: {
    ast(1) = makeBasicType(T_IMAGE2DMSARRAY);
} break;

#line 2642 "./glsl.g"

case 262: {
    ast(1) = makeBasicType(T_IMAGE2DRECT);
} break;

#line 2649 "./glsl.g"

case 263: {
    ast(1) = makeBasicType(T_IMAGE3D);
} break;

#line 2656 "./glsl.g"

case 264: {
    ast(1) = makeBasicType(T_IMAGEBUFFER);
} break;

#line 2663 "./glsl.g"

case 265: {
    ast(1) = makeBasicType(T_IMAGECUBE);
} break;

#line 2670 "./glsl.g"

case 266: {
    ast(1) = makeBasicType(T_IMAGECUBEARRAY);
} break;

#line 2677 "./glsl.g"

case 267: {
    ast(1) = makeBasicType(T_UIMAGE1D);
} break;

#line 2684 "./glsl.g"

case 268: {
    ast(1) = makeBasicType(T_UIMAGE1DARRAY);
} break;

#line 2691 "./glsl.g"

case 269: {
    ast(1) = makeBasicType(T_UIMAGE2D);
} break;

#line 2698 "./glsl.g"

case 270: {
    ast(1) = makeBasicType(T_UIMAGE2DARRAY);
} break;

#line 2705 "./glsl.g"

case 271: {
    ast(1) = makeBasicType(T_UIMAGE2DMS);
} break;

#line 2712 "./glsl.g"

case 272: {
    ast(1) = makeBasicType(T_UIMAGE2DMSARRAY);
} break;

#line 2719 "./glsl.g"

case 273: {
    ast(1) = makeBasicType(T_UIMAGE2DRECT);
} break;

#line 2726 "./glsl.g"

case 274: {
    ast(1) = makeBasicType(T_UIMAGE3D);
} break;

#line 2733 "./glsl.g"

case 275: {
    ast(1) = makeBasicType(T_UIMAGEBUFFER);
} break;

#line 2740 "./glsl.g"

case 276: {
    ast(1) = makeBasicType(T_UIMAGECUBE);
} break;

#line 2747 "./glsl.g"

case 277: {
    ast(1) = makeBasicType(T_UIMAGECUBEARRAY);
} break;

#line 2754 "./glsl.g"

case 278: {
    // nothing to do.
}   break;

#line 2761 "./glsl.g"

case 279: {
    ast(1) = makeAstNode<NamedTypeAST>(string(1));
}   break;

#line 2768 "./glsl.g"

case 280: {
    sym(1).precision = TypeAST::Highp;
}   break;

#line 2775 "./glsl.g"

case 281: {
    sym(1).precision = TypeAST::Mediump;
}   break;

#line 2782 "./glsl.g"

case 282: {
    sym(1).precision = TypeAST::Lowp;
}   break;

#line 2789 "./glsl.g"

case 283: {
    ast(1) = makeAstNode<StructTypeAST>(string(2), sym(4).field_list);
}   break;

#line 2796 "./glsl.g"

case 284: {
    ast(1) = makeAstNode<StructTypeAST>(sym(3).field_list);
}   break;

#line 2803 "./glsl.g"

case 285: {
    // nothing to do.
}   break;

#line 2810 "./glsl.g"

case 286: {
    sym(1).field_list = appendLists(sym(1).field_list, sym(2).field_list);
}   break;

#line 2817 "./glsl.g"

case 287: {
    sym(1).field_list = StructTypeAST::fixInnerTypes(type(1), sym(2).field_list);
}   break;

#line 2824 "./glsl.g"

case 288: {
    sym(1).field_list = StructTypeAST::fixInnerTypes
        (makeAstNode<QualifiedTypeAST>
            (sym(1).type_qualifier.qualifier, type(2),
             sym(1).type_qualifier.layout_list), sym(3).field_list);
}   break;

#line 2834 "./glsl.g"

case 289: {
    // nothing to do.
    sym(1).field_list = makeAstNode< List<StructTypeAST::Field *> >(sym(1).field);
}   break;

#line 2842 "./glsl.g"

case 290: {
    sym(1).field_list = makeAstNode< List<StructTypeAST::Field *> >(sym(1).field_list, sym(3).field);
}   break;

#line 2849 "./glsl.g"

case 291: {
    sym(1).field = makeAstNode<StructTypeAST::Field>(string(1));
}   break;

#line 2856 "./glsl.g"

case 292: {
    sym(1).field = makeAstNode<StructTypeAST::Field>
        (string(1), makeAstNode<ArrayTypeAST>((TypeAST *)nullptr));
}   break;

#line 2864 "./glsl.g"

case 293: {
    sym(1).field = makeAstNode<StructTypeAST::Field>
        (string(1), makeAstNode<ArrayTypeAST>((TypeAST *)nullptr, expression(3)));
}   break;

#line 2872 "./glsl.g"

case 294: {
    // nothing to do.
}   break;

#line 2879 "./glsl.g"

case 295: {
    ast(1) = makeAstNode<DeclarationStatementAST>(sym(1).declaration);
}   break;

#line 2886 "./glsl.g"

case 296: {
    // nothing to do.
}   break;

#line 2893 "./glsl.g"

case 297: {
    // nothing to do.
}   break;

#line 2900 "./glsl.g"

case 298: {
    // nothing to do.
}   break;

#line 2907 "./glsl.g"

case 299: {
    // nothing to do.
}   break;

#line 2914 "./glsl.g"

case 300: {
    // nothing to do.
}   break;

#line 2921 "./glsl.g"

case 301: {
    // nothing to do.
}   break;

#line 2928 "./glsl.g"

case 302: {
    // nothing to do.
}   break;

#line 2935 "./glsl.g"

case 303: {
    // nothing to do.
}   break;

#line 2942 "./glsl.g"

case 304: {
    // nothing to do.
}   break;

#line 2949 "./glsl.g"

case 305: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>();
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(2)).end();
    ast(1) = stmt;
}   break;

#line 2959 "./glsl.g"

case 306: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>(sym(2).statement_list);
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(3)).end();
    ast(1) = stmt;
}   break;

#line 2969 "./glsl.g"

case 307: {
    // nothing to do.
}   break;

#line 2976 "./glsl.g"

case 308: {
    // nothing to do.
}   break;

#line 2983 "./glsl.g"

case 309: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>();
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(2)).end();
    ast(1) = stmt;
}   break;

#line 2993 "./glsl.g"

case 310: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>(sym(2).statement_list);
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(3)).end();
    ast(1) = stmt;
}   break;

#line 3003 "./glsl.g"

case 311: {
    sym(1).statement_list = makeAstNode< List<StatementAST *> >(sym(1).statement);
}   break;

#line 3010 "./glsl.g"

case 312: {
    sym(1).statement_list = makeAstNode< List<StatementAST *> >(sym(1).statement_list, sym(2).statement);
}   break;

#line 3017 "./glsl.g"

case 313: {
    ast(1) = makeAstNode<CompoundStatementAST>();  // Empty statement
}   break;

#line 3024 "./glsl.g"

case 314: {
    ast(1) = makeAstNode<ExpressionStatementAST>(expression(1));
}   break;

#line 3031 "./glsl.g"

case 315: {
    ast(1) = makeAstNode<IfStatementAST>(expression(3), sym(5).ifstmt.thenClause, sym(5).ifstmt.elseClause);
}   break;

#line 3038 "./glsl.g"

case 316: {
    sym(1).ifstmt.thenClause = statement(1);
    sym(1).ifstmt.elseClause = statement(3);
}   break;

#line 3046 "./glsl.g"

case 317: {
    sym(1).ifstmt.thenClause = statement(1);
    sym(1).ifstmt.elseClause = nullptr;
}   break;

#line 3054 "./glsl.g"

case 318: {
    // nothing to do.
}   break;

#line 3061 "./glsl.g"

case 319: {
    ast(1) = makeAstNode<DeclarationExpressionAST>
        (type(1), string(2), expression(4));
}   break;

#line 3069 "./glsl.g"

case 320: {
    ast(1) = makeAstNode<SwitchStatementAST>(expression(3), statement(6));
}   break;

#line 3076 "./glsl.g"

case 321: {
    ast(1) = makeAstNode<CompoundStatementAST>();
}   break;

#line 3083 "./glsl.g"

case 322: {
    ast(1) = makeAstNode<CompoundStatementAST>(sym(1).statement_list);
}   break;

#line 3090 "./glsl.g"

case 323: {
    ast(1) = makeAstNode<CaseLabelStatementAST>(expression(2));
}   break;

#line 3097 "./glsl.g"

case 324: {
    ast(1) = makeAstNode<CaseLabelStatementAST>();
}   break;

#line 3104 "./glsl.g"

case 325: {
    ast(1) = makeAstNode<WhileStatementAST>(expression(3), statement(5));
}   break;

#line 3111 "./glsl.g"

case 326: {
    ast(1) = makeAstNode<DoStatementAST>(statement(2), expression(5));
}   break;

#line 3118 "./glsl.g"

case 327: {
    ast(1) = makeAstNode<ForStatementAST>(statement(3), sym(4).forstmt.condition, sym(4).forstmt.increment, statement(6));
}   break;

#line 3125 "./glsl.g"

case 328: {
    // nothing to do.
}   break;

#line 3132 "./glsl.g"

case 329: {
    // nothing to do.
}   break;

#line 3139 "./glsl.g"

case 330: {
    // nothing to do.
}   break;

#line 3146 "./glsl.g"

case 331: {
    // nothing to do.
}   break;

#line 3153 "./glsl.g"

case 332: {
    sym(1).forstmt.condition = expression(1);
    sym(1).forstmt.increment = nullptr;
}   break;

#line 3161 "./glsl.g"

case 333: {
    sym(1).forstmt.condition = expression(1);
    sym(1).forstmt.increment = expression(3);
}   break;

#line 3169 "./glsl.g"

case 334: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Continue);
}   break;

#line 3176 "./glsl.g"

case 335: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Break);
}   break;

#line 3183 "./glsl.g"

case 336: {
    ast(1) = makeAstNode<ReturnStatementAST>();
}   break;

#line 3190 "./glsl.g"

case 337: {
    ast(1) = makeAstNode<ReturnStatementAST>(expression(2));
}   break;

#line 3197 "./glsl.g"

case 338: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Discard);
}   break;

#line 3204 "./glsl.g"

case 339: {
    ast(1) = makeAstNode<TranslationUnitAST>(sym(1).declaration_list);
}   break;

#line 3211 "./glsl.g"

case 340: {
    if (sym(1).declaration) {
        sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration);
    } else {
        sym(1).declaration_list = nullptr;
    }
}   break;

#line 3223 "./glsl.g"

case 341: {
    if (sym(1).declaration_list && sym(2).declaration) {
        sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, sym(2).declaration);
    } else if (!sym(1).declaration_list) {
        if (sym(2).declaration) {
            sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
                (sym(2).declaration);
        } else {
            sym(1).declaration_list = nullptr;
        }
    }
}   break;

#line 3240 "./glsl.g"

case 342: {
    // nothing to do.
}   break;

#line 3247 "./glsl.g"

case 343: {
    // nothing to do.
}   break;

#line 3254 "./glsl.g"

case 344: {
    ast(1) = nullptr;
}   break;

#line 3261 "./glsl.g"

case 345: {
    function(1)->body = statement(2);
}   break;

#line 3268 "./glsl.g"

case 346: {
    ast(1) = nullptr;
}   break;

#line 3276 "./glsl.g"

case 347: {
    ast(1) = ast(2);
}   break;

#line 3283 "./glsl.g"

case 348: {
    ast(1) = ast(2);
}   break;

#line 3289 "./glsl.g"

} // end switch
} // end Parser::reduce()
