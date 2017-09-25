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

#include "diagnostic.h"

#include "clangstring.h"
#include "diagnosticset.h"
#include "fixit.h"
#include "sourcelocation.h"
#include "sourcerange.h"

#include <diagnosticcontainer.h>
#include <memory>

namespace ClangBackEnd {

Diagnostic::Diagnostic(CXDiagnostic cxDiagnostic)
    : cxDiagnostic(cxDiagnostic)
{
}

Diagnostic::~Diagnostic()
{
    clang_disposeDiagnostic(cxDiagnostic);
}

Diagnostic::Diagnostic(Diagnostic &&other)
    : cxDiagnostic(std::move(other.cxDiagnostic))
{
    other.cxDiagnostic = nullptr;
}

Diagnostic &Diagnostic::operator=(Diagnostic &&other)
{
    if (this != &other) {
        clang_disposeDiagnostic(cxDiagnostic);
        cxDiagnostic = std::move(other.cxDiagnostic);
        other.cxDiagnostic = nullptr;
    }

    return *this;
}

bool Diagnostic::isNull() const
{
    return cxDiagnostic == nullptr;
}

Utf8String Diagnostic::text() const
{
    return ClangString(clang_formatDiagnostic(cxDiagnostic, 0));
}

Utf8String Diagnostic::category() const
{
    return ClangString(clang_getDiagnosticCategoryText(cxDiagnostic));
}

std::pair<Utf8String, Utf8String> Diagnostic::options() const
{
    CXString disableString;

    const Utf8String enableOption = ClangString(clang_getDiagnosticOption(cxDiagnostic, &disableString));
    const Utf8String disableOption = ClangString(disableString);

    return {enableOption, disableOption};
}

SourceLocation Diagnostic::location() const
{
    return SourceLocation(clang_getDiagnosticLocation(cxDiagnostic));
}

DiagnosticSeverity Diagnostic::severity() const
{
    return static_cast<DiagnosticSeverity>(clang_getDiagnosticSeverity(cxDiagnostic));
}

std::vector<SourceRange> Diagnostic::ranges() const
{
    std::vector<SourceRange> ranges;
    const uint rangesCount = clang_getDiagnosticNumRanges(cxDiagnostic);
    ranges.reserve(rangesCount);

    for (uint index = 0; index < rangesCount; ++index) {
        const SourceRange sourceRange(clang_getDiagnosticRange(cxDiagnostic, index));

        if (sourceRange.isValid())
            ranges.push_back(std::move(sourceRange));
    }

    return ranges;
}

std::vector<FixIt> Diagnostic::fixIts() const
{
    std::vector<FixIt> fixIts;

    const uint fixItsCount = clang_getDiagnosticNumFixIts(cxDiagnostic);

    fixIts.reserve(fixItsCount);

    for (uint index = 0; index < fixItsCount; ++index)
        fixIts.push_back(FixIt(cxDiagnostic, index));

    return fixIts;
}

DiagnosticSet Diagnostic::childDiagnostics() const
{
    return DiagnosticSet(clang_getChildDiagnostics(cxDiagnostic));
}

DiagnosticContainer Diagnostic::toDiagnosticContainer() const
{
    return DiagnosticContainer(text(),
                               category(),
                               options(),
                               severity(),
                               location().toSourceLocationContainer(),
                               getSourceRangeContainers(),
                               getFixItContainers(),
                               childDiagnostics().toDiagnosticContainers());
}

QVector<SourceRangeContainer> Diagnostic::getSourceRangeContainers() const
{
    auto rangeVector = ranges();

    QVector<SourceRangeContainer> sourceRangeContainers;
    sourceRangeContainers.reserve(int(rangeVector.size()));

    for (auto &&sourceRange : rangeVector)
        sourceRangeContainers.push_back(sourceRange.toSourceRangeContainer());

    return sourceRangeContainers;
}

QVector<FixItContainer> Diagnostic::getFixItContainers() const
{
    auto fixItVector = fixIts();

    QVector<FixItContainer> fixItContainers;
    fixItContainers.reserve(int(fixItVector.size()));

    for (auto &&fixIt : fixItVector)
        fixItContainers.push_back(fixIt.toFixItContainer());

    return fixItContainers;
}

} // namespace ClangBackEnd

