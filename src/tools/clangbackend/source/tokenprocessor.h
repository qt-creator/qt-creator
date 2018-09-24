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

#include "fulltokeninfo.h"
#include "sourcerange.h"
#include "token.h"
#include "tokenprocessoriterator.h"
#include "tokeninfocontainer.h"

#include <utils/algorithm.h>

#include <clang-c/Index.h>

#include <QVector>

#include <vector>

namespace ClangBackEnd {

template<class T>
class TokenProcessor
{
    static_assert (std::is_base_of<TokenInfo, T>::value,
                   "Use TokenProcessor only with classes derived from TokenInfo");

public:
    using const_iterator = TokenProcessorIterator<T>;
    using value_type = T;

public:
    TokenProcessor() = default;
    TokenProcessor(const SourceRange &range)
        : tokens(range)
        , cursors(tokens.annotate())
    {
    }

    bool isEmpty() const
    {
        return cursors.empty();
    }
    bool isNull() const
    {
        return !tokens.size();
    }
    size_t size() const
    {
        return cursors.size();
    }

    const_iterator begin() const
    {
        return const_iterator(cursors.cbegin(),
                              tokens.cbegin(),
                              currentOutputArgumentRanges);
    }

    const_iterator end() const
    {
        return const_iterator(cursors.cend(),
                              tokens.cend(),
                              currentOutputArgumentRanges);
    }


    T operator[](size_t index) const
    {
        T tokenInfo(cursors[index], &tokens[index], currentOutputArgumentRanges);
        tokenInfo.evaluate();
        return tokenInfo;
    }

    QVector<TokenInfoContainer> toTokenInfoContainers() const
    {
        return toTokens<TokenInfoContainer>();
    }

    bool currentOutputArgumentRangesAreEmpty() const
    {
        return currentOutputArgumentRanges.empty();
    }

private:
    template<class TC>
    QVector<TC> toTokens() const
    {
        QVector<TC> tokenInfos;
        tokenInfos.reserve(int(size()));

        const auto isValidTokenInfo = [](const T &tokenInfo) {
            return !tokenInfo.hasInvalidMainType()
                    && !tokenInfo.hasMainType(HighlightingType::NumberLiteral)
                    && !tokenInfo.hasMainType(HighlightingType::Comment);
        };

        for (size_t index = 0; index < cursors.size(); ++index) {
            T tokenInfo = (*this)[index];
            if (isValidTokenInfo(tokenInfo))
                tokenInfos.push_back(tokenInfo);
        }

        return tokenInfos;
    }

    mutable std::vector<CXSourceRange> currentOutputArgumentRanges;
    Tokens tokens;
    std::vector<Cursor> cursors;
};

template <>
inline
QVector<TokenInfoContainer> TokenProcessor<FullTokenInfo>::toTokenInfoContainers() const
{
    QVector<FullTokenInfo> tokenInfos = toTokens<FullTokenInfo>();

    return Utils::transform(tokenInfos,
                            [&tokenInfos](FullTokenInfo &tokenInfo) -> TokenInfoContainer {
        if (!tokenInfo.m_extraInfo.declaration
                || tokenInfo.hasMainType(HighlightingType::LocalVariable)) {
            return tokenInfo;
        }

        const int index = tokenInfos.indexOf(tokenInfo);
        const SourceLocationContainer &tokenStart = tokenInfo.m_extraInfo.cursorRange.start;
        for (auto it = tokenInfos.rend() - index; it != tokenInfos.rend(); ++it) {
            if (it->m_extraInfo.declaration && !it->hasMainType(HighlightingType::LocalVariable)
                    && it->m_originalCursor != tokenInfo.m_originalCursor
                    && it->m_extraInfo.cursorRange.contains(tokenStart)) {
                if (tokenInfo.m_originalCursor.lexicalParent() != it->m_originalCursor
                        && !tokenInfo.hasMainType(HighlightingType::QtProperty)) {
                    continue;
                }
                tokenInfo.m_extraInfo.lexicalParentIndex = std::distance(it, tokenInfos.rend()) - 1;
                break;
            }
        }
        return tokenInfo;
    });
}

} // namespace ClangBackEnd
