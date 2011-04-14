/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#include "Literals.h"
#include "NameVisitor.h"
#include <cstring>
#include <algorithm>
#include <iostream>

using namespace CPlusPlus;

////////////////////////////////////////////////////////////////////////////////
Literal::Literal(const char *chars, unsigned size)
    : _next(0), _index(0)
{
    _chars = new char[size + 1];

    std::strncpy(_chars, chars, size);
    _chars[size] = '\0';
    _size = size;

    _hashCode = hashCode(_chars, _size);
}

Literal::~Literal()
{ delete[] _chars; }

bool Literal::equalTo(const Literal *other) const
{
    if (! other)
        return false;
    else if (this == other)
        return true;
    else if (hashCode() != other->hashCode())
        return false;
    else if (size() != other->size())
        return false;
    return ! std::strcmp(chars(), other->chars());
}

Literal::iterator Literal::begin() const
{ return _chars; }

Literal::iterator Literal::end() const
{ return _chars + _size; }

const char *Literal::chars() const
{ return _chars; }

char Literal::at(unsigned index) const
{ return _chars[index]; }

unsigned Literal::size() const
{ return _size; }

unsigned Literal::hashCode() const
{ return _hashCode; }

unsigned Literal::hashCode(const char *chars, unsigned size)
{
    unsigned h = 0;
    for (unsigned i = 0; i < size; ++i)
        h = (h >> 5) - h + chars[i];
    return h;
}

////////////////////////////////////////////////////////////////////////////////
StringLiteral::StringLiteral(const char *chars, unsigned size)
    : Literal(chars, size)
{ }

StringLiteral::~StringLiteral()
{ }

////////////////////////////////////////////////////////////////////////////////
enum {
    NumericLiteralIsInt,
    NumericLiteralIsFloat,
    NumericLiteralIsDouble,
    NumericLiteralIsLongDouble,
    NumericLiteralIsLong,
    NumericLiteralIsLongLong
};

NumericLiteral::NumericLiteral(const char *chars, unsigned size)
    : Literal(chars, size), _flags(0)
{
    f._type = NumericLiteralIsInt;

    if (size > 1 && chars[0] == '0' && (chars[1] == 'x' || chars[1] == 'X')) {
        f._isHex = true;
    } else {
        const char *begin = chars;
        const char *end = begin + size;

        bool done = false;
        const char *it = end - 1;

        for (; it != begin - 1 && ! done; --it) {
            switch (*it) {
            case 'l': case 'L': // long suffix
            case 'u': case 'U': // unsigned suffix
            case 'f': case 'F': // floating suffix
                break;

            default:
                done = true;
                break;
            } // switch
        }

        for (const char *dot = it; it != begin - 1; --it) {
            if (*dot == '.')
                f._type = NumericLiteralIsDouble;
        }

        for (++it; it != end; ++it) {
            if (*it == 'l' || *it == 'L') {
                if (f._type == NumericLiteralIsDouble) {
                    f._type = NumericLiteralIsLongDouble;
                } else if (it + 1 != end && (it[1] == 'l' || it[1] == 'L')) {
                    ++it;
                    f._type = NumericLiteralIsLongLong;
                } else {
                    f._type = NumericLiteralIsLong;
                }
            } else if (*it == 'f' || *it == 'F') {
                f._type = NumericLiteralIsFloat;
            } else if (*it == 'u' || *it == 'U') {
                f._isUnsigned = true;
            }
        }
    }
}

NumericLiteral::~NumericLiteral()
{ }

bool NumericLiteral::isHex() const
{ return f._isHex; }

bool NumericLiteral::isUnsigned() const
{ return f._isUnsigned; }

bool NumericLiteral::isInt() const
{ return f._type == NumericLiteralIsInt; }

bool NumericLiteral::isFloat() const
{ return f._type == NumericLiteralIsFloat; }

bool NumericLiteral::isDouble() const
{ return f._type == NumericLiteralIsDouble; }

bool NumericLiteral::isLongDouble() const
{ return f._type == NumericLiteralIsLongDouble; }

bool NumericLiteral::isLong() const
{ return f._type == NumericLiteralIsLong; }

bool NumericLiteral::isLongLong() const
{ return f._type == NumericLiteralIsLongLong; }

////////////////////////////////////////////////////////////////////////////////
Identifier::Identifier(const char *chars, unsigned size)
    : Literal(chars, size)
{ }

Identifier::~Identifier()
{ }

void Identifier::accept0(NameVisitor *visitor) const
{ visitor->visit(this); }

bool Identifier::isEqualTo(const Name *other) const
{
    if (this == other)
        return true;

    else if (other) {
        if (const Identifier *nameId = other->asNameId()) {
            return equalTo(nameId);
        }
    }
    return false;
}
