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

#ifndef CPLUSPLUS_PP_ENGINE_H
#define CPLUSPLUS_PP_ENGINE_H

#include "PreprocessorClient.h"

#include <Token.h>
#include <QVector>

namespace CPlusPlus {
    class Token;
}

namespace CPlusPlus {

    struct Value
    {
        enum Kind {
            Kind_Long,
            Kind_ULong,
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

    class CPLUSPLUS_EXPORT Preprocessor
    {
        Client *client;
        Environment &env;
        MacroExpander expand;

        enum { MAX_LEVEL = 512 };

        bool _skipping[MAX_LEVEL]; // ### move in state
        bool _true_test[MAX_LEVEL]; // ### move in state
        int iflevel; // ### move in state

        enum PP_DIRECTIVE_TYPE
        {
            PP_UNKNOWN_DIRECTIVE,
            PP_DEFINE,
            PP_INCLUDE,
            PP_INCLUDE_NEXT,
            PP_ELIF,
            PP_ELSE,
            PP_ENDIF,
            PP_IF,
            PP_IFDEF,
            PP_IFNDEF,
            PP_UNDEF
        };

        typedef const CPlusPlus::Token *TokenIterator;

        struct State {
            QByteArray source;
            QVector<CPlusPlus::Token> tokens;
            TokenIterator dot;
        };

        QList<State> _savedStates;

        State state() const;
        void pushState(const State &state);
        void popState();

        QByteArray _source;
        QVector<CPlusPlus::Token> _tokens;
        TokenIterator _dot;

        State createStateFromSource(const QByteArray &source) const;

    public:
        Preprocessor(Client *client, Environment &env);

        void operator()(const QByteArray &filename,
                        const QByteArray &source,
                        QByteArray *result);

        void operator()(const QByteArray &source,
                        QByteArray *result);

    private:
        void resetIfLevel();
        bool testIfLevel();
        int skipping() const;

        PP_DIRECTIVE_TYPE classifyDirective(const QByteArray &directive) const;

        Value evalExpression(TokenIterator firstToken,
                             TokenIterator lastToken,
                             const QByteArray &source) const;

        QVector<CPlusPlus::Token> tokenize(const QByteArray &text) const;

        const char *startOfToken(const CPlusPlus::Token &token) const;
        const char *endOfToken(const CPlusPlus::Token &token) const;

        QByteArray tokenSpell(const CPlusPlus::Token &token) const;
        QByteArray tokenText(const CPlusPlus::Token &token) const; // does a deep copy

        void processDirective(TokenIterator dot, TokenIterator lastToken);
        void processInclude(bool skipCurrentPath,
                            TokenIterator dot, TokenIterator lastToken,
                            bool acceptMacros = true);
        void processDefine(TokenIterator dot, TokenIterator lastToken);
        void processIf(TokenIterator dot, TokenIterator lastToken);
        void processElse(TokenIterator dot, TokenIterator lastToken);
        void processElif(TokenIterator dot, TokenIterator lastToken);
        void processEndif(TokenIterator dot, TokenIterator lastToken);
        void processIfdef(bool checkUndefined,
                          TokenIterator dot, TokenIterator lastToken);
        void processUndef(TokenIterator dot, TokenIterator lastToken);

        bool isQtReservedWord(const QByteArray &name) const;
    };

} // namespace CPlusPlus

#endif // CPLUSPLUS_PP_ENGINE_H
