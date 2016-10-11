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

#include "highlightingmarks.h"

#include "highlightingmarkcontainer.h"

#include <QVector>

namespace ClangBackEnd {

HighlightingMarks::HighlightingMarks(CXTranslationUnit cxTranslationUnit, CXToken *tokens, uint tokensCount)
    : cxTranslationUnit(cxTranslationUnit),
      cxToken(tokens),
      cxTokenCount(tokensCount)
{
    cxCursor.resize(tokensCount);
    clang_annotateTokens(cxTranslationUnit, cxToken, cxTokenCount, cxCursor.data());
}

HighlightingMarks::~HighlightingMarks()
{
    clang_disposeTokens(cxTranslationUnit, cxToken, cxTokenCount);
}

HighlightingMarks::const_iterator HighlightingMarks::begin() const
{
    return const_iterator(cxCursor.cbegin(),
                          cxToken,
                          cxTranslationUnit,
                          currentOutputArgumentRanges);
}

HighlightingMarks::const_iterator HighlightingMarks::end() const
{
    return const_iterator(cxCursor.cend(),
                          cxToken + cxTokenCount,
                          cxTranslationUnit,
                          currentOutputArgumentRanges);
}

QVector<HighlightingMarkContainer> HighlightingMarks::toHighlightingMarksContainers() const
{
    QVector<HighlightingMarkContainer> containers;
    containers.reserve(size());

    const auto isValidHighlightMark = [] (const HighlightingMark &highlightMark) {
        return !highlightMark.hasInvalidMainType()
             && !highlightMark.hasMainType(HighlightingType::StringLiteral)
             && !highlightMark.hasMainType(HighlightingType::NumberLiteral)
             && !highlightMark.hasMainType(HighlightingType::Comment);
    };

    for (const HighlightingMark &highlightMark : *this) {
        if (isValidHighlightMark(highlightMark))
            containers.push_back(highlightMark);
    }

    return containers;
}

bool HighlightingMarks::currentOutputArgumentRangesAreEmpty() const
{
    return currentOutputArgumentRanges.empty();
}

bool HighlightingMarks::isEmpty() const
{
    return cxTokenCount == 0;
}

bool ClangBackEnd::HighlightingMarks::isNull() const
{
    return cxToken == nullptr;
}

uint HighlightingMarks::size() const
{
    return cxTokenCount;
}

HighlightingMark HighlightingMarks::operator[](size_t index) const
{
    return HighlightingMark(cxCursor[index],
                            cxToken + index,
                            cxTranslationUnit,
                            currentOutputArgumentRanges);
}

} // namespace ClangBackEnd
