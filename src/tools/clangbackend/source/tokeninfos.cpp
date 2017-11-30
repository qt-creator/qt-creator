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

#include "tokeninfos.h"

#include "tokeninfocontainer.h"

#include <QVector>

namespace ClangBackEnd {

TokenInfos::TokenInfos(CXTranslationUnit cxTranslationUnit, CXToken *tokens, uint tokensCount)
    : cxTranslationUnit(cxTranslationUnit),
      cxToken(tokens),
      cxTokenCount(tokensCount)
{
    cxCursor.resize(tokensCount);
    clang_annotateTokens(cxTranslationUnit, cxToken, cxTokenCount, cxCursor.data());
}

TokenInfos::~TokenInfos()
{
    clang_disposeTokens(cxTranslationUnit, cxToken, cxTokenCount);
}

TokenInfos::const_iterator TokenInfos::begin() const
{
    return const_iterator(cxCursor.cbegin(),
                          cxToken,
                          cxTranslationUnit,
                          currentOutputArgumentRanges);
}

TokenInfos::const_iterator TokenInfos::end() const
{
    return const_iterator(cxCursor.cend(),
                          cxToken + cxTokenCount,
                          cxTranslationUnit,
                          currentOutputArgumentRanges);
}

QVector<TokenInfoContainer> TokenInfos::toTokenInfoContainers() const
{
    QVector<TokenInfoContainer> containers;
    containers.reserve(size());

    const auto isValidTokenInfo = [] (const TokenInfo &tokenInfo) {
        return !tokenInfo.hasInvalidMainType()
                && !tokenInfo.hasMainType(HighlightingType::NumberLiteral)
                && !tokenInfo.hasMainType(HighlightingType::Comment);
    };
    for (const TokenInfo &tokenInfo : *this)
        if (isValidTokenInfo(tokenInfo))
            containers.push_back(tokenInfo);

    return containers;
}

bool TokenInfos::currentOutputArgumentRangesAreEmpty() const
{
    return currentOutputArgumentRanges.empty();
}

bool TokenInfos::isEmpty() const
{
    return cxTokenCount == 0;
}

bool ClangBackEnd::TokenInfos::isNull() const
{
    return cxToken == nullptr;
}

uint TokenInfos::size() const
{
    return cxTokenCount;
}

TokenInfo TokenInfos::operator[](size_t index) const
{
    return TokenInfo(cxCursor[index],
                            cxToken + index,
                            cxTranslationUnit,
                            currentOutputArgumentRanges);
}

} // namespace ClangBackEnd
