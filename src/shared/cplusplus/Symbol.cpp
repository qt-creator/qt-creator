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

#include "Symbol.h"
#include "Symbols.h"
#include "Control.h"
#include "Names.h"
#include "TranslationUnit.h"
#include "Literals.h"
#include "MemoryPool.h"
#include "SymbolVisitor.h"
#include "NameVisitor.h"
#include "Scope.h"
#include <cstddef>
#include <cassert>

using namespace CPlusPlus;

class Symbol::HashCode: protected NameVisitor
{
public:
    HashCode()
        : _value(0)
    { }

    virtual ~HashCode()
    { }

    unsigned operator()(Name *name)
    {
        unsigned previousValue = switchValue(0);
        accept(name);
        return switchValue(previousValue);
    }

protected:
    unsigned switchValue(unsigned value)
    {
        unsigned previousValue = _value;
        _value = value;
        return previousValue;
    }

    virtual void visit(NameId *name)
    { _value = name->identifier()->hashCode(); }

    virtual void visit(TemplateNameId *name)
    { _value = name->identifier()->hashCode(); }

    virtual void visit(DestructorNameId *name)
    { _value = name->identifier()->hashCode(); }

    virtual void visit(OperatorNameId *name)
    { _value = unsigned(name->kind()); }

    virtual void visit(ConversionNameId *)
    { _value = 0; } // ### TODO: implement me

    virtual void visit(QualifiedNameId *name)
    { _value = operator()(name->unqualifiedNameId()); }

    virtual void visit(SelectorNameId *name)
    { _value = name->identifier()->hashCode(); }

private:
    unsigned _value;
};

class Symbol::IdentityForName: protected NameVisitor
{
public:
    IdentityForName()
        : _identity(0)
    { }

    virtual ~IdentityForName()
    { }

    Name *operator()(Name *name)
    {
        Name *previousIdentity = switchIdentity(0);
        accept(name);
        return switchIdentity(previousIdentity);
    }

protected:
    Name *switchIdentity(Name *identity)
    {
        Name *previousIdentity = _identity;
        _identity = identity;
        return previousIdentity;
    }

    virtual void visit(NameId *name)
    { _identity = name; }

    virtual void visit(TemplateNameId *name)
    { _identity = name; }

    virtual void visit(DestructorNameId *name)
    { _identity = name; }

    virtual void visit(OperatorNameId *name)
    { _identity = name; }

    virtual void visit(ConversionNameId *name)
    { _identity = name; }

    virtual void visit(QualifiedNameId *name)
    { _identity = name->unqualifiedNameId(); }

    virtual void visit(SelectorNameId *name)
    { _identity = name; }

private:
    Name *_identity;
};

Symbol::Symbol(TranslationUnit *translationUnit, unsigned sourceLocation, Name *name)
    : _control(translationUnit->control()),
      _sourceLocation(sourceLocation),
      _sourceOffset(0),
      _startOffset(0),
      _endOffset(0),
      _name(0),
      _hashCode(0),
      _storage(Symbol::NoStorage),
      _visibility(Symbol::Public),
      _scope(0),
      _index(0),
      _next(0),
      _isGenerated(false)
{
    setSourceLocation(sourceLocation);
    setName(name);
}

Symbol::~Symbol()
{ }

Control *Symbol::control() const
{ return _control; }

TranslationUnit *Symbol::translationUnit() const
{ return _control->translationUnit(); }

void Symbol::visitSymbol(SymbolVisitor *visitor)
{
    if (visitor->preVisit(this))
        visitSymbol0(visitor);
    visitor->postVisit(this);
}

void Symbol::visitSymbol(Symbol *symbol, SymbolVisitor *visitor)
{
    if (! symbol)
        return;

    symbol->visitSymbol(visitor);
}

unsigned Symbol::sourceLocation() const
{ return _sourceLocation; }

unsigned Symbol::sourceOffset() const
{ return _sourceOffset; }

bool Symbol::isGenerated() const
{ return _isGenerated; }

void Symbol::setSourceLocation(unsigned sourceLocation)
{
    _sourceLocation = sourceLocation;

    if (! _sourceLocation) {
        _isGenerated = false;
        _sourceOffset = 0;
    } else {
        TranslationUnit *unit = translationUnit();

        const Token &tk = unit->tokenAt(sourceLocation);

        _isGenerated = tk.f.generated;
        _sourceOffset = tk.offset;
    }
}

unsigned Symbol::line() const
{
    unsigned line = 0, column = 0;
    StringLiteral *fileId = 0;
    translationUnit()->getPosition(_sourceOffset, &line, &column, &fileId);
    return line;
}

unsigned Symbol::column() const
{
    unsigned line = 0, column = 0;
    StringLiteral *fileId = 0;
    translationUnit()->getPosition(_sourceOffset, &line, &column, &fileId);
    return column;
}

StringLiteral *Symbol::fileId() const
{
    unsigned line = 0, column = 0;
    StringLiteral *fileId = 0;
    translationUnit()->getPosition(_sourceOffset, &line, &column, &fileId);
    return fileId;
}

void Symbol::getPosition(unsigned *line, unsigned *column, StringLiteral **fileId) const
{ translationUnit()->getPosition(_sourceOffset, line, column, fileId); }

void Symbol::getStartPosition(unsigned *line, unsigned *column, StringLiteral **fileId) const
{ translationUnit()->getPosition(_startOffset, line, column, fileId); }

void Symbol::getEndPosition(unsigned *line, unsigned *column, StringLiteral **fileId) const
{ translationUnit()->getPosition(_endOffset, line, column, fileId); }

const char *Symbol::fileName() const
{ return fileId()->chars(); }

unsigned Symbol::fileNameLength() const
{ return fileId()->size(); }

unsigned Symbol::startOffset() const
{ return _startOffset; }

void Symbol::setStartOffset(unsigned offset)
{ _startOffset = offset; }

unsigned Symbol::endOffset() const
{ return _endOffset; }

void Symbol::setEndOffset(unsigned offset)
{ _endOffset = offset; }

Name *Symbol::identity() const
{
    IdentityForName id;
    return id(_name);
}

Name *Symbol::name() const
{ return _name; }

void Symbol::setName(Name *name)
{
    _name = name;

    if (! _name)
        _hashCode = 0;
    else {
        IdentityForName identityForName;
        HashCode hh;
        _hashCode = hh(identityForName(_name));
    }
}

Identifier *Symbol::identifier() const
{
    if (_name)
        return _name->identifier();

    return 0;
}

Scope *Symbol::scope() const
{ return _scope; }

void Symbol::setScope(Scope *scope)
{
    assert(! _scope);
    _scope = scope;
}

Symbol *Symbol::enclosingSymbol() const
{
    if (! _scope)
        return 0;

    return _scope->owner();
}

Scope *Symbol::enclosingNamespaceScope() const
{
    if (! _scope)
        return 0;

    else if (_scope->isNamespaceScope())
        return _scope;

    return _scope->enclosingNamespaceScope();
}

Scope *Symbol::enclosingClassScope() const
{
    if (! _scope)
        return 0;

    else if (_scope->isClassScope())
        return _scope;

    return _scope->enclosingClassScope();
}

Scope *Symbol::enclosingEnumScope() const
{
    if (! _scope)
        return 0;

    else if (_scope->isEnumScope())
        return _scope;

    return _scope->enclosingEnumScope();
}

Scope *Symbol::enclosingFunctionScope() const
{
    if (! _scope)
        return 0;

    else if (_scope->isFunctionScope())
        return _scope;

    return _scope->enclosingFunctionScope();
}

Scope *Symbol::enclosingBlockScope() const
{
    if (! _scope)
        return 0;

    else if (_scope->isBlockScope())
        return _scope;

    return _scope->enclosingBlockScope();
}

unsigned Symbol::index() const
{ return _index; }

Symbol *Symbol::next() const
{ return _next; }

unsigned Symbol::hashCode() const
{ return _hashCode; }

int Symbol::storage() const
{ return _storage; }

void Symbol::setStorage(int storage)
{ _storage = storage; }

int Symbol::visibility() const
{ return _visibility; }

void Symbol::setVisibility(int visibility)
{ _visibility = visibility; }

bool Symbol::isFriend() const
{ return _storage == Friend; }

bool Symbol::isRegister() const
{ return _storage == Register; }

bool Symbol::isStatic() const
{ return _storage == Static; }

bool Symbol::isExtern() const
{ return _storage == Extern; }

bool Symbol::isMutable() const
{ return _storage == Mutable; }

bool Symbol::isTypedef() const
{ return _storage == Typedef; }

bool Symbol::isPublic() const
{ return _visibility == Public; }

bool Symbol::isProtected() const
{ return _visibility == Protected; }

bool Symbol::isPrivate() const
{ return _visibility == Private; }

bool Symbol::isScopedSymbol() const
{ return asScopedSymbol() != 0; }

bool Symbol::isEnum() const
{ return asEnum()  != 0; }

bool Symbol::isFunction() const
{ return asFunction() != 0; }

bool Symbol::isNamespace() const
{ return asNamespace() != 0; }

bool Symbol::isClass() const
{ return asClass() != 0; }

bool Symbol::isForwardClassDeclaration() const
{ return asForwardClassDeclaration() != 0; }

bool Symbol::isBlock() const
{ return asBlock() != 0; }

bool Symbol::isUsingNamespaceDirective() const
{ return asUsingNamespaceDirective() != 0; }

bool Symbol::isUsingDeclaration() const
{ return asUsingDeclaration() != 0; }

bool Symbol::isDeclaration() const
{ return asDeclaration() != 0; }

bool Symbol::isArgument() const
{ return asArgument() != 0; }

bool Symbol::isBaseClass() const
{ return asBaseClass() != 0; }

bool Symbol::isObjCBaseClass() const
{ return asObjCBaseClass() != 0; }

bool Symbol::isObjCBaseProtocol() const
{ return asObjCBaseProtocol() != 0; }

bool Symbol::isObjCClass() const
{ return asObjCClass() != 0; }

bool Symbol::isObjCForwardClassDeclaration() const
{ return asObjCForwardClassDeclaration() != 0; }

bool Symbol::isObjCProtocol() const
{ return asObjCProtocol() != 0; }

bool Symbol::isObjCForwardProtocolDeclaration() const
{ return asObjCForwardProtocolDeclaration() != 0; }

bool Symbol::isObjCMethod() const
{ return asObjCMethod() != 0; }


