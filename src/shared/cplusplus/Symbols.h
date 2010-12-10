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

#ifndef CPLUSPLUS_SYMBOLS_H
#define CPLUSPLUS_SYMBOLS_H

#include "CPlusPlusForwardDeclarations.h"
#include "Symbol.h"
#include "Type.h"
#include "FullySpecifiedType.h"
#include "Scope.h"
#include <vector>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT UsingNamespaceDirective: public Symbol
{
public:
    UsingNamespaceDirective(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name);
    virtual ~UsingNamespaceDirective();

    // Symbol's interface
    virtual FullySpecifiedType type() const;

    virtual const UsingNamespaceDirective *asUsingNamespaceDirective() const
    { return this; }

    virtual UsingNamespaceDirective *asUsingNamespaceDirective()
    { return this; }

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);
};

class CPLUSPLUS_EXPORT UsingDeclaration: public Symbol
{
public:
    UsingDeclaration(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name);
    virtual ~UsingDeclaration();

    // Symbol's interface
    virtual FullySpecifiedType type() const;

    virtual const UsingDeclaration *asUsingDeclaration() const
    { return this; }

    virtual UsingDeclaration *asUsingDeclaration()
    { return this; }

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);
};

class CPLUSPLUS_EXPORT NamespaceAlias: public Symbol
{
public:
    NamespaceAlias(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name);
    virtual ~NamespaceAlias();

    const Name *namespaceName() const;
    void setNamespaceName(const Name *namespaceName);

    // Symbol's interface
    virtual FullySpecifiedType type() const;

    virtual const NamespaceAlias *asNamespaceAlias() const
    { return this; }

    virtual NamespaceAlias *asNamespaceAlias()
    { return this; }

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);

private:
    const Name *_namespaceName;
};

class CPLUSPLUS_EXPORT Declaration: public Symbol
{
public:
    Declaration(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name);
    virtual ~Declaration();

    void setType(const FullySpecifiedType &type);

    // Symbol's interface
    virtual FullySpecifiedType type() const;

    virtual const Declaration *asDeclaration() const
    { return this; }

    virtual Declaration *asDeclaration()
    { return this; }

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);

private:
    FullySpecifiedType _type;
};

class CPLUSPLUS_EXPORT Argument: public Symbol
{
public:
    Argument(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name);
    virtual ~Argument();

    void setType(const FullySpecifiedType &type);

    bool hasInitializer() const;

    const StringLiteral *initializer() const;
    void setInitializer(const StringLiteral *initializer);

    // Symbol's interface
    virtual FullySpecifiedType type() const;

    virtual const Argument *asArgument() const
    { return this; }

    virtual Argument *asArgument()
    { return this; }

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);

private:
    const StringLiteral *_initializer;
    FullySpecifiedType _type;
};

class CPLUSPLUS_EXPORT TypenameArgument: public Symbol
{
public:
    TypenameArgument(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name);
    virtual ~TypenameArgument();

    void setType(const FullySpecifiedType &type);

    // Symbol's interface
    virtual FullySpecifiedType type() const;

    virtual const TypenameArgument *asTypenameArgument() const
    { return this; }

    virtual TypenameArgument *asTypenameArgument()
    { return this; }

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);

private:
    FullySpecifiedType _type;
};

class CPLUSPLUS_EXPORT Block: public Scope
{
public:
    Block(TranslationUnit *translationUnit, unsigned sourceLocation);
    virtual ~Block();

    // Symbol's interface
    virtual FullySpecifiedType type() const;

    virtual const Block *asBlock() const
    { return this; }

    virtual Block *asBlock()
    { return this; }

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);
};

class CPLUSPLUS_EXPORT ForwardClassDeclaration: public Symbol, public Type
{
public:
    ForwardClassDeclaration(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name);
    virtual ~ForwardClassDeclaration();

    virtual FullySpecifiedType type() const;

    virtual bool isEqualTo(const Type *other) const;

    virtual const ForwardClassDeclaration *asForwardClassDeclaration() const
    { return this; }

    virtual ForwardClassDeclaration *asForwardClassDeclaration()
    { return this; }

    virtual const ForwardClassDeclaration *asForwardClassDeclarationType() const
    { return this; }

    virtual ForwardClassDeclaration *asForwardClassDeclarationType()
    { return this; }

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);
    virtual void accept0(TypeVisitor *visitor);
    virtual bool matchType0(const Type *otherType, TypeMatcher *matcher) const;
};

class CPLUSPLUS_EXPORT Enum: public Scope, public Type
{
public:
    Enum(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name);
    virtual ~Enum();

    // Symbol's interface
    virtual FullySpecifiedType type() const;

    // Type's interface
    virtual bool isEqualTo(const Type *other) const;

    virtual const Enum *asEnum() const
    { return this; }

    virtual Enum *asEnum()
    { return this; }

    virtual const Enum *asEnumType() const
    { return this; }

    virtual Enum *asEnumType()
    { return this; }

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);
    virtual void accept0(TypeVisitor *visitor);
    virtual bool matchType0(const Type *otherType, TypeMatcher *matcher) const;
};

class CPLUSPLUS_EXPORT Function: public Scope, public Type
{
public:
    enum MethodKey {
        NormalMethod,
        SlotMethod,
        SignalMethod,
        InvokableMethod
    };

public:
    Function(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name);
    virtual ~Function();

    bool isNormal() const;
    bool isSignal() const;
    bool isSlot() const;
    bool isInvokable() const;
    int methodKey() const;
    void setMethodKey(int key);

    FullySpecifiedType returnType() const;
    void setReturnType(const FullySpecifiedType &returnType);

    /** Convenience function that returns whether the function returns something (including void). */
    bool hasReturnType() const;

    unsigned argumentCount() const;
    Symbol *argumentAt(unsigned index) const;

    /** Convenience function that returns whether the function receives any arguments. */
    bool hasArguments() const;
    unsigned minimumArgumentCount() const;

    bool isVirtual() const;
    void setVirtual(bool isVirtual);

    bool isVariadic() const;
    void setVariadic(bool isVariadic);

    bool isConst() const;
    void setConst(bool isConst);

    bool isVolatile() const;
    void setVolatile(bool isVolatile);

    bool isPureVirtual() const;
    void setPureVirtual(bool isPureVirtual);

#ifdef ICHECK_BUILD
    bool isEqualTo(const Function* fct, bool ignoreName = false) const;
#endif

    // Symbol's interface
    virtual FullySpecifiedType type() const;

    // Type's interface
    virtual bool isEqualTo(const Type *other) const;

    virtual const Function *asFunction() const
    { return this; }

    virtual Function *asFunction()
    { return this; }

    virtual const Function *asFunctionType() const
    { return this; }

    virtual Function *asFunctionType()
    { return this; }

    bool isAmbiguous() const; // internal
    void setAmbiguous(bool isAmbiguous); // internal

    bool maybeValidPrototype(unsigned actualArgumentCount) const;

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);
    virtual void accept0(TypeVisitor *visitor);
    virtual bool matchType0(const Type *otherType, TypeMatcher *matcher) const;

private:
    FullySpecifiedType _returnType;
    struct Flags {
        unsigned _isVirtual: 1;
        unsigned _isVariadic: 1;
        unsigned _isPureVirtual: 1;
        unsigned _isConst: 1;
        unsigned _isVolatile: 1;
        unsigned _isAmbiguous: 1;
        unsigned _methodKey: 3;
    };
    union {
        unsigned _flags;
        Flags f;
    };
};

class CPLUSPLUS_EXPORT Template: public Scope, public Type
{
public:
    Template(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name);
    virtual ~Template();

    unsigned templateParameterCount() const;
    Symbol *templateParameterAt(unsigned index) const;
    Symbol *declaration() const;

    // Symbol's interface
    virtual FullySpecifiedType type() const;

    // Type's interface
    virtual bool isEqualTo(const Type *other) const;

    virtual const Template *asTemplate() const
    { return this; }

    virtual Template *asTemplate()
    { return this; }

    virtual const Template *asTemplateType() const
    { return this; }

    virtual Template *asTemplateType()
    { return this; }

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);
    virtual void accept0(TypeVisitor *visitor);
    virtual bool matchType0(const Type *otherType, TypeMatcher *matcher) const;
};


class CPLUSPLUS_EXPORT Namespace: public Scope, public Type
{
public:
    Namespace(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name);
    virtual ~Namespace();

    // Symbol's interface
    virtual FullySpecifiedType type() const;

    // Type's interface
    virtual bool isEqualTo(const Type *other) const;

    virtual const Namespace *asNamespace() const
    { return this; }

    virtual Namespace *asNamespace()
    { return this; }

    virtual const Namespace *asNamespaceType() const
    { return this; }

    virtual Namespace *asNamespaceType()
    { return this; }

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);
    virtual void accept0(TypeVisitor *visitor);
    virtual bool matchType0(const Type *otherType, TypeMatcher *matcher) const;
};

class CPLUSPLUS_EXPORT BaseClass: public Symbol
{
public:
    BaseClass(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name);
    virtual ~BaseClass();

    bool isVirtual() const;
    void setVirtual(bool isVirtual);

    // Symbol's interface
    virtual FullySpecifiedType type() const;
    void setType(const FullySpecifiedType &type);

    virtual const BaseClass *asBaseClass() const
    { return this; }

    virtual BaseClass *asBaseClass()
    { return this; }

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);

private:
    bool _isVirtual;
    FullySpecifiedType _type;
};

class CPLUSPLUS_EXPORT Class: public Scope, public Type
{
public:
    Class(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name);
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

    unsigned baseClassCount() const;
    BaseClass *baseClassAt(unsigned index) const;
    void addBaseClass(BaseClass *baseClass);

    // Symbol's interface
    virtual FullySpecifiedType type() const;

    // Type's interface
    virtual bool isEqualTo(const Type *other) const;

    virtual const Class *asClass() const
    { return this; }

    virtual Class *asClass()
    { return this; }

    virtual const Class *asClassType() const
    { return this; }

    virtual Class *asClassType()
    { return this; }

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);
    virtual void accept0(TypeVisitor *visitor);
    virtual bool matchType0(const Type *otherType, TypeMatcher *matcher) const;

private:
    Key _key;
    std::vector<BaseClass *> _baseClasses;
};

class CPLUSPLUS_EXPORT ObjCBaseClass: public Symbol
{
public:
    ObjCBaseClass(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name);
    virtual ~ObjCBaseClass();

    // Symbol's interface
    virtual FullySpecifiedType type() const;

    virtual const ObjCBaseClass *asObjCBaseClass() const
    { return this; }

    virtual ObjCBaseClass *asObjCBaseClass()
    { return this; }

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);
};

class CPLUSPLUS_EXPORT ObjCBaseProtocol: public Symbol
{
public:
    ObjCBaseProtocol(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name);
    virtual ~ObjCBaseProtocol();

    // Symbol's interface
    virtual FullySpecifiedType type() const;

    virtual const ObjCBaseProtocol *asObjCBaseProtocol() const
    { return this; }

    virtual ObjCBaseProtocol *asObjCBaseProtocol()
    { return this; }

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);
};

class CPLUSPLUS_EXPORT ObjCForwardProtocolDeclaration: public Symbol, public Type
{
public:
    ObjCForwardProtocolDeclaration(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name);
    virtual ~ObjCForwardProtocolDeclaration();

    virtual FullySpecifiedType type() const;

    virtual bool isEqualTo(const Type *other) const;

    virtual const ObjCForwardProtocolDeclaration *asObjCForwardProtocolDeclaration() const
    { return this; }

    virtual ObjCForwardProtocolDeclaration *asObjCForwardProtocolDeclaration()
    { return this; }

    virtual const ObjCForwardProtocolDeclaration *asObjCForwardProtocolDeclarationType() const
    { return this; }

    virtual ObjCForwardProtocolDeclaration *asObjCForwardProtocolDeclarationType()
    { return this; }

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);
    virtual void accept0(TypeVisitor *visitor);
    virtual bool matchType0(const Type *otherType, TypeMatcher *matcher) const;
};

class CPLUSPLUS_EXPORT ObjCProtocol: public Scope, public Type
{
public:
    ObjCProtocol(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name);
    virtual ~ObjCProtocol();

    unsigned protocolCount() const;
    ObjCBaseProtocol *protocolAt(unsigned index) const;
    void addProtocol(ObjCBaseProtocol *protocol);

    // Symbol's interface
    virtual FullySpecifiedType type() const;

    // Type's interface
    virtual bool isEqualTo(const Type *other) const;

    virtual const ObjCProtocol *asObjCProtocol() const
    { return this; }

    virtual ObjCProtocol *asObjCProtocol()
    { return this; }

    virtual const ObjCProtocol *asObjCProtocolType() const
    { return this; }

    virtual ObjCProtocol *asObjCProtocolType()
    { return this; }

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);
    virtual void accept0(TypeVisitor *visitor);
    virtual bool matchType0(const Type *otherType, TypeMatcher *matcher) const;

private:
    std::vector<ObjCBaseProtocol *> _protocols;
};

class CPLUSPLUS_EXPORT ObjCForwardClassDeclaration: public Symbol, public Type
{
public:
    ObjCForwardClassDeclaration(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name);
    virtual ~ObjCForwardClassDeclaration();

    virtual FullySpecifiedType type() const;

    virtual bool isEqualTo(const Type *other) const;

    virtual const ObjCForwardClassDeclaration *asObjCForwardClassDeclaration() const
    { return this; }

    virtual ObjCForwardClassDeclaration *asObjCForwardClassDeclaration()
    { return this; }

    virtual const ObjCForwardClassDeclaration *asObjCForwardClassDeclarationType() const
    { return this; }

    virtual ObjCForwardClassDeclaration *asObjCForwardClassDeclarationType()
    { return this; }

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);
    virtual void accept0(TypeVisitor *visitor);
    virtual bool matchType0(const Type *otherType, TypeMatcher *matcher) const;
};

class CPLUSPLUS_EXPORT ObjCClass: public Scope, public Type
{
public:
    ObjCClass(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name);
    virtual ~ObjCClass();

    bool isInterface() const;
    void setInterface(bool isInterface);

    bool isCategory() const;
    const Name *categoryName() const;
    void setCategoryName(const Name *categoryName);

    ObjCBaseClass *baseClass() const;
    void setBaseClass(ObjCBaseClass *baseClass);

    unsigned protocolCount() const;
    ObjCBaseProtocol *protocolAt(unsigned index) const;
    void addProtocol(ObjCBaseProtocol *protocol);

    // Symbol's interface
    virtual FullySpecifiedType type() const;

    // Type's interface
    virtual bool isEqualTo(const Type *other) const;

    virtual const ObjCClass *asObjCClass() const
    { return this; }

    virtual ObjCClass *asObjCClass()
    { return this; }

    virtual const ObjCClass *asObjCClassType() const
    { return this; }

    virtual ObjCClass *asObjCClassType()
    { return this; }

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);
    virtual void accept0(TypeVisitor *visitor);
    virtual bool matchType0(const Type *otherType, TypeMatcher *matcher) const;

private:
    const Name *_categoryName;
    ObjCBaseClass * _baseClass;
    std::vector<ObjCBaseProtocol *> _protocols;
    bool _isInterface;
};

class CPLUSPLUS_EXPORT ObjCMethod: public Scope, public Type
{
public:
    ObjCMethod(TranslationUnit *translationUnit, unsigned sourceLocation, const Name *name);
    virtual ~ObjCMethod();

    FullySpecifiedType returnType() const;
    void setReturnType(const FullySpecifiedType &returnType);

    /** Convenience function that returns whether the function returns something (including void). */
    bool hasReturnType() const;

    unsigned argumentCount() const;
    Symbol *argumentAt(unsigned index) const;

    /** Convenience function that returns whether the function receives any arguments. */
    bool hasArguments() const;

    bool isVariadic() const;
    void setVariadic(bool isVariadic);

    // Symbol's interface
    virtual FullySpecifiedType type() const;

    // Type's interface
    virtual bool isEqualTo(const Type *other) const;

    virtual const ObjCMethod *asObjCMethod() const
    { return this; }

    virtual ObjCMethod *asObjCMethod()
    { return this; }

    virtual const ObjCMethod *asObjCMethodType() const
    { return this; }

    virtual ObjCMethod *asObjCMethodType()
    { return this; }

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);
    virtual void accept0(TypeVisitor *visitor);
    virtual bool matchType0(const Type *otherType, TypeMatcher *matcher) const;

private:
    FullySpecifiedType _returnType;
    struct Flags {
        unsigned _isVariadic: 1;
    };
    union {
        unsigned _flags;
        Flags f;
    };
};

class CPLUSPLUS_EXPORT ObjCPropertyDeclaration: public Symbol
{
public:
    enum PropertyAttributes {
        None = 0,
        Assign = 1 << 0,
        Retain = 1 << 1,
        Copy = 1 << 2,
        ReadOnly = 1 << 3,
        ReadWrite = 1 << 4,
        Getter = 1 << 5,
        Setter = 1 << 6,
        NonAtomic = 1 << 7,

        WritabilityMask = ReadOnly | ReadWrite,
        SetterSemanticsMask = Assign | Retain | Copy
    };

public:
    ObjCPropertyDeclaration(TranslationUnit *translationUnit,
                            unsigned sourceLocation,
                            const Name *name);
    virtual ~ObjCPropertyDeclaration();

    bool hasAttribute(int attribute) const;
    void setAttributes(int attributes);

    bool hasGetter() const;
    bool hasSetter() const;

    const Name *getterName() const;

    void setGetterName(const Name *getterName);

    const Name *setterName() const;
    void setSetterName(const Name *setterName);

    void setType(const FullySpecifiedType &type);

    // Symbol's interface
    virtual FullySpecifiedType type() const;

    virtual const ObjCPropertyDeclaration *asObjCPropertyDeclaration() const
    { return this; }

    virtual ObjCPropertyDeclaration *asObjCPropertyDeclaration()
    { return this; }

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor);

private:
    const Name *_getterName;
    const Name *_setterName;
    FullySpecifiedType _type;
    int _propertyAttributes;
};

} // end of namespace CPlusPlus


#endif // CPLUSPLUS_SYMBOLS_H
