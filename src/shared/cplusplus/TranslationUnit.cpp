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

#include "TranslationUnit.h"
#include "Control.h"
#include "Parser.h"
#include "Lexer.h"
#include "MemoryPool.h"
#include "AST.h"
#include "Literals.h"
#include "DiagnosticClient.h"
#include <stack>
#include <cstdlib>
#include <cstdarg>
#include <algorithm>

CPLUSPLUS_BEGIN_NAMESPACE

TranslationUnit::TranslationUnit(Control *control, StringLiteral *fileId)
    : _control(control),
      _fileId(fileId),
      _firstSourceChar(0),
      _lastSourceChar(0),
      _pool(0),
      _ast(0),
      _flags(0)
{
    _tokens = new Array<Token, 8>();
    _previousTranslationUnit = control->switchTranslationUnit(this);
    _pool = new MemoryPool();
}

TranslationUnit::~TranslationUnit()
{
    (void) _control->switchTranslationUnit(_previousTranslationUnit);
    delete _tokens;
    delete _pool;
}

bool TranslationUnit::qtMocRunEnabled() const
{ return _qtMocRunEnabled; }

void TranslationUnit::setQtMocRunEnabled(bool onoff)
{ _qtMocRunEnabled = onoff; }

bool TranslationUnit::objCEnabled() const
{ return _objCEnabled; }

void TranslationUnit::setObjCEnabled(bool onoff)
{ _objCEnabled = onoff; }

Control *TranslationUnit::control() const
{ return _control; }

StringLiteral *TranslationUnit::fileId() const
{ return _fileId; }

const char *TranslationUnit::fileName() const
{ return _fileId->chars(); }

unsigned TranslationUnit::fileNameLength() const
{ return _fileId->size(); }

const char *TranslationUnit::firstSourceChar() const
{ return _firstSourceChar; }

const char *TranslationUnit::lastSourceChar() const
{ return _lastSourceChar; }

unsigned TranslationUnit::sourceLength() const
{ return _lastSourceChar - _firstSourceChar; }

void TranslationUnit::setSource(const char *source, unsigned size)
{
    _firstSourceChar = source;
    _lastSourceChar = source + size;
}

unsigned TranslationUnit::tokenCount() const
{ return _tokens->size(); }

const Token &TranslationUnit::tokenAt(unsigned index) const
{ return _tokens->at(index); }

int TranslationUnit::tokenKind(unsigned index) const
{ return _tokens->at(index).kind; }

Identifier *TranslationUnit::identifier(unsigned index) const
{ return _tokens->at(index).identifier; }

Literal *TranslationUnit::literal(unsigned index) const
{ return _tokens->at(index).literal; }

StringLiteral *TranslationUnit::stringLiteral(unsigned index) const
{ return _tokens->at(index).string; }

NumericLiteral *TranslationUnit::numericLiteral(unsigned index) const
{ return _tokens->at(index).number; }

unsigned TranslationUnit::matchingBrace(unsigned index) const
{ return _tokens->at(index).close_brace; }

MemoryPool *TranslationUnit::memoryPool() const
{ return _pool; }

AST *TranslationUnit::ast() const
{ return _ast; }

bool TranslationUnit::isTokenized() const
{ return _tokenized; }

bool TranslationUnit::isParsed() const
{ return _parsed; }

void TranslationUnit::tokenize()
{
    if (isTokenized())
        return;

    _tokenized = true;

    Lexer lex(this);
    lex.setQtMocRunEnabled(_qtMocRunEnabled);
    lex.setObjCEnabled(_objCEnabled);

    std::stack<unsigned> braces;
    _tokens->push_back(Token()); // the first token needs to be invalid!

    pushLineOffset(0);
    pushPreprocessorLine(0, 1, fileId());

    Identifier *lineId = control()->findOrInsertIdentifier("line");

    Token tk;
    do {
        lex(&tk);

      _Lrecognize:
        if (tk.is(T_POUND)) {
            unsigned offset = tk.offset;
            lex(&tk);
            if (! tk.newline && tk.is(T_IDENTIFIER) && tk.identifier == lineId)
                lex(&tk);
            if (! tk.newline && tk.is(T_INT_LITERAL)) {
                unsigned line = (unsigned) strtoul(tk.spell(), 0, 0);
                lex(&tk);
                if (! tk.newline && tk.is(T_STRING_LITERAL)) {
                    StringLiteral *fileName = control()->findOrInsertFileName(tk.string->chars(),
                                                                              tk.string->size());
                    pushPreprocessorLine(offset, line, fileName);
                    lex(&tk);
                }
            }
            while (tk.isNot(T_EOF_SYMBOL) && ! tk.newline)
                lex(&tk);
            goto _Lrecognize;
        } else if (tk.kind == T_LBRACE) {
            braces.push(_tokens->size());
        } else if (tk.kind == T_RBRACE && ! braces.empty()) {
            const unsigned open_brace_index = braces.top();
            braces.pop();
            (*_tokens)[open_brace_index].close_brace = _tokens->size();
        }
        _tokens->push_back(tk);
    } while (tk.kind);

    for (; ! braces.empty(); braces.pop()) {
        unsigned open_brace_index = braces.top();
        (*_tokens)[open_brace_index].close_brace = _tokens->size();
    }
}

bool TranslationUnit::skipFunctionBody() const
{ return _skipFunctionBody; }

void TranslationUnit::setSkipFunctionBody(bool skipFunctionBody)
{ _skipFunctionBody = skipFunctionBody; }

bool TranslationUnit::parse(ParseMode mode)
{
    if (isParsed())
        return false;

    if (! isTokenized())
        tokenize();

    Parser parser(this);
    parser.setQtMocRunEnabled(_qtMocRunEnabled);
    parser.setObjCEnabled(_objCEnabled);

    bool parsed = false;

    switch (mode) {
    case ParseTranlationUnit: {
        TranslationUnitAST *node = 0;
        parsed = parser.parseTranslationUnit(node);
        _ast = node;
    } break;

    case ParseDeclaration: {
        DeclarationAST *node = 0;
        parsed = parser.parseDeclaration(node);
        _ast = node;
    } break;

    case ParseExpression: {
        ExpressionAST *node = 0;
        parsed = parser.parseExpression(node);
        _ast = node;
    } break;

    case ParseStatement: {
        StatementAST *node = 0;
        parsed = parser.parseStatement(node);
        _ast = node;
    } break;

    default:
        break;
    } // switch

    return parsed;
}

void TranslationUnit::pushLineOffset(unsigned offset)
{ _lineOffsets.push_back(offset); }

void TranslationUnit::pushPreprocessorLine(unsigned offset,
                                           unsigned line,
                                           StringLiteral *fileName)
{ _ppLines.push_back(PPLine(offset, line, fileName)); }

unsigned TranslationUnit::findLineNumber(unsigned offset) const
{
    std::vector<unsigned>::const_iterator it =
        std::lower_bound(_lineOffsets.begin(), _lineOffsets.end(), offset);

    if (it != _lineOffsets.begin())
        --it;

    return it - _lineOffsets.begin();
}

TranslationUnit::PPLine TranslationUnit::findPreprocessorLine(unsigned offset) const
{
    std::vector<PPLine>::const_iterator it =
        std::lower_bound(_ppLines.begin(), _ppLines.end(), PPLine(offset));

    if (it != _ppLines.begin())
        --it;

    return *it;
}

unsigned TranslationUnit::findColumnNumber(unsigned offset, unsigned lineNumber) const
{
    if (! offset)
        return 0;

    return offset - _lineOffsets[lineNumber];
}

void TranslationUnit::getTokenPosition(unsigned index,
                                       unsigned *line,
                                       unsigned *column,
                                       StringLiteral **fileName) const
{ return getPosition(tokenAt(index).offset, line, column, fileName); }

void TranslationUnit::getPosition(unsigned tokenOffset,
                                  unsigned *line,
                                  unsigned *column,
                                  StringLiteral **fileName) const
{
    unsigned lineNumber = findLineNumber(tokenOffset);
    unsigned columnNumber = findColumnNumber(tokenOffset, lineNumber);
    const PPLine ppLine = findPreprocessorLine(tokenOffset);

    lineNumber -= findLineNumber(ppLine.offset) + 1;
    lineNumber += ppLine.line;

    if (line)
        *line = lineNumber;

    if (column)
        *column = columnNumber;

    if (fileName)
       *fileName = ppLine.fileName;
}

bool TranslationUnit::blockErrors(bool block)
{
    bool previous = _blockErrors;
    _blockErrors = block;
    return previous;
}

void TranslationUnit::warning(unsigned index, const char *format, ...)
{
    if (_blockErrors)
        return;

    index = std::min(index, tokenCount() - 1);

    unsigned line = 0, column = 0;
    StringLiteral *fileName = 0;
    getTokenPosition(index, &line, &column, &fileName);

    if (DiagnosticClient *client = control()->diagnosticClient()) {
        va_list args;
        va_start(args, format);
        client->report(DiagnosticClient::Warning, fileName, line, column,
                       format, args);
        va_end(args);
    } else {
        fprintf(stderr, "%s:%d: ", fileName->chars(), line);
        fprintf(stderr, "warning: ");

        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        fputc('\n', stderr);

        showErrorLine(index, column, stderr);
    }
}

void TranslationUnit::error(unsigned index, const char *format, ...)
{
    if (_blockErrors)
        return;

    index = std::min(index, tokenCount() - 1);

    unsigned line = 0, column = 0;
    StringLiteral *fileName = 0;
    getTokenPosition(index, &line, &column, &fileName);

    if (DiagnosticClient *client = control()->diagnosticClient()) {
        va_list args;
        va_start(args, format);
        client->report(DiagnosticClient::Error, fileName, line, column,
                       format, args);
        va_end(args);
    } else {
        fprintf(stderr, "%s:%d: ", fileName->chars(), line);
        fprintf(stderr, "error: ");

        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        fputc('\n', stderr);

        showErrorLine(index, column, stderr);
    }
}

void TranslationUnit::fatal(unsigned index, const char *format, ...)
{
    if (_blockErrors)
        return;

    index = std::min(index, tokenCount() - 1);

    unsigned line = 0, column = 0;
    StringLiteral *fileName = 0;
    getTokenPosition(index, &line, &column, &fileName);

    if (DiagnosticClient *client = control()->diagnosticClient()) {
        va_list args;
        va_start(args, format);
        client->report(DiagnosticClient::Fatal, fileName, line, column,
                       format, args);
        va_end(args);
    } else {
        fprintf(stderr, "%s:%d: ", fileName->chars(), line);
        fprintf(stderr, "fatal: ");

        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        fputc('\n', stderr);

        showErrorLine(index, column, stderr);
    }

    exit(EXIT_FAILURE);
}

void TranslationUnit::showErrorLine(unsigned index, unsigned column, FILE *out)
{
    unsigned lineOffset = _lineOffsets[findLineNumber(_tokens->at(index).offset)];
    for (const char *cp = _firstSourceChar + lineOffset + 1; *cp && *cp != '\n'; ++cp) {
        fputc(*cp, out);
    }
    fputc('\n', out);

    const char *end = _firstSourceChar + lineOffset + 1 + column - 1;
    for (const char *cp = _firstSourceChar + lineOffset + 1; cp != end; ++cp) {
        if (*cp != '\t')
            fputc(' ', out);
        else
            fputc('\t', out);
    }
    fputc('^', out);
    fputc('\n', out);
}

void TranslationUnit::resetAST()
{
    delete _pool;
    _pool = 0;
}

void TranslationUnit::release()
{
    resetAST();
    delete _tokens;
    _tokens = 0;
}

CPLUSPLUS_END_NAMESPACE
