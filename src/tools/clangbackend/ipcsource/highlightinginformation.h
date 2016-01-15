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

#ifndef CLANGBACKEND_HIGHLIGHTINGINFORMATION_H
#define CLANGBACKEND_HIGHLIGHTINGINFORMATION_H

#include <clangbackendipc_global.h>
#include <highlightingmarkcontainer.h>

#include "cursor.h"

#include <clang-c/Index.h>

namespace ClangBackEnd {

class HighlightingInformation
{
    friend bool operator==(const HighlightingInformation &first, const HighlightingInformation &second);
    friend void PrintTo(const HighlightingInformation& highlightingInformation, ::std::ostream *os);

public:
    HighlightingInformation(const CXCursor &cxCursor, CXToken *cxToken, CXTranslationUnit cxTranslationUnit);
    HighlightingInformation(uint line, uint column, uint length, HighlightingType type);

    bool hasType(HighlightingType type) const;
    bool hasFunctionArguments() const;
    QVector<HighlightingInformation> outputFunctionArguments() const;

    operator HighlightingMarkContainer() const;

private:
    HighlightingType identifierKind(const Cursor &cursor) const;
    HighlightingType referencedTypeKind(const Cursor &cursor) const;
    HighlightingType variableKind(const Cursor &cursor) const;
    bool isVirtualMethodDeclarationOrDefinition(const Cursor &cursor) const;
    HighlightingType functionKind(const Cursor &cursor) const;
    HighlightingType memberReferenceKind(const Cursor &cursor) const;
    HighlightingType kind(CXToken *cxToken, const Cursor &cursor) const;
    bool isRealDynamicCall(const Cursor &cursor) const;

private:
    Cursor originalCursor;
    uint line;
    uint column;
    uint length;
    HighlightingType type;
};

void PrintTo(const HighlightingInformation& highlightingInformation, ::std::ostream *os);

inline bool operator==(const HighlightingInformation &first, const HighlightingInformation &second)
{
    return first.line == second.line
        && first.column == second.column
        && first.length == second.length
        && first.type == second.type;
}

} // namespace ClangBackEnd

#endif // CLANGBACKEND_HIGHLIGHTINGINFORMATION_H
