
#line 337 "./glsl.g"

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

    fprintf(stderr, "unexpected token `%s' at line %d\n", yytoken != -1 ? spell[yytoken] : "",
        _tokens[yyloc].line + 1);

    return 0;
}

#line 499 "./glsl.g"

void Parser::reduce(int ruleno)
{
switch(ruleno) {

#line 508 "./glsl.g"

case 0: {
    ast(1) = makeAstNode<IdentifierExpression>(sym(1).string);
}   break;

#line 515 "./glsl.g"

case 1: {
    // ast(1) = new ...AST(...);
}   break;

#line 522 "./glsl.g"

case 2: {
    // ast(1) = new ...AST(...);
}   break;

#line 529 "./glsl.g"

case 3: {
    // ast(1) = new ...AST(...);
}   break;

#line 536 "./glsl.g"

case 4: {
    // nothing to do.
}   break;

#line 543 "./glsl.g"

case 5: {
    ast(1) = ast(2);
}   break;

#line 550 "./glsl.g"

case 6: {
    // nothing to do.
}   break;

#line 557 "./glsl.g"

case 7: {
    // ast(1) = new ...AST(...);
}   break;

#line 564 "./glsl.g"

case 8: {
    // nothing to do.
}   break;

#line 571 "./glsl.g"

case 9: {
    // ast(1) = new ...AST(...);
}   break;

#line 578 "./glsl.g"

case 10: {
    // ast(1) = new ...AST(...);
}   break;

#line 585 "./glsl.g"

case 11: {
    // ast(1) = new ...AST(...);
}   break;

#line 592 "./glsl.g"

case 12: {
    // nothing to do.
}   break;

#line 599 "./glsl.g"

case 13: {
    // nothing to do.
}   break;

#line 606 "./glsl.g"

case 14: {
    // nothing to do.
}   break;

#line 613 "./glsl.g"

case 15: {
    // ast(1) = new ...AST(...);
}   break;

#line 620 "./glsl.g"

case 16: {
    // ast(1) = new ...AST(...);
}   break;

#line 627 "./glsl.g"

case 17: {
    // ast(1) = new ...AST(...);
}   break;

#line 634 "./glsl.g"

case 18: {
    // ast(1) = new ...AST(...);
}   break;

#line 641 "./glsl.g"

case 19: {
    // ast(1) = new ...AST(...);
}   break;

#line 648 "./glsl.g"

case 20: {
    // ast(1) = new ...AST(...);
}   break;

#line 655 "./glsl.g"

case 21: {
    // ast(1) = new ...AST(...);
}   break;

#line 662 "./glsl.g"

case 22: {
    // ast(1) = new ...AST(...);
}   break;

#line 669 "./glsl.g"

case 23: {
    // ast(1) = new ...AST(...);
}   break;

#line 676 "./glsl.g"

case 24: {
    // ast(1) = new ...AST(...);
}   break;

#line 683 "./glsl.g"

case 25: {
    // nothing to do.
}   break;

#line 690 "./glsl.g"

case 26: {
    // ast(1) = new ...AST(...);
}   break;

#line 697 "./glsl.g"

case 27: {
    // ast(1) = new ...AST(...);
}   break;

#line 704 "./glsl.g"

case 28: {
    // ast(1) = new ...AST(...);
}   break;

#line 711 "./glsl.g"

case 29: {
    // ast(1) = new ...AST(...);
}   break;

#line 718 "./glsl.g"

case 30: {
    // ast(1) = new ...AST(...);
}   break;

#line 725 "./glsl.g"

case 31: {
    // ast(1) = new ...AST(...);
}   break;

#line 732 "./glsl.g"

case 32: {
    // ast(1) = new ...AST(...);
}   break;

#line 739 "./glsl.g"

case 33: {
    // nothing to do.
}   break;

#line 746 "./glsl.g"

case 34: {
    ast(1) = makeAstNode<BinaryExpression>(AST::Kind_Multiply, sym(1).expression, sym(3).expression);
}   break;

#line 753 "./glsl.g"

case 35: {
    // ast(1) = new ...AST(...);
}   break;

#line 760 "./glsl.g"

case 36: {
    // ast(1) = new ...AST(...);
}   break;

#line 767 "./glsl.g"

case 37: {
    // nothing to do.
}   break;

#line 774 "./glsl.g"

case 38: {
    // ast(1) = new ...AST(...);
}   break;

#line 781 "./glsl.g"

case 39: {
    // ast(1) = new ...AST(...);
}   break;

#line 788 "./glsl.g"

case 40: {
    // nothing to do.
}   break;

#line 795 "./glsl.g"

case 41: {
    // ast(1) = new ...AST(...);
}   break;

#line 802 "./glsl.g"

case 42: {
    // ast(1) = new ...AST(...);
}   break;

#line 809 "./glsl.g"

case 43: {
    // nothing to do.
}   break;

#line 816 "./glsl.g"

case 44: {
    // ast(1) = new ...AST(...);
}   break;

#line 823 "./glsl.g"

case 45: {
    // ast(1) = new ...AST(...);
}   break;

#line 830 "./glsl.g"

case 46: {
    // ast(1) = new ...AST(...);
}   break;

#line 837 "./glsl.g"

case 47: {
    // ast(1) = new ...AST(...);
}   break;

#line 844 "./glsl.g"

case 48: {
    // nothing to do.
}   break;

#line 851 "./glsl.g"

case 49: {
    // ast(1) = new ...AST(...);
}   break;

#line 858 "./glsl.g"

case 50: {
    // ast(1) = new ...AST(...);
}   break;

#line 865 "./glsl.g"

case 51: {
    // nothing to do.
}   break;

#line 872 "./glsl.g"

case 52: {
    // ast(1) = new ...AST(...);
}   break;

#line 879 "./glsl.g"

case 53: {
    // nothing to do.
}   break;

#line 886 "./glsl.g"

case 54: {
    // ast(1) = new ...AST(...);
}   break;

#line 893 "./glsl.g"

case 55: {
    // nothing to do.
}   break;

#line 900 "./glsl.g"

case 56: {
    // ast(1) = new ...AST(...);
}   break;

#line 907 "./glsl.g"

case 57: {
    // nothing to do.
}   break;

#line 914 "./glsl.g"

case 58: {
    // ast(1) = new ...AST(...);
}   break;

#line 921 "./glsl.g"

case 59: {
    // nothing to do.
}   break;

#line 928 "./glsl.g"

case 60: {
    // ast(1) = new ...AST(...);
}   break;

#line 935 "./glsl.g"

case 61: {
    // nothing to do.
}   break;

#line 942 "./glsl.g"

case 62: {
    // ast(1) = new ...AST(...);
}   break;

#line 949 "./glsl.g"

case 63: {
    // nothing to do.
}   break;

#line 956 "./glsl.g"

case 64: {
    // ast(1) = new ...AST(...);
}   break;

#line 963 "./glsl.g"

case 65: {
    // nothing to do.
}   break;

#line 970 "./glsl.g"

case 66: {
    // ast(1) = new ...AST(...);
}   break;

#line 977 "./glsl.g"

case 67: {
    // ast(1) = new ...AST(...);
}   break;

#line 984 "./glsl.g"

case 68: {
    // ast(1) = new ...AST(...);
}   break;

#line 991 "./glsl.g"

case 69: {
    // ast(1) = new ...AST(...);
}   break;

#line 998 "./glsl.g"

case 70: {
    // ast(1) = new ...AST(...);
}   break;

#line 1005 "./glsl.g"

case 71: {
    // ast(1) = new ...AST(...);
}   break;

#line 1012 "./glsl.g"

case 72: {
    // ast(1) = new ...AST(...);
}   break;

#line 1019 "./glsl.g"

case 73: {
    // ast(1) = new ...AST(...);
}   break;

#line 1026 "./glsl.g"

case 74: {
    // ast(1) = new ...AST(...);
}   break;

#line 1033 "./glsl.g"

case 75: {
    // ast(1) = new ...AST(...);
}   break;

#line 1040 "./glsl.g"

case 76: {
    // ast(1) = new ...AST(...);
}   break;

#line 1047 "./glsl.g"

case 77: {
    // ast(1) = new ...AST(...);
}   break;

#line 1054 "./glsl.g"

case 78: {
    // nothing to do.
}   break;

#line 1061 "./glsl.g"

case 79: {
    // ast(1) = new ...AST(...);
}   break;

#line 1068 "./glsl.g"

case 80: {
    // nothing to do.
}   break;

#line 1075 "./glsl.g"

case 81: {
    // ast(1) = new ...AST(...);
}   break;

#line 1082 "./glsl.g"

case 82: {
    // ast(1) = new ...AST(...);
}   break;

#line 1089 "./glsl.g"

case 83: {
    // ast(1) = new ...AST(...);
}   break;

#line 1096 "./glsl.g"

case 84: {
    // ast(1) = new ...AST(...);
}   break;

#line 1103 "./glsl.g"

case 85: {
    // ast(1) = new ...AST(...);
}   break;

#line 1110 "./glsl.g"

case 86: {
    // ast(1) = new ...AST(...);
}   break;

#line 1117 "./glsl.g"

case 87: {
    // ast(1) = new ...AST(...);
}   break;

#line 1124 "./glsl.g"

case 88: {
    // ast(1) = new ...AST(...);
}   break;

#line 1131 "./glsl.g"

case 89: {
    // ast(1) = new ...AST(...);
}   break;

#line 1138 "./glsl.g"

case 90: {
    // ast(1) = new ...AST(...);
}   break;

#line 1145 "./glsl.g"

case 91: {
    // ast(1) = new ...AST(...);
}   break;

#line 1152 "./glsl.g"

case 92: {
    // ast(1) = new ...AST(...);
}   break;

#line 1159 "./glsl.g"

case 93: {
    // ast(1) = new ...AST(...);
}   break;

#line 1166 "./glsl.g"

case 94: {
    // ast(1) = new ...AST(...);
}   break;

#line 1173 "./glsl.g"

case 95: {
    // ast(1) = new ...AST(...);
}   break;

#line 1180 "./glsl.g"

case 96: {
    // ast(1) = new ...AST(...);
}   break;

#line 1187 "./glsl.g"

case 97: {
    // ast(1) = new ...AST(...);
}   break;

#line 1194 "./glsl.g"

case 98: {
    // ast(1) = new ...AST(...);
}   break;

#line 1201 "./glsl.g"

case 99: {
    // ast(1) = new ...AST(...);
}   break;

#line 1208 "./glsl.g"

case 100: {
    // ast(1) = new ...AST(...);
}   break;

#line 1215 "./glsl.g"

case 101: {
    // ast(1) = new ...AST(...);
}   break;

#line 1222 "./glsl.g"

case 102: {
    // ast(1) = new ...AST(...);
}   break;

#line 1229 "./glsl.g"

case 103: {
    // ast(1) = new ...AST(...);
}   break;

#line 1236 "./glsl.g"

case 104: {
    // ast(1) = new ...AST(...);
}   break;

#line 1243 "./glsl.g"

case 105: {
    // ast(1) = new ...AST(...);
}   break;

#line 1250 "./glsl.g"

case 106: {
    // ast(1) = new ...AST(...);
}   break;

#line 1257 "./glsl.g"

case 107: {
    // ast(1) = new ...AST(...);
}   break;

#line 1264 "./glsl.g"

case 108: {
    // ast(1) = new ...AST(...);
}   break;

#line 1271 "./glsl.g"

case 109: {
    // ast(1) = new ...AST(...);
}   break;

#line 1278 "./glsl.g"

case 110: {
    // ast(1) = new ...AST(...);
}   break;

#line 1285 "./glsl.g"

case 111: {
    // ast(1) = new ...AST(...);
}   break;

#line 1292 "./glsl.g"

case 112: {
    // ast(1) = new ...AST(...);
}   break;

#line 1299 "./glsl.g"

case 113: {
    // ast(1) = new ...AST(...);
}   break;

#line 1306 "./glsl.g"

case 114: {
    // ast(1) = new ...AST(...);
}   break;

#line 1313 "./glsl.g"

case 115: {
    // ast(1) = new ...AST(...);
}   break;

#line 1320 "./glsl.g"

case 116: {
    // ast(1) = new ...AST(...);
}   break;

#line 1327 "./glsl.g"

case 117: {
    // ast(1) = new ...AST(...);
}   break;

#line 1334 "./glsl.g"

case 118: {
    // ast(1) = new ...AST(...);
}   break;

#line 1341 "./glsl.g"

case 119: {
    // ast(1) = new ...AST(...);
}   break;

#line 1348 "./glsl.g"

case 120: {
    // ast(1) = new ...AST(...);
}   break;

#line 1355 "./glsl.g"

case 121: {
    // ast(1) = new ...AST(...);
}   break;

#line 1362 "./glsl.g"

case 122: {
    // ast(1) = new ...AST(...);
}   break;

#line 1369 "./glsl.g"

case 123: {
    // ast(1) = new ...AST(...);
}   break;

#line 1376 "./glsl.g"

case 124: {
    // ast(1) = new ...AST(...);
}   break;

#line 1383 "./glsl.g"

case 125: {
    // ast(1) = new ...AST(...);
}   break;

#line 1390 "./glsl.g"

case 126: {
    // ast(1) = new ...AST(...);
}   break;

#line 1397 "./glsl.g"

case 127: {
    // ast(1) = new ...AST(...);
}   break;

#line 1404 "./glsl.g"

case 128: {
    // ast(1) = new ...AST(...);
}   break;

#line 1411 "./glsl.g"

case 129: {
    // ast(1) = new ...AST(...);
}   break;

#line 1418 "./glsl.g"

case 130: {
    // ast(1) = new ...AST(...);
}   break;

#line 1425 "./glsl.g"

case 131: {
    // ast(1) = new ...AST(...);
}   break;

#line 1432 "./glsl.g"

case 132: {
    // ast(1) = new ...AST(...);
}   break;

#line 1439 "./glsl.g"

case 133: {
    // ast(1) = new ...AST(...);
}   break;

#line 1446 "./glsl.g"

case 134: {
    // ast(1) = new ...AST(...);
}   break;

#line 1453 "./glsl.g"

case 135: {
    // ast(1) = new ...AST(...);
}   break;

#line 1460 "./glsl.g"

case 136: {
    // ast(1) = new ...AST(...);
}   break;

#line 1467 "./glsl.g"

case 137: {
    // ast(1) = new ...AST(...);
}   break;

#line 1474 "./glsl.g"

case 138: {
    // ast(1) = new ...AST(...);
}   break;

#line 1481 "./glsl.g"

case 139: {
    // ast(1) = new ...AST(...);
}   break;

#line 1488 "./glsl.g"

case 140: {
    // ast(1) = new ...AST(...);
}   break;

#line 1495 "./glsl.g"

case 141: {
    // ast(1) = new ...AST(...);
}   break;

#line 1502 "./glsl.g"

case 142: {
    // ast(1) = new ...AST(...);
}   break;

#line 1509 "./glsl.g"

case 143: {
    // ast(1) = new ...AST(...);
}   break;

#line 1516 "./glsl.g"

case 144: {
    // ast(1) = new ...AST(...);
}   break;

#line 1523 "./glsl.g"

case 145: {
    // ast(1) = new ...AST(...);
}   break;

#line 1530 "./glsl.g"

case 146: {
    // ast(1) = new ...AST(...);
}   break;

#line 1537 "./glsl.g"

case 147: {
    // ast(1) = new ...AST(...);
}   break;

#line 1544 "./glsl.g"

case 148: {
    // ast(1) = new ...AST(...);
}   break;

#line 1551 "./glsl.g"

case 149: {
    // ast(1) = new ...AST(...);
}   break;

#line 1558 "./glsl.g"

case 150: {
    // ast(1) = new ...AST(...);
}   break;

#line 1565 "./glsl.g"

case 151: {
    // ast(1) = new ...AST(...);
}   break;

#line 1572 "./glsl.g"

case 152: {
    // ast(1) = new ...AST(...);
}   break;

#line 1579 "./glsl.g"

case 153: {
    // ast(1) = new ...AST(...);
}   break;

#line 1586 "./glsl.g"

case 154: {
    // ast(1) = new ...AST(...);
}   break;

#line 1593 "./glsl.g"

case 155: {
    // ast(1) = new ...AST(...);
}   break;

#line 1600 "./glsl.g"

case 156: {
    // ast(1) = new ...AST(...);
}   break;

#line 1607 "./glsl.g"

case 157: {
    // ast(1) = new ...AST(...);
}   break;

#line 1614 "./glsl.g"

case 158: {
    // ast(1) = new ...AST(...);
}   break;

#line 1621 "./glsl.g"

case 159: {
    // ast(1) = new ...AST(...);
}   break;

#line 1628 "./glsl.g"

case 160: {
    // ast(1) = new ...AST(...);
}   break;

#line 1635 "./glsl.g"

case 161: {
    // ast(1) = new ...AST(...);
}   break;

#line 1642 "./glsl.g"

case 162: {
    // ast(1) = new ...AST(...);
}   break;

#line 1649 "./glsl.g"

case 163: {
    // ast(1) = new ...AST(...);
}   break;

#line 1656 "./glsl.g"

case 164: {
    // ast(1) = new ...AST(...);
}   break;

#line 1663 "./glsl.g"

case 165: {
    // ast(1) = new ...AST(...);
}   break;

#line 1670 "./glsl.g"

case 166: {
    // ast(1) = new ...AST(...);
}   break;

#line 1677 "./glsl.g"

case 167: {
    // ast(1) = new ...AST(...);
}   break;

#line 1684 "./glsl.g"

case 168: {
    // ast(1) = new ...AST(...);
}   break;

#line 1691 "./glsl.g"

case 169: {
    // ast(1) = new ...AST(...);
}   break;

#line 1698 "./glsl.g"

case 170: {
    // ast(1) = new ...AST(...);
}   break;

#line 1705 "./glsl.g"

case 171: {
    // ast(1) = new ...AST(...);
}   break;

#line 1712 "./glsl.g"

case 172: {
    // ast(1) = new ...AST(...);
}   break;

#line 1719 "./glsl.g"

case 173: {
    // ast(1) = new ...AST(...);
}   break;

#line 1726 "./glsl.g"

case 174: {
    // ast(1) = new ...AST(...);
}   break;

#line 1733 "./glsl.g"

case 175: {
    // ast(1) = new ...AST(...);
}   break;

#line 1740 "./glsl.g"

case 176: {
    // ast(1) = new ...AST(...);
}   break;

#line 1747 "./glsl.g"

case 177: {
    // ast(1) = new ...AST(...);
}   break;

#line 1754 "./glsl.g"

case 178: {
    // ast(1) = new ...AST(...);
}   break;

#line 1761 "./glsl.g"

case 179: {
    // ast(1) = new ...AST(...);
}   break;

#line 1768 "./glsl.g"

case 180: {
    // ast(1) = new ...AST(...);
}   break;

#line 1775 "./glsl.g"

case 181: {
    // ast(1) = new ...AST(...);
}   break;

#line 1782 "./glsl.g"

case 182: {
    // ast(1) = new ...AST(...);
}   break;

#line 1789 "./glsl.g"

case 183: {
    // ast(1) = new ...AST(...);
}   break;

#line 1796 "./glsl.g"

case 184: {
    // ast(1) = new ...AST(...);
}   break;

#line 1803 "./glsl.g"

case 185: {
    // ast(1) = new ...AST(...);
}   break;

#line 1810 "./glsl.g"

case 186: {
    // ast(1) = new ...AST(...);
}   break;

#line 1817 "./glsl.g"

case 187: {
    // ast(1) = new ...AST(...);
}   break;

#line 1824 "./glsl.g"

case 188: {
    // ast(1) = new ...AST(...);
}   break;

#line 1831 "./glsl.g"

case 189: {
    // ast(1) = new ...AST(...);
}   break;

#line 1838 "./glsl.g"

case 190: {
    // ast(1) = new ...AST(...);
}   break;

#line 1845 "./glsl.g"

case 191: {
    // ast(1) = new ...AST(...);
}   break;

#line 1852 "./glsl.g"

case 192: {
    // ast(1) = new ...AST(...);
}   break;

#line 1859 "./glsl.g"

case 193: {
    // ast(1) = new ...AST(...);
}   break;

#line 1866 "./glsl.g"

case 194: {
    // ast(1) = new ...AST(...);
}   break;

#line 1873 "./glsl.g"

case 195: {
    // ast(1) = new ...AST(...);
}   break;

#line 1880 "./glsl.g"

case 196: {
    // ast(1) = new ...AST(...);
}   break;

#line 1887 "./glsl.g"

case 197: {
    // ast(1) = new ...AST(...);
}   break;

#line 1894 "./glsl.g"

case 198: {
    // ast(1) = new ...AST(...);
}   break;

#line 1901 "./glsl.g"

case 199: {
    // ast(1) = new ...AST(...);
}   break;

#line 1908 "./glsl.g"

case 200: {
    // ast(1) = new ...AST(...);
}   break;

#line 1915 "./glsl.g"

case 201: {
    // ast(1) = new ...AST(...);
}   break;

#line 1922 "./glsl.g"

case 202: {
    // ast(1) = new ...AST(...);
}   break;

#line 1929 "./glsl.g"

case 203: {
    // ast(1) = new ...AST(...);
}   break;

#line 1936 "./glsl.g"

case 204: {
    // ast(1) = new ...AST(...);
}   break;

#line 1943 "./glsl.g"

case 205: {
    // ast(1) = new ...AST(...);
}   break;

#line 1950 "./glsl.g"

case 206: {
    // ast(1) = new ...AST(...);
}   break;

#line 1957 "./glsl.g"

case 207: {
    // ast(1) = new ...AST(...);
}   break;

#line 1964 "./glsl.g"

case 208: {
    // ast(1) = new ...AST(...);
}   break;

#line 1971 "./glsl.g"

case 209: {
    // ast(1) = new ...AST(...);
}   break;

#line 1978 "./glsl.g"

case 210: {
    // ast(1) = new ...AST(...);
}   break;

#line 1985 "./glsl.g"

case 211: {
    // ast(1) = new ...AST(...);
}   break;

#line 1992 "./glsl.g"

case 212: {
    // ast(1) = new ...AST(...);
}   break;

#line 1999 "./glsl.g"

case 213: {
    // ast(1) = new ...AST(...);
}   break;

#line 2006 "./glsl.g"

case 214: {
    // ast(1) = new ...AST(...);
}   break;

#line 2013 "./glsl.g"

case 215: {
    // ast(1) = new ...AST(...);
}   break;

#line 2020 "./glsl.g"

case 216: {
    // ast(1) = new ...AST(...);
}   break;

#line 2027 "./glsl.g"

case 217: {
    // ast(1) = new ...AST(...);
}   break;

#line 2034 "./glsl.g"

case 218: {
    // ast(1) = new ...AST(...);
}   break;

#line 2041 "./glsl.g"

case 219: {
    // ast(1) = new ...AST(...);
}   break;

#line 2048 "./glsl.g"

case 220: {
    // ast(1) = new ...AST(...);
}   break;

#line 2055 "./glsl.g"

case 221: {
    // ast(1) = new ...AST(...);
}   break;

#line 2062 "./glsl.g"

case 222: {
    // ast(1) = new ...AST(...);
}   break;

#line 2069 "./glsl.g"

case 223: {
    // ast(1) = new ...AST(...);
}   break;

#line 2076 "./glsl.g"

case 224: {
    // ast(1) = new ...AST(...);
}   break;

#line 2083 "./glsl.g"

case 225: {
    // ast(1) = new ...AST(...);
}   break;

#line 2090 "./glsl.g"

case 226: {
    // ast(1) = new ...AST(...);
}   break;

#line 2097 "./glsl.g"

case 227: {
    // ast(1) = new ...AST(...);
}   break;

#line 2104 "./glsl.g"

case 228: {
    // ast(1) = new ...AST(...);
}   break;

#line 2111 "./glsl.g"

case 229: {
    // ast(1) = new ...AST(...);
}   break;

#line 2118 "./glsl.g"

case 230: {
    // ast(1) = new ...AST(...);
}   break;

#line 2125 "./glsl.g"

case 231: {
    // ast(1) = new ...AST(...);
}   break;

#line 2132 "./glsl.g"

case 232: {
    // ast(1) = new ...AST(...);
}   break;

#line 2139 "./glsl.g"

case 233: {
    // ast(1) = new ...AST(...);
}   break;

#line 2146 "./glsl.g"

case 234: {
    // ast(1) = new ...AST(...);
}   break;

#line 2153 "./glsl.g"

case 235: {
    // ast(1) = new ...AST(...);
}   break;

#line 2160 "./glsl.g"

case 236: {
    // ast(1) = new ...AST(...);
}   break;

#line 2167 "./glsl.g"

case 237: {
    // ast(1) = new ...AST(...);
}   break;

#line 2174 "./glsl.g"

case 238: {
    // ast(1) = new ...AST(...);
}   break;

#line 2181 "./glsl.g"

case 239: {
    // ast(1) = new ...AST(...);
}   break;

#line 2188 "./glsl.g"

case 240: {
    // ast(1) = new ...AST(...);
}   break;

#line 2195 "./glsl.g"

case 241: {
    // ast(1) = new ...AST(...);
}   break;

#line 2202 "./glsl.g"

case 242: {
    // ast(1) = new ...AST(...);
}   break;

#line 2209 "./glsl.g"

case 243: {
    // ast(1) = new ...AST(...);
}   break;

#line 2216 "./glsl.g"

case 244: {
    // ast(1) = new ...AST(...);
}   break;

#line 2223 "./glsl.g"

case 245: {
    // ast(1) = new ...AST(...);
}   break;

#line 2230 "./glsl.g"

case 246: {
    // ast(1) = new ...AST(...);
}   break;

#line 2237 "./glsl.g"

case 247: {
    // ast(1) = new ...AST(...);
}   break;

#line 2244 "./glsl.g"

case 248: {
    // ast(1) = new ...AST(...);
}   break;

#line 2251 "./glsl.g"

case 249: {
    // ast(1) = new ...AST(...);
}   break;

#line 2258 "./glsl.g"

case 250: {
    // ast(1) = new ...AST(...);
}   break;

#line 2265 "./glsl.g"

case 251: {
    // ast(1) = new ...AST(...);
}   break;

#line 2272 "./glsl.g"

case 252: {
    // ast(1) = new ...AST(...);
}   break;

#line 2279 "./glsl.g"

case 253: {
    // ast(1) = new ...AST(...);
}   break;

#line 2286 "./glsl.g"

case 254: {
    // ast(1) = new ...AST(...);
}   break;

#line 2293 "./glsl.g"

case 255: {
    // ast(1) = new ...AST(...);
}   break;

#line 2300 "./glsl.g"

case 256: {
    // ast(1) = new ...AST(...);
}   break;

#line 2307 "./glsl.g"

case 257: {
    // ast(1) = new ...AST(...);
}   break;

#line 2314 "./glsl.g"

case 258: {
    // ast(1) = new ...AST(...);
}   break;

#line 2321 "./glsl.g"

case 259: {
    // ast(1) = new ...AST(...);
}   break;

#line 2328 "./glsl.g"

case 260: {
    // ast(1) = new ...AST(...);
}   break;

#line 2335 "./glsl.g"

case 261: {
    // ast(1) = new ...AST(...);
}   break;

#line 2342 "./glsl.g"

case 262: {
    // ast(1) = new ...AST(...);
}   break;

#line 2349 "./glsl.g"

case 263: {
    // ast(1) = new ...AST(...);
}   break;

#line 2356 "./glsl.g"

case 264: {
    // ast(1) = new ...AST(...);
}   break;

#line 2363 "./glsl.g"

case 265: {
    // ast(1) = new ...AST(...);
}   break;

#line 2370 "./glsl.g"

case 266: {
    // ast(1) = new ...AST(...);
}   break;

#line 2377 "./glsl.g"

case 267: {
    // ast(1) = new ...AST(...);
}   break;

#line 2384 "./glsl.g"

case 268: {
    // ast(1) = new ...AST(...);
}   break;

#line 2391 "./glsl.g"

case 269: {
    // ast(1) = new ...AST(...);
}   break;

#line 2398 "./glsl.g"

case 270: {
    // ast(1) = new ...AST(...);
}   break;

#line 2405 "./glsl.g"

case 271: {
    // ast(1) = new ...AST(...);
}   break;

#line 2412 "./glsl.g"

case 272: {
    // ast(1) = new ...AST(...);
}   break;

#line 2419 "./glsl.g"

case 273: {
    // ast(1) = new ...AST(...);
}   break;

#line 2426 "./glsl.g"

case 274: {
    // ast(1) = new ...AST(...);
}   break;

#line 2433 "./glsl.g"

case 275: {
    // ast(1) = new ...AST(...);
}   break;

#line 2440 "./glsl.g"

case 276: {
    // ast(1) = new ...AST(...);
}   break;

#line 2447 "./glsl.g"

case 277: {
    // ast(1) = new ...AST(...);
}   break;

#line 2454 "./glsl.g"

case 278: {
    // ast(1) = new ...AST(...);
}   break;

#line 2461 "./glsl.g"

case 279: {
    // ast(1) = new ...AST(...);
}   break;

#line 2468 "./glsl.g"

case 280: {
    // ast(1) = new ...AST(...);
}   break;

#line 2475 "./glsl.g"

case 281: {
    // ast(1) = new ...AST(...);
}   break;

#line 2482 "./glsl.g"

case 282: {
    // ast(1) = new ...AST(...);
}   break;

#line 2489 "./glsl.g"

case 283: {
    // ast(1) = new ...AST(...);
}   break;

#line 2496 "./glsl.g"

case 284: {
    // ast(1) = new ...AST(...);
}   break;

#line 2503 "./glsl.g"

case 285: {
    // ast(1) = new ...AST(...);
}   break;

#line 2510 "./glsl.g"

case 286: {
    // ast(1) = new ...AST(...);
}   break;

#line 2517 "./glsl.g"

case 287: {
    // ast(1) = new ...AST(...);
}   break;

#line 2524 "./glsl.g"

case 288: {
    // ast(1) = new ...AST(...);
}   break;

#line 2531 "./glsl.g"

case 289: {
    // ast(1) = new ...AST(...);
}   break;

#line 2538 "./glsl.g"

case 290: {
    // ast(1) = new ...AST(...);
}   break;

#line 2545 "./glsl.g"

case 291: {
    // ast(1) = new ...AST(...);
}   break;

#line 2552 "./glsl.g"

case 292: {
    // ast(1) = new ...AST(...);
}   break;

#line 2559 "./glsl.g"

case 293: {
    // ast(1) = new ...AST(...);
}   break;

#line 2566 "./glsl.g"

case 294: {
    // ast(1) = new ...AST(...);
}   break;

#line 2573 "./glsl.g"

case 295: {
    // ast(1) = new ...AST(...);
}   break;

#line 2580 "./glsl.g"

case 296: {
    // ast(1) = new ...AST(...);
}   break;

#line 2587 "./glsl.g"

case 297: {
    // ast(1) = new ...AST(...);
}   break;

#line 2594 "./glsl.g"

case 298: {
    // ast(1) = new ...AST(...);
}   break;

#line 2601 "./glsl.g"

case 299: {
    // ast(1) = new ...AST(...);
}   break;

#line 2608 "./glsl.g"

case 300: {
    // ast(1) = new ...AST(...);
}   break;

#line 2615 "./glsl.g"

case 301: {
    // ast(1) = new ...AST(...);
}   break;

#line 2622 "./glsl.g"

case 302: {
    // ast(1) = new ...AST(...);
}   break;

#line 2629 "./glsl.g"

case 303: {
    // ast(1) = new ...AST(...);
}   break;

#line 2636 "./glsl.g"

case 304: {
    // ast(1) = new ...AST(...);
}   break;

#line 2643 "./glsl.g"

case 305: {
    ast(1) = makeAstNode<TranslationUnit>(sym(1).declaration_list);
}   break;

#line 2650 "./glsl.g"

case 306: {
    sym(1).declaration_list = makeAstNode< List<Declaration *> >(sym(1).declaration);
}   break;

#line 2657 "./glsl.g"

case 307: {
    sym(1).declaration_list = makeAstNode< List<Declaration *> >(sym(1).declaration_list, sym(2).declaration);
}   break;

#line 2664 "./glsl.g"

case 308: {
    // ast(1) = new ...AST(...);
}   break;

#line 2671 "./glsl.g"

case 309: {
    // ast(1) = new ...AST(...);
}   break;

#line 2678 "./glsl.g"

case 310: {
    // ast(1) = new ...AST(...);
}   break;

#line 2685 "./glsl.g"

case 311: {
    // ast(1) = new ...AST(...);
}   break;

#line 2692 "./glsl.g"

case 312: {
    ast(1) = 0;
}   break;

#line 2700 "./glsl.g"

} // end switch
} // end Parser::reduce()
