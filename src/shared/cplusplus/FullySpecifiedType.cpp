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

#include "FullySpecifiedType.h"
#include "Type.h"
#include "CoreTypes.h"

CPLUSPLUS_BEGIN_NAMESPACE

FullySpecifiedType::FullySpecifiedType(Type *type) :
    _type(type), _flags(0)
{
    if (! type)
        _type = UndefinedType::instance();
}

FullySpecifiedType::~FullySpecifiedType()
{ }

bool FullySpecifiedType::isValid() const
{ return _type != UndefinedType::instance(); }

Type *FullySpecifiedType::type() const
{ return _type; }

void FullySpecifiedType::setType(Type *type)
{ _type = type; }

FullySpecifiedType FullySpecifiedType::qualifiedType() const
{
    FullySpecifiedType ty = *this;
    ty.setFriend(false);
    ty.setRegister(false);
    ty.setStatic(false);
    ty.setExtern(false);
    ty.setMutable(false);
    ty.setTypedef(false);
    return ty;
}

bool FullySpecifiedType::isConst() const
{ return f._isConst; }

void FullySpecifiedType::setConst(bool isConst)
{ f._isConst = isConst; }

bool FullySpecifiedType::isVolatile() const
{ return f._isVolatile; }

void FullySpecifiedType::setVolatile(bool isVolatile)
{ f._isVolatile = isVolatile; }

bool FullySpecifiedType::isSigned() const
{ return f._isSigned; }

void FullySpecifiedType::setSigned(bool isSigned)
{ f._isSigned = isSigned; }

bool FullySpecifiedType::isUnsigned() const
{ return f._isUnsigned; }

void FullySpecifiedType::setUnsigned(bool isUnsigned)
{ f._isUnsigned = isUnsigned; }

bool FullySpecifiedType::isFriend() const
{ return f._isFriend; }

void FullySpecifiedType::setFriend(bool isFriend)
{ f._isFriend = isFriend; }

bool FullySpecifiedType::isRegister() const
{ return f._isRegister; }

void FullySpecifiedType::setRegister(bool isRegister)
{ f._isRegister = isRegister; }

bool FullySpecifiedType::isStatic() const
{ return f._isStatic; }

void FullySpecifiedType::setStatic(bool isStatic)
{ f._isStatic = isStatic; }

bool FullySpecifiedType::isExtern() const
{ return f._isExtern; }

void FullySpecifiedType::setExtern(bool isExtern)
{ f._isExtern = isExtern; }

bool FullySpecifiedType::isMutable() const
{ return f._isMutable; }

void FullySpecifiedType::setMutable(bool isMutable)
{ f._isMutable = isMutable; }

bool FullySpecifiedType::isTypedef() const
{ return f._isTypedef; }

void FullySpecifiedType::setTypedef(bool isTypedef)
{ f._isTypedef = isTypedef; }

bool FullySpecifiedType::isInline() const
{ return f._isInline; }

void FullySpecifiedType::setInline(bool isInline)
{ f._isInline = isInline; }

bool FullySpecifiedType::isVirtual() const
{ return f._isVirtual; }

void FullySpecifiedType::setVirtual(bool isVirtual)
{ f._isVirtual = isVirtual; }

bool FullySpecifiedType::isExplicit() const
{ return f._isExplicit; }

void FullySpecifiedType::setExplicit(bool isExplicit)
{ f._isExplicit = isExplicit; }

bool FullySpecifiedType::isEqualTo(const FullySpecifiedType &other) const
{
    if (_flags != other._flags)
        return false;
    if (_type == other._type)
        return true;
    else if (! _type)
        return false;
    else
        return _type->isEqualTo(other._type);
}

Type &FullySpecifiedType::operator*()
{ return *_type; }

FullySpecifiedType::operator bool() const
{ return _type != UndefinedType::instance(); }

const Type &FullySpecifiedType::operator*() const
{ return *_type; }

Type *FullySpecifiedType::operator->()
{ return _type; }

const Type *FullySpecifiedType::operator->() const
{ return _type; }

bool FullySpecifiedType::operator == (const FullySpecifiedType &other) const
{ return _type == other._type && _flags == other._flags; }

bool FullySpecifiedType::operator != (const FullySpecifiedType &other) const
{ return ! operator ==(other); }

bool FullySpecifiedType::operator < (const FullySpecifiedType &other) const
{
    if (_type == other._type)
        return _flags < other._flags;
    return _type < other._type;
}

FullySpecifiedType FullySpecifiedType::simplified() const
{
    if (const ReferenceType *refTy = type()->asReferenceType())
        return refTy->elementType().simplified();

    return *this;
}

CPLUSPLUS_END_NAMESPACE
