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

#include <clangbackendipc_global.h>
#include <highlightingmarkcontainer.h>

#include "cursor.h"

#include <clang-c/Index.h>

namespace ClangBackEnd {

class HighlightingMark
{
    friend bool operator==(const HighlightingMark &first, const HighlightingMark &second);
    friend void PrintTo(const HighlightingMark& highlightingMark, ::std::ostream *os);

    enum class Recursion {
        FirstPass,
        RecursivePass
    };

public:
    HighlightingMark(const CXCursor &cxCursor,
                     CXToken *cxToken,
                     CXTranslationUnit cxTranslationUnit,
                     std::vector<CXSourceRange> &currentOutputArgumentRanges);
    HighlightingMark(uint line, uint column, uint length, HighlightingTypes types);
    HighlightingMark(uint line, uint column, uint length, HighlightingType type);

    bool hasInvalidMainType() const;
    bool hasMainType(HighlightingType type) const;
    bool hasMixinType(HighlightingType type) const;
    bool hasOnlyType(HighlightingType type) const;
    bool hasFunctionArguments() const;

    operator HighlightingMarkContainer() const;

private:
    void identifierKind(const Cursor &cursor, Recursion recursion);
    void referencedTypeKind(const Cursor &cursor);
    void variableKind(const Cursor &cursor);
    void fieldKind(const Cursor &cursor);
    bool isVirtualMethodDeclarationOrDefinition(const Cursor &cursor) const;
    void functionKind(const Cursor &cursor, Recursion recursion);
    void memberReferenceKind(const Cursor &cursor);
    HighlightingType punctuationKind(const Cursor &cursor);
    void collectKinds(CXToken *cxToken, const Cursor &cursor);
    bool isRealDynamicCall(const Cursor &cursor) const;
    void addExtraTypeIfFirstPass(HighlightingType type, Recursion recursion);
    bool isOutputArgument() const;
    void collectOutputArguments(const Cursor &cursor);
    void filterOutPreviousOutputArguments();
    bool isArgumentInCurrentOutputArgumentLocations() const;

private:
    std::vector<CXSourceRange> *currentOutputArgumentRanges = nullptr;
    Cursor originalCursor;
    uint line;
    uint column;
    uint length;
    uint offset = 0;
    HighlightingTypes types;
};

void PrintTo(const HighlightingMark& highlightingMark, ::std::ostream *os);

inline bool operator==(const HighlightingMark &first, const HighlightingMark &second)
{
    return first.line == second.line
        && first.column == second.column
        && first.length == second.length
        && first.types == second.types;
}

} // namespace ClangBackEnd
