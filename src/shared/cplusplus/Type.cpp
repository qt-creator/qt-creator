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

#include "Type.h"
#include "TypeVisitor.h"
#include "CoreTypes.h"
#include "Symbols.h"

CPLUSPLUS_BEGIN_NAMESPACE

Type::Type()
{ }

Type::~Type()
{ }

bool Type::isVoidType() const
{ return dynamic_cast<const VoidType *>(this) != 0; }

bool Type::isIntegerType() const
{ return dynamic_cast<const IntegerType *>(this) != 0; }

bool Type::isFloatType() const
{ return dynamic_cast<const FloatType *>(this) != 0; }

bool Type::isPointerType() const
{ return dynamic_cast<const PointerType *>(this) != 0; }

bool Type::isPointerToMemberType() const
{ return dynamic_cast<const PointerToMemberType *>(this) != 0; }

bool Type::isReferenceType() const
{ return dynamic_cast<const ReferenceType *>(this) != 0; }

bool Type::isArrayType() const
{ return dynamic_cast<const ArrayType *>(this) != 0; }

bool Type::isNamedType() const
{ return dynamic_cast<const NamedType *>(this) != 0; }

bool Type::isFunction() const
{ return dynamic_cast<const Function *>(this) != 0; }

bool Type::isNamespace() const
{ return dynamic_cast<const Namespace *>(this) != 0; }

bool Type::isClass() const
{ return dynamic_cast<const Class *>(this) != 0; }

bool Type::isEnum() const
{ return dynamic_cast<const Enum *>(this) != 0; }

bool Type::isScopedSymbol() const
{ return dynamic_cast<const ScopedSymbol *>(this) != 0; }

const VoidType *Type::asVoidType() const
{ return dynamic_cast<const VoidType *>(this); }

const IntegerType *Type::asIntegerType() const
{ return dynamic_cast<const IntegerType *>(this); }

const FloatType *Type::asFloatType() const
{ return dynamic_cast<const FloatType *>(this); }

const PointerType *Type::asPointerType() const
{ return dynamic_cast<const PointerType *>(this); }

const PointerToMemberType *Type::asPointerToMemberType() const
{ return dynamic_cast<const PointerToMemberType *>(this); }

const ReferenceType *Type::asReferenceType() const
{ return dynamic_cast<const ReferenceType *>(this); }

const ArrayType *Type::asArrayType() const
{ return dynamic_cast<const ArrayType *>(this); }

const NamedType *Type::asNamedType() const
{ return dynamic_cast<const NamedType *>(this); }

const Function *Type::asFunction() const
{ return dynamic_cast<const Function *>(this); }

const Namespace *Type::asNamespace() const
{ return dynamic_cast<const Namespace *>(this); }

const Class *Type::asClass() const
{ return dynamic_cast<const Class *>(this); }

const Enum *Type::asEnum() const
{ return dynamic_cast<const Enum *>(this); }

const ScopedSymbol *Type::asScopedSymbol() const
{ return dynamic_cast<const ScopedSymbol *>(this); }

VoidType *Type::asVoidType()
{ return dynamic_cast<VoidType *>(this); }

IntegerType *Type::asIntegerType()
{ return dynamic_cast<IntegerType *>(this); }

FloatType *Type::asFloatType()
{ return dynamic_cast<FloatType *>(this); }

PointerType *Type::asPointerType()
{ return dynamic_cast<PointerType *>(this); }

PointerToMemberType *Type::asPointerToMemberType()
{ return dynamic_cast<PointerToMemberType *>(this); }

ReferenceType *Type::asReferenceType()
{ return dynamic_cast<ReferenceType *>(this); }

ArrayType *Type::asArrayType()
{ return dynamic_cast<ArrayType *>(this); }

NamedType *Type::asNamedType()
{ return dynamic_cast<NamedType *>(this); }

Function *Type::asFunction()
{ return dynamic_cast<Function *>(this); }

Namespace *Type::asNamespace()
{ return dynamic_cast<Namespace *>(this); }

Class *Type::asClass()
{ return dynamic_cast<Class *>(this); }

Enum *Type::asEnum()
{ return dynamic_cast<Enum *>(this); }

ScopedSymbol *Type::asScopedSymbol()
{ return dynamic_cast<ScopedSymbol *>(this); }

void Type::accept(TypeVisitor *visitor)
{
    if (visitor->preVisit(this))
        accept0(visitor);
    visitor->postVisit(this);
}

void Type::accept(Type *type, TypeVisitor *visitor)
{
    if (! type)
        return;

    type->accept(visitor);
}

CPLUSPLUS_END_NAMESPACE
