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

#include "FullySpecifiedType.h"
#include "Type.h"

CPLUSPLUS_BEGIN_NAMESPACE

FullySpecifiedType::FullySpecifiedType(Type *type) :
    _type(type), _flags(0)
{ }

FullySpecifiedType::~FullySpecifiedType()
{ }

bool FullySpecifiedType::isValid() const
{ return _type != 0; }

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
{ return _isConst; }

void FullySpecifiedType::setConst(bool isConst)
{ _isConst = isConst; }

bool FullySpecifiedType::isVolatile() const
{ return _isVolatile; }

void FullySpecifiedType::setVolatile(bool isVolatile)
{ _isVolatile = isVolatile; }

bool FullySpecifiedType::isSigned() const
{ return _isSigned; }

void FullySpecifiedType::setSigned(bool isSigned)
{ _isSigned = isSigned; }

bool FullySpecifiedType::isUnsigned() const
{ return _isUnsigned; }

void FullySpecifiedType::setUnsigned(bool isUnsigned)
{ _isUnsigned = isUnsigned; }

bool FullySpecifiedType::isFriend() const
{ return _isFriend; }

void FullySpecifiedType::setFriend(bool isFriend)
{ _isFriend = isFriend; }

bool FullySpecifiedType::isRegister() const
{ return _isRegister; }

void FullySpecifiedType::setRegister(bool isRegister)
{ _isRegister = isRegister; }

bool FullySpecifiedType::isStatic() const
{ return _isStatic; }

void FullySpecifiedType::setStatic(bool isStatic)
{ _isStatic = isStatic; }

bool FullySpecifiedType::isExtern() const
{ return _isExtern; }

void FullySpecifiedType::setExtern(bool isExtern)
{ _isExtern = isExtern; }

bool FullySpecifiedType::isMutable() const
{ return _isMutable; }

void FullySpecifiedType::setMutable(bool isMutable)
{ _isMutable = isMutable; }

bool FullySpecifiedType::isTypedef() const
{ return _isTypedef; }

void FullySpecifiedType::setTypedef(bool isTypedef)
{ _isTypedef = isTypedef; }

bool FullySpecifiedType::isInline() const
{ return _isInline; }

void FullySpecifiedType::setInline(bool isInline)
{ _isInline = isInline; }

bool FullySpecifiedType::isVirtual() const
{ return _isVirtual; }

void FullySpecifiedType::setVirtual(bool isVirtual)
{ _isVirtual = isVirtual; }

bool FullySpecifiedType::isExplicit() const
{ return _isExplicit; }

void FullySpecifiedType::setExplicit(bool isExplicit)
{ _isExplicit = isExplicit; }

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
{ return _type != 0; }

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

CPLUSPLUS_END_NAMESPACE
