/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
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

#ifndef CPLUSPLUS_CORETYPES_H
#define CPLUSPLUS_CORETYPES_H

#include "CPlusPlusForwardDeclarations.h"
#include "Type.h"
#include "FullySpecifiedType.h"
#include <cstddef>


namespace CPlusPlus {

class CPLUSPLUS_EXPORT UndefinedType : public Type
{
public:
    static UndefinedType *instance()
    {
        static UndefinedType t;
        return &t;
    }

    virtual bool isEqualTo(const Type *other) const
    { return this == other; }

protected:
    virtual void accept0(TypeVisitor *)
    { }
};

class CPLUSPLUS_EXPORT VoidType: public Type
{
public:
    virtual bool isEqualTo(const Type *other) const;

    virtual const VoidType *asVoidType() const
    { return this; }

    virtual VoidType *asVoidType()
    { return this; }

protected:
    virtual void accept0(TypeVisitor *visitor);
};

class CPLUSPLUS_EXPORT IntegerType: public Type
{
public:
    enum Kind {
        Char,
        WideChar,
        Bool,
        Short,
        Int,
        Long,
        LongLong
    };

public:
    IntegerType(int kind);
    virtual ~IntegerType();

    int kind() const;

    virtual bool isEqualTo(const Type *other) const;

    virtual IntegerType *asIntegerType()
    { return this; }

    virtual const IntegerType *asIntegerType() const
    { return this; }

protected:
    virtual void accept0(TypeVisitor *visitor);

private:
    int _kind;
};

class CPLUSPLUS_EXPORT FloatType: public Type
{
public:
    enum Kind {
        Float,
        Double,
        LongDouble
    };

public:
    FloatType(int kind);
    virtual ~FloatType();

    int kind() const;

    virtual bool isEqualTo(const Type *other) const;

    virtual const FloatType *asFloatType() const
    { return this; }

    virtual FloatType *asFloatType()
    { return this; }

protected:
    virtual void accept0(TypeVisitor *visitor);

private:
    int _kind;
};

class CPLUSPLUS_EXPORT PointerType: public Type
{
public:
    PointerType(const FullySpecifiedType &elementType);
    virtual ~PointerType();

    FullySpecifiedType elementType() const;

    virtual bool isEqualTo(const Type *other) const;

    virtual const PointerType *asPointerType() const
    { return this; }

    virtual PointerType *asPointerType()
    { return this; }

protected:
    virtual void accept0(TypeVisitor *visitor);

private:
    FullySpecifiedType _elementType;
};

class CPLUSPLUS_EXPORT PointerToMemberType: public Type
{
public:
    PointerToMemberType(Name *memberName, FullySpecifiedType elementType);
    virtual ~PointerToMemberType();

    Name *memberName() const;
    FullySpecifiedType elementType() const;

    virtual bool isEqualTo(const Type *other) const;

    virtual const PointerToMemberType *asPointerToMemberType() const
    { return this; }

    virtual PointerToMemberType *asPointerToMemberType()
    { return this; }

protected:
    virtual void accept0(TypeVisitor *visitor);

private:
    Name *_memberName;
    FullySpecifiedType _elementType;
};

class CPLUSPLUS_EXPORT ReferenceType: public Type
{
public:
    ReferenceType(const FullySpecifiedType &elementType);
    virtual ~ReferenceType();

    FullySpecifiedType elementType() const;

    virtual bool isEqualTo(const Type *other) const;

    virtual const ReferenceType *asReferenceType() const
    { return this; }

    virtual ReferenceType *asReferenceType()
    { return this; }

protected:
    virtual void accept0(TypeVisitor *visitor);

private:
    FullySpecifiedType _elementType;
};

class CPLUSPLUS_EXPORT ArrayType: public Type
{
public:
    ArrayType(const FullySpecifiedType &elementType, size_t size);
    virtual ~ArrayType();

    FullySpecifiedType elementType() const;
    size_t size() const;

    virtual bool isEqualTo(const Type *other) const;

    virtual const ArrayType *asArrayType() const
    { return this; }

    virtual ArrayType *asArrayType()
    { return this; }

protected:
    virtual void accept0(TypeVisitor *visitor);

private:
    FullySpecifiedType _elementType;
    size_t _size;
};

class CPLUSPLUS_EXPORT NamedType: public Type
{
public:
    NamedType(Name *name);
    virtual ~NamedType();

    Name *name() const;

    virtual bool isEqualTo(const Type *other) const;

    virtual const NamedType *asNamedType() const
    { return this; }

    virtual NamedType *asNamedType()
    { return this; }

protected:
    virtual void accept0(TypeVisitor *visitor);

private:
    Name *_name;
};

} // end of namespace CPlusPlus


#endif // CPLUSPLUS_CORETYPES_H
