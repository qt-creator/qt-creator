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

#ifndef CPLUSPLUS_SCOPE_H
#define CPLUSPLUS_SCOPE_H

#include "CPlusPlusForwardDeclarations.h"

CPLUSPLUS_BEGIN_HEADER
CPLUSPLUS_BEGIN_NAMESPACE

class CPLUSPLUS_EXPORT Use
{
public:
    inline Name *name() const
    { return _name; }

    inline unsigned sourceOffset() const
    { return _sourceOffset; }

    inline Symbol *lastVisibleSymbol() const
    { return _lastVisibleSymbol; }

private:
    void init(unsigned sourceOffset, Name *name, Symbol *lastVisibleSymbol)
    {
        _sourceOffset = sourceOffset;
        _name = name;
        _lastVisibleSymbol = lastVisibleSymbol;
    }

    unsigned _sourceOffset;

    Name *_name;
    Symbol *_lastVisibleSymbol;

    friend class Scope;
};

class CPLUSPLUS_EXPORT Scope
{
    Scope(const Scope &other);
    void operator =(const Scope &other);

public:
    typedef Symbol **iterator;

public:
    /// Constructs an empty Scope.
    Scope(ScopedSymbol *owner = 0);

    /// Destroy this scope.
    ~Scope();

    /// Returns this scope's owner Symbol.
    ScopedSymbol *owner() const;

    /// Sets this scope's owner Symbol.
    void setOwner(ScopedSymbol *owner); // ### remove me

    /// Returns the enclosing scope.
    Scope *enclosingScope() const;

    /// Returns the eclosing namespace scope.
    Scope *enclosingNamespaceScope() const;

    /// Returns the enclosing class scope.
    Scope *enclosingClassScope() const;

    /// Returns the enclosing enum scope.
    Scope *enclosingEnumScope() const;

    /// Rerturns the enclosing function scope.
    Scope *enclosingFunctionScope() const;

    /// Rerturns the enclosing Block scope.
    Scope *enclosingBlockScope() const;

    /// Returns true if this scope's owner is a Namespace Symbol.
    bool isNamespaceScope() const;

    /// Returns true if this scope's owner is a Class Symbol.
    bool isClassScope() const;

    /// Returns true if this scope's owner is an Enum Symbol.
    bool isEnumScope() const;

    /// Returns true if this scope's owner is a Block Symbol.
    bool isBlockScope() const;

    /// Returns true if this scope's owner is a Function Symbol.
    bool isFunctionScope() const;

    /// Returns true if this scope's owner is a Prototype Symbol.
    bool isPrototypeScope() const;

    /// Adds a Symbol to this Scope.
    void enterSymbol(Symbol *symbol);

    /// Returns true if this Scope is empty; otherwise returns false.
    bool isEmpty() const;

    /// Returns the number of symbols is in the scope.
    unsigned symbolCount() const;

    /// Returns the Symbol at the given position.
    Symbol *symbolAt(unsigned index) const;

    /// Returns the first Symbol in the scope.
    iterator firstSymbol() const;

    /// Returns the last Symbol in the scope.
    iterator lastSymbol() const;

    Symbol *lookat(Identifier *id) const;
    Symbol *lookat(int operatorId) const;

    unsigned useCount() const;
    Use *useAt(unsigned index) const;
    void addUse(unsigned sourceOffset, Name *name);

private:
    /// Returns the hash value for the given Symbol.
    unsigned hashValue(Symbol *symbol) const;

    /// Updates the hash table.
    void rehash();

private:
    enum { DefaultInitialSize = 11 };

    ScopedSymbol *_owner;

    Symbol **_symbols;
    int _allocatedSymbols;
    int _symbolCount;

    Symbol **_hash;
    int _hashSize;

    Use *_uses;
    int _allocatedUses;
    int _useCount;
};

CPLUSPLUS_END_NAMESPACE
CPLUSPLUS_END_HEADER

#endif // CPLUSPLUS_SCOPE_H
