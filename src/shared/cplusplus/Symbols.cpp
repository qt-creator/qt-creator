/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "Symbols.h"
#include "Names.h"
#include "TypeVisitor.h"
#include "SymbolVisitor.h"
#include "TypeMatcher.h"
#include "Scope.h"

using namespace CPlusPlus;

TemplateParameters::TemplateParameters(Scope *scope)
    : _previous(0), _scope(scope)
{ }

TemplateParameters::TemplateParameters(TemplateParameters *previous, Scope *scope)
    : _previous(previous), _scope(scope)
{ }

TemplateParameters::~TemplateParameters()
{
    delete _previous;
    delete _scope;
}

TemplateParameters *TemplateParameters::previous() const
{ return _previous; }

Scope *TemplateParameters::scope() const
{ return _scope; }

UsingNamespaceDirective::UsingNamespaceDirective(TranslationUnit *translationUnit,
                                                 unsigned sourceLocation, const Name *name)
    : Symbol(translationUnit, sourceLocation, name)
{ }

UsingNamespaceDirective::~UsingNamespaceDirective()
{ }

FullySpecifiedType UsingNamespaceDirective::type() const
{ return FullySpecifiedType(); }

void UsingNamespaceDirective::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }

UsingDeclaration::UsingDeclaration(TranslationUnit *translationUnit,
                                   unsigned sourceLocation, const Name *name)
    : Symbol(translationUnit, sourceLocation, name)
{ }

UsingDeclaration::~UsingDeclaration()
{ }

FullySpecifiedType UsingDeclaration::type() const
{ return FullySpecifiedType(); }

void UsingDeclaration::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }

Declaration::Declaration(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name)
    : Symbol(translationUnit, sourceLocation, name),
      _templateParameters(0)
{ }

Declaration::~Declaration()
{ delete _templateParameters; }

TemplateParameters *Declaration::templateParameters() const
{ return _templateParameters; }

void Declaration::setTemplateParameters(TemplateParameters *templateParameters)
{ _templateParameters = templateParameters; }

void Declaration::setType(const FullySpecifiedType &type)
{ _type = type; }

FullySpecifiedType Declaration::type() const
{ return _type; }

void Declaration::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }

Argument::Argument(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name)
    : Symbol(translationUnit, sourceLocation, name),
      _initializer(0)
{ }

Argument::~Argument()
{ }

bool Argument::hasInitializer() const
{ return _initializer != 0; }

const StringLiteral *Argument::initializer() const
{ return _initializer; }

void Argument::setInitializer(const StringLiteral *initializer)
{ _initializer = initializer; }

void Argument::setType(const FullySpecifiedType &type)
{ _type = type; }

FullySpecifiedType Argument::type() const
{ return _type; }

void Argument::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }

TypenameArgument::TypenameArgument(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name)
    : Symbol(translationUnit, sourceLocation, name)
{ }

TypenameArgument::~TypenameArgument()
{ }

void TypenameArgument::setType(const FullySpecifiedType &type)
{ _type = type; }

FullySpecifiedType TypenameArgument::type() const
{ return _type; }

void TypenameArgument::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }

Function::Function(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name)
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
{ return f._methodKey == NormalMethod; }

bool Function::isSignal() const
{ return f._methodKey == SignalMethod; }

bool Function::isSlot() const
{ return f._methodKey == SlotMethod; }

int Function::methodKey() const
{ return f._methodKey; }

void Function::setMethodKey(int key)
{ f._methodKey = key; }

unsigned Function::templateParameterCount() const
{
    if (! _templateParameters)
        return 0;

    return _templateParameters->scope()->symbolCount();
}

Symbol *Function::templateParameterAt(unsigned index) const
{ return _templateParameters->scope()->symbolAt(index); }

TemplateParameters *Function::templateParameters() const
{ return _templateParameters; }

void Function::setTemplateParameters(TemplateParameters *templateParameters)
{ _templateParameters = templateParameters; }

bool Function::isEqualTo(const Type *other) const
{
    const Function *o = other->asFunctionType();
    if (! o)
        return false;
    else if (isConst() != o->isConst())
        return false;
    else if (isVolatile() != o->isVolatile())
        return false;
#ifdef ICHECK_BUILD
    else if (isInvokable() != o->isInvokable())
        return false;
    else if (isSignal() != o->isSignal())
        return false;
#endif

    const Name *l = identity();
    const Name *r = o->identity();
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

#ifdef ICHECK_BUILD
bool Function::isEqualTo(const Function* fct, bool ignoreName/* = false*/) const
{
    if(!ignoreName)
        return isEqualTo((Type*)fct);

    if (! fct)
        return false;
    else if (isConst() != fct->isConst())
        return false;
    else if (isVolatile() != fct->isVolatile())
        return false;
    else if (isInvokable() != fct->isInvokable())
        return false;
    else if (isSignal() != fct->isSignal())
        return false;

    if (_arguments->symbolCount() != fct->_arguments->symbolCount())
        return false;
    else if (! _returnType.isEqualTo(fct->_returnType))
        return false;
    for (unsigned i = 0; i < _arguments->symbolCount(); ++i) {
        Symbol *l = _arguments->symbolAt(i);
        Symbol *r = fct->_arguments->symbolAt(i);
        if (! l->type().isEqualTo(r->type()))
            return false;
    }
    return true;
}
#endif

void Function::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool Function::matchType0(const Type *otherType, TypeMatcher *matcher) const
{
    if (const Function *otherTy = otherType->asFunctionType())
        return matcher->match(this, otherTy);

    return false;
}

FullySpecifiedType Function::type() const
{ return FullySpecifiedType(const_cast<Function *>(this)); }

FullySpecifiedType Function::returnType() const
{ return _returnType; }

void Function::setReturnType(const FullySpecifiedType &returnType)
{ _returnType = returnType; }

bool Function::hasReturnType() const
{
    const FullySpecifiedType ty = returnType();
    return ty.isValid() || ty.isSigned() || ty.isUnsigned();
}

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

bool Function::hasArguments() const
{
    return ! (argumentCount() == 0 ||
              (argumentCount() == 1 && argumentAt(0)->type()->isVoidType()));
}

bool Function::isVirtual() const
{ return f._isVirtual; }

void Function::setVirtual(bool isVirtual)
{ f._isVirtual = isVirtual; }

bool Function::isVariadic() const
{ return f._isVariadic; }

void Function::setVariadic(bool isVariadic)
{ f._isVariadic = isVariadic; }

bool Function::isConst() const
{ return f._isConst; }

void Function::setConst(bool isConst)
{ f._isConst = isConst; }

bool Function::isVolatile() const
{ return f._isVolatile; }

void Function::setVolatile(bool isVolatile)
{ f._isVolatile = isVolatile; }

bool Function::isPureVirtual() const
{ return f._isPureVirtual; }

void Function::setPureVirtual(bool isPureVirtual)
{ f._isPureVirtual = isPureVirtual; }

bool Function::isAmbiguous() const
{ return f._isAmbiguous; }

void Function::setAmbiguous(bool isAmbiguous)
{ f._isAmbiguous = isAmbiguous; }

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

ScopedSymbol::ScopedSymbol(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name)
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
{
    if (visitor->visit(this)) {
        for (unsigned i = 0; i < memberCount(); ++i) {
            visitSymbol(memberAt(i), visitor);
        }
    }
}

Enum::Enum(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name)
    : ScopedSymbol(translationUnit, sourceLocation, name)
{ }

Enum::~Enum()
{ }

FullySpecifiedType Enum::type() const
{ return FullySpecifiedType(const_cast<Enum *>(this)); }

bool Enum::isEqualTo(const Type *other) const
{
    const Enum *o = other->asEnumType();
    if (! o)
        return false;
    const Name *l = identity();
    const Name *r = o->identity();
    if (l == r)
        return true;
    else if (! l)
        return false;
    return l->isEqualTo(r);
}

void Enum::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool Enum::matchType0(const Type *otherType, TypeMatcher *matcher) const
{
    if (const Enum *otherTy = otherType->asEnumType())
        return matcher->match(this, otherTy);

    return false;
}

void Enum::visitSymbol0(SymbolVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (unsigned i = 0; i < memberCount(); ++i) {
            visitSymbol(memberAt(i), visitor);
        }
    }
}

Namespace::Namespace(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name)
    : ScopedSymbol(translationUnit, sourceLocation, name)
{ }

Namespace::~Namespace()
{ }

bool Namespace::isEqualTo(const Type *other) const
{
    const Namespace *o = other->asNamespaceType();
    if (! o)
        return false;
    const Name *l = identity();
    const Name *r = o->identity();
    if (l == r || (l && l->isEqualTo(r)))
        return true;
    return false;
}

void Namespace::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool Namespace::matchType0(const Type *otherType, TypeMatcher *matcher) const
{
    if (const Namespace *otherTy = otherType->asNamespaceType())
        return matcher->match(this, otherTy);

    return false;
}

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

BaseClass::BaseClass(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name)
    : Symbol(translationUnit, sourceLocation, name),
      _isVirtual(false)
{ }

BaseClass::~BaseClass()
{ }

FullySpecifiedType BaseClass::type() const
{ return _type; }

void BaseClass::setType(const FullySpecifiedType &type)
{ _type = type; }

bool BaseClass::isVirtual() const
{ return _isVirtual; }

void BaseClass::setVirtual(bool isVirtual)
{ _isVirtual = isVirtual; }

void BaseClass::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }

ForwardClassDeclaration::ForwardClassDeclaration(TranslationUnit *translationUnit,
                                                 unsigned sourceLocation, const Name *name)
    : Symbol(translationUnit, sourceLocation, name),
      _templateParameters(0)
{ }

ForwardClassDeclaration::~ForwardClassDeclaration()
{ delete _templateParameters; }

TemplateParameters *ForwardClassDeclaration::templateParameters() const
{ return _templateParameters; }

void ForwardClassDeclaration::setTemplateParameters(TemplateParameters *templateParameters)
{ _templateParameters = templateParameters; }

FullySpecifiedType ForwardClassDeclaration::type() const
{ return FullySpecifiedType(const_cast<ForwardClassDeclaration *>(this)); }

bool ForwardClassDeclaration::isEqualTo(const Type *other) const
{
    if (const ForwardClassDeclaration *otherClassFwdTy = other->asForwardClassDeclarationType()) {
        if (name() == otherClassFwdTy->name())
            return true;
        else if (name() && otherClassFwdTy->name())
            return name()->isEqualTo(otherClassFwdTy->name());

        return false;
    }
    return false;
}

void ForwardClassDeclaration::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }

void ForwardClassDeclaration::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool ForwardClassDeclaration::matchType0(const Type *otherType, TypeMatcher *matcher) const
{
    if (const ForwardClassDeclaration *otherTy = otherType->asForwardClassDeclarationType())
        return matcher->match(this, otherTy);

    return false;
}

Class::Class(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name)
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

    return _templateParameters->scope()->symbolCount();
}

Symbol *Class::templateParameterAt(unsigned index) const
{ return _templateParameters->scope()->symbolAt(index); }

TemplateParameters *Class::templateParameters() const
{ return _templateParameters; }

void Class::setTemplateParameters(TemplateParameters *templateParameters)
{ _templateParameters = templateParameters; }

void Class::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool Class::matchType0(const Type *otherType, TypeMatcher *matcher) const
{
    if (const Class *otherTy = otherType->asClassType())
        return matcher->match(this, otherTy);

    return false;
}

unsigned Class::baseClassCount() const
{ return _baseClasses.size(); }

BaseClass *Class::baseClassAt(unsigned index) const
{ return _baseClasses.at(index); }

void Class::addBaseClass(BaseClass *baseClass)
{ _baseClasses.push_back(baseClass); }

FullySpecifiedType Class::type() const
{ return FullySpecifiedType(const_cast<Class *>(this)); }

bool Class::isEqualTo(const Type *other) const
{
    const Class *o = other->asClassType();
    if (! o)
        return false;
    const Name *l = identity();
    const Name *r = o->identity();
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

ObjCBaseClass::ObjCBaseClass(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name)
    : Symbol(translationUnit, sourceLocation, name)
{ }

ObjCBaseClass::~ObjCBaseClass()
{ }

FullySpecifiedType ObjCBaseClass::type() const
{ return FullySpecifiedType(); }

void ObjCBaseClass::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }

ObjCBaseProtocol::ObjCBaseProtocol(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name)
    : Symbol(translationUnit, sourceLocation, name)
{ }

ObjCBaseProtocol::~ObjCBaseProtocol()
{ }

FullySpecifiedType ObjCBaseProtocol::type() const
{ return FullySpecifiedType(); }

void ObjCBaseProtocol::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }

ObjCClass::ObjCClass(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name):
        ScopedSymbol(translationUnit, sourceLocation, name),
        _isInterface(false),
        _categoryName(0),
        _baseClass(0)
{
}

ObjCClass::~ObjCClass()
{}

FullySpecifiedType ObjCClass::type() const
{ return FullySpecifiedType(const_cast<ObjCClass *>(this)); }

bool ObjCClass::isEqualTo(const Type *other) const
{
    const ObjCClass *o = other->asObjCClassType();
    if (!o)
        return false;

    const Name *l = identity();
    const Name *r = o->identity();
    if (l == r || (l && l->isEqualTo(r)))
        return true;
    else
        return false;
}

void ObjCClass::visitSymbol0(SymbolVisitor *visitor)
{
    if (visitor->visit(this)) {
        if (_baseClass)
            visitSymbol(_baseClass, visitor);

        for (unsigned i = 0; i < _protocols.size(); ++i)
            visitSymbol(_protocols.at(i), visitor);

        for (unsigned i = 0; i < memberCount(); ++i)
            visitSymbol(memberAt(i), visitor);
    }
}

void ObjCClass::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool ObjCClass::matchType0(const Type *otherType, TypeMatcher *matcher) const
{
    if (const ObjCClass *otherTy = otherType->asObjCClassType())
        return matcher->match(this, otherTy);

    return false;
}

ObjCProtocol::ObjCProtocol(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name):
        ScopedSymbol(translationUnit, sourceLocation, name)
{
}

ObjCProtocol::~ObjCProtocol()
{}

FullySpecifiedType ObjCProtocol::type() const
{ return FullySpecifiedType(const_cast<ObjCProtocol *>(this)); }

bool ObjCProtocol::isEqualTo(const Type *other) const
{
    const ObjCProtocol *o = other->asObjCProtocolType();
    if (!o)
        return false;

    const Name *l = identity();
    const Name *r = o->identity();
    if (l == r || (l && l->isEqualTo(r)))
        return true;
    else
        return false;
}

void ObjCProtocol::visitSymbol0(SymbolVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (unsigned i = 0; i < _protocols.size(); ++i)
            visitSymbol(_protocols.at(i), visitor);
    }
}

void ObjCProtocol::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool ObjCProtocol::matchType0(const Type *otherType, TypeMatcher *matcher) const
{
    if (const ObjCProtocol *otherTy = otherType->asObjCProtocolType())
        return matcher->match(this, otherTy);

    return false;
}

ObjCForwardClassDeclaration::ObjCForwardClassDeclaration(TranslationUnit *translationUnit, unsigned sourceLocation,
                                                         const Name *name):
        Symbol(translationUnit, sourceLocation, name)
{
}

ObjCForwardClassDeclaration::~ObjCForwardClassDeclaration()
{}

FullySpecifiedType ObjCForwardClassDeclaration::type() const
{ return FullySpecifiedType(); }

bool ObjCForwardClassDeclaration::isEqualTo(const Type *other) const
{
    if (const ObjCForwardClassDeclaration *otherFwdClass = other->asObjCForwardClassDeclarationType()) {
        if (name() == otherFwdClass->name())
            return true;
        else if (name() && otherFwdClass->name())
            return name()->isEqualTo(otherFwdClass->name());
        else
            return false;
    }

    return false;
}

void ObjCForwardClassDeclaration::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }

void ObjCForwardClassDeclaration::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool ObjCForwardClassDeclaration::matchType0(const Type *otherType, TypeMatcher *matcher) const
{
    if (const ObjCForwardClassDeclaration *otherTy = otherType->asObjCForwardClassDeclarationType())
        return matcher->match(this, otherTy);

    return false;
}

ObjCForwardProtocolDeclaration::ObjCForwardProtocolDeclaration(TranslationUnit *translationUnit, unsigned sourceLocation,
                                                               const Name *name):
        Symbol(translationUnit, sourceLocation, name)
{
}

ObjCForwardProtocolDeclaration::~ObjCForwardProtocolDeclaration()
{}

FullySpecifiedType ObjCForwardProtocolDeclaration::type() const
{ return FullySpecifiedType(); }

bool ObjCForwardProtocolDeclaration::isEqualTo(const Type *other) const
{
    if (const ObjCForwardProtocolDeclaration *otherFwdProtocol = other->asObjCForwardProtocolDeclarationType()) {
        if (name() == otherFwdProtocol->name())
            return true;
        else if (name() && otherFwdProtocol->name())
            return name()->isEqualTo(otherFwdProtocol->name());
        else
            return false;
    }

    return false;
}

void ObjCForwardProtocolDeclaration::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }

void ObjCForwardProtocolDeclaration::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool ObjCForwardProtocolDeclaration::matchType0(const Type *otherType, TypeMatcher *matcher) const
{
    if (const ObjCForwardProtocolDeclaration *otherTy = otherType->asObjCForwardProtocolDeclarationType())
        return matcher->match(this, otherTy);

    return false;
}

ObjCMethod::ObjCMethod(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name)
    : ScopedSymbol(translationUnit, sourceLocation, name),
     _flags(0)
{ _arguments = new Scope(this); }

ObjCMethod::~ObjCMethod()
{
    delete _arguments;
}

bool ObjCMethod::isEqualTo(const Type *other) const
{
    const ObjCMethod *o = other->asObjCMethodType();
    if (! o)
        return false;

    const Name *l = identity();
    const Name *r = o->identity();
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

void ObjCMethod::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool ObjCMethod::matchType0(const Type *otherType, TypeMatcher *matcher) const
{
    if (const ObjCMethod *otherTy = otherType->asObjCMethodType())
        return matcher->match(this, otherTy);

    return false;
}

FullySpecifiedType ObjCMethod::type() const
{ return FullySpecifiedType(const_cast<ObjCMethod *>(this)); }

FullySpecifiedType ObjCMethod::returnType() const
{ return _returnType; }

void ObjCMethod::setReturnType(const FullySpecifiedType &returnType)
{ _returnType = returnType; }

bool ObjCMethod::hasReturnType() const
{
    const FullySpecifiedType ty = returnType();
    return ty.isValid() || ty.isSigned() || ty.isUnsigned();
}

unsigned ObjCMethod::argumentCount() const
{
    if (! _arguments)
        return 0;

    return _arguments->symbolCount();
}

Symbol *ObjCMethod::argumentAt(unsigned index) const
{ return _arguments->symbolAt(index); }

Scope *ObjCMethod::arguments() const
{ return _arguments; }

bool ObjCMethod::hasArguments() const
{
    return ! (argumentCount() == 0 ||
              (argumentCount() == 1 && argumentAt(0)->type()->isVoidType()));
}

bool ObjCMethod::isVariadic() const
{ return f._isVariadic; }

void ObjCMethod::setVariadic(bool isVariadic)
{ f._isVariadic = isVariadic; }

void ObjCMethod::visitSymbol0(SymbolVisitor *visitor)
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

ObjCPropertyDeclaration::ObjCPropertyDeclaration(TranslationUnit *translationUnit,
                                                 unsigned sourceLocation,
                                                 const Name *name):
    Symbol(translationUnit, sourceLocation, name),
    _propertyAttributes(None),
    _getterName(0),
    _setterName(0)
{}

ObjCPropertyDeclaration::~ObjCPropertyDeclaration()
{}

FullySpecifiedType ObjCPropertyDeclaration::type() const
{ return _type; }

void ObjCPropertyDeclaration::visitSymbol0(SymbolVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
}
