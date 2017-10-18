/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "filesystem-utilities.h"

#include "sourcerangecontainer-matcher.h"

#include <filecontainerv2.h>
#include <filepathcaching.h>
#include <refactoringdatabaseinitializer.h>

#include <sqlitedatabase.h>

#include <clangquerygatherer.h>

#include <QDir>

namespace {

using testing::AllOf;
using testing::AtLeast;
using testing::AtMost;
using testing::Contains;
using testing::Each;
using testing::ElementsAre;
using testing::Eq;
using testing::Ge;
using testing::IsEmpty;
using testing::Le;
using testing::NiceMock;
using testing::Pair;
using testing::PrintToString;
using testing::Property;
using testing::SizeIs;
using testing::UnorderedElementsAre;
using testing::_;

using ClangBackEnd::V2::FileContainer;
using ClangBackEnd::SourceRangesForQueryMessage;
using ClangBackEnd::SourceRangesContainer;
using ClangBackEnd::SourceRangesContainer;

MATCHER_P2(Contains, line, column,
           std::string(negation ? "isn't " : "is ")
           + "(" + PrintToString(line)
           + ", " + PrintToString(column)
           + ")"
           )
{
    return arg.line() == uint(line)
        && arg.column() == uint(column);
}

class ClangQueryGatherer : public ::testing::Test
{
protected:
    void SetUp() override;

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> databaseInitializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    Utils::SmallString sourceContent{"#include \"query_simplefunction.h\"\nvoid f() {}"};
    FileContainer source{{TESTDATA_DIR, "query_simplefunction.cpp"},
                         sourceContent.clone(),
                         {"cc", toNativePath(TESTDATA_DIR"/query_simplefunction.cpp"), "-I", TESTDATA_DIR}};
    FileContainer source2{{TESTDATA_DIR, "query_simplefunction2.cpp"},
                          {},
                          {"cc", toNativePath(TESTDATA_DIR"/query_simplefunction2.cpp"), "-I", TESTDATA_DIR}};
    FileContainer source3{{TESTDATA_DIR, "query_simplefunction3.cpp"},
                          {},
                          {"cc", toNativePath(TESTDATA_DIR"/query_simplefunction3.cpp"), "-I", TESTDATA_DIR}};
    Utils::SmallString unsavedContent{"void f();"};
    FileContainer unsaved{{TESTDATA_DIR, "query_simplefunction.h"},
                          unsavedContent.clone(),
                          {}};
    Utils::SmallString query{"functionDecl()"};
    ClangBackEnd::ClangQueryGatherer gatherer{&filePathCache, {source.clone()}, {unsaved.clone()}, query.clone()};
    ClangBackEnd::ClangQueryGatherer manyGatherer{&filePathCache,
                                                  {source3.clone(), source2.clone(), source.clone()},
                                                  {unsaved.clone()},
                                                  query.clone()};
};

TEST_F(ClangQueryGatherer, CreateSourceRanges)
{
    auto sourceRangesAndDiagnostics = gatherer.createSourceRangesForSource(&filePathCache, source.clone(), {unsaved}, query.clone());

    ASSERT_THAT(sourceRangesAndDiagnostics,
                Property(&SourceRangesForQueryMessage::sourceRanges,
                         Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                  Contains(IsSourceRangeWithText(2, 1, 2, 12, "void f() {}")))));
}

TEST_F(ClangQueryGatherer, CreateSourceRangessWithUnsavedContent)
{
    auto sourceRangesAndDiagnostics = gatherer.createSourceRangesForSource(&filePathCache, source.clone(), {unsaved}, query.clone());

    ASSERT_THAT(sourceRangesAndDiagnostics,
                Property(&SourceRangesForQueryMessage::sourceRanges,
                         Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                  Contains(IsSourceRangeWithText(1, 1, 1, 9, "void f();")))));
}

TEST_F(ClangQueryGatherer, CanCreateSourceRangesIfItHasSources)
{
    ASSERT_TRUE(gatherer.canCreateSourceRanges());
}

TEST_F(ClangQueryGatherer, CanNotCreateSourceRangesIfItHasNoSources)
{
    ClangBackEnd::ClangQueryGatherer empthyGatherer{&filePathCache, {}, {unsaved.clone()}, query.clone()};

    ASSERT_FALSE(empthyGatherer.canCreateSourceRanges());
}

TEST_F(ClangQueryGatherer, CreateSourceRangesForNextSource)
{
    auto sourceRangesAndDiagnostics = gatherer.createNextSourceRanges();

    ASSERT_THAT(sourceRangesAndDiagnostics,
                Property(&SourceRangesForQueryMessage::sourceRanges,
                         Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                  Contains(IsSourceRangeWithText(1, 1, 1, 9, "void f();")))));
}

TEST_F(ClangQueryGatherer, CreateSourceRangesForNextSourcePopsSource)
{
    manyGatherer.createNextSourceRanges();

    ASSERT_THAT(manyGatherer.sources(), SizeIs(2));
}

TEST_F(ClangQueryGatherer, StartCreateSourceRangesForNextSource)
{
    auto future = gatherer.startCreateNextSourceRangesMessage();
    future.wait();

    ASSERT_THAT(future.get(),
                Property(&SourceRangesForQueryMessage::sourceRanges,
                         Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                  Contains(IsSourceRangeWithText(1, 1, 1, 9, "void f();")))));
}

TEST_F(ClangQueryGatherer, StartCreateSourceRangesForNextSourcePopsSource)
{
    manyGatherer.startCreateNextSourceRangesMessage();

    ASSERT_THAT(manyGatherer.sources(), SizeIs(2));
}

TEST_F(ClangQueryGatherer, AfterStartCreateSourceRangesMessagesFutureCountIsTwos)
{
    manyGatherer.startCreateNextSourceRangesMessages();

    ASSERT_THAT(manyGatherer.sourceFutures(), SizeIs(2));
}

TEST_F(ClangQueryGatherer, AfterRestartCreateSourceRangesMessagesFutureCountIsTwos)
{
    manyGatherer.startCreateNextSourceRangesMessages();

    manyGatherer.startCreateNextSourceRangesMessages();

    ASSERT_THAT(manyGatherer.sourceFutures(), SizeIs(2));
}

TEST_F(ClangQueryGatherer, AfterStartCreateSourceRangesMessagesGetCollected)
{
    manyGatherer.startCreateNextSourceRangesMessages();

    ASSERT_THAT(manyGatherer.allCurrentProcessedMessages(),
                UnorderedElementsAre(
                    Property(&SourceRangesForQueryMessage::sourceRanges,
                             Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                      UnorderedElementsAre(IsSourceRangeWithText(1, 1, 1, 9, "void f();"),
                                                           IsSourceRangeWithText(2, 1, 2, 12, "void f() {}")))),
                    Property(&SourceRangesForQueryMessage::sourceRanges,
                             Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                      UnorderedElementsAre(
                                          IsSourceRangeWithText(1, 1, 1, 13, "int header();"),
                                          IsSourceRangeWithText(3, 1, 3, 15, "int function();"))))));
}

TEST_F(ClangQueryGatherer, GetFinishedMessages)
{
    manyGatherer.startCreateNextSourceRangesMessages();
    manyGatherer.waitForFinished();

    auto messages = manyGatherer.finishedMessages();

    ASSERT_THAT(messages,
                AllOf(SizeIs(2),
                      UnorderedElementsAre(
                          Property(&SourceRangesForQueryMessage::sourceRanges,
                                   Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                            UnorderedElementsAre(
                                                IsSourceRangeWithText(1, 1, 1, 9, "void f();"),
                                                IsSourceRangeWithText(2, 1, 2, 12, "void f() {}")))),
                          Property(&SourceRangesForQueryMessage::sourceRanges,
                                   Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                            UnorderedElementsAre(
                                                IsSourceRangeWithText(1, 1, 1, 13, "int header();"),
                                                IsSourceRangeWithText(3, 1, 3, 15, "int function();")))))));
}

TEST_F(ClangQueryGatherer, GetFinishedMessagesAfterSecondPass)
{
    manyGatherer.startCreateNextSourceRangesMessages();
    manyGatherer.waitForFinished();
    manyGatherer.finishedMessages();
    manyGatherer.startCreateNextSourceRangesMessages();
    manyGatherer.waitForFinished();

    auto messages = manyGatherer.finishedMessages();

    ASSERT_THAT(messages,
                AllOf(SizeIs(1),
                      ElementsAre(
                          Property(&SourceRangesForQueryMessage::sourceRanges,
                                   Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                            UnorderedElementsAre(
                                                IsSourceRangeWithText(3, 1, 3, 15, "int function();")))))));
}

TEST_F(ClangQueryGatherer, FilterDuplicates)
{
    manyGatherer.setProcessingSlotCount(3);
    manyGatherer.startCreateNextSourceRangesMessages();
    manyGatherer.waitForFinished();

    auto messages = manyGatherer.finishedMessages();

    ASSERT_THAT(messages,
                AllOf(SizeIs(3),
                      UnorderedElementsAre(
                          Property(&SourceRangesForQueryMessage::sourceRanges,
                                   Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                            UnorderedElementsAre(
                                                IsSourceRangeWithText(1, 1, 1, 9, "void f();"),
                                                IsSourceRangeWithText(2, 1, 2, 12, "void f() {}")))),
                          Property(&SourceRangesForQueryMessage::sourceRanges,
                                   Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                            UnorderedElementsAre(
                                                IsSourceRangeWithText(1, 1, 1, 13, "int header();"),
                                                IsSourceRangeWithText(3, 1, 3, 15, "int function();")))),
                          Property(&SourceRangesForQueryMessage::sourceRanges,
                                   Property(&SourceRangesContainer::sourceRangeWithTextContainers,
                                            UnorderedElementsAre(
                                                IsSourceRangeWithText(3, 1, 3, 15, "int function();")))))));
}

TEST_F(ClangQueryGatherer, AfterGetFinishedMessagesFuturesAreReduced)
{
    manyGatherer.startCreateNextSourceRangesMessages();
    manyGatherer.waitForFinished();

    manyGatherer.finishedMessages();

    ASSERT_THAT(manyGatherer.sourceFutures(), SizeIs(0));
}

TEST_F(ClangQueryGatherer, SourceFutureIsOneInTheSecondRun)
{
    manyGatherer.startCreateNextSourceRangesMessages();
    manyGatherer.waitForFinished();
    manyGatherer.finishedMessages();

    manyGatherer.startCreateNextSourceRangesMessages();

    ASSERT_THAT(manyGatherer.sourceFutures(), SizeIs(1));
}

TEST_F(ClangQueryGatherer, GetOneMessageInTheSecondRun)
{
    manyGatherer.startCreateNextSourceRangesMessages();
    manyGatherer.waitForFinished();
    manyGatherer.finishedMessages();
    manyGatherer.startCreateNextSourceRangesMessages();
    manyGatherer.waitForFinished();

    auto messages = manyGatherer.finishedMessages();

    ASSERT_THAT(messages, SizeIs(1));
}

TEST_F(ClangQueryGatherer, IsNotFinishedIfSourcesExists)
{
    manyGatherer.startCreateNextSourceRangesMessages();
    manyGatherer.waitForFinished();
    manyGatherer.finishedMessages();

    bool isFinished = manyGatherer.isFinished();

    ASSERT_FALSE(isFinished);
}

TEST_F(ClangQueryGatherer, IsNotFinishedIfSourceFuturesExists)
{
    manyGatherer.startCreateNextSourceRangesMessages();
    manyGatherer.waitForFinished();
    manyGatherer.finishedMessages();
    manyGatherer.startCreateNextSourceRangesMessages();

    bool isFinished = manyGatherer.isFinished();

    ASSERT_FALSE(isFinished);
}

TEST_F(ClangQueryGatherer, IsFinishedIfNoSourceAndSourceFuturesExists)
{
    manyGatherer.startCreateNextSourceRangesMessages();
    manyGatherer.waitForFinished();
    manyGatherer.finishedMessages();
    manyGatherer.startCreateNextSourceRangesMessages();
    manyGatherer.waitForFinished();
    manyGatherer.finishedMessages();

    bool isFinished = manyGatherer.isFinished();

    ASSERT_TRUE(isFinished);
}

void ClangQueryGatherer::SetUp()
{
    manyGatherer.setProcessingSlotCount(2);
}

}
