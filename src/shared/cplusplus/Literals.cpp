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

#include "Literals.h"
#include <cstring>
#include <algorithm>
#include <iostream>

CPLUSPLUS_BEGIN_NAMESPACE

////////////////////////////////////////////////////////////////////////////////
Literal::Literal(const char *chars, unsigned size)
    : _index(0), _next(0)
{
    _chars = new char[size + 1];

    strncpy(_chars, chars, size);
    _chars[size] = '\0';

    _size = size;

    _hashCode = hashCode(_chars, _size);
}

Literal::~Literal()
{ delete[] _chars; }

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
NumericLiteral::NumericLiteral(const char *chars, unsigned size)
    : Literal(chars, size)
{ }

NumericLiteral::~NumericLiteral()
{ }

////////////////////////////////////////////////////////////////////////////////
Identifier::Identifier(const char *chars, unsigned size)
    : Literal(chars, size)
{ }

Identifier::~Identifier()
{ }

bool Identifier::isEqualTo(const Identifier *other) const
{
    if (this == other)
        return true;
    else if (hashCode() != other->hashCode())
        return false;
    else if (size() != other->size())
        return false;
    return ! strcmp(chars(), other->chars());
}

CPLUSPLUS_END_NAMESPACE
