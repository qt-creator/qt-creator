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
    TokenProcessor(CXTranslationUnit cxTranslationUnit, const SourceRange &range)
        : cxTranslationUnit(cxTranslationUnit)
    {
        unsigned cxTokensCount = 0;
        clang_tokenize(cxTranslationUnit, range, &cxTokens, &cxTokensCount);
        cxCursors.resize(cxTokensCount);
        clang_annotateTokens(cxTranslationUnit, cxTokens, cxTokensCount, cxCursors.data());
    }
    ~TokenProcessor()
    {
        clang_disposeTokens(cxTranslationUnit, cxTokens, unsigned(cxCursors.size()));
    }

    bool isEmpty() const
    {
        return cxCursors.empty();
    }
    bool isNull() const
    {
        return cxTokens == nullptr;
    }
    size_t size() const
    {
        return cxCursors.size();
    }

    const_iterator begin() const
    {
        return const_iterator(cxCursors.cbegin(),
                              cxTokens,
                              cxTranslationUnit,
                              currentOutputArgumentRanges);
    }

    const_iterator end() const
    {
        return const_iterator(cxCursors.cend(),
                              cxTokens + cxCursors.size(),
                              cxTranslationUnit,
                              currentOutputArgumentRanges);
    }


    T operator[](size_t index) const
    {
        T tokenInfo(cxCursors[index], cxTokens + index, cxTranslationUnit,
                    currentOutputArgumentRanges);
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
        QVector<TC> tokens;
        tokens.reserve(int(size()));

        const auto isValidTokenInfo = [](const T &tokenInfo) {
            return !tokenInfo.hasInvalidMainType()
                    && !tokenInfo.hasMainType(HighlightingType::NumberLiteral)
                    && !tokenInfo.hasMainType(HighlightingType::Comment);
        };

        for (size_t index = 0; index < cxCursors.size(); ++index) {
            T tokenInfo = (*this)[index];
            if (isValidTokenInfo(tokenInfo))
                tokens.push_back(tokenInfo);
        }

        return tokens;
    }

    mutable std::vector<CXSourceRange> currentOutputArgumentRanges;
    CXTranslationUnit cxTranslationUnit = nullptr;
    CXToken *cxTokens = nullptr;

    std::vector<CXCursor> cxCursors;
};

template <>
inline
QVector<TokenInfoContainer> TokenProcessor<FullTokenInfo>::toTokenInfoContainers() const
{
    QVector<FullTokenInfo> tokens = toTokens<FullTokenInfo>();

    return Utils::transform(tokens,
                            [&tokens](FullTokenInfo &token) -> TokenInfoContainer {
        if (!token.m_extraInfo.declaration || token.hasMainType(HighlightingType::LocalVariable))
            return token;

        const int index = tokens.indexOf(token);
        const SourceLocationContainer &tokenStart = token.m_extraInfo.cursorRange.start;
        for (auto it = tokens.rend() - index; it != tokens.rend(); ++it) {
            if (it->m_extraInfo.declaration && !it->hasMainType(HighlightingType::LocalVariable)
                    && it->m_originalCursor != token.m_originalCursor
                    && it->m_extraInfo.cursorRange.contains(tokenStart)) {
                if (token.m_originalCursor.lexicalParent() != it->m_originalCursor
                        && !token.hasMainType(HighlightingType::QtProperty)) {
                    continue;
                }
                token.m_extraInfo.lexicalParentIndex = std::distance(it, tokens.rend()) - 1;
                break;
            }
        }
        return token;
    });
}

} // namespace ClangBackEnd
