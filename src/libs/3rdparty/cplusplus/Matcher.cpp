/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "Matcher.h"
#include "CoreTypes.h"
#include "Symbols.h"
#include "Names.h"
#include "Literals.h"

using namespace CPlusPlus;

Matcher::Matcher()
{
}

Matcher::~Matcher()
{
}

bool Matcher::match(const Type *type, const Type *otherType, Matcher *matcher)
{
    if (type == otherType)
        return true;
    if (!type || !otherType)
        return false;

    return type->match0(otherType, matcher);
}

bool Matcher::match(const Name *name, const Name *otherName, Matcher *matcher)
{
    if (name == otherName)
        return true;
    if (!name || !otherName)
        return false;

    return name->match0(otherName, matcher);
}

bool Matcher::match(const UndefinedType *, const UndefinedType *)
{
    return true;
}

bool Matcher::match(const VoidType *, const VoidType *)
{
    return true;
}

bool Matcher::match(const IntegerType *type, const IntegerType *otherType)
{
    if (type == otherType)
        return true;

    else if (type->kind() != otherType->kind())
        return false;

    return true;
}

bool Matcher::match(const FloatType *type, const FloatType *otherType)
{
    if (type == otherType)
        return true;

    else if (type->kind() != otherType->kind())
        return false;

    return true;
}

bool Matcher::match(const PointerToMemberType *type, const PointerToMemberType *otherType)
{
    if (type == otherType)
        return true;

    else if (! Name::match(type->memberName(), otherType->memberName(), this))
        return false;

    else if (! type->elementType().match(otherType->elementType(), this))
        return false;

    return true;
}

bool Matcher::match(const PointerType *type, const PointerType *otherType)
{
    if (type == otherType)
        return true;

    else if (! type->elementType().match(otherType->elementType(), this))
        return false;

    return true;
}

bool Matcher::match(const ReferenceType *type, const ReferenceType *otherType)
{
    if (type == otherType)
        return true;

    else if (type->isRvalueReference() != otherType->isRvalueReference())
        return false;

    else if (! type->elementType().match(otherType->elementType(), this))
        return false;

    return true;
}

bool Matcher::match(const ArrayType *type, const ArrayType *otherType)
{
    if (type == otherType)
        return true;

    else if (type->size() != otherType->size())
        return false;

    else if (! type->elementType().match(otherType->elementType(), this))
        return false;

    return true;
}

bool Matcher::match(const NamedType *type, const NamedType *otherType)
{
    if (type == otherType)
        return true;

    else if (! Name::match(type->name(), otherType->name(), this))
        return false;

    return true;
}

bool Matcher::match(const Function *type, const Function *otherType)
{
    if (type != otherType)
        return false;

    return true;
}

bool Matcher::match(const Enum *type, const Enum *otherType)
{
    if (type != otherType)
        return false;

    return true;
}

bool Matcher::match(const Namespace *type, const Namespace *otherType)
{
    if (type != otherType)
        return false;

    return true;
}

bool Matcher::match(const Template *type, const Template *otherType)
{
    if (type != otherType)
        return false;

    return true;
}

bool Matcher::match(const ForwardClassDeclaration *type, const ForwardClassDeclaration *otherType)
{
    if (type != otherType)
        return false;

    return true;
}

bool Matcher::match(const Class *type, const Class *otherType)
{
    if (type != otherType)
        return false;

    return true;
}

bool Matcher::match(const ObjCClass *type, const ObjCClass *otherType)
{
    if (type != otherType)
        return false;

    return true;
}

bool Matcher::match(const ObjCProtocol *type, const ObjCProtocol *otherType)
{
    if (type != otherType)
        return false;

    return true;
}

bool Matcher::match(const ObjCForwardClassDeclaration *type, const ObjCForwardClassDeclaration *otherType)
{
    if (type != otherType)
        return false;

    return true;
}

bool Matcher::match(const ObjCForwardProtocolDeclaration *type, const ObjCForwardProtocolDeclaration *otherType)
{
    if (type != otherType)
        return false;

    return true;
}

bool Matcher::match(const ObjCMethod *type, const ObjCMethod *otherType)
{
    if (type != otherType)
        return false;

    return true;
}

bool Matcher::match(const Identifier *name, const Identifier *otherName)
{
    if (name == otherName)
        return true;
    return name->equalTo(otherName);
}

bool Matcher::match(const AnonymousNameId *name, const AnonymousNameId *otherName)
{
    return otherName && name->classTokenIndex() == otherName->classTokenIndex();
}

bool Matcher::match(const TemplateNameId *name, const TemplateNameId *otherName)
{
    const Identifier *l = name->identifier();
    const Identifier *r = otherName->identifier();
    if (! match(l, r))
        return false;
    if (name->templateArgumentCount() != otherName->templateArgumentCount())
        return false;
    for (unsigned i = 0, ei = name->templateArgumentCount(); i != ei; ++i) {
        const FullySpecifiedType &l = name->templateArgumentAt(i);
        const FullySpecifiedType &r = otherName->templateArgumentAt(i);
        if (! l.match(r, this))
            return false;
    }
    return true;
}

bool Matcher::match(const DestructorNameId *name, const DestructorNameId *otherName)
{
    return Name::match(name->name(), otherName->name(), this);
}

bool Matcher::match(const OperatorNameId *name, const OperatorNameId *otherName)
{
    return name->kind() == otherName->kind();
}

bool Matcher::match(const ConversionNameId *name, const ConversionNameId *otherName)
{
    return name->type().match(otherName->type(), this);
}

bool Matcher::match(const QualifiedNameId *name, const QualifiedNameId *otherName)
{
    if (Name::match(name->base(), otherName->base(), this))
        return Name::match(name->name(), otherName->name(), this);
    return false;
}

bool Matcher::match(const SelectorNameId *name, const SelectorNameId *otherName)
{
    const unsigned nc = name->nameCount();
    if (name->hasArguments() != otherName->hasArguments() ||
            nc != otherName->nameCount())
        return false;
    for (unsigned i = 0; i < nc; ++i)
        if (!Name::match(name->nameAt(i), otherName->nameAt(i), this))
            return false;
    return true;
}
