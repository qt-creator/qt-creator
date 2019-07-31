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

namespace CPlusPlus {

class CPLUSPLUS_EXPORT Type
{
public:
    Type();
    virtual ~Type();

    bool isUndefinedType() const;
    bool isVoidType() const;
    bool isIntegerType() const;
    bool isFloatType() const;
    bool isPointerType() const;
    bool isPointerToMemberType() const;
    bool isReferenceType() const;
    bool isArrayType() const;
    bool isNamedType() const;
    bool isFunctionType() const;
    bool isNamespaceType() const;
    bool isTemplateType() const;
    bool isClassType() const;
    bool isEnumType() const;
    bool isForwardClassDeclarationType() const;
    bool isObjCClassType() const;
    bool isObjCProtocolType() const;
    bool isObjCMethodType() const;
    bool isObjCForwardClassDeclarationType() const;
    bool isObjCForwardProtocolDeclarationType() const;

    virtual const UndefinedType *asUndefinedType() const { return nullptr; }
    virtual const VoidType *asVoidType() const { return nullptr; }
    virtual const IntegerType *asIntegerType() const { return nullptr; }
    virtual const FloatType *asFloatType() const { return nullptr; }
    virtual const PointerType *asPointerType() const { return nullptr; }
    virtual const PointerToMemberType *asPointerToMemberType() const { return nullptr; }
    virtual const ReferenceType *asReferenceType() const { return nullptr; }
    virtual const ArrayType *asArrayType() const { return nullptr; }
    virtual const NamedType *asNamedType() const { return nullptr; }
    virtual const Function *asFunctionType() const { return nullptr; }
    virtual const Namespace *asNamespaceType() const { return nullptr; }
    virtual const Template *asTemplateType() const { return nullptr; }
    virtual const Class *asClassType() const { return nullptr; }
    virtual const Enum *asEnumType() const { return nullptr; }
    virtual const ForwardClassDeclaration *asForwardClassDeclarationType() const { return nullptr; }
    virtual const ObjCClass *asObjCClassType() const { return nullptr; }
    virtual const ObjCProtocol *asObjCProtocolType() const { return nullptr; }
    virtual const ObjCMethod *asObjCMethodType() const { return nullptr; }
    virtual const ObjCForwardClassDeclaration *asObjCForwardClassDeclarationType() const { return nullptr; }
    virtual const ObjCForwardProtocolDeclaration *asObjCForwardProtocolDeclarationType() const { return nullptr; }

    virtual UndefinedType *asUndefinedType() { return nullptr; }
    virtual VoidType *asVoidType() { return nullptr; }
    virtual IntegerType *asIntegerType() { return nullptr; }
    virtual FloatType *asFloatType() { return nullptr; }
    virtual PointerType *asPointerType() { return nullptr; }
    virtual PointerToMemberType *asPointerToMemberType() { return nullptr; }
    virtual ReferenceType *asReferenceType() { return nullptr; }
    virtual ArrayType *asArrayType() { return nullptr; }
    virtual NamedType *asNamedType() { return nullptr; }
    virtual Function *asFunctionType() { return nullptr; }
    virtual Namespace *asNamespaceType() { return nullptr; }
    virtual Template *asTemplateType() { return nullptr; }
    virtual Class *asClassType() { return nullptr; }
    virtual Enum *asEnumType() { return nullptr; }
    virtual ForwardClassDeclaration *asForwardClassDeclarationType() { return nullptr; }
    virtual ObjCClass *asObjCClassType() { return nullptr; }
    virtual ObjCProtocol *asObjCProtocolType() { return nullptr; }
    virtual ObjCMethod *asObjCMethodType() { return nullptr; }
    virtual ObjCForwardClassDeclaration *asObjCForwardClassDeclarationType() { return nullptr; }
    virtual ObjCForwardProtocolDeclaration *asObjCForwardProtocolDeclarationType() { return nullptr; }

    void accept(TypeVisitor *visitor);
    static void accept(Type *type, TypeVisitor *visitor);

    bool match(const Type *other, Matcher *matcher = nullptr) const;

protected:
    virtual void accept0(TypeVisitor *visitor) = 0;

protected: // for Matcher
    friend class Matcher;
    virtual bool match0(const Type *otherType, Matcher *matcher) const = 0;
};

} // namespace CPlusPlus
