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

#ifndef CPLUSPLUS_TYPE_H
#define CPLUSPLUS_TYPE_H

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
    bool isClassType() const;
    bool isEnumType() const;
    bool isForwardClassDeclarationType() const;
    bool isObjCClassType() const;
    bool isObjCProtocolType() const;
    bool isObjCMethodType() const;
    bool isObjCForwardClassDeclarationType() const;
    bool isObjCForwardProtocolDeclarationType() const;

    virtual const UndefinedType *asUndefinedType() const { return 0; }
    virtual const VoidType *asVoidType() const { return 0; }
    virtual const IntegerType *asIntegerType() const { return 0; }
    virtual const FloatType *asFloatType() const { return 0; }
    virtual const PointerType *asPointerType() const { return 0; }
    virtual const PointerToMemberType *asPointerToMemberType() const { return 0; }
    virtual const ReferenceType *asReferenceType() const { return 0; }
    virtual const ArrayType *asArrayType() const { return 0; }
    virtual const NamedType *asNamedType() const { return 0; }
    virtual const Function *asFunctionType() const { return 0; }
    virtual const Namespace *asNamespaceType() const { return 0; }
    virtual const Class *asClassType() const { return 0; }
    virtual const Enum *asEnumType() const { return 0; }
    virtual const ForwardClassDeclaration *asForwardClassDeclarationType() const { return 0; }
    virtual const ObjCClass *asObjCClassType() const { return 0; }
    virtual const ObjCProtocol *asObjCProtocolType() const { return 0; }
    virtual const ObjCMethod *asObjCMethodType() const { return 0; }
    virtual const ObjCForwardClassDeclaration *asObjCForwardClassDeclarationType() const { return 0; }
    virtual const ObjCForwardProtocolDeclaration *asObjCForwardProtocolDeclarationType() const { return 0; }

    virtual UndefinedType *asUndefinedType() { return 0; }
    virtual VoidType *asVoidType() { return 0; }
    virtual IntegerType *asIntegerType() { return 0; }
    virtual FloatType *asFloatType() { return 0; }
    virtual PointerType *asPointerType() { return 0; }
    virtual PointerToMemberType *asPointerToMemberType() { return 0; }
    virtual ReferenceType *asReferenceType() { return 0; }
    virtual ArrayType *asArrayType() { return 0; }
    virtual NamedType *asNamedType() { return 0; }
    virtual Function *asFunctionType() { return 0; }
    virtual Namespace *asNamespaceType() { return 0; }
    virtual Class *asClassType() { return 0; }
    virtual Enum *asEnumType() { return 0; }
    virtual ForwardClassDeclaration *asForwardClassDeclarationType() { return 0; }
    virtual ObjCClass *asObjCClassType() { return 0; }
    virtual ObjCProtocol *asObjCProtocolType() { return 0; }
    virtual ObjCMethod *asObjCMethodType() { return 0; }
    virtual ObjCForwardClassDeclaration *asObjCForwardClassDeclarationType() { return 0; }
    virtual ObjCForwardProtocolDeclaration *asObjCForwardProtocolDeclarationType() { return 0; }

    void accept(TypeVisitor *visitor);
    static void accept(Type *type, TypeVisitor *visitor);

    static bool matchType(const Type *type, const Type *otherType, TypeMatcher *matcher)
    {
        if (! type)
            return type == otherType;

        return type->matchType(otherType, matcher);
    }

    bool matchType(const Type *otherType, TypeMatcher *matcher) const;

    virtual bool isEqualTo(const Type *other) const = 0; // ### remove

protected:
    virtual void accept0(TypeVisitor *visitor) = 0;
    virtual bool matchType0(const Type *otherType, TypeMatcher *matcher) const = 0;
};

} // end of namespace CPlusPlus


#endif // CPLUSPLUS_TYPE_H
