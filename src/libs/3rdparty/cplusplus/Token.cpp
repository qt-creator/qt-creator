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

#include "Token.h"
#include "Literals.h"

using namespace CPlusPlus;

const char *token_names[] = {
    (""), ("<error>"),

    ("<C++ comment>"), ("<C++ doxy comment>"),
    ("<comment>"), ("<doxy comment>"),

    ("<identifier>"),

    ("<numeric literal>"),
    ("<char literal>"), ("<wide char literal>"), ("<utf16 char literal>"), ("<utf32 char literal>"),
    ("<string literal>"), ("<wide string literal>"), ("<utf8 string literal>"),
    ("<utf16 string literal>"), ("<utf32 string literal>"),
    ("<raw string literal>"), ("<raw wide string literal>"), ("<raw utf8 string literal>"),
    ("<raw utf16 string literal>"), ("<raw utf32 string literal>"),
    ("<@string literal>"), ("<angle string literal>"),

    ("&"), ("&&"), ("&="), ("->"), ("->*"), ("^"), ("^="), (":"), ("::"),
    (","), ("/"), ("/="), ("."), ("..."), (".*"), ("="), ("=="), ("!"),
    ("!="), (">"), (">="), (">>"), (">>="), ("{"), ("["), ("<"), ("<="),
    ("<<"), ("<<="), ("("), ("-"), ("-="), ("--"), ("%"), ("%="), ("|"),
    ("|="), ("||"), ("+"), ("+="), ("++"), ("#"), ("##"), ("?"), ("}"),
    ("]"), (")"), (";"), ("*"), ("*="), ("~"), ("~="),

    ("alignas"), ("alignof"), ("asm"), ("auto"), ("break"), ("case"), ("catch"),
    ("class"), ("const"), ("const_cast"), ("constexpr"), ("continue"),
    ("decltype"), ("default"),
    ("delete"), ("do"), ("dynamic_cast"), ("else"), ("enum"),
    ("explicit"), ("export"), ("extern"), ("false"), ("for"),
    ("friend"), ("goto"), ("if"), ("inline"),
    ("mutable"), ("namespace"), ("new"), ("noexcept"),
    ("nullptr"), ("operator"), ("private"),
    ("protected"), ("public"), ("register"), ("reinterpret_cast"),
    ("return"), ("sizeof"), ("static"), ("static_assert"),
    ("static_cast"), ("struct"), ("switch"), ("template"), ("this"), ("thread_local"),
    ("throw"), ("true"), ("try"), ("typedef"), ("typeid"), ("typename"),
    ("union"), ("using"), ("virtual"),
    ("volatile"), ("while"),

    // gnu
    ("__attribute__"), ("__thread"), ("__typeof__"),

    // objc @keywords
    ("@catch"), ("@class"), ("@compatibility_alias"), ("@defs"), ("@dynamic"),
    ("@encode"), ("@end"), ("@finally"), ("@implementation"), ("@interface"),
    ("@not_keyword"), ("@optional"), ("@package"), ("@private"), ("@property"),
    ("@protected"), ("@protocol"), ("@public"), ("@required"), ("@selector"),
    ("@synchronized"), ("@synthesize"), ("@throw"), ("@try"),

    // Primitive types
    ("bool"), ("char"), ("char16_t"), ("char32_t"), ("double"), ("float"), ("int"),
    ("long"), ("short"), ("signed"), ("unsigned"), ("void"), ("wchar_t"),

    // Qt keywords
    ("emit"), ("SIGNAL"), ("SLOT"), ("Q_SIGNAL"), ("Q_SLOT"), ("signals"), ("slots"),
    ("Q_FOREACH"), ("Q_D"), ("Q_Q"),
    ("Q_INVOKABLE"), ("Q_PROPERTY"), ("T_Q_PRIVATE_PROPERTY"),
    ("Q_INTERFACES"), ("Q_EMIT"), ("Q_ENUMS"), ("Q_FLAGS"),
    ("Q_PRIVATE_SLOT"), ("Q_DECLARE_INTERFACE"), ("Q_OBJECT"), ("Q_GADGET"),

};

void Token::reset()
{
    flags = 0;
    byteOffset = 0;
    utf16charOffset = 0;
    ptr = 0;
}

const char *Token::name(int kind)
{ return token_names[kind]; }

const char *Token::spell() const
{
    switch (f.kind) {
    case T_IDENTIFIER:
        return identifier->chars();

    case T_NUMERIC_LITERAL:
    case T_CHAR_LITERAL:
    case T_WIDE_CHAR_LITERAL:
    case T_UTF16_CHAR_LITERAL:
    case T_UTF32_CHAR_LITERAL:
    case T_STRING_LITERAL:
    case T_WIDE_STRING_LITERAL:
    case T_UTF8_STRING_LITERAL:
    case T_UTF16_STRING_LITERAL:
    case T_UTF32_STRING_LITERAL:
    case T_RAW_STRING_LITERAL:
    case T_RAW_WIDE_STRING_LITERAL:
    case T_RAW_UTF8_STRING_LITERAL:
    case T_RAW_UTF16_STRING_LITERAL:
    case T_RAW_UTF32_STRING_LITERAL:
    case T_AT_STRING_LITERAL:
    case T_ANGLE_STRING_LITERAL:
        return literal->chars();

    default:
        return token_names[f.kind];
    } // switch
}


