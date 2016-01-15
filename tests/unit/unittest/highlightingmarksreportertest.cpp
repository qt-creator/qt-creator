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

#include <chunksreportedmonitor.h>
#include <clangtranslationunit.h>
#include <highlightingmarkcontainer.h>
#include <clanghighlightingmarksreporter.h>
#include <projectpart.h>
#include <projects.h>
#include <translationunits.h>
#include <unsavedfiles.h>

#include <gmock/gmock.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>
#include "gtest-qt-printing.h"

using ClangBackEnd::Cursor;
using ClangBackEnd::HighlightingInformations;
using ClangBackEnd::HighlightingMarkContainer;
using ClangBackEnd::HighlightingType;
using ClangBackEnd::TranslationUnit;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::ProjectPart;
using ClangBackEnd::ProjectParts;
using ClangBackEnd::TranslationUnits;
using ClangBackEnd::ChunksReportedMonitor;

namespace {

struct Data {
    ProjectParts projects;
    UnsavedFiles unsavedFiles;
    TranslationUnits translationUnits{projects, unsavedFiles};
    TranslationUnit translationUnit{Utf8StringLiteral(TESTDATA_DIR"/highlightinginformations.cpp"),
                                    ProjectPart(Utf8StringLiteral("projectPartId"),
                                                {Utf8StringLiteral("-std=c++14")}),
                                    {},
                                    translationUnits};
};

class HighlightingMarksReporter : public ::testing::Test
{
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

protected:
    static Data *d;
};

QVector<HighlightingMarkContainer> noHighlightingMarks()
{
    return QVector<HighlightingMarkContainer>();
}

QVector<HighlightingMarkContainer> generateHighlightingMarks(uint count)
{
    auto container = QVector<HighlightingMarkContainer>();

    for (uint i = 0; i < count; ++i) {
        const uint line = i + 1;
        container.append(HighlightingMarkContainer(line, 1, 1, HighlightingType::Type));
    }

    return container;
}

TEST_F(HighlightingMarksReporter, StartAndFinish)
{
    auto reporter = new ClangCodeModel::HighlightingMarksReporter(noHighlightingMarks());

    auto future = reporter->start();

    future.waitForFinished();
    ASSERT_THAT(future.isFinished(), true);
}

TEST_F(HighlightingMarksReporter, ReportNothingIfNothingToReport)
{
    auto reporter = new ClangCodeModel::HighlightingMarksReporter(generateHighlightingMarks(0));

    auto future = reporter->start();

    ChunksReportedMonitor monitor(future);
    ASSERT_THAT(monitor.resultsReadyCounter(), 0L);
}

TEST_F(HighlightingMarksReporter, ReportSingleResultAsOneChunk)
{
    auto reporter = new ClangCodeModel::HighlightingMarksReporter(generateHighlightingMarks(1));
    reporter->setChunkSize(1);

    auto future = reporter->start();

    ChunksReportedMonitor monitor(future);
    ASSERT_THAT(monitor.resultsReadyCounter(), 1L);
}

TEST_F(HighlightingMarksReporter, ReportRestIfChunkSizeNotReached)
{
    auto reporter = new ClangCodeModel::HighlightingMarksReporter(generateHighlightingMarks(1));
    const int notReachedChunkSize = 100;
    reporter->setChunkSize(notReachedChunkSize);

    auto future = reporter->start();

    ChunksReportedMonitor monitor(future);
    ASSERT_THAT(monitor.resultsReadyCounter(), 1L);
}

TEST_F(HighlightingMarksReporter, ReportChunksWithoutRest)
{
    auto reporter = new ClangCodeModel::HighlightingMarksReporter(generateHighlightingMarks(4));
    reporter->setChunkSize(1);

    auto future = reporter->start();

    ChunksReportedMonitor monitor(future);
    ASSERT_THAT(monitor.resultsReadyCounter(), 2L);
}

TEST_F(HighlightingMarksReporter, ReportSingleChunkAndRest)
{
    auto reporter = new ClangCodeModel::HighlightingMarksReporter(generateHighlightingMarks(5));
    reporter->setChunkSize(2);

    auto future = reporter->start();

    ChunksReportedMonitor monitor(future);
    ASSERT_THAT(monitor.resultsReadyCounter(), 2L);
}

TEST_F(HighlightingMarksReporter, ReportCompleteLines)
{
    QVector<HighlightingMarkContainer> highlightingMarks {
        HighlightingMarkContainer(1, 1, 1, HighlightingType::Type),
        HighlightingMarkContainer(1, 2, 1, HighlightingType::Type),
        HighlightingMarkContainer(2, 1, 1, HighlightingType::Type),
    };
    auto reporter = new ClangCodeModel::HighlightingMarksReporter(highlightingMarks);
    reporter->setChunkSize(1);

    auto future = reporter->start();

    ChunksReportedMonitor monitor(future);
    ASSERT_THAT(monitor.resultsReadyCounter(), 2L);
}

Data *HighlightingMarksReporter::d;

void HighlightingMarksReporter::SetUpTestCase()
{
    d = new Data;
}

void HighlightingMarksReporter::TearDownTestCase()
{
    delete d;
    d = nullptr;
}

} // anonymous
