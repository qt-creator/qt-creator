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

#include "googletest.h"

#include <clangisdiagnosticrelatedtolocation.h>
#include <diagnosticcontainer.h>

using ClangBackEnd::DiagnosticContainer;
using ClangBackEnd::SourceLocationContainer;
using ClangBackEnd::SourceRangeContainer;
using ClangCodeModel::Internal::isDiagnosticRelatedToLocation;

namespace {

SourceRangeContainer createRange(uint startLine,
                                 uint startColumn,
                                 uint endLine,
                                 uint endColumn)
{
    const SourceLocationContainer startLocation(Utf8String(), startLine, startColumn);
    const SourceLocationContainer endLocation(Utf8String(), endLine, endColumn);

    return SourceRangeContainer(startLocation, endLocation);
}

DiagnosticContainer createDiagnosticWithLocation(uint line, uint column)
{
    const SourceLocationContainer location(Utf8String(), line, column);

    return DiagnosticContainer(Utf8String(),
                               Utf8String(),
                               std::pair<Utf8String, Utf8String>(),
                               ClangBackEnd::DiagnosticSeverity::Error,
                               location,
                               QVector<SourceRangeContainer>(),
                               QVector<ClangBackEnd::FixItContainer>(),
                               QVector<DiagnosticContainer>());
}

DiagnosticContainer createDiagnosticWithRange(uint startLine,
                                              uint startColumn,
                                              uint endLine,
                                              uint endColumn)
{
    const SourceRangeContainer range = createRange(startLine, startColumn, endLine, endColumn);

    return DiagnosticContainer(Utf8String(),
                               Utf8String(),
                               std::pair<Utf8String, Utf8String>(),
                               ClangBackEnd::DiagnosticSeverity::Error,
                               SourceLocationContainer(),
                               {range},
                               QVector<ClangBackEnd::FixItContainer>(),
                               QVector<DiagnosticContainer>());
}

class ClangIsDiagnosticRelatedToLocation : public ::testing::Test
{
public:
    ClangIsDiagnosticRelatedToLocation() {}

protected:
    const QVector<SourceRangeContainer> emptyRanges;
};

TEST_F(ClangIsDiagnosticRelatedToLocation, MatchLocation)
{
    const DiagnosticContainer diagnostic = createDiagnosticWithLocation(5, 5);

    ASSERT_TRUE(isDiagnosticRelatedToLocation(diagnostic, emptyRanges, 5, 5));
}

TEST_F(ClangIsDiagnosticRelatedToLocation, DoNotMatchLocation)
{
    const DiagnosticContainer diagnostic = createDiagnosticWithLocation(5, 5);

    ASSERT_FALSE(isDiagnosticRelatedToLocation(diagnostic, emptyRanges, 5, 4));
}

TEST_F(ClangIsDiagnosticRelatedToLocation, MatchStartRange)
{
    const DiagnosticContainer diagnostic = createDiagnosticWithRange(5, 5, 5, 10);

    ASSERT_TRUE(isDiagnosticRelatedToLocation(diagnostic, emptyRanges, 5, 5));
}

TEST_F(ClangIsDiagnosticRelatedToLocation, MatchWithinRange)
{
    const DiagnosticContainer diagnostic = createDiagnosticWithRange(5, 5, 5, 10);

    ASSERT_TRUE(isDiagnosticRelatedToLocation(diagnostic, emptyRanges, 5, 7));
}

TEST_F(ClangIsDiagnosticRelatedToLocation, MatchEndRange)
{
    const DiagnosticContainer diagnostic = createDiagnosticWithRange(5, 5, 5, 10);

    ASSERT_TRUE(isDiagnosticRelatedToLocation(diagnostic, emptyRanges, 5, 10));
}

TEST_F(ClangIsDiagnosticRelatedToLocation, DoNoMatchBeforeRangeStart)
{
    const DiagnosticContainer diagnostic = createDiagnosticWithRange(5, 5, 5, 10);

    ASSERT_FALSE(isDiagnosticRelatedToLocation(diagnostic, emptyRanges, 5, 4));
}

TEST_F(ClangIsDiagnosticRelatedToLocation, DoNoMatchAfterRangeEnd)
{
    const DiagnosticContainer diagnostic = createDiagnosticWithRange(5, 5, 5, 10);

    ASSERT_FALSE(isDiagnosticRelatedToLocation(diagnostic, emptyRanges, 5, 11));
}

TEST_F(ClangIsDiagnosticRelatedToLocation, MatchInAdditionalRange)
{
    const QVector<SourceRangeContainer> additionalRanges{ createRange(5, 5, 5, 10) };
    const DiagnosticContainer diagnostic = createDiagnosticWithLocation(1, 1);

    ASSERT_TRUE(isDiagnosticRelatedToLocation(diagnostic, additionalRanges, 5, 7));
}

} // anonymous
