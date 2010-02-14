/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Copyright (c) 2008 Roberto Raggi <roberto.raggi@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "Lexer.h"
#include "Control.h"
#include "TranslationUnit.h"
#include "Literals.h"
#include <cctype>
#include <cassert>

using namespace CPlusPlus;

Lexer::Lexer(TranslationUnit *unit)
    : _translationUnit(unit),
      _state(State_Default),
      _flags(0),
      _currentLine(1)
{
    f._scanKeywords = true;
    setSource(_translationUnit->firstSourceChar(),
              _translationUnit->lastSourceChar());
}

Lexer::Lexer(const char *firstChar, const char *lastChar)
    : _translationUnit(0),
      _state(State_Default),
      _flags(0),
      _currentLine(1)
{
    f._scanKeywords = true;
    setSource(firstChar, lastChar);
}

Lexer::~Lexer()
{ }

TranslationUnit *Lexer::translationUnit() const
{ return _translationUnit; }

Control *Lexer::control() const
{
    if (_translationUnit)
        return _translationUnit->control();

    return 0;
}

void Lexer::setSource(const char *firstChar, const char *lastChar)
{
    _firstChar = firstChar;
    _lastChar = lastChar;
    _currentChar = _firstChar - 1;
    _tokenStart = _currentChar;
    _yychar = '\n';
}

void Lexer::setStartWithNewline(bool enabled)
{
    if (enabled)
        _yychar = '\n';
    else
        _yychar = ' ';
}

int Lexer::state() const
{ return _state; }

void Lexer::setState(int state)
{ _state = state; }

bool Lexer::qtMocRunEnabled() const
{ return f._qtMocRunEnabled; }

void Lexer::setQtMocRunEnabled(bool onoff)
{ f._qtMocRunEnabled = onoff; }

bool Lexer::objCEnabled() const
{ return f._objCEnabled; }

void Lexer::setObjCEnabled(bool onoff)
{ f._objCEnabled = onoff; }

bool Lexer::isIncremental() const
{ return f._isIncremental; }

void Lexer::setIncremental(bool isIncremental)
{ f._isIncremental = isIncremental; }

bool Lexer::scanCommentTokens() const
{ return f._scanCommentTokens; }

void Lexer::setScanCommentTokens(bool onoff)
{ f._scanCommentTokens = onoff; }

bool Lexer::scanKeywords() const
{ return f._scanKeywords; }

void Lexer::setScanKeywords(bool onoff)
{ f._scanKeywords = onoff; }

void Lexer::setScanAngleStringLiteralTokens(bool onoff)
{ f._scanAngleStringLiteralTokens = onoff; }

void Lexer::pushLineStartOffset()
{
    ++_currentLine;

    if (_translationUnit)
        _translationUnit->pushLineOffset(_currentChar - _firstChar);
}

unsigned Lexer::tokenOffset() const
{ return _tokenStart - _firstChar; }

unsigned Lexer::tokenLength() const
{ return _currentChar - _tokenStart; }

const char *Lexer::tokenBegin() const
{ return _tokenStart; }

const char *Lexer::tokenEnd() const
{ return _currentChar; }

unsigned Lexer::currentLine() const
{ return _currentLine; }

void Lexer::scan(Token *tok)
{
    tok->reset();
    scan_helper(tok);
    tok->f.length = _currentChar - _tokenStart;
}

void Lexer::scan_helper(Token *tok)
{
  _Lagain:
    while (_yychar && std::isspace(_yychar)) {
        if (_yychar == '\n') {
            tok->f.joined = false;
            tok->f.newline = true;
        } else {
            tok->f.whitespace = true;
        }
        yyinp();
    }

    if (! _translationUnit)
        tok->lineno = _currentLine;

    _tokenStart = _currentChar;
    tok->offset = _currentChar - _firstChar;

    if (_state == State_MultiLineComment || _state == State_MultiLineDoxyComment) {
        const int originalState = _state;

        if (! _yychar) {
            tok->f.kind = T_EOF_SYMBOL;
            return;
        }

        while (_yychar) {
            if (_yychar != '*')
                yyinp();
            else {
                yyinp();
                if (_yychar == '/') {
                    yyinp();
                    _state = State_Default;
                    break;
                }
            }
        }

        if (! f._scanCommentTokens)
            goto _Lagain;

        else if (originalState == State_MultiLineComment)
            tok->f.kind = T_COMMENT;
        else
            tok->f.kind = T_DOXY_COMMENT;
        return; // done
    }

    if (! _yychar) {
        tok->f.kind = T_EOF_SYMBOL;
        return;
    }

    unsigned char ch = _yychar;
    yyinp();

    switch (ch) {
    case '\\':
        while (_yychar != '\n' && std::isspace(_yychar))
            yyinp();
        // ### assert(! _yychar || _yychar == '\n');
        if (_yychar == '\n') {
            tok->f.joined = true;
            tok->f.newline = false;
            yyinp();
        }
        goto _Lagain;

    case '"': case '\'': {
        const char quote = ch;

        tok->f.kind = quote == '"'
            ? T_STRING_LITERAL
            : T_CHAR_LITERAL;

        const char *yytext = _currentChar;

        while (_yychar && _yychar != quote) {
            if (_yychar == '\n')
                break;
            else if (_yychar != '\\')
                yyinp();
            else {
                yyinp(); // skip `\\'

                if (_yychar)
                    yyinp();
            }
        }
        // assert(_yychar == quote);

        int yylen = _currentChar - yytext;

        if (_yychar == quote)
            yyinp();

        if (control())
            tok->string = control()->findOrInsertStringLiteral(yytext, yylen);
    } break;

    case '{':
        tok->f.kind = T_LBRACE;
        break;

    case '}':
        tok->f.kind = T_RBRACE;
        break;

    case '[':
        tok->f.kind = T_LBRACKET;
        break;

    case ']':
        tok->f.kind = T_RBRACKET;
        break;

    case '#':
        if (_yychar == '#') {
            tok->f.kind = T_POUND_POUND;
            yyinp();
        } else {
            tok->f.kind = T_POUND;
        }
        break;

    case '(':
        tok->f.kind = T_LPAREN;
        break;

    case ')':
        tok->f.kind = T_RPAREN;
        break;

    case ';':
        tok->f.kind = T_SEMICOLON;
        break;

    case ':':
        if (_yychar == ':') {
            yyinp();
            tok->f.kind = T_COLON_COLON;
        } else {
            tok->f.kind = T_COLON;
        }
        break;

    case '.':
        if (_yychar == '*') {
            yyinp();
            tok->f.kind = T_DOT_STAR;
        } else if (_yychar == '.') {
            yyinp();
            // ### assert(_yychar);
            if (_yychar == '.') {
                yyinp();
                tok->f.kind = T_DOT_DOT_DOT;
            } else {
                tok->f.kind = T_ERROR;
            }
        } else if (std::isdigit(_yychar)) {
            const char *yytext = _currentChar - 2;
            do {
                if (_yychar == 'e' || _yychar == 'E') {
                    yyinp();
                    if (_yychar == '-' || _yychar == '+') {
                        yyinp();
                        // ### assert(std::isdigit(_yychar));
                    }
                } else if (std::isalnum(_yychar) || _yychar == '.') {
                    yyinp();
                } else {
                    break;
                }
            } while (_yychar);
            int yylen = _currentChar - yytext;
            tok->f.kind = T_NUMERIC_LITERAL;
            if (control())
                tok->number = control()->findOrInsertNumericLiteral(yytext, yylen);
        } else {
            tok->f.kind = T_DOT;
        }
        break;

    case '?':
        tok->f.kind = T_QUESTION;
        break;

    case '+':
        if (_yychar == '+') {
            yyinp();
            tok->f.kind = T_PLUS_PLUS;
        } else if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_PLUS_EQUAL;
        } else {
            tok->f.kind = T_PLUS;
        }
        break;

    case '-':
        if (_yychar == '-') {
            yyinp();
            tok->f.kind = T_MINUS_MINUS;
        } else if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_MINUS_EQUAL;
        } else if (_yychar == '>') {
            yyinp();
            if (_yychar == '*') {
                yyinp();
                tok->f.kind = T_ARROW_STAR;
            } else {
                tok->f.kind = T_ARROW;
            }
        } else {
            tok->f.kind = T_MINUS;
        }
        break;

    case '*':
        if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_STAR_EQUAL;
        } else {
            tok->f.kind = T_STAR;
        }
        break;

    case '/':
        if (_yychar == '/') {
            yyinp();

            bool doxy = false;

            if (_yychar == '/' || _yychar == '!') {
                yyinp();

                if (_yychar == '<')
                    yyinp();

                if (_yychar != '\n' && std::isspace(_yychar))
                    doxy = true;
            }

            while (_yychar && _yychar != '\n')
                yyinp();

            if (! f._scanCommentTokens)
                goto _Lagain;

            tok->f.kind = doxy ? T_CPP_DOXY_COMMENT : T_CPP_COMMENT;

        } else if (_yychar == '*') {
            yyinp();

            bool doxy = false;

            if (_yychar == '*' || _yychar == '!') {
                const char ch = _yychar;

                yyinp();

                if (ch == '*' && _yychar == '/')
                    goto _Ldone;

                if (_yychar == '<')
                    yyinp();

                if (! _yychar || std::isspace(_yychar))
                    doxy = true;
            }

            while (_yychar) {
                if (_yychar != '*') {
                    yyinp();
                } else {
                    yyinp();
                    if (_yychar == '/')
                        break;
                }
            }

        _Ldone:
            if (_yychar)
                yyinp();
            else
                _state = doxy ? State_MultiLineDoxyComment : State_MultiLineComment;

            if (! f._scanCommentTokens)
                goto _Lagain;

            tok->f.kind = doxy ? T_DOXY_COMMENT : T_COMMENT;

        } else if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_SLASH_EQUAL;
        } else {
            tok->f.kind = T_SLASH;
        }
        break;

    case '%':
        if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_PERCENT_EQUAL;
        } else {
            tok->f.kind = T_PERCENT;
        }
        break;

    case '^':
        if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_CARET_EQUAL;
        } else {
            tok->f.kind = T_CARET;
        }
        break;

    case '&':
        if (_yychar == '&') {
            yyinp();
            tok->f.kind = T_AMPER_AMPER;
        } else if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_AMPER_EQUAL;
        } else {
            tok->f.kind = T_AMPER;
        }
        break;

    case '|':
        if (_yychar == '|') {
            yyinp();
            tok->f.kind = T_PIPE_PIPE;
        } else if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_PIPE_EQUAL;
        } else {
            tok->f.kind = T_PIPE;
        }
        break;

    case '~':
        if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_TILDE_EQUAL;
        } else {
            tok->f.kind = T_TILDE;
        }
        break;

    case '!':
        if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_EXCLAIM_EQUAL;
        } else {
            tok->f.kind = T_EXCLAIM;
        }
        break;

    case '=':
        if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_EQUAL_EQUAL;
        } else {
            tok->f.kind = T_EQUAL;
        }
        break;

    case '<':
        if (f._scanAngleStringLiteralTokens) {
            const char *yytext = _currentChar;
            while (_yychar && _yychar != '>')
                yyinp();
            int yylen = _currentChar - yytext;
            // ### assert(_yychar == '>');
            if (_yychar == '>')
                yyinp();
            if (control())
                tok->string = control()->findOrInsertStringLiteral(yytext, yylen);
            tok->f.kind = T_ANGLE_STRING_LITERAL;
        } else if (_yychar == '<') {
            yyinp();
            if (_yychar == '=') {
                yyinp();
                tok->f.kind = T_LESS_LESS_EQUAL;
            } else
                tok->f.kind = T_LESS_LESS;
        } else if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_LESS_EQUAL;
        } else {
            tok->f.kind = T_LESS;
        }
        break;

    case '>':
        if (_yychar == '>') {
            yyinp();
            if (_yychar == '=') {
                yyinp();
                tok->f.kind = T_GREATER_GREATER_EQUAL;
            } else
                tok->f.kind = T_LESS_LESS;
            tok->f.kind = T_GREATER_GREATER;
        } else if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_GREATER_EQUAL;
        } else {
            tok->f.kind = T_GREATER;
        }
        break;

    case ',':
        tok->f.kind = T_COMMA;
        break;

    case '@':
        tok->f.kind = T_AT;
        break;

    default: {
        if (ch == 'L' && (_yychar == '"' || _yychar == '\'')) {
            // wide char/string literals
            ch = _yychar;
            yyinp();

            const char quote = ch;

            tok->f.kind = quote == '"'
                ? T_WIDE_STRING_LITERAL
                : T_WIDE_CHAR_LITERAL;

            const char *yytext = _currentChar;

            while (_yychar && _yychar != quote) {
                if (_yychar != '\\')
                    yyinp();
                else {
                    yyinp(); // skip `\\'

                    if (_yychar)
                        yyinp();
                }
            }
            // assert(_yychar == quote);

            int yylen = _currentChar - yytext;

            if (_yychar == quote)
                yyinp();

            if (control())
                tok->string = control()->findOrInsertStringLiteral(yytext, yylen);
        } else if (std::isalpha(ch) || ch == '_' || ch == '$') {
            const char *yytext = _currentChar - 1;
            while (std::isalnum(_yychar) || _yychar == '_' || _yychar == '$')
                yyinp();
            int yylen = _currentChar - yytext;
            if (f._scanKeywords) {
                tok->f.kind = classify(yytext, yylen, f._qtMocRunEnabled);
            } else {
                tok->f.kind = T_IDENTIFIER;
            }
            // ### is this correct w.r.t. the _scanKeywords?
            if (f._objCEnabled && tok->f.kind == T_IDENTIFIER) {
                tok->f.kind = classifyObjCAtKeyword(yytext, yylen);
                if (tok->f.kind == T_ERROR)
                    tok->f.kind = T_IDENTIFIER;
            }
            // ### is this correct w.r.t. the _scanKeywords?
            if (tok->f.kind == T_IDENTIFIER) {
                tok->f.kind = classifyOperator(yytext, yylen);

                if (control())
                    tok->identifier = control()->findOrInsertIdentifier(yytext, yylen);
            }
            break;
        } else if (std::isdigit(ch)) {
            const char *yytext = _currentChar - 1;
            while (_yychar) {
                if (_yychar == 'e' || _yychar == 'E') {
                    yyinp();
                    if (_yychar == '-' || _yychar == '+') {
                        yyinp();
                        // ### assert(std::isdigit(_yychar));
                    }
                } else if (std::isalnum(_yychar) || _yychar == '.') {
                    yyinp();
                } else {
                    break;
                }
            }
            int yylen = _currentChar - yytext;
            tok->f.kind = T_NUMERIC_LITERAL;
            if (control())
                tok->number = control()->findOrInsertNumericLiteral(yytext, yylen);
            break;
        } else {
            tok->f.kind = T_ERROR;
            break;
        }
    } // default

    } // switch
}


