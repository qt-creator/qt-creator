
#line 475 "./glsl.g"

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

#line 685 "./glsl.g"

void Parser::reduce(int ruleno)
{
switch(ruleno) {

#line 694 "./glsl.g"

case 0: {
    ast(1) = makeAstNode<IdentifierExpressionAST>(string(1));
}   break;

#line 701 "./glsl.g"

case 1: {
    ast(1) = makeAstNode<LiteralExpressionAST>(string(1));
}   break;

#line 708 "./glsl.g"

case 2: {
    ast(1) = makeAstNode<LiteralExpressionAST>(_engine->identifier("true", 4));
}   break;

#line 715 "./glsl.g"

case 3: {
    ast(1) = makeAstNode<LiteralExpressionAST>(_engine->identifier("false", 5));
}   break;

#line 722 "./glsl.g"

case 4: {
    // nothing to do.
}   break;

#line 729 "./glsl.g"

case 5: {
    ast(1) = ast(2);
}   break;

#line 736 "./glsl.g"

case 6: {
    // nothing to do.
}   break;

#line 743 "./glsl.g"

case 7: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ArrayAccess, expression(1), expression(3));
}   break;

#line 750 "./glsl.g"

case 8: {
    // nothing to do.
}   break;

#line 757 "./glsl.g"

case 9: {
    ast(1) = makeAstNode<MemberAccessExpressionAST>(expression(1), string(3));
}   break;

#line 764 "./glsl.g"

case 10: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PostIncrement, expression(1));
}   break;

#line 771 "./glsl.g"

case 11: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PostDecrement, expression(1));
}   break;

#line 778 "./glsl.g"

case 12: {
    // nothing to do.
}   break;

#line 785 "./glsl.g"

case 13: {
    // nothing to do.
}   break;

#line 792 "./glsl.g"

case 14: {
    ast(1) = makeAstNode<FunctionCallExpressionAST>
        (sym(1).function.id, sym(1).function.arguments);
}   break;

#line 800 "./glsl.g"

case 15: {
    ast(1) = makeAstNode<FunctionCallExpressionAST>
        (expression(1), sym(3).function.id, sym(3).function.arguments);
}   break;

#line 808 "./glsl.g"

case 16: {
    // nothing to do.
}   break;

#line 815 "./glsl.g"

case 17: {
    // nothing to do.
}   break;

#line 822 "./glsl.g"

case 18: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments = nullptr;
}   break;

#line 830 "./glsl.g"

case 19: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments = nullptr;
}   break;

#line 838 "./glsl.g"

case 20: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments =
        makeAstNode< List<ExpressionAST *> >(expression(2));
}   break;

#line 847 "./glsl.g"

case 21: {
    sym(1).function.arguments =
        makeAstNode< List<ExpressionAST *> >
            (sym(1).function.arguments, expression(3));
}   break;

#line 856 "./glsl.g"

case 22: {
    // nothing to do.
}   break;

#line 863 "./glsl.g"

case 23: {
    ast(1) = makeAstNode<FunctionIdentifierAST>(type(1));
}   break;

#line 870 "./glsl.g"

case 24: {
    ast(1) = makeAstNode<FunctionIdentifierAST>(string(1));
}   break;

#line 877 "./glsl.g"

case 25: {
    // nothing to do.
}   break;

#line 884 "./glsl.g"

case 26: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PreIncrement, expression(2));
}   break;

#line 891 "./glsl.g"

case 27: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PreDecrement, expression(2));
}   break;

#line 898 "./glsl.g"

case 28: {
    ast(1) = makeAstNode<UnaryExpressionAST>(sym(1).kind, expression(2));
}   break;

#line 905 "./glsl.g"

case 29: {
    sym(1).kind = AST::Kind_UnaryPlus;
}   break;

#line 912 "./glsl.g"

case 30: {
    sym(1).kind = AST::Kind_UnaryMinus;
}   break;

#line 919 "./glsl.g"

case 31: {
    sym(1).kind = AST::Kind_LogicalNot;
}   break;

#line 926 "./glsl.g"

case 32: {
    sym(1).kind = AST::Kind_BitwiseNot;
}   break;

#line 933 "./glsl.g"

case 33: {
    // nothing to do.
}   break;

#line 940 "./glsl.g"

case 34: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Multiply, expression(1), expression(3));
}   break;

#line 947 "./glsl.g"

case 35: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Divide, expression(1), expression(3));
}   break;

#line 954 "./glsl.g"

case 36: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Modulus, expression(1), expression(3));
}   break;

#line 961 "./glsl.g"

case 37: {
    // nothing to do.
}   break;

#line 968 "./glsl.g"

case 38: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Plus, expression(1), expression(3));
}   break;

#line 975 "./glsl.g"

case 39: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Minus, expression(1), expression(3));
}   break;

#line 982 "./glsl.g"

case 40: {
    // nothing to do.
}   break;

#line 989 "./glsl.g"

case 41: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ShiftLeft, expression(1), expression(3));
}   break;

#line 996 "./glsl.g"

case 42: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ShiftRight, expression(1), expression(3));
}   break;

#line 1003 "./glsl.g"

case 43: {
    // nothing to do.
}   break;

#line 1010 "./glsl.g"

case 44: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LessThan, expression(1), expression(3));
}   break;

#line 1017 "./glsl.g"

case 45: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_GreaterThan, expression(1), expression(3));
}   break;

#line 1024 "./glsl.g"

case 46: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LessEqual, expression(1), expression(3));
}   break;

#line 1031 "./glsl.g"

case 47: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_GreaterEqual, expression(1), expression(3));
}   break;

#line 1038 "./glsl.g"

case 48: {
    // nothing to do.
}   break;

#line 1045 "./glsl.g"

case 49: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Equal, expression(1), expression(3));
}   break;

#line 1052 "./glsl.g"

case 50: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_NotEqual, expression(1), expression(3));
}   break;

#line 1059 "./glsl.g"

case 51: {
    // nothing to do.
}   break;

#line 1066 "./glsl.g"

case 52: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseAnd, expression(1), expression(3));
}   break;

#line 1073 "./glsl.g"

case 53: {
    // nothing to do.
}   break;

#line 1080 "./glsl.g"

case 54: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseXor, expression(1), expression(3));
}   break;

#line 1087 "./glsl.g"

case 55: {
    // nothing to do.
}   break;

#line 1094 "./glsl.g"

case 56: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseOr, expression(1), expression(3));
}   break;

#line 1101 "./glsl.g"

case 57: {
    // nothing to do.
}   break;

#line 1108 "./glsl.g"

case 58: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalAnd, expression(1), expression(3));
}   break;

#line 1115 "./glsl.g"

case 59: {
    // nothing to do.
}   break;

#line 1122 "./glsl.g"

case 60: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalXor, expression(1), expression(3));
}   break;

#line 1129 "./glsl.g"

case 61: {
    // nothing to do.
}   break;

#line 1136 "./glsl.g"

case 62: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalOr, expression(1), expression(3));
}   break;

#line 1143 "./glsl.g"

case 63: {
    // nothing to do.
}   break;

#line 1150 "./glsl.g"

case 64: {
    ast(1) = makeAstNode<TernaryExpressionAST>(AST::Kind_Conditional, expression(1), expression(3), expression(5));
}   break;

#line 1157 "./glsl.g"

case 65: {
    // nothing to do.
}   break;

#line 1164 "./glsl.g"

case 66: {
    ast(1) = makeAstNode<AssignmentExpressionAST>(sym(2).kind, expression(1), expression(3));
}   break;

#line 1171 "./glsl.g"

case 67: {
    sym(1).kind = AST::Kind_Assign;
}   break;

#line 1178 "./glsl.g"

case 68: {
    sym(1).kind = AST::Kind_AssignMultiply;
}   break;

#line 1185 "./glsl.g"

case 69: {
    sym(1).kind = AST::Kind_AssignDivide;
}   break;

#line 1192 "./glsl.g"

case 70: {
    sym(1).kind = AST::Kind_AssignModulus;
}   break;

#line 1199 "./glsl.g"

case 71: {
    sym(1).kind = AST::Kind_AssignPlus;
}   break;

#line 1206 "./glsl.g"

case 72: {
    sym(1).kind = AST::Kind_AssignMinus;
}   break;

#line 1213 "./glsl.g"

case 73: {
    sym(1).kind = AST::Kind_AssignShiftLeft;
}   break;

#line 1220 "./glsl.g"

case 74: {
    sym(1).kind = AST::Kind_AssignShiftRight;
}   break;

#line 1227 "./glsl.g"

case 75: {
    sym(1).kind = AST::Kind_AssignAnd;
}   break;

#line 1234 "./glsl.g"

case 76: {
    sym(1).kind = AST::Kind_AssignXor;
}   break;

#line 1241 "./glsl.g"

case 77: {
    sym(1).kind = AST::Kind_AssignOr;
}   break;

#line 1248 "./glsl.g"

case 78: {
    // nothing to do.
}   break;

#line 1255 "./glsl.g"

case 79: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Comma, expression(1), expression(3));
}   break;

#line 1262 "./glsl.g"

case 80: {
    // nothing to do.
}   break;

#line 1269 "./glsl.g"

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

#line 1283 "./glsl.g"

case 82: {
    ast(1) = makeAstNode<InitDeclarationAST>(sym(1).declaration_list);
}   break;

#line 1290 "./glsl.g"

case 83: {
    ast(1) = makeAstNode<PrecisionDeclarationAST>(sym(2).precision, type(3));
}   break;

#line 1297 "./glsl.g"

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

#line 1319 "./glsl.g"

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

#line 1351 "./glsl.g"

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

#line 1385 "./glsl.g"

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

#line 1419 "./glsl.g"

case 88: {
    TypeAST *type = makeAstNode<QualifiedTypeAST>
        (sym(1).type_qualifier.qualifier, (TypeAST *)nullptr,
         sym(1).type_qualifier.layout_list);
    ast(1) = makeAstNode<TypeDeclarationAST>(type);
}   break;

#line 1429 "./glsl.g"

case 89: {
    TypeAST *type = makeAstNode<QualifiedTypeAST>
        (sym(1).type_qualifier.qualifier, (TypeAST *)nullptr,
         sym(1).type_qualifier.layout_list);
    ast(1) = makeAstNode<TypeAndVariableDeclarationAST>
        (makeAstNode<TypeDeclarationAST>(type),
         makeAstNode<VariableDeclarationAST>(type, string(2)));
}   break;

#line 1441 "./glsl.g"

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

#line 1469 "./glsl.g"

case 91: {
    IdentifierExpressionAST *id = makeAstNode<IdentifierExpressionAST>(string(2));
    sym(1).identifier_list = makeAstNode<List<IdentifierExpressionAST *>>(id);
}   break;

#line 1477 "./glsl.g"

case 92: {
    IdentifierExpressionAST *id = makeAstNode<IdentifierExpressionAST>(string(3));
    List<IdentifierExpressionAST *> *nextId = makeAstNode<List<IdentifierExpressionAST *>>(id);
    sym(1).identifier_list = appendLists(sym(1).identifier_list, nextId);
}   break;

#line 1486 "./glsl.g"

case 93: {
    function(1)->finishParams();
}   break;

#line 1493 "./glsl.g"

case 94: {
    // nothing to do.
}   break;

#line 1500 "./glsl.g"

case 95: {
    // nothing to do.
}   break;

#line 1507 "./glsl.g"

case 96: {
    function(1)->params = makeAstNode< List<ParameterDeclarationAST *> >
        (sym(2).param_declaration);
}   break;

#line 1515 "./glsl.g"

case 97: {
    function(1)->params = makeAstNode< List<ParameterDeclarationAST *> >
        (function(1)->params, sym(3).param_declaration);
}   break;

#line 1523 "./glsl.g"

case 98: {
    function(1) = makeAstNode<FunctionDeclarationAST>(type(1), string(2));
}   break;

#line 1530 "./glsl.g"

case 99: {
    sym(1).param_declarator.type = type(1);
    sym(1).param_declarator.name = string(2);
}   break;

#line 1538 "./glsl.g"

case 100: {
    sym(1).param_declarator.type = makeAstNode<ArrayTypeAST>(type(1), sym(3).array_specifier);
    sym(1).param_declarator.name = string(2);
}   break;

#line 1546 "./glsl.g"

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

#line 1562 "./glsl.g"

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

#line 1578 "./glsl.g"

case 103: {
    sym(1).qualifier |= ParameterDeclarationAST::None; // or should we just do nothing?
}   break;

#line 1585 "./glsl.g"

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

#line 1617 "./glsl.g"

case 105: {
    sym(1).qualifier = ParameterDeclarationAST::In;
}   break;

#line 1624 "./glsl.g"

case 106: {
    sym(1).qualifier = ParameterDeclarationAST::Out;
}   break;

#line 1631 "./glsl.g"

case 107: {
    sym(1).qualifier = ParameterDeclarationAST::InOut;
}   break;

#line 1638 "./glsl.g"

case 108: {
    sym(1).qualifier = ParameterDeclarationAST::Const;
}   break;

#line 1645 "./glsl.g"

case 109: {
    sym(1).qualifier = ParameterDeclarationAST::Precise;
}   break;

#line 1652 "./glsl.g"

case 110: {
    sym(1).qualifier = ParameterDeclarationAST::Lowp;
}   break;

#line 1659 "./glsl.g"

case 111: {
    sym(1).qualifier = ParameterDeclarationAST::Mediump;
}   break;

#line 1666 "./glsl.g"

case 112: {
    sym(1).qualifier = ParameterDeclarationAST::Highp;
}   break;

#line 1673 "./glsl.g"

case 113: {
    sym(1).qualifier = ParameterDeclarationAST::Coherent;
}   break;

#line 1680 "./glsl.g"

case 114: {
    sym(1).qualifier = ParameterDeclarationAST::Volatile;
}   break;

#line 1687 "./glsl.g"

case 115: {
    sym(1).qualifier = ParameterDeclarationAST::Restrict;
}   break;

#line 1694 "./glsl.g"

case 116: {
    sym(1).qualifier = ParameterDeclarationAST::Readonly;
}   break;

#line 1701 "./glsl.g"

case 117: {
    sym(1).qualifier = ParameterDeclarationAST::Writeonly;
}   break;

#line 1708 "./glsl.g"

case 118: {
    // nothing to do.
}   break;

#line 1715 "./glsl.g"

case 119: {
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
        (sym(1).declaration);
}   break;

#line 1723 "./glsl.g"

case 120: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1733 "./glsl.g"

case 121: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type, sym(4).array_specifier);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1744 "./glsl.g"

case 122: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type, sym(4).array_specifier);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>
            (type, string(3), expression(6));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1756 "./glsl.g"

case 123: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>
            (type, string(3), expression(5));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1767 "./glsl.g"

case 124: {
    ast(1) = makeAstNode<TypeDeclarationAST>(type(1));
}   break;

#line 1774 "./glsl.g"

case 125: {
    ast(1) = makeAstNode<VariableDeclarationAST>(type(1), string(2));
}   break;

#line 1781 "./glsl.g"

case 126: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1), sym(3).array_specifier), string(2));
}   break;

#line 1789 "./glsl.g"

case 127: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1), sym(3).array_specifier), string(2), expression(5));
}   break;

#line 1797 "./glsl.g"

case 128: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (type(1), string(2), expression(4));
}   break;

#line 1805 "./glsl.g"

case 129: {
    ast(1) = makeAstNode<QualifiedTypeAST>(0, type(1), (List<LayoutQualifierAST *> *)nullptr);
}   break;

#line 1812 "./glsl.g"

case 130: {
    ast(1) = makeAstNode<QualifiedTypeAST>
        (sym(1).type_qualifier.qualifier, type(2),
         sym(1).type_qualifier.layout_list);
}   break;

#line 1821 "./glsl.g"

case 131: {
    sym(1).qualifier = QualifiedTypeAST::Invariant;
}   break;

#line 1828 "./glsl.g"

case 132: {
    sym(1).qualifier = QualifiedTypeAST::Smooth;
}   break;

#line 1835 "./glsl.g"

case 133: {
    sym(1).qualifier = QualifiedTypeAST::Flat;
}   break;

#line 1842 "./glsl.g"

case 134: {
    sym(1).qualifier = QualifiedTypeAST::NoPerspective;
}   break;

#line 1849 "./glsl.g"

case 135: {
    sym(1).type_qualifier.layout_list = sym(3).layout_list;
}   break;

#line 1856 "./glsl.g"

case 136: {
    sym(1).layout_list = makeAstNode< List<LayoutQualifierAST *> >(sym(1).layout);
}   break;

#line 1863 "./glsl.g"

case 137: {
    sym(1).layout_list = makeAstNode< List<LayoutQualifierAST *> >(sym(1).layout_list, sym(3).layout);
}   break;

#line 1870 "./glsl.g"

case 138: {
    sym(1).layout = makeAstNode<LayoutQualifierAST>(string(1), (const QString *)nullptr);
}   break;

#line 1877 "./glsl.g"

case 139: {
    sym(1).layout = makeAstNode<LayoutQualifierAST>(string(1), string(3));
}   break;

#line 1884 "./glsl.g"

case 140: {
    sym(1).layout = makeAstNode<LayoutQualifierAST>(string(1), (const QString *)nullptr);
}   break;

#line 1891 "./glsl.g"

case 141: {
    sym(1).precision = TypeAST::Precise;
}   break;

#line 1898 "./glsl.g"

case 142: {
    // nothing to do.
}   break;

#line 1905 "./glsl.g"

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

#line 1923 "./glsl.g"

case 144: {
    int qualifier = sym(1).qualifier;
    sym(1).type_qualifier = {TypeAST::PrecUnspecified, 0, nullptr, nullptr};
    sym(1).type_qualifier.qualifier = qualifier;
}   break;

#line 1932 "./glsl.g"

case 145: {
    List<NamedTypeAST *> *typeNameList = sym(1).type_name_list;
    sym(1).type_qualifier = {TypeAST::PrecUnspecified, 0, nullptr, nullptr};
    sym(1).type_qualifier.qualifier = QualifiedTypeAST::Subroutine;
    sym(1).type_qualifier.type_name_list = typeNameList;
}   break;

#line 1942 "./glsl.g"

case 146: {
    // nothing to do.
}   break;

#line 1949 "./glsl.g"

case 147: {
    TypeAST::Precision precision = sym(1).precision;
    sym(1).type_qualifier = {TypeAST::PrecUnspecified, 0, nullptr, nullptr};
    sym(1).type_qualifier.precision = precision;
}   break;

#line 1958 "./glsl.g"

case 148: {
    int qualifier = sym(1).qualifier;
    sym(1).type_qualifier = {TypeAST::PrecUnspecified, 0, nullptr, nullptr};
    sym(1).type_qualifier.qualifier = qualifier;
}   break;

#line 1967 "./glsl.g"

case 149: {
    int qualifier = sym(1).qualifier;
    sym(1).type_qualifier = {TypeAST::PrecUnspecified, 0, nullptr, nullptr};
    sym(1).type_qualifier.qualifier = qualifier;
}   break;

#line 1976 "./glsl.g"

case 150: {
    TypeAST::Precision precision = sym(1).precision;
    sym(1).type_qualifier = {TypeAST::PrecUnspecified, 0, nullptr, nullptr};
    sym(1).type_qualifier.precision = precision;
}   break;

#line 1985 "./glsl.g"

case 151: {
    sym(1).qualifier = QualifiedTypeAST::Const;
}   break;

#line 1992 "./glsl.g"

case 152: {
    sym(1).qualifier = QualifiedTypeAST::Attribute;
}   break;

#line 1999 "./glsl.g"

case 153: {
    sym(1).qualifier = QualifiedTypeAST::Varying;
}   break;

#line 2006 "./glsl.g"

case 154: {
    sym(1).qualifier = QualifiedTypeAST::Centroid;
}   break;

#line 2013 "./glsl.g"

case 155: {
    sym(1).qualifier = QualifiedTypeAST::In;
}   break;

#line 2020 "./glsl.g"

case 156: {
    sym(1).qualifier = QualifiedTypeAST::Out;
}   break;

#line 2027 "./glsl.g"

case 157: {
    sym(1).qualifier = QualifiedTypeAST::Patch;
}   break;

#line 2034 "./glsl.g"

case 158: {
    sym(1).qualifier = QualifiedTypeAST::Sample;
}   break;

#line 2041 "./glsl.g"

case 159: {
    sym(1).qualifier = QualifiedTypeAST::Uniform;
}   break;

#line 2048 "./glsl.g"

case 160: {
    sym(1).qualifier = QualifiedTypeAST::Buffer;
}   break;

#line 2055 "./glsl.g"

case 161: {
    sym(1).qualifier = QualifiedTypeAST::Shared;
}   break;

#line 2062 "./glsl.g"

case 162: {
    sym(1).qualifier = QualifiedTypeAST::Coherent;
} break;

#line 2069 "./glsl.g"

case 163: {
    sym(1).qualifier = QualifiedTypeAST::Volatile;
}   break;

#line 2076 "./glsl.g"

case 164: {
    sym(1).qualifier = QualifiedTypeAST::Restrict;
}   break;

#line 2083 "./glsl.g"

case 165: {
    sym(1).qualifier = QualifiedTypeAST::Readonly;
}   break;

#line 2090 "./glsl.g"

case 166: {
    sym(1).qualifier = QualifiedTypeAST::Writeonly;
}   break;

#line 2097 "./glsl.g"

case 167: {
    sym(1).qualifier = QualifiedTypeAST::Subroutine;
}   break;

#line 2104 "./glsl.g"

case 168: {
    sym(1).type_name_list = sym(3).type_name_list;
}   break;

#line 2111 "./glsl.g"

case 169: {
    NamedTypeAST *namedType = makeAstNode<NamedTypeAST>(string(1));
    sym(1).type_name_list = makeAstNode<List<NamedTypeAST *>>(namedType);
}   break;

#line 2119 "./glsl.g"

case 170: {
    NamedTypeAST *namedType = makeAstNode<NamedTypeAST>(string(3));
    sym(1).type_name_list = appendLists(sym(1).type_name_list,
                                        makeAstNode<List<NamedTypeAST *>>(namedType));

}   break;

#line 2129 "./glsl.g"

case 171: {
    // nothing to do.
}   break;

#line 2136 "./glsl.g"

case 172: {
    ast(1) = makeAstNode<ArrayTypeAST>(type(1), sym(2).array_specifier);
}   break;

#line 2143 "./glsl.g"

case 173: {
    sym(1).array_specifier = makeAstNode< List<ArrayTypeAST::ArraySpecAST *>>(nullptr);
}   break;

#line 2150 "./glsl.g"

case 174: {
    ArrayTypeAST::ArraySpecAST *spec = makeAstNode<ArrayTypeAST::ArraySpecAST>(expression(2));
    sym(1).array_specifier = makeAstNode< List<ArrayTypeAST::ArraySpecAST *>>(spec);
}   break;

#line 2158 "./glsl.g"

case 175: {
    List<ArrayTypeAST::ArraySpecAST *> *empty
        = makeAstNode< List<ArrayTypeAST::ArraySpecAST *>>(nullptr);
    sym(1).array_specifier = appendLists(sym(1).array_specifier, empty);
}   break;

#line 2167 "./glsl.g"

case 176: {
    ArrayTypeAST::ArraySpecAST *spec = makeAstNode<ArrayTypeAST::ArraySpecAST>(expression(3));
    List<ArrayTypeAST::ArraySpecAST *> *specifier
        = makeAstNode< List<ArrayTypeAST::ArraySpecAST *>>(spec);
    sym(1).array_specifier = appendLists(sym(1).array_specifier, specifier);
}   break;

#line 2177 "./glsl.g"

case 177: {
    ast(1) = makeBasicType(T_VOID);
}   break;

#line 2184 "./glsl.g"

case 178: {
    ast(1) = makeBasicType(T_FLOAT);
}   break;

#line 2191 "./glsl.g"

case 179: {
    ast(1) = makeBasicType(T_DOUBLE);
}   break;

#line 2198 "./glsl.g"

case 180: {
    ast(1) = makeBasicType(T_INT);
}   break;

#line 2205 "./glsl.g"

case 181: {
    ast(1) = makeBasicType(T_UINT);
}   break;

#line 2212 "./glsl.g"

case 182: {
    ast(1) = makeBasicType(T_ATOMIC_UINT);
} break;

#line 2219 "./glsl.g"

case 183: {
    ast(1) = makeBasicType(T_BOOL);
}   break;

#line 2226 "./glsl.g"

case 184: {
    ast(1) = makeBasicType(T_VEC2);
}   break;

#line 2233 "./glsl.g"

case 185: {
    ast(1) = makeBasicType(T_VEC3);
}   break;

#line 2240 "./glsl.g"

case 186: {
    ast(1) = makeBasicType(T_VEC4);
}   break;

#line 2247 "./glsl.g"

case 187: {
    ast(1) = makeBasicType(T_DVEC2);
}   break;

#line 2254 "./glsl.g"

case 188: {
    ast(1) = makeBasicType(T_DVEC3);
}   break;

#line 2261 "./glsl.g"

case 189: {
    ast(1) = makeBasicType(T_DVEC4);
}   break;

#line 2268 "./glsl.g"

case 190: {
    ast(1) = makeBasicType(T_BVEC2);
}   break;

#line 2275 "./glsl.g"

case 191: {
    ast(1) = makeBasicType(T_BVEC3);
}   break;

#line 2282 "./glsl.g"

case 192: {
    ast(1) = makeBasicType(T_BVEC4);
}   break;

#line 2289 "./glsl.g"

case 193: {
    ast(1) = makeBasicType(T_IVEC2);
}   break;

#line 2296 "./glsl.g"

case 194: {
    ast(1) = makeBasicType(T_IVEC3);
}   break;

#line 2303 "./glsl.g"

case 195: {
    ast(1) = makeBasicType(T_IVEC4);
}   break;

#line 2310 "./glsl.g"

case 196: {
    ast(1) = makeBasicType(T_UVEC2);
}   break;

#line 2317 "./glsl.g"

case 197: {
    ast(1) = makeBasicType(T_UVEC3);
}   break;

#line 2324 "./glsl.g"

case 198: {
    ast(1) = makeBasicType(T_UVEC4);
}   break;

#line 2331 "./glsl.g"

case 199: {
    ast(1) = makeBasicType(T_MAT2);
}   break;

#line 2338 "./glsl.g"

case 200: {
    ast(1) = makeBasicType(T_MAT3);
}   break;

#line 2345 "./glsl.g"

case 201: {
    ast(1) = makeBasicType(T_MAT4);
}   break;

#line 2352 "./glsl.g"

case 202: {
    ast(1) = makeBasicType(T_MAT2);
}   break;

#line 2359 "./glsl.g"

case 203: {
    ast(1) = makeBasicType(T_MAT2X3);
}   break;

#line 2366 "./glsl.g"

case 204: {
    ast(1) = makeBasicType(T_MAT2X4);
}   break;

#line 2373 "./glsl.g"

case 205: {
    ast(1) = makeBasicType(T_MAT3X2);
}   break;

#line 2380 "./glsl.g"

case 206: {
    ast(1) = makeBasicType(T_MAT3);
}   break;

#line 2387 "./glsl.g"

case 207: {
    ast(1) = makeBasicType(T_MAT3X4);
}   break;

#line 2394 "./glsl.g"

case 208: {
    ast(1) = makeBasicType(T_MAT4X2);
}   break;

#line 2401 "./glsl.g"

case 209: {
    ast(1) = makeBasicType(T_MAT4X3);
}   break;

#line 2408 "./glsl.g"

case 210: {
    ast(1) = makeBasicType(T_MAT4);
}   break;

#line 2415 "./glsl.g"

case 211: {
    ast(1) = makeBasicType(T_DMAT2);
}   break;

#line 2422 "./glsl.g"

case 212: {
    ast(1) = makeBasicType(T_DMAT3);
}   break;

#line 2429 "./glsl.g"

case 213: {
    ast(1) = makeBasicType(T_DMAT4);
}   break;

#line 2436 "./glsl.g"

case 214: {
    ast(1) = makeBasicType(T_DMAT2);
}   break;

#line 2443 "./glsl.g"

case 215: {
    ast(1) = makeBasicType(T_DMAT2X3);
}   break;

#line 2450 "./glsl.g"

case 216: {
    ast(1) = makeBasicType(T_DMAT2X4);
}   break;

#line 2457 "./glsl.g"

case 217: {
    ast(1) = makeBasicType(T_DMAT3X2);
}   break;

#line 2464 "./glsl.g"

case 218: {
    ast(1) = makeBasicType(T_DMAT3);
}   break;

#line 2471 "./glsl.g"

case 219: {
    ast(1) = makeBasicType(T_DMAT3X4);
}   break;

#line 2478 "./glsl.g"

case 220: {
    ast(1) = makeBasicType(T_DMAT4X2);
}   break;

#line 2485 "./glsl.g"

case 221: {
    ast(1) = makeBasicType(T_DMAT4X3);
}   break;

#line 2492 "./glsl.g"

case 222: {
    ast(1) = makeBasicType(T_DMAT4);
}   break;

#line 2499 "./glsl.g"

case 223: {
    ast(1) = makeBasicType(T_SAMPLER1D);
}   break;

#line 2506 "./glsl.g"

case 224: {
    ast(1) = makeBasicType(T_SAMPLER2D);
}   break;

#line 2513 "./glsl.g"

case 225: {
    ast(1) = makeBasicType(T_SAMPLER3D);
}   break;

#line 2520 "./glsl.g"

case 226: {
    ast(1) = makeBasicType(T_SAMPLERCUBE);
}   break;

#line 2527 "./glsl.g"

case 227: {
    ast(1) = makeBasicType(T_SAMPLER1DSHADOW);
}   break;

#line 2534 "./glsl.g"

case 228: {
    ast(1) = makeBasicType(T_SAMPLER2DSHADOW);
}   break;

#line 2541 "./glsl.g"

case 229: {
    ast(1) = makeBasicType(T_SAMPLERCUBESHADOW);
}   break;

#line 2548 "./glsl.g"

case 230: {
    ast(1) = makeBasicType(T_SAMPLER1DARRAY);
}   break;

#line 2555 "./glsl.g"

case 231: {
    ast(1) = makeBasicType(T_SAMPLER2DARRAY);
}   break;

#line 2562 "./glsl.g"

case 232: {
    ast(1) = makeBasicType(T_SAMPLER1DARRAYSHADOW);
}   break;

#line 2569 "./glsl.g"

case 233: {
    ast(1) = makeBasicType(T_SAMPLER2DARRAYSHADOW);
}   break;

#line 2576 "./glsl.g"

case 234: {
    ast(1) = makeBasicType(T_SAMPLERCUBEARRAY);
}   break;

#line 2583 "./glsl.g"

case 235: {
    ast(1) = makeBasicType(T_SAMPLERCUBEARRAYSHADOW);
}   break;

#line 2590 "./glsl.g"

case 236: {
    ast(1) = makeBasicType(T_ISAMPLER1D);
}   break;

#line 2597 "./glsl.g"

case 237: {
    ast(1) = makeBasicType(T_ISAMPLER2D);
}   break;

#line 2604 "./glsl.g"

case 238: {
    ast(1) = makeBasicType(T_ISAMPLER3D);
}   break;

#line 2611 "./glsl.g"

case 239: {
    ast(1) = makeBasicType(T_ISAMPLERCUBE);
}   break;

#line 2618 "./glsl.g"

case 240: {
    ast(1) = makeBasicType(T_ISAMPLER1DARRAY);
}   break;

#line 2625 "./glsl.g"

case 241: {
    ast(1) = makeBasicType(T_ISAMPLER2DARRAY);
}   break;

#line 2632 "./glsl.g"

case 242: {
    ast(1) = makeBasicType(T_ISAMPLERCUBEARRAY);
}   break;

#line 2639 "./glsl.g"

case 243: {
    ast(1) = makeBasicType(T_USAMPLER1D);
}   break;

#line 2646 "./glsl.g"

case 244: {
    ast(1) = makeBasicType(T_USAMPLER2D);
}   break;

#line 2653 "./glsl.g"

case 245: {
    ast(1) = makeBasicType(T_USAMPLER3D);
}   break;

#line 2660 "./glsl.g"

case 246: {
    ast(1) = makeBasicType(T_USAMPLERCUBE);
}   break;

#line 2667 "./glsl.g"

case 247: {
    ast(1) = makeBasicType(T_USAMPLER1DARRAY);
}   break;

#line 2674 "./glsl.g"

case 248: {
    ast(1) = makeBasicType(T_USAMPLER2DARRAY);
}   break;

#line 2681 "./glsl.g"

case 249: {
    ast(1) = makeBasicType(T_USAMPLERCUBEARRAY);
}   break;

#line 2688 "./glsl.g"

case 250: {
    ast(1) = makeBasicType(T_SAMPLER2DRECT);
}   break;

#line 2695 "./glsl.g"

case 251: {
    ast(1) = makeBasicType(T_SAMPLER2DRECTSHADOW);
}   break;

#line 2702 "./glsl.g"

case 252: {
    ast(1) = makeBasicType(T_ISAMPLER2DRECT);
}   break;

#line 2709 "./glsl.g"

case 253: {
    ast(1) = makeBasicType(T_USAMPLER2DRECT);
}   break;

#line 2716 "./glsl.g"

case 254: {
    ast(1) = makeBasicType(T_SAMPLERBUFFER);
}   break;

#line 2723 "./glsl.g"

case 255: {
    ast(1) = makeBasicType(T_ISAMPLERBUFFER);
}   break;

#line 2730 "./glsl.g"

case 256: {
    ast(1) = makeBasicType(T_USAMPLERBUFFER);
}   break;

#line 2737 "./glsl.g"

case 257: {
    ast(1) = makeBasicType(T_SAMPLER2DMS);
}   break;

#line 2744 "./glsl.g"

case 258: {
    ast(1) = makeBasicType(T_ISAMPLER2DMS);
}   break;

#line 2751 "./glsl.g"

case 259: {
    ast(1) = makeBasicType(T_USAMPLER2DMS);
}   break;

#line 2758 "./glsl.g"

case 260: {
    ast(1) = makeBasicType(T_SAMPLER2DMSARRAY);
}   break;

#line 2765 "./glsl.g"

case 261: {
    ast(1) = makeBasicType(T_ISAMPLER2DMSARRAY);
}   break;

#line 2772 "./glsl.g"

case 262: {
    ast(1) = makeBasicType(T_USAMPLER2DMSARRAY);
}   break;

#line 2779 "./glsl.g"

case 263: {
    ast(1) = makeBasicType(T_IIMAGE1D);
} break;

#line 2786 "./glsl.g"

case 264: {
    ast(1) = makeBasicType(T_IIMAGE1DARRAY);
} break;

#line 2793 "./glsl.g"

case 265: {
    ast(1) = makeBasicType(T_IIMAGE2D);
} break;

#line 2800 "./glsl.g"

case 266: {
    ast(1) = makeBasicType(T_IIMAGE2DARRAY);
} break;

#line 2807 "./glsl.g"

case 267: {
    ast(1) = makeBasicType(T_IIMAGE2DMS);
} break;

#line 2814 "./glsl.g"

case 268: {
    ast(1) = makeBasicType(T_IIMAGE2DMSARRAY);
} break;

#line 2821 "./glsl.g"

case 269: {
    ast(1) = makeBasicType(T_IIMAGE2DRECT);
} break;

#line 2828 "./glsl.g"

case 270: {
    ast(1) = makeBasicType(T_IIMAGE3D);
} break;

#line 2835 "./glsl.g"

case 271: {
    ast(1) = makeBasicType(T_IIMAGEBUFFER);
} break;

#line 2842 "./glsl.g"

case 272: {
    ast(1) = makeBasicType(T_IIMAGECUBE);
} break;

#line 2849 "./glsl.g"

case 273: {
    ast(1) = makeBasicType(T_IIMAGECUBEARRAY);
} break;

#line 2856 "./glsl.g"

case 274: {
    ast(1) = makeBasicType(T_IMAGE1D);
} break;

#line 2863 "./glsl.g"

case 275: {
    ast(1) = makeBasicType(T_IMAGE1DARRAY);
} break;

#line 2870 "./glsl.g"

case 276: {
    ast(1) = makeBasicType(T_IMAGE2D);
} break;

#line 2877 "./glsl.g"

case 277: {
    ast(1) = makeBasicType(T_IMAGE2DARRAY);
} break;

#line 2884 "./glsl.g"

case 278: {
    ast(1) = makeBasicType(T_IMAGE2DMS);
} break;

#line 2891 "./glsl.g"

case 279: {
    ast(1) = makeBasicType(T_IMAGE2DMSARRAY);
} break;

#line 2898 "./glsl.g"

case 280: {
    ast(1) = makeBasicType(T_IMAGE2DRECT);
} break;

#line 2905 "./glsl.g"

case 281: {
    ast(1) = makeBasicType(T_IMAGE3D);
} break;

#line 2912 "./glsl.g"

case 282: {
    ast(1) = makeBasicType(T_IMAGEBUFFER);
} break;

#line 2919 "./glsl.g"

case 283: {
    ast(1) = makeBasicType(T_IMAGECUBE);
} break;

#line 2926 "./glsl.g"

case 284: {
    ast(1) = makeBasicType(T_IMAGECUBEARRAY);
} break;

#line 2933 "./glsl.g"

case 285: {
    ast(1) = makeBasicType(T_UIMAGE1D);
} break;

#line 2940 "./glsl.g"

case 286: {
    ast(1) = makeBasicType(T_UIMAGE1DARRAY);
} break;

#line 2947 "./glsl.g"

case 287: {
    ast(1) = makeBasicType(T_UIMAGE2D);
} break;

#line 2954 "./glsl.g"

case 288: {
    ast(1) = makeBasicType(T_UIMAGE2DARRAY);
} break;

#line 2961 "./glsl.g"

case 289: {
    ast(1) = makeBasicType(T_UIMAGE2DMS);
} break;

#line 2968 "./glsl.g"

case 290: {
    ast(1) = makeBasicType(T_UIMAGE2DMSARRAY);
} break;

#line 2975 "./glsl.g"

case 291: {
    ast(1) = makeBasicType(T_UIMAGE2DRECT);
} break;

#line 2982 "./glsl.g"

case 292: {
    ast(1) = makeBasicType(T_UIMAGE3D);
} break;

#line 2989 "./glsl.g"

case 293: {
    ast(1) = makeBasicType(T_UIMAGEBUFFER);
} break;

#line 2996 "./glsl.g"

case 294: {
    ast(1) = makeBasicType(T_UIMAGECUBE);
} break;

#line 3003 "./glsl.g"

case 295: {
    ast(1) = makeBasicType(T_UIMAGECUBEARRAY);
} break;

#line 3010 "./glsl.g"

case 296: {
    // nothing to do.
}   break;

#line 3017 "./glsl.g"

case 297: {
    ast(1) = makeAstNode<NamedTypeAST>(string(1));
}   break;

#line 3026 "./glsl.g"

case 298: {
    ast(1) = makeBasicType(T_ISUBPASSINPUT);
}   break;

#line 3033 "./glsl.g"

case 299: {
    ast(1) = makeBasicType(T_ISUBPASSINPUTMS);
}   break;

#line 3040 "./glsl.g"

case 300: {
    ast(1) = makeBasicType(T_ITEXTURE1D);
}   break;

#line 3047 "./glsl.g"

case 301: {
    ast(1) = makeBasicType(T_ITEXTURE1DARRAY);
}   break;

#line 3054 "./glsl.g"

case 302: {
    ast(1) = makeBasicType(T_ITEXTURE2D);
}   break;

#line 3061 "./glsl.g"

case 303: {
    ast(1) = makeBasicType(T_ITEXTURE2DARRAY);
}   break;

#line 3068 "./glsl.g"

case 304: {
    ast(1) = makeBasicType(T_ITEXTURE2DMS);
}   break;

#line 3075 "./glsl.g"

case 305: {
    ast(1) = makeBasicType(T_ITEXTURE2DMSARRAY);
}   break;

#line 3082 "./glsl.g"

case 306: {
    ast(1) = makeBasicType(T_ITEXTURE2DRECT);
}   break;

#line 3089 "./glsl.g"

case 307: {
    ast(1) = makeBasicType(T_ITEXTURE3D);
}   break;

#line 3096 "./glsl.g"

case 308: {
    ast(1) = makeBasicType(T_ITEXTUREBUFFER);
}   break;

#line 3103 "./glsl.g"

case 309: {
    ast(1) = makeBasicType(T_ITEXTURECUBE);
}   break;

#line 3110 "./glsl.g"

case 310: {
    ast(1) = makeBasicType(T_ITEXTURECUBEARRAY);
}   break;

#line 3117 "./glsl.g"

case 311: {
    ast(1) = makeBasicType(T_SAMPLER);
}   break;

#line 3124 "./glsl.g"

case 312: {
    ast(1) = makeBasicType(T_SAMPLERSHADOW);
}   break;

#line 3131 "./glsl.g"

case 313: {
    ast(1) = makeBasicType(T_SUBPASSINPUT);
}   break;

#line 3138 "./glsl.g"

case 314: {
    ast(1) = makeBasicType(T_SUBPASSINPUTMS);
}   break;

#line 3145 "./glsl.g"

case 315: {
    ast(1) = makeBasicType(T_TEXTURE1D);
}   break;

#line 3152 "./glsl.g"

case 316: {
    ast(1) = makeBasicType(T_TEXTURE1DARRAY);
}   break;

#line 3159 "./glsl.g"

case 317: {
    ast(1) = makeBasicType(T_TEXTURE2D);
}   break;

#line 3166 "./glsl.g"

case 318: {
    ast(1) = makeBasicType(T_TEXTURE2DARRAY);
}   break;

#line 3173 "./glsl.g"

case 319: {
    ast(1) = makeBasicType(T_TEXTURE2DMS);
}   break;

#line 3180 "./glsl.g"

case 320: {
    ast(1) = makeBasicType(T_TEXTURE2DMSARRAY);
}   break;

#line 3187 "./glsl.g"

case 321: {
    ast(1) = makeBasicType(T_TEXTURE2DRECT);
}   break;

#line 3194 "./glsl.g"

case 322: {
    ast(1) = makeBasicType(T_TEXTURE3D);
}   break;

#line 3201 "./glsl.g"

case 323: {
    ast(1) = makeBasicType(T_TEXTUREBUFFER);
}   break;

#line 3208 "./glsl.g"

case 324: {
    ast(1) = makeBasicType(T_TEXTURECUBE);
}   break;

#line 3215 "./glsl.g"

case 325: {
    ast(1) = makeBasicType(T_TEXTURECUBEARRAY);
}   break;

#line 3222 "./glsl.g"

case 326: {
    ast(1) = makeBasicType(T_USUBPASSINPUT);
}   break;

#line 3229 "./glsl.g"

case 327: {
    ast(1) = makeBasicType(T_USUBPASSINPUTMS);
}   break;

#line 3236 "./glsl.g"

case 328: {
    ast(1) = makeBasicType(T_UTEXTURE1D);
}   break;

#line 3243 "./glsl.g"

case 329: {
    ast(1) = makeBasicType(T_UTEXTURE1DARRAY);
}   break;

#line 3250 "./glsl.g"

case 330: {
    ast(1) = makeBasicType(T_UTEXTURE2D);
}   break;

#line 3257 "./glsl.g"

case 331: {
    ast(1) = makeBasicType(T_UTEXTURE2DARRAY);
}   break;

#line 3264 "./glsl.g"

case 332: {
    ast(1) = makeBasicType(T_UTEXTURE2DMS);
}   break;

#line 3271 "./glsl.g"

case 333: {
    ast(1) = makeBasicType(T_UTEXTURE2DMSARRAY);
}   break;

#line 3278 "./glsl.g"

case 334: {
    ast(1) = makeBasicType(T_UTEXTURE2DRECT);
}   break;

#line 3285 "./glsl.g"

case 335: {
    ast(1) = makeBasicType(T_UTEXTURE3D);
}   break;

#line 3292 "./glsl.g"

case 336: {
    ast(1) = makeBasicType(T_UTEXTUREBUFFER);
}   break;

#line 3299 "./glsl.g"

case 337: {
    ast(1) = makeBasicType(T_UTEXTURECUBE);
}   break;

#line 3306 "./glsl.g"

case 338: {
    ast(1) = makeBasicType(T_UTEXTURECUBEARRAY);
}   break;

#line 3315 "./glsl.g"

case 339: {
    sym(1).precision = TypeAST::Highp;
}   break;

#line 3322 "./glsl.g"

case 340: {
    sym(1).precision = TypeAST::Mediump;
}   break;

#line 3329 "./glsl.g"

case 341: {
    sym(1).precision = TypeAST::Lowp;
}   break;

#line 3336 "./glsl.g"

case 342: {
    ast(1) = makeAstNode<StructTypeAST>(string(2), sym(4).field_list);
}   break;

#line 3343 "./glsl.g"

case 343: {
    ast(1) = makeAstNode<StructTypeAST>(sym(3).field_list);
}   break;

#line 3350 "./glsl.g"

case 344: {
    // nothing to do.
}   break;

#line 3357 "./glsl.g"

case 345: {
    sym(1).field_list = appendLists(sym(1).field_list, sym(2).field_list);
}   break;

#line 3364 "./glsl.g"

case 346: {
    sym(1).field_list = StructTypeAST::fixInnerTypes(type(1), sym(2).field_list);
}   break;

#line 3371 "./glsl.g"

case 347: {
    sym(1).field_list = StructTypeAST::fixInnerTypes
        (makeAstNode<QualifiedTypeAST>
            (sym(1).type_qualifier.qualifier, type(2),
             sym(1).type_qualifier.layout_list), sym(3).field_list);
}   break;

#line 3381 "./glsl.g"

case 348: {
    // nothing to do.
    sym(1).field_list = makeAstNode< List<StructTypeAST::Field *> >(sym(1).field);
}   break;

#line 3389 "./glsl.g"

case 349: {
    sym(1).field_list = makeAstNode< List<StructTypeAST::Field *> >(sym(1).field_list, sym(3).field);
}   break;

#line 3396 "./glsl.g"

case 350: {
    sym(1).field = makeAstNode<StructTypeAST::Field>(string(1));
}   break;

#line 3403 "./glsl.g"

case 351: {
    sym(1).field = makeAstNode<StructTypeAST::Field>
        (string(1), makeAstNode<ArrayTypeAST>((TypeAST *)nullptr, sym(2).array_specifier));
}   break;

#line 3411 "./glsl.g"

case 352: {
    List<ExpressionAST *> *expressionList = makeAstNode<List<ExpressionAST *>>(expression(1));
    sym(1).expression = makeAstNode<InitializerListExpressionAST>(expressionList);
}   break;

#line 3419 "./glsl.g"

case 353: {
    sym(1).expression = makeAstNode<InitializerListExpressionAST>(sym(2).expression_list);
}   break;

#line 3426 "./glsl.g"

case 354: {
    sym(1).expression = makeAstNode<InitializerListExpressionAST>(sym(2).expression_list);
}   break;

#line 3433 "./glsl.g"

case 355: {
    sym(1).expression_list = makeAstNode<List<ExpressionAST *>>(expression(1));
}   break;

#line 3440 "./glsl.g"

case 356: {
    sym(1).expression_list = makeAstNode<List<ExpressionAST *>>
            (sym(1).expression_list, expression(3));
}   break;

#line 3448 "./glsl.g"

case 357: {
    ast(1) = makeAstNode<DeclarationStatementAST>(sym(1).declaration);
}   break;

#line 3455 "./glsl.g"

case 358: {
    // nothing to do.
}   break;

#line 3462 "./glsl.g"

case 359: {
    // nothing to do.
}   break;

#line 3469 "./glsl.g"

case 360: {
    // nothing to do.
}   break;

#line 3476 "./glsl.g"

case 361: {
    // nothing to do.
}   break;

#line 3483 "./glsl.g"

case 362: {
    // nothing to do.
}   break;

#line 3490 "./glsl.g"

case 363: {
    // nothing to do.
}   break;

#line 3497 "./glsl.g"

case 364: {
    // nothing to do.
}   break;

#line 3504 "./glsl.g"

case 365: {
    // nothing to do.
}   break;

#line 3511 "./glsl.g"

case 366: {
    // nothing to do.
}   break;

#line 3518 "./glsl.g"

case 367: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>();
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(2)).end();
    ast(1) = stmt;
}   break;

#line 3528 "./glsl.g"

case 368: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>(sym(2).statement_list);
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(3)).end();
    ast(1) = stmt;
}   break;

#line 3538 "./glsl.g"

case 369: {
    // nothing to do.
}   break;

#line 3545 "./glsl.g"

case 370: {
    // nothing to do.
}   break;

#line 3552 "./glsl.g"

case 371: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>();
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(2)).end();
    ast(1) = stmt;
}   break;

#line 3562 "./glsl.g"

case 372: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>(sym(2).statement_list);
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(3)).end();
    ast(1) = stmt;
}   break;

#line 3572 "./glsl.g"

case 373: {
    sym(1).statement_list = makeAstNode< List<StatementAST *> >(sym(1).statement);
}   break;

#line 3579 "./glsl.g"

case 374: {
    sym(1).statement_list = makeAstNode< List<StatementAST *> >(sym(1).statement_list, sym(2).statement);
}   break;

#line 3586 "./glsl.g"

case 375: {
    ast(1) = makeAstNode<CompoundStatementAST>();  // Empty statement
}   break;

#line 3593 "./glsl.g"

case 376: {
    ast(1) = makeAstNode<ExpressionStatementAST>(expression(1));
}   break;

#line 3600 "./glsl.g"

case 377: {
    ast(1) = makeAstNode<IfStatementAST>(expression(3), sym(5).ifstmt.thenClause, sym(5).ifstmt.elseClause);
}   break;

#line 3607 "./glsl.g"

case 378: {
    sym(1).ifstmt.thenClause = statement(1);
    sym(1).ifstmt.elseClause = statement(3);
}   break;

#line 3615 "./glsl.g"

case 379: {
    sym(1).ifstmt.thenClause = statement(1);
    sym(1).ifstmt.elseClause = nullptr;
}   break;

#line 3623 "./glsl.g"

case 380: {
    // nothing to do.
}   break;

#line 3630 "./glsl.g"

case 381: {
    ast(1) = makeAstNode<DeclarationExpressionAST>
        (type(1), string(2), expression(4));
}   break;

#line 3638 "./glsl.g"

case 382: {
    ast(1) = makeAstNode<SwitchStatementAST>(expression(3), statement(6));
}   break;

#line 3645 "./glsl.g"

case 383: {
    ast(1) = makeAstNode<CompoundStatementAST>();
}   break;

#line 3652 "./glsl.g"

case 384: {
    ast(1) = makeAstNode<CompoundStatementAST>(sym(1).statement_list);
}   break;

#line 3659 "./glsl.g"

case 385: {
    ast(1) = makeAstNode<CaseLabelStatementAST>(expression(2));
}   break;

#line 3666 "./glsl.g"

case 386: {
    ast(1) = makeAstNode<CaseLabelStatementAST>();
}   break;

#line 3673 "./glsl.g"

case 387: {
    ast(1) = makeAstNode<WhileStatementAST>(expression(3), statement(5));
}   break;

#line 3680 "./glsl.g"

case 388: {
    ast(1) = makeAstNode<DoStatementAST>(statement(2), expression(5));
}   break;

#line 3687 "./glsl.g"

case 389: {
    ast(1) = makeAstNode<ForStatementAST>(statement(3), sym(4).forstmt.condition, sym(4).forstmt.increment, statement(6));
}   break;

#line 3694 "./glsl.g"

case 390: {
    // nothing to do.
}   break;

#line 3701 "./glsl.g"

case 391: {
    // nothing to do.
}   break;

#line 3708 "./glsl.g"

case 392: {
    // nothing to do.
}   break;

#line 3715 "./glsl.g"

case 393: {
    // nothing to do.
}   break;

#line 3722 "./glsl.g"

case 394: {
    sym(1).forstmt.condition = expression(1);
    sym(1).forstmt.increment = nullptr;
}   break;

#line 3730 "./glsl.g"

case 395: {
    sym(1).forstmt.condition = expression(1);
    sym(1).forstmt.increment = expression(3);
}   break;

#line 3738 "./glsl.g"

case 396: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Continue);
}   break;

#line 3745 "./glsl.g"

case 397: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Break);
}   break;

#line 3752 "./glsl.g"

case 398: {
    ast(1) = makeAstNode<ReturnStatementAST>();
}   break;

#line 3759 "./glsl.g"

case 399: {
    ast(1) = makeAstNode<ReturnStatementAST>(expression(2));
}   break;

#line 3766 "./glsl.g"

case 400: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Discard);
}   break;

#line 3773 "./glsl.g"

case 401: {
    ast(1) = makeAstNode<TranslationUnitAST>(sym(1).declaration_list);
}   break;

#line 3780 "./glsl.g"

case 402: {
    if (sym(1).declaration) {
        sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration);
    } else {
        sym(1).declaration_list = nullptr;
    }
}   break;

#line 3792 "./glsl.g"

case 403: {
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

#line 3809 "./glsl.g"

case 404: {
    // nothing to do.
}   break;

#line 3816 "./glsl.g"

case 405: {
    // nothing to do.
}   break;

#line 3823 "./glsl.g"

case 406: {
    ast(1) = nullptr;
}   break;

#line 3830 "./glsl.g"

case 407: {
    function(1)->body = statement(2);
}   break;

#line 3837 "./glsl.g"

case 408: {
    ast(1) = nullptr;
}   break;

#line 3845 "./glsl.g"

case 409: {
    ast(1) = ast(2);
}   break;

#line 3852 "./glsl.g"

case 410: {
    ast(1) = ast(2);
}   break;

#line 3858 "./glsl.g"

} // end switch
} // end Parser::reduce()
