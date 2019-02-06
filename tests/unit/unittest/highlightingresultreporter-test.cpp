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
#include "testenvironment.h"

#include <chunksreportedmonitor.h>
#include <clangdocument.h>
#include <clangdocuments.h>
#include <cursor.h>
#include <tokeninfocontainer.h>
#include <tokenprocessor.h>
#include <clanghighlightingresultreporter.h>
#include <unsavedfiles.h>

using ClangBackEnd::Cursor;
using ClangBackEnd::TokenProcessor;
using ClangBackEnd::TokenInfoContainer;
using ClangBackEnd::HighlightingType;
using ClangBackEnd::Document;
using ClangBackEnd::Documents;
using ClangBackEnd::UnsavedFiles;
using ClangBackEnd::ChunksReportedMonitor;
using ClangCodeModel::Internal::HighlightingResultReporter;

namespace {

struct Data {
    UnsavedFiles unsavedFiles;
    Documents documents{unsavedFiles};
    Document document{Utf8StringLiteral(TESTDATA_DIR "/highlightingmarks.cpp"),
                      TestEnvironment::addPlatformArguments({Utf8StringLiteral("-std=c++14")}),
                      Utf8StringVector(),
                      documents};
};

class HighlightingResultReporter : public ::testing::Test
{
public:
    static void SetUpTestCase();
    static void TearDownTestCase();

protected:
    static Data *d;
};

QVector<TokenInfoContainer> noTokenInfos()
{
    return QVector<TokenInfoContainer>();
}

QVector<TokenInfoContainer> generateTokenInfos(uint count)
{
    auto container = QVector<TokenInfoContainer>();

    for (uint i = 0; i < count; ++i) {
        const uint line = i + 1;
        ClangBackEnd::HighlightingTypes types;
        types.mainHighlightingType = ClangBackEnd::HighlightingType::Type;
        container.append(TokenInfoContainer(line, 1, 1, types));
    }

    return container;
}

TEST_F(HighlightingResultReporter, StartAndFinish)
{
    auto reporter = new ::HighlightingResultReporter(noTokenInfos());

    auto future = reporter->start();

    future.waitForFinished();
    ASSERT_THAT(future.isFinished(), true);
}

TEST_F(HighlightingResultReporter, ReportNothingIfNothingToReport)
{
    auto reporter = new ::HighlightingResultReporter(generateTokenInfos(0));

    auto future = reporter->start();

    ChunksReportedMonitor monitor(future);
    ASSERT_THAT(monitor.resultsReadyCounter(), 0L);
}

TEST_F(HighlightingResultReporter, ReportSingleResultAsOneChunk)
{
    auto reporter = new ::HighlightingResultReporter(generateTokenInfos(1));
    reporter->setChunkSize(1);

    auto future = reporter->start();

    ChunksReportedMonitor monitor(future);
    ASSERT_THAT(monitor.resultsReadyCounter(), 1L);
}

TEST_F(HighlightingResultReporter, ReportRestIfChunkSizeNotReached)
{
    auto reporter = new ::HighlightingResultReporter(generateTokenInfos(1));
    const int notReachedChunkSize = 100;
    reporter->setChunkSize(notReachedChunkSize);

    auto future = reporter->start();

    ChunksReportedMonitor monitor(future);
    ASSERT_THAT(monitor.resultsReadyCounter(), 1L);
}

TEST_F(HighlightingResultReporter, ReportChunksWithoutRest)
{
    auto reporter = new ::HighlightingResultReporter(generateTokenInfos(4));
    reporter->setChunkSize(1);

    auto future = reporter->start();

    ChunksReportedMonitor monitor(future);
    ASSERT_THAT(monitor.resultsReadyCounter(), 2L);
}

TEST_F(HighlightingResultReporter, ReportSingleChunkAndRest)
{
    auto reporter = new ::HighlightingResultReporter(generateTokenInfos(5));
    reporter->setChunkSize(2);

    auto future = reporter->start();

    ChunksReportedMonitor monitor(future);
    ASSERT_THAT(monitor.resultsReadyCounter(), 2L);
}

TEST_F(HighlightingResultReporter, ReportCompleteLines)
{
    ClangBackEnd::HighlightingTypes types;
    types.mainHighlightingType = ClangBackEnd::HighlightingType::Type;
    QVector<TokenInfoContainer> tokenInfos {
        TokenInfoContainer(1, 1, 1, types),
        TokenInfoContainer(1, 2, 1, types),
        TokenInfoContainer(2, 1, 1, types),
    };
    auto reporter = new ::HighlightingResultReporter(tokenInfos);
    reporter->setChunkSize(1);

    auto future = reporter->start();

    ChunksReportedMonitor monitor(future);
    ASSERT_THAT(monitor.resultsReadyCounter(), 2L);
}

Data *HighlightingResultReporter::d;

void HighlightingResultReporter::SetUpTestCase()
{
    d = new Data;
}

void HighlightingResultReporter::TearDownTestCase()
{
    delete d;
    d = nullptr;
}

} // anonymous
