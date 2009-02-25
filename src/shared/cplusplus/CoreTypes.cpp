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

#include "CoreTypes.h"
#include "TypeVisitor.h"
#include "Names.h"
#include <algorithm>

CPLUSPLUS_BEGIN_NAMESPACE


bool VoidType::isEqualTo(const Type *other) const
{
    const VoidType *o = other->asVoidType();
    return o != 0;
}

void VoidType::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

PointerToMemberType::PointerToMemberType(Name *memberName, FullySpecifiedType elementType)
    : _memberName(memberName),
      _elementType(elementType)
{ }

PointerToMemberType::~PointerToMemberType()
{ }

Name *PointerToMemberType::memberName() const
{ return _memberName; }

FullySpecifiedType PointerToMemberType::elementType() const
{ return _elementType; }

bool PointerToMemberType::isEqualTo(const Type *other) const
{
    const PointerToMemberType *o = other->asPointerToMemberType();
    if (! o)
        return false;
    else if (! _memberName->isEqualTo(o->_memberName))
        return false;
    return _elementType.isEqualTo(o->_elementType);
}

void PointerToMemberType::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

PointerType::PointerType(FullySpecifiedType elementType)
    : _elementType(elementType)
{ }

PointerType::~PointerType()
{ }

bool PointerType::isEqualTo(const Type *other) const
{
    const PointerType *o = other->asPointerType();
    if (! o)
        return false;
    return _elementType.isEqualTo(o->_elementType);
}

void PointerType::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

FullySpecifiedType PointerType::elementType() const
{ return _elementType; }

ReferenceType::ReferenceType(FullySpecifiedType elementType)
    : _elementType(elementType)
{ }

ReferenceType::~ReferenceType()
{ }

bool ReferenceType::isEqualTo(const Type *other) const
{
    const ReferenceType *o = other->asReferenceType();
    if (! o)
        return false;
    return _elementType.isEqualTo(o->_elementType);
}

void ReferenceType::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

FullySpecifiedType ReferenceType::elementType() const
{ return _elementType; }

IntegerType::IntegerType(int kind)
    : _kind(kind)
{ }

IntegerType::~IntegerType()
{ }

bool IntegerType::isEqualTo(const Type *other) const
{
    const IntegerType *o = other->asIntegerType();
    if (! o)
        return false;
    return _kind == o->_kind;
}

void IntegerType::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

int IntegerType::kind() const
{ return _kind; }

FloatType::FloatType(int kind)
    : _kind(kind)
{ }

FloatType::~FloatType()
{ }

void FloatType::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

int FloatType::kind() const
{ return _kind; }

bool FloatType::isEqualTo(const Type *other) const
{
    const FloatType *o = other->asFloatType();
    if (! o)
        return false;
    return _kind == o->_kind;
}

ArrayType::ArrayType(FullySpecifiedType elementType, size_t size)
    : _elementType(elementType), _size(size)
{ }

ArrayType::~ArrayType()
{ }

bool ArrayType::isEqualTo(const Type *other) const
{
    const ArrayType *o = other->asArrayType();
    if (! o)
        return false;
    else if (_size != o->_size)
        return false;
    return _elementType.isEqualTo(o->_elementType);
}

void ArrayType::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

FullySpecifiedType ArrayType::elementType() const
{ return _elementType; }

size_t ArrayType::size() const
{ return _size; }

NamedType::NamedType(Name *name)
    : _name(name)
{ }

NamedType::~NamedType()
{ }

Name *NamedType::name() const
{ return _name; }

bool NamedType::isEqualTo(const Type *other) const
{
    const NamedType *o = other->asNamedType();
    if (! o)
        return false;

    Name *name = _name;
    if (QualifiedNameId *q = name->asQualifiedNameId())
        name = q->unqualifiedNameId();

    Name *otherName = o->name();
    if (QualifiedNameId *q = otherName->asQualifiedNameId())
        otherName = q->unqualifiedNameId();

    return name->isEqualTo(otherName);
}

void NamedType::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

CPLUSPLUS_END_NAMESPACE
