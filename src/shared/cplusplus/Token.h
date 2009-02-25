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

#ifndef CPLUSPLUS_TOKEN_H
#define CPLUSPLUS_TOKEN_H

#include "CPlusPlusForwardDeclarations.h"
#include <cstddef>

CPLUSPLUS_BEGIN_HEADER
CPLUSPLUS_BEGIN_NAMESPACE

enum Kind {
    T_EOF_SYMBOL = 0,
    T_ERROR,

    T_COMMENT,
    T_IDENTIFIER,

    T_FIRST_LITERAL,
    T_INT_LITERAL = T_FIRST_LITERAL,
    T_FLOAT_LITERAL,
    T_CHAR_LITERAL,
    T_WIDE_CHAR_LITERAL,
    T_STRING_LITERAL,
    T_WIDE_STRING_LITERAL,
    T_AT_STRING_LITERAL,
    T_ANGLE_STRING_LITERAL,
    T_LAST_LITERAL = T_ANGLE_STRING_LITERAL,

    T_FIRST_OPERATOR,
    T_AMPER = T_FIRST_OPERATOR,
    T_AMPER_AMPER,
    T_AMPER_EQUAL,
    T_ARROW,
    T_ARROW_STAR,
    T_CARET,
    T_CARET_EQUAL,
    T_COLON,
    T_COLON_COLON,
    T_COMMA,
    T_SLASH,
    T_SLASH_EQUAL,
    T_DOT,
    T_DOT_DOT_DOT,
    T_DOT_STAR,
    T_EQUAL,
    T_EQUAL_EQUAL,
    T_EXCLAIM,
    T_EXCLAIM_EQUAL,
    T_GREATER,
    T_GREATER_EQUAL,
    T_GREATER_GREATER,
    T_GREATER_GREATER_EQUAL,
    T_LBRACE,
    T_LBRACKET,
    T_LESS,
    T_LESS_EQUAL,
    T_LESS_LESS,
    T_LESS_LESS_EQUAL,
    T_LPAREN,
    T_MINUS,
    T_MINUS_EQUAL,
    T_MINUS_MINUS,
    T_PERCENT,
    T_PERCENT_EQUAL,
    T_PIPE,
    T_PIPE_EQUAL,
    T_PIPE_PIPE,
    T_PLUS,
    T_PLUS_EQUAL,
    T_PLUS_PLUS,
    T_POUND,
    T_POUND_POUND,
    T_QUESTION,
    T_RBRACE,
    T_RBRACKET,
    T_RPAREN,
    T_SEMICOLON,
    T_STAR,
    T_STAR_EQUAL,
    T_TILDE,
    T_TILDE_EQUAL,
    T_LAST_OPERATOR = T_TILDE_EQUAL,

    T_FIRST_KEYWORD,
    T_ASM = T_FIRST_KEYWORD,
    T_AUTO,
    T_BOOL,
    T_BREAK,
    T_CASE,
    T_CATCH,
    T_CHAR,
    T_CLASS,
    T_CONST,
    T_CONST_CAST,
    T_CONTINUE,
    T_DEFAULT,
    T_DELETE,
    T_DO,
    T_DOUBLE,
    T_DYNAMIC_CAST,
    T_ELSE,
    T_ENUM,
    T_EXPLICIT,
    T_EXPORT,
    T_EXTERN,
    T_FALSE,
    T_FLOAT,
    T_FOR,
    T_FRIEND,
    T_GOTO,
    T_IF,
    T_INLINE,
    T_INT,
    T_LONG,
    T_MUTABLE,
    T_NAMESPACE,
    T_NEW,
    T_OPERATOR,
    T_PRIVATE,
    T_PROTECTED,
    T_PUBLIC,
    T_REGISTER,
    T_REINTERPRET_CAST,
    T_RETURN,
    T_SHORT,
    T_SIGNED,
    T_SIZEOF,
    T_STATIC,
    T_STATIC_CAST,
    T_STRUCT,
    T_SWITCH,
    T_TEMPLATE,
    T_THIS,
    T_THROW,
    T_TRUE,
    T_TRY,
    T_TYPEDEF,
    T_TYPEID,
    T_TYPENAME,
    T_UNION,
    T_UNSIGNED,
    T_USING,
    T_VIRTUAL,
    T_VOID,
    T_VOLATILE,
    T_WCHAR_T,
    T_WHILE,

    T___ATTRIBUTE__,
    T___TYPEOF__,

    // obj c++ @ keywords
    T_FIRST_OBJC_AT_KEYWORD,

    T_AT_CATCH = T_FIRST_OBJC_AT_KEYWORD,
    T_AT_CLASS,
    T_AT_COMPATIBILITY_ALIAS,
    T_AT_DEFS,
    T_AT_DYNAMIC,
    T_AT_ENCODE,
    T_AT_END,
    T_AT_FINALLY,
    T_AT_IMPLEMENTATION,
    T_AT_INTERFACE,
    T_AT_NOT_KEYWORD,
    T_AT_OPTIONAL,
    T_AT_PACKAGE,
    T_AT_PRIVATE,
    T_AT_PROPERTY,
    T_AT_PROTECTED,
    T_AT_PROTOCOL,
    T_AT_PUBLIC,
    T_AT_REQUIRED,
    T_AT_SELECTOR,
    T_AT_SYNCHRONIZED,
    T_AT_SYNTHESIZE,
    T_AT_THROW,
    T_AT_TRY,

    T_LAST_OBJC_AT_KEYWORD,

    T_FIRST_QT_KEYWORD = T_LAST_OBJC_AT_KEYWORD,

    // Qt keywords
    T_SIGNAL = T_FIRST_QT_KEYWORD,
    T_SLOT,
    T_SIGNALS,
    T_SLOTS,

    T_LAST_KEYWORD = T_SLOTS,

    // aliases
    T_OR = T_PIPE_PIPE,
    T_AND = T_AMPER_AMPER,
    T_NOT = T_EXCLAIM,
    T_XOR = T_CARET,
    T_BITOR = T_PIPE,
    T_COMPL = T_TILDE,
    T_OR_EQ = T_PIPE_EQUAL,
    T_AND_EQ = T_AMPER_EQUAL,
    T_BITAND = T_AMPER,
    T_NOT_EQ = T_EXCLAIM_EQUAL,
    T_XOR_EQ = T_CARET_EQUAL,

    T___ASM = T_ASM,
    T___ASM__ = T_ASM,

    T_TYPEOF = T___TYPEOF__,
    T___TYPEOF = T___TYPEOF__,

    T___INLINE = T_INLINE,
    T___INLINE__ = T_INLINE,

    T___CONST = T_CONST,
    T___CONST__ = T_CONST,

    T___VOLATILE = T_VOLATILE,
    T___VOLATILE__ = T_VOLATILE,

    T___ATTRIBUTE = T___ATTRIBUTE__
};

class CPLUSPLUS_EXPORT Token
{
public:
    Token();
    ~Token();

    inline bool is(unsigned k) const    { return kind == k; }
    inline bool isNot(unsigned k) const { return kind != k; }
    const char *spell() const;
    void reset();

    inline unsigned begin() const
    { return offset; }

    inline unsigned end() const
    { return offset + length; }

    inline bool isLiteral() const
    { return kind >= T_FIRST_LITERAL && kind <= T_LAST_LITERAL; }

    inline bool isOperator() const
    { return kind >= T_FIRST_OPERATOR && kind <= T_LAST_OPERATOR; }

    inline bool isKeyword() const
    { return kind >= T_FIRST_KEYWORD && kind < T_FIRST_QT_KEYWORD; }

    inline bool isObjCAtKeyword() const
    { return kind >= T_FIRST_OBJC_AT_KEYWORD && kind < T_LAST_OBJC_AT_KEYWORD; }

    static const char *name(int kind);

public:
    union {
        unsigned flags;

        struct {
            unsigned kind       : 8;
            unsigned newline    : 1;
            unsigned whitespace : 1;
            unsigned joined     : 1;
            unsigned expanded   : 1;
            unsigned pad        : 4;
            unsigned length     : 16;
        };
    };

    unsigned offset;

    union {
        void *ptr;
        Literal *literal;
        NumericLiteral *number;
        StringLiteral *string;
        Identifier *identifier;
        unsigned close_brace;
        unsigned lineno;
    };
};

CPLUSPLUS_END_NAMESPACE
CPLUSPLUS_END_HEADER

#endif // CPLUSPLUS_TOKEN_H
