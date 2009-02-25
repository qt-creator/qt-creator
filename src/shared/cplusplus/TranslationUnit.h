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

#ifndef CPLUSPLUS_TRANSLATIONUNIT_H
#define CPLUSPLUS_TRANSLATIONUNIT_H

#include "CPlusPlusForwardDeclarations.h"
#include "ASTfwd.h"
#include "Token.h"
#include "Array.h"
#include <cstdio>
#include <vector> // ### remove me

CPLUSPLUS_BEGIN_HEADER
CPLUSPLUS_BEGIN_NAMESPACE

class CPLUSPLUS_EXPORT TranslationUnit
{
    TranslationUnit(const TranslationUnit &other);
    void operator =(const TranslationUnit &other);

public:
    TranslationUnit(Control *control, StringLiteral *fileId);
    ~TranslationUnit();

    Control *control() const;

    StringLiteral *fileId() const;
    const char *fileName() const;
    unsigned fileNameLength() const;

    const char *firstSourceChar() const;
    const char *lastSourceChar() const;
    unsigned sourceLength() const;

    void setSource(const char *source, unsigned size);

    unsigned tokenCount() const;
    const Token &tokenAt(unsigned index) const;
    int tokenKind(unsigned index) const;

    unsigned matchingBrace(unsigned index) const;
    Identifier *identifier(unsigned index) const;
    Literal *literal(unsigned index) const;
    StringLiteral *stringLiteral(unsigned index) const;
    NumericLiteral *numericLiteral(unsigned index) const;

    MemoryPool *memoryPool() const;
    AST *ast() const;

    bool blockErrors(bool block);

    bool qtMocRunEnabled() const;
    void setQtMocRunEnabled(bool onoff);

    bool objCEnabled() const;
    void setObjCEnabled(bool onoff);

    void warning(unsigned index, const char *fmt, ...);
    void error(unsigned index, const char *fmt, ...);
    void fatal(unsigned index, const char *fmt, ...);

    bool isTokenized() const;
    void tokenize();

    bool skipFunctionBody() const;
    void setSkipFunctionBody(bool skipFunctionBody);

    bool isParsed() const;

    enum ParseMode {
        ParseTranlationUnit,
        ParseDeclaration,
        ParseExpression,
        ParseStatement
    };

    bool parse(ParseMode mode = ParseTranlationUnit);

    void resetAST();
    void release();

    void getPosition(unsigned offset,
                     unsigned *line,
                     unsigned *column = 0,
                     StringLiteral **fileName = 0) const;

    void getTokenPosition(unsigned index,
                          unsigned *line,
                          unsigned *column = 0,
                          StringLiteral **fileName = 0) const;

    void pushLineOffset(unsigned offset);
    void pushPreprocessorLine(unsigned offset,
                              unsigned line,
                              StringLiteral *fileName);

public:
    struct PPLine {
        unsigned offset;
        unsigned line;
        StringLiteral *fileName;

        PPLine(unsigned offset = 0,
               unsigned line = 0,
               StringLiteral *fileName = 0)
            : offset(offset), line(line), fileName(fileName)
        { }

        bool operator == (const PPLine &other) const
        { return offset == other.offset; }

        bool operator != (const PPLine &other) const
        { return offset != other.offset; }

        bool operator < (const PPLine &other) const
        { return offset < other.offset; }
    };

private:
    unsigned findLineNumber(unsigned offset) const;
    unsigned findColumnNumber(unsigned offset, unsigned lineNumber) const;
    PPLine findPreprocessorLine(unsigned offset) const;
    void showErrorLine(unsigned index, unsigned column, FILE *out);

    Control *_control;
    StringLiteral *_fileId;
    const char *_firstSourceChar;
    const char *_lastSourceChar;
    Array<Token, 8> *_tokens;
    std::vector<unsigned> _lineOffsets;
    std::vector<PPLine> _ppLines;
    MemoryPool *_pool;
    AST *_ast;
    TranslationUnit *_previousTranslationUnit;
    union {
        unsigned _flags;
        struct {
            unsigned _tokenized: 1;
            unsigned _parsed: 1;
            unsigned _blockErrors: 1;
            unsigned _skipFunctionBody: 1;
            unsigned _qtMocRunEnabled: 1;
            unsigned _objCEnabled: 1;
        };
    };
};

CPLUSPLUS_END_NAMESPACE
CPLUSPLUS_END_HEADER

#endif // CPLUSPLUS_TRANSLATIONUNIT_H
