/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "clangtype.h"

#include "clangstring.h"
#include "cursor.h"

#include <utf8string.h>

#include <ostream>

namespace ClangBackEnd {

bool Type::isConstant() const
{
    return clang_isConstQualifiedType(cxType);
}

bool Type::isConstantReference()
{
    return isLValueReference() && pointeeType().isConstant();
}

bool Type::isPointer() const
{
    return cxType.kind == CXType_Pointer;
}

bool Type::isPointerToConstant() const
{
    return isPointer() && pointeeType().isConstant();
}

bool Type::isConstantPointer() const
{
    return isPointer() && isConstant();
}

bool Type::isLValueReference() const
{
    return cxType.kind == CXType_LValueReference;
}

bool Type::isReferencingConstant() const
{
    return (isPointer() || isLValueReference()) && pointeeType().isConstant();
}

bool Type::isOutputParameter() const
{
    return (isPointer() || isLValueReference()) && !pointeeType().isConstant();
}

Utf8String Type::utf8Spelling() const
{
    return  ClangString(clang_getTypeSpelling(cxType));
}

ClangString Type::spelling() const
{
    return ClangString(clang_getTypeSpelling(cxType));
}

int Type::argumentCount() const
{
    return clang_getNumArgTypes(cxType);
}

Type Type::alias() const
{
    return clang_getTypedefDeclUnderlyingType(clang_getTypeDeclaration(cxType));
}

Type Type::canonical() const
{
    return clang_getCanonicalType(cxType);
}

Type Type::classType() const
{
    return clang_Type_getClassType(cxType);
}

Type Type::pointeeType() const
{
    return clang_getPointeeType(cxType);
}

Type Type::argument(int index) const
{
    return clang_getArgType(cxType, index);
}

Cursor Type::declaration() const
{
    return clang_getTypeDeclaration(cxType);
}

CXTypeKind Type::kind() const
{
    return cxType.kind;
}

Type::Type(CXType cxType)
    : cxType(cxType)
{
}

bool operator==(Type first, Type second)
{
    return clang_equalTypes(first.cxType, second.cxType);
}

void PrintTo(CXTypeKind typeKind, ::std::ostream* os)
{
    ClangString typeKindSpelling(clang_getTypeKindSpelling(typeKind));
    *os << typeKindSpelling.cString();
}

void PrintTo(const Type &type, ::std::ostream* os)
{
    ClangString typeKindSpelling(clang_getTypeKindSpelling(type.kind()));
    *os << typeKindSpelling.cString()
        << ": \"" << type.spelling().cString() << "\"";
}


} // namespace ClangBackEnd

