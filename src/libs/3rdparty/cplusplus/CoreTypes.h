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

#pragma once

#include "CPlusPlusForwardDeclarations.h"
#include "Type.h"
#include "FullySpecifiedType.h"

namespace CPlusPlus {

class CPLUSPLUS_EXPORT UndefinedType final : public Type
{
public:
    static UndefinedType instance;

    const UndefinedType *asUndefinedType() const override { return this; }
    UndefinedType *asUndefinedType() override { return this; }

protected:
    void accept0(TypeVisitor *visitor) override;
    bool match0(const Type *otherType, Matcher *matcher) const override;
};

class CPLUSPLUS_EXPORT VoidType final : public Type
{
public:
    const VoidType *asVoidType() const override { return this; }
    VoidType *asVoidType() override { return this; }

protected:
    void accept0(TypeVisitor *visitor) override;
    bool match0(const Type *otherType, Matcher *matcher) const override;
};

class CPLUSPLUS_EXPORT IntegerType final : public Type
{
public:
    enum Kind {
        Char,
        Char16,
        Char32,
        WideChar,
        Bool,
        Short,
        Int,
        Long,
        LongLong
    };

public:
    IntegerType(int kind) : _kind(kind) {}
    ~IntegerType() override = default;

    int kind() const { return _kind; }

    IntegerType *asIntegerType() override { return this; }
    const IntegerType *asIntegerType() const override { return this; }

protected:
    void accept0(TypeVisitor *visitor) override;
    bool match0(const Type *otherType, Matcher *matcher) const override;

private:
    int _kind;
};

class CPLUSPLUS_EXPORT FloatType final : public Type
{
public:
    enum Kind {
        Float,
        Double,
        LongDouble
    };

public:
    FloatType(int kind) : _kind(kind) {}
    ~FloatType() override = default;

    int kind() const { return _kind; }

    const FloatType *asFloatType() const override { return this; }
    FloatType *asFloatType() override { return this; }

protected:
    void accept0(TypeVisitor *visitor) override;
    bool match0(const Type *otherType, Matcher *matcher) const override;

private:
    int _kind;
};

class CPLUSPLUS_EXPORT PointerType final : public Type
{
public:
    PointerType(const FullySpecifiedType &elementType) : _elementType(elementType) {}
    ~PointerType() override = default;

    FullySpecifiedType elementType() const { return _elementType; }

    const PointerType *asPointerType() const override { return this; }
    PointerType *asPointerType() override { return this; }

protected:
    void accept0(TypeVisitor *visitor) override;
    bool match0(const Type *otherType, Matcher *matcher) const override;

private:
    FullySpecifiedType _elementType;
};

class CPLUSPLUS_EXPORT PointerToMemberType final : public Type
{
public:
    PointerToMemberType(const Name *memberName, const FullySpecifiedType &elementType);
    ~PointerToMemberType() override = default;

    const Name *memberName() const { return _memberName; }
    FullySpecifiedType elementType() const { return _elementType; }

    const PointerToMemberType *asPointerToMemberType() const override { return this; }
    PointerToMemberType *asPointerToMemberType() override { return this; }

protected:
    void accept0(TypeVisitor *visitor) override;
    bool match0(const Type *otherType, Matcher *matcher) const override;

private:
    const Name *_memberName;
    FullySpecifiedType _elementType;
};

class CPLUSPLUS_EXPORT ReferenceType final : public Type
{
public:
    ReferenceType(const FullySpecifiedType &elementType, bool rvalueRef);
    ~ReferenceType() override = default;

    FullySpecifiedType elementType() const { return _elementType; }
    bool isRvalueReference() const { return _rvalueReference; }

    const ReferenceType *asReferenceType() const override { return this; }
    ReferenceType *asReferenceType() override { return this; }

protected:
    void accept0(TypeVisitor *visitor) override;
    bool match0(const Type *otherType, Matcher *matcher) const override;

private:
    FullySpecifiedType _elementType;
    bool _rvalueReference;
};

class CPLUSPLUS_EXPORT ArrayType final : public Type
{
public:
    ArrayType(const FullySpecifiedType &elementType, unsigned size);
    ~ArrayType() override = default;

    FullySpecifiedType elementType() const { return _elementType; }
    unsigned size() const { return _size; }

    const ArrayType *asArrayType() const override { return this; }
    ArrayType *asArrayType() override { return this; }

protected:
    void accept0(TypeVisitor *visitor) override;
    bool match0(const Type *otherType, Matcher *matcher) const override;

private:
    FullySpecifiedType _elementType;
    unsigned _size;
};

class CPLUSPLUS_EXPORT NamedType final : public Type
{
public:
    NamedType(const Name *name) : _name(name) {}
    ~NamedType() override = default;

    const Name *name() const { return _name; }

    const NamedType *asNamedType() const override { return this; }
    NamedType *asNamedType() override { return this; }

protected:
    void accept0(TypeVisitor *visitor) override;
    bool match0(const Type *otherType, Matcher *matcher) const override;

private:
    const Name *_name;
};

} // namespace CPlusPlus
