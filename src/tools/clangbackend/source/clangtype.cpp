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

#include "clangtype.h"

#include "clangstring.h"
#include "cursor.h"

#include <utf8string.h>

#include <ostream>

namespace ClangBackEnd {

bool Type::isValid() const
{
    return m_cxType.kind != CXType_Invalid;
}

bool Type::isConstant() const
{
    return clang_isConstQualifiedType(m_cxType);
}

bool Type::isConstantReference()
{
    return isLValueReference() && pointeeType().isConstant();
}

bool Type::isPointer() const
{
    return m_cxType.kind == CXType_Pointer;
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
    return m_cxType.kind == CXType_LValueReference;
}

bool Type::isReferencingConstant() const
{
    return (isPointer() || isLValueReference()) && pointeeType().isConstant();
}

bool Type::isOutputArgument() const
{
    return isLValueReference() && !pointeeType().isConstant();
}

bool Type::isBuiltinType() const
{
    return m_cxType.kind >= CXType_FirstBuiltin && m_cxType.kind <= CXType_LastBuiltin;
}

bool Type::isUnsigned() const
{
    return m_cxType.kind == CXType_UChar
       ||  m_cxType.kind == CXType_UShort
       ||  m_cxType.kind == CXType_UInt
       ||  m_cxType.kind == CXType_ULong
       ||  m_cxType.kind == CXType_ULongLong
       ||  m_cxType.kind == CXType_UInt128;
}

Utf8String Type::utf8Spelling() const
{
    return  ClangString(clang_getTypeSpelling(m_cxType));
}

ClangString Type::spelling() const
{
    return ClangString(clang_getTypeSpelling(m_cxType));
}

static const char *builtinTypeToText(CXTypeKind kind)
{
    // CLANG-UPGRADE-CHECK: Check for added built-in types.

    switch (kind) {
    case CXType_Void:
        return "void";
    case CXType_Bool:
        return "bool";

    // See also ${CLANG_REPOSITORY}/lib/Sema/SemaChecking.cpp - IsSameCharType().
    case CXType_Char_U:
    case CXType_UChar:
        return "unsigned char";
    case CXType_Char_S:
    case CXType_SChar:
        return "signed char";

    case CXType_Char16:
        return "char16_t";
    case CXType_Char32:
        return "char32_t";
    case CXType_WChar:
        return "wchar_t";

    case CXType_UShort:
        return "unsigned short";
    case CXType_UInt:
        return "unsigned int";
    case CXType_ULong:
        return "unsigned long";
    case CXType_ULongLong:
        return "unsigned long long";
    case CXType_Short:
        return "short";

    case CXType_Int:
        return "int";
    case CXType_Long:
        return "long";
    case CXType_LongLong:
        return "long long";

    case CXType_Float:
        return "float";
    case CXType_Double:
        return "double";
    case CXType_LongDouble:
        return "long double";

    case CXType_NullPtr:
        return "nullptr_t";

    // https://gcc.gnu.org/onlinedocs/gcc/_005f_005fint128.html
    case CXType_Int128: return "__int128";
    case CXType_UInt128: return "unsigned __int128";

    // https://gcc.gnu.org/onlinedocs/gcc/Floating-Types.html
    case CXType_Float128: return "__float128";
    case CXType_Float16: return "_Float16";

    // https://www.khronos.org/registry/OpenCL/sdk/2.1/docs/man/xhtml/scalarDataTypes.html
    case CXType_Half:
        return "half";

    default:
        return "";
    }
}

Utf8String Type::builtinTypeToString() const
{
    const char *text = builtinTypeToText(m_cxType.kind);
    return Utf8String::fromByteArray(QByteArray::fromRawData(text, int(strlen(text))));
}

int Type::argumentCount() const
{
    return clang_getNumArgTypes(m_cxType);
}

Type Type::alias() const
{
    return clang_getTypedefDeclUnderlyingType(clang_getTypeDeclaration(m_cxType));
}

Type Type::canonical() const
{
    return clang_getCanonicalType(m_cxType);
}

Type Type::classType() const
{
    return clang_Type_getClassType(m_cxType);
}

Type Type::pointeeType() const
{
    return clang_getPointeeType(m_cxType);
}

Type Type::resultType() const
{
    return clang_getResultType(m_cxType);
}

Type Type::argument(int index) const
{
    return clang_getArgType(m_cxType, index);
}

Cursor Type::declaration() const
{
    return clang_getTypeDeclaration(m_cxType);
}

long long Type::sizeOf(bool *isValid) const
{
    const long long size = clang_Type_getSizeOf(m_cxType);
    *isValid = size != CXTypeLayoutError_Invalid
            && size != CXTypeLayoutError_Incomplete
            && size != CXTypeLayoutError_Dependent;

    return size;
}

CXTypeKind Type::kind() const
{
    return m_cxType.kind;
}

Type::Type(CXType cxType)
    : m_cxType(cxType)
{
}

bool operator==(Type first, Type second)
{
    return clang_equalTypes(first.m_cxType, second.m_cxType);
}

bool operator!=(Type first, Type second)
{
    return !operator==(first, second);
}

std::ostream &operator<<(std::ostream &os, CXTypeKind typeKind)
{
    ClangString typeKindSpelling(clang_getTypeKindSpelling(typeKind));

    return os << typeKindSpelling.cString();
}

std::ostream &operator<<(std::ostream &os, const Type &type)
{
    ClangString typeKindSpelling(clang_getTypeKindSpelling(type.kind()));
    os << typeKindSpelling
       << ": \"" << type.spelling() << "\"";

    return os;
}


} // namespace ClangBackEnd

