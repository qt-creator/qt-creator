/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
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

#ifndef CPLUSPLUS_LEXER_H
#define CPLUSPLUS_LEXER_H

#include "CPlusPlusForwardDeclarations.h"
#include "Token.h"

CPLUSPLUS_BEGIN_HEADER
CPLUSPLUS_BEGIN_NAMESPACE

class CPLUSPLUS_EXPORT Lexer
{
    Lexer(const Lexer &other);
    void operator =(const Lexer &other);

public:
    enum State {
        DefaultState,
        MultiLineCommentState
    };

    Lexer(TranslationUnit *unit);
    Lexer(const char *firstChar, const char *lastChar);
    ~Lexer();

    Control *control() const;
    TranslationUnit *translationUnit() const;

    bool qtMocRunEnabled() const;
    void setQtMocRunEnabled(bool onoff);

    bool objCEnabled() const;
    void setObjCEnabled(bool onoff);

    void scan(Token *tok);

    inline void operator()(Token *tok)
    { scan(tok); }

    unsigned tokenOffset() const;
    unsigned tokenLength() const;
    const char *tokenBegin() const;
    const char *tokenEnd() const;
    unsigned currentLine() const;

    bool scanCommentTokens() const;
    void setScanCommentTokens(bool onoff);

    bool scanKeywords() const;
    void setScanKeywords(bool onoff);

    bool scanAngleStringLiteralTokens() const;
    void setScanAngleStringLiteralTokens(bool onoff);

    void setStartWithNewline(bool enabled);

    int state() const;
    void setState(int state);

    bool isIncremental() const;
    void setIncremental(bool isIncremental);

private:
    void scan_helper(Token *tok);
    void setSource(const char *firstChar, const char *lastChar);
    static int classify(const char *string, int length, bool q);
    static int classifyObjCAtKeyword(const char *s, int n);
    static int classifyOperator(const char *string, int length);

    inline void yyinp()
    {
        if (++_currentChar == _lastChar)
            _yychar = 0;
        else {
            _yychar = *_currentChar;
            if (_yychar == '\n')
                pushLineStartOffset();
        }
    }

    void pushLineStartOffset();

private:
    TranslationUnit *_translationUnit;
    const char *_firstChar;
    const char *_currentChar;
    const char *_lastChar;
    const char *_tokenStart;
    unsigned char _yychar;
    int _state;
    union {
        unsigned _flags;
        struct {
            unsigned _isIncremental: 1;
            unsigned _scanCommentTokens: 1;
            unsigned _scanKeywords: 1;
            unsigned _scanAngleStringLiteralTokens: 1;
            unsigned _qtMocRunEnabled: 1;
            unsigned _objCEnabled: 1;
        };
    };
    unsigned _currentLine;
};

CPLUSPLUS_END_NAMESPACE
CPLUSPLUS_END_HEADER

#endif // CPLUSPLUS_LEXER_H
