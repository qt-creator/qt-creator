
#line 292 "./glsl.g"

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
    : _engine(engine), _tos(-1), _index(0)
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

#line 454 "./glsl.g"

void Parser::reduce(int ruleno)
{
switch(ruleno) {

#line 463 "./glsl.g"

case 0: {
    ast(1) = new IdentifierExpression(sym(1).string);
}   break;

#line 470 "./glsl.g"

case 1: {
    // ast(1) = new ...AST(...);
}   break;

#line 477 "./glsl.g"

case 2: {
    // ast(1) = new ...AST(...);
}   break;

#line 484 "./glsl.g"

case 3: {
    // ast(1) = new ...AST(...);
}   break;

#line 491 "./glsl.g"

case 4: {
    // ast(1) = new ...AST(...);
}   break;

#line 498 "./glsl.g"

case 5: {
    // ast(1) = new ...AST(...);
}   break;

#line 505 "./glsl.g"

case 6: {
    // ast(1) = new ...AST(...);
}   break;

#line 512 "./glsl.g"

case 7: {
    // ast(1) = new ...AST(...);
}   break;

#line 519 "./glsl.g"

case 8: {
    // nothing to do.
}   break;

#line 526 "./glsl.g"

case 9: {
    // ast(1) = new ...AST(...);
}   break;

#line 533 "./glsl.g"

case 10: {
    // ast(1) = new ...AST(...);
}   break;

#line 540 "./glsl.g"

case 11: {
    // ast(1) = new ...AST(...);
}   break;

#line 547 "./glsl.g"

case 12: {
    // nothing to do.
}   break;

#line 554 "./glsl.g"

case 13: {
    // nothing to do.
}   break;

#line 561 "./glsl.g"

case 14: {
    // nothing to do.
}   break;

#line 568 "./glsl.g"

case 15: {
    // ast(1) = new ...AST(...);
}   break;

#line 575 "./glsl.g"

case 16: {
    // ast(1) = new ...AST(...);
}   break;

#line 582 "./glsl.g"

case 17: {
    // ast(1) = new ...AST(...);
}   break;

#line 589 "./glsl.g"

case 18: {
    // ast(1) = new ...AST(...);
}   break;

#line 596 "./glsl.g"

case 19: {
    // ast(1) = new ...AST(...);
}   break;

#line 603 "./glsl.g"

case 20: {
    // ast(1) = new ...AST(...);
}   break;

#line 610 "./glsl.g"

case 21: {
    // ast(1) = new ...AST(...);
}   break;

#line 617 "./glsl.g"

case 22: {
    // ast(1) = new ...AST(...);
}   break;

#line 624 "./glsl.g"

case 23: {
    // ast(1) = new ...AST(...);
}   break;

#line 631 "./glsl.g"

case 24: {
    // ast(1) = new ...AST(...);
}   break;

#line 638 "./glsl.g"

case 25: {
    // nothing to do.
}   break;

#line 645 "./glsl.g"

case 26: {
    // ast(1) = new ...AST(...);
}   break;

#line 652 "./glsl.g"

case 27: {
    // ast(1) = new ...AST(...);
}   break;

#line 659 "./glsl.g"

case 28: {
    // ast(1) = new ...AST(...);
}   break;

#line 666 "./glsl.g"

case 29: {
    // ast(1) = new ...AST(...);
}   break;

#line 673 "./glsl.g"

case 30: {
    // ast(1) = new ...AST(...);
}   break;

#line 680 "./glsl.g"

case 31: {
    // ast(1) = new ...AST(...);
}   break;

#line 687 "./glsl.g"

case 32: {
    // ast(1) = new ...AST(...);
}   break;

#line 694 "./glsl.g"

case 33: {
    // nothing to do.
}   break;

#line 701 "./glsl.g"

case 34: {
    // ast(1) = new ...AST(...);
}   break;

#line 708 "./glsl.g"

case 35: {
    // ast(1) = new ...AST(...);
}   break;

#line 715 "./glsl.g"

case 36: {
    // ast(1) = new ...AST(...);
}   break;

#line 722 "./glsl.g"

case 37: {
    // nothing to do.
}   break;

#line 729 "./glsl.g"

case 38: {
    // ast(1) = new ...AST(...);
}   break;

#line 736 "./glsl.g"

case 39: {
    // ast(1) = new ...AST(...);
}   break;

#line 743 "./glsl.g"

case 40: {
    // nothing to do.
}   break;

#line 750 "./glsl.g"

case 41: {
    // ast(1) = new ...AST(...);
}   break;

#line 757 "./glsl.g"

case 42: {
    // ast(1) = new ...AST(...);
}   break;

#line 764 "./glsl.g"

case 43: {
    // nothing to do.
}   break;

#line 771 "./glsl.g"

case 44: {
    // ast(1) = new ...AST(...);
}   break;

#line 778 "./glsl.g"

case 45: {
    // ast(1) = new ...AST(...);
}   break;

#line 785 "./glsl.g"

case 46: {
    // ast(1) = new ...AST(...);
}   break;

#line 792 "./glsl.g"

case 47: {
    // ast(1) = new ...AST(...);
}   break;

#line 799 "./glsl.g"

case 48: {
    // nothing to do.
}   break;

#line 806 "./glsl.g"

case 49: {
    // ast(1) = new ...AST(...);
}   break;

#line 813 "./glsl.g"

case 50: {
    // ast(1) = new ...AST(...);
}   break;

#line 820 "./glsl.g"

case 51: {
    // nothing to do.
}   break;

#line 827 "./glsl.g"

case 52: {
    // ast(1) = new ...AST(...);
}   break;

#line 834 "./glsl.g"

case 53: {
    // nothing to do.
}   break;

#line 841 "./glsl.g"

case 54: {
    // ast(1) = new ...AST(...);
}   break;

#line 848 "./glsl.g"

case 55: {
    // nothing to do.
}   break;

#line 855 "./glsl.g"

case 56: {
    // ast(1) = new ...AST(...);
}   break;

#line 862 "./glsl.g"

case 57: {
    // nothing to do.
}   break;

#line 869 "./glsl.g"

case 58: {
    // ast(1) = new ...AST(...);
}   break;

#line 876 "./glsl.g"

case 59: {
    // nothing to do.
}   break;

#line 883 "./glsl.g"

case 60: {
    // ast(1) = new ...AST(...);
}   break;

#line 890 "./glsl.g"

case 61: {
    // nothing to do.
}   break;

#line 897 "./glsl.g"

case 62: {
    // ast(1) = new ...AST(...);
}   break;

#line 904 "./glsl.g"

case 63: {
    // nothing to do.
}   break;

#line 911 "./glsl.g"

case 64: {
    // ast(1) = new ...AST(...);
}   break;

#line 918 "./glsl.g"

case 65: {
    // nothing to do.
}   break;

#line 925 "./glsl.g"

case 66: {
    // ast(1) = new ...AST(...);
}   break;

#line 932 "./glsl.g"

case 67: {
    // ast(1) = new ...AST(...);
}   break;

#line 939 "./glsl.g"

case 68: {
    // ast(1) = new ...AST(...);
}   break;

#line 946 "./glsl.g"

case 69: {
    // ast(1) = new ...AST(...);
}   break;

#line 953 "./glsl.g"

case 70: {
    // ast(1) = new ...AST(...);
}   break;

#line 960 "./glsl.g"

case 71: {
    // ast(1) = new ...AST(...);
}   break;

#line 967 "./glsl.g"

case 72: {
    // ast(1) = new ...AST(...);
}   break;

#line 974 "./glsl.g"

case 73: {
    // ast(1) = new ...AST(...);
}   break;

#line 981 "./glsl.g"

case 74: {
    // ast(1) = new ...AST(...);
}   break;

#line 988 "./glsl.g"

case 75: {
    // ast(1) = new ...AST(...);
}   break;

#line 995 "./glsl.g"

case 76: {
    // ast(1) = new ...AST(...);
}   break;

#line 1002 "./glsl.g"

case 77: {
    // ast(1) = new ...AST(...);
}   break;

#line 1009 "./glsl.g"

case 78: {
    // nothing to do.
}   break;

#line 1016 "./glsl.g"

case 79: {
    // ast(1) = new ...AST(...);
}   break;

#line 1023 "./glsl.g"

case 80: {
    // nothing to do.
}   break;

#line 1030 "./glsl.g"

case 81: {
    // ast(1) = new ...AST(...);
}   break;

#line 1037 "./glsl.g"

case 82: {
    // ast(1) = new ...AST(...);
}   break;

#line 1044 "./glsl.g"

case 83: {
    // ast(1) = new ...AST(...);
}   break;

#line 1051 "./glsl.g"

case 84: {
    // ast(1) = new ...AST(...);
}   break;

#line 1058 "./glsl.g"

case 85: {
    // ast(1) = new ...AST(...);
}   break;

#line 1065 "./glsl.g"

case 86: {
    // ast(1) = new ...AST(...);
}   break;

#line 1072 "./glsl.g"

case 87: {
    // ast(1) = new ...AST(...);
}   break;

#line 1079 "./glsl.g"

case 88: {
    // ast(1) = new ...AST(...);
}   break;

#line 1086 "./glsl.g"

case 89: {
    // ast(1) = new ...AST(...);
}   break;

#line 1093 "./glsl.g"

case 90: {
    // ast(1) = new ...AST(...);
}   break;

#line 1100 "./glsl.g"

case 91: {
    // ast(1) = new ...AST(...);
}   break;

#line 1107 "./glsl.g"

case 92: {
    // ast(1) = new ...AST(...);
}   break;

#line 1114 "./glsl.g"

case 93: {
    // ast(1) = new ...AST(...);
}   break;

#line 1121 "./glsl.g"

case 94: {
    // ast(1) = new ...AST(...);
}   break;

#line 1128 "./glsl.g"

case 95: {
    // ast(1) = new ...AST(...);
}   break;

#line 1135 "./glsl.g"

case 96: {
    // ast(1) = new ...AST(...);
}   break;

#line 1142 "./glsl.g"

case 97: {
    // ast(1) = new ...AST(...);
}   break;

#line 1149 "./glsl.g"

case 98: {
    // ast(1) = new ...AST(...);
}   break;

#line 1156 "./glsl.g"

case 99: {
    // ast(1) = new ...AST(...);
}   break;

#line 1163 "./glsl.g"

case 100: {
    // ast(1) = new ...AST(...);
}   break;

#line 1170 "./glsl.g"

case 101: {
    // ast(1) = new ...AST(...);
}   break;

#line 1177 "./glsl.g"

case 102: {
    // ast(1) = new ...AST(...);
}   break;

#line 1184 "./glsl.g"

case 103: {
    // ast(1) = new ...AST(...);
}   break;

#line 1191 "./glsl.g"

case 104: {
    // ast(1) = new ...AST(...);
}   break;

#line 1198 "./glsl.g"

case 105: {
    // ast(1) = new ...AST(...);
}   break;

#line 1205 "./glsl.g"

case 106: {
    // ast(1) = new ...AST(...);
}   break;

#line 1212 "./glsl.g"

case 107: {
    // ast(1) = new ...AST(...);
}   break;

#line 1219 "./glsl.g"

case 108: {
    // ast(1) = new ...AST(...);
}   break;

#line 1226 "./glsl.g"

case 109: {
    // ast(1) = new ...AST(...);
}   break;

#line 1233 "./glsl.g"

case 110: {
    // ast(1) = new ...AST(...);
}   break;

#line 1240 "./glsl.g"

case 111: {
    // ast(1) = new ...AST(...);
}   break;

#line 1247 "./glsl.g"

case 112: {
    // ast(1) = new ...AST(...);
}   break;

#line 1254 "./glsl.g"

case 113: {
    // ast(1) = new ...AST(...);
}   break;

#line 1261 "./glsl.g"

case 114: {
    // ast(1) = new ...AST(...);
}   break;

#line 1268 "./glsl.g"

case 115: {
    // ast(1) = new ...AST(...);
}   break;

#line 1275 "./glsl.g"

case 116: {
    // ast(1) = new ...AST(...);
}   break;

#line 1282 "./glsl.g"

case 117: {
    // ast(1) = new ...AST(...);
}   break;

#line 1289 "./glsl.g"

case 118: {
    // ast(1) = new ...AST(...);
}   break;

#line 1296 "./glsl.g"

case 119: {
    // ast(1) = new ...AST(...);
}   break;

#line 1303 "./glsl.g"

case 120: {
    // ast(1) = new ...AST(...);
}   break;

#line 1310 "./glsl.g"

case 121: {
    // ast(1) = new ...AST(...);
}   break;

#line 1317 "./glsl.g"

case 122: {
    // ast(1) = new ...AST(...);
}   break;

#line 1324 "./glsl.g"

case 123: {
    // ast(1) = new ...AST(...);
}   break;

#line 1331 "./glsl.g"

case 124: {
    // ast(1) = new ...AST(...);
}   break;

#line 1338 "./glsl.g"

case 125: {
    // ast(1) = new ...AST(...);
}   break;

#line 1345 "./glsl.g"

case 126: {
    // ast(1) = new ...AST(...);
}   break;

#line 1352 "./glsl.g"

case 127: {
    // ast(1) = new ...AST(...);
}   break;

#line 1359 "./glsl.g"

case 128: {
    // ast(1) = new ...AST(...);
}   break;

#line 1366 "./glsl.g"

case 129: {
    // ast(1) = new ...AST(...);
}   break;

#line 1373 "./glsl.g"

case 130: {
    // ast(1) = new ...AST(...);
}   break;

#line 1380 "./glsl.g"

case 131: {
    // ast(1) = new ...AST(...);
}   break;

#line 1387 "./glsl.g"

case 132: {
    // ast(1) = new ...AST(...);
}   break;

#line 1394 "./glsl.g"

case 133: {
    // ast(1) = new ...AST(...);
}   break;

#line 1401 "./glsl.g"

case 134: {
    // ast(1) = new ...AST(...);
}   break;

#line 1408 "./glsl.g"

case 135: {
    // ast(1) = new ...AST(...);
}   break;

#line 1415 "./glsl.g"

case 136: {
    // ast(1) = new ...AST(...);
}   break;

#line 1422 "./glsl.g"

case 137: {
    // ast(1) = new ...AST(...);
}   break;

#line 1429 "./glsl.g"

case 138: {
    // ast(1) = new ...AST(...);
}   break;

#line 1436 "./glsl.g"

case 139: {
    // ast(1) = new ...AST(...);
}   break;

#line 1443 "./glsl.g"

case 140: {
    // ast(1) = new ...AST(...);
}   break;

#line 1450 "./glsl.g"

case 141: {
    // ast(1) = new ...AST(...);
}   break;

#line 1457 "./glsl.g"

case 142: {
    // ast(1) = new ...AST(...);
}   break;

#line 1464 "./glsl.g"

case 143: {
    // ast(1) = new ...AST(...);
}   break;

#line 1471 "./glsl.g"

case 144: {
    // ast(1) = new ...AST(...);
}   break;

#line 1478 "./glsl.g"

case 145: {
    // ast(1) = new ...AST(...);
}   break;

#line 1485 "./glsl.g"

case 146: {
    // ast(1) = new ...AST(...);
}   break;

#line 1492 "./glsl.g"

case 147: {
    // ast(1) = new ...AST(...);
}   break;

#line 1499 "./glsl.g"

case 148: {
    // ast(1) = new ...AST(...);
}   break;

#line 1506 "./glsl.g"

case 149: {
    // ast(1) = new ...AST(...);
}   break;

#line 1513 "./glsl.g"

case 150: {
    // ast(1) = new ...AST(...);
}   break;

#line 1520 "./glsl.g"

case 151: {
    // ast(1) = new ...AST(...);
}   break;

#line 1527 "./glsl.g"

case 152: {
    // ast(1) = new ...AST(...);
}   break;

#line 1534 "./glsl.g"

case 153: {
    // ast(1) = new ...AST(...);
}   break;

#line 1541 "./glsl.g"

case 154: {
    // ast(1) = new ...AST(...);
}   break;

#line 1548 "./glsl.g"

case 155: {
    // ast(1) = new ...AST(...);
}   break;

#line 1555 "./glsl.g"

case 156: {
    // ast(1) = new ...AST(...);
}   break;

#line 1562 "./glsl.g"

case 157: {
    // ast(1) = new ...AST(...);
}   break;

#line 1569 "./glsl.g"

case 158: {
    // ast(1) = new ...AST(...);
}   break;

#line 1576 "./glsl.g"

case 159: {
    // ast(1) = new ...AST(...);
}   break;

#line 1583 "./glsl.g"

case 160: {
    // ast(1) = new ...AST(...);
}   break;

#line 1590 "./glsl.g"

case 161: {
    // ast(1) = new ...AST(...);
}   break;

#line 1597 "./glsl.g"

case 162: {
    // ast(1) = new ...AST(...);
}   break;

#line 1604 "./glsl.g"

case 163: {
    // ast(1) = new ...AST(...);
}   break;

#line 1611 "./glsl.g"

case 164: {
    // ast(1) = new ...AST(...);
}   break;

#line 1618 "./glsl.g"

case 165: {
    // ast(1) = new ...AST(...);
}   break;

#line 1625 "./glsl.g"

case 166: {
    // ast(1) = new ...AST(...);
}   break;

#line 1632 "./glsl.g"

case 167: {
    // ast(1) = new ...AST(...);
}   break;

#line 1639 "./glsl.g"

case 168: {
    // ast(1) = new ...AST(...);
}   break;

#line 1646 "./glsl.g"

case 169: {
    // ast(1) = new ...AST(...);
}   break;

#line 1653 "./glsl.g"

case 170: {
    // ast(1) = new ...AST(...);
}   break;

#line 1660 "./glsl.g"

case 171: {
    // ast(1) = new ...AST(...);
}   break;

#line 1667 "./glsl.g"

case 172: {
    // ast(1) = new ...AST(...);
}   break;

#line 1674 "./glsl.g"

case 173: {
    // ast(1) = new ...AST(...);
}   break;

#line 1681 "./glsl.g"

case 174: {
    // ast(1) = new ...AST(...);
}   break;

#line 1688 "./glsl.g"

case 175: {
    // ast(1) = new ...AST(...);
}   break;

#line 1695 "./glsl.g"

case 176: {
    // ast(1) = new ...AST(...);
}   break;

#line 1702 "./glsl.g"

case 177: {
    // ast(1) = new ...AST(...);
}   break;

#line 1709 "./glsl.g"

case 178: {
    // ast(1) = new ...AST(...);
}   break;

#line 1716 "./glsl.g"

case 179: {
    // ast(1) = new ...AST(...);
}   break;

#line 1723 "./glsl.g"

case 180: {
    // ast(1) = new ...AST(...);
}   break;

#line 1730 "./glsl.g"

case 181: {
    // ast(1) = new ...AST(...);
}   break;

#line 1737 "./glsl.g"

case 182: {
    // ast(1) = new ...AST(...);
}   break;

#line 1744 "./glsl.g"

case 183: {
    // ast(1) = new ...AST(...);
}   break;

#line 1751 "./glsl.g"

case 184: {
    // ast(1) = new ...AST(...);
}   break;

#line 1758 "./glsl.g"

case 185: {
    // ast(1) = new ...AST(...);
}   break;

#line 1765 "./glsl.g"

case 186: {
    // ast(1) = new ...AST(...);
}   break;

#line 1772 "./glsl.g"

case 187: {
    // ast(1) = new ...AST(...);
}   break;

#line 1779 "./glsl.g"

case 188: {
    // ast(1) = new ...AST(...);
}   break;

#line 1786 "./glsl.g"

case 189: {
    // ast(1) = new ...AST(...);
}   break;

#line 1793 "./glsl.g"

case 190: {
    // ast(1) = new ...AST(...);
}   break;

#line 1800 "./glsl.g"

case 191: {
    // ast(1) = new ...AST(...);
}   break;

#line 1807 "./glsl.g"

case 192: {
    // ast(1) = new ...AST(...);
}   break;

#line 1814 "./glsl.g"

case 193: {
    // ast(1) = new ...AST(...);
}   break;

#line 1821 "./glsl.g"

case 194: {
    // ast(1) = new ...AST(...);
}   break;

#line 1828 "./glsl.g"

case 195: {
    // ast(1) = new ...AST(...);
}   break;

#line 1835 "./glsl.g"

case 196: {
    // ast(1) = new ...AST(...);
}   break;

#line 1842 "./glsl.g"

case 197: {
    // ast(1) = new ...AST(...);
}   break;

#line 1849 "./glsl.g"

case 198: {
    // ast(1) = new ...AST(...);
}   break;

#line 1856 "./glsl.g"

case 199: {
    // ast(1) = new ...AST(...);
}   break;

#line 1863 "./glsl.g"

case 200: {
    // ast(1) = new ...AST(...);
}   break;

#line 1870 "./glsl.g"

case 201: {
    // ast(1) = new ...AST(...);
}   break;

#line 1877 "./glsl.g"

case 202: {
    // ast(1) = new ...AST(...);
}   break;

#line 1884 "./glsl.g"

case 203: {
    // ast(1) = new ...AST(...);
}   break;

#line 1891 "./glsl.g"

case 204: {
    // ast(1) = new ...AST(...);
}   break;

#line 1898 "./glsl.g"

case 205: {
    // ast(1) = new ...AST(...);
}   break;

#line 1905 "./glsl.g"

case 206: {
    // ast(1) = new ...AST(...);
}   break;

#line 1912 "./glsl.g"

case 207: {
    // ast(1) = new ...AST(...);
}   break;

#line 1919 "./glsl.g"

case 208: {
    // ast(1) = new ...AST(...);
}   break;

#line 1926 "./glsl.g"

case 209: {
    // ast(1) = new ...AST(...);
}   break;

#line 1933 "./glsl.g"

case 210: {
    // ast(1) = new ...AST(...);
}   break;

#line 1940 "./glsl.g"

case 211: {
    // ast(1) = new ...AST(...);
}   break;

#line 1947 "./glsl.g"

case 212: {
    // ast(1) = new ...AST(...);
}   break;

#line 1954 "./glsl.g"

case 213: {
    // ast(1) = new ...AST(...);
}   break;

#line 1961 "./glsl.g"

case 214: {
    // ast(1) = new ...AST(...);
}   break;

#line 1968 "./glsl.g"

case 215: {
    // ast(1) = new ...AST(...);
}   break;

#line 1975 "./glsl.g"

case 216: {
    // ast(1) = new ...AST(...);
}   break;

#line 1982 "./glsl.g"

case 217: {
    // ast(1) = new ...AST(...);
}   break;

#line 1989 "./glsl.g"

case 218: {
    // ast(1) = new ...AST(...);
}   break;

#line 1996 "./glsl.g"

case 219: {
    // ast(1) = new ...AST(...);
}   break;

#line 2003 "./glsl.g"

case 220: {
    // ast(1) = new ...AST(...);
}   break;

#line 2010 "./glsl.g"

case 221: {
    // ast(1) = new ...AST(...);
}   break;

#line 2017 "./glsl.g"

case 222: {
    // ast(1) = new ...AST(...);
}   break;

#line 2024 "./glsl.g"

case 223: {
    // ast(1) = new ...AST(...);
}   break;

#line 2031 "./glsl.g"

case 224: {
    // ast(1) = new ...AST(...);
}   break;

#line 2038 "./glsl.g"

case 225: {
    // ast(1) = new ...AST(...);
}   break;

#line 2045 "./glsl.g"

case 226: {
    // ast(1) = new ...AST(...);
}   break;

#line 2052 "./glsl.g"

case 227: {
    // ast(1) = new ...AST(...);
}   break;

#line 2059 "./glsl.g"

case 228: {
    // ast(1) = new ...AST(...);
}   break;

#line 2066 "./glsl.g"

case 229: {
    // ast(1) = new ...AST(...);
}   break;

#line 2073 "./glsl.g"

case 230: {
    // ast(1) = new ...AST(...);
}   break;

#line 2080 "./glsl.g"

case 231: {
    // ast(1) = new ...AST(...);
}   break;

#line 2087 "./glsl.g"

case 232: {
    // ast(1) = new ...AST(...);
}   break;

#line 2094 "./glsl.g"

case 233: {
    // ast(1) = new ...AST(...);
}   break;

#line 2101 "./glsl.g"

case 234: {
    // ast(1) = new ...AST(...);
}   break;

#line 2108 "./glsl.g"

case 235: {
    // ast(1) = new ...AST(...);
}   break;

#line 2115 "./glsl.g"

case 236: {
    // ast(1) = new ...AST(...);
}   break;

#line 2122 "./glsl.g"

case 237: {
    // ast(1) = new ...AST(...);
}   break;

#line 2129 "./glsl.g"

case 238: {
    // ast(1) = new ...AST(...);
}   break;

#line 2136 "./glsl.g"

case 239: {
    // ast(1) = new ...AST(...);
}   break;

#line 2143 "./glsl.g"

case 240: {
    // ast(1) = new ...AST(...);
}   break;

#line 2150 "./glsl.g"

case 241: {
    // ast(1) = new ...AST(...);
}   break;

#line 2157 "./glsl.g"

case 242: {
    // ast(1) = new ...AST(...);
}   break;

#line 2164 "./glsl.g"

case 243: {
    // ast(1) = new ...AST(...);
}   break;

#line 2171 "./glsl.g"

case 244: {
    // ast(1) = new ...AST(...);
}   break;

#line 2178 "./glsl.g"

case 245: {
    // ast(1) = new ...AST(...);
}   break;

#line 2185 "./glsl.g"

case 246: {
    // ast(1) = new ...AST(...);
}   break;

#line 2192 "./glsl.g"

case 247: {
    // ast(1) = new ...AST(...);
}   break;

#line 2199 "./glsl.g"

case 248: {
    // ast(1) = new ...AST(...);
}   break;

#line 2206 "./glsl.g"

case 249: {
    // ast(1) = new ...AST(...);
}   break;

#line 2213 "./glsl.g"

case 250: {
    // ast(1) = new ...AST(...);
}   break;

#line 2220 "./glsl.g"

case 251: {
    // ast(1) = new ...AST(...);
}   break;

#line 2227 "./glsl.g"

case 252: {
    // ast(1) = new ...AST(...);
}   break;

#line 2234 "./glsl.g"

case 253: {
    // ast(1) = new ...AST(...);
}   break;

#line 2241 "./glsl.g"

case 254: {
    // ast(1) = new ...AST(...);
}   break;

#line 2248 "./glsl.g"

case 255: {
    // ast(1) = new ...AST(...);
}   break;

#line 2255 "./glsl.g"

case 256: {
    // ast(1) = new ...AST(...);
}   break;

#line 2262 "./glsl.g"

case 257: {
    // ast(1) = new ...AST(...);
}   break;

#line 2269 "./glsl.g"

case 258: {
    // ast(1) = new ...AST(...);
}   break;

#line 2276 "./glsl.g"

case 259: {
    // ast(1) = new ...AST(...);
}   break;

#line 2283 "./glsl.g"

case 260: {
    // ast(1) = new ...AST(...);
}   break;

#line 2290 "./glsl.g"

case 261: {
    // ast(1) = new ...AST(...);
}   break;

#line 2297 "./glsl.g"

case 262: {
    // ast(1) = new ...AST(...);
}   break;

#line 2304 "./glsl.g"

case 263: {
    // ast(1) = new ...AST(...);
}   break;

#line 2311 "./glsl.g"

case 264: {
    // ast(1) = new ...AST(...);
}   break;

#line 2318 "./glsl.g"

case 265: {
    // ast(1) = new ...AST(...);
}   break;

#line 2325 "./glsl.g"

case 266: {
    // ast(1) = new ...AST(...);
}   break;

#line 2332 "./glsl.g"

case 267: {
    // ast(1) = new ...AST(...);
}   break;

#line 2339 "./glsl.g"

case 268: {
    // ast(1) = new ...AST(...);
}   break;

#line 2346 "./glsl.g"

case 269: {
    // ast(1) = new ...AST(...);
}   break;

#line 2353 "./glsl.g"

case 270: {
    // ast(1) = new ...AST(...);
}   break;

#line 2360 "./glsl.g"

case 271: {
    // ast(1) = new ...AST(...);
}   break;

#line 2367 "./glsl.g"

case 272: {
    // ast(1) = new ...AST(...);
}   break;

#line 2374 "./glsl.g"

case 273: {
    // ast(1) = new ...AST(...);
}   break;

#line 2381 "./glsl.g"

case 274: {
    // ast(1) = new ...AST(...);
}   break;

#line 2388 "./glsl.g"

case 275: {
    // ast(1) = new ...AST(...);
}   break;

#line 2395 "./glsl.g"

case 276: {
    // ast(1) = new ...AST(...);
}   break;

#line 2402 "./glsl.g"

case 277: {
    // ast(1) = new ...AST(...);
}   break;

#line 2409 "./glsl.g"

case 278: {
    // ast(1) = new ...AST(...);
}   break;

#line 2416 "./glsl.g"

case 279: {
    // ast(1) = new ...AST(...);
}   break;

#line 2423 "./glsl.g"

case 280: {
    // ast(1) = new ...AST(...);
}   break;

#line 2430 "./glsl.g"

case 281: {
    // ast(1) = new ...AST(...);
}   break;

#line 2437 "./glsl.g"

case 282: {
    // ast(1) = new ...AST(...);
}   break;

#line 2444 "./glsl.g"

case 283: {
    // ast(1) = new ...AST(...);
}   break;

#line 2451 "./glsl.g"

case 284: {
    // ast(1) = new ...AST(...);
}   break;

#line 2458 "./glsl.g"

case 285: {
    // ast(1) = new ...AST(...);
}   break;

#line 2465 "./glsl.g"

case 286: {
    // ast(1) = new ...AST(...);
}   break;

#line 2472 "./glsl.g"

case 287: {
    // ast(1) = new ...AST(...);
}   break;

#line 2479 "./glsl.g"

case 288: {
    // ast(1) = new ...AST(...);
}   break;

#line 2486 "./glsl.g"

case 289: {
    // ast(1) = new ...AST(...);
}   break;

#line 2493 "./glsl.g"

case 290: {
    // ast(1) = new ...AST(...);
}   break;

#line 2500 "./glsl.g"

case 291: {
    // ast(1) = new ...AST(...);
}   break;

#line 2507 "./glsl.g"

case 292: {
    // ast(1) = new ...AST(...);
}   break;

#line 2514 "./glsl.g"

case 293: {
    // ast(1) = new ...AST(...);
}   break;

#line 2521 "./glsl.g"

case 294: {
    // ast(1) = new ...AST(...);
}   break;

#line 2528 "./glsl.g"

case 295: {
    // ast(1) = new ...AST(...);
}   break;

#line 2535 "./glsl.g"

case 296: {
    // ast(1) = new ...AST(...);
}   break;

#line 2542 "./glsl.g"

case 297: {
    // ast(1) = new ...AST(...);
}   break;

#line 2549 "./glsl.g"

case 298: {
    // ast(1) = new ...AST(...);
}   break;

#line 2556 "./glsl.g"

case 299: {
    // ast(1) = new ...AST(...);
}   break;

#line 2563 "./glsl.g"

case 300: {
    // ast(1) = new ...AST(...);
}   break;

#line 2570 "./glsl.g"

case 301: {
    // ast(1) = new ...AST(...);
}   break;

#line 2577 "./glsl.g"

case 302: {
    // ast(1) = new ...AST(...);
}   break;

#line 2584 "./glsl.g"

case 303: {
    // ast(1) = new ...AST(...);
}   break;

#line 2591 "./glsl.g"

case 304: {
    // ast(1) = new ...AST(...);
}   break;

#line 2598 "./glsl.g"

case 305: {
    ast(1) = new TranslationUnit(sym(1).declaration_list);
}   break;

#line 2605 "./glsl.g"

case 306: {
    sym(1).declaration_list = new (_engine->pool()) List<Declaration *>(sym(1).declaration);
}   break;

#line 2612 "./glsl.g"

case 307: {
    sym(1).declaration_list = new (_engine->pool())  List<Declaration *>(sym(1).declaration_list, sym(2).declaration);
}   break;

#line 2619 "./glsl.g"

case 308: {
    // ast(1) = new ...AST(...);
}   break;

#line 2626 "./glsl.g"

case 309: {
    // ast(1) = new ...AST(...);
}   break;

#line 2633 "./glsl.g"

case 310: {
    // ast(1) = new ...AST(...);
}   break;

#line 2640 "./glsl.g"

case 311: {
    // ast(1) = new ...AST(...);
}   break;

#line 2647 "./glsl.g"

case 312: {
    ast(1) = 0;
}   break;

#line 2655 "./glsl.g"

} // end switch
} // end Parser::reduce()
