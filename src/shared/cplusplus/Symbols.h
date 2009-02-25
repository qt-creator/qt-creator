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

#ifndef CPLUSPLUS_SYMBOLS_H
#define CPLUSPLUS_SYMBOLS_H

#include "CPlusPlusForwardDeclarations.h"
#include "Symbol.h"
#include "Type.h"
#include "FullySpecifiedType.h"
#include "Array.h"

CPLUSPLUS_BEGIN_HEADER
CPLUSPLUS_BEGIN_NAMESPACE

class CPLUSPLUS_EXPORT UsingNamespaceDirective: public Symbol
{
public:
    UsingNamespaceDirective(TranslationUnit *translationUnit, unsigned sourceLocation, Name *name);
    virtual ~UsingNamespaceDirective();

    // Symbol's interface
    virtual FullySpecifiedType type() const;

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);
};

class CPLUSPLUS_EXPORT UsingDeclaration: public Symbol
{
public:
    UsingDeclaration(TranslationUnit *translationUnit, unsigned sourceLocation, Name *name);
    virtual ~UsingDeclaration();

    // Symbol's interface
    virtual FullySpecifiedType type() const;

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);
};

class CPLUSPLUS_EXPORT Declaration: public Symbol
{
public:
    Declaration(TranslationUnit *translationUnit, unsigned sourceLocation, Name *name);
    virtual ~Declaration();

    unsigned templateParameterCount() const;
    Symbol *templateParameterAt(unsigned index) const;

    Scope *templateParameters() const;
    void setTemplateParameters(Scope *templateParameters);

    void setType(FullySpecifiedType type);

    // Symbol's interface
    virtual FullySpecifiedType type() const;

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);

private:
    FullySpecifiedType _type;
    Scope *_templateParameters;
};

class CPLUSPLUS_EXPORT Argument: public Symbol
{
public:
    Argument(TranslationUnit *translationUnit, unsigned sourceLocation, Name *name);
    virtual ~Argument();

    void setType(FullySpecifiedType type);

    bool hasInitializer() const;
    void setInitializer(bool hasInitializer);

    // Symbol's interface
    virtual FullySpecifiedType type() const;

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);

private:
    FullySpecifiedType _type;
    bool _initializer: 1;
};

class CPLUSPLUS_EXPORT ScopedSymbol: public Symbol
{
public:
    ScopedSymbol(TranslationUnit *translationUnit, unsigned sourceLocation, Name *name);
    virtual ~ScopedSymbol();

    unsigned memberCount() const;
    Symbol *memberAt(unsigned index) const;
    Scope *members() const;
    void addMember(Symbol *member);

private:
    Scope *_members;
};

class CPLUSPLUS_EXPORT Block: public ScopedSymbol
{
public:
    Block(TranslationUnit *translationUnit, unsigned sourceLocation);
    virtual ~Block();

    // Symbol's interface
    virtual FullySpecifiedType type() const;

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);
};

class CPLUSPLUS_EXPORT Enum: public ScopedSymbol, public Type
{
public:
    Enum(TranslationUnit *translationUnit, unsigned sourceLocation, Name *name);
    virtual ~Enum();

    // Symbol's interface
    virtual FullySpecifiedType type() const;

    // Type's interface
    virtual bool isEqualTo(const Type *other) const;

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);
    virtual void accept0(TypeVisitor *visitor);
};

class CPLUSPLUS_EXPORT Function: public ScopedSymbol, public Type
{
public:
    enum MethodKey {
        NormalMethod,
        SlotMethod,
        SignalMethod
    };

public:
    Function(TranslationUnit *translationUnit, unsigned sourceLocation, Name *name);
    virtual ~Function();

    bool isNormal() const;
    bool isSignal() const;
    bool isSlot() const;
    int methodKey() const;
    void setMethodKey(int key);

    unsigned templateParameterCount() const;
    Symbol *templateParameterAt(unsigned index) const;

    Scope *templateParameters() const;
    void setTemplateParameters(Scope *templateParameters);

    FullySpecifiedType returnType() const;
    void setReturnType(FullySpecifiedType returnType);

    unsigned argumentCount() const;
    Symbol *argumentAt(unsigned index) const;
    Scope *arguments() const;

    bool isVariadic() const;
    void setVariadic(bool isVariadic);

    bool isConst() const;
    void setConst(bool isConst);

    bool isVolatile() const;
    void setVolatile(bool isVolatile);

    bool isPureVirtual() const;
    void setPureVirtual(bool isPureVirtual);

    // Symbol's interface
    virtual FullySpecifiedType type() const;

    // Type's interface
    virtual bool isEqualTo(const Type *other) const;

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);
    virtual void accept0(TypeVisitor *visitor);

private:
    Name *_name;
    Scope *_templateParameters;
    FullySpecifiedType _returnType;
    union {
        unsigned _flags;

        struct {
            unsigned _isVariadic: 1;
            unsigned _isPureVirtual: 1;
            unsigned _isConst: 1;
            unsigned _isVolatile: 1;
            unsigned _methodKey: 3;
        };
    };
    Scope *_arguments;
};

class CPLUSPLUS_EXPORT Namespace: public ScopedSymbol, public Type
{
public:
    Namespace(TranslationUnit *translationUnit, unsigned sourceLocation, Name *name);
    virtual ~Namespace();

    // Symbol's interface
    virtual FullySpecifiedType type() const;

    // Type's interface
    virtual bool isEqualTo(const Type *other) const;

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);
    virtual void accept0(TypeVisitor *visitor);
};

class CPLUSPLUS_EXPORT BaseClass: public Symbol
{
public:
    BaseClass(TranslationUnit *translationUnit, unsigned sourceLocation, Name *name);
    virtual ~BaseClass();

    bool isVirtual() const;
    void setVirtual(bool isVirtual);

    // Symbol's interface
    virtual FullySpecifiedType type() const;

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);

private:
    bool _isVirtual;
};

class CPLUSPLUS_EXPORT Class: public ScopedSymbol, public Type
{
public:
    Class(TranslationUnit *translationUnit, unsigned sourceLocation, Name *name);
    virtual ~Class();

    enum Key {
        ClassKey,
        StructKey,
        UnionKey
    };

    bool isClass() const;
    bool isStruct() const;
    bool isUnion() const;
    Key classKey() const;
    void setClassKey(Key key);

    unsigned templateParameterCount() const;
    Symbol *templateParameterAt(unsigned index) const;

    Scope *templateParameters() const;
    void setTemplateParameters(Scope *templateParameters);

    unsigned baseClassCount() const;
    BaseClass *baseClassAt(unsigned index) const;
    void addBaseClass(BaseClass *baseClass);

    // Symbol's interface
    virtual FullySpecifiedType type() const;

    // Type's interface
    virtual bool isEqualTo(const Type *other) const;

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);
    virtual void accept0(TypeVisitor *visitor);

private:
    Key _key;
    Scope *_templateParameters;
    Array<BaseClass *> _baseClasses;
};

CPLUSPLUS_END_NAMESPACE
CPLUSPLUS_END_HEADER

#endif // CPLUSPLUS_SYMBOLS_H
