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

#ifndef CPLUSPLUS_LITERALTABLE_H
#define CPLUSPLUS_LITERALTABLE_H

#include "CPlusPlusForwardDeclarations.h"
#include <cstring>
#include <cstdlib>

CPLUSPLUS_BEGIN_HEADER
CPLUSPLUS_BEGIN_NAMESPACE

template <typename _Literal>
class LiteralTable
{
    LiteralTable(const LiteralTable &other);
    void operator =(const LiteralTable &other);

public:
    typedef _Literal **iterator;

public:
    LiteralTable()
       : _literals(0),
         _allocatedLiterals(0),
         _literalCount(-1),
         _buckets(0),
         _allocatedBuckets(0)
    { }

    ~LiteralTable()
    {
       if (_literals) {
           _Literal **lastLiteral = _literals + _literalCount + 1;
           for (_Literal **it = _literals; it != lastLiteral; ++it)
              delete *it;
           free(_literals);
       }
       if (_buckets)
           free(_buckets);
    }

    bool empty() const
    { return _literalCount == -1; }

    unsigned size() const
    { return _literalCount + 1; }

    _Literal *at(unsigned index) const
    { return _literals[index]; }

    iterator begin() const
    { return _literals; }

    iterator end() const
    { return _literals + _literalCount + 1; }

    _Literal *findOrInsertLiteral(const char *chars, unsigned size)
    {
       if (_buckets) {
           unsigned h = _Literal::hashCode(chars, size);
           _Literal *literal = _buckets[h % _allocatedBuckets];
           for (; literal; literal = static_cast<_Literal *>(literal->_next)) {
              if (literal->size() == size && ! strncmp(literal->chars(), chars, size))
                  return literal;
           }
       }

       _Literal *literal = new _Literal(chars, size);

       if (++_literalCount == _allocatedLiterals) {
           _allocatedLiterals <<= 1;

           if (! _allocatedLiterals)
              _allocatedLiterals = 256;

           _literals = (_Literal **) realloc(_literals, sizeof(_Literal *) * _allocatedLiterals);
       }

       _literals[_literalCount] = literal;

       if (! _buckets || _literalCount >= _allocatedBuckets * .6)
           rehash();
       else {
           unsigned h = literal->hashCode() % _allocatedBuckets;
           literal->_next = _buckets[h];
           _buckets[h] = literal;
       }

       return literal;
    }

protected:
    void rehash()
    {
       if (_buckets)
           free(_buckets);

       _allocatedBuckets <<= 1;

       if (! _allocatedBuckets)
           _allocatedBuckets = 256;

       _buckets = (_Literal **) calloc(_allocatedBuckets, sizeof(_Literal *));

       _Literal **lastLiteral = _literals + (_literalCount + 1);

       for (_Literal **it = _literals; it != lastLiteral; ++it) {
           _Literal *literal = *it;
           unsigned h = literal->hashCode() % _allocatedBuckets;

           literal->_next = _buckets[h];
           _buckets[h] = literal;
       }
    }

protected:
    MemoryPool *_pool;

    _Literal **_literals;
    int _allocatedLiterals;
    int _literalCount;

    _Literal **_buckets;
    int _allocatedBuckets;
};

CPLUSPLUS_END_NAMESPACE
CPLUSPLUS_END_HEADER

#endif // CPLUSPLUS_LITERALTABLE_H
