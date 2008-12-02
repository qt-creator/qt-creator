/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
/*
  Copyright 2005 Roberto Raggi <roberto@kdevelop.org>

  Permission to use, copy, modify, distribute, and sell this software and its
  documentation for any purpose is hereby granted without fee, provided that
  the above copyright notice appear in all copies and that both that
  copyright notice and this permission notice appear in supporting
  documentation.

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
  KDEVELOP TEAM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
  AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "pp.h"

#include <Lexer.h>
#include <Token.h>
#include <QtDebug>
#include <algorithm>

using namespace rpp;
using namespace CPlusPlus;

namespace {

class RangeLexer
{
    const Token *first;
    const Token *last;
    Token trivial;

public:
    inline RangeLexer(const Token *first, const Token *last)
        : first(first), last(last)
    {
        // WARN: `last' must be a valid iterator.
        trivial.offset = last->offset;
    }

    inline operator bool() const
    { return first != last; }

    inline bool isValid() const
    { return first != last; }

    inline int size() const
    { return std::distance(first, last); }

    inline const Token *dot() const
    { return first; }

    inline const Token &operator*() const
    {
        if (first != last)
            return *first;

        return trivial;
    }

    inline const Token *operator->() const
    {
        if (first != last)
            return first;

        return &trivial;
    }

    inline RangeLexer &operator++()
    {
        ++first;
        return *this;
    }
};

class ExpressionEvaluator
{
    ExpressionEvaluator(const ExpressionEvaluator &other);
    void operator = (const ExpressionEvaluator &other);

public:
    ExpressionEvaluator(Environment *env)
        : env(env), _lex(0)
    { }

    Value operator()(const Token *firstToken, const Token *lastToken,
                     const QByteArray &source)
    {
        this->source = source;
        const Value previousValue = switchValue(Value());
        RangeLexer tmp(firstToken, lastToken);
        RangeLexer *previousLex = _lex;
        _lex = &tmp;
        process_expression();
        _lex = previousLex;
        return switchValue(previousValue);
    }

protected:
    Value switchValue(const Value &value)
    {
        Value previousValue = _value;
        _value = value;
        return previousValue;
    }

    bool isTokenDefined() const
    {
        if ((*_lex)->isNot(T_IDENTIFIER))
            return false;
        const QByteArray spell = tokenSpell();
        if (spell.size() != 7)
            return false;
        return spell == "defined";
    }

    QByteArray tokenSpell() const
    {
        const QByteArray text = QByteArray::fromRawData(source.constData() + (*_lex)->offset,
                                                        (*_lex)->length);
        return text;
    }

    bool process_expression()
    { return process_constant_expression(); }

    bool process_primary()
    {
        if ((*_lex)->is(T_INT_LITERAL)) {
            _value.set_long(tokenSpell().toLong());
            ++(*_lex);
            return true;
        } else if (isTokenDefined()) {
            ++(*_lex);
            if ((*_lex)->is(T_IDENTIFIER)) {
                _value.set_long(env->resolve(tokenSpell()) != 0);
                ++(*_lex);
                return true;
            } else if ((*_lex)->is(T_LPAREN)) {
                ++(*_lex);
                if ((*_lex)->is(T_IDENTIFIER)) {
                    _value.set_long(env->resolve(tokenSpell()) != 0);
                    ++(*_lex);
                    if ((*_lex)->is(T_RPAREN)) {
                        ++(*_lex);
                        return true;
                    }
                }
                return false;
            }
            return true;
        } else if ((*_lex)->is(T_IDENTIFIER)) {
            _value.set_long(0);
            ++(*_lex);
            return true;
        } else if ((*_lex)->is(T_MINUS)) {
            ++(*_lex);
            process_primary();
            _value.set_long(- _value.l);
            return true;
        } else if ((*_lex)->is(T_PLUS)) {
            ++(*_lex);
            process_primary();
            return true;
        } else if ((*_lex)->is(T_EXCLAIM)) {
            ++(*_lex);
            process_primary();
            _value.set_long(_value.is_zero());
            return true;
        } else if ((*_lex)->is(T_LPAREN)) {
            ++(*_lex);
            process_expression();
            if ((*_lex)->is(T_RPAREN))
                ++(*_lex);
            return true;
        }

        return false;
    }

    bool process_multiplicative()
    {
        process_primary();

        while ((*_lex)->is(T_STAR) || (*_lex)->is(T_SLASH) || (*_lex)->is(T_PERCENT)) {
            const Token op = *(*_lex);
            ++(*_lex);

            const Value left = _value;
            process_primary();

            if (op.is(T_STAR)) {
                _value = left * _value;
            } else if (op.is(T_SLASH)) {
                if (_value.is_zero())
                    _value.set_long(0);
                else
                    _value = left / _value;
            } else if (op.is(T_PERCENT)) {
                if (_value.is_zero())
                    _value.set_long(0);
                else
                    _value = left % _value;
            }
        }

        return true;
    }

    bool process_additive()
    {
        process_multiplicative();

        while ((*_lex)->is(T_PLUS) || (*_lex)->is(T_MINUS)) {
            const Token op = *(*_lex);
            ++(*_lex);

            const Value left = _value;
            process_multiplicative();

            if (op.is(T_PLUS))
                _value = left + _value;
            else if (op.is(T_MINUS))
                _value = left - _value;
        }

        return true;
    }

    bool process_shift()
    {
        process_additive();

        while ((*_lex)->is(T_MINUS_MINUS) || (*_lex)->is(T_GREATER_GREATER)) {
            const Token op = *(*_lex);
            ++(*_lex);

            const Value left = _value;
            process_additive();

            if (op.is(T_MINUS_MINUS))
                _value = left << _value;
            else if (op.is(T_GREATER_GREATER))
                _value = left >> _value;
        }

        return true;
    }

    bool process_relational()
    {
        process_shift();

        while ((*_lex)->is(T_LESS)    || (*_lex)->is(T_LESS_EQUAL) ||
               (*_lex)->is(T_GREATER) || (*_lex)->is(T_GREATER_EQUAL)) {
            const Token op = *(*_lex);
            ++(*_lex);

            const Value left = _value;
            process_shift();

            if (op.is(T_LESS))
                _value = left < _value;
            else if (op.is(T_LESS_EQUAL))
                _value = left <= _value;
            else if (op.is(T_GREATER))
                _value = left > _value;
            else if (op.is(T_GREATER_EQUAL))
                _value = left >= _value;
        }

        return true;
    }

    bool process_equality()
    {
        process_relational();

        while ((*_lex)->is(T_EXCLAIM_EQUAL) || (*_lex)->is(T_EQUAL_EQUAL)) {
            const Token op = *(*_lex);
            ++(*_lex);

            const Value left = _value;
            process_relational();

            if (op.is(T_EXCLAIM_EQUAL))
                _value = left != _value;
            else if (op.is(T_EQUAL_EQUAL))
                _value = left == _value;
        }

        return true;
    }

    bool process_and()
    {
        process_equality();

        while ((*_lex)->is(T_AMPER)) {
            const Token op = *(*_lex);
            ++(*_lex);

            const Value left = _value;
            process_equality();

            _value = left & _value;
        }

        return true;
    }

    bool process_xor()
    {
        process_and();

        while ((*_lex)->is(T_CARET)) {
            const Token op = *(*_lex);
            ++(*_lex);

            const Value left = _value;
            process_and();

            _value = left ^ _value;
        }

        return true;
    }

    bool process_or()
    {
        process_xor();

        while ((*_lex)->is(T_CARET)) {
            const Token op = *(*_lex);
            ++(*_lex);

            const Value left = _value;
            process_xor();

            _value = left | _value;
        }

        return true;
    }

    bool process_logical_and()
    {
        process_or();

        while ((*_lex)->is(T_AMPER_AMPER)) {
            const Token op = *(*_lex);
            ++(*_lex);

            const Value left = _value;
            process_or();

            _value = left && _value;
        }

        return true;
    }

    bool process_logical_or()
    {
        process_logical_and();

        while ((*_lex)->is(T_PIPE_PIPE)) {
            const Token op = *(*_lex);
            ++(*_lex);

            const Value left = _value;
            process_logical_and();

            _value = left || _value;
        }

        return true;
    }

    bool process_constant_expression()
    {
        process_logical_or();
        const Value cond = _value;
        if ((*_lex)->is(T_QUESTION)) {
            ++(*_lex);
            process_constant_expression();
            Value left = _value, right;
            if ((*_lex)->is(T_COLON)) {
                ++(*_lex);
                process_constant_expression();
                right = _value;
            }
            _value = ! cond.is_zero() ? left : right;
        }

        return true;
    }

private:
    Environment *env;
    QByteArray source;
    RangeLexer *_lex;
    Value _value;
};

} // end of anonymous namespace


pp::pp (Client *client, Environment &env)
    : client(client),
      env(env),
      expand(env)
{
    resetIfLevel ();
}

void pp::pushState(const State &s)
{
    _savedStates.append(state());
    _source = s.source;
    _tokens = s.tokens;
    _dot = s.dot;
}

pp::State pp::state() const
{
    State state;
    state.source = _source;
    state.tokens = _tokens;
    state.dot = _dot;
    return state;
}

void pp::popState()
{
    const State &state = _savedStates.last();
    _source = state.source;
    _tokens = state.tokens;
    _dot = state.dot;
    _savedStates.removeLast();
}

void pp::operator () (const QByteArray &filename,
                      const QByteArray &source,
                      QByteArray *result)
{
    const QByteArray previousFile = env.current_file;
    env.current_file = filename;

    operator () (source, result);

    env.current_file = previousFile;
}

pp::State pp::createStateFromSource(const QByteArray &source) const
{
    State state;
    state.source = source;
    Lexer lex(state.source.constBegin(), state.source.constEnd());
    lex.setScanKeywords(false);
    Token tok;
    do {
        lex(&tok);
        state.tokens.append(tok);
    } while (tok.isNot(T_EOF_SYMBOL));
    state.dot = state.tokens.constBegin();
    return state;
}

void pp::operator()(const QByteArray &source, QByteArray *result)
{
    pushState(createStateFromSource(source));

    const unsigned previousCurrentLine = env.currentLine;
    env.currentLine = 0;

    while (true) {
        if (env.currentLine != _dot->lineno) {
            if (env.currentLine > _dot->lineno) {
                result->append('\n');
                result->append('#');
                result->append(QByteArray::number(_dot->lineno));
                result->append(' ');
                result->append('"');
                result->append(env.current_file);
                result->append('"');
                result->append('\n');
            } else {
                for (unsigned i = env.currentLine; i < _dot->lineno; ++i)
                    result->append('\n');
            }
            env.currentLine = _dot->lineno;
        }

        if (_dot->is(T_EOF_SYMBOL)) {
            break;
        } else if (_dot->is(T_POUND) && (! _dot->joined && _dot->newline)) {
            TokenIterator start = _dot;
            do {
                ++_dot;
            } while (_dot->isNot(T_EOF_SYMBOL) && (_dot->joined || ! _dot->newline));

            //qDebug() << QByteArray(first + beginPP.offset,
                                   //tokens.last().end() - beginPP.offset);

            const bool skippingBlocks = _skipping[iflevel];

            processDirective(start, _dot);

            if (client && skippingBlocks != _skipping[iflevel]) {
                unsigned offset = start->offset;
                if (_skipping[iflevel]) {
                    if (_dot->newline)
                        ++offset;
                    client->startSkippingBlocks(offset);
                } else {
                    if (offset)
                        --offset;
                    client->stopSkippingBlocks(offset);
                }
            }
        } else if (skipping()) {
            do {
                ++_dot;
            } while (_dot->isNot(T_EOF_SYMBOL) && (_dot->joined || ! _dot->newline));
        } else {
            if (_dot->joined)
                result->append("\\\n");
            else if (_dot->newline) {
                result->append('\n');
                result->append('#');
                result->append(QByteArray::number(_dot->lineno));
                result->append(' ');
                result->append('"');
                result->append(env.current_file);
                result->append('"');
                result->append('\n');
            }
            else if (_dot->whitespace)
                result->append(' ');

            if (_dot->isNot(T_IDENTIFIER)) {
                result->append(tokenSpell(*_dot));
                ++_dot;
            } else {
                const TokenIterator identifierToken = _dot;
                ++_dot; // skip T_IDENTIFIER

                const QByteArray spell = tokenSpell(*identifierToken);
                if (env.isBuiltinMacro(spell)) {
                    expand(spell.constBegin(), spell.constEnd(), result);
                    continue;
                }

                Macro *m = env.resolve(spell);
                if (! m) {
                    result->append(spell);
                } else {
                    if (! m->function_like) {
                        if (_dot->isNot(T_LPAREN)) {
                            expand(m->definition.constBegin(),
                                   m->definition.constEnd(),
                                   result);
                            continue;
                        } else {
                            QByteArray tmp;
                            expand(m->definition.constBegin(),
                                   m->definition.constEnd(),
                                   &tmp);

                            m = 0; // reset the active the macro

                            pushState(createStateFromSource(tmp));
                            if (_dot->is(T_IDENTIFIER)) {
                                const QByteArray id = tokenSpell(*_dot);
                                Macro *macro = env.resolve(id);
                                if (macro && macro->function_like)
                                    m = macro;
                            }
                            popState();

                            if (! m) {
                                result->append(tmp);
                                continue;
                            }
                        }
                    }

                    // collect the actual arguments
                    if (_dot->isNot(T_LPAREN)) {
                        // ### warnng expected T_LPAREN
                        result->append(m->name);
                        continue;
                    }

                    int count = 0;
                    while (_dot->isNot(T_EOF_SYMBOL)) {
                        if (_dot->is(T_LPAREN))
                            ++count;
                        else if (_dot->is(T_RPAREN)) {
                            if (! --count)
                                break;
                        }
                        ++_dot;
                    }
                    if (_dot->isNot(T_RPAREN)) {
                        // ### warning expected T_RPAREN
                    } else {
                        const char *beginOfText = startOfToken(*identifierToken);
                        const char *endOfText = endOfToken(*_dot);
                        ++_dot; // skip T_RPAREN
                        expand(beginOfText, endOfText, result);
                    }
                }
            }
        }
    }

    popState();
    env.currentLine = previousCurrentLine;
}

const char *pp::startOfToken(const Token &token) const
{ return _source.constBegin() + token.begin(); }

const char *pp::endOfToken(const Token &token) const
{ return _source.constBegin() + token.end(); }

QByteArray pp::tokenSpell(const Token &token) const
{
    const QByteArray text = QByteArray::fromRawData(_source.constBegin() + token.offset,
                                                    token.length);
    return text;
}

QByteArray pp::tokenText(const Token &token) const
{
    const QByteArray text(_source.constBegin() + token.offset,
                          token.length);
    return text;
}

void pp::processDirective(TokenIterator firstToken, TokenIterator lastToken)
{
    RangeLexer tk(firstToken, lastToken);
    ++tk; // skip T_POUND

    if (tk->is(T_IDENTIFIER)) {
        const QByteArray directive = tokenSpell(*tk);
        switch (PP_DIRECTIVE_TYPE d = classifyDirective(directive)) {
        case PP_DEFINE:
            if (! skipping())
                processDefine(firstToken, lastToken);
            break;

        case PP_INCLUDE:
        case PP_INCLUDE_NEXT:
            if (! skipping())
                processInclude(d == PP_INCLUDE_NEXT, firstToken, lastToken);
            break;

        case PP_UNDEF:
            if (! skipping())
                processUndef(firstToken, lastToken);
            break;

        case PP_ELIF:
            processElif(firstToken, lastToken);
            break;

        case PP_ELSE:
            processElse(firstToken, lastToken);
            break;

        case PP_ENDIF:
            processEndif(firstToken, lastToken);
            break;

        case PP_IF:
            processIf(firstToken, lastToken);
            break;

        case PP_IFDEF:
        case PP_IFNDEF:
            processIfdef(d == PP_IFNDEF, firstToken, lastToken);
            break;

        default:
            break;
        } // switch
    }
}

QVector<Token> pp::tokenize(const QByteArray &text) const
{
    QVector<Token> tokens;
    Lexer lex(text.constBegin(), text.constEnd());
    lex.setScanKeywords(false);
    Token tk;
    do {
        lex(&tk);
        tokens.append(tk);
    } while (tk.isNot(T_EOF_SYMBOL));
    return tokens;
}

void pp::processInclude(bool skipCurentPath,
                        TokenIterator firstToken, TokenIterator lastToken,
                        bool acceptMacros)
{
    RangeLexer tk(firstToken, lastToken);
    ++tk; // skip T_POUND
    ++tk; // skip `include|nclude_next'

    if (acceptMacros && tk->is(T_IDENTIFIER)) {
#if 0
        QByteArray name;
        name.reserve(256);
        MacroExpander expandInclude(env);
        expandInclude(startOfToken(tokens.at(2)),
                      startOfToken(tokens.last()),
                      &name);
        const QByteArray previousSource = switchSource(name);
        //processInclude(skipCurentPath, tokenize(name), /*accept macros=*/ false);
        (void) switchSource(previousSource);
#endif
    } else if (tk->is(T_LESS)) {
        TokenIterator start = tk.dot();
        for (; tk->isNot(T_EOF_SYMBOL); ++tk) {
            if (tk->is(T_GREATER))
                break;
        }
        const char *beginOfPath = endOfToken(*start);
        const char *endOfPath = startOfToken(*tk);
        const QByteArray path = QByteArray::fromRawData(beginOfPath,
                                                        endOfPath - beginOfPath);

        QString fn = QString::fromUtf8(path.constData(), path.length());

        if (client)
            client->sourceNeeded(fn, Client::IncludeGlobal);
    } else if (tk->is(T_ANGLE_STRING_LITERAL) || tk->is(T_STRING_LITERAL)) {
        const QByteArray spell = tokenSpell(*tk);
        const char *beginOfPath = spell.constBegin();
        const char *endOfPath = spell.constEnd();
        const char quote = *beginOfPath;
        if (beginOfPath + 1 != endOfPath && ((quote == '"' && endOfPath[-1] == '"') ||
                                             (quote == '<' && endOfPath[-1] == '>'))) {
            const QByteArray path = QByteArray::fromRawData(beginOfPath + 1,
                                                            spell.length() - 2);
            QString fn = QString::fromUtf8(path.constData(), path.length());

            if (client)
                client->sourceNeeded(fn, Client::IncludeLocal);
        }
    }
}

void pp::processDefine(TokenIterator firstToken, TokenIterator lastToken)
{
    RangeLexer tk(firstToken, lastToken);

    if (tk.size() < 3)
        return; // nothing to do

    ++tk; // skip T_POUND
    ++tk; // skip T_DEFINE

    if (tk->isNot(T_IDENTIFIER)) {
        // ### warning expected an `identifier'
        return;
    }

    Macro macro;
    macro.name = tokenText(*tk);
    ++tk; // skip T_IDENTIFIER

    if (tk->is(T_LPAREN) && ! tk->whitespace) {
        // a function-like macro definition
        macro.function_like = true;

        ++tk; // skip T_LPAREN
        if (tk->is(T_IDENTIFIER)) {
            macro.formals.append(tokenText(*tk));
            ++tk; // skip T_IDENTIFIER
            while (tk->is(T_COMMA)) {
                ++tk;// skip T_COMMA
                if (tk->isNot(T_IDENTIFIER))
                    break;
                macro.formals.append(tokenText(*tk));
                ++tk; // skip T_IDENTIFIER
            }
        }

        if (tk->is(T_DOT_DOT_DOT)) {
            macro.variadics = true;
            ++tk; // skip T_DOT_DOT_DOT
        }

        if (tk->isNot(T_RPAREN)) {
            // ### warning expected `)'
            return;
        }

        ++tk; // skip T_RPAREN
    }

    QByteArray macroId = macro.name;
    const bool isQtWord = isQtReservedWord(macroId);

    if (macro.function_like) {
        macroId += '(';
        for (int i = 0; i < macro.formals.size(); ++i) {
            if (i != 0)
                macroId += ", ";

            const QByteArray formal = macro.formals.at(i);
            macroId += formal;
        }
        macroId += ')';
    }

    if (isQtWord)
        macro.definition = macroId;
    else {
        const char *startOfDefinition = startOfToken(*tk);
        const char *endOfDefinition = startOfToken(*lastToken);
        macro.definition.append(startOfDefinition,
                                endOfDefinition - startOfDefinition);
        macro.definition.replace("\\\n", " ");
    }

    env.bind(macro);

    QByteArray macroText;
    macroText.reserve(64);
    macroText += "#define ";

    macroText += macroId;
    macroText += ' ';
    macroText += macro.definition;
    macroText += '\n';

    client->macroAdded(macroId, macroText);
}

void pp::processIf(TokenIterator firstToken, TokenIterator lastToken)
{
    RangeLexer tk(firstToken, lastToken);

    ++tk; // skip T_POUND
    ++tk; // skipt `if'

    if (testIfLevel()) {
        const char *first = startOfToken(*tk);
        const char *last = startOfToken(*lastToken);

        MacroExpander expandCondition (env);
        QByteArray condition;
        condition.reserve(256);
        expandCondition(first, last, &condition);

        QVector<Token> tokens = tokenize(condition);

        const Value result = evalExpression(tokens.constBegin(),
                                            tokens.constEnd() - 1,
                                            condition);

        _true_test[iflevel] = ! result.is_zero ();
        _skipping[iflevel]  =   result.is_zero ();
    }
}

void pp::processElse(TokenIterator firstToken, TokenIterator lastToken)
{
    RangeLexer tk(firstToken, lastToken);

    if (iflevel == 0 && !skipping ()) {
        // std::cerr << "*** WARNING #else without #if" << std::endl;
    } else if (iflevel > 0 && _skipping[iflevel - 1]) {
        _skipping[iflevel] = true;
    } else {
        _skipping[iflevel] = _true_test[iflevel];
    }
}

void pp::processElif(TokenIterator firstToken, TokenIterator lastToken)
{
    RangeLexer tk(firstToken, lastToken);
    ++tk; // skip T_POUND
    ++tk; // skipt `elif'

    if (! (iflevel > 0)) {
        // std::cerr << "*** WARNING: " << __FILE__ << __LINE__ << std::endl;
    } else if (iflevel == 0 && !skipping()) {
        // std::cerr << "*** WARNING #else without #if" << std::endl;
    } else if (!_true_test[iflevel] && !_skipping[iflevel - 1]) {
        const Value result = evalExpression(tk.dot(), lastToken, _source);
        _true_test[iflevel] = ! result.is_zero ();
        _skipping[iflevel]  =   result.is_zero ();
    } else {
        _skipping[iflevel] = true;
    }
}

void pp::processEndif(TokenIterator, TokenIterator)
{
    if (iflevel == 0 && !skipping()) {
        // std::cerr << "*** WARNING #endif without #if" << std::endl;
    } else {
        _skipping[iflevel] = false;
        _true_test[iflevel] = false;

        --iflevel;
    }
}

void pp::processIfdef(bool checkUndefined,
                      TokenIterator firstToken, TokenIterator lastToken)
{
    RangeLexer tk(firstToken, lastToken);

    ++tk; // skip T_POUND
    ++tk; // skip `ifdef'
    if (testIfLevel()) {
        if (tk->is(T_IDENTIFIER)) {
            const QByteArray macroName = tokenSpell(*tk);
            bool value = env.resolve(macroName) != 0 || env.isBuiltinMacro(macroName);

            if (checkUndefined)
                value = ! value;

            _true_test[iflevel] =   value;
            _skipping [iflevel] = ! value;
        }
    }
}

void pp::processUndef(TokenIterator firstToken, TokenIterator lastToken)
{
    RangeLexer tk(firstToken, lastToken);

    ++tk; // skip T_POUND
    ++tk; // skip `undef'

    if (tk->is(T_IDENTIFIER)) {
        const QByteArray macroName = tokenText(*tk);
        env.remove(macroName);

        QByteArray macroText;
        macroText += "#undef ";
        macroText += macroName;
        macroText += '\n';
        client->macroAdded(macroName, macroText);
    }
}

void pp::resetIfLevel ()
{
    iflevel = 0;
    _skipping[iflevel] = false;
    _true_test[iflevel] = false;
}

pp::PP_DIRECTIVE_TYPE pp::classifyDirective (const QByteArray &__directive) const
{
    switch (__directive.size())
    {
    case 2:
        if (__directive[0] == 'i' && __directive[1] == 'f')
            return PP_IF;
        break;

    case 4:
        if (__directive[0] == 'e' && __directive == "elif")
            return PP_ELIF;
        else if (__directive[0] == 'e' && __directive == "else")
            return PP_ELSE;
        break;

    case 5:
        if (__directive[0] == 'i' && __directive == "ifdef")
            return PP_IFDEF;
        else if (__directive[0] == 'u' && __directive == "undef")
            return PP_UNDEF;
        else if (__directive[0] == 'e' && __directive == "endif")
            return PP_ENDIF;
        break;

    case 6:
        if (__directive[0] == 'i' && __directive == "ifndef")
            return PP_IFNDEF;
        else if (__directive[0] == 'd' && __directive == "define")
            return PP_DEFINE;
        break;

    case 7:
        if (__directive[0] == 'i' && __directive == "include")
            return PP_INCLUDE;
        break;

    case 12:
        if (__directive[0] == 'i' && __directive == "include_next")
            return PP_INCLUDE_NEXT;
        break;

    default:
        break;
    }

    return PP_UNKNOWN_DIRECTIVE;
}

bool pp::testIfLevel()
{
    const bool result = !_skipping[iflevel++];
    _skipping[iflevel] = _skipping[iflevel - 1];
    _true_test[iflevel] = false;
    return result;
}

int pp::skipping() const
{ return _skipping[iflevel]; }

Value pp::evalExpression(TokenIterator firstToken, TokenIterator lastToken,
                         const QByteArray &source) const
{
    ExpressionEvaluator eval(&env);
    const Value result = eval(firstToken, lastToken, source);
    return result;
}

bool pp::isQtReservedWord (const QByteArray &macroId) const
{
    const int size = macroId.size();
    if      (size == 9 && macroId.at(0) == 'Q' && macroId == "Q_SIGNALS")
        return true;
    else if (size == 7 && macroId.at(0) == 'Q' && macroId == "Q_SLOTS")
        return true;
    else if (size == 6 && macroId.at(0) == 'S' && macroId == "SIGNAL")
        return true;
    else if (size == 4 && macroId.at(0) == 'S' && macroId == "SLOT")
        return true;
    else if (size == 7 && macroId.at(0) == 's' && macroId == "signals")
        return true;
    else if (size == 5 && macroId.at(0) == 's' && macroId == "slots")
        return true;
    return false;
}
