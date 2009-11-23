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

#ifndef TYPEMATCHER_H
#define TYPEMATCHER_H

#include "CPlusPlusForwardDeclarations.h"

namespace CPlusPlus {

class CPLUSPLUS_EXPORT TypeMatcher
{
    TypeMatcher(const TypeMatcher &other);
    void operator = (const TypeMatcher &other);

public:
    TypeMatcher();
    virtual ~TypeMatcher();

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
    virtual bool match(const ForwardClassDeclaration *type, const ForwardClassDeclaration *otherType);
    virtual bool match(const Class *type, const Class *otherType);
    virtual bool match(const ObjCClass *type, const ObjCClass *otherType);
    virtual bool match(const ObjCProtocol *type, const ObjCProtocol *otherType);
    virtual bool match(const ObjCForwardClassDeclaration *type, const ObjCForwardClassDeclaration *otherType);
    virtual bool match(const ObjCForwardProtocolDeclaration *type, const ObjCForwardProtocolDeclaration *otherType);
    virtual bool match(const ObjCMethod *type, const ObjCMethod *otherType);

    bool isEqualTo(const Name *name, const Name *otherName) const;
};

} // end of namespace CPlusPlus

#endif // TYPEMATCHER_H
