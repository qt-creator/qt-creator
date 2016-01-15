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

#ifndef HIGHLIGHTINGINFORMATIONSITERATOR_H
#define HIGHLIGHTINGINFORMATIONSITERATOR_H

#include "highlightinginformation.h"

#include <iterator>
#include <vector>

#include <clang-c/Index.h>

namespace ClangBackEnd {

using uint = unsigned int;

class DiagnosticSet;
class Diagnostic;

class HighlightingInformationsIterator : public std::iterator<std::forward_iterator_tag, HighlightingInformation, uint>
{
public:
    HighlightingInformationsIterator(std::vector<CXCursor>::const_iterator cxCursorIterator,
                                     CXToken *cxToken,
                                     CXTranslationUnit cxTranslationUnit)
        : cxCursorIterator(cxCursorIterator),
          cxToken(cxToken),
          cxTranslationUnit(cxTranslationUnit)
    {}

    HighlightingInformationsIterator& operator++()
    {
        ++cxCursorIterator;
        ++cxToken;

        return *this;
    }

    HighlightingInformationsIterator operator++(int)
    {
        return HighlightingInformationsIterator(cxCursorIterator++, cxToken++, cxTranslationUnit);
    }

    bool operator==(HighlightingInformationsIterator other) const
    {
        return cxCursorIterator == other.cxCursorIterator;
    }

    bool operator!=(HighlightingInformationsIterator other) const
    {
        return cxCursorIterator != other.cxCursorIterator;
    }

    HighlightingInformation operator*()
    {
        return HighlightingInformation(*cxCursorIterator, cxToken, cxTranslationUnit);
    }

private:
    std::vector<CXCursor>::const_iterator cxCursorIterator;
    CXToken *cxToken;
    CXTranslationUnit cxTranslationUnit;
};

} // namespace ClangBackEnd

#endif // HIGHLIGHTINGINFORMATIONSITERATOR_H
