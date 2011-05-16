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

#ifndef CPLUSPLUS_LITERALS_H
#define CPLUSPLUS_LITERALS_H

#include "CPlusPlusForwardDeclarations.h"
#include "Token.h"
#include "Name.h"

namespace CPlusPlus {

class CPLUSPLUS_EXPORT Literal
{
    Literal(const Literal &other);
    void operator =(const Literal &other);

public:
    typedef const char *iterator;
    typedef iterator const_iterator;

public:
    Literal(const char *chars, unsigned size);
    virtual ~Literal();

    iterator begin() const;
    iterator end() const;

    char at(unsigned index) const;
    const char *chars() const;
    unsigned size() const;

    unsigned hashCode() const;
    static unsigned hashCode(const char *chars, unsigned size);

    bool equalTo(const Literal *other) const;

    Literal *_next; // ## private

private:
    char *_chars;
    unsigned _size;
    unsigned _hashCode;

public:
    unsigned _index;     // ### private
};

class CPLUSPLUS_EXPORT StringLiteral: public Literal
{
public:
    StringLiteral(const char *chars, unsigned size);
    virtual ~StringLiteral();
};

class CPLUSPLUS_EXPORT NumericLiteral: public Literal
{
public:
    NumericLiteral(const char *chars, unsigned size);
    virtual ~NumericLiteral();

    bool isInt() const;
    bool isFloat() const;
    bool isDouble() const;
    bool isLongDouble() const;
    bool isLong() const;
    bool isLongLong() const;

    bool isUnsigned() const;
    bool isHex() const;

private:
    struct Flags {
        unsigned _type      : 8;
        unsigned _isHex     : 1;
        unsigned _isUnsigned: 1;
    };
    union {
        unsigned _flags;
        Flags f;
    };
};

class CPLUSPLUS_EXPORT Identifier: public Literal, public Name
{
public:
    Identifier(const char *chars, unsigned size);
    virtual ~Identifier();

    virtual const Identifier *identifier() const { return this; }

    virtual bool isEqualTo(const Name *other) const;

    virtual const Identifier *asNameId() const
    { return this; }

protected:
    virtual void accept0(NameVisitor *visitor) const;
};

} // namespace CPlusPlus


#endif // CPLUSPLUS_LITERALS_H
