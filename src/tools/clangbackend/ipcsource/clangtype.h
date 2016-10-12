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

#include <clang-c/Index.h>

#include <iosfwd>

class Utf8String;

namespace ClangBackEnd {

class Cursor;
class ClangString;

class Type
{
    friend class Cursor;
    friend bool operator==(Type first, Type second);

public:
    bool isConstant() const;
    bool isConstantReference();
    bool isPointer() const;
    bool isPointerToConstant() const;
    bool isConstantPointer() const;
    bool isLValueReference() const;
    bool isReferencingConstant() const;
    bool isOutputArgument() const;

    Utf8String utf8Spelling() const;
    ClangString spelling() const;
    int argumentCount() const;

    Type alias() const;
    Type canonical() const;
    Type classType() const;
    Type pointeeType() const;
    Type argument(int index) const;

    Cursor declaration() const;

    CXTypeKind kind() const;

private:
    Type(CXType cxType);

private:
    CXType cxType;
};

bool operator==(Type first, Type second);

void PrintTo(CXTypeKind typeKind, ::std::ostream* os);
void PrintTo(const Type &type, ::std::ostream* os);
} // namespace ClangBackEnd
