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

namespace CPlusPlus {

template <typename _Literal>
class LiteralTable
{
    LiteralTable(const LiteralTable &other);
    void operator =(const LiteralTable &other);

public:
    typedef _Literal *const *iterator;

public:
    LiteralTable()
       : _literals(0),
         _buckets(0),
         _allocatedLiterals(0),
         _literalCount(-1),
         _allocatedBuckets(0)
    { }

    ~LiteralTable()
    {
        reset();
    }

    void reset()
    {
        if (_literals) {
            _Literal **lastLiteral = _literals + _literalCount + 1;
            for (_Literal **it = _literals; it != lastLiteral; ++it)
                delete *it;
            std::free(_literals);
        }
        if (_buckets)
            std::free(_buckets);

        _literals = 0;
        _buckets = 0;
        _allocatedLiterals = 0;
        _literalCount = -1;
        _allocatedBuckets = 0;
    }

    bool empty() const
    { return _literalCount == -1; }

    unsigned size() const
    { return _literalCount + 1; }

    const _Literal *at(unsigned index) const
    { return _literals[index]; }

    iterator begin() const
    { return _literals; }

    iterator end() const
    { return _literals + _literalCount + 1; }

    const _Literal *findLiteral(const char *chars, unsigned size) const
    {
        if (_buckets) {
            unsigned h = _Literal::hashCode(chars, size);
            _Literal *literal = _buckets[h % _allocatedBuckets];
            for (; literal; literal = static_cast<_Literal *>(literal->_next)) {
                if (literal->size() == size && ! std::strncmp(literal->chars(), chars, size))
                    return literal;
            }
        }

        return 0;
    }

    const _Literal *findOrInsertLiteral(const char *chars, unsigned size)
    {
        if (_buckets) {
            unsigned h = _Literal::hashCode(chars, size);
            _Literal *literal = _buckets[h % _allocatedBuckets];
            for (; literal; literal = static_cast<_Literal *>(literal->_next)) {
                if (literal->size() == size && ! std::strncmp(literal->chars(), chars, size))
                    return literal;
            }
        }

        _Literal *literal = new _Literal(chars, size);

        if (++_literalCount == _allocatedLiterals) {
            if (! _allocatedLiterals)
                _allocatedLiterals = 4;
            else
                _allocatedLiterals <<= 1;

            _literals = (_Literal **) std::realloc(_literals, sizeof(_Literal *) * _allocatedLiterals);
        }

        _literals[_literalCount] = literal;

        if (! _buckets || _literalCount * 5 >= _allocatedBuckets * 3)
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
           std::free(_buckets);

       if (! _allocatedBuckets)
           _allocatedBuckets = 4;
       else
           _allocatedBuckets <<= 1;

       _buckets = (_Literal **) std::calloc(_allocatedBuckets, sizeof(_Literal *));

       _Literal **lastLiteral = _literals + (_literalCount + 1);

       for (_Literal **it = _literals; it != lastLiteral; ++it) {
           _Literal *literal = *it;
           unsigned h = literal->hashCode() % _allocatedBuckets;

           literal->_next = _buckets[h];
           _buckets[h] = literal;
       }
    }

protected:
    _Literal **_literals;
    _Literal **_buckets;
    int _allocatedLiterals;
    int _literalCount;
    int _allocatedBuckets;
};

} // namespace CPlusPlus


#endif // CPLUSPLUS_LITERALTABLE_H
