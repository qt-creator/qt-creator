
#line 371 "./glsl.g"

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

#line 581 "./glsl.g"

void Parser::reduce(int ruleno)
{
switch(ruleno) {

#line 590 "./glsl.g"

case 0: {
    ast(1) = makeAstNode<IdentifierExpressionAST>(string(1));
}   break;

#line 597 "./glsl.g"

case 1: {
    ast(1) = makeAstNode<LiteralExpressionAST>(string(1));
}   break;

#line 604 "./glsl.g"

case 2: {
    ast(1) = makeAstNode<LiteralExpressionAST>(_engine->identifier("true", 4));
}   break;

#line 611 "./glsl.g"

case 3: {
    ast(1) = makeAstNode<LiteralExpressionAST>(_engine->identifier("false", 5));
}   break;

#line 618 "./glsl.g"

case 4: {
    // nothing to do.
}   break;

#line 625 "./glsl.g"

case 5: {
    ast(1) = ast(2);
}   break;

#line 632 "./glsl.g"

case 6: {
    // nothing to do.
}   break;

#line 639 "./glsl.g"

case 7: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ArrayAccess, expression(1), expression(3));
}   break;

#line 646 "./glsl.g"

case 8: {
    // nothing to do.
}   break;

#line 653 "./glsl.g"

case 9: {
    ast(1) = makeAstNode<MemberAccessExpressionAST>(expression(1), string(3));
}   break;

#line 660 "./glsl.g"

case 10: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PostIncrement, expression(1));
}   break;

#line 667 "./glsl.g"

case 11: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PostDecrement, expression(1));
}   break;

#line 674 "./glsl.g"

case 12: {
    // nothing to do.
}   break;

#line 681 "./glsl.g"

case 13: {
    // nothing to do.
}   break;

#line 688 "./glsl.g"

case 14: {
    ast(1) = makeAstNode<FunctionCallExpressionAST>
        (sym(1).function.id, sym(1).function.arguments);
}   break;

#line 696 "./glsl.g"

case 15: {
    ast(1) = makeAstNode<FunctionCallExpressionAST>
        (expression(1), sym(3).function.id, sym(3).function.arguments);
}   break;

#line 704 "./glsl.g"

case 16: {
    // nothing to do.
}   break;

#line 711 "./glsl.g"

case 17: {
    // nothing to do.
}   break;

#line 718 "./glsl.g"

case 18: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments = nullptr;
}   break;

#line 726 "./glsl.g"

case 19: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments = nullptr;
}   break;

#line 734 "./glsl.g"

case 20: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments =
        makeAstNode< List<ExpressionAST *> >(expression(2));
}   break;

#line 743 "./glsl.g"

case 21: {
    sym(1).function.arguments =
        makeAstNode< List<ExpressionAST *> >
            (sym(1).function.arguments, expression(3));
}   break;

#line 752 "./glsl.g"

case 22: {
    // nothing to do.
}   break;

#line 759 "./glsl.g"

case 23: {
    ast(1) = makeAstNode<FunctionIdentifierAST>(type(1));
}   break;

#line 766 "./glsl.g"

case 24: {
    ast(1) = makeAstNode<FunctionIdentifierAST>(string(1));
}   break;

#line 773 "./glsl.g"

case 25: {
    // nothing to do.
}   break;

#line 780 "./glsl.g"

case 26: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PreIncrement, expression(2));
}   break;

#line 787 "./glsl.g"

case 27: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PreDecrement, expression(2));
}   break;

#line 794 "./glsl.g"

case 28: {
    ast(1) = makeAstNode<UnaryExpressionAST>(sym(1).kind, expression(2));
}   break;

#line 801 "./glsl.g"

case 29: {
    sym(1).kind = AST::Kind_UnaryPlus;
}   break;

#line 808 "./glsl.g"

case 30: {
    sym(1).kind = AST::Kind_UnaryMinus;
}   break;

#line 815 "./glsl.g"

case 31: {
    sym(1).kind = AST::Kind_LogicalNot;
}   break;

#line 822 "./glsl.g"

case 32: {
    sym(1).kind = AST::Kind_BitwiseNot;
}   break;

#line 829 "./glsl.g"

case 33: {
    // nothing to do.
}   break;

#line 836 "./glsl.g"

case 34: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Multiply, expression(1), expression(3));
}   break;

#line 843 "./glsl.g"

case 35: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Divide, expression(1), expression(3));
}   break;

#line 850 "./glsl.g"

case 36: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Modulus, expression(1), expression(3));
}   break;

#line 857 "./glsl.g"

case 37: {
    // nothing to do.
}   break;

#line 864 "./glsl.g"

case 38: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Plus, expression(1), expression(3));
}   break;

#line 871 "./glsl.g"

case 39: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Minus, expression(1), expression(3));
}   break;

#line 878 "./glsl.g"

case 40: {
    // nothing to do.
}   break;

#line 885 "./glsl.g"

case 41: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ShiftLeft, expression(1), expression(3));
}   break;

#line 892 "./glsl.g"

case 42: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ShiftRight, expression(1), expression(3));
}   break;

#line 899 "./glsl.g"

case 43: {
    // nothing to do.
}   break;

#line 906 "./glsl.g"

case 44: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LessThan, expression(1), expression(3));
}   break;

#line 913 "./glsl.g"

case 45: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_GreaterThan, expression(1), expression(3));
}   break;

#line 920 "./glsl.g"

case 46: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LessEqual, expression(1), expression(3));
}   break;

#line 927 "./glsl.g"

case 47: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_GreaterEqual, expression(1), expression(3));
}   break;

#line 934 "./glsl.g"

case 48: {
    // nothing to do.
}   break;

#line 941 "./glsl.g"

case 49: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Equal, expression(1), expression(3));
}   break;

#line 948 "./glsl.g"

case 50: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_NotEqual, expression(1), expression(3));
}   break;

#line 955 "./glsl.g"

case 51: {
    // nothing to do.
}   break;

#line 962 "./glsl.g"

case 52: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseAnd, expression(1), expression(3));
}   break;

#line 969 "./glsl.g"

case 53: {
    // nothing to do.
}   break;

#line 976 "./glsl.g"

case 54: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseXor, expression(1), expression(3));
}   break;

#line 983 "./glsl.g"

case 55: {
    // nothing to do.
}   break;

#line 990 "./glsl.g"

case 56: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseOr, expression(1), expression(3));
}   break;

#line 997 "./glsl.g"

case 57: {
    // nothing to do.
}   break;

#line 1004 "./glsl.g"

case 58: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalAnd, expression(1), expression(3));
}   break;

#line 1011 "./glsl.g"

case 59: {
    // nothing to do.
}   break;

#line 1018 "./glsl.g"

case 60: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalXor, expression(1), expression(3));
}   break;

#line 1025 "./glsl.g"

case 61: {
    // nothing to do.
}   break;

#line 1032 "./glsl.g"

case 62: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalOr, expression(1), expression(3));
}   break;

#line 1039 "./glsl.g"

case 63: {
    // nothing to do.
}   break;

#line 1046 "./glsl.g"

case 64: {
    ast(1) = makeAstNode<TernaryExpressionAST>(AST::Kind_Conditional, expression(1), expression(3), expression(5));
}   break;

#line 1053 "./glsl.g"

case 65: {
    // nothing to do.
}   break;

#line 1060 "./glsl.g"

case 66: {
    ast(1) = makeAstNode<AssignmentExpressionAST>(sym(2).kind, expression(1), expression(3));
}   break;

#line 1067 "./glsl.g"

case 67: {
    sym(1).kind = AST::Kind_Assign;
}   break;

#line 1074 "./glsl.g"

case 68: {
    sym(1).kind = AST::Kind_AssignMultiply;
}   break;

#line 1081 "./glsl.g"

case 69: {
    sym(1).kind = AST::Kind_AssignDivide;
}   break;

#line 1088 "./glsl.g"

case 70: {
    sym(1).kind = AST::Kind_AssignModulus;
}   break;

#line 1095 "./glsl.g"

case 71: {
    sym(1).kind = AST::Kind_AssignPlus;
}   break;

#line 1102 "./glsl.g"

case 72: {
    sym(1).kind = AST::Kind_AssignMinus;
}   break;

#line 1109 "./glsl.g"

case 73: {
    sym(1).kind = AST::Kind_AssignShiftLeft;
}   break;

#line 1116 "./glsl.g"

case 74: {
    sym(1).kind = AST::Kind_AssignShiftRight;
}   break;

#line 1123 "./glsl.g"

case 75: {
    sym(1).kind = AST::Kind_AssignAnd;
}   break;

#line 1130 "./glsl.g"

case 76: {
    sym(1).kind = AST::Kind_AssignXor;
}   break;

#line 1137 "./glsl.g"

case 77: {
    sym(1).kind = AST::Kind_AssignOr;
}   break;

#line 1144 "./glsl.g"

case 78: {
    // nothing to do.
}   break;

#line 1151 "./glsl.g"

case 79: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Comma, expression(1), expression(3));
}   break;

#line 1158 "./glsl.g"

case 80: {
    // nothing to do.
}   break;

#line 1165 "./glsl.g"

case 81: {
    // nothing to do.
}   break;

#line 1172 "./glsl.g"

case 82: {
    ast(1) = makeAstNode<InitDeclarationAST>(sym(1).declaration_list);
}   break;

#line 1179 "./glsl.g"

case 83: {
    ast(1) = makeAstNode<PrecisionDeclarationAST>(sym(2).precision, type(3));
}   break;

#line 1186 "./glsl.g"

case 84: {
    if (sym(1).type_qualifier.qualifier != QualifiedTypeAST::Struct) {
        // TODO: issue an error if the qualifier is not "struct".
    }
    TypeAST *type = makeAstNode<StructTypeAST>(string(2), sym(4).field_list);
    ast(1) = makeAstNode<TypeDeclarationAST>(type);
}   break;

#line 1197 "./glsl.g"

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

#line 1216 "./glsl.g"

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

#line 1236 "./glsl.g"

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

#line 1256 "./glsl.g"

case 88: {
    TypeAST *type = makeAstNode<QualifiedTypeAST>
        (sym(1).type_qualifier.qualifier, (TypeAST *)nullptr,
         sym(1).type_qualifier.layout_list);
    ast(1) = makeAstNode<TypeDeclarationAST>(type);
}   break;

#line 1266 "./glsl.g"

case 89: {
    function(1)->finishParams();
}   break;

#line 1273 "./glsl.g"

case 90: {
    // nothing to do.
}   break;

#line 1280 "./glsl.g"

case 91: {
    // nothing to do.
}   break;

#line 1287 "./glsl.g"

case 92: {
    function(1)->params = makeAstNode< List<ParameterDeclarationAST *> >
        (sym(2).param_declaration);
}   break;

#line 1295 "./glsl.g"

case 93: {
    function(1)->params = makeAstNode< List<ParameterDeclarationAST *> >
        (function(1)->params, sym(3).param_declaration);
}   break;

#line 1303 "./glsl.g"

case 94: {
    function(1) = makeAstNode<FunctionDeclarationAST>(type(1), string(2));
}   break;

#line 1310 "./glsl.g"

case 95: {
    sym(1).param_declarator.type = type(1);
    sym(1).param_declarator.name = string(2);
}   break;

#line 1318 "./glsl.g"

case 96: {
    sym(1).param_declarator.type = makeAstNode<ArrayTypeAST>(type(1), expression(4));
    sym(1).param_declarator.name = string(2);
}   break;

#line 1326 "./glsl.g"

case 97: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (makeAstNode<QualifiedTypeAST>
            (sym(1).qualifier, sym(3).param_declarator.type,
             (List<LayoutQualifierAST *> *)nullptr),
         ParameterDeclarationAST::Qualifier(sym(2).qualifier),
         sym(3).param_declarator.name);
}   break;

#line 1338 "./glsl.g"

case 98: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (sym(2).param_declarator.type,
         ParameterDeclarationAST::Qualifier(sym(1).qualifier),
         sym(2).param_declarator.name);
}   break;

#line 1348 "./glsl.g"

case 99: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (makeAstNode<QualifiedTypeAST>
            (sym(1).qualifier, type(3), (List<LayoutQualifierAST *> *)nullptr),
         ParameterDeclarationAST::Qualifier(sym(2).qualifier),
         (const QString *)nullptr);
}   break;

#line 1359 "./glsl.g"

case 100: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (type(2), ParameterDeclarationAST::Qualifier(sym(1).qualifier),
         (const QString *)nullptr);
}   break;

#line 1368 "./glsl.g"

case 101: {
    sym(1).qualifier = ParameterDeclarationAST::In;
}   break;

#line 1375 "./glsl.g"

case 102: {
    sym(1).qualifier = ParameterDeclarationAST::In;
}   break;

#line 1382 "./glsl.g"

case 103: {
    sym(1).qualifier = ParameterDeclarationAST::Out;
}   break;

#line 1389 "./glsl.g"

case 104: {
    sym(1).qualifier = ParameterDeclarationAST::InOut;
}   break;

#line 1396 "./glsl.g"

case 105: {
    // nothing to do.
}   break;

#line 1403 "./glsl.g"

case 106: {
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
        (sym(1).declaration);
}   break;

#line 1411 "./glsl.g"

case 107: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1421 "./glsl.g"

case 108: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1432 "./glsl.g"

case 109: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type, expression(5));
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1443 "./glsl.g"

case 110: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>
            (type, string(3), expression(7));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1455 "./glsl.g"

case 111: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type, expression(5));
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>
            (type, string(3), expression(8));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1467 "./glsl.g"

case 112: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>
            (type, string(3), expression(5));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1478 "./glsl.g"

case 113: {
    ast(1) = makeAstNode<TypeDeclarationAST>(type(1));
}   break;

#line 1485 "./glsl.g"

case 114: {
    ast(1) = makeAstNode<VariableDeclarationAST>(type(1), string(2));
}   break;

#line 1492 "./glsl.g"

case 115: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1)), string(2));
}   break;

#line 1500 "./glsl.g"

case 116: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1), expression(4)), string(2));
}   break;

#line 1508 "./glsl.g"

case 117: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1)), string(2), expression(6));
}   break;

#line 1516 "./glsl.g"

case 118: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1), expression(4)),
         string(2), expression(7));
}   break;

#line 1525 "./glsl.g"

case 119: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (type(1), string(2), expression(4));
}   break;

#line 1533 "./glsl.g"

case 120: {
    ast(1) = makeAstNode<InvariantDeclarationAST>(string(2));
}   break;

#line 1540 "./glsl.g"

case 121: {
    ast(1) = makeAstNode<QualifiedTypeAST>(0, type(1), (List<LayoutQualifierAST *> *)nullptr);
}   break;

#line 1547 "./glsl.g"

case 122: {
    ast(1) = makeAstNode<QualifiedTypeAST>
        (sym(1).type_qualifier.qualifier, type(2),
         sym(1).type_qualifier.layout_list);
}   break;

#line 1556 "./glsl.g"

case 123: {
    sym(1).qualifier = QualifiedTypeAST::Invariant;
}   break;

#line 1563 "./glsl.g"

case 124: {
    sym(1).qualifier = QualifiedTypeAST::Smooth;
}   break;

#line 1570 "./glsl.g"

case 125: {
    sym(1).qualifier = QualifiedTypeAST::Flat;
}   break;

#line 1577 "./glsl.g"

case 126: {
    sym(1).qualifier = QualifiedTypeAST::NoPerspective;
}   break;

#line 1584 "./glsl.g"

case 127: {
    sym(1) = sym(3);
}   break;

#line 1591 "./glsl.g"

case 128: {
    sym(1).layout_list = makeAstNode< List<LayoutQualifierAST *> >(sym(1).layout);
}   break;

#line 1598 "./glsl.g"

case 129: {
    sym(1).layout_list = makeAstNode< List<LayoutQualifierAST *> >(sym(1).layout_list, sym(3).layout);
}   break;

#line 1605 "./glsl.g"

case 130: {
    sym(1).layout = makeAstNode<LayoutQualifierAST>(string(1), (const QString *)nullptr);
}   break;

#line 1612 "./glsl.g"

case 131: {
    sym(1).layout = makeAstNode<LayoutQualifierAST>(string(1), string(3));
}   break;

#line 1619 "./glsl.g"

case 132: {
    sym(1).qualifier = QualifiedTypeAST::Const;
}   break;

#line 1626 "./glsl.g"

case 133: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier;
    sym(1).type_qualifier.layout_list = nullptr;
}   break;

#line 1634 "./glsl.g"

case 134: {
    sym(1).type_qualifier.layout_list = sym(1).layout_list;
    sym(1).type_qualifier.qualifier = 0;
}   break;

#line 1642 "./glsl.g"

case 135: {
    sym(1).type_qualifier.layout_list = sym(1).layout_list;
    sym(1).type_qualifier.qualifier = sym(2).qualifier;
}   break;

#line 1650 "./glsl.g"

case 136: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier;
    sym(1).type_qualifier.layout_list = nullptr;
}   break;

#line 1658 "./glsl.g"

case 137: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier;
    sym(1).type_qualifier.layout_list = nullptr;
}   break;

#line 1666 "./glsl.g"

case 138: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier;
    sym(1).type_qualifier.layout_list = nullptr;
}   break;

#line 1674 "./glsl.g"

case 139: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier | sym(3).qualifier;
    sym(1).type_qualifier.layout_list = nullptr;
}   break;

#line 1682 "./glsl.g"

case 140: {
    sym(1).type_qualifier.qualifier = QualifiedTypeAST::Invariant;
    sym(1).type_qualifier.layout_list = nullptr;
}   break;

#line 1690 "./glsl.g"

case 141: {
    sym(1).qualifier = QualifiedTypeAST::Const;
}   break;

#line 1697 "./glsl.g"

case 142: {
    sym(1).qualifier = QualifiedTypeAST::Attribute;
}   break;

#line 1704 "./glsl.g"

case 143: {
    sym(1).qualifier = QualifiedTypeAST::Varying;
}   break;

#line 1711 "./glsl.g"

case 144: {
    sym(1).qualifier = QualifiedTypeAST::CentroidVarying;
}   break;

#line 1718 "./glsl.g"

case 145: {
    sym(1).qualifier = QualifiedTypeAST::In;
}   break;

#line 1725 "./glsl.g"

case 146: {
    sym(1).qualifier = QualifiedTypeAST::Out;
}   break;

#line 1732 "./glsl.g"

case 147: {
    sym(1).qualifier = QualifiedTypeAST::CentroidIn;
}   break;

#line 1739 "./glsl.g"

case 148: {
    sym(1).qualifier = QualifiedTypeAST::CentroidOut;
}   break;

#line 1746 "./glsl.g"

case 149: {
    sym(1).qualifier = QualifiedTypeAST::PatchIn;
}   break;

#line 1753 "./glsl.g"

case 150: {
    sym(1).qualifier = QualifiedTypeAST::PatchOut;
}   break;

#line 1760 "./glsl.g"

case 151: {
    sym(1).qualifier = QualifiedTypeAST::SampleIn;
}   break;

#line 1767 "./glsl.g"

case 152: {
    sym(1).qualifier = QualifiedTypeAST::SampleOut;
}   break;

#line 1774 "./glsl.g"

case 153: {
    sym(1).qualifier = QualifiedTypeAST::Uniform;
}   break;

#line 1781 "./glsl.g"

case 154: {
    // nothing to do.
}   break;

#line 1788 "./glsl.g"

case 155: {
    if (!type(2)->setPrecision(sym(1).precision)) {
        // TODO: issue an error about precision not allowed on this type.
    }
    ast(1) = type(2);
}   break;

#line 1798 "./glsl.g"

case 156: {
    // nothing to do.
}   break;

#line 1805 "./glsl.g"

case 157: {
    ast(1) = makeAstNode<ArrayTypeAST>(type(1));
}   break;

#line 1812 "./glsl.g"

case 158: {
    ast(1) = makeAstNode<ArrayTypeAST>(type(1), expression(3));
}   break;

#line 1819 "./glsl.g"

case 159: {
    ast(1) = makeBasicType(T_VOID);
}   break;

#line 1826 "./glsl.g"

case 160: {
    ast(1) = makeBasicType(T_FLOAT);
}   break;

#line 1833 "./glsl.g"

case 161: {
    ast(1) = makeBasicType(T_DOUBLE);
}   break;

#line 1840 "./glsl.g"

case 162: {
    ast(1) = makeBasicType(T_INT);
}   break;

#line 1847 "./glsl.g"

case 163: {
    ast(1) = makeBasicType(T_UINT);
}   break;

#line 1854 "./glsl.g"

case 164: {
    ast(1) = makeBasicType(T_BOOL);
}   break;

#line 1861 "./glsl.g"

case 165: {
    ast(1) = makeBasicType(T_VEC2);
}   break;

#line 1868 "./glsl.g"

case 166: {
    ast(1) = makeBasicType(T_VEC3);
}   break;

#line 1875 "./glsl.g"

case 167: {
    ast(1) = makeBasicType(T_VEC4);
}   break;

#line 1882 "./glsl.g"

case 168: {
    ast(1) = makeBasicType(T_DVEC2);
}   break;

#line 1889 "./glsl.g"

case 169: {
    ast(1) = makeBasicType(T_DVEC3);
}   break;

#line 1896 "./glsl.g"

case 170: {
    ast(1) = makeBasicType(T_DVEC4);
}   break;

#line 1903 "./glsl.g"

case 171: {
    ast(1) = makeBasicType(T_BVEC2);
}   break;

#line 1910 "./glsl.g"

case 172: {
    ast(1) = makeBasicType(T_BVEC3);
}   break;

#line 1917 "./glsl.g"

case 173: {
    ast(1) = makeBasicType(T_BVEC4);
}   break;

#line 1924 "./glsl.g"

case 174: {
    ast(1) = makeBasicType(T_IVEC2);
}   break;

#line 1931 "./glsl.g"

case 175: {
    ast(1) = makeBasicType(T_IVEC3);
}   break;

#line 1938 "./glsl.g"

case 176: {
    ast(1) = makeBasicType(T_IVEC4);
}   break;

#line 1945 "./glsl.g"

case 177: {
    ast(1) = makeBasicType(T_UVEC2);
}   break;

#line 1952 "./glsl.g"

case 178: {
    ast(1) = makeBasicType(T_UVEC3);
}   break;

#line 1959 "./glsl.g"

case 179: {
    ast(1) = makeBasicType(T_UVEC4);
}   break;

#line 1966 "./glsl.g"

case 180: {
    ast(1) = makeBasicType(T_MAT2);
}   break;

#line 1973 "./glsl.g"

case 181: {
    ast(1) = makeBasicType(T_MAT3);
}   break;

#line 1980 "./glsl.g"

case 182: {
    ast(1) = makeBasicType(T_MAT4);
}   break;

#line 1987 "./glsl.g"

case 183: {
    ast(1) = makeBasicType(T_MAT2);
}   break;

#line 1994 "./glsl.g"

case 184: {
    ast(1) = makeBasicType(T_MAT2X3);
}   break;

#line 2001 "./glsl.g"

case 185: {
    ast(1) = makeBasicType(T_MAT2X4);
}   break;

#line 2008 "./glsl.g"

case 186: {
    ast(1) = makeBasicType(T_MAT3X2);
}   break;

#line 2015 "./glsl.g"

case 187: {
    ast(1) = makeBasicType(T_MAT3);
}   break;

#line 2022 "./glsl.g"

case 188: {
    ast(1) = makeBasicType(T_MAT3X4);
}   break;

#line 2029 "./glsl.g"

case 189: {
    ast(1) = makeBasicType(T_MAT4X2);
}   break;

#line 2036 "./glsl.g"

case 190: {
    ast(1) = makeBasicType(T_MAT4X3);
}   break;

#line 2043 "./glsl.g"

case 191: {
    ast(1) = makeBasicType(T_MAT4);
}   break;

#line 2050 "./glsl.g"

case 192: {
    ast(1) = makeBasicType(T_DMAT2);
}   break;

#line 2057 "./glsl.g"

case 193: {
    ast(1) = makeBasicType(T_DMAT3);
}   break;

#line 2064 "./glsl.g"

case 194: {
    ast(1) = makeBasicType(T_DMAT4);
}   break;

#line 2071 "./glsl.g"

case 195: {
    ast(1) = makeBasicType(T_DMAT2);
}   break;

#line 2078 "./glsl.g"

case 196: {
    ast(1) = makeBasicType(T_DMAT2X3);
}   break;

#line 2085 "./glsl.g"

case 197: {
    ast(1) = makeBasicType(T_DMAT2X4);
}   break;

#line 2092 "./glsl.g"

case 198: {
    ast(1) = makeBasicType(T_DMAT3X2);
}   break;

#line 2099 "./glsl.g"

case 199: {
    ast(1) = makeBasicType(T_DMAT3);
}   break;

#line 2106 "./glsl.g"

case 200: {
    ast(1) = makeBasicType(T_DMAT3X4);
}   break;

#line 2113 "./glsl.g"

case 201: {
    ast(1) = makeBasicType(T_DMAT4X2);
}   break;

#line 2120 "./glsl.g"

case 202: {
    ast(1) = makeBasicType(T_DMAT4X3);
}   break;

#line 2127 "./glsl.g"

case 203: {
    ast(1) = makeBasicType(T_DMAT4);
}   break;

#line 2134 "./glsl.g"

case 204: {
    ast(1) = makeBasicType(T_SAMPLER1D);
}   break;

#line 2141 "./glsl.g"

case 205: {
    ast(1) = makeBasicType(T_SAMPLER2D);
}   break;

#line 2148 "./glsl.g"

case 206: {
    ast(1) = makeBasicType(T_SAMPLER3D);
}   break;

#line 2155 "./glsl.g"

case 207: {
    ast(1) = makeBasicType(T_SAMPLERCUBE);
}   break;

#line 2162 "./glsl.g"

case 208: {
    ast(1) = makeBasicType(T_SAMPLER1DSHADOW);
}   break;

#line 2169 "./glsl.g"

case 209: {
    ast(1) = makeBasicType(T_SAMPLER2DSHADOW);
}   break;

#line 2176 "./glsl.g"

case 210: {
    ast(1) = makeBasicType(T_SAMPLERCUBESHADOW);
}   break;

#line 2183 "./glsl.g"

case 211: {
    ast(1) = makeBasicType(T_SAMPLER1DARRAY);
}   break;

#line 2190 "./glsl.g"

case 212: {
    ast(1) = makeBasicType(T_SAMPLER2DARRAY);
}   break;

#line 2197 "./glsl.g"

case 213: {
    ast(1) = makeBasicType(T_SAMPLER1DARRAYSHADOW);
}   break;

#line 2204 "./glsl.g"

case 214: {
    ast(1) = makeBasicType(T_SAMPLER2DARRAYSHADOW);
}   break;

#line 2211 "./glsl.g"

case 215: {
    ast(1) = makeBasicType(T_SAMPLERCUBEARRAY);
}   break;

#line 2218 "./glsl.g"

case 216: {
    ast(1) = makeBasicType(T_SAMPLERCUBEARRAYSHADOW);
}   break;

#line 2225 "./glsl.g"

case 217: {
    ast(1) = makeBasicType(T_ISAMPLER1D);
}   break;

#line 2232 "./glsl.g"

case 218: {
    ast(1) = makeBasicType(T_ISAMPLER2D);
}   break;

#line 2239 "./glsl.g"

case 219: {
    ast(1) = makeBasicType(T_ISAMPLER3D);
}   break;

#line 2246 "./glsl.g"

case 220: {
    ast(1) = makeBasicType(T_ISAMPLERCUBE);
}   break;

#line 2253 "./glsl.g"

case 221: {
    ast(1) = makeBasicType(T_ISAMPLER1DARRAY);
}   break;

#line 2260 "./glsl.g"

case 222: {
    ast(1) = makeBasicType(T_ISAMPLER2DARRAY);
}   break;

#line 2267 "./glsl.g"

case 223: {
    ast(1) = makeBasicType(T_ISAMPLERCUBEARRAY);
}   break;

#line 2274 "./glsl.g"

case 224: {
    ast(1) = makeBasicType(T_USAMPLER1D);
}   break;

#line 2281 "./glsl.g"

case 225: {
    ast(1) = makeBasicType(T_USAMPLER2D);
}   break;

#line 2288 "./glsl.g"

case 226: {
    ast(1) = makeBasicType(T_USAMPLER3D);
}   break;

#line 2295 "./glsl.g"

case 227: {
    ast(1) = makeBasicType(T_USAMPLERCUBE);
}   break;

#line 2302 "./glsl.g"

case 228: {
    ast(1) = makeBasicType(T_USAMPLER1DARRAY);
}   break;

#line 2309 "./glsl.g"

case 229: {
    ast(1) = makeBasicType(T_USAMPLER2DARRAY);
}   break;

#line 2316 "./glsl.g"

case 230: {
    ast(1) = makeBasicType(T_USAMPLERCUBEARRAY);
}   break;

#line 2323 "./glsl.g"

case 231: {
    ast(1) = makeBasicType(T_SAMPLER2DRECT);
}   break;

#line 2330 "./glsl.g"

case 232: {
    ast(1) = makeBasicType(T_SAMPLER2DRECTSHADOW);
}   break;

#line 2337 "./glsl.g"

case 233: {
    ast(1) = makeBasicType(T_ISAMPLER2DRECT);
}   break;

#line 2344 "./glsl.g"

case 234: {
    ast(1) = makeBasicType(T_USAMPLER2DRECT);
}   break;

#line 2351 "./glsl.g"

case 235: {
    ast(1) = makeBasicType(T_SAMPLERBUFFER);
}   break;

#line 2358 "./glsl.g"

case 236: {
    ast(1) = makeBasicType(T_ISAMPLERBUFFER);
}   break;

#line 2365 "./glsl.g"

case 237: {
    ast(1) = makeBasicType(T_USAMPLERBUFFER);
}   break;

#line 2372 "./glsl.g"

case 238: {
    ast(1) = makeBasicType(T_SAMPLER2DMS);
}   break;

#line 2379 "./glsl.g"

case 239: {
    ast(1) = makeBasicType(T_ISAMPLER2DMS);
}   break;

#line 2386 "./glsl.g"

case 240: {
    ast(1) = makeBasicType(T_USAMPLER2DMS);
}   break;

#line 2393 "./glsl.g"

case 241: {
    ast(1) = makeBasicType(T_SAMPLER2DMSARRAY);
}   break;

#line 2400 "./glsl.g"

case 242: {
    ast(1) = makeBasicType(T_ISAMPLER2DMSARRAY);
}   break;

#line 2407 "./glsl.g"

case 243: {
    ast(1) = makeBasicType(T_USAMPLER2DMSARRAY);
}   break;

#line 2414 "./glsl.g"

case 244: {
    // nothing to do.
}   break;

#line 2421 "./glsl.g"

case 245: {
    ast(1) = makeAstNode<NamedTypeAST>(string(1));
}   break;

#line 2428 "./glsl.g"

case 246: {
    sym(1).precision = TypeAST::Highp;
}   break;

#line 2435 "./glsl.g"

case 247: {
    sym(1).precision = TypeAST::Mediump;
}   break;

#line 2442 "./glsl.g"

case 248: {
    sym(1).precision = TypeAST::Lowp;
}   break;

#line 2449 "./glsl.g"

case 249: {
    ast(1) = makeAstNode<StructTypeAST>(string(2), sym(4).field_list);
}   break;

#line 2456 "./glsl.g"

case 250: {
    ast(1) = makeAstNode<StructTypeAST>(sym(3).field_list);
}   break;

#line 2463 "./glsl.g"

case 251: {
    // nothing to do.
}   break;

#line 2470 "./glsl.g"

case 252: {
    sym(1).field_list = appendLists(sym(1).field_list, sym(2).field_list);
}   break;

#line 2477 "./glsl.g"

case 253: {
    sym(1).field_list = StructTypeAST::fixInnerTypes(type(1), sym(2).field_list);
}   break;

#line 2484 "./glsl.g"

case 254: {
    sym(1).field_list = StructTypeAST::fixInnerTypes
        (makeAstNode<QualifiedTypeAST>
            (sym(1).type_qualifier.qualifier, type(2),
             sym(1).type_qualifier.layout_list), sym(3).field_list);
}   break;

#line 2494 "./glsl.g"

case 255: {
    // nothing to do.
    sym(1).field_list = makeAstNode< List<StructTypeAST::Field *> >(sym(1).field);
}   break;

#line 2502 "./glsl.g"

case 256: {
    sym(1).field_list = makeAstNode< List<StructTypeAST::Field *> >(sym(1).field_list, sym(3).field);
}   break;

#line 2509 "./glsl.g"

case 257: {
    sym(1).field = makeAstNode<StructTypeAST::Field>(string(1));
}   break;

#line 2516 "./glsl.g"

case 258: {
    sym(1).field = makeAstNode<StructTypeAST::Field>
        (string(1), makeAstNode<ArrayTypeAST>((TypeAST *)nullptr));
}   break;

#line 2524 "./glsl.g"

case 259: {
    sym(1).field = makeAstNode<StructTypeAST::Field>
        (string(1), makeAstNode<ArrayTypeAST>((TypeAST *)nullptr, expression(3)));
}   break;

#line 2532 "./glsl.g"

case 260: {
    // nothing to do.
}   break;

#line 2539 "./glsl.g"

case 261: {
    ast(1) = makeAstNode<DeclarationStatementAST>(sym(1).declaration);
}   break;

#line 2546 "./glsl.g"

case 262: {
    // nothing to do.
}   break;

#line 2553 "./glsl.g"

case 263: {
    // nothing to do.
}   break;

#line 2560 "./glsl.g"

case 264: {
    // nothing to do.
}   break;

#line 2567 "./glsl.g"

case 265: {
    // nothing to do.
}   break;

#line 2574 "./glsl.g"

case 266: {
    // nothing to do.
}   break;

#line 2581 "./glsl.g"

case 267: {
    // nothing to do.
}   break;

#line 2588 "./glsl.g"

case 268: {
    // nothing to do.
}   break;

#line 2595 "./glsl.g"

case 269: {
    // nothing to do.
}   break;

#line 2602 "./glsl.g"

case 270: {
    // nothing to do.
}   break;

#line 2609 "./glsl.g"

case 271: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>();
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(2)).end();
    ast(1) = stmt;
}   break;

#line 2619 "./glsl.g"

case 272: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>(sym(2).statement_list);
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(3)).end();
    ast(1) = stmt;
}   break;

#line 2629 "./glsl.g"

case 273: {
    // nothing to do.
}   break;

#line 2636 "./glsl.g"

case 274: {
    // nothing to do.
}   break;

#line 2643 "./glsl.g"

case 275: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>();
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(2)).end();
    ast(1) = stmt;
}   break;

#line 2653 "./glsl.g"

case 276: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>(sym(2).statement_list);
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(3)).end();
    ast(1) = stmt;
}   break;

#line 2663 "./glsl.g"

case 277: {
    sym(1).statement_list = makeAstNode< List<StatementAST *> >(sym(1).statement);
}   break;

#line 2670 "./glsl.g"

case 278: {
    sym(1).statement_list = makeAstNode< List<StatementAST *> >(sym(1).statement_list, sym(2).statement);
}   break;

#line 2677 "./glsl.g"

case 279: {
    ast(1) = makeAstNode<CompoundStatementAST>();  // Empty statement
}   break;

#line 2684 "./glsl.g"

case 280: {
    ast(1) = makeAstNode<ExpressionStatementAST>(expression(1));
}   break;

#line 2691 "./glsl.g"

case 281: {
    ast(1) = makeAstNode<IfStatementAST>(expression(3), sym(5).ifstmt.thenClause, sym(5).ifstmt.elseClause);
}   break;

#line 2698 "./glsl.g"

case 282: {
    sym(1).ifstmt.thenClause = statement(1);
    sym(1).ifstmt.elseClause = statement(3);
}   break;

#line 2706 "./glsl.g"

case 283: {
    sym(1).ifstmt.thenClause = statement(1);
    sym(1).ifstmt.elseClause = nullptr;
}   break;

#line 2714 "./glsl.g"

case 284: {
    // nothing to do.
}   break;

#line 2721 "./glsl.g"

case 285: {
    ast(1) = makeAstNode<DeclarationExpressionAST>
        (type(1), string(2), expression(4));
}   break;

#line 2729 "./glsl.g"

case 286: {
    ast(1) = makeAstNode<SwitchStatementAST>(expression(3), statement(6));
}   break;

#line 2736 "./glsl.g"

case 287: {
    ast(1) = makeAstNode<CompoundStatementAST>();
}   break;

#line 2743 "./glsl.g"

case 288: {
    ast(1) = makeAstNode<CompoundStatementAST>(sym(1).statement_list);
}   break;

#line 2750 "./glsl.g"

case 289: {
    ast(1) = makeAstNode<CaseLabelStatementAST>(expression(2));
}   break;

#line 2757 "./glsl.g"

case 290: {
    ast(1) = makeAstNode<CaseLabelStatementAST>();
}   break;

#line 2764 "./glsl.g"

case 291: {
    ast(1) = makeAstNode<WhileStatementAST>(expression(3), statement(5));
}   break;

#line 2771 "./glsl.g"

case 292: {
    ast(1) = makeAstNode<DoStatementAST>(statement(2), expression(5));
}   break;

#line 2778 "./glsl.g"

case 293: {
    ast(1) = makeAstNode<ForStatementAST>(statement(3), sym(4).forstmt.condition, sym(4).forstmt.increment, statement(6));
}   break;

#line 2785 "./glsl.g"

case 294: {
    // nothing to do.
}   break;

#line 2792 "./glsl.g"

case 295: {
    // nothing to do.
}   break;

#line 2799 "./glsl.g"

case 296: {
    // nothing to do.
}   break;

#line 2806 "./glsl.g"

case 297: {
    // nothing to do.
}   break;

#line 2813 "./glsl.g"

case 298: {
    sym(1).forstmt.condition = expression(1);
    sym(1).forstmt.increment = nullptr;
}   break;

#line 2821 "./glsl.g"

case 299: {
    sym(1).forstmt.condition = expression(1);
    sym(1).forstmt.increment = expression(3);
}   break;

#line 2829 "./glsl.g"

case 300: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Continue);
}   break;

#line 2836 "./glsl.g"

case 301: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Break);
}   break;

#line 2843 "./glsl.g"

case 302: {
    ast(1) = makeAstNode<ReturnStatementAST>();
}   break;

#line 2850 "./glsl.g"

case 303: {
    ast(1) = makeAstNode<ReturnStatementAST>(expression(2));
}   break;

#line 2857 "./glsl.g"

case 304: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Discard);
}   break;

#line 2864 "./glsl.g"

case 305: {
    ast(1) = makeAstNode<TranslationUnitAST>(sym(1).declaration_list);
}   break;

#line 2871 "./glsl.g"

case 306: {
    if (sym(1).declaration) {
        sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration);
    } else {
        sym(1).declaration_list = nullptr;
    }
}   break;

#line 2883 "./glsl.g"

case 307: {
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

#line 2900 "./glsl.g"

case 308: {
    // nothing to do.
}   break;

#line 2907 "./glsl.g"

case 309: {
    // nothing to do.
}   break;

#line 2914 "./glsl.g"

case 310: {
    ast(1) = nullptr;
}   break;

#line 2921 "./glsl.g"

case 311: {
    function(1)->body = statement(2);
}   break;

#line 2928 "./glsl.g"

case 312: {
    ast(1) = nullptr;
}   break;

#line 2936 "./glsl.g"

case 313: {
    ast(1) = ast(2);
}   break;

#line 2943 "./glsl.g"

case 314: {
    ast(1) = ast(2);
}   break;

#line 2949 "./glsl.g"

} // end switch
} // end Parser::reduce()
