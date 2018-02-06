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

#include "sourcerange.h"
#include "tokenprocessoriterator.h"
#include "tokeninfocontainer.h"

#include <clang-c/Index.h>

#include <QVector>

#include <vector>

namespace ClangBackEnd {

using uint = unsigned int;

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
        uint cxTokensCount = 0;
        clang_tokenize(cxTranslationUnit, range, &cxTokens, &cxTokensCount);
        cxCursors.resize(cxTokensCount);
        clang_annotateTokens(cxTranslationUnit, cxTokens, cxTokensCount, cxCursors.data());
    }
    ~TokenProcessor()
    {
        clang_disposeTokens(cxTranslationUnit, cxTokens, cxCursors.size());
    }

    bool isEmpty() const
    {
        return cxCursors.empty();
    }
    bool isNull() const
    {
        return cxTokens == nullptr;
    }
    uint size() const
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
        QVector<TokenInfoContainer> containers;
        containers.reserve(size());

        const auto isValidTokenInfo = [] (const T &tokenInfo) {
            return !tokenInfo.hasInvalidMainType()
                    && !tokenInfo.hasMainType(HighlightingType::NumberLiteral)
                    && !tokenInfo.hasMainType(HighlightingType::Comment);
        };
        for (size_t index = 0; index < cxCursors.size(); ++index) {
            T tokenInfo = (*this)[index];
            if (isValidTokenInfo(tokenInfo))
                containers.push_back(tokenInfo);
        }

        return containers;
    }

    bool currentOutputArgumentRangesAreEmpty() const
    {
        return currentOutputArgumentRanges.empty();
    }

private:
    mutable std::vector<CXSourceRange> currentOutputArgumentRanges;
    CXTranslationUnit cxTranslationUnit = nullptr;
    CXToken *cxTokens = nullptr;

    std::vector<CXCursor> cxCursors;
};

} // namespace ClangBackEnd
