/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
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

#include <cplusplus/Control.h>
#include <cplusplus/Lexer.h>
#include <cplusplus/Token.h>
#include <cplusplus/Literals.h>

#include <utils/scopedswap.h>

#include <QDebug>
#include <QList>
#include <QDate>
#include <QTime>
#include <QPair>

#include <cctype>
#include <list>
#include <algorithm>

#define NO_DEBUG

#ifndef NO_DEBUG
#  include <iostream>
#endif // NO_DEBUG

#include <deque>

using namespace Utils;

namespace {
enum {
    MAX_TOKEN_EXPANSION_COUNT = 5000,
    MAX_TOKEN_BUFFER_DEPTH = 16000 // for when macros are using some kind of right-folding, this is the list of "delayed" buffers waiting to be expanded after the current one.
};
}

namespace {
static bool same(const char *a, const char *b, int size)
{
    return strncmp(a, b, size) == 0;
}

static bool isQtReservedWord(const char *name, int size)
{
    if (size < 4)
        return false;

    const char c = name[0];
    if (c == 'Q') {
        if (name[1] == '_') {
            name += 2;
            size -= 2;
            switch (size) {
            case 1:
                return name[2] == 'D' || name[2] == 'Q';
            case 4:
                return same(name, "SLOT", size) || same(name, "EMIT", size);
            case 5:
                return same(name, "SLOTS", size) || same(name, "ENUMS", size)
                        || same(name, "FLAGS", size);
            case 6:
                return same(name, "SIGNAL", size);
            case 7:
                return same(name, "SIGNALS", size) || same(name, "FOREACH", size);
            case 8:
                return same(name, "PROPERTY", size);
            case 9:
                return same(name, "INVOKABLE", size);
            case 10:
                return same(name, "INTERFACES", size);
            case 16:
                return same(name, "PRIVATE_PROPERTY", size);
            }
        }
        return false;
    }

    if (c == 'S')
        return (size == 6 && same(name, "SIGNAL", size)) || (size == 4 && same(name, "SLOT", size));

    if (c == 's')
        return (size == 7 && same(name, "signals", size)) || (size == 5 && same(name, "slots", size));

    if (c == 'f')
        return size == 7 && same(name, "foreach", size);

    if (c == 'e')
        return size == 4 && same(name, "emit", size);

    return false;
}

static void nestingTooDeep()
{
#ifndef NO_DEBUG
        std::cerr << "*** WARNING #if / #ifdef nesting exceeded the max level " << MAX_LEVEL << std::endl;
#endif
}

} // anonymous namespace

namespace CPlusPlus {

namespace Internal {
/// Buffers tokens for the Preprocessor::lex() to read next. Do not use  this
/// class directly, but use Preprocessor::State::pushTokenBuffer .
///
/// New tokens are added when undoing look-ahead, or after expanding a macro.
/// When macro expansion happened, the macro is passed in, and blocked until
/// all tokens generated by it (and by subsequent expansion of those generated
/// tokens) are read from the buffer. See Preprocessor::lex() for details on
/// exactly when the buffer (and subsequently a blocking macro) is removed.
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
            if (it->macro)
                if (it->macro == macro || (it->macro->name() == macro->name()))
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

inline bool isContinuationToken(const PPToken &tk)
{
    return tk.isNot(T_EOF_SYMBOL) && (! tk.newline() || tk.joined());
}

Macro *macroDefinition(const ByteArrayRef &name,
                       unsigned bytesOffset,
                       unsigned utf16charsOffset,
                       unsigned line,
                       Environment *env,
                       Client *client)
{
    Macro *m = env->resolve(name);
    if (client) {
        if (m)
            client->passedMacroDefinitionCheck(bytesOffset, utf16charsOffset, line, *m);
        else
            client->failedMacroDefinitionCheck(bytesOffset, utf16charsOffset, name);
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
        trivial.byteOffset = last->byteOffset;
        trivial.utf16charOffset = last->utf16charOffset;
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
        return source.constData() + (*_lex)->byteOffset;
    }

    int tokenLength() const
    {
        return (*_lex)->f.bytes;
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
            // TODO: if (vend != end) error(NaN)
            // TODO: binary literals
            // TODO: float literals
            ++(*_lex);
        } else if (isTokenDefined()) {
            ++(*_lex);
            if ((*_lex)->is(T_IDENTIFIER)) {
                _value.set_long(macroDefinition(tokenSpell(),
                                                (*_lex)->byteOffset,
                                                (*_lex)->utf16charOffset,
                                                (*_lex)->lineno, env, client)
                                != 0);
                ++(*_lex);
            } else if ((*_lex)->is(T_LPAREN)) {
                ++(*_lex);
                if ((*_lex)->is(T_IDENTIFIER)) {
                    _value.set_long(macroDefinition(tokenSpell(),
                                                    (*_lex)->byteOffset,
                                                    (*_lex)->utf16charOffset,
                                                    (*_lex)->lineno,
                                                    env, client)
                                    != 0);
                    ++(*_lex);
                    if ((*_lex)->is(T_RPAREN))
                        ++(*_lex);
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
    , m_tokenBufferDepth(0)
    , m_tokenBuffer(0)
    , m_inPreprocessorDirective(false)
    , m_markExpandedTokens(true)
    , m_noLines(false)
    , m_inCondition(false)
    , m_bytesOffsetRef(0)
    , m_utf16charsOffsetRef(0)
    , m_result(0)
    , m_lineRef(1)
    , m_currentExpansion(0)
    , m_includeGuardState(IncludeGuardState_BeforeIfndef)
{
    m_skipping[m_ifLevel] = false;
    m_trueTest[m_ifLevel] = false;

    m_expansionResult.reserve(256);
    setExpansionStatus(NotExpanding);
}

#define COMPRESS_TOKEN_BUFFER
void Preprocessor::State::pushTokenBuffer(const PPToken *start, const PPToken *end, const Macro *macro)
{
    if (m_tokenBufferDepth <= MAX_TOKEN_BUFFER_DEPTH) {
#ifdef COMPRESS_TOKEN_BUFFER
        if (macro || !m_tokenBuffer) {
            // If there is a new blocking macro (or no token buffer yet), create
            // one.
            m_tokenBuffer = new TokenBuffer(start, end, macro, m_tokenBuffer);
            ++m_tokenBufferDepth;
        } else {
            // No new blocking macro is passed in, so tokens can be prepended to
            // the existing buffer.
            m_tokenBuffer->tokens.insert(m_tokenBuffer->tokens.begin(), start, end);
        }
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

#ifdef DEBUG_INCLUDE_GUARD_TRACKING
QString Preprocessor::State::guardStateToString(int guardState)
{
    switch (guardState) {
    case IncludeGuardState_NoGuard: return QLatin1String("NoGuard");
    case IncludeGuardState_BeforeIfndef: return QLatin1String("BeforeIfndef");
    case IncludeGuardState_AfterIfndef: return QLatin1String("AfterIfndef");
    case IncludeGuardState_AfterDefine: return QLatin1String("AfterDefine");
    case IncludeGuardState_AfterEndif: return QLatin1String("AfterEndif");
    default: return QLatin1String("UNKNOWN");
    }
}
#endif // DEBUG_INCLUDE_GUARD_TRACKING

/**
 * @brief Update the include-guard tracking state.
 *
 * Include guards are the #ifdef/#define/#endif sequence typically found in
 * header files to prevent repeated definition of the contents of that header
 * file. So, for a file to have an include guard, it must look like this:
 * \code
 * #ifndef SOME_ID
 * ... all declarations/definitions/etc. go here ...
 * #endif
 * \endcode
 *
 * SOME_ID is an identifier, and is also the include guard. The only tokens
 * allowed before the #ifndef and after the #endif are comments (in any form)
 * or #line directives. The only other requirement is that a #define SOME_ID
 * occurs inside the #ifndef block, but not nested inside other
 * #if/#ifdef/#ifndef blocks.
 *
 * This function tracks the state, and is called from \c updateIncludeGuardState
 * which handles the most common no-op cases.
 *
 * @param hint indicates what kind of token is encountered in the input
 * @param idToken the identifier token that ought to be in the input
 *        after a #ifndef or a #define .
 */
void Preprocessor::State::updateIncludeGuardState_helper(IncludeGuardStateHint hint, PPToken *idToken)
{
#ifdef DEBUG_INCLUDE_GUARD_TRACKING
    int oldIncludeGuardState = m_includeGuardState;
    QByteArray oldIncludeGuardMacroName = m_includeGuardMacroName;
#endif // DEBUG_INCLUDE_GUARD_TRACKING

    switch (m_includeGuardState) {
    case IncludeGuardState_NoGuard:
        break;
    case IncludeGuardState_BeforeIfndef:
        if (hint == IncludeGuardStateHint_Ifndef
                && idToken && idToken->is(T_IDENTIFIER)) {
            m_includeGuardMacroName = idToken->asByteArrayRef().toByteArray();
            m_includeGuardState = IncludeGuardState_AfterIfndef;
        } else {
            m_includeGuardState = IncludeGuardState_NoGuard;
        }
        break;
    case IncludeGuardState_AfterIfndef:
        if (hint == IncludeGuardStateHint_Define
                && idToken && idToken->is(T_IDENTIFIER)
                && idToken->asByteArrayRef() == m_includeGuardMacroName)
            m_includeGuardState = IncludeGuardState_AfterDefine;
        break;
    case IncludeGuardState_AfterDefine:
        if (hint == IncludeGuardStateHint_Endif)
            m_includeGuardState = IncludeGuardState_AfterEndif;
        break;
    case IncludeGuardState_AfterEndif:
        m_includeGuardState = IncludeGuardState_NoGuard;
        m_includeGuardMacroName.clear();
        break;
    }

#ifdef DEBUG_INCLUDE_GUARD_TRACKING
    qDebug() << "***" << guardStateToString(oldIncludeGuardState)
             << "->" << guardStateToString(m_includeGuardState)
             << "hint:" << hint
             << "guard:" << oldIncludeGuardMacroName << "->" << m_includeGuardMacroName;
#endif // DEBUG_INCLUDE_GUARD_TRACKING
}

const QString Preprocessor::configurationFileName = QLatin1String("<configuration>");

Preprocessor::Preprocessor(Client *client, Environment *env)
    : m_client(client)
    , m_env(env)
    , m_expandFunctionlikeMacros(true)
    , m_keepComments(false)
{
}

QByteArray Preprocessor::run(const QString &fileName, const QString &source)
{
    return run(fileName, source.toUtf8());
}

QByteArray Preprocessor::run(const QString &fileName,
                             const QByteArray &source,
                             bool noLines,
                             bool markGeneratedTokens)
{
    m_scratchBuffer.clear();

    QByteArray preprocessed, includeGuardMacroName;
    preprocessed.reserve(source.size() * 2); // multiply by 2 because we insert #gen lines.
    preprocess(fileName, source, &preprocessed, &includeGuardMacroName, noLines,
               markGeneratedTokens, false);
    if (!includeGuardMacroName.isEmpty())
        m_client->markAsIncludeGuard(includeGuardMacroName);
    return preprocessed;
}

bool Preprocessor::expandFunctionlikeMacros() const
{
    return m_expandFunctionlikeMacros;
}

void Preprocessor::setExpandFunctionlikeMacros(bool expandMacros)
{
    m_expandFunctionlikeMacros = expandMacros;
}

bool Preprocessor::keepComments() const
{
    return m_keepComments;
}

void Preprocessor::setKeepComments(bool keepComments)
{
    m_keepComments = keepComments;
}

void Preprocessor::generateOutputLineMarker(unsigned lineno)
{
    maybeStartOutputLine();
    QByteArray &marker = currentOutputBuffer();
    marker.append("# ");
    marker.append(QByteArray::number(lineno));
    marker.append(" \"");
    marker.append(m_env->currentFileUtf8);
    marker.append("\"\n");
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
    } while (isContinuationToken(*tk));


    if (lparenSeen && tk->is(T_RPAREN))
        lex(tk);

    pushToken(tk);

    QByteArray result(1, '0');
    const ByteArrayRef macroName = idToken.asByteArrayRef();
    if (macroDefinition(macroName,
                        idToken.byteOffset + m_state.m_bytesOffsetRef,
                        idToken.utf16charOffset + m_state.m_utf16charsOffsetRef,
                        idToken.lineno, m_env, m_client)) {
        result[0] = '1';
    }
    *tk = generateToken(T_NUMERIC_LITERAL, result.constData(), result.size(), lineno, false);
}

void Preprocessor::pushToken(Preprocessor::PPToken *tk)
{
    const PPToken currentTokenBuffer[] = { *tk };
    m_state.pushTokenBuffer(currentTokenBuffer, currentTokenBuffer + 1, 0);
}

void Preprocessor::lex(PPToken *tk)
{
again:
    if (m_state.m_tokenBuffer) {
        // There is a token buffer, so read from there.
        if (m_state.m_tokenBuffer->tokens.empty()) {
            // The token buffer is empty, so pop it, and start over.
            m_state.popTokenBuffer();
            goto again;
        }
        *tk = m_state.m_tokenBuffer->tokens.front();
        m_state.m_tokenBuffer->tokens.pop_front();
        // The token buffer might now be empty. We leave it in, because the
        // token we just read might expand into new tokens, or might be a call
        // to the macro that generated this token. In either case, the macro
        // that generated the token still needs to be blocked (!), which is
        // recorded in the token buffer. Removing the blocked macro and the
        // empty token buffer happens the next time that this function is called.
    } else {
        // No token buffer, so have the lexer scan the next token.
        tk->setSource(m_state.m_source);
        m_state.m_lexer->scan(tk);
    }

    // Adjust token's line number in order to take into account the environment reference.
    tk->lineno += m_state.m_lineRef - 1;

reclassify:
    if (! m_state.m_inPreprocessorDirective) {
        if (tk->newline() && tk->is(T_POUND)) {
            handlePreprocessorDirective(tk);
            goto reclassify;
        } else if (tk->newline() && skipping()) {
            ScopedBoolSwap s(m_state.m_inPreprocessorDirective, true);
            do {
                lex(tk);
            } while (isContinuationToken(*tk));
            goto reclassify;
        } else if (tk->is(T_IDENTIFIER) && !isQtReservedWord(tk->tokenStart(), tk->bytes())) {
            m_state.updateIncludeGuardState(State::IncludeGuardStateHint_OtherToken);
            if (m_state.m_inCondition && tk->asByteArrayRef() == "defined") {
                handleDefined(tk);
            } else {
                synchronizeOutputLines(*tk);
                if (handleIdentifier(tk))
                    goto again;
            }
        } else if (tk->isNot(T_COMMENT) && tk->isNot(T_EOF_SYMBOL)) {
            m_state.updateIncludeGuardState(State::IncludeGuardStateHint_OtherToken);
        }
    }
}

void Preprocessor::skipPreprocesorDirective(PPToken *tk)
{
    ScopedBoolSwap s(m_state.m_inPreprocessorDirective, true);

    while (isContinuationToken(*tk)) {
        scanComment(tk);
        lex(tk);
    }
}

bool Preprocessor::handleIdentifier(PPToken *tk)
{
    ScopedBoolSwap s(m_state.m_inPreprocessorDirective, !tk->f.expanded);

    static const QByteArray ppLine("__LINE__");
    static const QByteArray ppFile("__FILE__");
    static const QByteArray ppDate("__DATE__");
    static const QByteArray ppTime("__TIME__");

    ByteArrayRef macroNameRef = tk->asByteArrayRef();

    if (macroNameRef.size() == 8
            && macroNameRef[0] == '_'
            && macroNameRef[1] == '_') {
        PPToken newTk;
        if (macroNameRef == ppLine) {
            QByteArray txt = QByteArray::number(tk->lineno);
            newTk = generateToken(T_STRING_LITERAL, txt.constData(), txt.size(), tk->lineno, false);
        } else if (macroNameRef == ppFile) {
            QByteArray txt;
            txt.append('"');
            txt.append(m_env->currentFileUtf8);
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

        if (newTk.hasSource()) {
            newTk.f.newline = tk->newline();
            newTk.f.whitespace = tk->whitespace();
            *tk = newTk;
            return false;
        }
    }

    Macro *macro = m_env->resolve(macroNameRef);
    if (!macro
            || (tk->expanded()
                && m_state.m_tokenBuffer
                && m_state.m_tokenBuffer->isBlocked(macro))) {
        return false;
    }
//    qDebug() << "expanding" << macro->name() << "on line" << tk->lineno;

    // Keep track the of the macro identifier token.
    PPToken idTk = *tk;

    // Expanded tokens which are not generated ones preserve the original line number from
    // their corresponding argument in macro substitution. For expanded tokens which are
    // generated, this information must be taken from somewhere else. What we do is to keep
    // a "reference" line initialize set to the line where expansion happens.
    unsigned baseLine = idTk.lineno - m_state.m_lineRef + 1;

    QVector<PPToken> body = macro->definitionTokens();

    // Within nested expansion we might reach a previously added marker token. In this case,
    // we need to move it from its current possition to outside the nesting.
    PPToken oldMarkerTk;

    if (macro->isFunctionLike()) {
        if (!expandFunctionlikeMacros()
                // Still expand if this originally started with an object-like macro.
                && m_state.m_expansionStatus != Expanding) {
            m_client->notifyMacroReference(m_state.m_bytesOffsetRef + idTk.byteOffset,
                                           m_state.m_utf16charsOffsetRef + idTk.utf16charOffset,
                                           idTk.lineno,
                                           *macro);
            return false;
        }

        // Collect individual tokens that form the macro arguments.
        QVector<QVector<PPToken> > allArgTks;
        bool hasArgs = collectActualArguments(tk, &allArgTks);

        // Check whether collecting arguments failed due to a previously added marker
        // that goot nested in a sequence of expansions. If so, store it and try again.
        if (!hasArgs
                && !tk->hasSource()
                && m_state.m_markExpandedTokens
                && (m_state.m_expansionStatus == Expanding
                    || m_state.m_expansionStatus == ReadyForExpansion)) {
            oldMarkerTk = *tk;
            hasArgs = collectActualArguments(tk, &allArgTks);
        }

        // Check for matching parameter/argument count.
        bool hasMatchingArgs = false;
        if (hasArgs) {
            const int expectedArgCount = macro->formals().size();
            if (macro->isVariadic() && allArgTks.size() == expectedArgCount - 1)
                allArgTks.push_back(QVector<PPToken>());
            const int actualArgCount = allArgTks.size();
            if (expectedArgCount == actualArgCount
                    || (macro->isVariadic() && actualArgCount > expectedArgCount - 1)
                    // Handle '#define foo()' when invoked as 'foo()'
                    || (expectedArgCount == 0
                        && actualArgCount == 1
                        && allArgTks.at(0).isEmpty())) {
                hasMatchingArgs = true;
            }
        }

        if (!hasArgs || !hasMatchingArgs) {
            //### TODO: error message
            pushToken(tk);
            // If a previous marker was found, make sure to put it back.
            if (oldMarkerTk.bytes())
                pushToken(&oldMarkerTk);
            *tk = idTk;
            return false;
        }

        if (m_client && !idTk.generated()) {
            // Bundle each token sequence into a macro argument "reference" for notification.
            // Even empty ones, which are not necessarily important on its own, but for the matter
            // of couting their number - such as in foo(,)
            QVector<MacroArgumentReference> argRefs;
            for (int i = 0; i < allArgTks.size(); ++i) {
                const QVector<PPToken> &argTks = allArgTks.at(i);
                if (argTks.isEmpty()) {
                    argRefs.push_back(MacroArgumentReference());
                } else {

                    argRefs.push_back(MacroArgumentReference(
                                  m_state.m_bytesOffsetRef + argTks.first().bytesBegin(),
                                  argTks.last().bytesBegin() + argTks.last().bytes()
                                    - argTks.first().bytesBegin(),
                                  m_state.m_utf16charsOffsetRef + argTks.first().utf16charsBegin(),
                                  argTks.last().utf16charsBegin() + argTks.last().utf16chars()
                                    - argTks.first().utf16charsBegin()));
                }
            }

            m_client->startExpandingMacro(m_state.m_bytesOffsetRef + idTk.byteOffset,
                                          m_state.m_utf16charsOffsetRef + idTk.utf16charOffset,
                                          idTk.lineno,
                                          *macro,
                                          argRefs);
        }

        if (!handleFunctionLikeMacro(macro, body, allArgTks, baseLine)) {
            if (m_client && !idTk.expanded())
                m_client->stopExpandingMacro(idTk.byteOffset, *macro);
            return false;
        }
    } else if (m_client && !idTk.generated()) {
        m_client->startExpandingMacro(m_state.m_bytesOffsetRef + idTk.byteOffset,
                                      m_state.m_utf16charsOffsetRef + idTk.utf16charOffset,
                                      idTk.lineno, *macro);
    }

    if (body.isEmpty()) {
        if (m_state.m_markExpandedTokens
                && (m_state.m_expansionStatus == NotExpanding
                    || m_state.m_expansionStatus == JustFinishedExpansion)) {
            // This is not the most beautiful approach but it's quite reasonable. What we do here
            // is to create a fake identifier token which is only composed by whitespaces. It's
            // also not marked as expanded so it it can be treated as a regular token.
            const QByteArray content(int(idTk.bytes() + computeDistance(idTk)), ' ');
            PPToken fakeIdentifier = generateToken(T_IDENTIFIER,
                                                   content.constData(), content.length(),
                                                   idTk.lineno, false, false);
            fakeIdentifier.f.whitespace = true;
            fakeIdentifier.f.expanded = false;
            fakeIdentifier.f.generated = false;
            body.push_back(fakeIdentifier);
        }
    } else {
        // The first body token replaces the macro invocation so its whitespace and
        // newline info is replicated.
        PPToken &bodyTk = body[0];
        bodyTk.f.whitespace = idTk.whitespace();
        bodyTk.f.newline = idTk.newline();

        // Expansions are tracked from a "top-level" basis. This means that each expansion
        // section in the output corresponds to a direct use of a macro (either object-like
        // or function-like) in the source code and all its recurring expansions - they are
        // surrounded by two marker tokens, one at the begin and the other at the end.
        // For instance, the following code will generate 3 expansions in total, but the
        // output will aggregate the tokens in only 2 expansion sections.
        //  - The first corresponds to BAR expanding to FOO and then FOO expanding to T o;
        //  - The second corresponds to FOO expanding to T o;
        //
        // #define FOO(T, o) T o;
        // #define BAR(T, o) FOO(T, o)
        // BAR(Test, x) FOO(Test, y)
        if (m_state.m_markExpandedTokens) {
            if (m_state.m_expansionStatus == NotExpanding
                    || m_state.m_expansionStatus == JustFinishedExpansion) {
                PPToken marker;
                marker.f.expanded = true;
                marker.f.bytes = idTk.bytes();
                marker.byteOffset = idTk.byteOffset;
                marker.lineno = idTk.lineno;
                body.prepend(marker);
                body.append(marker);
                m_state.setExpansionStatus(ReadyForExpansion);
            } else if (oldMarkerTk.bytes()
                       && (m_state.m_expansionStatus == ReadyForExpansion
                           || m_state.m_expansionStatus == Expanding)) {
                body.append(oldMarkerTk);
            }
        }
    }

    m_state.pushTokenBuffer(body.begin(), body.end(), macro);

    if (m_client && !idTk.generated())
        m_client->stopExpandingMacro(idTk.byteOffset, *macro);

    return true;
}

bool Preprocessor::handleFunctionLikeMacro(const Macro *macro,
                                           QVector<PPToken> &body,
                                           const QVector<QVector<PPToken> > &actuals,
                                           unsigned baseLine)
{
    QVector<PPToken> expanded;
    expanded.reserve(MAX_TOKEN_EXPANSION_COUNT);

    const size_t bodySize = body.size();
    for (size_t i = 0; i < bodySize && expanded.size() < MAX_TOKEN_EXPANSION_COUNT;
            ++i) {
        int expandedSize = expanded.size();
        PPToken bodyTk = body.at(int(i));

        if (bodyTk.is(T_IDENTIFIER)) {
            const ByteArrayRef id = bodyTk.asByteArrayRef();
            const QVector<QByteArray> &formals = macro->formals();
            int j = 0;
            for (; j < formals.size() && expanded.size() < MAX_TOKEN_EXPANSION_COUNT; ++j) {
                if (formals[j] == id) {
                    QVector<PPToken> actualsForThisParam = actuals.at(j);
                    unsigned lineno = baseLine;

                    // Collect variadic arguments
                    if (id == "__VA_ARGS__" || (macro->isVariadic() && j + 1 == formals.size())) {
                        for (int k = j + 1; k < actuals.size(); ++k) {
                            actualsForThisParam.append(generateToken(T_COMMA, ",", 1, lineno, true));
                            actualsForThisParam += actuals.at(k);
                        }
                    }

                    const int actualsSize = actualsForThisParam.size();

                    if (i > 0 && body[int(i) - 1].is(T_POUND)) {
                        QByteArray enclosedString;
                        enclosedString.reserve(256);

                        for (int i = 0; i < actualsSize; ++i) {
                            const PPToken &t = actualsForThisParam.at(i);
                            if (i == 0)
                                lineno = t.lineno;
                            else if (t.whitespace())
                                enclosedString.append(' ');
                            enclosedString.append(t.tokenStart(), t.bytes());
                        }
                        enclosedString.replace("\\", "\\\\");
                        enclosedString.replace("\"", "\\\"");

                        expanded.push_back(generateToken(T_STRING_LITERAL,
                                                         enclosedString.constData(),
                                                         enclosedString.size(),
                                                         lineno, true));
                    } else {
                        for (int k = 0; k < actualsSize; ++k) {
                            // Mark the actual tokens (which are the replaced version of the
                            // body's one) as expanded. For the first token we replicate the
                            // body's whitespace info.
                            PPToken actual = actualsForThisParam.at(k);
                            actual.f.expanded = true;
                            if (k == 0)
                                actual.f.whitespace = bodyTk.whitespace();
                            expanded += actual;
                            if (k == actualsSize - 1)
                                lineno = actual.lineno;
                        }
                    }

                    // Get a better (more up-to-date) value for the base line.
                    baseLine = lineno;

                    break;
                }
            }

            if (j == formals.size()) {
                // No formal macro parameter for this identifier in the body.
                bodyTk.f.generated = true;
                bodyTk.lineno = baseLine;
                expanded.push_back(bodyTk);
            }
        } else if (bodyTk.isNot(T_POUND) && bodyTk.isNot(T_POUND_POUND)) {
            bodyTk.f.generated = true;
            bodyTk.lineno = baseLine;
            expanded.push_back(bodyTk);
        }

        if (i > 1 && body[int(i) - 1].is(T_POUND_POUND)) {
            if (expandedSize < 1 || expanded.size() == expandedSize) //### TODO: [cpp.concat] placemarkers
                continue;
            const PPToken &leftTk = expanded[expandedSize - 1];
            const PPToken &rightTk = expanded[expandedSize];
            expanded[expandedSize - 1] = generateConcatenated(leftTk, rightTk);
            expanded.remove(expandedSize);
        }
    }

    // The "new" body.
    body = expanded;
    body.squeeze();

    return true;
}

void Preprocessor::trackExpansionCycles(PPToken *tk)
{
    if (m_state.m_markExpandedTokens) {
        // Identify a macro expansion section. The format is as follows:
        //
        // # expansion begin x,y ~g l:c
        // ...
        // # expansion end
        //
        // The x and y correspond, respectively, to the offset where the macro invocation happens
        // and the macro name's length. Following that there might be an unlimited number of
        // token marks which are directly mapped to each token that appears in the expansion.
        // Something like ~g indicates that the following g tokens are all generated. While
        // something like l:c indicates that the following token is expanded but not generated
        // and is positioned on line l and column c. Example:
        //
        // #define FOO(X) int f(X = 0)  // line 1
        // FOO(int
        //     a);
        //
        // Output would be:
        // # expansion begin 8,3 ~3 2:4 3:4 ~3
        // int f(int a = 0)
        // # expansion end
        // # 3 filename
        //       ;
        if (tk->expanded() && !tk->hasSource()) {
            if (m_state.m_expansionStatus == ReadyForExpansion) {
                m_state.setExpansionStatus(Expanding);
                m_state.m_expansionResult.clear();
                m_state.m_expandedTokensInfo.clear();
            } else if (m_state.m_expansionStatus == Expanding) {
                m_state.setExpansionStatus(JustFinishedExpansion);

                QByteArray &buffer = currentOutputBuffer();
                maybeStartOutputLine();

                // Offset and length of the macro invocation
                char chunk[40];
                qsnprintf(chunk, sizeof(chunk), "# expansion begin %d,%d", tk->byteOffset,
                          tk->bytes());
                buffer.append(chunk);

                // Expanded tokens
                unsigned generatedCount = 0;
                for (int i = 0; i < m_state.m_expandedTokensInfo.size(); ++i) {
                    const QPair<unsigned, unsigned> &p = m_state.m_expandedTokensInfo.at(i);
                    if (p.first) {
                        if (generatedCount) {
                            qsnprintf(chunk, sizeof(chunk), " ~%d", generatedCount);
                            buffer.append(chunk);
                            generatedCount = 0;
                        }
                        qsnprintf(chunk, sizeof(chunk), " %d:%d", p.first, p.second);
                        buffer.append(chunk);
                    } else {
                        ++generatedCount;
                    }
                }
                if (generatedCount) {
                    qsnprintf(chunk, sizeof(chunk), " ~%d", generatedCount);
                    buffer.append(chunk);
                }
                buffer.append('\n');
                buffer.append(m_state.m_expansionResult);
                maybeStartOutputLine();
                buffer.append("# expansion end\n");
            }

            lex(tk);

            if (tk->expanded() && !tk->hasSource())
                trackExpansionCycles(tk);
        }
    }
}

static void adjustForCommentOrStringNewlines(unsigned *currentLine, const PPToken &tk)
{
    if (tk.isComment() || tk.isStringLiteral())
        (*currentLine) += tk.asByteArrayRef().count('\n');
}

void Preprocessor::synchronizeOutputLines(const PPToken &tk, bool forceLine)
{
    if (m_state.m_expansionStatus != NotExpanding
            || (!forceLine && m_env->currentLine == tk.lineno)) {
        adjustForCommentOrStringNewlines(&m_env->currentLine, tk);
        return;
    }

    if (forceLine || m_env->currentLine > tk.lineno || tk.lineno - m_env->currentLine >= 9) {
        if (m_state.m_noLines) {
            if (!m_state.m_markExpandedTokens)
                currentOutputBuffer().append(' ');
        } else {
            generateOutputLineMarker(tk.lineno);
        }
    } else {
        for (unsigned i = m_env->currentLine; i < tk.lineno; ++i)
            currentOutputBuffer().append('\n');
    }

    m_env->currentLine = tk.lineno;
    adjustForCommentOrStringNewlines(&m_env->currentLine, tk);
}

std::size_t Preprocessor::computeDistance(const Preprocessor::PPToken &tk, bool forceTillLine)
{
    // Find previous non-space character or line begin.
    const char *buffer = tk.bufferStart();
    const char *tokenBegin = tk.tokenStart();
    const char *it = tokenBegin - 1;
    for (; it >= buffer; --it) {
        if (*it == '\n'|| (!pp_isspace(*it) && !forceTillLine))
            break;
    }
    ++it;

    return tokenBegin - it;
}


void Preprocessor::enforceSpacing(const Preprocessor::PPToken &tk, bool forceSpacing)
{
    if (tk.whitespace() || forceSpacing) {
        QByteArray &buffer = currentOutputBuffer();
        // For expanded tokens we simply add a whitespace, if necessary - the exact amount of
        // whitespaces is irrelevant within an expansion section. For real tokens we must be
        // more specific and get the information from the original source.
        if (tk.expanded() && !atStartOfOutputLine()) {
            buffer.append(' ');
        } else {
            const std::size_t spacing = computeDistance(tk, forceSpacing);
            const char *tokenBegin = tk.tokenStart();
            const char *it = tokenBegin - spacing;

            // Reproduce the content as in the original line.
            for (; it != tokenBegin; ++it)
                buffer.append(pp_isspace(*it) ? *it : ' ');
        }
    }
}

/// invalid pp-tokens are used as markers to force whitespace checks.
void Preprocessor::preprocess(const QString &fileName, const QByteArray &source,
                              QByteArray *result, QByteArray *includeGuardMacroName,
                              bool noLines,
                              bool markGeneratedTokens, bool inCondition,
                              unsigned bytesOffsetRef, unsigned utf16charOffsetRef,
                              unsigned lineRef)
{
    if (source.isEmpty())
        return;

    ScopedSwap<State> savedState(m_state, State());
    m_state.m_currentFileName = fileName;
    m_state.m_source = source;
    m_state.m_lexer = new Lexer(source.constBegin(), source.constEnd());
    m_state.m_lexer->setScanKeywords(false);
    m_state.m_lexer->setScanAngleStringLiteralTokens(false);
    m_state.m_lexer->setPreprocessorMode(true);
    if (m_keepComments)
        m_state.m_lexer->setScanCommentTokens(true);
    m_state.m_result = result;
    m_state.setExpansionStatus(m_state.m_expansionStatus); // Re-set m_currentExpansion
    m_state.m_noLines = noLines;
    m_state.m_markExpandedTokens = markGeneratedTokens;
    m_state.m_inCondition = inCondition;
    m_state.m_bytesOffsetRef = bytesOffsetRef;
    m_state.m_utf16charsOffsetRef = utf16charOffsetRef;
    m_state.m_lineRef = lineRef;

    ScopedSwap<QString> savedFileName(m_env->currentFile, fileName);
    ScopedSwap<QByteArray> savedUtf8FileName(m_env->currentFileUtf8, fileName.toUtf8());
    ScopedSwap<unsigned> savedCurrentLine(m_env->currentLine, 1);

    if (!m_state.m_noLines)
        generateOutputLineMarker(1);

    PPToken tk(m_state.m_source);
    do {
        lex(&tk);

        // Track the start and end of macro expansion cycles.
        trackExpansionCycles(&tk);

        bool macroExpanded = false;
        if (m_state.m_expansionStatus == Expanding) {
            // Collect the line and column from the tokens undergoing expansion. Those will
            // be available in the expansion section for further referencing about their real
            // location.
            unsigned trackedLine = 0;
            unsigned trackedColumn = 0;
            if (tk.expanded() && !tk.generated()) {
                trackedLine = tk.lineno;
                trackedColumn = unsigned(computeDistance(tk, true));
            }
            m_state.m_expandedTokensInfo.append(qMakePair(trackedLine, trackedColumn));
        } else if (m_state.m_expansionStatus == JustFinishedExpansion) {
            m_state.setExpansionStatus(NotExpanding);
            macroExpanded = true;
        }

        // Update environment line information.
        synchronizeOutputLines(tk, macroExpanded);

        // Make sure spacing between tokens is handled properly.
        enforceSpacing(tk, macroExpanded);

        // Finally output the token.
        currentOutputBuffer().append(tk.tokenStart(), tk.bytes());

    } while (tk.isNot(T_EOF_SYMBOL));

    if (includeGuardMacroName) {
        if (m_state.m_includeGuardState == State::IncludeGuardState_AfterDefine
                || m_state.m_includeGuardState == State::IncludeGuardState_AfterEndif)
            *includeGuardMacroName = m_state.m_includeGuardMacroName;
    }
    delete m_state.m_lexer;
    while (m_state.m_tokenBuffer)
        m_state.popTokenBuffer();
}

bool Preprocessor::scanComment(Preprocessor::PPToken *tk)
{
    if (!tk->isComment())
        return false;
    synchronizeOutputLines(*tk);
    enforceSpacing(*tk, true);
    currentOutputBuffer().append(tk->tokenStart(), tk->bytes());
    return true;
}

bool Preprocessor::consumeComments(PPToken *tk)
{
    while (scanComment(tk))
        lex(tk);
    return tk->isNot(T_EOF_SYMBOL);
}

bool Preprocessor::collectActualArguments(PPToken *tk, QVector<QVector<PPToken> > *actuals)
{
    Q_ASSERT(tk);
    Q_ASSERT(actuals);

    lex(tk); // consume the identifier

    bool lastCommentIsCpp = false;
    while (scanComment(tk)) {
        /* After C++ comments we need to add a new line
           e.g.
             #define foo(a, b) int a = b
             foo // comment
             (x, 3);
           can result in
                 // commentint
             x = 3;
        */
        lastCommentIsCpp = tk->is(T_CPP_COMMENT) || tk->is(T_CPP_DOXY_COMMENT);
        lex(tk);
    }
    if (lastCommentIsCpp)
        maybeStartOutputLine();

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

    if (!tk->is(T_RPAREN)) {
        //###TODO: else error message
    }
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

        if (m_keepComments
                && (tk->is(T_CPP_COMMENT) || tk->is(T_CPP_DOXY_COMMENT))) {
            // Even in keep comments mode, we cannot preserve C++ style comments inside the
            // expansion. We stick with GCC's approach which is to replace them by C style
            // comments (currently clang just gets rid of them) and transform internals */
            // into *|.
            QByteArray text = m_state.m_source.mid(tk->bytesBegin() + 2,
                                                   tk->bytesEnd() - tk->bytesBegin() - 2);
            const QByteArray &comment = "/*" + text.replace("*/", "*|") + "*/";
            tokens->append(generateToken(T_COMMENT,
                                         comment.constData(), comment.size(),
                                         tk->lineno, false));
        } else {
            tokens->append(*tk);
        }

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

        if (!skipping() && directive == ppDefine) {
            handleDefineDirective(tk);
        } else if (directive == ppIfNDef) {
            handleIfDefDirective(true, tk);
        } else if (directive == ppEndIf) {
            handleEndIfDirective(tk, poundToken);
        } else {
            m_state.updateIncludeGuardState(State::IncludeGuardStateHint_OtherToken);

            if (!skipping() && directive == ppUndef)
                handleUndefDirective(tk);
            else if (!skipping() && (directive == ppInclude
                                    || directive == ppImport))
                handleIncludeDirective(tk, false);
            else if (!skipping() && directive == ppIncludeNext)
                handleIncludeDirective(tk, true);
            else if (directive == ppIf)
                handleIfDirective(tk);
            else if (directive == ppIfDef)
                handleIfDefDirective(false, tk);
            else if (directive == ppElse)
                handleElseDirective(tk, poundToken);
            else if (directive == ppElif)
                handleElifDirective(tk, poundToken);
        }
    }

    skipPreprocesorDirective(tk);
}


void Preprocessor::handleIncludeDirective(PPToken *tk, bool includeNext)
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
    if (includeNext)
        mode = Client::IncludeNext;
    else if (included.at(0) == '"')
        mode = Client::IncludeLocal;
    else if (included.at(0) == '<')
        mode = Client::IncludeGlobal;
    else
        return; //### TODO: add error message?

    if (m_client) {
        QString inc = QString::fromUtf8(included.constData() + 1, included.size() - 2);
        m_client->sourceNeeded(line, inc, mode);
    }
}

void Preprocessor::handleDefineDirective(PPToken *tk)
{
    const unsigned defineOffset = tk->byteOffset;
    lex(tk); // consume "define" token

    if (!consumeComments(tk))
        return;

    if (tk->isNot(T_IDENTIFIER))
        return;

    Macro macro;
    macro.setFileName(m_env->currentFile);
    macro.setLine(tk->lineno);
    QByteArray macroName = tk->asByteArrayRef().toByteArray();
    macro.setName(macroName);
    macro.setBytesOffset(tk->byteOffset);
    macro.setUtf16charOffset(tk->utf16charOffset);

    PPToken idToken(*tk);

    lex(tk);

    if (isContinuationToken(*tk) && tk->is(T_LPAREN) && ! tk->whitespace()) {
        macro.setFunctionLike(true);

        lex(tk); // skip `('
        if (!consumeComments(tk))
            return;

        bool hasIdentifier = false;
        if (isContinuationToken(*tk) && tk->is(T_IDENTIFIER)) {
            hasIdentifier = true;
            macro.addFormal(tk->asByteArrayRef().toByteArray());

            lex(tk);
            if (!consumeComments(tk))
                return;

            while (isContinuationToken(*tk) && tk->is(T_COMMA)) {
                lex(tk);
                if (!consumeComments(tk))
                    return;

                if (isContinuationToken(*tk) && tk->is(T_IDENTIFIER)) {
                    macro.addFormal(tk->asByteArrayRef().toByteArray());
                    lex(tk);
                    if (!consumeComments(tk))
                        return;
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
            if (!consumeComments(tk))
                return;
        }
        if (isContinuationToken(*tk) && tk->is(T_RPAREN))
            lex(tk); // consume ")" token
    } else {
        if (m_state.m_ifLevel == 1)
            m_state.updateIncludeGuardState(State::IncludeGuardStateHint_Define, &idToken);
    }

    QVector<PPToken> bodyTokens;
    unsigned previousBytesOffset = 0;
    unsigned previousUtf16charsOffset = 0;
    unsigned previousLine = 0;
    Macro *macroReference = 0;
    while (isContinuationToken(*tk)) {
        // Macro tokens are always marked as expanded. However, only for object-like macros
        // we mark them as generated too. For function-like macros we postpone it until the
        // formals are identified in the bodies.
        tk->f.expanded = true;
        if (!macro.isFunctionLike())
            tk->f.generated = true;

        // Identifiers must not be eagerly expanded inside defines, but we should still notify
        // in the case they are macros.
        if (tk->is(T_IDENTIFIER) && m_client) {
            macroReference = m_env->resolve(tk->asByteArrayRef());
            if (macroReference) {
                if (!macroReference->isFunctionLike()) {
                    m_client->notifyMacroReference(tk->byteOffset, tk->utf16charOffset,
                                                   tk->lineno, *macroReference);
                    macroReference = 0;
                }
            }
        } else if (macroReference) {
            if (tk->is(T_LPAREN)) {
                m_client->notifyMacroReference(previousBytesOffset, previousUtf16charsOffset,
                                               previousLine, *macroReference);
            }
            macroReference = 0;
        }

        previousBytesOffset = tk->byteOffset;
        previousUtf16charsOffset = tk->utf16charOffset;
        previousLine = tk->lineno;

        if (!scanComment(tk))
            bodyTokens.push_back(*tk);

        lex(tk);
    }

    if (isQtReservedWord(macroName.data(), macroName.size())) {
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
    } else if (!bodyTokens.isEmpty()) {
        PPToken &firstBodyToken = bodyTokens[0];
        int start = firstBodyToken.byteOffset;
        int len = tk->byteOffset - start;
        QByteArray bodyText = firstBodyToken.source().mid(start, len).trimmed();

        const int bodySize = bodyTokens.size();
        for (int i = 0; i < bodySize; ++i) {
            PPToken &t = bodyTokens[i];
            if (t.hasSource())
                t.squeezeSource();
        }
        macro.setDefinition(bodyText, bodyTokens);
    }

    macro.setLength(tk->byteOffset - defineOffset);
    m_env->bind(macro);

//    qDebug() << "adding macro" << macro.name() << "defined at" << macro.fileName() << ":"<<macro.line();

    if (m_client)
        m_client->macroAdded(macro);
}

QByteArray Preprocessor::expand(PPToken *tk, PPToken *lastConditionToken)
{
    unsigned line = tk->lineno;
    unsigned bytesBegin = tk->bytesBegin();
    unsigned utf16charsBegin = tk->utf16charsBegin();
    PPToken lastTk;
    while (isContinuationToken(*tk)) {
        lastTk = *tk;
        lex(tk);
    }
    // Gather the exact spelling of the content in the source.
    QByteArray condition(m_state.m_source.mid(bytesBegin, lastTk.bytesBegin() + lastTk.bytes()
                                              - bytesBegin));

//    qDebug("*** Condition before: [%s]", condition.constData());
    QByteArray result;
    result.reserve(256);
    preprocess(m_state.m_currentFileName, condition, &result, 0, true, false, true,
               bytesBegin, utf16charsBegin, line);
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
    lexer.setPreprocessorMode(true);
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

    if (m_state.m_ifLevel >= MAX_LEVEL - 1) {
        nestingTooDeep();
        return;
    }

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
                // start skipping because the preceding then-part was not skipped
                m_state.m_skipping[m_state.m_ifLevel] = true;
                if (m_client)
                    startSkippingBlocks(poundToken);
            }
        } else {
            // preceding then-part was skipped, so calculate if we should start
            // skipping, depending on the condition
            Value result;
            evalExpression(tk, result);

            bool startSkipping = result.is_zero();
            m_state.m_trueTest[m_state.m_ifLevel] = !startSkipping;
            m_state.m_skipping[m_state.m_ifLevel] = startSkipping;
            if (m_client && !startSkipping)
                m_client->stopSkippingBlocks(poundToken.utf16charOffset - 1);
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
                m_client->stopSkippingBlocks(poundToken.utf16charOffset - 1);
            else if (m_client && !wasSkipping && startSkipping)
                startSkippingBlocks(poundToken);
        }
#ifndef NO_DEBUG
    } else {
        std::cerr << "*** WARNING #else without #if" << std::endl;
#endif // NO_DEBUG
    }
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
            m_client->stopSkippingBlocks(poundToken.utf16charOffset - 1);

        if (m_state.m_ifLevel == 0)
            m_state.updateIncludeGuardState(State::IncludeGuardStateHint_Endif);
    }

    lex(tk); // consume "endif" token
}

void Preprocessor::handleIfDefDirective(bool checkUndefined, PPToken *tk)
{
    lex(tk); // consume "ifdef" token
    if (tk->is(T_IDENTIFIER)) {
        if (checkUndefined && m_state.m_ifLevel == 0)
            m_state.updateIncludeGuardState(State::IncludeGuardStateHint_Ifndef, tk);

        bool value = false;
        const ByteArrayRef macroName = tk->asByteArrayRef();
        if (Macro *macro = macroDefinition(macroName, tk->byteOffset, tk->utf16charOffset,
                                           tk->lineno, m_env, m_client)) {
            value = true;

            // the macro is a feature constraint(e.g. QT_NO_XXX)
            if (checkUndefined && macroName.startsWith("QT_NO_")) {
                if (macro->fileName() == configurationFileName) {
                    // and it' defined in a pro file (e.g. DEFINES += QT_NO_QOBJECT)

                    value = false; // take the branch
                }
            }
        } else if (m_env->isBuiltinMacro(macroName)) {
            value = true;
        }

        if (checkUndefined)
            value = !value;

        const bool wasSkipping = m_state.m_skipping[m_state.m_ifLevel];

        if (m_state.m_ifLevel < MAX_LEVEL - 1) {
            ++m_state.m_ifLevel;
            m_state.m_trueTest[m_state.m_ifLevel] = value;
            m_state.m_skipping[m_state.m_ifLevel] = wasSkipping ? wasSkipping : !value;

            if (m_client && !wasSkipping && !value)
                startSkippingBlocks(*tk);
        } else {
            nestingTooDeep();
        }

        lex(tk); // consume the identifier
#ifndef NO_DEBUG
    } else {
        std::cerr << "*** WARNING #ifdef without identifier" << std::endl;
#endif // NO_DEBUG
    }
}

void Preprocessor::handleUndefDirective(PPToken *tk)
{
    lex(tk); // consume "undef" token
    if (tk->is(T_IDENTIFIER)) {
        const ByteArrayRef macroName = tk->asByteArrayRef();
        const unsigned bytesOffset = tk->byteOffset + m_state.m_bytesOffsetRef;
        const unsigned utf16charsOffset = tk->utf16charOffset + m_state.m_utf16charsOffsetRef;
        // Track macro use if previously defined
        if (m_client) {
            if (const Macro *existingMacro = m_env->resolve(macroName)) {
                m_client->notifyMacroReference(bytesOffset, utf16charsOffset,
                                               tk->lineno, *existingMacro);
            }
        }
        synchronizeOutputLines(*tk);
        Macro *macro = m_env->remove(macroName);

        if (m_client && macro) {
            macro->setBytesOffset(bytesOffset);
            macro->setUtf16charOffset(utf16charsOffset);
            m_client->macroAdded(*macro);
        }
        lex(tk); // consume macro name
#ifndef NO_DEBUG
    } else {
        std::cerr << "*** WARNING #undef without identifier" << std::endl;
#endif // NO_DEBUG
    }
}

PPToken Preprocessor::generateToken(enum Kind kind,
                                    const char *content, int length,
                                    unsigned lineno,
                                    bool addQuotes,
                                    bool addToControl)
{
    // When the token is a generated token, the column position cannot be
    // reconstructed, but we also have to prevent it from searching the whole
    // scratch buffer. So inserting a newline before the new token will give
    // an indent width of 0 (zero).
    m_scratchBuffer.append('\n');

    const size_t pos = m_scratchBuffer.size();

    if (kind == T_STRING_LITERAL && addQuotes)
        m_scratchBuffer.append('"');
    m_scratchBuffer.append(content, length);
    if (kind == T_STRING_LITERAL && addQuotes) {
        m_scratchBuffer.append('"');
        length += 2;
    }

    PPToken tk(m_scratchBuffer);
    tk.f.kind = kind;
    if (m_state.m_lexer->control() && addToControl) {
        if (kind == T_STRING_LITERAL)
            tk.string = m_state.m_lexer->control()->stringLiteral(m_scratchBuffer.constData() + pos, length);
        else if (kind == T_IDENTIFIER)
            tk.identifier = m_state.m_lexer->control()->identifier(m_scratchBuffer.constData() + pos, length);
        else if (kind == T_NUMERIC_LITERAL)
            tk.number = m_state.m_lexer->control()->numericLiteral(m_scratchBuffer.constData() + pos, length);
    }
    tk.byteOffset = unsigned(pos);
    tk.f.bytes = length;
    tk.f.generated = true;
    tk.f.expanded = true;
    tk.lineno = lineno;

    return tk;
}

PPToken Preprocessor::generateConcatenated(const PPToken &leftTk, const PPToken &rightTk)
{
    QByteArray newText;
    newText.reserve(leftTk.bytes() + rightTk.bytes());
    newText.append(leftTk.tokenStart(), leftTk.bytes());
    newText.append(rightTk.tokenStart(), rightTk.bytes());
    PPToken result = generateToken(T_IDENTIFIER, newText.constData(), newText.size(), leftTk.lineno, true);
    result.f.whitespace = leftTk.whitespace();
    return result;
}

void Preprocessor::startSkippingBlocks(const Preprocessor::PPToken &tk) const
{
    if (!m_client)
        return;

    unsigned utf16charIter = tk.utf16charsEnd();
    const char *source = tk.source().constData() + tk.bytesEnd();
    const char *sourceEnd = tk.source().constEnd();
    unsigned char yychar = *source;

    do {
        if (yychar == '\n') {
            m_client->startSkippingBlocks(utf16charIter + 1);
            return;
        }
        Lexer::yyinp_utf8(source, yychar, utf16charIter);
    } while (source < sourceEnd);
}

bool Preprocessor::atStartOfOutputLine() const
{
    const QByteArray *buffer = m_state.m_currentExpansion;
    return buffer->isEmpty() || buffer->endsWith('\n');
}

void Preprocessor::maybeStartOutputLine()
{
    QByteArray &buffer = currentOutputBuffer();
    if (buffer.isEmpty())
        return;
    if (!buffer.endsWith('\n'))
        buffer.append('\n');
    // If previous line ends with \ (possibly followed by whitespace), add another \n
    const char *start = buffer.constData();
    const char *ch = start + buffer.length() - 2;
    while (ch > start && (*ch != '\n') && std::isspace(*ch))
        --ch;
    if (*ch == '\\')
        buffer.append('\n');
}
