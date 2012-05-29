/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
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
#include "pp-cctype.h"

#include <Control.h>
#include <Lexer.h>
#include <Token.h>
#include <Literals.h>
#include <cctype>

#include <QtDebug>
#include <algorithm>
#include <list>
#include <QList>
#include <QDate>
#include <QTime>

#define NO_DEBUG

#ifndef NO_DEBUG
#  include <iostream>
#endif // NO_DEBUG

#include <deque>

namespace {
enum {
    eagerExpansion = 1,
    MAX_TOKEN_EXPANSION_COUNT = 5000,
    MAX_TOKEN_BUFFER_DEPTH = 16000 // for when macros are using some kind of right-folding, this is the list of "delayed" buffers waiting to be expanded after the current one.
};
}

namespace {
template<typename _T>
class ScopedSwap
{
    _T oldValue;
    _T &ref;

public:
    ScopedSwap(_T &var, _T newValue)
        : oldValue(newValue)
        , ref(var)
    {
        std::swap(ref, oldValue);
    }

    ~ScopedSwap()
    {
        std::swap(ref, oldValue);
    }
};
typedef ScopedSwap<bool> ScopedBoolSwap;
typedef ScopedSwap<unsigned> ScopedUnsignedSwap;
} // anonymous namespace

namespace CPlusPlus {

namespace Internal {
struct TokenBuffer
{
    std::deque<PPToken> tokens;
    const Macro *macro;
    TokenBuffer *next;

    TokenBuffer(const PPToken *start, const PPToken *end, const Macro *macro, TokenBuffer *next)
        : tokens(start, end), macro(macro), next(next)
    {}

    bool isBlocked(const Macro *macro) const {
        if (!macro)
            return false;

        for (const TokenBuffer *it = this; it; it = it->next)
            if (it->macro == macro && it->macro->name() == macro->name())
                return true;
        return false;
    }
};

struct Value
{
    enum Kind {
        Kind_Long,
        Kind_ULong
    };

    Kind kind;

    union {
        long l;
        unsigned long ul;
    };


    Value()
        : kind(Kind_Long), l(0)
    { }

    inline bool is_ulong () const
    { return kind == Kind_ULong; }

    inline void set_ulong (unsigned long v)
    {
        ul = v;
        kind = Kind_ULong;
    }

    inline void set_long (long v)
    {
        l = v;
        kind = Kind_Long;
    }

    inline bool is_zero () const
    { return l == 0; }

#define PP_DEFINE_BIN_OP(name, op) \
    inline Value operator op(const Value &other) const \
    { \
        Value v = *this; \
        if (v.is_ulong () || other.is_ulong ()) \
            v.set_ulong (v.ul op other.ul); \
        else \
            v.set_long (v.l op other.l); \
        return v; \
    }

    PP_DEFINE_BIN_OP(op_add, +)
    PP_DEFINE_BIN_OP(op_sub, -)
    PP_DEFINE_BIN_OP(op_mult, *)
    PP_DEFINE_BIN_OP(op_div, /)
    PP_DEFINE_BIN_OP(op_mod, %)
    PP_DEFINE_BIN_OP(op_lhs, <<)
    PP_DEFINE_BIN_OP(op_rhs, >>)
    PP_DEFINE_BIN_OP(op_lt, <)
    PP_DEFINE_BIN_OP(op_gt, >)
    PP_DEFINE_BIN_OP(op_le, <=)
    PP_DEFINE_BIN_OP(op_ge, >=)
    PP_DEFINE_BIN_OP(op_eq, ==)
    PP_DEFINE_BIN_OP(op_ne, !=)
    PP_DEFINE_BIN_OP(op_bit_and, &)
    PP_DEFINE_BIN_OP(op_bit_or, |)
    PP_DEFINE_BIN_OP(op_bit_xor, ^)
    PP_DEFINE_BIN_OP(op_and, &&)
    PP_DEFINE_BIN_OP(op_or, ||)

#undef PP_DEFINE_BIN_OP
};

} // namespace Internal
} // namespace CPlusPlus

using namespace CPlusPlus;
using namespace CPlusPlus::Internal;

namespace {

inline bool isValidToken(const PPToken &tk)
{
    return tk.isNot(T_EOF_SYMBOL) && (! tk.newline() || tk.joined());
}

Macro *macroDefinition(const ByteArrayRef &name, unsigned offset, Environment *env, Client *client)
{
    Macro *m = env->resolve(name);
    if (client) {
        if (m)
            client->passedMacroDefinitionCheck(offset, *m);
        else
            client->failedMacroDefinitionCheck(offset, name);
    }
    return m;
}

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
    ExpressionEvaluator(Client *client, Environment *env)
        : client(client), env(env), _lex(0)
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
        const ByteArrayRef spell = tokenSpell();
        if (spell.size() != 7)
            return false;
        return spell == "defined";
    }

    const char *tokenPosition() const
    {
        return source.constData() + (*_lex)->offset;
    }

    int tokenLength() const
    {
        return (*_lex)->f.length;
    }

    ByteArrayRef tokenSpell() const
    {
        return ByteArrayRef(tokenPosition(), tokenLength());
    }

    inline void process_expression()
    { process_constant_expression(); }

    void process_primary()
    {
        if ((*_lex)->is(T_NUMERIC_LITERAL)) {
            const char *spell = tokenPosition();
            int len = tokenLength();
            while (len) {
                const char ch = spell[len - 1];

                if (! (ch == 'u' || ch == 'U' || ch == 'l' || ch == 'L'))
                    break;
                --len;
            }

            const char *end = spell + len;
            char *vend = const_cast<char *>(end);
            _value.set_long(strtol(spell, &vend, 0));
            ++(*_lex);
        } else if (isTokenDefined()) {
            ++(*_lex);
            if ((*_lex)->is(T_IDENTIFIER)) {
                _value.set_long(macroDefinition(tokenSpell(), (*_lex)->offset, env, client) != 0);
                ++(*_lex);
            } else if ((*_lex)->is(T_LPAREN)) {
                ++(*_lex);
                if ((*_lex)->is(T_IDENTIFIER)) {
                    _value.set_long(macroDefinition(tokenSpell(), (*_lex)->offset, env, client) != 0);
                    ++(*_lex);
                    if ((*_lex)->is(T_RPAREN)) {
                        ++(*_lex);
                    }
                }
            }
        } else if ((*_lex)->is(T_IDENTIFIER)) {
            _value.set_long(0);
            ++(*_lex);
        } else if ((*_lex)->is(T_MINUS)) {
            ++(*_lex);
            process_primary();
            _value.set_long(- _value.l);
        } else if ((*_lex)->is(T_PLUS)) {
            ++(*_lex);
            process_primary();
        } else if ((*_lex)->is(T_TILDE)) {
            ++(*_lex);
            process_primary();
            _value.set_long(~ _value.l);
        } else if ((*_lex)->is(T_EXCLAIM)) {
            ++(*_lex);
            process_primary();
            _value.set_long(_value.is_zero());
        } else if ((*_lex)->is(T_LPAREN)) {
            ++(*_lex);
            process_expression();
            if ((*_lex)->is(T_RPAREN))
                ++(*_lex);
        }
    }

    Value process_expression_with_operator_precedence(const Value &lhs, int minPrecedence)
    {
        Value result = lhs;

        while (precedence((*_lex)->kind()) >= minPrecedence) {
            const int oper = (*_lex)->kind();
            const int operPrecedence = precedence(oper);
            ++(*_lex);
            process_primary();
            Value rhs = _value;

            for (int LA_token_kind = (*_lex)->kind(), LA_precedence = precedence(LA_token_kind);
                    LA_precedence > operPrecedence && isBinaryOperator(LA_token_kind);
                    LA_token_kind = (*_lex)->kind(), LA_precedence = precedence(LA_token_kind)) {
                rhs = process_expression_with_operator_precedence(rhs, LA_precedence);
            }

            result = evaluate_expression(oper, result, rhs);
        }

        return result;
    }

    void process_constant_expression()
    {
        process_primary();
        _value = process_expression_with_operator_precedence(_value, precedence(T_PIPE_PIPE));

        if ((*_lex)->is(T_QUESTION)) {
            const Value cond = _value;
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
    }

private:
    inline int precedence(int tokenKind) const
    {
        switch (tokenKind) {
        case T_PIPE_PIPE:       return 0;
        case T_AMPER_AMPER:     return 1;
        case T_PIPE:            return 2;
        case T_CARET:           return 3;
        case T_AMPER:           return 4;
        case T_EQUAL_EQUAL:
        case T_EXCLAIM_EQUAL:   return 5;
        case T_GREATER:
        case T_LESS:
        case T_LESS_EQUAL:
        case T_GREATER_EQUAL:   return 6;
        case T_LESS_LESS:
        case T_GREATER_GREATER: return 7;
        case T_PLUS:
        case T_MINUS:           return 8;
        case T_STAR:
        case T_SLASH:
        case T_PERCENT:         return 9;

        default:
            return -1;
        }
    }

    static inline bool isBinaryOperator(int tokenKind)
    {
        switch (tokenKind) {
        case T_PIPE_PIPE:
        case T_AMPER_AMPER:
        case T_PIPE:
        case T_CARET:
        case T_AMPER:
        case T_EQUAL_EQUAL:
        case T_EXCLAIM_EQUAL:
        case T_GREATER:
        case T_LESS:
        case T_LESS_EQUAL:
        case T_GREATER_EQUAL:
        case T_LESS_LESS:
        case T_GREATER_GREATER:
        case T_PLUS:
        case T_MINUS:
        case T_STAR:
        case T_SLASH:
        case T_PERCENT:
            return true;

        default:
            return false;
        }
    }

    static inline Value evaluate_expression(int tokenKind, const Value &lhs, const Value &rhs)
    {
        switch (tokenKind) {
        case T_PIPE_PIPE:       return lhs || rhs;
        case T_AMPER_AMPER:     return lhs && rhs;
        case T_PIPE:            return lhs | rhs;
        case T_CARET:           return lhs ^ rhs;
        case T_AMPER:           return lhs & rhs;
        case T_EQUAL_EQUAL:     return lhs == rhs;
        case T_EXCLAIM_EQUAL:   return lhs != rhs;
        case T_GREATER:         return lhs > rhs;
        case T_LESS:            return lhs < rhs;
        case T_LESS_EQUAL:      return lhs <= rhs;
        case T_GREATER_EQUAL:   return lhs >= rhs;
        case T_LESS_LESS:       return lhs << rhs;
        case T_GREATER_GREATER: return lhs >> rhs;
        case T_PLUS:            return lhs + rhs;
        case T_MINUS:           return lhs - rhs;
        case T_STAR:            return lhs * rhs;
        case T_SLASH:           return rhs.is_zero() ? Value() : lhs / rhs;
        case T_PERCENT:         return rhs.is_zero() ? Value() : lhs % rhs;

        default:
            return Value();
        }
    }

private:
    Client *client;
    Environment *env;
    QByteArray source;
    RangeLexer *_lex;
    Value _value;
};

} // end of anonymous namespace

Preprocessor::State::State()
    : m_lexer(0)
    , m_skipping(MAX_LEVEL)
    , m_trueTest(MAX_LEVEL)
    , m_ifLevel(0)
    , m_tokenBuffer(0)
    , m_tokenBufferDepth(0)
    , m_inPreprocessorDirective(false)
    , m_result(0)
    , m_markGeneratedTokens(true)
    , m_noLines(false)
    , m_inCondition(false)
    , m_inDefine(false)
    , m_offsetRef(0)
    , m_envLineRef(1)
{
    m_skipping[m_ifLevel] = false;
    m_trueTest[m_ifLevel] = false;
}

//#define COMPRESS_TOKEN_BUFFER
void Preprocessor::State::pushTokenBuffer(const PPToken *start, const PPToken *end, const Macro *macro)
{
    if (m_tokenBufferDepth <= MAX_TOKEN_BUFFER_DEPTH) {
#ifdef COMPRESS_TOKEN_BUFFER
        // This does not work correctly for boost's preprocessor library, or that library exposes a bug in the code.
        if (macro || !m_tokenBuffer) {
            m_tokenBuffer = new TokenBuffer(start, end, macro, m_tokenBuffer);
            ++m_tokenBufferDepth;
        } else {
            m_tokenBuffer->tokens.insert(m_tokenBuffer->tokens.begin(), start, end);
        }
        unsigned tkCount = 0;
        for (TokenBuffer *it = m_tokenBuffer; it; it = m_tokenBuffer->next)
            tkCount += it->tokens.size();
        qDebug()<<"New depth:" << m_tokenBufferDepth << "with total token count:" << tkCount;
#else
        m_tokenBuffer = new TokenBuffer(start, end, macro, m_tokenBuffer);
        ++m_tokenBufferDepth;
#endif
    }
}

void Preprocessor::State::popTokenBuffer()
{
    TokenBuffer *r = m_tokenBuffer;
    m_tokenBuffer = m_tokenBuffer->next;
    delete r;

    if (m_tokenBufferDepth)
        --m_tokenBufferDepth;
}

Preprocessor::Preprocessor(Client *client, Environment *env)
    : m_client(client)
    , m_env(env)
    , m_expandMacros(true)
    , m_keepComments(false)
{
}

QByteArray Preprocessor::run(const QString &fileName, const QString &source)
{
    return run(fileName, source.toLatin1());
}

QByteArray Preprocessor::run(const QString &fileName,
                             const QByteArray &source,
                             bool noLines,
                             bool markGeneratedTokens)
{
    QByteArray preprocessed;
//    qDebug()<<"running" << fileName<<"with"<<source.count('\n')<<"lines...";
    preprocess(fileName, source, &preprocessed, noLines, markGeneratedTokens, false);
    return preprocessed;
}

bool Preprocessor::expandMacros() const
{
    return m_expandMacros;
}

void Preprocessor::setExpandMacros(bool expandMacros)
{
    m_expandMacros = expandMacros;
}

bool Preprocessor::keepComments() const
{
    return m_keepComments;
}

void Preprocessor::setKeepComments(bool keepComments)
{
    m_keepComments = keepComments;
}

void Preprocessor::genLine(unsigned lineno, const QByteArray &fileName) const
{
    startNewOutputLine();
    out("# ");
    out(QByteArray::number(lineno));
    out(" \"");
    out(fileName);
    out("\"\n");
}

void Preprocessor::handleDefined(PPToken *tk)
{
    ScopedBoolSwap s(m_state.m_inPreprocessorDirective, true);
    unsigned lineno = tk->lineno;
    lex(tk); // consume "defined" token
    bool lparenSeen = tk->is(T_LPAREN);
    if (lparenSeen)
        lex(tk); // consume "(" token
    if (tk->isNot(T_IDENTIFIER))
        //### TODO: generate error message
        return;
    PPToken idToken = *tk;
    do {
        lex(tk);
        if (tk->isNot(T_POUND_POUND))
            break;
        lex(tk);
        if (tk->is(T_IDENTIFIER))
            idToken = generateConcatenated(idToken, *tk);
        else
            break;
    } while (isValidToken(*tk));


    if (lparenSeen && tk->is(T_RPAREN))
        lex(tk);

    pushToken(tk);

    QByteArray result(1, '0');
    if (m_env->resolve(idToken.asByteArrayRef()))
        result[0] = '1';
    *tk = generateToken(T_NUMERIC_LITERAL, result.constData(), result.size(), lineno, false);
}

void Preprocessor::pushToken(Preprocessor::PPToken *tk)
{
    const PPToken currentTokenBuffer[] = { *tk };
    m_state.pushTokenBuffer(currentTokenBuffer, currentTokenBuffer + 1, 0);
}

void Preprocessor::lex(PPToken *tk)
{
_Lagain:
    if (m_state.m_tokenBuffer) {
        if (m_state.m_tokenBuffer->tokens.empty()) {
            m_state.popTokenBuffer();
            goto _Lagain;
        }
        *tk = m_state.m_tokenBuffer->tokens.front();
        m_state.m_tokenBuffer->tokens.pop_front();
    } else {
        tk->setSource(m_state.m_source);
        m_state.m_lexer->scan(tk);
    }

_Lclassify:
    if (! m_state.m_inPreprocessorDirective) {
        // Bellow, during directive and identifier handling the current environment line is
        // updated in accordance to "global" context in order for clients to rely on consistent
        // information. Afterwards, it's restored until output is eventually processed.
        if (tk->newline() && tk->is(T_POUND)) {
            unsigned envLine = m_env->currentLine;
            m_env->currentLine = tk->lineno + m_state.m_envLineRef - 1;
            handlePreprocessorDirective(tk);
            m_env->currentLine = envLine;
            goto _Lclassify;
        } else if (tk->newline() && skipping()) {
            ScopedBoolSwap s(m_state.m_inPreprocessorDirective, true);
            do {
                lex(tk);
            } while (isValidToken(*tk));
            goto _Lclassify;
        } else if (tk->is(T_IDENTIFIER) && !isQtReservedWord(tk->asByteArrayRef())) {
            if (m_state.m_inCondition && tk->asByteArrayRef() == "defined") {
                handleDefined(tk);
            } else {
                unsigned envLine = m_env->currentLine;
                m_env->currentLine = tk->lineno + m_state.m_envLineRef - 1;
                bool willExpand = handleIdentifier(tk);
                m_env->currentLine = envLine;
                if (willExpand)
                    goto _Lagain;
            }
        }
    }
}

void Preprocessor::skipPreprocesorDirective(PPToken *tk)
{
    ScopedBoolSwap s(m_state.m_inPreprocessorDirective, true);

    while (isValidToken(*tk)) {
        lex(tk);
    }
}

bool Preprocessor::handleIdentifier(PPToken *tk)
{
    if (!expandMacros())
        return false;

    ScopedBoolSwap s(m_state.m_inPreprocessorDirective, true);

    static const QByteArray ppLine("__LINE__");
    static const QByteArray ppFile("__FILE__");
    static const QByteArray ppDate("__DATE__");
    static const QByteArray ppTime("__TIME__");

    ByteArrayRef macroNameRef = tk->asByteArrayRef();
    bool newline = tk->newline();

    if (!m_state.m_inDefine && macroNameRef.size() == 8 && macroNameRef[0] == '_' && macroNameRef[1] == '_') {
        PPToken newTk;
        if (macroNameRef == ppLine) {
            QByteArray txt = QByteArray::number(tk->lineno);
            newTk = generateToken(T_STRING_LITERAL, txt.constData(), txt.size(), tk->lineno, false);
        } else if (macroNameRef == ppFile) {
            QByteArray txt;
            txt.append('"');
            txt.append(m_env->currentFile.toUtf8());
            txt.append('"');
            newTk = generateToken(T_STRING_LITERAL, txt.constData(), txt.size(), tk->lineno, false);
        } else if (macroNameRef == ppDate) {
            QByteArray txt;
            txt.append('"');
            txt.append(QDate::currentDate().toString().toUtf8());
            txt.append('"');
            newTk = generateToken(T_STRING_LITERAL, txt.constData(), txt.size(), tk->lineno, false);
        } else if (macroNameRef == ppTime) {
            QByteArray txt;
            txt.append('"');
            txt.append(QTime::currentTime().toString().toUtf8());
            txt.append('"');
            newTk = generateToken(T_STRING_LITERAL, txt.constData(), txt.size(), tk->lineno, false);
        }

        if (newTk.isValid()) {
            newTk.f.newline = newline;
            newTk.f.whitespace = tk->whitespace();
            *tk = newTk;
            return false;
        }
    }

    Macro *macro = m_env->resolve(macroNameRef);
    if (!macro
            || (tk->generated()
                && m_state.m_tokenBuffer
                && m_state.m_tokenBuffer->isBlocked(macro))) {
        return false;
    }
//    qDebug() << "expanding" << macro->name() << "on line" << tk->lineno;


    PPToken idTk = *tk;
    QVector<PPToken> body = macro->definitionTokens();

    if (macro->isFunctionLike()) {
        // Collect individual tokens that form the macro arguments.
        QVector<QVector<PPToken> > allArgTks;
        if (!collectActualArguments(tk, &allArgTks)) {
            pushToken(tk);
            *tk = idTk;
            return false;
        }

        if (m_client && !idTk.generated()) {
            // Bundle each token sequence into a macro argument "reference" for notification.
            QVector<MacroArgumentReference> argRefs;
            for (int i = 0; i < allArgTks.size(); ++i) {
                const QVector<PPToken> &argTks = allArgTks.at(i);
                if (argTks.isEmpty())
                    continue;

                argRefs.push_back(
                            MacroArgumentReference(
                                m_state.m_offsetRef + argTks.first().begin(),
                                argTks.last().begin() + argTks.last().length() - argTks.first().begin()));
            }

            m_client->startExpandingMacro(m_state.m_offsetRef + idTk.offset, *macro, macroNameRef,
                                          argRefs);
        }

        if (!handleFunctionLikeMacro(tk, macro, body, !m_state.m_inDefine, allArgTks)) {
            if (m_client && !idTk.generated())
                m_client->stopExpandingMacro(idTk.offset, *macro);
            return false;
        }
    } else if (m_client && !idTk.generated()) {
        m_client->startExpandingMacro(m_state.m_offsetRef + idTk.offset, *macro, macroNameRef);
    }

    if (body.isEmpty()) {
        if (!m_state.m_inDefine) {
            // macro expanded to empty, so characters disappeared, hence force a re-indent.
            PPToken forceWhitespacingToken;
            // special case: for a macro that expanded to empty, we do not want
            // to generate a new #line and re-indent, but just generate the
            // amount of spaces that the macro name took up.
            forceWhitespacingToken.f.length = tk->length() + (tk->whitespace() ? 1 : 0);
            body.push_front(forceWhitespacingToken);
        }
    } else {
        PPToken &firstNewTk = body.first();
        firstNewTk.f.newline = newline;
        firstNewTk.f.whitespace = true; // the macro call is removed, so space the first token correctly.
    }

    m_state.pushTokenBuffer(body.begin(), body.end(), macro);

    if (m_client && !idTk.generated())
        m_client->stopExpandingMacro(idTk.offset, *macro);

    return true;
}

bool Preprocessor::handleFunctionLikeMacro(PPToken *tk,
                                           const Macro *macro,
                                           QVector<PPToken> &body,
                                           bool addWhitespaceMarker,
                                           const QVector<QVector<PPToken> > &actuals)
{
    QVector<PPToken> expanded;
    expanded.reserve(MAX_TOKEN_EXPANSION_COUNT);
    for (size_t i = 0, bodySize = body.size(); i < bodySize && expanded.size() < MAX_TOKEN_EXPANSION_COUNT; ++i) {
        int expandedSize = expanded.size();
        const PPToken &token = body.at(i);

        if (token.is(T_IDENTIFIER)) {
            const ByteArrayRef id = token.asByteArrayRef();
            const QVector<QByteArray> &formals = macro->formals();
            int j = 0;
            for (; j < formals.size() && expanded.size() < MAX_TOKEN_EXPANSION_COUNT; ++j) {
                if (formals[j] == id) {
                    if (actuals.size() <= j) {
                        // too few actual parameters
                        //### TODO: error message
                        goto exitNicely;
                    }

                    QVector<PPToken> actualsForThisParam = actuals.at(j);
                    if (id == "__VA_ARGS__" || (macro->isVariadic() && j + 1 == formals.size())) {
                        unsigned lineno = 0;
                        const char comma = ',';
                        for (int k = j + 1; k < actuals.size(); ++k) {
                            if (!actualsForThisParam.isEmpty())
                                lineno = actualsForThisParam.last().lineno;
                            actualsForThisParam.append(generateToken(T_COMMA, &comma, 1, lineno, true));
                            actualsForThisParam += actuals[k];
                        }
                    }

                    if (i > 0  && body[i - 1].is(T_POUND)) {
                        QByteArray newText;
                        newText.reserve(256);
                        unsigned lineno = 0;
                        for (int i = 0, ei = actualsForThisParam.size(); i < ei; ++i) {
                            const PPToken &t = actualsForThisParam.at(i);
                            if (i == 0)
                                lineno = t.lineno;
                            else if (t.whitespace())
                                newText.append(' ');
                            newText.append(t.tokenStart(), t.length());
                        }
                        newText.replace("\\", "\\\\");
                        newText.replace("\"", "\\\"");
                        expanded.push_back(generateToken(T_STRING_LITERAL, newText.constData(), newText.size(), lineno, true));
                    } else  {
                        for (int k = 0, kk = actualsForThisParam.size(); k < kk; ++k)
                            expanded += actualsForThisParam.at(k);
                    }
                    break;
                }
            }

            if (j == formals.size())
                expanded.push_back(token);
        } else if (token.isNot(T_POUND) && token.isNot(T_POUND_POUND)) {
            expanded.push_back(token);
        }

        if (i > 1 && body[i - 1].is(T_POUND_POUND)) {
            if (expandedSize < 1 || expanded.size() == expandedSize) //### TODO: [cpp.concat] placemarkers
                continue;
            const PPToken &leftTk = expanded[expandedSize - 1];
            const PPToken &rightTk = expanded[expandedSize];
            expanded[expandedSize - 1] = generateConcatenated(leftTk, rightTk);
            expanded.remove(expandedSize);
        }
    }

exitNicely:
    pushToken(tk);
    if (addWhitespaceMarker) {
        PPToken forceWhitespacingToken;
        expanded.push_front(forceWhitespacingToken);
    }
    body = expanded;
    body.squeeze();
    return true;
}

/// invalid pp-tokens are used as markers to force whitespace checks.
void Preprocessor::preprocess(const QString &fileName, const QByteArray &source,
                              QByteArray *result, bool noLines,
                              bool markGeneratedTokens, bool inCondition,
                              unsigned offsetRef, unsigned envLineRef)
{
    if (source.isEmpty())
        return;

    const State savedState = m_state;

    m_state = State();
    m_state.m_currentFileName = fileName;
    m_state.m_source = source;
    m_state.m_lexer = new Lexer(source.constBegin(), source.constEnd());
    m_state.m_lexer->setScanKeywords(false);
    m_state.m_lexer->setScanAngleStringLiteralTokens(false);
    if (m_keepComments)
        m_state.m_lexer->setScanCommentTokens(true);
    m_state.m_result = result;
    m_state.m_noLines = noLines;
    m_state.m_markGeneratedTokens = markGeneratedTokens;
    m_state.m_inCondition = inCondition;
    m_state.m_offsetRef = offsetRef;
    m_state.m_envLineRef = envLineRef;

    const QString previousFileName = m_env->currentFile;
    m_env->currentFile = fileName;

    const unsigned previousCurrentLine = m_env->currentLine;
    m_env->currentLine = 1;

    const QByteArray fn = fileName.toUtf8();
    if (!m_state.m_noLines)
        genLine(1, fn);

    PPToken tk(m_state.m_source), prevTk;
    do {
_Lrestart:
        bool forceLine = false;
        lex(&tk);

        if (!tk.isValid()) {
            bool wasGenerated = prevTk.generated();
            prevTk = tk;
            prevTk.f.generated = wasGenerated;
            goto _Lrestart;
        }

        if (m_state.m_markGeneratedTokens && tk.generated() && !prevTk.generated()) {
            startNewOutputLine();
            out("#gen true\n");
            ++m_env->currentLine;
            forceLine = true;
        } else if (m_state.m_markGeneratedTokens && !tk.generated() && prevTk.generated()) {
            startNewOutputLine();
            out("#gen false\n");
            ++m_env->currentLine;
            forceLine = true;
        }

        if (forceLine || m_env->currentLine != tk.lineno) {
            if (forceLine || m_env->currentLine > tk.lineno || tk.lineno - m_env->currentLine > 3) {
                if (m_state.m_noLines) {
                    if (!m_state.m_markGeneratedTokens)
                        out(' ');
                } else {
                    genLine(tk.lineno, fn);
                }
            } else {
                for (unsigned i = m_env->currentLine; i < tk.lineno; ++i)
                    out('\n');
            }
        } else {
            if (tk.newline() && prevTk.isValid())
                out('\n');
        }

        if (tk.whitespace() || prevTk.generated() != tk.generated() || !prevTk.isValid()) {
            if (prevTk.generated() && tk.generated()) {
                out(' ');
            } else if (tk.isValid() && !prevTk.isValid() && tk.lineno == m_env->currentLine) {
                out(QByteArray(prevTk.length() + (tk.whitespace() ? 1 : 0), ' '));
            } else if (prevTk.generated() != tk.generated() || !prevTk.isValid()) {
                const char *begin = tk.bufferStart();
                const char *end = tk.tokenStart();
                const char *it = end - 1;
                for (; it >= begin; --it)
                    if (*it == '\n')
                        break;
                ++it;
                for (; it < end; ++it)
                    out(' ');
            } else {
                const char *begin = tk.bufferStart();
                const char *end = tk.tokenStart();
                const char *it = end - 1;
                for (; it >= begin; --it)
                    if (!pp_isspace(*it) || *it == '\n')
                        break;
                ++it;
                for (; it < end; ++it)
                    out(*it);
            }
        }

        const ByteArrayRef tkBytes = tk.asByteArrayRef();
        out(tkBytes);
        m_env->currentLine = tk.lineno;
        if (tk.is(T_COMMENT) || tk.is(T_DOXY_COMMENT))
            m_env->currentLine += tkBytes.count('\n');
        prevTk = tk;
    } while (tk.isNot(T_EOF_SYMBOL));

    delete m_state.m_lexer;
    m_state = savedState;

    m_env->currentFile = previousFileName;
    m_env->currentLine = previousCurrentLine;
}

bool Preprocessor::collectActualArguments(PPToken *tk, QVector<QVector<PPToken> > *actuals)
{
    Q_ASSERT(tk);
    Q_ASSERT(actuals);

    lex(tk); // consume the identifier

    if (tk->isNot(T_LPAREN))
        //### TODO: error message
        return false;

    QVector<PPToken> tokens;
    lex(tk);
    scanActualArgument(tk, &tokens);

    actuals->append(tokens);

    while (tk->is(T_COMMA)) {
        lex(tk);

        QVector<PPToken> tokens;
        scanActualArgument(tk, &tokens);
        actuals->append(tokens);
    }

    if (tk->is(T_RPAREN))
        lex(tk);
    //###TODO: else error message
    return true;
}

void Preprocessor::scanActualArgument(PPToken *tk, QVector<PPToken> *tokens)
{
    Q_ASSERT(tokens);

    int count = 0;

    while (tk->isNot(T_EOF_SYMBOL)) {
        if (tk->is(T_LPAREN)) {
            ++count;
        } else if (tk->is(T_RPAREN)) {
            if (! count)
                break;
            --count;
        } else if (! count && tk->is(T_COMMA)) {
            break;
        }

        tokens->append(*tk);
        lex(tk);
    }
}

void Preprocessor::handlePreprocessorDirective(PPToken *tk)
{
    ScopedBoolSwap s(m_state.m_inPreprocessorDirective, true);

    PPToken poundToken = *tk;
    lex(tk); // scan the directive

    if (tk->newline() && ! tk->joined())
        return; // nothing to do.

    static const QByteArray ppDefine("define");
    static const QByteArray ppIf("if");
    static const QByteArray ppIfDef("ifdef");
    static const QByteArray ppIfNDef("ifndef");
    static const QByteArray ppEndIf("endif");
    static const QByteArray ppElse("else");
    static const QByteArray ppUndef("undef");
    static const QByteArray ppElif("elif");
    static const QByteArray ppInclude("include");
    static const QByteArray ppIncludeNext("include_next");
    static const QByteArray ppImport("import");
    //### TODO:
    // line
    // error
    // pragma

    if (tk->is(T_IDENTIFIER)) {
        const ByteArrayRef directive = tk->asByteArrayRef();

        if (!skipping() && directive == ppDefine)
            handleDefineDirective(tk);
        else if (!skipping() && directive == ppUndef)
            handleUndefDirective(tk);
        else if (!skipping() && (directive == ppInclude
                                 || directive == ppIncludeNext
                                 || directive == ppImport))
            handleIncludeDirective(tk);
        else if (directive == ppIf)
            handleIfDirective(tk);
        else if (directive == ppIfDef)
            handleIfDefDirective(false, tk);
        else if (directive == ppIfNDef)
            handleIfDefDirective(true, tk);
        else if (directive == ppEndIf)
            handleEndIfDirective(tk, poundToken);
        else if (directive == ppElse)
            handleElseDirective(tk, poundToken);
        else if (directive == ppElif)
            handleElifDirective(tk, poundToken);

        skipPreprocesorDirective(tk);
    }
}


void Preprocessor::handleIncludeDirective(PPToken *tk)
{
    m_state.m_lexer->setScanAngleStringLiteralTokens(true);
    lex(tk); // consume "include" token
    m_state.m_lexer->setScanAngleStringLiteralTokens(false);
    const unsigned line = tk->lineno;
    QByteArray included;

    if (tk->is(T_STRING_LITERAL) || tk->is(T_ANGLE_STRING_LITERAL)) {
        included = tk->asByteArrayRef().toByteArray();
        lex(tk); // consume string token
    } else {
        included = expand(tk);
    }
    included = included.trimmed();

    if (included.isEmpty()) {
        //### TODO: error message
        return;
    }

//    qDebug("include [[%s]]", included.toUtf8().constData());
    Client::IncludeType mode;
    if (included.at(0) == '"')
        mode = Client::IncludeLocal;
    else if (included.at(0) == '<')
        mode = Client::IncludeGlobal;
    else
        return; //### TODO: add error message?

    if (m_client) {
        QString inc = QString::fromUtf8(included.constData() + 1, included.size() - 2);
        m_client->sourceNeeded(inc, mode, line);
    }
}

void Preprocessor::handleDefineDirective(PPToken *tk)
{
    const unsigned defineOffset = tk->offset;
    lex(tk); // consume "define" token

    bool hasIdentifier = false;
    if (tk->isNot(T_IDENTIFIER))
        return;

    ScopedBoolSwap inDefine(m_state.m_inDefine, true);

    Macro macro;
    macro.setFileName(m_env->currentFile);
    macro.setLine(tk->lineno);
    QByteArray macroName = tk->asByteArrayRef().toByteArray();
    macro.setName(macroName);
    macro.setOffset(tk->offset);

    lex(tk);

    if (isValidToken(*tk) && tk->is(T_LPAREN) && ! tk->whitespace()) {
        macro.setFunctionLike(true);

        lex(tk); // skip `('

        if (isValidToken(*tk) && tk->is(T_IDENTIFIER)) {
            hasIdentifier = true;
            macro.addFormal(tk->asByteArrayRef().toByteArray());

            lex(tk);

            while (isValidToken(*tk) && tk->is(T_COMMA)) {
                lex(tk);

                if (isValidToken(*tk) && tk->is(T_IDENTIFIER)) {
                    macro.addFormal(tk->asByteArrayRef().toByteArray());
                    lex(tk);
                } else {
                    hasIdentifier = false;
                }
            }
        }

        if (tk->is(T_DOT_DOT_DOT)) {
            macro.setVariadic(true);
            if (!hasIdentifier)
                macro.addFormal("__VA_ARGS__");
            lex(tk); // consume elipsis token
        }
        if (isValidToken(*tk) && tk->is(T_RPAREN))
            lex(tk); // consume ")" token
    }

    QVector<PPToken> bodyTokens;
    PPToken firstBodyToken = *tk;
    while (isValidToken(*tk)) {
        tk->f.generated = true;
        bodyTokens.push_back(*tk);
        lex(tk);
        if (eagerExpansion)
            while (tk->is(T_IDENTIFIER)
                   && (!tk->newline() || tk->joined())
                   && !isQtReservedWord(tk->asByteArrayRef())
                   && handleIdentifier(tk)) {
                lex(tk);
            }
    }

    if (isQtReservedWord(ByteArrayRef(&macroName))) {
        QByteArray macroId = macro.name();

        if (macro.isFunctionLike()) {
            macroId += '(';
            bool fst = true;
            foreach (const QByteArray &formal, macro.formals()) {
                if (! fst)
                    macroId += ", ";
                fst = false;
                macroId += formal;
            }
            macroId += ')';
        }

        bodyTokens.clear();
        macro.setDefinition(macroId, bodyTokens);
    } else {
        int start = firstBodyToken.offset;
        int len = tk->offset - start;
        QByteArray bodyText = firstBodyToken.source().mid(start, len).trimmed();
        for (int i = 0, count = bodyTokens.size(); i < count; ++i) {
            PPToken &t = bodyTokens[i];
            if (t.isValid())
                t.squeeze();
        }
        macro.setDefinition(bodyText, bodyTokens);
    }

    macro.setLength(tk->offset - defineOffset);
    m_env->bind(macro);

//    qDebug() << "adding macro" << macro.name() << "defined at" << macro.fileName() << ":"<<macro.line();

    if (m_client)
        m_client->macroAdded(macro);
}

QByteArray Preprocessor::expand(PPToken *tk, PPToken *lastConditionToken)
{
    unsigned begin = tk->begin();
    PPToken lastTk;
    while (isValidToken(*tk)) {
        lastTk = *tk;
        lex(tk);
    }
    // Gather the exact spelling of the content in the source.
    QByteArray condition(m_state.m_source.mid(begin, lastTk.begin() + lastTk.length() - begin));

//    qDebug("*** Condition before: [%s]", condition.constData());
    QByteArray result;
    result.reserve(256);
    preprocess(m_state.m_currentFileName, condition, &result, true, false, true, begin,
               m_env->currentLine);
    result.squeeze();
//    qDebug("*** Condition after: [%s]", result.constData());

    if (lastConditionToken)
        *lastConditionToken = lastTk;

    return result;
}

const PPToken Preprocessor::evalExpression(PPToken *tk, Value &result)
{
    PPToken lastConditionToken;
    const QByteArray expanded = expand(tk, &lastConditionToken);
    Lexer lexer(expanded.constData(), expanded.constData() + expanded.size());
    std::vector<Token> buf;
    Token t;
    do {
        lexer.scan(&t);
        buf.push_back(t);
    } while (t.isNot(T_EOF_SYMBOL));
    ExpressionEvaluator eval(m_client, m_env);
    result = eval(&buf[0], &buf[buf.size() - 1], expanded);
    return lastConditionToken;
}

void Preprocessor::handleIfDirective(PPToken *tk)
{
    lex(tk); // consume "if" token
    Value result;
    const PPToken lastExpressionToken = evalExpression(tk, result);
    const bool value = !result.is_zero();

    const bool wasSkipping = m_state.m_skipping[m_state.m_ifLevel];
    ++m_state.m_ifLevel;
    m_state.m_trueTest[m_state.m_ifLevel] = value;
    if (wasSkipping) {
        m_state.m_skipping[m_state.m_ifLevel] = wasSkipping;
    } else {
        bool startSkipping = !value;
        m_state.m_skipping[m_state.m_ifLevel] = startSkipping;
        if (startSkipping && m_client)
            startSkippingBlocks(lastExpressionToken);
    }

}

void Preprocessor::handleElifDirective(PPToken *tk, const PPToken &poundToken)
{
    if (m_state.m_ifLevel == 0) {
//        std::cerr << "*** WARNING #elif without #if" << std::endl;
        handleIfDirective(tk);
    } else {
        lex(tk); // consume "elif" token
        if (m_state.m_skipping[m_state.m_ifLevel - 1]) {
            // we keep on skipping because we are nested in a skipped block
            m_state.m_skipping[m_state.m_ifLevel] = true;
        } else if (m_state.m_trueTest[m_state.m_ifLevel]) {
            if (!m_state.m_skipping[m_state.m_ifLevel]) {
                // start skipping because the preceeding then-part was not skipped
                m_state.m_skipping[m_state.m_ifLevel] = true;
                if (m_client)
                    startSkippingBlocks(poundToken);
            }
        } else {
            // preceeding then-part was skipped, so calculate if we should start
            // skipping, depending on the condition
            Value result;
            evalExpression(tk, result);

            bool startSkipping = result.is_zero();
            m_state.m_trueTest[m_state.m_ifLevel] = !startSkipping;
            m_state.m_skipping[m_state.m_ifLevel] = startSkipping;
            if (m_client && !startSkipping)
                m_client->stopSkippingBlocks(poundToken.offset - 1);
        }
    }
}

void Preprocessor::handleElseDirective(PPToken *tk, const PPToken &poundToken)
{
    lex(tk); // consume "else" token

    if (m_state.m_ifLevel != 0) {
        if (m_state.m_skipping[m_state.m_ifLevel - 1]) {
            // we keep on skipping because we are nested in a skipped block
            m_state.m_skipping[m_state.m_ifLevel] = true;
        } else {
            bool wasSkipping = m_state.m_skipping[m_state.m_ifLevel];
            bool startSkipping = m_state.m_trueTest[m_state.m_ifLevel];
            m_state.m_skipping[m_state.m_ifLevel] = startSkipping;

            if (m_client && wasSkipping && !startSkipping)
                m_client->stopSkippingBlocks(poundToken.offset - 1);
            else if (m_client && !wasSkipping && startSkipping)
                startSkippingBlocks(poundToken);
        }
    }
#ifndef NO_DEBUG
    else {
        std::cerr << "*** WARNING #else without #if" << std::endl;
    }
#endif // NO_DEBUG
}

void Preprocessor::handleEndIfDirective(PPToken *tk, const PPToken &poundToken)
{
    if (m_state.m_ifLevel == 0) {
#ifndef NO_DEBUG
        std::cerr << "*** WARNING #endif without #if";
        if (!tk->generated())
            std::cerr << " on line " << tk->lineno << " of file " << m_state.m_currentFileName.toUtf8().constData();
        std::cerr << std::endl;
#endif // NO_DEBUG
    } else {
        bool wasSkipping = m_state.m_skipping[m_state.m_ifLevel];
        m_state.m_skipping[m_state.m_ifLevel] = false;
        m_state.m_trueTest[m_state.m_ifLevel] = false;
        --m_state.m_ifLevel;
        if (m_client && wasSkipping && !m_state.m_skipping[m_state.m_ifLevel])
            m_client->stopSkippingBlocks(poundToken.offset - 1);
    }

    lex(tk); // consume "endif" token
}

void Preprocessor::handleIfDefDirective(bool checkUndefined, PPToken *tk)
{
    static const QByteArray qCreatorRun("Q_CREATOR_RUN");

    lex(tk); // consume "ifdef" token
    if (tk->is(T_IDENTIFIER)) {
        bool value = false;
        const ByteArrayRef macroName = tk->asByteArrayRef();
        if (Macro *macro = macroDefinition(macroName, tk->offset, m_env, m_client)) {
            value = true;

            // the macro is a feature constraint(e.g. QT_NO_XXX)
            if (checkUndefined && macroName.startsWith("QT_NO_")) {
                if (macro->fileName() == QLatin1String("<configuration>")) {
                    // and it' defined in a pro file (e.g. DEFINES += QT_NO_QOBJECT)

                    value = false; // take the branch
                }
            }
        } else if (m_env->isBuiltinMacro(macroName)) {
            value = true;
        } else if (macroName == qCreatorRun) {
            value = true;
        }

        if (checkUndefined)
            value = !value;

        const bool wasSkipping = m_state.m_skipping[m_state.m_ifLevel];
        ++m_state.m_ifLevel;
        m_state.m_trueTest[m_state.m_ifLevel] = value;
        m_state.m_skipping[m_state.m_ifLevel] = wasSkipping ? wasSkipping : !value;

        if (m_client && !wasSkipping && !value)
            startSkippingBlocks(*tk);

        lex(tk); // consume the identifier
    }
#ifndef NO_DEBUG
    else {
        std::cerr << "*** WARNING #ifdef without identifier" << std::endl;
    }
#endif // NO_DEBUG
}

void Preprocessor::handleUndefDirective(PPToken *tk)
{
    lex(tk); // consume "undef" token
    if (tk->is(T_IDENTIFIER)) {
        const ByteArrayRef macroName = tk->asByteArrayRef();
        const Macro *macro = m_env->remove(macroName);

        if (m_client && macro)
            m_client->macroAdded(*macro);
        lex(tk); // consume macro name
    }
#ifndef NO_DEBUG
    else {
        std::cerr << "*** WARNING #undef without identifier" << std::endl;
    }
#endif // NO_DEBUG
}

bool Preprocessor::isQtReservedWord(const ByteArrayRef &macroId)
{
    const int size = macroId.size();
    if      (size == 9 && macroId.at(0) == 'Q' && macroId == "Q_SIGNALS")
        return true;
    else if (size == 9 && macroId.at(0) == 'Q' && macroId == "Q_FOREACH")
        return true;
    else if (size == 7 && macroId.at(0) == 'Q' && macroId == "Q_SLOTS")
        return true;
    else if (size == 8 && macroId.at(0) == 'Q' && macroId == "Q_SIGNAL")
        return true;
    else if (size == 6 && macroId.at(0) == 'Q' && macroId == "Q_SLOT")
        return true;
    else if (size == 3 && macroId.at(0) == 'Q' && macroId == "Q_D")
        return true;
    else if (size == 3 && macroId.at(0) == 'Q' && macroId == "Q_Q")
        return true;
    else if (size == 10 && macroId.at(0) == 'Q' && macroId == "Q_PROPERTY")
        return true;
    else if (size == 18 && macroId.at(0) == 'Q' && macroId == "Q_PRIVATE_PROPERTY")
        return true;
    else if (size == 7 && macroId.at(0) == 'Q' && macroId == "Q_ENUMS")
        return true;
    else if (size == 7 && macroId.at(0) == 'Q' && macroId == "Q_FLAGS")
        return true;
    else if (size == 12 && macroId.at(0) == 'Q' && macroId == "Q_INTERFACES")
        return true;
    else if (size == 11 && macroId.at(0) == 'Q' && macroId == "Q_INVOKABLE")
        return true;
    else if (size == 6 && macroId.at(0) == 'S' && macroId == "SIGNAL")
        return true;
    else if (size == 4 && macroId.at(0) == 'S' && macroId == "SLOT")
        return true;
    else if (size == 7 && macroId.at(0) == 's' && macroId == "signals")
        return true;
    else if (size == 7 && macroId.at(0) == 'f' && macroId == "foreach")
        return true;
    else if (size == 5 && macroId.at(0) == 's' && macroId == "slots")
        return true;
    return false;
}

PPToken Preprocessor::generateToken(enum Kind kind, const char *content, int len, unsigned lineno, bool addQuotes)
{
    // When reconstructing the column position of a token,
    // Preprocessor::preprocess will search for the last preceeding newline.
    // When the token is a generated token, the column position cannot be
    // reconstructed, but we also have to prevent it from searching the whole
    // scratch buffer. So inserting a newline before the new token will give
    // an indent width of 0 (zero).
    m_scratchBuffer.append('\n');

    const size_t pos = m_scratchBuffer.size();

    if (kind == T_STRING_LITERAL && addQuotes)
        m_scratchBuffer.append('"');
    m_scratchBuffer.append(content, len);
    if (kind == T_STRING_LITERAL && addQuotes) {
        m_scratchBuffer.append('"');
        len += 2;
    }

    PPToken tok(m_scratchBuffer);
    tok.f.kind = kind;
    if (m_state.m_lexer->control()) {
        if (kind == T_STRING_LITERAL)
            tok.string = m_state.m_lexer->control()->stringLiteral(m_scratchBuffer.constData() + pos, len);
        else if (kind == T_IDENTIFIER)
            tok.identifier = m_state.m_lexer->control()->identifier(m_scratchBuffer.constData() + pos, len);
        else if (kind == T_NUMERIC_LITERAL)
            tok.number = m_state.m_lexer->control()->numericLiteral(m_scratchBuffer.constData() + pos, len);
    }
    tok.offset = pos;
    tok.f.length = len;
    tok.f.generated = true;
    tok.lineno = lineno;
    return tok;
}

PPToken Preprocessor::generateConcatenated(const PPToken &leftTk, const PPToken &rightTk)
{
    QByteArray newText;
    newText.reserve(leftTk.length() + rightTk.length());
    newText.append(leftTk.tokenStart(), leftTk.length());
    newText.append(rightTk.tokenStart(), rightTk.length());
    PPToken result = generateToken(T_IDENTIFIER, newText.constData(), newText.size(), leftTk.lineno, true);
    result.f.whitespace = leftTk.f.whitespace;
    return result;
}

void Preprocessor::startSkippingBlocks(const Preprocessor::PPToken &tk) const
{
    if (!m_client)
        return;

    int iter = tk.end();
    const QByteArray &txt = tk.source();
    for (; iter < txt.size(); ++iter) {
        if (txt.at(iter) == '\n') {
            m_client->startSkippingBlocks(iter + 1);
            return;
        }
    }
}
