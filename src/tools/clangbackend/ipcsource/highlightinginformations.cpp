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

#include "highlightinginformations.h"

#include "highlightingmarkcontainer.h"

#include <QVector>

namespace ClangBackEnd {

HighlightingInformations::HighlightingInformations(CXTranslationUnit cxTranslationUnit, CXToken *tokens, uint tokensCount)
    : cxTranslationUnit(cxTranslationUnit),
      cxToken(tokens),
      cxTokenCount(tokensCount)
{
    cxCursor.resize(tokensCount);
    clang_annotateTokens(cxTranslationUnit, cxToken, cxTokenCount, cxCursor.data());
}

HighlightingInformations::~HighlightingInformations()
{
    clang_disposeTokens(cxTranslationUnit, cxToken, cxTokenCount);
}

HighlightingInformations::const_iterator HighlightingInformations::begin() const
{
    return const_iterator(cxCursor.cbegin(), cxToken, cxTranslationUnit);
}

HighlightingInformations::const_iterator HighlightingInformations::end() const
{
    return const_iterator(cxCursor.cend(), cxToken + cxTokenCount, cxTranslationUnit);
}

QVector<HighlightingMarkContainer> HighlightingInformations::toHighlightingMarksContainers() const
{
    QVector<HighlightingMarkContainer> containers;
    containers.reserve(size());

    const auto isValidHighlightMark = [] (const HighlightingInformation &highlightMark) {
        return !highlightMark.hasType(HighlightingType::Invalid);
    };

    std::copy_if(begin(), end(), std::back_inserter(containers), isValidHighlightMark);

    return containers;
}

bool HighlightingInformations::isEmpty() const
{
    return cxTokenCount == 0;
}

bool ClangBackEnd::HighlightingInformations::isNull() const
{
    return cxToken == nullptr;
}

uint HighlightingInformations::size() const
{
    return cxTokenCount;
}

HighlightingInformation HighlightingInformations::operator[](size_t index) const
{
    return HighlightingInformation(cxCursor[index], cxToken + index, cxTranslationUnit);
}

} // namespace ClangBackEnd
