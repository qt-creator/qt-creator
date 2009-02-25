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

#include "Symbols.h"
#include "Names.h"
#include "TypeVisitor.h"
#include "SymbolVisitor.h"
#include "Scope.h"
#include <cstdlib>

CPLUSPLUS_BEGIN_NAMESPACE

UsingNamespaceDirective::UsingNamespaceDirective(TranslationUnit *translationUnit,
                                                 unsigned sourceLocation, Name *name)
    : Symbol(translationUnit, sourceLocation, name)
{ }

UsingNamespaceDirective::~UsingNamespaceDirective()
{ }

FullySpecifiedType UsingNamespaceDirective::type() const
{ return FullySpecifiedType(); }

void UsingNamespaceDirective::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }

UsingDeclaration::UsingDeclaration(TranslationUnit *translationUnit,
                                   unsigned sourceLocation, Name *name)
    : Symbol(translationUnit, sourceLocation, name)
{ }

UsingDeclaration::~UsingDeclaration()
{ }

FullySpecifiedType UsingDeclaration::type() const
{ return FullySpecifiedType(); }

void UsingDeclaration::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }

Declaration::Declaration(TranslationUnit *translationUnit, unsigned sourceLocation, Name *name)
    : Symbol(translationUnit, sourceLocation, name),
      _templateParameters(0)
{ }

Declaration::~Declaration()
{ delete _templateParameters; }

unsigned Declaration::templateParameterCount() const
{
    if (! _templateParameters)
        return 0;
    return _templateParameters->symbolCount();
}

Symbol *Declaration::templateParameterAt(unsigned index) const
{ return _templateParameters->symbolAt(index); }

Scope *Declaration::templateParameters() const
{ return _templateParameters; }

void Declaration::setTemplateParameters(Scope *templateParameters)
{ _templateParameters = templateParameters; }

void Declaration::setType(FullySpecifiedType type)
{ _type = type; }

FullySpecifiedType Declaration::type() const
{ return _type; }

void Declaration::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }

Argument::Argument(TranslationUnit *translationUnit, unsigned sourceLocation, Name *name)
    : Symbol(translationUnit, sourceLocation, name),
      _initializer(false)
{ }

Argument::~Argument()
{ }

bool Argument::hasInitializer() const
{ return _initializer; }

void Argument::setInitializer(bool hasInitializer)
{ _initializer = hasInitializer; }

void Argument::setType(FullySpecifiedType type)
{ _type = type; }

FullySpecifiedType Argument::type() const
{ return _type; }

void Argument::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }

Function::Function(TranslationUnit *translationUnit, unsigned sourceLocation, Name *name)
    : ScopedSymbol(translationUnit, sourceLocation, name),
     _templateParameters(0),
     _flags(0)
{ _arguments = new Scope(this); }

Function::~Function()
{
    delete _templateParameters;
    delete _arguments;
}

bool Function::isNormal() const
{ return _methodKey == NormalMethod; }

bool Function::isSignal() const
{ return _methodKey == SignalMethod; }

bool Function::isSlot() const
{ return _methodKey == SlotMethod; }

int Function::methodKey() const
{ return _methodKey; }

void Function::setMethodKey(int key)
{ _methodKey = key; }

unsigned Function::templateParameterCount() const
{
    if (! _templateParameters)
        return 0;
    return _templateParameters->symbolCount();
}

Symbol *Function::templateParameterAt(unsigned index) const
{ return _templateParameters->symbolAt(index); }

Scope *Function::templateParameters() const
{ return _templateParameters; }

void Function::setTemplateParameters(Scope *templateParameters)
{ _templateParameters = templateParameters; }

bool Function::isEqualTo(const Type *other) const
{
    const Function *o = other->asFunction();
    if (! o)
        return false;
    Name *l = identity();
    Name *r = o->identity();
    if (l == r || (l && l->isEqualTo(r))) {
        if (_arguments->symbolCount() != o->_arguments->symbolCount())
            return false;
        else if (! _returnType.isEqualTo(o->_returnType))
            return false;
        for (unsigned i = 0; i < _arguments->symbolCount(); ++i) {
            Symbol *l = _arguments->symbolAt(i);
            Symbol *r = o->_arguments->symbolAt(i);
            if (! l->type().isEqualTo(r->type()))
                return false;
        }
        return true;
    }
    return false;
}

void Function::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

FullySpecifiedType Function::type() const
{ return FullySpecifiedType(const_cast<Function *>(this)); }

FullySpecifiedType Function::returnType() const
{ return _returnType; }

void Function::setReturnType(FullySpecifiedType returnType)
{ _returnType = returnType; }

unsigned Function::argumentCount() const
{
    if (! _arguments)
        return 0;

    return _arguments->symbolCount();
}

Symbol *Function::argumentAt(unsigned index) const
{ return _arguments->symbolAt(index); }

Scope *Function::arguments() const
{ return _arguments; }

bool Function::isVariadic() const
{ return _isVariadic; }

void Function::setVariadic(bool isVariadic)
{ _isVariadic = isVariadic; }

bool Function::isConst() const
{ return _isConst; }

void Function::setConst(bool isConst)
{ _isConst = isConst; }

bool Function::isVolatile() const
{ return _isVolatile; }

void Function::setVolatile(bool isVolatile)
{ _isVolatile = isVolatile; }

bool Function::isPureVirtual() const
{ return _isPureVirtual; }

void Function::setPureVirtual(bool isPureVirtual)
{ _isPureVirtual = isPureVirtual; }

void Function::visitSymbol0(SymbolVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (unsigned i = 0; i < _arguments->symbolCount(); ++i) {
            visitSymbol(_arguments->symbolAt(i), visitor);
        }
        for (unsigned i = 0; i < memberCount(); ++i) {
            visitSymbol(memberAt(i), visitor);
        }
    }
}

ScopedSymbol::ScopedSymbol(TranslationUnit *translationUnit, unsigned sourceLocation, Name *name)
    : Symbol(translationUnit, sourceLocation, name)
{ _members = new Scope(this); }

ScopedSymbol::~ScopedSymbol()
{ delete _members; }

unsigned ScopedSymbol::memberCount() const
{
    if (! _members)
        return 0;
    return _members->symbolCount();
}

Symbol *ScopedSymbol::memberAt(unsigned index) const
{
    if (! _members)
        return 0;
    return _members->symbolAt(index);
}

Scope *ScopedSymbol::members() const
{ return _members; }

void ScopedSymbol::addMember(Symbol *member)
{ _members->enterSymbol(member); }

Block::Block(TranslationUnit *translationUnit, unsigned sourceLocation)
    : ScopedSymbol(translationUnit, sourceLocation, /*name = */ 0)
{ }

Block::~Block()
{ }

FullySpecifiedType Block::type() const
{ return FullySpecifiedType(); }

void Block::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }

Enum::Enum(TranslationUnit *translationUnit, unsigned sourceLocation, Name *name)
    : ScopedSymbol(translationUnit, sourceLocation, name)
{ }

Enum::~Enum()
{ }

FullySpecifiedType Enum::type() const
{ return FullySpecifiedType(const_cast<Enum *>(this)); }

bool Enum::isEqualTo(const Type *other) const
{
    const Enum *o = other->asEnum();
    if (! o)
        return false;
    Name *l = identity();
    Name *r = o->identity();
    if (l == r)
        return true;
    else if (! l)
        return false;
    return l->isEqualTo(r);
}

void Enum::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

void Enum::visitSymbol0(SymbolVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (unsigned i = 0; i < memberCount(); ++i) {
            visitSymbol(memberAt(i), visitor);
        }
    }
}

Namespace::Namespace(TranslationUnit *translationUnit, unsigned sourceLocation, Name *name)
    : ScopedSymbol(translationUnit, sourceLocation, name)
{ }

Namespace::~Namespace()
{ }

bool Namespace::isEqualTo(const Type *other) const
{
    const Namespace *o = other->asNamespace();
    if (! o)
        return false;
    Name *l = identity();
    Name *r = o->identity();
    if (l == r || (l && l->isEqualTo(r)))
        return true;
    return false;
}

void Namespace::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

void Namespace::visitSymbol0(SymbolVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (unsigned i = 0; i < memberCount(); ++i) {
            visitSymbol(memberAt(i), visitor);
        }
    }
}

FullySpecifiedType Namespace::type() const
{ return FullySpecifiedType(const_cast<Namespace *>(this)); }

BaseClass::BaseClass(TranslationUnit *translationUnit, unsigned sourceLocation, Name *name)
    : Symbol(translationUnit, sourceLocation, name),
      _isVirtual(false)
{ }

BaseClass::~BaseClass()
{ }

FullySpecifiedType BaseClass::type() const
{ return FullySpecifiedType(); }

bool BaseClass::isVirtual() const
{ return _isVirtual; }

void BaseClass::setVirtual(bool isVirtual)
{ _isVirtual = isVirtual; }

void BaseClass::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }

Class::Class(TranslationUnit *translationUnit, unsigned sourceLocation, Name *name)
    : ScopedSymbol(translationUnit, sourceLocation, name),
      _key(ClassKey),
      _templateParameters(0)
{ }

Class::~Class()
{ delete _templateParameters; }

bool Class::isClass() const
{ return _key == ClassKey; }

bool Class::isStruct() const
{ return _key == StructKey; }

bool Class::isUnion() const
{ return _key == UnionKey; }

Class::Key Class::classKey() const
{ return _key; }

void Class::setClassKey(Key key)
{ _key = key; }

unsigned Class::templateParameterCount() const
{
    if (! _templateParameters)
        return 0;
    return _templateParameters->symbolCount();
}

Symbol *Class::templateParameterAt(unsigned index) const
{ return _templateParameters->symbolAt(index); }

Scope *Class::templateParameters() const
{ return _templateParameters; }

void Class::setTemplateParameters(Scope *templateParameters)
{ _templateParameters = templateParameters; }

void Class::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

unsigned Class::baseClassCount() const
{ return _baseClasses.count(); }

BaseClass *Class::baseClassAt(unsigned index) const
{ return _baseClasses.at(index); }

void Class::addBaseClass(BaseClass *baseClass)
{ _baseClasses.push_back(baseClass); }

FullySpecifiedType Class::type() const
{ return FullySpecifiedType(const_cast<Class *>(this)); }

bool Class::isEqualTo(const Type *other) const
{
    const Class *o = other->asClass();
    if (! o)
        return false;
    Name *l = identity();
    Name *r = o->identity();
    if (l == r || (l && l->isEqualTo(r)))
        return true;
    else
        return false;
}

void Class::visitSymbol0(SymbolVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (unsigned i = 0; i < _baseClasses.size(); ++i) {
            visitSymbol(_baseClasses.at(i), visitor);
        }
        for (unsigned i = 0; i < memberCount(); ++i) {
            visitSymbol(memberAt(i), visitor);
        }
    }
}

CPLUSPLUS_END_NAMESPACE
