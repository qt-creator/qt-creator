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

#include "FullySpecifiedType.h"
#include "Type.h"
#include "CoreTypes.h"

#include <functional>

using namespace CPlusPlus;

FullySpecifiedType::FullySpecifiedType() :
    _type(&UndefinedType::instance), _flags(0)
{}

FullySpecifiedType::FullySpecifiedType(Type *type) :
    _type(type), _flags(0)
{
    if (! type)
        _type = &UndefinedType::instance;
}

bool FullySpecifiedType::isValid() const
{ return _type != &UndefinedType::instance; }


FullySpecifiedType FullySpecifiedType::qualifiedType() const
{
    FullySpecifiedType ty = *this;
    ty.setFriend(false);
    ty.setRegister(false);
    ty.setStatic(false);
    ty.setExtern(false);
    ty.setMutable(false);
    ty.setTypedef(false);

    ty.setInline(false);
    ty.setVirtual(false);
    ty.setOverride(false);
    ty.setFinal(false);
    ty.setExplicit(false);

    ty.setDeprecated(false);
    ty.setUnavailable(false);
    return ty;
}

FullySpecifiedType::operator bool() const
{ return _type != &UndefinedType::instance; }

bool FullySpecifiedType::operator == (const FullySpecifiedType &other) const
{ return _type == other._type && _flags == other._flags; }

bool FullySpecifiedType::operator < (const FullySpecifiedType &other) const
{
    if (_type == other._type)
        return _flags < other._flags;
    return _type < other._type;
}

size_t FullySpecifiedType::hash() const
{
    return std::hash<Type *>()(_type) ^ std::hash<unsigned>()(_flags);
}

FullySpecifiedType FullySpecifiedType::simplified() const
{
    if (const ReferenceType *refTy = type()->asReferenceType())
        return refTy->elementType().simplified();

    return *this;
}

bool FullySpecifiedType::match(const FullySpecifiedType &otherTy, Matcher *matcher) const
{
    static const unsigned flagsMask = [](){
        FullySpecifiedType ty;
        ty.f._isConst = true;
        ty.f._isVolatile = true;
        ty.f._isSigned = true;
        ty.f._isUnsigned = true;
        return ty._flags;
    }();

    if ((_flags & flagsMask) != (otherTy._flags & flagsMask))
        return false;
    return type()->match(otherTy.type(), matcher);
}
