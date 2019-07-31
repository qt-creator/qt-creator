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

#pragma once

#include "CPlusPlusForwardDeclarations.h"
#include "ASTfwd.h"
#include "Token.h"
#include "DiagnosticClient.h"
#include <cstdio>
#include <unordered_map>
#include <vector>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT TranslationUnit
{
    TranslationUnit(const TranslationUnit &other);
    void operator =(const TranslationUnit &other);

public:
    TranslationUnit(Control *control, const StringLiteral *fileId);
    ~TranslationUnit();

    Control *control() const;

    const StringLiteral *fileId() const;
    const char *fileName() const;
    int fileNameLength() const;

    const char *firstSourceChar() const;
    const char *lastSourceChar() const;
    int sourceLength() const;

    void setSource(const char *source, int size);

    int tokenCount() const { return _tokens ? int(_tokens->size()) : 0; }
    const Token &tokenAt(int index) const
    { return _tokens && index < tokenCount() ? (*_tokens)[index] : nullToken; }

    Kind tokenKind(int index) const { return tokenAt(index).kind(); }
    const char *spell(int index) const;

    int commentCount() const;
    const Token &commentAt(int index) const;

    int matchingBrace(int index) const;
    const Identifier *identifier(int index) const;
    const Literal *literal(int index) const;
    const StringLiteral *stringLiteral(int index) const;
    const NumericLiteral *numericLiteral(int index) const;

    MemoryPool *memoryPool() const;
    AST *ast() const;

    bool blockErrors() const { return f._blockErrors; }
    bool blockErrors(bool block)
    {
        const bool previous = f._blockErrors;
        f._blockErrors = block;
        return previous;
    }

    void warning(int index, const char *fmt, ...);
    void error(int index, const char *fmt, ...);
    void fatal(int index, const char *fmt, ...);

    void message(DiagnosticClient::Level level, int index,
                 const char *format, va_list ap);

    bool isTokenized() const;
    void tokenize();

    bool skipFunctionBody() const;
    void setSkipFunctionBody(bool skipFunctionBody);

    bool isParsed() const;

    enum ParseMode {
        ParseTranlationUnit,
        ParseDeclaration,
        ParseExpression,
        ParseDeclarator,
        ParseStatement
    };

    bool parse(ParseMode mode = ParseTranlationUnit);

    void resetAST();
    void release();

    void getTokenStartPosition(int index, int *line,
                               int *column = nullptr,
                               const StringLiteral **fileName = nullptr) const;

    void getTokenEndPosition(int index, int *line,
                             int *column = nullptr,
                             const StringLiteral **fileName = nullptr) const;

    void getPosition(int utf16charOffset,
                     int *line,
                     int *column = nullptr,
                     const StringLiteral **fileName = nullptr) const;

    void getTokenPosition(int index,
                          int *line,
                          int *column = nullptr,
                          const StringLiteral **fileName = nullptr) const;

    void pushLineOffset(int offset);
    void pushPreprocessorLine(int utf16charOffset,
                              int line,
                              const StringLiteral *fileName);

    int findPreviousLineOffset(int tokenIndex) const;

    bool maybeSplitGreaterGreaterToken(int tokenIndex);

    LanguageFeatures languageFeatures() const { return _languageFeatures; }
    void setLanguageFeatures(LanguageFeatures features) { _languageFeatures = features; }

private:
    struct PPLine {
        int utf16charOffset;
        int line;
        const StringLiteral *fileName;

        PPLine(int utf16charOffset = 0,
               int line = 0,
               const StringLiteral *fileName = nullptr)
            : utf16charOffset(utf16charOffset), line(line), fileName(fileName)
        { }

        bool operator == (const PPLine &other) const
        { return utf16charOffset == other.utf16charOffset; }

        bool operator != (const PPLine &other) const
        { return utf16charOffset != other.utf16charOffset; }

        bool operator < (const PPLine &other) const
        { return utf16charOffset < other.utf16charOffset; }
    };

    void releaseTokensAndComments();
    int findLineNumber(int utf16charOffset) const;
    int findColumnNumber(int utf16CharOffset, int lineNumber) const;
    PPLine findPreprocessorLine(int utf16charOffset) const;

    static const Token nullToken;

    Control *_control;
    const StringLiteral *_fileId;
    const char *_firstSourceChar;
    const char *_lastSourceChar;
    std::vector<Token> *_tokens;
    std::vector<Token> *_comments;
    std::vector<int> _lineOffsets;
    std::vector<PPLine> _ppLines;
    typedef std::unordered_map<unsigned, std::pair<int, int> > TokenLineColumn;
    TokenLineColumn _expandedLineColumn;
    MemoryPool *_pool;
    AST *_ast;
    TranslationUnit *_previousTranslationUnit;
    struct Flags {
        unsigned _tokenized: 1;
        unsigned _parsed: 1;
        unsigned _blockErrors: 1;
        unsigned _skipFunctionBody: 1;
    };
    union {
        unsigned _flags;
        Flags f;
    };
    LanguageFeatures _languageFeatures;
};

} // namespace CPlusPlus
