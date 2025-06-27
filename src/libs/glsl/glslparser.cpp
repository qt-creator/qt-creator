
#line 434 "./glsl.g"

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

#line 644 "./glsl.g"

void Parser::reduce(int ruleno)
{
switch(ruleno) {

#line 653 "./glsl.g"

case 0: {
    ast(1) = makeAstNode<IdentifierExpressionAST>(string(1));
}   break;

#line 660 "./glsl.g"

case 1: {
    ast(1) = makeAstNode<LiteralExpressionAST>(string(1));
}   break;

#line 667 "./glsl.g"

case 2: {
    ast(1) = makeAstNode<LiteralExpressionAST>(_engine->identifier("true", 4));
}   break;

#line 674 "./glsl.g"

case 3: {
    ast(1) = makeAstNode<LiteralExpressionAST>(_engine->identifier("false", 5));
}   break;

#line 681 "./glsl.g"

case 4: {
    // nothing to do.
}   break;

#line 688 "./glsl.g"

case 5: {
    ast(1) = ast(2);
}   break;

#line 695 "./glsl.g"

case 6: {
    // nothing to do.
}   break;

#line 702 "./glsl.g"

case 7: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ArrayAccess, expression(1), expression(3));
}   break;

#line 709 "./glsl.g"

case 8: {
    // nothing to do.
}   break;

#line 716 "./glsl.g"

case 9: {
    ast(1) = makeAstNode<MemberAccessExpressionAST>(expression(1), string(3));
}   break;

#line 723 "./glsl.g"

case 10: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PostIncrement, expression(1));
}   break;

#line 730 "./glsl.g"

case 11: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PostDecrement, expression(1));
}   break;

#line 737 "./glsl.g"

case 12: {
    // nothing to do.
}   break;

#line 744 "./glsl.g"

case 13: {
    // nothing to do.
}   break;

#line 751 "./glsl.g"

case 14: {
    ast(1) = makeAstNode<FunctionCallExpressionAST>
        (sym(1).function.id, sym(1).function.arguments);
}   break;

#line 759 "./glsl.g"

case 15: {
    ast(1) = makeAstNode<FunctionCallExpressionAST>
        (expression(1), sym(3).function.id, sym(3).function.arguments);
}   break;

#line 767 "./glsl.g"

case 16: {
    // nothing to do.
}   break;

#line 774 "./glsl.g"

case 17: {
    // nothing to do.
}   break;

#line 781 "./glsl.g"

case 18: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments = nullptr;
}   break;

#line 789 "./glsl.g"

case 19: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments = nullptr;
}   break;

#line 797 "./glsl.g"

case 20: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments =
        makeAstNode< List<ExpressionAST *> >(expression(2));
}   break;

#line 806 "./glsl.g"

case 21: {
    sym(1).function.arguments =
        makeAstNode< List<ExpressionAST *> >
            (sym(1).function.arguments, expression(3));
}   break;

#line 815 "./glsl.g"

case 22: {
    // nothing to do.
}   break;

#line 822 "./glsl.g"

case 23: {
    ast(1) = makeAstNode<FunctionIdentifierAST>(type(1));
}   break;

#line 829 "./glsl.g"

case 24: {
    ast(1) = makeAstNode<FunctionIdentifierAST>(string(1));
}   break;

#line 836 "./glsl.g"

case 25: {
    // nothing to do.
}   break;

#line 843 "./glsl.g"

case 26: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PreIncrement, expression(2));
}   break;

#line 850 "./glsl.g"

case 27: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PreDecrement, expression(2));
}   break;

#line 857 "./glsl.g"

case 28: {
    ast(1) = makeAstNode<UnaryExpressionAST>(sym(1).kind, expression(2));
}   break;

#line 864 "./glsl.g"

case 29: {
    sym(1).kind = AST::Kind_UnaryPlus;
}   break;

#line 871 "./glsl.g"

case 30: {
    sym(1).kind = AST::Kind_UnaryMinus;
}   break;

#line 878 "./glsl.g"

case 31: {
    sym(1).kind = AST::Kind_LogicalNot;
}   break;

#line 885 "./glsl.g"

case 32: {
    sym(1).kind = AST::Kind_BitwiseNot;
}   break;

#line 892 "./glsl.g"

case 33: {
    // nothing to do.
}   break;

#line 899 "./glsl.g"

case 34: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Multiply, expression(1), expression(3));
}   break;

#line 906 "./glsl.g"

case 35: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Divide, expression(1), expression(3));
}   break;

#line 913 "./glsl.g"

case 36: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Modulus, expression(1), expression(3));
}   break;

#line 920 "./glsl.g"

case 37: {
    // nothing to do.
}   break;

#line 927 "./glsl.g"

case 38: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Plus, expression(1), expression(3));
}   break;

#line 934 "./glsl.g"

case 39: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Minus, expression(1), expression(3));
}   break;

#line 941 "./glsl.g"

case 40: {
    // nothing to do.
}   break;

#line 948 "./glsl.g"

case 41: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ShiftLeft, expression(1), expression(3));
}   break;

#line 955 "./glsl.g"

case 42: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ShiftRight, expression(1), expression(3));
}   break;

#line 962 "./glsl.g"

case 43: {
    // nothing to do.
}   break;

#line 969 "./glsl.g"

case 44: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LessThan, expression(1), expression(3));
}   break;

#line 976 "./glsl.g"

case 45: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_GreaterThan, expression(1), expression(3));
}   break;

#line 983 "./glsl.g"

case 46: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LessEqual, expression(1), expression(3));
}   break;

#line 990 "./glsl.g"

case 47: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_GreaterEqual, expression(1), expression(3));
}   break;

#line 997 "./glsl.g"

case 48: {
    // nothing to do.
}   break;

#line 1004 "./glsl.g"

case 49: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Equal, expression(1), expression(3));
}   break;

#line 1011 "./glsl.g"

case 50: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_NotEqual, expression(1), expression(3));
}   break;

#line 1018 "./glsl.g"

case 51: {
    // nothing to do.
}   break;

#line 1025 "./glsl.g"

case 52: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseAnd, expression(1), expression(3));
}   break;

#line 1032 "./glsl.g"

case 53: {
    // nothing to do.
}   break;

#line 1039 "./glsl.g"

case 54: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseXor, expression(1), expression(3));
}   break;

#line 1046 "./glsl.g"

case 55: {
    // nothing to do.
}   break;

#line 1053 "./glsl.g"

case 56: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseOr, expression(1), expression(3));
}   break;

#line 1060 "./glsl.g"

case 57: {
    // nothing to do.
}   break;

#line 1067 "./glsl.g"

case 58: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalAnd, expression(1), expression(3));
}   break;

#line 1074 "./glsl.g"

case 59: {
    // nothing to do.
}   break;

#line 1081 "./glsl.g"

case 60: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalXor, expression(1), expression(3));
}   break;

#line 1088 "./glsl.g"

case 61: {
    // nothing to do.
}   break;

#line 1095 "./glsl.g"

case 62: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalOr, expression(1), expression(3));
}   break;

#line 1102 "./glsl.g"

case 63: {
    // nothing to do.
}   break;

#line 1109 "./glsl.g"

case 64: {
    ast(1) = makeAstNode<TernaryExpressionAST>(AST::Kind_Conditional, expression(1), expression(3), expression(5));
}   break;

#line 1116 "./glsl.g"

case 65: {
    // nothing to do.
}   break;

#line 1123 "./glsl.g"

case 66: {
    ast(1) = makeAstNode<AssignmentExpressionAST>(sym(2).kind, expression(1), expression(3));
}   break;

#line 1130 "./glsl.g"

case 67: {
    sym(1).kind = AST::Kind_Assign;
}   break;

#line 1137 "./glsl.g"

case 68: {
    sym(1).kind = AST::Kind_AssignMultiply;
}   break;

#line 1144 "./glsl.g"

case 69: {
    sym(1).kind = AST::Kind_AssignDivide;
}   break;

#line 1151 "./glsl.g"

case 70: {
    sym(1).kind = AST::Kind_AssignModulus;
}   break;

#line 1158 "./glsl.g"

case 71: {
    sym(1).kind = AST::Kind_AssignPlus;
}   break;

#line 1165 "./glsl.g"

case 72: {
    sym(1).kind = AST::Kind_AssignMinus;
}   break;

#line 1172 "./glsl.g"

case 73: {
    sym(1).kind = AST::Kind_AssignShiftLeft;
}   break;

#line 1179 "./glsl.g"

case 74: {
    sym(1).kind = AST::Kind_AssignShiftRight;
}   break;

#line 1186 "./glsl.g"

case 75: {
    sym(1).kind = AST::Kind_AssignAnd;
}   break;

#line 1193 "./glsl.g"

case 76: {
    sym(1).kind = AST::Kind_AssignXor;
}   break;

#line 1200 "./glsl.g"

case 77: {
    sym(1).kind = AST::Kind_AssignOr;
}   break;

#line 1207 "./glsl.g"

case 78: {
    // nothing to do.
}   break;

#line 1214 "./glsl.g"

case 79: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Comma, expression(1), expression(3));
}   break;

#line 1221 "./glsl.g"

case 80: {
    // nothing to do.
}   break;

#line 1228 "./glsl.g"

case 81: {
    if (auto q = function(1)->returnType->asQualifiedType()) {
        if ((q->qualifiers & QualifiedTypeAST::Subroutine) != 0) {
            SubroutineTypeAST *namedSubroutineType = makeAstNode<SubroutineTypeAST>(function(1));
            ast(1) = makeAstNode<TypeDeclarationAST>(namedSubroutineType);
        }
    } else {
        // nothing to do
    }
}   break;

#line 1242 "./glsl.g"

case 82: {
    ast(1) = makeAstNode<InitDeclarationAST>(sym(1).declaration_list);
}   break;

#line 1249 "./glsl.g"

case 83: {
    ast(1) = makeAstNode<PrecisionDeclarationAST>(sym(2).precision, type(3));
}   break;

#line 1256 "./glsl.g"

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

#line 1278 "./glsl.g"

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

#line 1310 "./glsl.g"

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

#line 1344 "./glsl.g"

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
                (makeAstNode<ArrayTypeAST>(type, sym(7).array_specifier), string(6)));
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
                (makeAstNode<ArrayTypeAST>(qualtype, sym(7).array_specifier), string(6)));
    }
}   break;

#line 1378 "./glsl.g"

case 88: {
    TypeAST *type = makeAstNode<QualifiedTypeAST>
        (sym(1).type_qualifier.qualifier, (TypeAST *)nullptr,
         sym(1).type_qualifier.layout_list);
    ast(1) = makeAstNode<TypeDeclarationAST>(type);
}   break;

#line 1388 "./glsl.g"

case 89: {
    TypeAST *type = makeAstNode<QualifiedTypeAST>
        (sym(1).type_qualifier.qualifier, (TypeAST *)nullptr,
         sym(1).type_qualifier.layout_list);
    ast(1) = makeAstNode<TypeAndVariableDeclarationAST>
        (makeAstNode<TypeDeclarationAST>(type),
         makeAstNode<VariableDeclarationAST>(type, string(2)));
}   break;

#line 1400 "./glsl.g"

case 90: {
    TypeAST *type = makeAstNode<QualifiedTypeAST>
        (sym(1).type_qualifier.qualifier, (TypeAST *)nullptr,
         sym(1).type_qualifier.layout_list);

    DeclarationAST * first = makeAstNode<TypeAndVariableDeclarationAST>
        (makeAstNode<TypeDeclarationAST>(type),
         makeAstNode<VariableDeclarationAST>(type, string(2)));
    List<DeclarationAST *> *allDeclarations = makeAstNode<List<DeclarationAST *>>(first);

    // get identifier, convert circular to linked list
    List<IdentifierExpressionAST *> *idList = sym(3).identifier_list->finish();
    while (idList) {
        DeclarationAST *secondary = makeAstNode<TypeAndVariableDeclarationAST>
            (makeAstNode<TypeDeclarationAST>(type),
             makeAstNode<VariableDeclarationAST>(type, idList->value->name));

        allDeclarations = appendLists(allDeclarations,
                                      makeAstNode<List<DeclarationAST *>>(secondary));
        idList = idList->next;
    }

    ast(1) = makeAstNode<InitDeclarationAST>(allDeclarations);
}   break;

#line 1428 "./glsl.g"

case 91: {
    IdentifierExpressionAST *id = makeAstNode<IdentifierExpressionAST>(string(2));
    sym(1).identifier_list = makeAstNode<List<IdentifierExpressionAST *>>(id);
}   break;

#line 1436 "./glsl.g"

case 92: {
    IdentifierExpressionAST *id = makeAstNode<IdentifierExpressionAST>(string(3));
    List<IdentifierExpressionAST *> *nextId = makeAstNode<List<IdentifierExpressionAST *>>(id);
    sym(1).identifier_list = appendLists(sym(1).identifier_list, nextId);
}   break;

#line 1445 "./glsl.g"

case 93: {
    function(1)->finishParams();
}   break;

#line 1452 "./glsl.g"

case 94: {
    // nothing to do.
}   break;

#line 1459 "./glsl.g"

case 95: {
    // nothing to do.
}   break;

#line 1466 "./glsl.g"

case 96: {
    function(1)->params = makeAstNode< List<ParameterDeclarationAST *> >
        (sym(2).param_declaration);
}   break;

#line 1474 "./glsl.g"

case 97: {
    function(1)->params = makeAstNode< List<ParameterDeclarationAST *> >
        (function(1)->params, sym(3).param_declaration);
}   break;

#line 1482 "./glsl.g"

case 98: {
    function(1) = makeAstNode<FunctionDeclarationAST>(type(1), string(2));
}   break;

#line 1489 "./glsl.g"

case 99: {
    sym(1).param_declarator.type = type(1);
    sym(1).param_declarator.name = string(2);
}   break;

#line 1497 "./glsl.g"

case 100: {
    sym(1).param_declarator.type = makeAstNode<ArrayTypeAST>(type(1), sym(3).array_specifier);
    sym(1).param_declarator.name = string(2);
}   break;

#line 1505 "./glsl.g"

case 101: {
    int qualifier = sym(1).qualifier;
    // ensure correct general qualifier
    if ((qualifier & ParameterDeclarationAST::StorageMask) == 0)
        qualifier |= ParameterDeclarationAST::In;
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (makeAstNode<QualifiedTypeAST>
            (qualifier & QualifiedTypeAST::MemoryMask, sym(2).param_declarator.type,
             (List<LayoutQualifierAST *> *)nullptr),
         ParameterDeclarationAST::Qualifier(qualifier),
         sym(2).param_declarator.name);
}   break;

#line 1521 "./glsl.g"

case 102: {
    int qualifier = sym(1).qualifier;
    // ensure correct general qualifier
    if ((qualifier & ParameterDeclarationAST::StorageMask) == 0)
        qualifier |= ParameterDeclarationAST::In;
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (makeAstNode<QualifiedTypeAST>
            (qualifier & QualifiedTypeAST::MemoryMask, type(2),
             (List<LayoutQualifierAST *> *)nullptr),
         ParameterDeclarationAST::Qualifier(qualifier),
         (const QString *)nullptr);
}   break;

#line 1537 "./glsl.g"

case 103: {
    sym(1).qualifier |= ParameterDeclarationAST::None; // or should we just do nothing?
}   break;

#line 1544 "./glsl.g"

case 104: {
    if ((sym(1).qualifier & sym(2).qualifier) != 0) {
        int loc = location(1);
        int lineno = loc >= 0 ? (_tokens[loc].line + 1) : 0;
        error(lineno, "Duplicate qualifier.");
    } else if ((sym(1).qualifier & ParameterDeclarationAST::PrecisionMask) != 0
                && (sym(2).qualifier & ParameterDeclarationAST::PrecisionMask) != 0) {
        int loc = location(1);
        int lineno = loc >= 0 ? (_tokens[loc].line + 1) : 0;
        error(lineno, "Conflicting precision qualifier.");
    }
    sym(1).qualifier |= sym(2).qualifier;
    if ((sym(1).qualifier & ParameterDeclarationAST::Const) != 0) {
        if (((sym(1).qualifier & ParameterDeclarationAST::InOut) != 0)
            || (sym(1).qualifier & ParameterDeclarationAST::Out) != 0) {
            int loc = location(1);
            int lineno = loc >= 0 ? (_tokens[loc].line + 1) : 0;
            error(lineno, "const cannot be used with out or inout.");
        }
    } else if ((sym(1).qualifier & ParameterDeclarationAST::InOut) != 0) {
        if ((sym(1).qualifier & ParameterDeclarationAST::In) != 0
            || (sym(1).qualifier & ParameterDeclarationAST::Out) != 0) {
            int loc = location(1);
            int lineno = loc >= 0 ? (_tokens[loc].line + 1) : 0;
            error(lineno, "Duplicate qualifier.");
        }
    }
}   break;

#line 1576 "./glsl.g"

case 105: {
    sym(1).qualifier = ParameterDeclarationAST::In;
}   break;

#line 1583 "./glsl.g"

case 106: {
    sym(1).qualifier = ParameterDeclarationAST::Out;
}   break;

#line 1590 "./glsl.g"

case 107: {
    sym(1).qualifier = ParameterDeclarationAST::InOut;
}   break;

#line 1597 "./glsl.g"

case 108: {
    sym(1).qualifier = ParameterDeclarationAST::Const;
}   break;

#line 1604 "./glsl.g"

case 109: {
    sym(1).qualifier = ParameterDeclarationAST::Precise;
}   break;

#line 1611 "./glsl.g"

case 110: {
    sym(1).qualifier = ParameterDeclarationAST::Lowp;
}   break;

#line 1618 "./glsl.g"

case 111: {
    sym(1).qualifier = ParameterDeclarationAST::Mediump;
}   break;

#line 1625 "./glsl.g"

case 112: {
    sym(1).qualifier = ParameterDeclarationAST::Highp;
}   break;

#line 1632 "./glsl.g"

case 113: {
    sym(1).qualifier = ParameterDeclarationAST::Coherent;
}   break;

#line 1639 "./glsl.g"

case 114: {
    sym(1).qualifier = ParameterDeclarationAST::Volatile;
}   break;

#line 1646 "./glsl.g"

case 115: {
    sym(1).qualifier = ParameterDeclarationAST::Restrict;
}   break;

#line 1653 "./glsl.g"

case 116: {
    sym(1).qualifier = ParameterDeclarationAST::Readonly;
}   break;

#line 1660 "./glsl.g"

case 117: {
    sym(1).qualifier = ParameterDeclarationAST::Writeonly;
}   break;

#line 1667 "./glsl.g"

case 118: {
    // nothing to do.
}   break;

#line 1674 "./glsl.g"

case 119: {
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
        (sym(1).declaration);
}   break;

#line 1682 "./glsl.g"

case 120: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1692 "./glsl.g"

case 121: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type, sym(4).array_specifier);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1703 "./glsl.g"

case 122: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type, sym(4).array_specifier);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>
            (type, string(3), expression(6));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1715 "./glsl.g"

case 123: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>
            (type, string(3), expression(5));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1726 "./glsl.g"

case 124: {
    ast(1) = makeAstNode<TypeDeclarationAST>(type(1));
}   break;

#line 1733 "./glsl.g"

case 125: {
    ast(1) = makeAstNode<VariableDeclarationAST>(type(1), string(2));
}   break;

#line 1740 "./glsl.g"

case 126: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1), sym(3).array_specifier), string(2));
}   break;

#line 1748 "./glsl.g"

case 127: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1), sym(3).array_specifier), string(2), expression(5));
}   break;

#line 1756 "./glsl.g"

case 128: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (type(1), string(2), expression(4));
}   break;

#line 1764 "./glsl.g"

case 129: {
    ast(1) = makeAstNode<QualifiedTypeAST>(0, type(1), (List<LayoutQualifierAST *> *)nullptr);
}   break;

#line 1771 "./glsl.g"

case 130: {
    ast(1) = makeAstNode<QualifiedTypeAST>
        (sym(1).type_qualifier.qualifier, type(2),
         sym(1).type_qualifier.layout_list);
}   break;

#line 1780 "./glsl.g"

case 131: {
    sym(1).qualifier = QualifiedTypeAST::Invariant;
}   break;

#line 1787 "./glsl.g"

case 132: {
    sym(1).qualifier = QualifiedTypeAST::Smooth;
}   break;

#line 1794 "./glsl.g"

case 133: {
    sym(1).qualifier = QualifiedTypeAST::Flat;
}   break;

#line 1801 "./glsl.g"

case 134: {
    sym(1).qualifier = QualifiedTypeAST::NoPerspective;
}   break;

#line 1808 "./glsl.g"

case 135: {
    sym(1).type_qualifier.layout_list = sym(3).layout_list;
}   break;

#line 1815 "./glsl.g"

case 136: {
    sym(1).layout_list = makeAstNode< List<LayoutQualifierAST *> >(sym(1).layout);
}   break;

#line 1822 "./glsl.g"

case 137: {
    sym(1).layout_list = makeAstNode< List<LayoutQualifierAST *> >(sym(1).layout_list, sym(3).layout);
}   break;

#line 1829 "./glsl.g"

case 138: {
    sym(1).layout = makeAstNode<LayoutQualifierAST>(string(1), (const QString *)nullptr);
}   break;

#line 1836 "./glsl.g"

case 139: {
    sym(1).layout = makeAstNode<LayoutQualifierAST>(string(1), string(3));
}   break;

#line 1843 "./glsl.g"

case 140: {
    sym(1).layout = makeAstNode<LayoutQualifierAST>(string(1), (const QString *)nullptr);
}   break;

#line 1850 "./glsl.g"

case 141: {
    sym(1).precision = TypeAST::Precise;
}   break;

#line 1857 "./glsl.g"

case 142: {
    // nothing to do.
}   break;

#line 1864 "./glsl.g"

case 143: {
    if (sym(2).type_qualifier.layout_list)
        sym(1).type_qualifier.layout_list = sym(2).type_qualifier.layout_list;
    if ((sym(1).type_qualifier.qualifier & sym(2).type_qualifier.qualifier) != 0) {
        int loc = location(1);
        int lineno = loc >= 0 ? (_tokens[loc].line + 1) : 0;
        error(lineno, "Duplicate qualifier.");
    }
    // TODO check for too many qualifiers?
    sym(1).type_qualifier.qualifier |= sym(2).type_qualifier.qualifier;
    sym(1).type_qualifier.precision = sym(2).type_qualifier.precision;
    if (sym(2).type_qualifier.type_name_list)
        sym(1).type_qualifier.type_name_list = sym(2).type_qualifier.type_name_list;
}   break;

#line 1882 "./glsl.g"

case 144: {
    int qualifier = sym(1).qualifier;
    sym(1).type_qualifier = {TypeAST::PrecUnspecified, 0, nullptr, nullptr};
    sym(1).type_qualifier.qualifier = qualifier;
}   break;

#line 1891 "./glsl.g"

case 145: {
    List<NamedTypeAST *> *typeNameList = sym(1).type_name_list;
    sym(1).type_qualifier = {TypeAST::PrecUnspecified, 0, nullptr, nullptr};
    sym(1).type_qualifier.qualifier = QualifiedTypeAST::Subroutine;
    sym(1).type_qualifier.type_name_list = typeNameList;
}   break;

#line 1901 "./glsl.g"

case 146: {
    // nothing to do.
}   break;

#line 1908 "./glsl.g"

case 147: {
    TypeAST::Precision precision = sym(1).precision;
    sym(1).type_qualifier = {TypeAST::PrecUnspecified, 0, nullptr, nullptr};
    sym(1).type_qualifier.precision = precision;
}   break;

#line 1917 "./glsl.g"

case 148: {
    int qualifier = sym(1).qualifier;
    sym(1).type_qualifier = {TypeAST::PrecUnspecified, 0, nullptr, nullptr};
    sym(1).type_qualifier.qualifier = qualifier;
}   break;

#line 1926 "./glsl.g"

case 149: {
    int qualifier = sym(1).qualifier;
    sym(1).type_qualifier = {TypeAST::PrecUnspecified, 0, nullptr, nullptr};
    sym(1).type_qualifier.qualifier = qualifier;
}   break;

#line 1935 "./glsl.g"

case 150: {
    TypeAST::Precision precision = sym(1).precision;
    sym(1).type_qualifier = {TypeAST::PrecUnspecified, 0, nullptr, nullptr};
    sym(1).type_qualifier.precision = precision;
}   break;

#line 1944 "./glsl.g"

case 151: {
    sym(1).qualifier = QualifiedTypeAST::Const;
}   break;

#line 1951 "./glsl.g"

case 152: {
    sym(1).qualifier = QualifiedTypeAST::Attribute;
}   break;

#line 1958 "./glsl.g"

case 153: {
    sym(1).qualifier = QualifiedTypeAST::Varying;
}   break;

#line 1965 "./glsl.g"

case 154: {
    sym(1).qualifier = QualifiedTypeAST::Centroid;
}   break;

#line 1972 "./glsl.g"

case 155: {
    sym(1).qualifier = QualifiedTypeAST::In;
}   break;

#line 1979 "./glsl.g"

case 156: {
    sym(1).qualifier = QualifiedTypeAST::Out;
}   break;

#line 1986 "./glsl.g"

case 157: {
    sym(1).qualifier = QualifiedTypeAST::Patch;
}   break;

#line 1993 "./glsl.g"

case 158: {
    sym(1).qualifier = QualifiedTypeAST::Sample;
}   break;

#line 2000 "./glsl.g"

case 159: {
    sym(1).qualifier = QualifiedTypeAST::Uniform;
}   break;

#line 2007 "./glsl.g"

case 160: {
    sym(1).qualifier = QualifiedTypeAST::Buffer;
}   break;

#line 2014 "./glsl.g"

case 161: {
    sym(1).qualifier = QualifiedTypeAST::Shared;
}   break;

#line 2021 "./glsl.g"

case 162: {
    sym(1).qualifier = QualifiedTypeAST::Coherent;
} break;

#line 2028 "./glsl.g"

case 163: {
    sym(1).qualifier = QualifiedTypeAST::Volatile;
}   break;

#line 2035 "./glsl.g"

case 164: {
    sym(1).qualifier = QualifiedTypeAST::Restrict;
}   break;

#line 2042 "./glsl.g"

case 165: {
    sym(1).qualifier = QualifiedTypeAST::Readonly;
}   break;

#line 2049 "./glsl.g"

case 166: {
    sym(1).qualifier = QualifiedTypeAST::Writeonly;
}   break;

#line 2056 "./glsl.g"

case 167: {
    sym(1).qualifier = QualifiedTypeAST::Subroutine;
}   break;

#line 2063 "./glsl.g"

case 168: {
    sym(1).type_name_list = sym(3).type_name_list;
}   break;

#line 2070 "./glsl.g"

case 169: {
    NamedTypeAST *namedType = makeAstNode<NamedTypeAST>(string(1));
    sym(1).type_name_list = makeAstNode<List<NamedTypeAST *>>(namedType);
}   break;

#line 2078 "./glsl.g"

case 170: {
    NamedTypeAST *namedType = makeAstNode<NamedTypeAST>(string(3));
    sym(1).type_name_list = appendLists(sym(1).type_name_list,
                                        makeAstNode<List<NamedTypeAST *>>(namedType));

}   break;

#line 2088 "./glsl.g"

case 171: {
    // nothing to do.
}   break;

#line 2095 "./glsl.g"

case 172: {
    ast(1) = makeAstNode<ArrayTypeAST>(type(1), sym(2).array_specifier);
}   break;

#line 2102 "./glsl.g"

case 173: {
    sym(1).array_specifier = makeAstNode< List<ArrayTypeAST::ArraySpecAST *>>(nullptr);
}   break;

#line 2109 "./glsl.g"

case 174: {
    ArrayTypeAST::ArraySpecAST *spec = makeAstNode<ArrayTypeAST::ArraySpecAST>(expression(2));
    sym(1).array_specifier = makeAstNode< List<ArrayTypeAST::ArraySpecAST *>>(spec);
}   break;

#line 2117 "./glsl.g"

case 175: {
    List<ArrayTypeAST::ArraySpecAST *> *empty
        = makeAstNode< List<ArrayTypeAST::ArraySpecAST *>>(nullptr);
    sym(1).array_specifier = appendLists(sym(1).array_specifier, empty);
}   break;

#line 2126 "./glsl.g"

case 176: {
    ArrayTypeAST::ArraySpecAST *spec = makeAstNode<ArrayTypeAST::ArraySpecAST>(expression(3));
    List<ArrayTypeAST::ArraySpecAST *> *specifier
        = makeAstNode< List<ArrayTypeAST::ArraySpecAST *>>(spec);
    sym(1).array_specifier = appendLists(sym(1).array_specifier, specifier);
}   break;

#line 2136 "./glsl.g"

case 177: {
    ast(1) = makeBasicType(T_VOID);
}   break;

#line 2143 "./glsl.g"

case 178: {
    ast(1) = makeBasicType(T_FLOAT);
}   break;

#line 2150 "./glsl.g"

case 179: {
    ast(1) = makeBasicType(T_DOUBLE);
}   break;

#line 2157 "./glsl.g"

case 180: {
    ast(1) = makeBasicType(T_INT);
}   break;

#line 2164 "./glsl.g"

case 181: {
    ast(1) = makeBasicType(T_UINT);
}   break;

#line 2171 "./glsl.g"

case 182: {
    ast(1) = makeBasicType(T_ATOMIC_UINT);
} break;

#line 2178 "./glsl.g"

case 183: {
    ast(1) = makeBasicType(T_BOOL);
}   break;

#line 2185 "./glsl.g"

case 184: {
    ast(1) = makeBasicType(T_VEC2);
}   break;

#line 2192 "./glsl.g"

case 185: {
    ast(1) = makeBasicType(T_VEC3);
}   break;

#line 2199 "./glsl.g"

case 186: {
    ast(1) = makeBasicType(T_VEC4);
}   break;

#line 2206 "./glsl.g"

case 187: {
    ast(1) = makeBasicType(T_DVEC2);
}   break;

#line 2213 "./glsl.g"

case 188: {
    ast(1) = makeBasicType(T_DVEC3);
}   break;

#line 2220 "./glsl.g"

case 189: {
    ast(1) = makeBasicType(T_DVEC4);
}   break;

#line 2227 "./glsl.g"

case 190: {
    ast(1) = makeBasicType(T_BVEC2);
}   break;

#line 2234 "./glsl.g"

case 191: {
    ast(1) = makeBasicType(T_BVEC3);
}   break;

#line 2241 "./glsl.g"

case 192: {
    ast(1) = makeBasicType(T_BVEC4);
}   break;

#line 2248 "./glsl.g"

case 193: {
    ast(1) = makeBasicType(T_IVEC2);
}   break;

#line 2255 "./glsl.g"

case 194: {
    ast(1) = makeBasicType(T_IVEC3);
}   break;

#line 2262 "./glsl.g"

case 195: {
    ast(1) = makeBasicType(T_IVEC4);
}   break;

#line 2269 "./glsl.g"

case 196: {
    ast(1) = makeBasicType(T_UVEC2);
}   break;

#line 2276 "./glsl.g"

case 197: {
    ast(1) = makeBasicType(T_UVEC3);
}   break;

#line 2283 "./glsl.g"

case 198: {
    ast(1) = makeBasicType(T_UVEC4);
}   break;

#line 2290 "./glsl.g"

case 199: {
    ast(1) = makeBasicType(T_MAT2);
}   break;

#line 2297 "./glsl.g"

case 200: {
    ast(1) = makeBasicType(T_MAT3);
}   break;

#line 2304 "./glsl.g"

case 201: {
    ast(1) = makeBasicType(T_MAT4);
}   break;

#line 2311 "./glsl.g"

case 202: {
    ast(1) = makeBasicType(T_MAT2);
}   break;

#line 2318 "./glsl.g"

case 203: {
    ast(1) = makeBasicType(T_MAT2X3);
}   break;

#line 2325 "./glsl.g"

case 204: {
    ast(1) = makeBasicType(T_MAT2X4);
}   break;

#line 2332 "./glsl.g"

case 205: {
    ast(1) = makeBasicType(T_MAT3X2);
}   break;

#line 2339 "./glsl.g"

case 206: {
    ast(1) = makeBasicType(T_MAT3);
}   break;

#line 2346 "./glsl.g"

case 207: {
    ast(1) = makeBasicType(T_MAT3X4);
}   break;

#line 2353 "./glsl.g"

case 208: {
    ast(1) = makeBasicType(T_MAT4X2);
}   break;

#line 2360 "./glsl.g"

case 209: {
    ast(1) = makeBasicType(T_MAT4X3);
}   break;

#line 2367 "./glsl.g"

case 210: {
    ast(1) = makeBasicType(T_MAT4);
}   break;

#line 2374 "./glsl.g"

case 211: {
    ast(1) = makeBasicType(T_DMAT2);
}   break;

#line 2381 "./glsl.g"

case 212: {
    ast(1) = makeBasicType(T_DMAT3);
}   break;

#line 2388 "./glsl.g"

case 213: {
    ast(1) = makeBasicType(T_DMAT4);
}   break;

#line 2395 "./glsl.g"

case 214: {
    ast(1) = makeBasicType(T_DMAT2);
}   break;

#line 2402 "./glsl.g"

case 215: {
    ast(1) = makeBasicType(T_DMAT2X3);
}   break;

#line 2409 "./glsl.g"

case 216: {
    ast(1) = makeBasicType(T_DMAT2X4);
}   break;

#line 2416 "./glsl.g"

case 217: {
    ast(1) = makeBasicType(T_DMAT3X2);
}   break;

#line 2423 "./glsl.g"

case 218: {
    ast(1) = makeBasicType(T_DMAT3);
}   break;

#line 2430 "./glsl.g"

case 219: {
    ast(1) = makeBasicType(T_DMAT3X4);
}   break;

#line 2437 "./glsl.g"

case 220: {
    ast(1) = makeBasicType(T_DMAT4X2);
}   break;

#line 2444 "./glsl.g"

case 221: {
    ast(1) = makeBasicType(T_DMAT4X3);
}   break;

#line 2451 "./glsl.g"

case 222: {
    ast(1) = makeBasicType(T_DMAT4);
}   break;

#line 2458 "./glsl.g"

case 223: {
    ast(1) = makeBasicType(T_SAMPLER1D);
}   break;

#line 2465 "./glsl.g"

case 224: {
    ast(1) = makeBasicType(T_SAMPLER2D);
}   break;

#line 2472 "./glsl.g"

case 225: {
    ast(1) = makeBasicType(T_SAMPLER3D);
}   break;

#line 2479 "./glsl.g"

case 226: {
    ast(1) = makeBasicType(T_SAMPLERCUBE);
}   break;

#line 2486 "./glsl.g"

case 227: {
    ast(1) = makeBasicType(T_SAMPLER1DSHADOW);
}   break;

#line 2493 "./glsl.g"

case 228: {
    ast(1) = makeBasicType(T_SAMPLER2DSHADOW);
}   break;

#line 2500 "./glsl.g"

case 229: {
    ast(1) = makeBasicType(T_SAMPLERCUBESHADOW);
}   break;

#line 2507 "./glsl.g"

case 230: {
    ast(1) = makeBasicType(T_SAMPLER1DARRAY);
}   break;

#line 2514 "./glsl.g"

case 231: {
    ast(1) = makeBasicType(T_SAMPLER2DARRAY);
}   break;

#line 2521 "./glsl.g"

case 232: {
    ast(1) = makeBasicType(T_SAMPLER1DARRAYSHADOW);
}   break;

#line 2528 "./glsl.g"

case 233: {
    ast(1) = makeBasicType(T_SAMPLER2DARRAYSHADOW);
}   break;

#line 2535 "./glsl.g"

case 234: {
    ast(1) = makeBasicType(T_SAMPLERCUBEARRAY);
}   break;

#line 2542 "./glsl.g"

case 235: {
    ast(1) = makeBasicType(T_SAMPLERCUBEARRAYSHADOW);
}   break;

#line 2549 "./glsl.g"

case 236: {
    ast(1) = makeBasicType(T_ISAMPLER1D);
}   break;

#line 2556 "./glsl.g"

case 237: {
    ast(1) = makeBasicType(T_ISAMPLER2D);
}   break;

#line 2563 "./glsl.g"

case 238: {
    ast(1) = makeBasicType(T_ISAMPLER3D);
}   break;

#line 2570 "./glsl.g"

case 239: {
    ast(1) = makeBasicType(T_ISAMPLERCUBE);
}   break;

#line 2577 "./glsl.g"

case 240: {
    ast(1) = makeBasicType(T_ISAMPLER1DARRAY);
}   break;

#line 2584 "./glsl.g"

case 241: {
    ast(1) = makeBasicType(T_ISAMPLER2DARRAY);
}   break;

#line 2591 "./glsl.g"

case 242: {
    ast(1) = makeBasicType(T_ISAMPLERCUBEARRAY);
}   break;

#line 2598 "./glsl.g"

case 243: {
    ast(1) = makeBasicType(T_USAMPLER1D);
}   break;

#line 2605 "./glsl.g"

case 244: {
    ast(1) = makeBasicType(T_USAMPLER2D);
}   break;

#line 2612 "./glsl.g"

case 245: {
    ast(1) = makeBasicType(T_USAMPLER3D);
}   break;

#line 2619 "./glsl.g"

case 246: {
    ast(1) = makeBasicType(T_USAMPLERCUBE);
}   break;

#line 2626 "./glsl.g"

case 247: {
    ast(1) = makeBasicType(T_USAMPLER1DARRAY);
}   break;

#line 2633 "./glsl.g"

case 248: {
    ast(1) = makeBasicType(T_USAMPLER2DARRAY);
}   break;

#line 2640 "./glsl.g"

case 249: {
    ast(1) = makeBasicType(T_USAMPLERCUBEARRAY);
}   break;

#line 2647 "./glsl.g"

case 250: {
    ast(1) = makeBasicType(T_SAMPLER2DRECT);
}   break;

#line 2654 "./glsl.g"

case 251: {
    ast(1) = makeBasicType(T_SAMPLER2DRECTSHADOW);
}   break;

#line 2661 "./glsl.g"

case 252: {
    ast(1) = makeBasicType(T_ISAMPLER2DRECT);
}   break;

#line 2668 "./glsl.g"

case 253: {
    ast(1) = makeBasicType(T_USAMPLER2DRECT);
}   break;

#line 2675 "./glsl.g"

case 254: {
    ast(1) = makeBasicType(T_SAMPLERBUFFER);
}   break;

#line 2682 "./glsl.g"

case 255: {
    ast(1) = makeBasicType(T_ISAMPLERBUFFER);
}   break;

#line 2689 "./glsl.g"

case 256: {
    ast(1) = makeBasicType(T_USAMPLERBUFFER);
}   break;

#line 2696 "./glsl.g"

case 257: {
    ast(1) = makeBasicType(T_SAMPLER2DMS);
}   break;

#line 2703 "./glsl.g"

case 258: {
    ast(1) = makeBasicType(T_ISAMPLER2DMS);
}   break;

#line 2710 "./glsl.g"

case 259: {
    ast(1) = makeBasicType(T_USAMPLER2DMS);
}   break;

#line 2717 "./glsl.g"

case 260: {
    ast(1) = makeBasicType(T_SAMPLER2DMSARRAY);
}   break;

#line 2724 "./glsl.g"

case 261: {
    ast(1) = makeBasicType(T_ISAMPLER2DMSARRAY);
}   break;

#line 2731 "./glsl.g"

case 262: {
    ast(1) = makeBasicType(T_USAMPLER2DMSARRAY);
}   break;

#line 2738 "./glsl.g"

case 263: {
    ast(1) = makeBasicType(T_IIMAGE1D);
} break;

#line 2745 "./glsl.g"

case 264: {
    ast(1) = makeBasicType(T_IIMAGE1DARRAY);
} break;

#line 2752 "./glsl.g"

case 265: {
    ast(1) = makeBasicType(T_IIMAGE2D);
} break;

#line 2759 "./glsl.g"

case 266: {
    ast(1) = makeBasicType(T_IIMAGE2DARRAY);
} break;

#line 2766 "./glsl.g"

case 267: {
    ast(1) = makeBasicType(T_IIMAGE2DMS);
} break;

#line 2773 "./glsl.g"

case 268: {
    ast(1) = makeBasicType(T_IIMAGE2DMSARRAY);
} break;

#line 2780 "./glsl.g"

case 269: {
    ast(1) = makeBasicType(T_IIMAGE2DRECT);
} break;

#line 2787 "./glsl.g"

case 270: {
    ast(1) = makeBasicType(T_IIMAGE3D);
} break;

#line 2794 "./glsl.g"

case 271: {
    ast(1) = makeBasicType(T_IIMAGEBUFFER);
} break;

#line 2801 "./glsl.g"

case 272: {
    ast(1) = makeBasicType(T_IIMAGECUBE);
} break;

#line 2808 "./glsl.g"

case 273: {
    ast(1) = makeBasicType(T_IIMAGECUBEARRAY);
} break;

#line 2815 "./glsl.g"

case 274: {
    ast(1) = makeBasicType(T_IMAGE1D);
} break;

#line 2822 "./glsl.g"

case 275: {
    ast(1) = makeBasicType(T_IMAGE1DARRAY);
} break;

#line 2829 "./glsl.g"

case 276: {
    ast(1) = makeBasicType(T_IMAGE2D);
} break;

#line 2836 "./glsl.g"

case 277: {
    ast(1) = makeBasicType(T_IMAGE2DARRAY);
} break;

#line 2843 "./glsl.g"

case 278: {
    ast(1) = makeBasicType(T_IMAGE2DMS);
} break;

#line 2850 "./glsl.g"

case 279: {
    ast(1) = makeBasicType(T_IMAGE2DMSARRAY);
} break;

#line 2857 "./glsl.g"

case 280: {
    ast(1) = makeBasicType(T_IMAGE2DRECT);
} break;

#line 2864 "./glsl.g"

case 281: {
    ast(1) = makeBasicType(T_IMAGE3D);
} break;

#line 2871 "./glsl.g"

case 282: {
    ast(1) = makeBasicType(T_IMAGEBUFFER);
} break;

#line 2878 "./glsl.g"

case 283: {
    ast(1) = makeBasicType(T_IMAGECUBE);
} break;

#line 2885 "./glsl.g"

case 284: {
    ast(1) = makeBasicType(T_IMAGECUBEARRAY);
} break;

#line 2892 "./glsl.g"

case 285: {
    ast(1) = makeBasicType(T_UIMAGE1D);
} break;

#line 2899 "./glsl.g"

case 286: {
    ast(1) = makeBasicType(T_UIMAGE1DARRAY);
} break;

#line 2906 "./glsl.g"

case 287: {
    ast(1) = makeBasicType(T_UIMAGE2D);
} break;

#line 2913 "./glsl.g"

case 288: {
    ast(1) = makeBasicType(T_UIMAGE2DARRAY);
} break;

#line 2920 "./glsl.g"

case 289: {
    ast(1) = makeBasicType(T_UIMAGE2DMS);
} break;

#line 2927 "./glsl.g"

case 290: {
    ast(1) = makeBasicType(T_UIMAGE2DMSARRAY);
} break;

#line 2934 "./glsl.g"

case 291: {
    ast(1) = makeBasicType(T_UIMAGE2DRECT);
} break;

#line 2941 "./glsl.g"

case 292: {
    ast(1) = makeBasicType(T_UIMAGE3D);
} break;

#line 2948 "./glsl.g"

case 293: {
    ast(1) = makeBasicType(T_UIMAGEBUFFER);
} break;

#line 2955 "./glsl.g"

case 294: {
    ast(1) = makeBasicType(T_UIMAGECUBE);
} break;

#line 2962 "./glsl.g"

case 295: {
    ast(1) = makeBasicType(T_UIMAGECUBEARRAY);
} break;

#line 2969 "./glsl.g"

case 296: {
    // nothing to do.
}   break;

#line 2976 "./glsl.g"

case 297: {
    ast(1) = makeAstNode<NamedTypeAST>(string(1));
}   break;

#line 2983 "./glsl.g"

case 298: {
    sym(1).precision = TypeAST::Highp;
}   break;

#line 2990 "./glsl.g"

case 299: {
    sym(1).precision = TypeAST::Mediump;
}   break;

#line 2997 "./glsl.g"

case 300: {
    sym(1).precision = TypeAST::Lowp;
}   break;

#line 3004 "./glsl.g"

case 301: {
    ast(1) = makeAstNode<StructTypeAST>(string(2), sym(4).field_list);
}   break;

#line 3011 "./glsl.g"

case 302: {
    ast(1) = makeAstNode<StructTypeAST>(sym(3).field_list);
}   break;

#line 3018 "./glsl.g"

case 303: {
    // nothing to do.
}   break;

#line 3025 "./glsl.g"

case 304: {
    sym(1).field_list = appendLists(sym(1).field_list, sym(2).field_list);
}   break;

#line 3032 "./glsl.g"

case 305: {
    sym(1).field_list = StructTypeAST::fixInnerTypes(type(1), sym(2).field_list);
}   break;

#line 3039 "./glsl.g"

case 306: {
    sym(1).field_list = StructTypeAST::fixInnerTypes
        (makeAstNode<QualifiedTypeAST>
            (sym(1).type_qualifier.qualifier, type(2),
             sym(1).type_qualifier.layout_list), sym(3).field_list);
}   break;

#line 3049 "./glsl.g"

case 307: {
    // nothing to do.
    sym(1).field_list = makeAstNode< List<StructTypeAST::Field *> >(sym(1).field);
}   break;

#line 3057 "./glsl.g"

case 308: {
    sym(1).field_list = makeAstNode< List<StructTypeAST::Field *> >(sym(1).field_list, sym(3).field);
}   break;

#line 3064 "./glsl.g"

case 309: {
    sym(1).field = makeAstNode<StructTypeAST::Field>(string(1));
}   break;

#line 3071 "./glsl.g"

case 310: {
    sym(1).field = makeAstNode<StructTypeAST::Field>
        (string(1), makeAstNode<ArrayTypeAST>((TypeAST *)nullptr, sym(2).array_specifier));
}   break;

#line 3079 "./glsl.g"

case 311: {
    List<ExpressionAST *> *expressionList = makeAstNode<List<ExpressionAST *>>(expression(1));
    sym(1).expression = makeAstNode<InitializerListExpressionAST>(expressionList);
}   break;

#line 3087 "./glsl.g"

case 312: {
    sym(1).expression = makeAstNode<InitializerListExpressionAST>(sym(2).expression_list);
}   break;

#line 3094 "./glsl.g"

case 313: {
    sym(1).expression = makeAstNode<InitializerListExpressionAST>(sym(2).expression_list);
}   break;

#line 3101 "./glsl.g"

case 314: {
    sym(1).expression_list = makeAstNode<List<ExpressionAST *>>(expression(1));
}   break;

#line 3108 "./glsl.g"

case 315: {
    sym(1).expression_list = makeAstNode<List<ExpressionAST *>>
            (sym(1).expression_list, expression(3));
}   break;

#line 3116 "./glsl.g"

case 316: {
    ast(1) = makeAstNode<DeclarationStatementAST>(sym(1).declaration);
}   break;

#line 3123 "./glsl.g"

case 317: {
    // nothing to do.
}   break;

#line 3130 "./glsl.g"

case 318: {
    // nothing to do.
}   break;

#line 3137 "./glsl.g"

case 319: {
    // nothing to do.
}   break;

#line 3144 "./glsl.g"

case 320: {
    // nothing to do.
}   break;

#line 3151 "./glsl.g"

case 321: {
    // nothing to do.
}   break;

#line 3158 "./glsl.g"

case 322: {
    // nothing to do.
}   break;

#line 3165 "./glsl.g"

case 323: {
    // nothing to do.
}   break;

#line 3172 "./glsl.g"

case 324: {
    // nothing to do.
}   break;

#line 3179 "./glsl.g"

case 325: {
    // nothing to do.
}   break;

#line 3186 "./glsl.g"

case 326: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>();
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(2)).end();
    ast(1) = stmt;
}   break;

#line 3196 "./glsl.g"

case 327: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>(sym(2).statement_list);
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(3)).end();
    ast(1) = stmt;
}   break;

#line 3206 "./glsl.g"

case 328: {
    // nothing to do.
}   break;

#line 3213 "./glsl.g"

case 329: {
    // nothing to do.
}   break;

#line 3220 "./glsl.g"

case 330: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>();
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(2)).end();
    ast(1) = stmt;
}   break;

#line 3230 "./glsl.g"

case 331: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>(sym(2).statement_list);
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(3)).end();
    ast(1) = stmt;
}   break;

#line 3240 "./glsl.g"

case 332: {
    sym(1).statement_list = makeAstNode< List<StatementAST *> >(sym(1).statement);
}   break;

#line 3247 "./glsl.g"

case 333: {
    sym(1).statement_list = makeAstNode< List<StatementAST *> >(sym(1).statement_list, sym(2).statement);
}   break;

#line 3254 "./glsl.g"

case 334: {
    ast(1) = makeAstNode<CompoundStatementAST>();  // Empty statement
}   break;

#line 3261 "./glsl.g"

case 335: {
    ast(1) = makeAstNode<ExpressionStatementAST>(expression(1));
}   break;

#line 3268 "./glsl.g"

case 336: {
    ast(1) = makeAstNode<IfStatementAST>(expression(3), sym(5).ifstmt.thenClause, sym(5).ifstmt.elseClause);
}   break;

#line 3275 "./glsl.g"

case 337: {
    sym(1).ifstmt.thenClause = statement(1);
    sym(1).ifstmt.elseClause = statement(3);
}   break;

#line 3283 "./glsl.g"

case 338: {
    sym(1).ifstmt.thenClause = statement(1);
    sym(1).ifstmt.elseClause = nullptr;
}   break;

#line 3291 "./glsl.g"

case 339: {
    // nothing to do.
}   break;

#line 3298 "./glsl.g"

case 340: {
    ast(1) = makeAstNode<DeclarationExpressionAST>
        (type(1), string(2), expression(4));
}   break;

#line 3306 "./glsl.g"

case 341: {
    ast(1) = makeAstNode<SwitchStatementAST>(expression(3), statement(6));
}   break;

#line 3313 "./glsl.g"

case 342: {
    ast(1) = makeAstNode<CompoundStatementAST>();
}   break;

#line 3320 "./glsl.g"

case 343: {
    ast(1) = makeAstNode<CompoundStatementAST>(sym(1).statement_list);
}   break;

#line 3327 "./glsl.g"

case 344: {
    ast(1) = makeAstNode<CaseLabelStatementAST>(expression(2));
}   break;

#line 3334 "./glsl.g"

case 345: {
    ast(1) = makeAstNode<CaseLabelStatementAST>();
}   break;

#line 3341 "./glsl.g"

case 346: {
    ast(1) = makeAstNode<WhileStatementAST>(expression(3), statement(5));
}   break;

#line 3348 "./glsl.g"

case 347: {
    ast(1) = makeAstNode<DoStatementAST>(statement(2), expression(5));
}   break;

#line 3355 "./glsl.g"

case 348: {
    ast(1) = makeAstNode<ForStatementAST>(statement(3), sym(4).forstmt.condition, sym(4).forstmt.increment, statement(6));
}   break;

#line 3362 "./glsl.g"

case 349: {
    // nothing to do.
}   break;

#line 3369 "./glsl.g"

case 350: {
    // nothing to do.
}   break;

#line 3376 "./glsl.g"

case 351: {
    // nothing to do.
}   break;

#line 3383 "./glsl.g"

case 352: {
    // nothing to do.
}   break;

#line 3390 "./glsl.g"

case 353: {
    sym(1).forstmt.condition = expression(1);
    sym(1).forstmt.increment = nullptr;
}   break;

#line 3398 "./glsl.g"

case 354: {
    sym(1).forstmt.condition = expression(1);
    sym(1).forstmt.increment = expression(3);
}   break;

#line 3406 "./glsl.g"

case 355: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Continue);
}   break;

#line 3413 "./glsl.g"

case 356: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Break);
}   break;

#line 3420 "./glsl.g"

case 357: {
    ast(1) = makeAstNode<ReturnStatementAST>();
}   break;

#line 3427 "./glsl.g"

case 358: {
    ast(1) = makeAstNode<ReturnStatementAST>(expression(2));
}   break;

#line 3434 "./glsl.g"

case 359: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Discard);
}   break;

#line 3441 "./glsl.g"

case 360: {
    ast(1) = makeAstNode<TranslationUnitAST>(sym(1).declaration_list);
}   break;

#line 3448 "./glsl.g"

case 361: {
    if (sym(1).declaration) {
        sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration);
    } else {
        sym(1).declaration_list = nullptr;
    }
}   break;

#line 3460 "./glsl.g"

case 362: {
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

#line 3477 "./glsl.g"

case 363: {
    // nothing to do.
}   break;

#line 3484 "./glsl.g"

case 364: {
    // nothing to do.
}   break;

#line 3491 "./glsl.g"

case 365: {
    ast(1) = nullptr;
}   break;

#line 3498 "./glsl.g"

case 366: {
    function(1)->body = statement(2);
}   break;

#line 3505 "./glsl.g"

case 367: {
    ast(1) = nullptr;
}   break;

#line 3513 "./glsl.g"

case 368: {
    ast(1) = ast(2);
}   break;

#line 3520 "./glsl.g"

case 369: {
    ast(1) = ast(2);
}   break;

#line 3526 "./glsl.g"

} // end switch
} // end Parser::reduce()
