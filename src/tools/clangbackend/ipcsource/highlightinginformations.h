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

#ifndef CLANGBACKEND_HIGHLIGHTINGINFORMATIONS_H
#define CLANGBACKEND_HIGHLIGHTINGINFORMATIONS_H

#include "highlightinginformationsiterator.h"

#include <clang-c/Index.h>

#include <vector>

namespace ClangBackEnd {

using uint = unsigned int;
class HighlightingMarkContainer;

class HighlightingInformations
{
public:
    using const_iterator = HighlightingInformationsIterator;
    using value_type = HighlightingInformation;

public:
    HighlightingInformations() = default;
    HighlightingInformations(CXTranslationUnit cxTranslationUnit, CXToken *tokens, uint tokensCount);
    ~HighlightingInformations();

    bool isEmpty() const;
    bool isNull() const;
    uint size() const;

    HighlightingInformation operator[](size_t index) const;

    const_iterator begin() const;
    const_iterator end() const;

    QVector<HighlightingMarkContainer> toHighlightingMarksContainers() const;

private:
    CXTranslationUnit cxTranslationUnit = nullptr;
    CXToken *const cxToken = nullptr;
    const uint cxTokenCount = 0;

    std::vector<CXCursor> cxCursor;
};

} // namespace ClangBackEnd

#endif // CLANGBACKEND_HIGHLIGHTINGINFORMATIONS_H
