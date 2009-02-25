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

#ifndef CPLUSPLUS_SYMBOL_H
#define CPLUSPLUS_SYMBOL_H

#include "CPlusPlusForwardDeclarations.h"

CPLUSPLUS_BEGIN_HEADER
CPLUSPLUS_BEGIN_NAMESPACE

class CPLUSPLUS_EXPORT Symbol
{
    Symbol(const Symbol &other);
    void operator =(const Symbol &other);

public:
    /// Storage class specifier
    enum Storage {
        NoStorage = 0,
        Friend,
        Register,
        Static,
        Extern,
        Mutable,
        Typedef
    };

    /// Access specifier.
    enum Visibility {
        Public,
        Protected,
        Private
    };

public:
    /// Constructs a Symbol with the given source location, name and translation unit.
    Symbol(TranslationUnit *translationUnit, unsigned sourceLocation, Name *name);

    /// Destroy this Symbol.
    virtual ~Symbol();

    /// Returns this Symbol's Control object.
    Control *control() const;

    /// Returns this Symbol's source location.
    unsigned sourceLocation() const;

    /// Returns this Symbol's source offset.
    unsigned sourceOffset() const;

    /// Returns this Symbol's line number.
    unsigned line() const;

    /// Returns this Symbol's column number.
    unsigned column() const;

    /// Returns this Symbol's file name.
    StringLiteral *fileId() const;

    /// Returns this Symbol's file name.
    const char *fileName() const;

    /// Returns this Symbol's file name length.
    unsigned fileNameLength() const;

    /// Returns this Symbol's name.
    Name *name() const;

    /// Sets this Symbol's name.
    void setName(Name *name); // ### dangerous

    /// Returns this Symbol's storage class specifier.
    int storage() const;

    /// Sets this Symbol's storage class specifier.
    void setStorage(int storage);

    /// Returns this Symbol's visibility.
    int visibility() const;

    /// Sets this Symbol's visibility.
    void setVisibility(int visibility);

    /// Returns this Symbol's scope.
    Scope *scope() const;

    /// Returns the next chained Symbol.
    Symbol *next() const;

    /// Returns true if this Symbol has friend storage specifier.
    bool isFriend() const;

    /// Returns true if this Symbol has register storage specifier.
    bool isRegister() const;

    /// Returns true if this Symbol has static storage specifier.
    bool isStatic() const;

    /// Returns true if this Symbol has extern storage specifier.
    bool isExtern() const;

    /// Returns true if this Symbol has mutable storage specifier.
    bool isMutable() const;

    /// Returns true if this Symbol has typedef storage specifier.
    bool isTypedef() const;

    /// Returns true if this Symbol's visibility is public.
    bool isPublic() const;

    /// Returns true if this Symbol's visibility is protected.
    bool isProtected() const;

    /// Returns true if this Symbol's visibility is private.
    bool isPrivate() const;

    /// Returns true if this Symbol is a ScopedSymbol.
    bool isScopedSymbol() const;

    /// Returns true if this Symbol is an Enum.
    bool isEnum() const;

    /// Returns true if this Symbol is an Function.
    bool isFunction() const;

    /// Returns true if this Symbol is a Namespace.
    bool isNamespace() const;

    /// Returns true if this Symbol is a Class.
    bool isClass() const;

    /// Returns true if this Symbol is a Block.
    bool isBlock() const;

    /// Returns true if this Symbol is a UsingNamespaceDirective.
    bool isUsingNamespaceDirective() const;

    /// Returns true if this Symbol is a UsingDeclaration.
    bool isUsingDeclaration() const;

    /// Returns true if this Symbol is a Declaration.
    bool isDeclaration() const;

    /// Returns true if this Symbol is an Argument.
    bool isArgument() const;

    /// Returns true if this Symbol is a BaseClass.
    bool isBaseClass() const;

    const ScopedSymbol *asScopedSymbol() const;
    const Enum *asEnum() const;
    const Function *asFunction() const;
    const Namespace *asNamespace() const;
    const Class *asClass() const;
    const Block *asBlock() const;
    const UsingNamespaceDirective *asUsingNamespaceDirective() const;
    const UsingDeclaration *asUsingDeclaration() const;
    const Declaration *asDeclaration() const;
    const Argument *asArgument() const;
    const BaseClass *asBaseClass() const;

    ScopedSymbol *asScopedSymbol();
    Enum *asEnum();
    Function *asFunction();
    Namespace *asNamespace();
    Class *asClass();
    Block *asBlock();
    UsingNamespaceDirective *asUsingNamespaceDirective();
    UsingDeclaration *asUsingDeclaration();
    Declaration *asDeclaration();
    Argument *asArgument();
    BaseClass *asBaseClass();

    /// Returns this Symbol's type.
    virtual FullySpecifiedType type() const = 0;

    /// Returns this Symbol's hash value.
    unsigned hashCode() const;

    /// Returns this Symbol's index.
    unsigned index() const;

    Name *identity() const;

    void setScope(Scope *scope); // ### make me private

    void visitSymbol(SymbolVisitor *visitor);
    static void visitSymbol(Symbol *symbol, SymbolVisitor *visitor);

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor) = 0;

    TranslationUnit *translationUnit() const;

private:
    Control *_control;
    unsigned _sourceLocation;
    unsigned _sourceOffset;
    Name *_name;
    unsigned _hashCode;
    int _storage;
    int _visibility;
    Scope *_scope;
    unsigned _index;
    Symbol *_next;

    class IdentityForName;
    class HashCode;

    friend class Scope;
};

CPLUSPLUS_END_NAMESPACE
CPLUSPLUS_END_HEADER

#endif // CPLUSPLUS_SYMBOL_H
