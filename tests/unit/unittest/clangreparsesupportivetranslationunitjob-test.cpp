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

#include "clangasyncjob-base.h"

#include <clangreparsesupportivetranslationunitjob.h>
#include <clangtranslationunits.h>

using namespace ClangBackEnd;

using testing::Eq;
using testing::Not;
using testing::_;

namespace {

class ReparseSupportiveTranslationUnitJob : public ClangAsyncJobTest
{
protected:
    void SetUp() override { BaseSetUp(JobRequest::Type::ReparseSupportiveTranslationUnit, job); }

    TimePoint parseTimePointOfDocument();
    void parse();

protected:
    ClangBackEnd::ReparseSupportiveTranslationUnitJob job;
};

using ReparseSupportiveTranslationUnitJobSlowTest = ReparseSupportiveTranslationUnitJob;

TEST_F(ReparseSupportiveTranslationUnitJob, PrepareAsyncRun)
{
    job.setContext(jobContext);

    ASSERT_TRUE(job.prepareAsyncRun());
}

TEST_F(ReparseSupportiveTranslationUnitJobSlowTest, RunAsync)
{
    parse();
    job.setContext(jobContext);
    job.prepareAsyncRun();

    job.runAsync();

    ASSERT_TRUE(waitUntilJobFinished(job));
}

TEST_F(ReparseSupportiveTranslationUnitJobSlowTest, IncorporateUpdaterResult)
{
    parse();
    const TimePoint parseTimePointBefore = parseTimePointOfDocument();
    job.setContext(jobContext);
    job.prepareAsyncRun();

    job.runAsync();

    ASSERT_TRUE(waitUntilJobFinished(job));
    ASSERT_THAT(parseTimePointOfDocument(), Not(Eq(parseTimePointBefore)));
}

TEST_F(ReparseSupportiveTranslationUnitJobSlowTest, DoNotIncorporateUpdaterResultIfDocumentWasClosed)
{
    parse();
    const TimePoint parseTimePointBefore = parseTimePointOfDocument();
    job.setContext(jobContext);
    job.prepareAsyncRun();

    job.runAsync();
    documents.remove({FileContainer{filePath, projectPartId}});

    ASSERT_TRUE(waitUntilJobFinished(job));
    ASSERT_THAT(parseTimePointOfDocument(), Eq(parseTimePointBefore));
}

TimePoint ReparseSupportiveTranslationUnitJob::parseTimePointOfDocument()
{
    const Utf8String translationUnitId = document.translationUnit().id();

    return document.translationUnits().parseTimePoint(translationUnitId);
}

void ReparseSupportiveTranslationUnitJob::parse()
{
    projects.createOrUpdate({ProjectPartContainer{projectPartId, Utf8StringVector()}});
    document.parse();
}

} // anonymous
