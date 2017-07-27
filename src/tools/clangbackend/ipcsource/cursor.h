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
    bool isReference() const;
    bool isExpression() const;
    bool isFunctionLike() const;
    bool isConstructorOrDestructor() const;
    bool isTemplateLike() const;
    bool hasFinalFunctionAttribute() const;
    bool hasFinalClassAttribute() const;
    bool isUnexposed() const;

    ClangString unifiedSymbolResolution() const;
    ClangString mangling() const;
    ClangString spelling() const;
    ClangString displayName() const;
    ClangString briefComment() const;
    ClangString rawComment() const;
    int argumentCount() const;

    Type type() const;
    Type nonPointerTupe() const;

    SourceLocation sourceLocation() const;
    CXSourceLocation cxSourceLocation() const;
    SourceRange sourceRange() const;
    CXSourceRange cxSourceRange() const;
    CXTranslationUnit cxTranslationUnit() const;
    SourceRange commentRange() const;
    bool hasSameSourceLocationAs(const Cursor &other) const;

    Cursor definition() const;
    Cursor canonical() const;
    Cursor alias() const;
    Cursor referenced() const;
    Cursor semanticParent() const;
    Cursor lexicalParent() const;
    Cursor functionBaseDeclaration() const;
    Cursor functionBase() const;
    Cursor argument(int index) const;
    unsigned overloadedDeclarationsCount() const;
    Cursor overloadedDeclaration(unsigned index) const;
    Cursor specializedCursorTemplate() const;

    void collectOutputArgumentRangesTo(
            std::vector<CXSourceRange> &outputArgumentRanges) const;
    std::vector<CXSourceRange> outputArgumentRanges() const;

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
bool operator!=(const Cursor &first, const Cursor &second);

std::ostream &operator<<(std::ostream &os, CXCursorKind cursorKind);
std::ostream &operator<<(std::ostream &os, const Cursor &cursor);

} // namespace ClangBackEnd
