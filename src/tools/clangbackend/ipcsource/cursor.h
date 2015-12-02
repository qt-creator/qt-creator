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

#ifndef CLANGBACKEND_CURSOR_H
#define CLANGBACKEND_CURSOR_H

#include "clangtype.h"

#include <clang-c/Index.h>

#include <iosfwd>

#include <vector>

class Utf8String;

namespace ClangBackEnd {

class SourceLocation;
class SourceRange;
class ClangString;

class Cursor
{
    friend class Type;
    friend bool operator==(const Cursor &first, const Cursor &second);
public:
    Cursor();
    Cursor(CXCursor cxCursor);

    bool isNull() const;
    bool isValid() const;

    bool isTranslationUnit() const;
    bool isDefinition() const;
    bool isDynamicCall() const;
    bool isVirtualMethod() const;
    bool isPureVirtualMethod() const;
    bool isConstantMethod() const;
    bool isStaticMethod() const;
    bool isCompoundType() const;
    bool isDeclaration() const;
    bool isLocalVariable() const;
    bool hasFinalFunctionAttribute() const;
    bool hasFinalClassAttribute() const;
    bool isUnexposed() const;

    Utf8String unifiedSymbolResolution() const;
    Utf8String mangling() const;
    ClangString spelling() const;
    Utf8String displayName() const;
    Utf8String briefComment() const;
    Utf8String rawComment() const;
    int argumentCount() const;

    Type type() const;
    Type nonPointerTupe() const;

    SourceLocation sourceLocation() const;
    SourceRange sourceRange() const;
    SourceRange commentRange() const;

    Cursor definition() const;
    Cursor canonical() const;
    Cursor alias() const;
    Cursor referenced() const;
    Cursor semanticParent() const;
    Cursor lexicalParent() const;
    Cursor functionBaseDeclaration() const;
    Cursor functionBase() const;
    Cursor argument(int index) const;
    std::vector<Cursor> outputArguments() const;

    CXCursorKind kind() const;

    template <class VisitorCallback>
    void visit(VisitorCallback visitorCallback) const;

private:
    CXCursor cxCursor;
};

template <class VisitorCallback>
void Cursor::visit(VisitorCallback visitorCallback) const
{
    auto visitor = [] (CXCursor cursor, CXCursor parent, CXClientData lambda) -> CXChildVisitResult {
        auto &visitorCallback = *static_cast<VisitorCallback*>(lambda);

        return visitorCallback(cursor, parent);
    };

    clang_visitChildren(cxCursor, visitor, &visitorCallback);
}

bool operator==(const Cursor &first, const Cursor &second);

void PrintTo(CXCursorKind cursorKind, ::std::ostream *os);
void PrintTo(const Cursor &cursor, ::std::ostream* os);
} // namespace ClangBackEnd


#endif // CLANGBACKEND_CURSOR_H
