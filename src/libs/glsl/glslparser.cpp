
#line 405 "./glsl.g"

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

#line 615 "./glsl.g"

void Parser::reduce(int ruleno)
{
switch(ruleno) {

#line 624 "./glsl.g"

case 0: {
    ast(1) = makeAstNode<IdentifierExpressionAST>(string(1));
}   break;

#line 631 "./glsl.g"

case 1: {
    ast(1) = makeAstNode<LiteralExpressionAST>(string(1));
}   break;

#line 638 "./glsl.g"

case 2: {
    ast(1) = makeAstNode<LiteralExpressionAST>(_engine->identifier("true", 4));
}   break;

#line 645 "./glsl.g"

case 3: {
    ast(1) = makeAstNode<LiteralExpressionAST>(_engine->identifier("false", 5));
}   break;

#line 652 "./glsl.g"

case 4: {
    // nothing to do.
}   break;

#line 659 "./glsl.g"

case 5: {
    ast(1) = ast(2);
}   break;

#line 666 "./glsl.g"

case 6: {
    // nothing to do.
}   break;

#line 673 "./glsl.g"

case 7: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ArrayAccess, expression(1), expression(3));
}   break;

#line 680 "./glsl.g"

case 8: {
    // nothing to do.
}   break;

#line 687 "./glsl.g"

case 9: {
    ast(1) = makeAstNode<MemberAccessExpressionAST>(expression(1), string(3));
}   break;

#line 694 "./glsl.g"

case 10: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PostIncrement, expression(1));
}   break;

#line 701 "./glsl.g"

case 11: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PostDecrement, expression(1));
}   break;

#line 708 "./glsl.g"

case 12: {
    // nothing to do.
}   break;

#line 715 "./glsl.g"

case 13: {
    // nothing to do.
}   break;

#line 722 "./glsl.g"

case 14: {
    ast(1) = makeAstNode<FunctionCallExpressionAST>
        (sym(1).function.id, sym(1).function.arguments);
}   break;

#line 730 "./glsl.g"

case 15: {
    ast(1) = makeAstNode<FunctionCallExpressionAST>
        (expression(1), sym(3).function.id, sym(3).function.arguments);
}   break;

#line 738 "./glsl.g"

case 16: {
    // nothing to do.
}   break;

#line 745 "./glsl.g"

case 17: {
    // nothing to do.
}   break;

#line 752 "./glsl.g"

case 18: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments = nullptr;
}   break;

#line 760 "./glsl.g"

case 19: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments = nullptr;
}   break;

#line 768 "./glsl.g"

case 20: {
    sym(1).function.id = sym(1).function_identifier;
    sym(1).function.arguments =
        makeAstNode< List<ExpressionAST *> >(expression(2));
}   break;

#line 777 "./glsl.g"

case 21: {
    sym(1).function.arguments =
        makeAstNode< List<ExpressionAST *> >
            (sym(1).function.arguments, expression(3));
}   break;

#line 786 "./glsl.g"

case 22: {
    // nothing to do.
}   break;

#line 793 "./glsl.g"

case 23: {
    ast(1) = makeAstNode<FunctionIdentifierAST>(type(1));
}   break;

#line 800 "./glsl.g"

case 24: {
    ast(1) = makeAstNode<FunctionIdentifierAST>(string(1));
}   break;

#line 807 "./glsl.g"

case 25: {
    // nothing to do.
}   break;

#line 814 "./glsl.g"

case 26: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PreIncrement, expression(2));
}   break;

#line 821 "./glsl.g"

case 27: {
    ast(1) = makeAstNode<UnaryExpressionAST>(AST::Kind_PreDecrement, expression(2));
}   break;

#line 828 "./glsl.g"

case 28: {
    ast(1) = makeAstNode<UnaryExpressionAST>(sym(1).kind, expression(2));
}   break;

#line 835 "./glsl.g"

case 29: {
    sym(1).kind = AST::Kind_UnaryPlus;
}   break;

#line 842 "./glsl.g"

case 30: {
    sym(1).kind = AST::Kind_UnaryMinus;
}   break;

#line 849 "./glsl.g"

case 31: {
    sym(1).kind = AST::Kind_LogicalNot;
}   break;

#line 856 "./glsl.g"

case 32: {
    sym(1).kind = AST::Kind_BitwiseNot;
}   break;

#line 863 "./glsl.g"

case 33: {
    // nothing to do.
}   break;

#line 870 "./glsl.g"

case 34: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Multiply, expression(1), expression(3));
}   break;

#line 877 "./glsl.g"

case 35: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Divide, expression(1), expression(3));
}   break;

#line 884 "./glsl.g"

case 36: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Modulus, expression(1), expression(3));
}   break;

#line 891 "./glsl.g"

case 37: {
    // nothing to do.
}   break;

#line 898 "./glsl.g"

case 38: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Plus, expression(1), expression(3));
}   break;

#line 905 "./glsl.g"

case 39: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Minus, expression(1), expression(3));
}   break;

#line 912 "./glsl.g"

case 40: {
    // nothing to do.
}   break;

#line 919 "./glsl.g"

case 41: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ShiftLeft, expression(1), expression(3));
}   break;

#line 926 "./glsl.g"

case 42: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_ShiftRight, expression(1), expression(3));
}   break;

#line 933 "./glsl.g"

case 43: {
    // nothing to do.
}   break;

#line 940 "./glsl.g"

case 44: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LessThan, expression(1), expression(3));
}   break;

#line 947 "./glsl.g"

case 45: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_GreaterThan, expression(1), expression(3));
}   break;

#line 954 "./glsl.g"

case 46: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LessEqual, expression(1), expression(3));
}   break;

#line 961 "./glsl.g"

case 47: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_GreaterEqual, expression(1), expression(3));
}   break;

#line 968 "./glsl.g"

case 48: {
    // nothing to do.
}   break;

#line 975 "./glsl.g"

case 49: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Equal, expression(1), expression(3));
}   break;

#line 982 "./glsl.g"

case 50: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_NotEqual, expression(1), expression(3));
}   break;

#line 989 "./glsl.g"

case 51: {
    // nothing to do.
}   break;

#line 996 "./glsl.g"

case 52: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseAnd, expression(1), expression(3));
}   break;

#line 1003 "./glsl.g"

case 53: {
    // nothing to do.
}   break;

#line 1010 "./glsl.g"

case 54: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseXor, expression(1), expression(3));
}   break;

#line 1017 "./glsl.g"

case 55: {
    // nothing to do.
}   break;

#line 1024 "./glsl.g"

case 56: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_BitwiseOr, expression(1), expression(3));
}   break;

#line 1031 "./glsl.g"

case 57: {
    // nothing to do.
}   break;

#line 1038 "./glsl.g"

case 58: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalAnd, expression(1), expression(3));
}   break;

#line 1045 "./glsl.g"

case 59: {
    // nothing to do.
}   break;

#line 1052 "./glsl.g"

case 60: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalXor, expression(1), expression(3));
}   break;

#line 1059 "./glsl.g"

case 61: {
    // nothing to do.
}   break;

#line 1066 "./glsl.g"

case 62: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_LogicalOr, expression(1), expression(3));
}   break;

#line 1073 "./glsl.g"

case 63: {
    // nothing to do.
}   break;

#line 1080 "./glsl.g"

case 64: {
    ast(1) = makeAstNode<TernaryExpressionAST>(AST::Kind_Conditional, expression(1), expression(3), expression(5));
}   break;

#line 1087 "./glsl.g"

case 65: {
    // nothing to do.
}   break;

#line 1094 "./glsl.g"

case 66: {
    ast(1) = makeAstNode<AssignmentExpressionAST>(sym(2).kind, expression(1), expression(3));
}   break;

#line 1101 "./glsl.g"

case 67: {
    sym(1).kind = AST::Kind_Assign;
}   break;

#line 1108 "./glsl.g"

case 68: {
    sym(1).kind = AST::Kind_AssignMultiply;
}   break;

#line 1115 "./glsl.g"

case 69: {
    sym(1).kind = AST::Kind_AssignDivide;
}   break;

#line 1122 "./glsl.g"

case 70: {
    sym(1).kind = AST::Kind_AssignModulus;
}   break;

#line 1129 "./glsl.g"

case 71: {
    sym(1).kind = AST::Kind_AssignPlus;
}   break;

#line 1136 "./glsl.g"

case 72: {
    sym(1).kind = AST::Kind_AssignMinus;
}   break;

#line 1143 "./glsl.g"

case 73: {
    sym(1).kind = AST::Kind_AssignShiftLeft;
}   break;

#line 1150 "./glsl.g"

case 74: {
    sym(1).kind = AST::Kind_AssignShiftRight;
}   break;

#line 1157 "./glsl.g"

case 75: {
    sym(1).kind = AST::Kind_AssignAnd;
}   break;

#line 1164 "./glsl.g"

case 76: {
    sym(1).kind = AST::Kind_AssignXor;
}   break;

#line 1171 "./glsl.g"

case 77: {
    sym(1).kind = AST::Kind_AssignOr;
}   break;

#line 1178 "./glsl.g"

case 78: {
    // nothing to do.
}   break;

#line 1185 "./glsl.g"

case 79: {
    ast(1) = makeAstNode<BinaryExpressionAST>(AST::Kind_Comma, expression(1), expression(3));
}   break;

#line 1192 "./glsl.g"

case 80: {
    // nothing to do.
}   break;

#line 1199 "./glsl.g"

case 81: {
    // nothing to do.
}   break;

#line 1206 "./glsl.g"

case 82: {
    ast(1) = makeAstNode<InitDeclarationAST>(sym(1).declaration_list);
}   break;

#line 1213 "./glsl.g"

case 83: {
    ast(1) = makeAstNode<PrecisionDeclarationAST>(sym(2).precision, type(3));
}   break;

#line 1220 "./glsl.g"

case 84: {
    if (sym(1).type_qualifier.qualifier != QualifiedTypeAST::Struct) {
        // TODO: issue an error if the qualifier is not "struct".
    }
    TypeAST *type = makeAstNode<StructTypeAST>(string(2), sym(4).field_list);
    ast(1) = makeAstNode<TypeDeclarationAST>(type);
}   break;

#line 1231 "./glsl.g"

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

#line 1250 "./glsl.g"

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

#line 1270 "./glsl.g"

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

#line 1290 "./glsl.g"

case 88: {
    TypeAST *type = makeAstNode<QualifiedTypeAST>
        (sym(1).type_qualifier.qualifier, (TypeAST *)nullptr,
         sym(1).type_qualifier.layout_list);
    ast(1) = makeAstNode<TypeDeclarationAST>(type);
}   break;

#line 1300 "./glsl.g"

case 89: {
    function(1)->finishParams();
}   break;

#line 1307 "./glsl.g"

case 90: {
    // nothing to do.
}   break;

#line 1314 "./glsl.g"

case 91: {
    // nothing to do.
}   break;

#line 1321 "./glsl.g"

case 92: {
    function(1)->params = makeAstNode< List<ParameterDeclarationAST *> >
        (sym(2).param_declaration);
}   break;

#line 1329 "./glsl.g"

case 93: {
    function(1)->params = makeAstNode< List<ParameterDeclarationAST *> >
        (function(1)->params, sym(3).param_declaration);
}   break;

#line 1337 "./glsl.g"

case 94: {
    function(1) = makeAstNode<FunctionDeclarationAST>(type(1), string(2));
}   break;

#line 1344 "./glsl.g"

case 95: {
    sym(1).param_declarator.type = type(1);
    sym(1).param_declarator.name = string(2);
}   break;

#line 1352 "./glsl.g"

case 96: {
    sym(1).param_declarator.type = makeAstNode<ArrayTypeAST>(type(1), expression(4));
    sym(1).param_declarator.name = string(2);
}   break;

#line 1360 "./glsl.g"

case 97: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (makeAstNode<QualifiedTypeAST>
            (sym(1).qualifier, sym(3).param_declarator.type,
             (List<LayoutQualifierAST *> *)nullptr),
         ParameterDeclarationAST::Qualifier(sym(2).qualifier),
         sym(3).param_declarator.name);
}   break;

#line 1372 "./glsl.g"

case 98: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (sym(2).param_declarator.type,
         ParameterDeclarationAST::Qualifier(sym(1).qualifier),
         sym(2).param_declarator.name);
}   break;

#line 1382 "./glsl.g"

case 99: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (makeAstNode<QualifiedTypeAST>
            (sym(1).qualifier, type(3), (List<LayoutQualifierAST *> *)nullptr),
         ParameterDeclarationAST::Qualifier(sym(2).qualifier),
         (const QString *)nullptr);
}   break;

#line 1393 "./glsl.g"

case 100: {
    ast(1) = makeAstNode<ParameterDeclarationAST>
        (type(2), ParameterDeclarationAST::Qualifier(sym(1).qualifier),
         (const QString *)nullptr);
}   break;

#line 1402 "./glsl.g"

case 101: {
    sym(1).qualifier = ParameterDeclarationAST::In;
}   break;

#line 1409 "./glsl.g"

case 102: {
    sym(1).qualifier = ParameterDeclarationAST::In;
}   break;

#line 1416 "./glsl.g"

case 103: {
    sym(1).qualifier = ParameterDeclarationAST::Out;
}   break;

#line 1423 "./glsl.g"

case 104: {
    sym(1).qualifier = ParameterDeclarationAST::InOut;
}   break;

#line 1430 "./glsl.g"

case 105: {
    // nothing to do.
}   break;

#line 1437 "./glsl.g"

case 106: {
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
        (sym(1).declaration);
}   break;

#line 1445 "./glsl.g"

case 107: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1455 "./glsl.g"

case 108: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1466 "./glsl.g"

case 109: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type, expression(5));
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>(type, string(3));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1477 "./glsl.g"

case 110: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>
            (type, string(3), expression(7));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1489 "./glsl.g"

case 111: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    type = makeAstNode<ArrayTypeAST>(type, expression(5));
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>
            (type, string(3), expression(8));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1501 "./glsl.g"

case 112: {
    TypeAST *type = VariableDeclarationAST::declarationType(sym(1).declaration_list);
    DeclarationAST *decl = makeAstNode<VariableDeclarationAST>
            (type, string(3), expression(5));
    sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration_list, decl);
}   break;

#line 1512 "./glsl.g"

case 113: {
    ast(1) = makeAstNode<TypeDeclarationAST>(type(1));
}   break;

#line 1519 "./glsl.g"

case 114: {
    ast(1) = makeAstNode<VariableDeclarationAST>(type(1), string(2));
}   break;

#line 1526 "./glsl.g"

case 115: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1)), string(2));
}   break;

#line 1534 "./glsl.g"

case 116: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1), expression(4)), string(2));
}   break;

#line 1542 "./glsl.g"

case 117: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1)), string(2), expression(6));
}   break;

#line 1550 "./glsl.g"

case 118: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (makeAstNode<ArrayTypeAST>(type(1), expression(4)),
         string(2), expression(7));
}   break;

#line 1559 "./glsl.g"

case 119: {
    ast(1) = makeAstNode<VariableDeclarationAST>
        (type(1), string(2), expression(4));
}   break;

#line 1567 "./glsl.g"

case 120: {
    ast(1) = makeAstNode<InvariantDeclarationAST>(string(2));
}   break;

#line 1574 "./glsl.g"

case 121: {
    ast(1) = makeAstNode<QualifiedTypeAST>(0, type(1), (List<LayoutQualifierAST *> *)nullptr);
}   break;

#line 1581 "./glsl.g"

case 122: {
    ast(1) = makeAstNode<QualifiedTypeAST>
        (sym(1).type_qualifier.qualifier, type(2),
         sym(1).type_qualifier.layout_list);
}   break;

#line 1590 "./glsl.g"

case 123: {
    sym(1).qualifier = QualifiedTypeAST::Invariant;
}   break;

#line 1597 "./glsl.g"

case 124: {
    sym(1).qualifier = QualifiedTypeAST::Smooth;
}   break;

#line 1604 "./glsl.g"

case 125: {
    sym(1).qualifier = QualifiedTypeAST::Flat;
}   break;

#line 1611 "./glsl.g"

case 126: {
    sym(1).qualifier = QualifiedTypeAST::NoPerspective;
}   break;

#line 1618 "./glsl.g"

case 127: {
    sym(1) = sym(3);
}   break;

#line 1625 "./glsl.g"

case 128: {
    sym(1).layout_list = makeAstNode< List<LayoutQualifierAST *> >(sym(1).layout);
}   break;

#line 1632 "./glsl.g"

case 129: {
    sym(1).layout_list = makeAstNode< List<LayoutQualifierAST *> >(sym(1).layout_list, sym(3).layout);
}   break;

#line 1639 "./glsl.g"

case 130: {
    sym(1).layout = makeAstNode<LayoutQualifierAST>(string(1), (const QString *)nullptr);
}   break;

#line 1646 "./glsl.g"

case 131: {
    sym(1).layout = makeAstNode<LayoutQualifierAST>(string(1), string(3));
}   break;

#line 1653 "./glsl.g"

case 132: {
    sym(1).qualifier = QualifiedTypeAST::Const;
}   break;

#line 1660 "./glsl.g"

case 133: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier;
    sym(1).type_qualifier.layout_list = nullptr;
}   break;

#line 1668 "./glsl.g"

case 134: {
    sym(1).type_qualifier.layout_list = sym(1).layout_list;
    sym(1).type_qualifier.qualifier = 0;
}   break;

#line 1676 "./glsl.g"

case 135: {
    sym(1).type_qualifier.layout_list = sym(1).layout_list;
    sym(1).type_qualifier.qualifier = sym(2).qualifier;
}   break;

#line 1684 "./glsl.g"

case 136: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier;
    sym(1).type_qualifier.layout_list = nullptr;
}   break;

#line 1692 "./glsl.g"

case 137: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier;
    sym(1).type_qualifier.layout_list = nullptr;
}   break;

#line 1700 "./glsl.g"

case 138: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier;
    sym(1).type_qualifier.layout_list = nullptr;
}   break;

#line 1708 "./glsl.g"

case 139: {
    sym(1).type_qualifier.qualifier = sym(1).qualifier | sym(2).qualifier | sym(3).qualifier;
    sym(1).type_qualifier.layout_list = nullptr;
}   break;

#line 1716 "./glsl.g"

case 140: {
    sym(1).type_qualifier.qualifier = QualifiedTypeAST::Invariant;
    sym(1).type_qualifier.layout_list = nullptr;
}   break;

#line 1724 "./glsl.g"

case 141: {
    sym(1).qualifier = QualifiedTypeAST::Const;
}   break;

#line 1731 "./glsl.g"

case 142: {
    sym(1).qualifier = QualifiedTypeAST::Attribute;
}   break;

#line 1738 "./glsl.g"

case 143: {
    sym(1).qualifier = QualifiedTypeAST::Varying;
}   break;

#line 1745 "./glsl.g"

case 144: {
    sym(1).qualifier = QualifiedTypeAST::CentroidVarying;
}   break;

#line 1752 "./glsl.g"

case 145: {
    sym(1).qualifier = QualifiedTypeAST::In;
}   break;

#line 1759 "./glsl.g"

case 146: {
    sym(1).qualifier = QualifiedTypeAST::Out;
}   break;

#line 1766 "./glsl.g"

case 147: {
    sym(1).qualifier = QualifiedTypeAST::CentroidIn;
}   break;

#line 1773 "./glsl.g"

case 148: {
    sym(1).qualifier = QualifiedTypeAST::CentroidOut;
}   break;

#line 1780 "./glsl.g"

case 149: {
    sym(1).qualifier = QualifiedTypeAST::PatchIn;
}   break;

#line 1787 "./glsl.g"

case 150: {
    sym(1).qualifier = QualifiedTypeAST::PatchOut;
}   break;

#line 1794 "./glsl.g"

case 151: {
    sym(1).qualifier = QualifiedTypeAST::SampleIn;
}   break;

#line 1801 "./glsl.g"

case 152: {
    sym(1).qualifier = QualifiedTypeAST::SampleOut;
}   break;

#line 1808 "./glsl.g"

case 153: {
    sym(1).qualifier = QualifiedTypeAST::Uniform;
}   break;

#line 1815 "./glsl.g"

case 154: {
    // nothing to do.
}   break;

#line 1822 "./glsl.g"

case 155: {
    if (!type(2)->setPrecision(sym(1).precision)) {
        // TODO: issue an error about precision not allowed on this type.
    }
    ast(1) = type(2);
}   break;

#line 1832 "./glsl.g"

case 156: {
    // nothing to do.
}   break;

#line 1839 "./glsl.g"

case 157: {
    ast(1) = makeAstNode<ArrayTypeAST>(type(1));
}   break;

#line 1846 "./glsl.g"

case 158: {
    ast(1) = makeAstNode<ArrayTypeAST>(type(1), expression(3));
}   break;

#line 1853 "./glsl.g"

case 159: {
    ast(1) = makeBasicType(T_VOID);
}   break;

#line 1860 "./glsl.g"

case 160: {
    ast(1) = makeBasicType(T_FLOAT);
}   break;

#line 1867 "./glsl.g"

case 161: {
    ast(1) = makeBasicType(T_DOUBLE);
}   break;

#line 1874 "./glsl.g"

case 162: {
    ast(1) = makeBasicType(T_INT);
}   break;

#line 1881 "./glsl.g"

case 163: {
    ast(1) = makeBasicType(T_UINT);
}   break;

#line 1888 "./glsl.g"

case 164: {
    ast(1) = makeBasicType(T_ATOMIC_UINT);
} break;

#line 1895 "./glsl.g"

case 165: {
    ast(1) = makeBasicType(T_BOOL);
}   break;

#line 1902 "./glsl.g"

case 166: {
    ast(1) = makeBasicType(T_VEC2);
}   break;

#line 1909 "./glsl.g"

case 167: {
    ast(1) = makeBasicType(T_VEC3);
}   break;

#line 1916 "./glsl.g"

case 168: {
    ast(1) = makeBasicType(T_VEC4);
}   break;

#line 1923 "./glsl.g"

case 169: {
    ast(1) = makeBasicType(T_DVEC2);
}   break;

#line 1930 "./glsl.g"

case 170: {
    ast(1) = makeBasicType(T_DVEC3);
}   break;

#line 1937 "./glsl.g"

case 171: {
    ast(1) = makeBasicType(T_DVEC4);
}   break;

#line 1944 "./glsl.g"

case 172: {
    ast(1) = makeBasicType(T_BVEC2);
}   break;

#line 1951 "./glsl.g"

case 173: {
    ast(1) = makeBasicType(T_BVEC3);
}   break;

#line 1958 "./glsl.g"

case 174: {
    ast(1) = makeBasicType(T_BVEC4);
}   break;

#line 1965 "./glsl.g"

case 175: {
    ast(1) = makeBasicType(T_IVEC2);
}   break;

#line 1972 "./glsl.g"

case 176: {
    ast(1) = makeBasicType(T_IVEC3);
}   break;

#line 1979 "./glsl.g"

case 177: {
    ast(1) = makeBasicType(T_IVEC4);
}   break;

#line 1986 "./glsl.g"

case 178: {
    ast(1) = makeBasicType(T_UVEC2);
}   break;

#line 1993 "./glsl.g"

case 179: {
    ast(1) = makeBasicType(T_UVEC3);
}   break;

#line 2000 "./glsl.g"

case 180: {
    ast(1) = makeBasicType(T_UVEC4);
}   break;

#line 2007 "./glsl.g"

case 181: {
    ast(1) = makeBasicType(T_MAT2);
}   break;

#line 2014 "./glsl.g"

case 182: {
    ast(1) = makeBasicType(T_MAT3);
}   break;

#line 2021 "./glsl.g"

case 183: {
    ast(1) = makeBasicType(T_MAT4);
}   break;

#line 2028 "./glsl.g"

case 184: {
    ast(1) = makeBasicType(T_MAT2);
}   break;

#line 2035 "./glsl.g"

case 185: {
    ast(1) = makeBasicType(T_MAT2X3);
}   break;

#line 2042 "./glsl.g"

case 186: {
    ast(1) = makeBasicType(T_MAT2X4);
}   break;

#line 2049 "./glsl.g"

case 187: {
    ast(1) = makeBasicType(T_MAT3X2);
}   break;

#line 2056 "./glsl.g"

case 188: {
    ast(1) = makeBasicType(T_MAT3);
}   break;

#line 2063 "./glsl.g"

case 189: {
    ast(1) = makeBasicType(T_MAT3X4);
}   break;

#line 2070 "./glsl.g"

case 190: {
    ast(1) = makeBasicType(T_MAT4X2);
}   break;

#line 2077 "./glsl.g"

case 191: {
    ast(1) = makeBasicType(T_MAT4X3);
}   break;

#line 2084 "./glsl.g"

case 192: {
    ast(1) = makeBasicType(T_MAT4);
}   break;

#line 2091 "./glsl.g"

case 193: {
    ast(1) = makeBasicType(T_DMAT2);
}   break;

#line 2098 "./glsl.g"

case 194: {
    ast(1) = makeBasicType(T_DMAT3);
}   break;

#line 2105 "./glsl.g"

case 195: {
    ast(1) = makeBasicType(T_DMAT4);
}   break;

#line 2112 "./glsl.g"

case 196: {
    ast(1) = makeBasicType(T_DMAT2);
}   break;

#line 2119 "./glsl.g"

case 197: {
    ast(1) = makeBasicType(T_DMAT2X3);
}   break;

#line 2126 "./glsl.g"

case 198: {
    ast(1) = makeBasicType(T_DMAT2X4);
}   break;

#line 2133 "./glsl.g"

case 199: {
    ast(1) = makeBasicType(T_DMAT3X2);
}   break;

#line 2140 "./glsl.g"

case 200: {
    ast(1) = makeBasicType(T_DMAT3);
}   break;

#line 2147 "./glsl.g"

case 201: {
    ast(1) = makeBasicType(T_DMAT3X4);
}   break;

#line 2154 "./glsl.g"

case 202: {
    ast(1) = makeBasicType(T_DMAT4X2);
}   break;

#line 2161 "./glsl.g"

case 203: {
    ast(1) = makeBasicType(T_DMAT4X3);
}   break;

#line 2168 "./glsl.g"

case 204: {
    ast(1) = makeBasicType(T_DMAT4);
}   break;

#line 2175 "./glsl.g"

case 205: {
    ast(1) = makeBasicType(T_SAMPLER1D);
}   break;

#line 2182 "./glsl.g"

case 206: {
    ast(1) = makeBasicType(T_SAMPLER2D);
}   break;

#line 2189 "./glsl.g"

case 207: {
    ast(1) = makeBasicType(T_SAMPLER3D);
}   break;

#line 2196 "./glsl.g"

case 208: {
    ast(1) = makeBasicType(T_SAMPLERCUBE);
}   break;

#line 2203 "./glsl.g"

case 209: {
    ast(1) = makeBasicType(T_SAMPLER1DSHADOW);
}   break;

#line 2210 "./glsl.g"

case 210: {
    ast(1) = makeBasicType(T_SAMPLER2DSHADOW);
}   break;

#line 2217 "./glsl.g"

case 211: {
    ast(1) = makeBasicType(T_SAMPLERCUBESHADOW);
}   break;

#line 2224 "./glsl.g"

case 212: {
    ast(1) = makeBasicType(T_SAMPLER1DARRAY);
}   break;

#line 2231 "./glsl.g"

case 213: {
    ast(1) = makeBasicType(T_SAMPLER2DARRAY);
}   break;

#line 2238 "./glsl.g"

case 214: {
    ast(1) = makeBasicType(T_SAMPLER1DARRAYSHADOW);
}   break;

#line 2245 "./glsl.g"

case 215: {
    ast(1) = makeBasicType(T_SAMPLER2DARRAYSHADOW);
}   break;

#line 2252 "./glsl.g"

case 216: {
    ast(1) = makeBasicType(T_SAMPLERCUBEARRAY);
}   break;

#line 2259 "./glsl.g"

case 217: {
    ast(1) = makeBasicType(T_SAMPLERCUBEARRAYSHADOW);
}   break;

#line 2266 "./glsl.g"

case 218: {
    ast(1) = makeBasicType(T_ISAMPLER1D);
}   break;

#line 2273 "./glsl.g"

case 219: {
    ast(1) = makeBasicType(T_ISAMPLER2D);
}   break;

#line 2280 "./glsl.g"

case 220: {
    ast(1) = makeBasicType(T_ISAMPLER3D);
}   break;

#line 2287 "./glsl.g"

case 221: {
    ast(1) = makeBasicType(T_ISAMPLERCUBE);
}   break;

#line 2294 "./glsl.g"

case 222: {
    ast(1) = makeBasicType(T_ISAMPLER1DARRAY);
}   break;

#line 2301 "./glsl.g"

case 223: {
    ast(1) = makeBasicType(T_ISAMPLER2DARRAY);
}   break;

#line 2308 "./glsl.g"

case 224: {
    ast(1) = makeBasicType(T_ISAMPLERCUBEARRAY);
}   break;

#line 2315 "./glsl.g"

case 225: {
    ast(1) = makeBasicType(T_USAMPLER1D);
}   break;

#line 2322 "./glsl.g"

case 226: {
    ast(1) = makeBasicType(T_USAMPLER2D);
}   break;

#line 2329 "./glsl.g"

case 227: {
    ast(1) = makeBasicType(T_USAMPLER3D);
}   break;

#line 2336 "./glsl.g"

case 228: {
    ast(1) = makeBasicType(T_USAMPLERCUBE);
}   break;

#line 2343 "./glsl.g"

case 229: {
    ast(1) = makeBasicType(T_USAMPLER1DARRAY);
}   break;

#line 2350 "./glsl.g"

case 230: {
    ast(1) = makeBasicType(T_USAMPLER2DARRAY);
}   break;

#line 2357 "./glsl.g"

case 231: {
    ast(1) = makeBasicType(T_USAMPLERCUBEARRAY);
}   break;

#line 2364 "./glsl.g"

case 232: {
    ast(1) = makeBasicType(T_SAMPLER2DRECT);
}   break;

#line 2371 "./glsl.g"

case 233: {
    ast(1) = makeBasicType(T_SAMPLER2DRECTSHADOW);
}   break;

#line 2378 "./glsl.g"

case 234: {
    ast(1) = makeBasicType(T_ISAMPLER2DRECT);
}   break;

#line 2385 "./glsl.g"

case 235: {
    ast(1) = makeBasicType(T_USAMPLER2DRECT);
}   break;

#line 2392 "./glsl.g"

case 236: {
    ast(1) = makeBasicType(T_SAMPLERBUFFER);
}   break;

#line 2399 "./glsl.g"

case 237: {
    ast(1) = makeBasicType(T_ISAMPLERBUFFER);
}   break;

#line 2406 "./glsl.g"

case 238: {
    ast(1) = makeBasicType(T_USAMPLERBUFFER);
}   break;

#line 2413 "./glsl.g"

case 239: {
    ast(1) = makeBasicType(T_SAMPLER2DMS);
}   break;

#line 2420 "./glsl.g"

case 240: {
    ast(1) = makeBasicType(T_ISAMPLER2DMS);
}   break;

#line 2427 "./glsl.g"

case 241: {
    ast(1) = makeBasicType(T_USAMPLER2DMS);
}   break;

#line 2434 "./glsl.g"

case 242: {
    ast(1) = makeBasicType(T_SAMPLER2DMSARRAY);
}   break;

#line 2441 "./glsl.g"

case 243: {
    ast(1) = makeBasicType(T_ISAMPLER2DMSARRAY);
}   break;

#line 2448 "./glsl.g"

case 244: {
    ast(1) = makeBasicType(T_USAMPLER2DMSARRAY);
}   break;

#line 2455 "./glsl.g"

case 245: {
    ast(1) = makeBasicType(T_IIMAGE1D);
} break;

#line 2462 "./glsl.g"

case 246: {
    ast(1) = makeBasicType(T_IIMAGE1DARRAY);
} break;

#line 2469 "./glsl.g"

case 247: {
    ast(1) = makeBasicType(T_IIMAGE2D);
} break;

#line 2476 "./glsl.g"

case 248: {
    ast(1) = makeBasicType(T_IIMAGE2DARRAY);
} break;

#line 2483 "./glsl.g"

case 249: {
    ast(1) = makeBasicType(T_IIMAGE2DMS);
} break;

#line 2490 "./glsl.g"

case 250: {
    ast(1) = makeBasicType(T_IIMAGE2DMSARRAY);
} break;

#line 2497 "./glsl.g"

case 251: {
    ast(1) = makeBasicType(T_IIMAGE2DRECT);
} break;

#line 2504 "./glsl.g"

case 252: {
    ast(1) = makeBasicType(T_IIMAGE3D);
} break;

#line 2511 "./glsl.g"

case 253: {
    ast(1) = makeBasicType(T_IIMAGEBUFFER);
} break;

#line 2518 "./glsl.g"

case 254: {
    ast(1) = makeBasicType(T_IIMAGECUBE);
} break;

#line 2525 "./glsl.g"

case 255: {
    ast(1) = makeBasicType(T_IIMAGECUBEARRAY);
} break;

#line 2532 "./glsl.g"

case 256: {
    ast(1) = makeBasicType(T_IMAGE1D);
} break;

#line 2539 "./glsl.g"

case 257: {
    ast(1) = makeBasicType(T_IMAGE1DARRAY);
} break;

#line 2546 "./glsl.g"

case 258: {
    ast(1) = makeBasicType(T_IMAGE2D);
} break;

#line 2553 "./glsl.g"

case 259: {
    ast(1) = makeBasicType(T_IMAGE2DARRAY);
} break;

#line 2560 "./glsl.g"

case 260: {
    ast(1) = makeBasicType(T_IMAGE2DMS);
} break;

#line 2567 "./glsl.g"

case 261: {
    ast(1) = makeBasicType(T_IMAGE2DMSARRAY);
} break;

#line 2574 "./glsl.g"

case 262: {
    ast(1) = makeBasicType(T_IMAGE2DRECT);
} break;

#line 2581 "./glsl.g"

case 263: {
    ast(1) = makeBasicType(T_IMAGE3D);
} break;

#line 2588 "./glsl.g"

case 264: {
    ast(1) = makeBasicType(T_IMAGEBUFFER);
} break;

#line 2595 "./glsl.g"

case 265: {
    ast(1) = makeBasicType(T_IMAGECUBE);
} break;

#line 2602 "./glsl.g"

case 266: {
    ast(1) = makeBasicType(T_IMAGECUBEARRAY);
} break;

#line 2609 "./glsl.g"

case 267: {
    ast(1) = makeBasicType(T_UIMAGE1D);
} break;

#line 2616 "./glsl.g"

case 268: {
    ast(1) = makeBasicType(T_UIMAGE1DARRAY);
} break;

#line 2623 "./glsl.g"

case 269: {
    ast(1) = makeBasicType(T_UIMAGE2D);
} break;

#line 2630 "./glsl.g"

case 270: {
    ast(1) = makeBasicType(T_UIMAGE2DARRAY);
} break;

#line 2637 "./glsl.g"

case 271: {
    ast(1) = makeBasicType(T_UIMAGE2DMS);
} break;

#line 2644 "./glsl.g"

case 272: {
    ast(1) = makeBasicType(T_UIMAGE2DMSARRAY);
} break;

#line 2651 "./glsl.g"

case 273: {
    ast(1) = makeBasicType(T_UIMAGE2DRECT);
} break;

#line 2658 "./glsl.g"

case 274: {
    ast(1) = makeBasicType(T_UIMAGE3D);
} break;

#line 2665 "./glsl.g"

case 275: {
    ast(1) = makeBasicType(T_UIMAGEBUFFER);
} break;

#line 2672 "./glsl.g"

case 276: {
    ast(1) = makeBasicType(T_UIMAGECUBE);
} break;

#line 2679 "./glsl.g"

case 277: {
    ast(1) = makeBasicType(T_UIMAGECUBEARRAY);
} break;

#line 2686 "./glsl.g"

case 278: {
    // nothing to do.
}   break;

#line 2693 "./glsl.g"

case 279: {
    ast(1) = makeAstNode<NamedTypeAST>(string(1));
}   break;

#line 2700 "./glsl.g"

case 280: {
    sym(1).precision = TypeAST::Highp;
}   break;

#line 2707 "./glsl.g"

case 281: {
    sym(1).precision = TypeAST::Mediump;
}   break;

#line 2714 "./glsl.g"

case 282: {
    sym(1).precision = TypeAST::Lowp;
}   break;

#line 2721 "./glsl.g"

case 283: {
    ast(1) = makeAstNode<StructTypeAST>(string(2), sym(4).field_list);
}   break;

#line 2728 "./glsl.g"

case 284: {
    ast(1) = makeAstNode<StructTypeAST>(sym(3).field_list);
}   break;

#line 2735 "./glsl.g"

case 285: {
    // nothing to do.
}   break;

#line 2742 "./glsl.g"

case 286: {
    sym(1).field_list = appendLists(sym(1).field_list, sym(2).field_list);
}   break;

#line 2749 "./glsl.g"

case 287: {
    sym(1).field_list = StructTypeAST::fixInnerTypes(type(1), sym(2).field_list);
}   break;

#line 2756 "./glsl.g"

case 288: {
    sym(1).field_list = StructTypeAST::fixInnerTypes
        (makeAstNode<QualifiedTypeAST>
            (sym(1).type_qualifier.qualifier, type(2),
             sym(1).type_qualifier.layout_list), sym(3).field_list);
}   break;

#line 2766 "./glsl.g"

case 289: {
    // nothing to do.
    sym(1).field_list = makeAstNode< List<StructTypeAST::Field *> >(sym(1).field);
}   break;

#line 2774 "./glsl.g"

case 290: {
    sym(1).field_list = makeAstNode< List<StructTypeAST::Field *> >(sym(1).field_list, sym(3).field);
}   break;

#line 2781 "./glsl.g"

case 291: {
    sym(1).field = makeAstNode<StructTypeAST::Field>(string(1));
}   break;

#line 2788 "./glsl.g"

case 292: {
    sym(1).field = makeAstNode<StructTypeAST::Field>
        (string(1), makeAstNode<ArrayTypeAST>((TypeAST *)nullptr));
}   break;

#line 2796 "./glsl.g"

case 293: {
    sym(1).field = makeAstNode<StructTypeAST::Field>
        (string(1), makeAstNode<ArrayTypeAST>((TypeAST *)nullptr, expression(3)));
}   break;

#line 2804 "./glsl.g"

case 294: {
    // nothing to do.
}   break;

#line 2811 "./glsl.g"

case 295: {
    ast(1) = makeAstNode<DeclarationStatementAST>(sym(1).declaration);
}   break;

#line 2818 "./glsl.g"

case 296: {
    // nothing to do.
}   break;

#line 2825 "./glsl.g"

case 297: {
    // nothing to do.
}   break;

#line 2832 "./glsl.g"

case 298: {
    // nothing to do.
}   break;

#line 2839 "./glsl.g"

case 299: {
    // nothing to do.
}   break;

#line 2846 "./glsl.g"

case 300: {
    // nothing to do.
}   break;

#line 2853 "./glsl.g"

case 301: {
    // nothing to do.
}   break;

#line 2860 "./glsl.g"

case 302: {
    // nothing to do.
}   break;

#line 2867 "./glsl.g"

case 303: {
    // nothing to do.
}   break;

#line 2874 "./glsl.g"

case 304: {
    // nothing to do.
}   break;

#line 2881 "./glsl.g"

case 305: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>();
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(2)).end();
    ast(1) = stmt;
}   break;

#line 2891 "./glsl.g"

case 306: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>(sym(2).statement_list);
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(3)).end();
    ast(1) = stmt;
}   break;

#line 2901 "./glsl.g"

case 307: {
    // nothing to do.
}   break;

#line 2908 "./glsl.g"

case 308: {
    // nothing to do.
}   break;

#line 2915 "./glsl.g"

case 309: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>();
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(2)).end();
    ast(1) = stmt;
}   break;

#line 2925 "./glsl.g"

case 310: {
    CompoundStatementAST *stmt = makeAstNode<CompoundStatementAST>(sym(2).statement_list);
    stmt->start = tokenAt(location(1)).begin();
    stmt->end = tokenAt(location(3)).end();
    ast(1) = stmt;
}   break;

#line 2935 "./glsl.g"

case 311: {
    sym(1).statement_list = makeAstNode< List<StatementAST *> >(sym(1).statement);
}   break;

#line 2942 "./glsl.g"

case 312: {
    sym(1).statement_list = makeAstNode< List<StatementAST *> >(sym(1).statement_list, sym(2).statement);
}   break;

#line 2949 "./glsl.g"

case 313: {
    ast(1) = makeAstNode<CompoundStatementAST>();  // Empty statement
}   break;

#line 2956 "./glsl.g"

case 314: {
    ast(1) = makeAstNode<ExpressionStatementAST>(expression(1));
}   break;

#line 2963 "./glsl.g"

case 315: {
    ast(1) = makeAstNode<IfStatementAST>(expression(3), sym(5).ifstmt.thenClause, sym(5).ifstmt.elseClause);
}   break;

#line 2970 "./glsl.g"

case 316: {
    sym(1).ifstmt.thenClause = statement(1);
    sym(1).ifstmt.elseClause = statement(3);
}   break;

#line 2978 "./glsl.g"

case 317: {
    sym(1).ifstmt.thenClause = statement(1);
    sym(1).ifstmt.elseClause = nullptr;
}   break;

#line 2986 "./glsl.g"

case 318: {
    // nothing to do.
}   break;

#line 2993 "./glsl.g"

case 319: {
    ast(1) = makeAstNode<DeclarationExpressionAST>
        (type(1), string(2), expression(4));
}   break;

#line 3001 "./glsl.g"

case 320: {
    ast(1) = makeAstNode<SwitchStatementAST>(expression(3), statement(6));
}   break;

#line 3008 "./glsl.g"

case 321: {
    ast(1) = makeAstNode<CompoundStatementAST>();
}   break;

#line 3015 "./glsl.g"

case 322: {
    ast(1) = makeAstNode<CompoundStatementAST>(sym(1).statement_list);
}   break;

#line 3022 "./glsl.g"

case 323: {
    ast(1) = makeAstNode<CaseLabelStatementAST>(expression(2));
}   break;

#line 3029 "./glsl.g"

case 324: {
    ast(1) = makeAstNode<CaseLabelStatementAST>();
}   break;

#line 3036 "./glsl.g"

case 325: {
    ast(1) = makeAstNode<WhileStatementAST>(expression(3), statement(5));
}   break;

#line 3043 "./glsl.g"

case 326: {
    ast(1) = makeAstNode<DoStatementAST>(statement(2), expression(5));
}   break;

#line 3050 "./glsl.g"

case 327: {
    ast(1) = makeAstNode<ForStatementAST>(statement(3), sym(4).forstmt.condition, sym(4).forstmt.increment, statement(6));
}   break;

#line 3057 "./glsl.g"

case 328: {
    // nothing to do.
}   break;

#line 3064 "./glsl.g"

case 329: {
    // nothing to do.
}   break;

#line 3071 "./glsl.g"

case 330: {
    // nothing to do.
}   break;

#line 3078 "./glsl.g"

case 331: {
    // nothing to do.
}   break;

#line 3085 "./glsl.g"

case 332: {
    sym(1).forstmt.condition = expression(1);
    sym(1).forstmt.increment = nullptr;
}   break;

#line 3093 "./glsl.g"

case 333: {
    sym(1).forstmt.condition = expression(1);
    sym(1).forstmt.increment = expression(3);
}   break;

#line 3101 "./glsl.g"

case 334: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Continue);
}   break;

#line 3108 "./glsl.g"

case 335: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Break);
}   break;

#line 3115 "./glsl.g"

case 336: {
    ast(1) = makeAstNode<ReturnStatementAST>();
}   break;

#line 3122 "./glsl.g"

case 337: {
    ast(1) = makeAstNode<ReturnStatementAST>(expression(2));
}   break;

#line 3129 "./glsl.g"

case 338: {
    ast(1) = makeAstNode<JumpStatementAST>(AST::Kind_Discard);
}   break;

#line 3136 "./glsl.g"

case 339: {
    ast(1) = makeAstNode<TranslationUnitAST>(sym(1).declaration_list);
}   break;

#line 3143 "./glsl.g"

case 340: {
    if (sym(1).declaration) {
        sym(1).declaration_list = makeAstNode< List<DeclarationAST *> >
            (sym(1).declaration);
    } else {
        sym(1).declaration_list = nullptr;
    }
}   break;

#line 3155 "./glsl.g"

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

#line 3172 "./glsl.g"

case 342: {
    // nothing to do.
}   break;

#line 3179 "./glsl.g"

case 343: {
    // nothing to do.
}   break;

#line 3186 "./glsl.g"

case 344: {
    ast(1) = nullptr;
}   break;

#line 3193 "./glsl.g"

case 345: {
    function(1)->body = statement(2);
}   break;

#line 3200 "./glsl.g"

case 346: {
    ast(1) = nullptr;
}   break;

#line 3208 "./glsl.g"

case 347: {
    ast(1) = ast(2);
}   break;

#line 3215 "./glsl.g"

case 348: {
    ast(1) = ast(2);
}   break;

#line 3221 "./glsl.g"

} // end switch
} // end Parser::reduce()
