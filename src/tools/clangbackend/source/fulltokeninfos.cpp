/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "fulltokeninfos.h"

#include <clangsupport/tokeninfocontainer.h>

#include <QVector>

namespace ClangBackEnd {

FullTokenInfos::FullTokenInfos(CXTranslationUnit cxTranslationUnit, CXToken *tokens, uint tokensCount)
    : cxTranslationUnit(cxTranslationUnit),
      cxTokens(tokens),
      cxTokenCount(tokensCount)
{
    cxCursors.resize(tokensCount);
    clang_annotateTokens(cxTranslationUnit, cxTokens, cxTokenCount, cxCursors.data());
}

FullTokenInfos::~FullTokenInfos()
{
    clang_disposeTokens(cxTranslationUnit, cxTokens, cxTokenCount);
}

QVector<TokenInfoContainer> FullTokenInfos::toTokenInfoContainers() const
{
    QVector<TokenInfoContainer> containers;
    containers.reserve(static_cast<int>(size()));

    const auto isValidTokenInfo = [] (const TokenInfo &tokenInfo) {
        // Do not exclude StringLiteral because it can be a filename for an #include
        return !tokenInfo.hasInvalidMainType()
                && !tokenInfo.hasMainType(HighlightingType::NumberLiteral)
                && !tokenInfo.hasMainType(HighlightingType::Comment);
    };
    for (size_t index = 0; index < cxCursors.size(); ++index) {
        FullTokenInfo fullTokenInfo = (*this)[index];
        if (isValidTokenInfo(fullTokenInfo))
            containers.push_back(fullTokenInfo);
    }

    return containers;
}

bool FullTokenInfos::isEmpty() const
{
    return cxTokenCount == 0;
}

bool FullTokenInfos::isNull() const
{
    return cxTokens == nullptr;
}

size_t FullTokenInfos::size() const
{
    return cxTokenCount;
}

FullTokenInfo FullTokenInfos::operator[](size_t index) const
{
    FullTokenInfo tokenInfo(cxCursors[index],
                            cxTokens + index,
                            cxTranslationUnit,
                            currentOutputArgumentRanges);
    tokenInfo.evaluate();
    return tokenInfo;
}

} // namespace ClangBackEnd
