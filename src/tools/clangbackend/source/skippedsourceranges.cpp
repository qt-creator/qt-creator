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

#include "skippedsourceranges.h"

#include "sourcerangecontainer.h"

#include <QVector>

#include <algorithm>

namespace ClangBackEnd {

SkippedSourceRanges::SkippedSourceRanges(CXTranslationUnit cxTranslationUnit, const char *filePath)
    : cxTranslationUnit(cxTranslationUnit)
    , cxSkippedSourceRanges(clang_getSkippedRanges(cxTranslationUnit,
                                                   clang_getFile(cxTranslationUnit, filePath)))
{
}

SkippedSourceRanges::~SkippedSourceRanges()
{
    clang_disposeSourceRangeList(cxSkippedSourceRanges);
}

SkippedSourceRanges &SkippedSourceRanges::operator=(SkippedSourceRanges &&other)
{
    if (this != &other) {
        this->~SkippedSourceRanges();
        cxTranslationUnit = other.cxTranslationUnit;
        cxSkippedSourceRanges = other.cxSkippedSourceRanges;
        other.cxTranslationUnit = nullptr;
        other.cxSkippedSourceRanges = nullptr;
    }

    return *this;
}

// For some reason, libclang starts the skipped range on the line containing the
// preprocessor directive preceding the ifdef'ed out code (i.e. #if or #else)
// and ends it on the line following the ifdef'ed out code (#else or #endif, respectively).
// We don't want the preprocessor directives grayed out, so adapt the locations.
static SourceRange adaptedSourceRange(CXTranslationUnit cxTranslationUnit, const SourceRange &range)
{
    const SourceLocation end = range.end();

    return SourceRange {
        SourceLocation(cxTranslationUnit,
                       clang_getLocation(cxTranslationUnit,
                                         clang_getFile(cxTranslationUnit,
                                                       end.filePath().constData()),
                                         range.start().line() + 1, 1)),
        SourceLocation(cxTranslationUnit,
                       clang_getLocation(cxTranslationUnit,
                                         clang_getFile(cxTranslationUnit,
                                                       end.filePath().constData()),
                                         end.line(), 1))
    };
}

// TODO: This should report a line range.
std::vector<SourceRange> SkippedSourceRanges::sourceRanges() const
{
    std::vector<SourceRange> sourceRanges;

    auto sourceRangeCount = cxSkippedSourceRanges->count;
    sourceRanges.reserve(sourceRangeCount);

    for (uint i = 0; i < cxSkippedSourceRanges->count; ++i) {
        const SourceRange range {cxTranslationUnit, cxSkippedSourceRanges->ranges[i]};
        const SourceRange adaptedRange = adaptedSourceRange(cxTranslationUnit, range);

        sourceRanges.push_back(adaptedRange);
    }

    return sourceRanges;
}

QVector<SourceRangeContainer> SkippedSourceRanges::toSourceRangeContainers() const
{
    QVector<SourceRangeContainer> sourceRangeContainers;

    auto sourceRanges = this->sourceRanges();

    std::copy(sourceRanges.cbegin(),
              sourceRanges.cend(),
              std::back_inserter(sourceRangeContainers));

    return sourceRangeContainers;
}

bool SkippedSourceRanges::isNull() const
{

    return cxTranslationUnit == nullptr || cxSkippedSourceRanges == nullptr;
}

ClangBackEnd::SkippedSourceRanges::operator QVector<SourceRangeContainer>() const
{
    return toSourceRangeContainers();
}

SkippedSourceRanges::SkippedSourceRanges(SkippedSourceRanges &&other)
    : cxTranslationUnit(other.cxTranslationUnit)
    , cxSkippedSourceRanges(other.cxSkippedSourceRanges)
{
    other.cxTranslationUnit = nullptr;
    other.cxSkippedSourceRanges = nullptr;
}

} // namespace ClangBackEnd

