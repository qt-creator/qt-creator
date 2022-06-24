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
#include "Matcher.h"

#include <algorithm>

namespace CPlusPlus {

UndefinedType UndefinedType::instance;

void UndefinedType::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool UndefinedType::match0(const Type *otherType, Matcher *matcher) const
{
    if (const UndefinedType *otherUndefinedTy = otherType->asUndefinedType())
        return matcher->match(this, otherUndefinedTy);

    return false;
}

void VoidType::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool VoidType::match0(const Type *otherType, Matcher *matcher) const
{
    if (const VoidType *otherVoidTy = otherType->asVoidType())
        return matcher->match(this, otherVoidTy);

    return false;
}

PointerToMemberType::PointerToMemberType(const Name *memberName, const FullySpecifiedType &elementType)
    : _memberName(memberName),
      _elementType(elementType)
{ }


void PointerToMemberType::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool PointerToMemberType::match0(const Type *otherType, Matcher *matcher) const
{
    if (const PointerToMemberType *otherTy = otherType->asPointerToMemberType())
        return matcher->match(this, otherTy);

    return false;
}


void PointerType::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool PointerType::match0(const Type *otherType, Matcher *matcher) const
{
    if (const PointerType *otherTy = otherType->asPointerType())
        return matcher->match(this, otherTy);

    return false;
}


ReferenceType::ReferenceType(const FullySpecifiedType &elementType, bool rvalueRef)
    : _elementType(elementType), _rvalueReference(rvalueRef)
{ }

void ReferenceType::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool ReferenceType::match0(const Type *otherType, Matcher *matcher) const
{
    if (const ReferenceType *otherTy = otherType->asReferenceType())
        return matcher->match(this, otherTy);

    return false;
}

void IntegerType::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool IntegerType::match0(const Type *otherType, Matcher *matcher) const
{
    if (const IntegerType *otherTy = otherType->asIntegerType())
        return matcher->match(this, otherTy);

    return false;
}


void FloatType::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool FloatType::match0(const Type *otherType, Matcher *matcher) const
{
    if (const FloatType *otherTy = otherType->asFloatType())
        return matcher->match(this, otherTy);

    return false;
}


ArrayType::ArrayType(const FullySpecifiedType &elementType, unsigned size)
    : _elementType(elementType), _size(size)
{ }

void ArrayType::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool ArrayType::match0(const Type *otherType, Matcher *matcher) const
{
    if (const ArrayType *otherTy = otherType->asArrayType())
        return matcher->match(this, otherTy);

    return false;
}




void NamedType::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool NamedType::match0(const Type *otherType, Matcher *matcher) const
{
    if (const NamedType *otherTy = otherType->asNamedType())
        return matcher->match(this, otherTy);

    return false;
}

} // CPlusPlus
