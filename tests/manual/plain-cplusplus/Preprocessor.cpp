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

#include "Preprocessor.h"
#include "Lexer.h"
#include <list>
#include <iostream>
#include <cassert>

using namespace CPlusPlus;

std::ostream &operator << (std::ostream &out, const StringRef &s)
{
    out.write(s.text(), s.size());
    return out;
}

struct Preprocessor::TokenBuffer
{
    std::list<Token> tokens;
    const Macro *macro;
    TokenBuffer *next;

    template <typename _Iterator>
    TokenBuffer(_Iterator firstToken, _Iterator lastToken, const Macro *macro, TokenBuffer *next)
        : tokens(firstToken, lastToken), macro(macro), next(next) {}
};

Lexer *Preprocessor::switchLexer(Lexer *lex)
{
    Lexer *previousLexer = _lexer;
    _lexer = lex;
    return previousLexer;
}

StringRef Preprocessor::switchSource(const StringRef &source)
{
    StringRef previousSource = _source;
    _source = source;
    return previousSource;
}

const Preprocessor::Macro *Preprocessor::resolveMacro(const StringRef &name) const
{
    std::map<StringRef, Macro>::const_iterator it = macros.find(name);
    if (it != macros.end()) {
        const Macro *m = &it->second;
        for (TokenBuffer *r = _tokenBuffer; r; r = r->next) {
            if (r->macro == m)
                return 0;
        }
        return m;
    }

    return 0;
}

void Preprocessor::collectActualArguments(Token *tk, std::vector<std::vector<Token> > *actuals)
{
    lex(tk);

    assert(tk->is(T_LPAREN));

    lex(tk);

    std::vector<Token> tokens;
    scanActualArgument(tk, &tokens);

    actuals->push_back(tokens);

    while (tk->is(T_COMMA)) {
        lex(tk);

        std::vector<Token> tokens;
        scanActualArgument(tk, &tokens);
        actuals->push_back(tokens);
    }

    assert(tk->is(T_RPAREN));
    lex(tk);
}

void Preprocessor::scanActualArgument(Token *tk, std::vector<Token> *tokens)
{
    int count = 0;

    while (tk->isNot(T_EOF_SYMBOL)) {
        if (tk->is(T_LPAREN))
            ++count;

        else if (tk->is(T_RPAREN)) {
            if (! count)
                break;

            --count;
        }

        else if (! count && tk->is(T_COMMA))
            break;

        tokens->push_back(*tk);
        lex(tk);
    }
}

void Preprocessor::lex(Token *tk)
{
_Lagain:
    if (_tokenBuffer) {
        if (_tokenBuffer->tokens.empty()) {
            TokenBuffer *r = _tokenBuffer;
            _tokenBuffer = _tokenBuffer->next;
            delete r;
            goto _Lagain;
        }
        *tk = _tokenBuffer->tokens.front();
        _tokenBuffer->tokens.pop_front();
    } else {
        _lexer->scan(tk);
    }

_Lclassify:
    if (! inPreprocessorDirective) {
        if (tk->newline() && tk->is(T_POUND)) {
            handlePreprocessorDirective(tk);
            goto _Lclassify;

        } else if (tk->is(T_IDENTIFIER)) {
            const StringRef id = asStringRef(*tk);

            if (const Macro *macro = resolveMacro(id)) {
                std::vector<Token> body = macro->body;

                if (macro->isFunctionLike) {
                    std::vector<std::vector<Token> > actuals;
                    collectActualArguments(tk, &actuals);

                    std::vector<Token> expanded;
                    for (size_t i = 0; i < body.size(); ++i) {
                        const Token &token = body[i];

                        if (token.isNot(T_IDENTIFIER))
                            expanded.push_back(token);
                        else {
                            const StringRef id = asStringRef(token);
                            size_t j = 0;
                            for (; j < macro->formals.size(); ++j) {
                                if (macro->formals[j] == id) {
                                    expanded.insert(expanded.end(), actuals[j].begin(), actuals[j].end());
                                    break;
                                }
                            }

                            if (j == macro->formals.size())
                                expanded.push_back(token);
                        }
                    }

                    const Token currentTokenBuffer[] = { *tk };
                    _tokenBuffer = new TokenBuffer(currentTokenBuffer, currentTokenBuffer + 1,
                                                   /*macro */ 0, _tokenBuffer);

                    body = expanded;
                }

                _tokenBuffer = new TokenBuffer(body.begin(), body.end(),
                                               macro, _tokenBuffer);
                goto _Lagain;
            }
        }
    }
}

void Preprocessor::handlePreprocessorDirective(Token *tk)
{
    inPreprocessorDirective = true;

    lex(tk); // scan the directive

    if (tk->newline() && ! tk->joined())
        return; // nothing to do.

    const StringRef ppDefine("define", 6);

    if (tk->is(T_IDENTIFIER)) {
        const StringRef directive = asStringRef(*tk);

        if (directive == ppDefine)
            handleDefineDirective(tk);
        else
            skipPreprocesorDirective(tk);
    }

    inPreprocessorDirective = false;
}

bool Preprocessor::isValidToken(const Token &tk) const
{
    if (tk.isNot(T_EOF_SYMBOL) && (! tk.newline() || tk.joined()))
        return true;

    return false;
}

void Preprocessor::handleDefineDirective(Token *tk)
{
    lex(tk);

    if (tk->is(T_IDENTIFIER)) {
        const StringRef macroName = asStringRef(*tk);
        Macro macro;

        lex(tk);

        if (isValidToken(*tk) && tk->is(T_LPAREN) && ! tk->whitespace()) {
            macro.isFunctionLike = true;

            lex(tk); // skip `('

            if (isValidToken(*tk) && tk->is(T_IDENTIFIER)) {
                macro.formals.push_back(asStringRef(*tk));

                lex(tk);

                while (isValidToken(*tk) && tk->is(T_COMMA)) {
                    lex(tk);

                    if (isValidToken(*tk) && tk->is(T_IDENTIFIER)) {
                        macro.formals.push_back(asStringRef(*tk));
                        lex(tk);
                    }
                }
            }

            if (isValidToken(*tk) && tk->is(T_RPAREN))
                lex(tk); // skip `)'
        }

        while (isValidToken(*tk)) {
            macro.body.push_back(*tk);
            lex(tk);
        }

        macros.insert(std::make_pair(macroName, macro));
    } else {
        skipPreprocesorDirective(tk);
    }
}

void Preprocessor::skipPreprocesorDirective(Token *tk)
{
    do {
        lex(tk);
    } while (isValidToken(*tk));
}

StringRef Preprocessor::asStringRef(const Token &tk) const
{ return StringRef(_source.begin() + tk.begin(), tk.length()); }

Preprocessor::Preprocessor(std::ostream &out)
    : out(out), _lexer(0), inPreprocessorDirective(false)
{ }

void Preprocessor::operator()(const char *source, unsigned size, const StringRef &currentFileName)
{
    _currentFileName = currentFileName;
    run(source, size);
}

void Preprocessor::run(const char *source, unsigned size)
{
    _tokenBuffer = 0;

    const StringRef previousSource = switchSource(StringRef(source, size));
    Lexer thisLexer(source, source + size);
    thisLexer.setScanKeywords(false);
    Lexer *previousLexer = switchLexer(&thisLexer);
    inPreprocessorDirective = false;

    Token tk;
    unsigned lineno = 0;
    do {
        lex(&tk);

        if (lineno != tk.lineno) {
            if (lineno > tk.lineno || tk.lineno - lineno > 3)
                out << std::endl << "#line " << tk.lineno << " \"" << _currentFileName << "\"" << std::endl;

            else {
                for (unsigned i = lineno; i < tk.lineno; ++i)
                    out << std::endl;
            }

            lineno = tk.lineno;

        } else {
            if (tk.newline())
                out << std::endl;

            if (tk.whitespace())
                out << ' ';
        }

        out << asStringRef(tk);
        lineno = tk.lineno;
    } while (tk.isNot(T_EOF_SYMBOL));

    out << std::endl;

    switchLexer(previousLexer);
    switchSource(previousSource);
}




