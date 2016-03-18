/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "CPlusPlusForwardDeclarations.h"

namespace CPlusPlus {

class CPLUSPLUS_EXPORT Matcher
{
    Matcher(const Matcher &other);
    void operator = (const Matcher &other);

public:
    Matcher();
    virtual ~Matcher();

    static bool match(const Type *type, const Type *otherType, Matcher *matcher = 0);
    static bool match(const Name *name, const Name *otherName, Matcher *matcher = 0);

    virtual bool match(const UndefinedType *type, const UndefinedType *otherType);
    virtual bool match(const VoidType *type, const VoidType *otherType);
    virtual bool match(const IntegerType *type, const IntegerType *otherType);
    virtual bool match(const FloatType *type, const FloatType *otherType);
    virtual bool match(const PointerToMemberType *type, const PointerToMemberType *otherType);
    virtual bool match(const PointerType *type, const PointerType *otherType);
    virtual bool match(const ReferenceType *type, const ReferenceType *otherType);
    virtual bool match(const ArrayType *type, const ArrayType *otherType);
    virtual bool match(const NamedType *type, const NamedType *otherType);

    virtual bool match(const Function *type, const Function *otherType);
    virtual bool match(const Enum *type, const Enum *otherType);
    virtual bool match(const Namespace *type, const Namespace *otherType);
    virtual bool match(const Template *type, const Template *otherType);
    virtual bool match(const ForwardClassDeclaration *type, const ForwardClassDeclaration *otherType);
    virtual bool match(const Class *type, const Class *otherType);
    virtual bool match(const ObjCClass *type, const ObjCClass *otherType);
    virtual bool match(const ObjCProtocol *type, const ObjCProtocol *otherType);
    virtual bool match(const ObjCForwardClassDeclaration *type, const ObjCForwardClassDeclaration *otherType);
    virtual bool match(const ObjCForwardProtocolDeclaration *type, const ObjCForwardProtocolDeclaration *otherType);
    virtual bool match(const ObjCMethod *type, const ObjCMethod *otherType);

    virtual bool match(const Identifier *name, const Identifier *otherName);
    virtual bool match(const AnonymousNameId *name, const AnonymousNameId *otherName);
    virtual bool match(const TemplateNameId *name, const TemplateNameId *otherName);
    virtual bool match(const DestructorNameId *name, const DestructorNameId *otherName);
    virtual bool match(const OperatorNameId *name, const OperatorNameId *otherName);
    virtual bool match(const ConversionNameId *name, const ConversionNameId *otherName);
    virtual bool match(const QualifiedNameId *name, const QualifiedNameId *otherName);
    virtual bool match(const SelectorNameId *name, const SelectorNameId *otherName);
};

} // namespace CPlusPlus
