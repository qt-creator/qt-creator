
#line 291 "./glsl.g"

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
#include <cstdio>
#include <cassert>

using namespace GLSL;

Parser::Parser(Engine *engine, const char *source, unsigned size, int variant)
    : _tos(-1), _index(0)
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
    int yyloc = -1;
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

    fprintf(stderr, "unexpected token `%s' at line %d\n", yytoken != -1 ? spell[yytoken] : "",
        _tokens[yyloc].line + 1);

    return 0;
}

#line 452 "./glsl.g"

void Parser::reduce(int ruleno)
{
switch(ruleno) {

#line 461 "./glsl.g"

case 0: {
    ast(1) = new IdentifierExpression(*sym(1).string);
}   break;

#line 468 "./glsl.g"

case 1: {
    // ast(1) = new ...AST(...);
}   break;

#line 475 "./glsl.g"

case 2: {
    // ast(1) = new ...AST(...);
}   break;

#line 482 "./glsl.g"

case 3: {
    // ast(1) = new ...AST(...);
}   break;

#line 489 "./glsl.g"

case 4: {
    // ast(1) = new ...AST(...);
}   break;

#line 496 "./glsl.g"

case 5: {
    // ast(1) = new ...AST(...);
}   break;

#line 503 "./glsl.g"

case 6: {
    // ast(1) = new ...AST(...);
}   break;

#line 510 "./glsl.g"

case 7: {
    // ast(1) = new ...AST(...);
}   break;

#line 517 "./glsl.g"

case 8: {
    // ast(1) = new ...AST(...);
}   break;

#line 524 "./glsl.g"

case 9: {
    // ast(1) = new ...AST(...);
}   break;

#line 531 "./glsl.g"

case 10: {
    // ast(1) = new ...AST(...);
}   break;

#line 538 "./glsl.g"

case 11: {
    // ast(1) = new ...AST(...);
}   break;

#line 545 "./glsl.g"

case 12: {
    // ast(1) = new ...AST(...);
}   break;

#line 552 "./glsl.g"

case 13: {
    // ast(1) = new ...AST(...);
}   break;

#line 559 "./glsl.g"

case 14: {
    // ast(1) = new ...AST(...);
}   break;

#line 566 "./glsl.g"

case 15: {
    // ast(1) = new ...AST(...);
}   break;

#line 573 "./glsl.g"

case 16: {
    // ast(1) = new ...AST(...);
}   break;

#line 580 "./glsl.g"

case 17: {
    // ast(1) = new ...AST(...);
}   break;

#line 587 "./glsl.g"

case 18: {
    // ast(1) = new ...AST(...);
}   break;

#line 594 "./glsl.g"

case 19: {
    // ast(1) = new ...AST(...);
}   break;

#line 601 "./glsl.g"

case 20: {
    // ast(1) = new ...AST(...);
}   break;

#line 608 "./glsl.g"

case 21: {
    // ast(1) = new ...AST(...);
}   break;

#line 615 "./glsl.g"

case 22: {
    // ast(1) = new ...AST(...);
}   break;

#line 622 "./glsl.g"

case 23: {
    // ast(1) = new ...AST(...);
}   break;

#line 629 "./glsl.g"

case 24: {
    // ast(1) = new ...AST(...);
}   break;

#line 636 "./glsl.g"

case 25: {
    // ast(1) = new ...AST(...);
}   break;

#line 643 "./glsl.g"

case 26: {
    // ast(1) = new ...AST(...);
}   break;

#line 650 "./glsl.g"

case 27: {
    // ast(1) = new ...AST(...);
}   break;

#line 657 "./glsl.g"

case 28: {
    // ast(1) = new ...AST(...);
}   break;

#line 664 "./glsl.g"

case 29: {
    // ast(1) = new ...AST(...);
}   break;

#line 671 "./glsl.g"

case 30: {
    // ast(1) = new ...AST(...);
}   break;

#line 678 "./glsl.g"

case 31: {
    // ast(1) = new ...AST(...);
}   break;

#line 685 "./glsl.g"

case 32: {
    // ast(1) = new ...AST(...);
}   break;

#line 692 "./glsl.g"

case 33: {
    // ast(1) = new ...AST(...);
}   break;

#line 699 "./glsl.g"

case 34: {
    // ast(1) = new ...AST(...);
}   break;

#line 706 "./glsl.g"

case 35: {
    // ast(1) = new ...AST(...);
}   break;

#line 713 "./glsl.g"

case 36: {
    // ast(1) = new ...AST(...);
}   break;

#line 720 "./glsl.g"

case 37: {
    // ast(1) = new ...AST(...);
}   break;

#line 727 "./glsl.g"

case 38: {
    // ast(1) = new ...AST(...);
}   break;

#line 734 "./glsl.g"

case 39: {
    // ast(1) = new ...AST(...);
}   break;

#line 741 "./glsl.g"

case 40: {
    // ast(1) = new ...AST(...);
}   break;

#line 748 "./glsl.g"

case 41: {
    // ast(1) = new ...AST(...);
}   break;

#line 755 "./glsl.g"

case 42: {
    // ast(1) = new ...AST(...);
}   break;

#line 762 "./glsl.g"

case 43: {
    // ast(1) = new ...AST(...);
}   break;

#line 769 "./glsl.g"

case 44: {
    // ast(1) = new ...AST(...);
}   break;

#line 776 "./glsl.g"

case 45: {
    // ast(1) = new ...AST(...);
}   break;

#line 783 "./glsl.g"

case 46: {
    // ast(1) = new ...AST(...);
}   break;

#line 790 "./glsl.g"

case 47: {
    // ast(1) = new ...AST(...);
}   break;

#line 797 "./glsl.g"

case 48: {
    // ast(1) = new ...AST(...);
}   break;

#line 804 "./glsl.g"

case 49: {
    // ast(1) = new ...AST(...);
}   break;

#line 811 "./glsl.g"

case 50: {
    // ast(1) = new ...AST(...);
}   break;

#line 818 "./glsl.g"

case 51: {
    // ast(1) = new ...AST(...);
}   break;

#line 825 "./glsl.g"

case 52: {
    // ast(1) = new ...AST(...);
}   break;

#line 832 "./glsl.g"

case 53: {
    // ast(1) = new ...AST(...);
}   break;

#line 839 "./glsl.g"

case 54: {
    // ast(1) = new ...AST(...);
}   break;

#line 846 "./glsl.g"

case 55: {
    // ast(1) = new ...AST(...);
}   break;

#line 853 "./glsl.g"

case 56: {
    // ast(1) = new ...AST(...);
}   break;

#line 860 "./glsl.g"

case 57: {
    // ast(1) = new ...AST(...);
}   break;

#line 867 "./glsl.g"

case 58: {
    // ast(1) = new ...AST(...);
}   break;

#line 874 "./glsl.g"

case 59: {
    // ast(1) = new ...AST(...);
}   break;

#line 881 "./glsl.g"

case 60: {
    // ast(1) = new ...AST(...);
}   break;

#line 888 "./glsl.g"

case 61: {
    // ast(1) = new ...AST(...);
}   break;

#line 895 "./glsl.g"

case 62: {
    // ast(1) = new ...AST(...);
}   break;

#line 902 "./glsl.g"

case 63: {
    // ast(1) = new ...AST(...);
}   break;

#line 909 "./glsl.g"

case 64: {
    // ast(1) = new ...AST(...);
}   break;

#line 916 "./glsl.g"

case 65: {
    // ast(1) = new ...AST(...);
}   break;

#line 923 "./glsl.g"

case 66: {
    // ast(1) = new ...AST(...);
}   break;

#line 930 "./glsl.g"

case 67: {
    // ast(1) = new ...AST(...);
}   break;

#line 937 "./glsl.g"

case 68: {
    // ast(1) = new ...AST(...);
}   break;

#line 944 "./glsl.g"

case 69: {
    // ast(1) = new ...AST(...);
}   break;

#line 951 "./glsl.g"

case 70: {
    // ast(1) = new ...AST(...);
}   break;

#line 958 "./glsl.g"

case 71: {
    // ast(1) = new ...AST(...);
}   break;

#line 965 "./glsl.g"

case 72: {
    // ast(1) = new ...AST(...);
}   break;

#line 972 "./glsl.g"

case 73: {
    // ast(1) = new ...AST(...);
}   break;

#line 979 "./glsl.g"

case 74: {
    // ast(1) = new ...AST(...);
}   break;

#line 986 "./glsl.g"

case 75: {
    // ast(1) = new ...AST(...);
}   break;

#line 993 "./glsl.g"

case 76: {
    // ast(1) = new ...AST(...);
}   break;

#line 1000 "./glsl.g"

case 77: {
    // ast(1) = new ...AST(...);
}   break;

#line 1007 "./glsl.g"

case 78: {
    // ast(1) = new ...AST(...);
}   break;

#line 1014 "./glsl.g"

case 79: {
    // ast(1) = new ...AST(...);
}   break;

#line 1021 "./glsl.g"

case 80: {
    // ast(1) = new ...AST(...);
}   break;

#line 1028 "./glsl.g"

case 81: {
    // ast(1) = new ...AST(...);
}   break;

#line 1035 "./glsl.g"

case 82: {
    // ast(1) = new ...AST(...);
}   break;

#line 1042 "./glsl.g"

case 83: {
    // ast(1) = new ...AST(...);
}   break;

#line 1049 "./glsl.g"

case 84: {
    // ast(1) = new ...AST(...);
}   break;

#line 1056 "./glsl.g"

case 85: {
    // ast(1) = new ...AST(...);
}   break;

#line 1063 "./glsl.g"

case 86: {
    // ast(1) = new ...AST(...);
}   break;

#line 1070 "./glsl.g"

case 87: {
    // ast(1) = new ...AST(...);
}   break;

#line 1077 "./glsl.g"

case 88: {
    // ast(1) = new ...AST(...);
}   break;

#line 1084 "./glsl.g"

case 89: {
    // ast(1) = new ...AST(...);
}   break;

#line 1091 "./glsl.g"

case 90: {
    // ast(1) = new ...AST(...);
}   break;

#line 1098 "./glsl.g"

case 91: {
    // ast(1) = new ...AST(...);
}   break;

#line 1105 "./glsl.g"

case 92: {
    // ast(1) = new ...AST(...);
}   break;

#line 1112 "./glsl.g"

case 93: {
    // ast(1) = new ...AST(...);
}   break;

#line 1119 "./glsl.g"

case 94: {
    // ast(1) = new ...AST(...);
}   break;

#line 1126 "./glsl.g"

case 95: {
    // ast(1) = new ...AST(...);
}   break;

#line 1133 "./glsl.g"

case 96: {
    // ast(1) = new ...AST(...);
}   break;

#line 1140 "./glsl.g"

case 97: {
    // ast(1) = new ...AST(...);
}   break;

#line 1147 "./glsl.g"

case 98: {
    // ast(1) = new ...AST(...);
}   break;

#line 1154 "./glsl.g"

case 99: {
    // ast(1) = new ...AST(...);
}   break;

#line 1161 "./glsl.g"

case 100: {
    // ast(1) = new ...AST(...);
}   break;

#line 1168 "./glsl.g"

case 101: {
    // ast(1) = new ...AST(...);
}   break;

#line 1175 "./glsl.g"

case 102: {
    // ast(1) = new ...AST(...);
}   break;

#line 1182 "./glsl.g"

case 103: {
    // ast(1) = new ...AST(...);
}   break;

#line 1189 "./glsl.g"

case 104: {
    // ast(1) = new ...AST(...);
}   break;

#line 1196 "./glsl.g"

case 105: {
    // ast(1) = new ...AST(...);
}   break;

#line 1203 "./glsl.g"

case 106: {
    // ast(1) = new ...AST(...);
}   break;

#line 1210 "./glsl.g"

case 107: {
    // ast(1) = new ...AST(...);
}   break;

#line 1217 "./glsl.g"

case 108: {
    // ast(1) = new ...AST(...);
}   break;

#line 1224 "./glsl.g"

case 109: {
    // ast(1) = new ...AST(...);
}   break;

#line 1231 "./glsl.g"

case 110: {
    // ast(1) = new ...AST(...);
}   break;

#line 1238 "./glsl.g"

case 111: {
    // ast(1) = new ...AST(...);
}   break;

#line 1245 "./glsl.g"

case 112: {
    // ast(1) = new ...AST(...);
}   break;

#line 1252 "./glsl.g"

case 113: {
    // ast(1) = new ...AST(...);
}   break;

#line 1259 "./glsl.g"

case 114: {
    // ast(1) = new ...AST(...);
}   break;

#line 1266 "./glsl.g"

case 115: {
    // ast(1) = new ...AST(...);
}   break;

#line 1273 "./glsl.g"

case 116: {
    // ast(1) = new ...AST(...);
}   break;

#line 1280 "./glsl.g"

case 117: {
    // ast(1) = new ...AST(...);
}   break;

#line 1287 "./glsl.g"

case 118: {
    // ast(1) = new ...AST(...);
}   break;

#line 1294 "./glsl.g"

case 119: {
    // ast(1) = new ...AST(...);
}   break;

#line 1301 "./glsl.g"

case 120: {
    // ast(1) = new ...AST(...);
}   break;

#line 1308 "./glsl.g"

case 121: {
    // ast(1) = new ...AST(...);
}   break;

#line 1315 "./glsl.g"

case 122: {
    // ast(1) = new ...AST(...);
}   break;

#line 1322 "./glsl.g"

case 123: {
    // ast(1) = new ...AST(...);
}   break;

#line 1329 "./glsl.g"

case 124: {
    // ast(1) = new ...AST(...);
}   break;

#line 1336 "./glsl.g"

case 125: {
    // ast(1) = new ...AST(...);
}   break;

#line 1343 "./glsl.g"

case 126: {
    // ast(1) = new ...AST(...);
}   break;

#line 1350 "./glsl.g"

case 127: {
    // ast(1) = new ...AST(...);
}   break;

#line 1357 "./glsl.g"

case 128: {
    // ast(1) = new ...AST(...);
}   break;

#line 1364 "./glsl.g"

case 129: {
    // ast(1) = new ...AST(...);
}   break;

#line 1371 "./glsl.g"

case 130: {
    // ast(1) = new ...AST(...);
}   break;

#line 1378 "./glsl.g"

case 131: {
    // ast(1) = new ...AST(...);
}   break;

#line 1385 "./glsl.g"

case 132: {
    // ast(1) = new ...AST(...);
}   break;

#line 1392 "./glsl.g"

case 133: {
    // ast(1) = new ...AST(...);
}   break;

#line 1399 "./glsl.g"

case 134: {
    // ast(1) = new ...AST(...);
}   break;

#line 1406 "./glsl.g"

case 135: {
    // ast(1) = new ...AST(...);
}   break;

#line 1413 "./glsl.g"

case 136: {
    // ast(1) = new ...AST(...);
}   break;

#line 1420 "./glsl.g"

case 137: {
    // ast(1) = new ...AST(...);
}   break;

#line 1427 "./glsl.g"

case 138: {
    // ast(1) = new ...AST(...);
}   break;

#line 1434 "./glsl.g"

case 139: {
    // ast(1) = new ...AST(...);
}   break;

#line 1441 "./glsl.g"

case 140: {
    // ast(1) = new ...AST(...);
}   break;

#line 1448 "./glsl.g"

case 141: {
    // ast(1) = new ...AST(...);
}   break;

#line 1455 "./glsl.g"

case 142: {
    // ast(1) = new ...AST(...);
}   break;

#line 1462 "./glsl.g"

case 143: {
    // ast(1) = new ...AST(...);
}   break;

#line 1469 "./glsl.g"

case 144: {
    // ast(1) = new ...AST(...);
}   break;

#line 1476 "./glsl.g"

case 145: {
    // ast(1) = new ...AST(...);
}   break;

#line 1483 "./glsl.g"

case 146: {
    // ast(1) = new ...AST(...);
}   break;

#line 1490 "./glsl.g"

case 147: {
    // ast(1) = new ...AST(...);
}   break;

#line 1497 "./glsl.g"

case 148: {
    // ast(1) = new ...AST(...);
}   break;

#line 1504 "./glsl.g"

case 149: {
    // ast(1) = new ...AST(...);
}   break;

#line 1511 "./glsl.g"

case 150: {
    // ast(1) = new ...AST(...);
}   break;

#line 1518 "./glsl.g"

case 151: {
    // ast(1) = new ...AST(...);
}   break;

#line 1525 "./glsl.g"

case 152: {
    // ast(1) = new ...AST(...);
}   break;

#line 1532 "./glsl.g"

case 153: {
    // ast(1) = new ...AST(...);
}   break;

#line 1539 "./glsl.g"

case 154: {
    // ast(1) = new ...AST(...);
}   break;

#line 1546 "./glsl.g"

case 155: {
    // ast(1) = new ...AST(...);
}   break;

#line 1553 "./glsl.g"

case 156: {
    // ast(1) = new ...AST(...);
}   break;

#line 1560 "./glsl.g"

case 157: {
    // ast(1) = new ...AST(...);
}   break;

#line 1567 "./glsl.g"

case 158: {
    // ast(1) = new ...AST(...);
}   break;

#line 1574 "./glsl.g"

case 159: {
    // ast(1) = new ...AST(...);
}   break;

#line 1581 "./glsl.g"

case 160: {
    // ast(1) = new ...AST(...);
}   break;

#line 1588 "./glsl.g"

case 161: {
    // ast(1) = new ...AST(...);
}   break;

#line 1595 "./glsl.g"

case 162: {
    // ast(1) = new ...AST(...);
}   break;

#line 1602 "./glsl.g"

case 163: {
    // ast(1) = new ...AST(...);
}   break;

#line 1609 "./glsl.g"

case 164: {
    // ast(1) = new ...AST(...);
}   break;

#line 1616 "./glsl.g"

case 165: {
    // ast(1) = new ...AST(...);
}   break;

#line 1623 "./glsl.g"

case 166: {
    // ast(1) = new ...AST(...);
}   break;

#line 1630 "./glsl.g"

case 167: {
    // ast(1) = new ...AST(...);
}   break;

#line 1637 "./glsl.g"

case 168: {
    // ast(1) = new ...AST(...);
}   break;

#line 1644 "./glsl.g"

case 169: {
    // ast(1) = new ...AST(...);
}   break;

#line 1651 "./glsl.g"

case 170: {
    // ast(1) = new ...AST(...);
}   break;

#line 1658 "./glsl.g"

case 171: {
    // ast(1) = new ...AST(...);
}   break;

#line 1665 "./glsl.g"

case 172: {
    // ast(1) = new ...AST(...);
}   break;

#line 1672 "./glsl.g"

case 173: {
    // ast(1) = new ...AST(...);
}   break;

#line 1679 "./glsl.g"

case 174: {
    // ast(1) = new ...AST(...);
}   break;

#line 1686 "./glsl.g"

case 175: {
    // ast(1) = new ...AST(...);
}   break;

#line 1693 "./glsl.g"

case 176: {
    // ast(1) = new ...AST(...);
}   break;

#line 1700 "./glsl.g"

case 177: {
    // ast(1) = new ...AST(...);
}   break;

#line 1707 "./glsl.g"

case 178: {
    // ast(1) = new ...AST(...);
}   break;

#line 1714 "./glsl.g"

case 179: {
    // ast(1) = new ...AST(...);
}   break;

#line 1721 "./glsl.g"

case 180: {
    // ast(1) = new ...AST(...);
}   break;

#line 1728 "./glsl.g"

case 181: {
    // ast(1) = new ...AST(...);
}   break;

#line 1735 "./glsl.g"

case 182: {
    // ast(1) = new ...AST(...);
}   break;

#line 1742 "./glsl.g"

case 183: {
    // ast(1) = new ...AST(...);
}   break;

#line 1749 "./glsl.g"

case 184: {
    // ast(1) = new ...AST(...);
}   break;

#line 1756 "./glsl.g"

case 185: {
    // ast(1) = new ...AST(...);
}   break;

#line 1763 "./glsl.g"

case 186: {
    // ast(1) = new ...AST(...);
}   break;

#line 1770 "./glsl.g"

case 187: {
    // ast(1) = new ...AST(...);
}   break;

#line 1777 "./glsl.g"

case 188: {
    // ast(1) = new ...AST(...);
}   break;

#line 1784 "./glsl.g"

case 189: {
    // ast(1) = new ...AST(...);
}   break;

#line 1791 "./glsl.g"

case 190: {
    // ast(1) = new ...AST(...);
}   break;

#line 1798 "./glsl.g"

case 191: {
    // ast(1) = new ...AST(...);
}   break;

#line 1805 "./glsl.g"

case 192: {
    // ast(1) = new ...AST(...);
}   break;

#line 1812 "./glsl.g"

case 193: {
    // ast(1) = new ...AST(...);
}   break;

#line 1819 "./glsl.g"

case 194: {
    // ast(1) = new ...AST(...);
}   break;

#line 1826 "./glsl.g"

case 195: {
    // ast(1) = new ...AST(...);
}   break;

#line 1833 "./glsl.g"

case 196: {
    // ast(1) = new ...AST(...);
}   break;

#line 1840 "./glsl.g"

case 197: {
    // ast(1) = new ...AST(...);
}   break;

#line 1847 "./glsl.g"

case 198: {
    // ast(1) = new ...AST(...);
}   break;

#line 1854 "./glsl.g"

case 199: {
    // ast(1) = new ...AST(...);
}   break;

#line 1861 "./glsl.g"

case 200: {
    // ast(1) = new ...AST(...);
}   break;

#line 1868 "./glsl.g"

case 201: {
    // ast(1) = new ...AST(...);
}   break;

#line 1875 "./glsl.g"

case 202: {
    // ast(1) = new ...AST(...);
}   break;

#line 1882 "./glsl.g"

case 203: {
    // ast(1) = new ...AST(...);
}   break;

#line 1889 "./glsl.g"

case 204: {
    // ast(1) = new ...AST(...);
}   break;

#line 1896 "./glsl.g"

case 205: {
    // ast(1) = new ...AST(...);
}   break;

#line 1903 "./glsl.g"

case 206: {
    // ast(1) = new ...AST(...);
}   break;

#line 1910 "./glsl.g"

case 207: {
    // ast(1) = new ...AST(...);
}   break;

#line 1917 "./glsl.g"

case 208: {
    // ast(1) = new ...AST(...);
}   break;

#line 1924 "./glsl.g"

case 209: {
    // ast(1) = new ...AST(...);
}   break;

#line 1931 "./glsl.g"

case 210: {
    // ast(1) = new ...AST(...);
}   break;

#line 1938 "./glsl.g"

case 211: {
    // ast(1) = new ...AST(...);
}   break;

#line 1945 "./glsl.g"

case 212: {
    // ast(1) = new ...AST(...);
}   break;

#line 1952 "./glsl.g"

case 213: {
    // ast(1) = new ...AST(...);
}   break;

#line 1959 "./glsl.g"

case 214: {
    // ast(1) = new ...AST(...);
}   break;

#line 1966 "./glsl.g"

case 215: {
    // ast(1) = new ...AST(...);
}   break;

#line 1973 "./glsl.g"

case 216: {
    // ast(1) = new ...AST(...);
}   break;

#line 1980 "./glsl.g"

case 217: {
    // ast(1) = new ...AST(...);
}   break;

#line 1987 "./glsl.g"

case 218: {
    // ast(1) = new ...AST(...);
}   break;

#line 1994 "./glsl.g"

case 219: {
    // ast(1) = new ...AST(...);
}   break;

#line 2001 "./glsl.g"

case 220: {
    // ast(1) = new ...AST(...);
}   break;

#line 2008 "./glsl.g"

case 221: {
    // ast(1) = new ...AST(...);
}   break;

#line 2015 "./glsl.g"

case 222: {
    // ast(1) = new ...AST(...);
}   break;

#line 2022 "./glsl.g"

case 223: {
    // ast(1) = new ...AST(...);
}   break;

#line 2029 "./glsl.g"

case 224: {
    // ast(1) = new ...AST(...);
}   break;

#line 2036 "./glsl.g"

case 225: {
    // ast(1) = new ...AST(...);
}   break;

#line 2043 "./glsl.g"

case 226: {
    // ast(1) = new ...AST(...);
}   break;

#line 2050 "./glsl.g"

case 227: {
    // ast(1) = new ...AST(...);
}   break;

#line 2057 "./glsl.g"

case 228: {
    // ast(1) = new ...AST(...);
}   break;

#line 2064 "./glsl.g"

case 229: {
    // ast(1) = new ...AST(...);
}   break;

#line 2071 "./glsl.g"

case 230: {
    // ast(1) = new ...AST(...);
}   break;

#line 2078 "./glsl.g"

case 231: {
    // ast(1) = new ...AST(...);
}   break;

#line 2085 "./glsl.g"

case 232: {
    // ast(1) = new ...AST(...);
}   break;

#line 2092 "./glsl.g"

case 233: {
    // ast(1) = new ...AST(...);
}   break;

#line 2099 "./glsl.g"

case 234: {
    // ast(1) = new ...AST(...);
}   break;

#line 2106 "./glsl.g"

case 235: {
    // ast(1) = new ...AST(...);
}   break;

#line 2113 "./glsl.g"

case 236: {
    // ast(1) = new ...AST(...);
}   break;

#line 2120 "./glsl.g"

case 237: {
    // ast(1) = new ...AST(...);
}   break;

#line 2127 "./glsl.g"

case 238: {
    // ast(1) = new ...AST(...);
}   break;

#line 2134 "./glsl.g"

case 239: {
    // ast(1) = new ...AST(...);
}   break;

#line 2141 "./glsl.g"

case 240: {
    // ast(1) = new ...AST(...);
}   break;

#line 2148 "./glsl.g"

case 241: {
    // ast(1) = new ...AST(...);
}   break;

#line 2155 "./glsl.g"

case 242: {
    // ast(1) = new ...AST(...);
}   break;

#line 2162 "./glsl.g"

case 243: {
    // ast(1) = new ...AST(...);
}   break;

#line 2169 "./glsl.g"

case 244: {
    // ast(1) = new ...AST(...);
}   break;

#line 2176 "./glsl.g"

case 245: {
    // ast(1) = new ...AST(...);
}   break;

#line 2183 "./glsl.g"

case 246: {
    // ast(1) = new ...AST(...);
}   break;

#line 2190 "./glsl.g"

case 247: {
    // ast(1) = new ...AST(...);
}   break;

#line 2197 "./glsl.g"

case 248: {
    // ast(1) = new ...AST(...);
}   break;

#line 2204 "./glsl.g"

case 249: {
    // ast(1) = new ...AST(...);
}   break;

#line 2211 "./glsl.g"

case 250: {
    // ast(1) = new ...AST(...);
}   break;

#line 2218 "./glsl.g"

case 251: {
    // ast(1) = new ...AST(...);
}   break;

#line 2225 "./glsl.g"

case 252: {
    // ast(1) = new ...AST(...);
}   break;

#line 2232 "./glsl.g"

case 253: {
    // ast(1) = new ...AST(...);
}   break;

#line 2239 "./glsl.g"

case 254: {
    // ast(1) = new ...AST(...);
}   break;

#line 2246 "./glsl.g"

case 255: {
    // ast(1) = new ...AST(...);
}   break;

#line 2253 "./glsl.g"

case 256: {
    // ast(1) = new ...AST(...);
}   break;

#line 2260 "./glsl.g"

case 257: {
    // ast(1) = new ...AST(...);
}   break;

#line 2267 "./glsl.g"

case 258: {
    // ast(1) = new ...AST(...);
}   break;

#line 2274 "./glsl.g"

case 259: {
    // ast(1) = new ...AST(...);
}   break;

#line 2281 "./glsl.g"

case 260: {
    // ast(1) = new ...AST(...);
}   break;

#line 2288 "./glsl.g"

case 261: {
    // ast(1) = new ...AST(...);
}   break;

#line 2295 "./glsl.g"

case 262: {
    // ast(1) = new ...AST(...);
}   break;

#line 2302 "./glsl.g"

case 263: {
    // ast(1) = new ...AST(...);
}   break;

#line 2309 "./glsl.g"

case 264: {
    // ast(1) = new ...AST(...);
}   break;

#line 2316 "./glsl.g"

case 265: {
    // ast(1) = new ...AST(...);
}   break;

#line 2323 "./glsl.g"

case 266: {
    // ast(1) = new ...AST(...);
}   break;

#line 2330 "./glsl.g"

case 267: {
    // ast(1) = new ...AST(...);
}   break;

#line 2337 "./glsl.g"

case 268: {
    // ast(1) = new ...AST(...);
}   break;

#line 2344 "./glsl.g"

case 269: {
    // ast(1) = new ...AST(...);
}   break;

#line 2351 "./glsl.g"

case 270: {
    // ast(1) = new ...AST(...);
}   break;

#line 2358 "./glsl.g"

case 271: {
    // ast(1) = new ...AST(...);
}   break;

#line 2365 "./glsl.g"

case 272: {
    // ast(1) = new ...AST(...);
}   break;

#line 2372 "./glsl.g"

case 273: {
    // ast(1) = new ...AST(...);
}   break;

#line 2379 "./glsl.g"

case 274: {
    // ast(1) = new ...AST(...);
}   break;

#line 2386 "./glsl.g"

case 275: {
    // ast(1) = new ...AST(...);
}   break;

#line 2393 "./glsl.g"

case 276: {
    // ast(1) = new ...AST(...);
}   break;

#line 2400 "./glsl.g"

case 277: {
    // ast(1) = new ...AST(...);
}   break;

#line 2407 "./glsl.g"

case 278: {
    // ast(1) = new ...AST(...);
}   break;

#line 2414 "./glsl.g"

case 279: {
    // ast(1) = new ...AST(...);
}   break;

#line 2421 "./glsl.g"

case 280: {
    // ast(1) = new ...AST(...);
}   break;

#line 2428 "./glsl.g"

case 281: {
    // ast(1) = new ...AST(...);
}   break;

#line 2435 "./glsl.g"

case 282: {
    // ast(1) = new ...AST(...);
}   break;

#line 2442 "./glsl.g"

case 283: {
    // ast(1) = new ...AST(...);
}   break;

#line 2449 "./glsl.g"

case 284: {
    // ast(1) = new ...AST(...);
}   break;

#line 2456 "./glsl.g"

case 285: {
    // ast(1) = new ...AST(...);
}   break;

#line 2463 "./glsl.g"

case 286: {
    // ast(1) = new ...AST(...);
}   break;

#line 2470 "./glsl.g"

case 287: {
    // ast(1) = new ...AST(...);
}   break;

#line 2477 "./glsl.g"

case 288: {
    // ast(1) = new ...AST(...);
}   break;

#line 2484 "./glsl.g"

case 289: {
    // ast(1) = new ...AST(...);
}   break;

#line 2491 "./glsl.g"

case 290: {
    // ast(1) = new ...AST(...);
}   break;

#line 2498 "./glsl.g"

case 291: {
    // ast(1) = new ...AST(...);
}   break;

#line 2505 "./glsl.g"

case 292: {
    // ast(1) = new ...AST(...);
}   break;

#line 2512 "./glsl.g"

case 293: {
    // ast(1) = new ...AST(...);
}   break;

#line 2519 "./glsl.g"

case 294: {
    // ast(1) = new ...AST(...);
}   break;

#line 2526 "./glsl.g"

case 295: {
    // ast(1) = new ...AST(...);
}   break;

#line 2533 "./glsl.g"

case 296: {
    // ast(1) = new ...AST(...);
}   break;

#line 2540 "./glsl.g"

case 297: {
    // ast(1) = new ...AST(...);
}   break;

#line 2547 "./glsl.g"

case 298: {
    // ast(1) = new ...AST(...);
}   break;

#line 2554 "./glsl.g"

case 299: {
    // ast(1) = new ...AST(...);
}   break;

#line 2561 "./glsl.g"

case 300: {
    // ast(1) = new ...AST(...);
}   break;

#line 2568 "./glsl.g"

case 301: {
    // ast(1) = new ...AST(...);
}   break;

#line 2575 "./glsl.g"

case 302: {
    // ast(1) = new ...AST(...);
}   break;

#line 2582 "./glsl.g"

case 303: {
    // ast(1) = new ...AST(...);
}   break;

#line 2589 "./glsl.g"

case 304: {
    // ast(1) = new ...AST(...);
}   break;

#line 2596 "./glsl.g"

case 305: {
    ast(1) = new TranslationUnit(sym(1).declaration_list);
}   break;

#line 2603 "./glsl.g"

case 306: {
    sym(1).declaration_list = new List<Declaration *>(sym(1).declaration);
}   break;

#line 2610 "./glsl.g"

case 307: {
    sym(1).declaration_list = new List<Declaration *>(sym(1).declaration_list, sym(2).declaration);
}   break;

#line 2617 "./glsl.g"

case 308: {
    // ast(1) = new ...AST(...);
}   break;

#line 2624 "./glsl.g"

case 309: {
    // ast(1) = new ...AST(...);
}   break;

#line 2631 "./glsl.g"

case 310: {
    // ast(1) = new ...AST(...);
}   break;

#line 2638 "./glsl.g"

case 311: {
    // ast(1) = new ...AST(...);
}   break;

#line 2645 "./glsl.g"

case 312: {
    ast(1) = 0;
}   break;

#line 2653 "./glsl.g"

} // end switch
} // end Parser::reduce()
