/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef CPLUSPLUS_PREPROCESSOR_H
#define CPLUSPLUS_PREPROCESSOR_H

#include <CPlusPlusForwardDeclarations.h>
#include <iosfwd>
#include <vector>
#include <map>
#include <cstring>

namespace CPlusPlus {

class Lexer;
class Token;

class CPLUSPLUS_EXPORT StringRef
{
    const char *_text;
    unsigned _size;

public:
    typedef const char *iterator;
    typedef const char *const_iterator;

    StringRef()
        : _text(0), _size(0) {}

    StringRef(const char *text, unsigned size)
        : _text(text), _size(size) {}

    StringRef(const char *text)
        : _text(text), _size(std::strlen(text)) {}

    inline const char *text() const { return _text; }
    inline unsigned size() const { return _size; }

    inline const_iterator begin() const { return _text; }
    inline const_iterator end() const { return _text + _size; }

    bool operator == (const StringRef &other) const
    {
        if (_size == other._size)
            return _text == other._text || ! std::strncmp(_text, other._text, _size);

        return false;
    }

    bool operator != (const StringRef &other) const
    { return ! operator == (other); }

    bool operator < (const StringRef &other) const
    { return std::lexicographical_compare(begin(), end(), other.begin(), other.end()); }
};

class CPLUSPLUS_EXPORT Preprocessor
{
public:
    Preprocessor(std::ostream &out);

    void operator()(const char *source, unsigned size, const StringRef &currentFileName);

private:
    struct Macro
    {
        Macro(): isFunctionLike(false), isVariadic(false) {}

        std::vector<StringRef> formals;
        std::vector<Token> body;
        bool isFunctionLike: 1;
        bool isVariadic: 1;
    };

    void run(const char *source, unsigned size);

    Lexer *switchLexer(Lexer *lex);
    StringRef switchSource(const StringRef &source);

    const Macro *resolveMacro(const StringRef &name) const;

    StringRef asStringRef(const Token &tk) const;
    void lex(Token *tk);
    bool isValidToken(const Token &tk) const;

    void handlePreprocessorDirective(Token *tk);
    void handleDefineDirective(Token *tk);
    void skipPreprocesorDirective(Token *tk);

    void collectActualArguments(Token *tk, std::vector<std::vector<Token> > *actuals);
    void scanActualArgument(Token *tk, std::vector<Token> *tokens);

private:
    struct TokenBuffer;

    std::ostream &out;
    StringRef _currentFileName;
    Lexer *_lexer;
    StringRef _source;
    TokenBuffer *_tokenBuffer;
    bool inPreprocessorDirective: 1;
    std::map<StringRef, Macro> macros;
};

} // end of namespace CPlusPlus

CPLUSPLUS_EXPORT std::ostream &operator << (std::ostream &out, const CPlusPlus::StringRef &s);

#endif // CPLUSPLUS_PREPROCESSOR_H

