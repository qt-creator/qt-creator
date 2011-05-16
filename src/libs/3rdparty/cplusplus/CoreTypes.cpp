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
#include "TypeMatcher.h"
#include "Names.h"
#include <algorithm>

using namespace CPlusPlus;

bool UndefinedType::isEqualTo(const Type *other) const
{
    if (other->isUndefinedType())
        return true;

    return false;
}

void UndefinedType::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool UndefinedType::matchType0(const Type *otherType, TypeMatcher *matcher) const
{
    if (const UndefinedType *otherUndefinedTy = otherType->asUndefinedType())
        return matcher->match(this, otherUndefinedTy);

    return false;
}

bool VoidType::isEqualTo(const Type *other) const
{
    const VoidType *o = other->asVoidType();
    return o != 0;
}

void VoidType::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool VoidType::matchType0(const Type *otherType, TypeMatcher *matcher) const
{
    if (const VoidType *otherVoidTy = otherType->asVoidType())
        return matcher->match(this, otherVoidTy);

    return false;
}

PointerToMemberType::PointerToMemberType(const Name *memberName, const FullySpecifiedType &elementType)
    : _memberName(memberName),
      _elementType(elementType)
{ }

PointerToMemberType::~PointerToMemberType()
{ }

const Name *PointerToMemberType::memberName() const
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

bool PointerToMemberType::matchType0(const Type *otherType, TypeMatcher *matcher) const
{
    if (const PointerToMemberType *otherTy = otherType->asPointerToMemberType())
        return matcher->match(this, otherTy);

    return false;
}

PointerType::PointerType(const FullySpecifiedType &elementType)
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

bool PointerType::matchType0(const Type *otherType, TypeMatcher *matcher) const
{
    if (const PointerType *otherTy = otherType->asPointerType())
        return matcher->match(this, otherTy);

    return false;
}

FullySpecifiedType PointerType::elementType() const
{ return _elementType; }

ReferenceType::ReferenceType(const FullySpecifiedType &elementType, bool rvalueRef)
    : _elementType(elementType), _rvalueReference(rvalueRef)
{ }

ReferenceType::~ReferenceType()
{ }

bool ReferenceType::isEqualTo(const Type *other) const
{
    const ReferenceType *o = other->asReferenceType();
    if (! o)
        return false;
    else if (isRvalueReference() != o->isRvalueReference())
        return false;
    return _elementType.isEqualTo(o->_elementType);
}

void ReferenceType::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool ReferenceType::matchType0(const Type *otherType, TypeMatcher *matcher) const
{
    if (const ReferenceType *otherTy = otherType->asReferenceType())
        return matcher->match(this, otherTy);

    return false;
}

FullySpecifiedType ReferenceType::elementType() const
{ return _elementType; }

bool ReferenceType::isRvalueReference() const
{ return _rvalueReference; }

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

bool IntegerType::matchType0(const Type *otherType, TypeMatcher *matcher) const
{
    if (const IntegerType *otherTy = otherType->asIntegerType())
        return matcher->match(this, otherTy);

    return false;
}

int IntegerType::kind() const
{ return _kind; }

FloatType::FloatType(int kind)
    : _kind(kind)
{ }

FloatType::~FloatType()
{ }

void FloatType::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool FloatType::matchType0(const Type *otherType, TypeMatcher *matcher) const
{
    if (const FloatType *otherTy = otherType->asFloatType())
        return matcher->match(this, otherTy);

    return false;
}

int FloatType::kind() const
{ return _kind; }

bool FloatType::isEqualTo(const Type *other) const
{
    const FloatType *o = other->asFloatType();
    if (! o)
        return false;
    return _kind == o->_kind;
}

ArrayType::ArrayType(const FullySpecifiedType &elementType, unsigned size)
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

bool ArrayType::matchType0(const Type *otherType, TypeMatcher *matcher) const
{
    if (const ArrayType *otherTy = otherType->asArrayType())
        return matcher->match(this, otherTy);

    return false;
}

FullySpecifiedType ArrayType::elementType() const
{ return _elementType; }

unsigned ArrayType::size() const
{ return _size; }

NamedType::NamedType(const Name *name)
    : _name(name)
{ }

NamedType::~NamedType()
{ }

const Name *NamedType::name() const
{ return _name; }

bool NamedType::isEqualTo(const Type *other) const
{
    const NamedType *o = other->asNamedType();
    if (! o)
        return false;

    const Name *name = _name;
    if (const QualifiedNameId *q = name->asQualifiedNameId())
        name = q->name();

    const Name *otherName = o->name();
    if (const QualifiedNameId *q = otherName->asQualifiedNameId())
        otherName = q->name();

    return name->isEqualTo(otherName);
}

void NamedType::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool NamedType::matchType0(const Type *otherType, TypeMatcher *matcher) const
{
    if (const NamedType *otherTy = otherType->asNamedType())
        return matcher->match(this, otherTy);

    return false;
}
