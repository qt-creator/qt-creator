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

#include "Scope.h"
#include "Symbols.h"
#include "Names.h"
#include "Literals.h"
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <iostream>

CPLUSPLUS_BEGIN_NAMESPACE

Scope::Scope(ScopedSymbol *owner)
    : _owner(owner),
      _symbols(0),
      _allocatedSymbols(0),
      _symbolCount(-1),
      _hash(0),
      _hashSize(0),
      _uses(0),
      _allocatedUses(0),
      _useCount(-1)
{ }

Scope::~Scope()
{
    if (_symbols)
        free(_symbols);
    if (_hash)
        free(_hash);
    if (_uses)
        free(_uses);
}

ScopedSymbol *Scope::owner() const
{ return _owner; }

void Scope::setOwner(ScopedSymbol *owner)
{ _owner = owner; }

Scope *Scope::enclosingScope() const
{
    if (! _owner)
        return 0;

    return _owner->scope();
}

Scope *Scope::enclosingNamespaceScope() const
{
    Scope *scope = enclosingScope();
    for (; scope; scope = scope->enclosingScope()) {
        if (scope->owner()->isNamespace())
            break;
    }
    return scope;
}

Scope *Scope::enclosingClassScope() const
{
    Scope *scope = enclosingScope();
    for (; scope; scope = scope->enclosingScope()) {
        if (scope->owner()->isClass())
            break;
    }
    return scope;
}

Scope *Scope::enclosingEnumScope() const
{
    Scope *scope = enclosingScope();
    for (; scope; scope = scope->enclosingScope()) {
        if (scope->owner()->isEnum())
            break;
    }
    return scope;
}

Scope *Scope::enclosingFunctionScope() const
{
    Scope *scope = enclosingScope();
    for (; scope; scope = scope->enclosingScope()) {
        if (scope->owner()->isFunction())
            break;
    }
    return scope;
}

Scope *Scope::enclosingBlockScope() const
{
    Scope *scope = enclosingScope();
    for (; scope; scope = scope->enclosingScope()) {
        if (scope->owner()->isBlock())
            break;
    }
    return scope;
}

bool Scope::isNamespaceScope() const
{ return dynamic_cast<const Namespace *>(_owner) != 0; }

bool Scope::isClassScope() const
{ return dynamic_cast<const Class *>(_owner) != 0; }

bool Scope::isEnumScope() const
{ return dynamic_cast<const Enum *>(_owner) != 0; }

bool Scope::isBlockScope() const
{ return dynamic_cast<const Block *>(_owner) != 0; }

bool Scope::isPrototypeScope() const
{
    if (const Function *f = dynamic_cast<const Function *>(_owner))
        return f->arguments() == this;
    return false;
}

bool Scope::isFunctionScope() const
{
    if (const Function *f = dynamic_cast<const Function *>(_owner))
        return f->arguments() != this;
    return false;
}

void Scope::enterSymbol(Symbol *symbol)
{
    if (++_symbolCount == _allocatedSymbols) {
        _allocatedSymbols <<= 1;
        if (! _allocatedSymbols)
            _allocatedSymbols = DefaultInitialSize;

        _symbols = reinterpret_cast<Symbol **>(realloc(_symbols, sizeof(Symbol *) * _allocatedSymbols));
    }

    assert(! symbol->_scope || symbol->scope() == this);
    symbol->_index = _symbolCount;
    symbol->_scope = this;
    _symbols[_symbolCount] = symbol;

    if (_symbolCount >= _hashSize * 0.6)
        rehash();
    else {
        const unsigned h = hashValue(symbol);
        symbol->_next = _hash[h];
        _hash[h] = symbol;
    }
}

Symbol *Scope::lookat(Identifier *id) const
{
    if (! _hash)
        return 0;

    const unsigned h = id->hashCode() % _hashSize;
    Symbol *symbol = _hash[h];
    for (; symbol; symbol = symbol->_next) {
        Name *identity = symbol->identity();
        if (NameId *nameId = identity->asNameId()) {
            if (nameId->identifier()->isEqualTo(id))
                break;
        } else if (TemplateNameId *t = identity->asTemplateNameId()) {
            if (t->identifier()->isEqualTo(id))
                break;
        } else if (DestructorNameId *d = identity->asDestructorNameId()) {
            if (d->identifier()->isEqualTo(id))
                break;
        } else if (identity->isQualifiedNameId()) {
            assert(0);
        }
    }
    return symbol;
}

Symbol *Scope::lookat(int operatorId) const
{
    if (! _hash)
        return 0;

    const unsigned h = operatorId % _hashSize;
    Symbol *symbol = _hash[h];
    for (; symbol; symbol = symbol->_next) {
        Name *identity = symbol->identity();
        if (OperatorNameId *op = identity->asOperatorNameId()) {
            if (op->kind() == operatorId)
                break;
        }
    }
    return symbol;
}

void Scope::rehash()
{
    _hashSize <<= 1;

    if (! _hashSize)
        _hashSize = DefaultInitialSize;

    _hash = reinterpret_cast<Symbol **>(realloc(_hash, sizeof(Symbol *) * _hashSize));
    memset(_hash, 0, sizeof(Symbol *) * _hashSize);

    for (int index = 0; index < _symbolCount + 1; ++index) {
        Symbol *symbol = _symbols[index];
        const unsigned h = hashValue(symbol);
        symbol->_next = _hash[h];
        _hash[h] = symbol;
    }
}

unsigned Scope::hashValue(Symbol *symbol) const
{
    if (! symbol)
        return 0;

    return symbol->hashCode() % _hashSize;
}

bool Scope::isEmpty() const
{ return _symbolCount == -1; }

unsigned Scope::symbolCount() const
{ return _symbolCount + 1; }

Symbol *Scope::symbolAt(unsigned index) const
{
    if (! _symbols)
        return 0;
    return _symbols[index];
}

Scope::iterator Scope::firstSymbol() const
{ return _symbols; }

Scope::iterator Scope::lastSymbol() const
{ return _symbols + _symbolCount + 1; }

unsigned Scope::useCount() const
{ return _useCount + 1; }

Use *Scope::useAt(unsigned index) const
{ return &_uses[index]; }

void Scope::addUse(unsigned sourceOffset, Name *name)
{
#ifdef CPLUSPLUS_WITH_USES
    if (++_useCount == _allocatedUses) {
        _allocatedUses += 4;
        _uses = reinterpret_cast<Use *>(realloc(_uses, _allocatedUses * sizeof(Use)));
    }

    Symbol *lastVisibleSymbol;
    if (_symbolCount == -1)
        lastVisibleSymbol = owner();
    else
        lastVisibleSymbol = _symbols[_symbolCount];
    _uses[_useCount].init(sourceOffset, name, lastVisibleSymbol);
#endif
}

CPLUSPLUS_END_NAMESPACE
