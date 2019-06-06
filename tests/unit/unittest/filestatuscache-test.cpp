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
#include "mockfilesystem.h"

#include <filepathcaching.h>
#include <filestatuscache.h>
#include <refactoringdatabaseinitializer.h>

#include <QDateTime>
#include <QFileInfo>

#include <fstream>

namespace {

using ClangBackEnd::FilePathId;
using ClangBackEnd::FilePathIds;

class FileStatusCache : public testing::Test
{
protected:
    FileStatusCache()
    {
        ON_CALL(fileSystem, lastModified(Eq(header))).WillByDefault(Return(headerLastModifiedTime));
        ON_CALL(fileSystem, lastModified(Eq(source))).WillByDefault(Return(sourceLastModifiedTime));
        ON_CALL(fileSystem, lastModified(Eq(header2))).WillByDefault(Return(header2LastModifiedTime));
        ON_CALL(fileSystem, lastModified(Eq(source2))).WillByDefault(Return(source2LastModifiedTime));
    }

protected:
    NiceMock<MockFileSystem> fileSystem;
    ClangBackEnd::FileStatusCache cache{fileSystem};
    FilePathId header{1};
    FilePathId source{2};
    FilePathId header2{3};
    FilePathId source2{4};
    FilePathIds entries{header, source, header2, source2};
    long long headerLastModifiedTime = 100;
    long long headerLastModifiedTime2 = 110;
    long long header2LastModifiedTime = 300;
    long long header2LastModifiedTime2 = 310;
    long long sourceLastModifiedTime = 200;
    long long source2LastModifiedTime = 400;
};

TEST_F(FileStatusCache, CreateEntry)
{
    cache.lastModifiedTime(header);

    ASSERT_THAT(cache, SizeIs(1));
}

TEST_F(FileStatusCache, AskCreatedEntryForLastModifiedTime)
{
    auto lastModified = cache.lastModifiedTime(header);

    ASSERT_THAT(lastModified, headerLastModifiedTime);
}

TEST_F(FileStatusCache, AskCachedEntryForLastModifiedTime)
{
    cache.lastModifiedTime(header);

    auto lastModified = cache.lastModifiedTime(header);

    ASSERT_THAT(lastModified, headerLastModifiedTime);
}

TEST_F(FileStatusCache, DontAddEntryTwice)
{
    cache.lastModifiedTime(header);

    cache.lastModifiedTime(header);

    ASSERT_THAT(cache, SizeIs(1));
}

TEST_F(FileStatusCache, AddNewEntry)
{
    cache.lastModifiedTime(header);

    cache.lastModifiedTime(source);

    ASSERT_THAT(cache, SizeIs(2));
}

TEST_F(FileStatusCache, AskNewEntryForLastModifiedTime)
{
    cache.lastModifiedTime(header);

    auto lastModified = cache.lastModifiedTime(source);

    ASSERT_THAT(lastModified, sourceLastModifiedTime);
}

TEST_F(FileStatusCache, AddNewEntryReverseOrder)
{
    cache.lastModifiedTime(source);

    cache.lastModifiedTime(header);

    ASSERT_THAT(cache, SizeIs(2));
}

TEST_F(FileStatusCache, AskNewEntryReverseOrderAddedForLastModifiedTime)
{
    cache.lastModifiedTime(source);

    auto lastModified = cache.lastModifiedTime(header);

    ASSERT_THAT(lastModified, headerLastModifiedTime);
}

TEST_F(FileStatusCache, UpdateFile)
{
    EXPECT_CALL(fileSystem, lastModified(Eq(header)))
        .Times(2)
        .WillOnce(Return(headerLastModifiedTime))
        .WillOnce(Return(headerLastModifiedTime2));
    cache.lastModifiedTime(header);

    cache.update(header);

    ASSERT_THAT(cache.lastModifiedTime(header), headerLastModifiedTime2);
}

TEST_F(FileStatusCache, UpdateFileDoesNotChangeEntryCount)
{
    EXPECT_CALL(fileSystem, lastModified(Eq(header)))
        .Times(2)
        .WillOnce(Return(headerLastModifiedTime))
        .WillOnce(Return(headerLastModifiedTime2));
    cache.lastModifiedTime(header);

    cache.update(header);

    ASSERT_THAT(cache, SizeIs(1));
}

TEST_F(FileStatusCache, UpdateFileForNonExistingEntry)
{
    cache.update(header);

    ASSERT_THAT(cache, SizeIs(0));
}

TEST_F(FileStatusCache, UpdateFiles)
{
    EXPECT_CALL(fileSystem, lastModified(Eq(header)))
        .Times(2)
        .WillOnce(Return(headerLastModifiedTime))
        .WillOnce(Return(headerLastModifiedTime2));
    EXPECT_CALL(fileSystem, lastModified(Eq(header2)))
        .Times(2)
        .WillOnce(Return(header2LastModifiedTime))
        .WillOnce(Return(header2LastModifiedTime2));
    cache.lastModifiedTime(header);
    cache.lastModifiedTime(header2);

    cache.update(entries);

    ASSERT_THAT(cache.lastModifiedTime(header), headerLastModifiedTime2);
    ASSERT_THAT(cache.lastModifiedTime(header2), header2LastModifiedTime2);
}

TEST_F(FileStatusCache, UpdateFilesDoesNotChangeEntryCount)
{
    EXPECT_CALL(fileSystem, lastModified(Eq(header)))
        .Times(2)
        .WillOnce(Return(headerLastModifiedTime))
        .WillOnce(Return(headerLastModifiedTime2));
    EXPECT_CALL(fileSystem, lastModified(Eq(header2)))
        .Times(2)
        .WillOnce(Return(header2LastModifiedTime))
        .WillOnce(Return(header2LastModifiedTime2));
    cache.lastModifiedTime(header);
    cache.lastModifiedTime(header2);

    cache.update(entries);

    ASSERT_THAT(cache, SizeIs(2));
}

TEST_F(FileStatusCache, UpdateFilesForNonExistingEntry)
{
    cache.update(entries);

    ASSERT_THAT(cache, SizeIs(0));
}

TEST_F(FileStatusCache, NewModifiedEntries)
{
    auto modifiedIds = cache.modified(entries);

    ASSERT_THAT(modifiedIds, entries);
}

TEST_F(FileStatusCache, NoNewModifiedEntries)
{
    cache.modified(entries);

    auto modifiedIds = cache.modified(entries);

    ASSERT_THAT(modifiedIds, IsEmpty());
}

TEST_F(FileStatusCache, SomeNewModifiedEntries)
{
    cache.modified({source, header2});

    auto modifiedIds = cache.modified(entries);

    ASSERT_THAT(modifiedIds, ElementsAre(header, source2));
}

TEST_F(FileStatusCache, SomeAlreadyExistingModifiedEntries)
{
    EXPECT_CALL(fileSystem, lastModified(Eq(header)))
        .Times(2)
        .WillOnce(Return(headerLastModifiedTime))
        .WillOnce(Return(headerLastModifiedTime2));
    EXPECT_CALL(fileSystem, lastModified(Eq(header2)))
        .Times(2)
        .WillOnce(Return(header2LastModifiedTime))
        .WillOnce(Return(header2LastModifiedTime2));
    EXPECT_CALL(fileSystem, lastModified(Eq(source))).Times(2).WillRepeatedly(Return(sourceLastModifiedTime));
    EXPECT_CALL(fileSystem, lastModified(Eq(source2)))
        .Times(2)
        .WillRepeatedly(Return(source2LastModifiedTime));
    cache.modified(entries);

    auto modifiedIds = cache.modified(entries);

    ASSERT_THAT(modifiedIds, ElementsAre(header, header2));
}

TEST_F(FileStatusCache, SomeAlreadyExistingAndSomeNewModifiedEntries)
{
    EXPECT_CALL(fileSystem, lastModified(Eq(header))).WillRepeatedly(Return(headerLastModifiedTime));
    EXPECT_CALL(fileSystem, lastModified(Eq(header2)))
        .Times(2)
        .WillOnce(Return(header2LastModifiedTime))
        .WillOnce(Return(header2LastModifiedTime2));
    EXPECT_CALL(fileSystem, lastModified(Eq(source))).Times(2).WillRepeatedly(Return(sourceLastModifiedTime));
    EXPECT_CALL(fileSystem, lastModified(Eq(source2))).WillRepeatedly(Return(source2LastModifiedTime));
    cache.modified({source, header2});

    auto modifiedIds = cache.modified(entries);

    ASSERT_THAT(modifiedIds, ElementsAre(header, header2, source2));
}

TEST_F(FileStatusCache, TimeIsUpdatedForSomeAlreadyExistingModifiedEntries)
{
    EXPECT_CALL(fileSystem, lastModified(Eq(header)))
        .Times(2)
        .WillOnce(Return(headerLastModifiedTime))
        .WillOnce(Return(headerLastModifiedTime2));
    EXPECT_CALL(fileSystem, lastModified(Eq(header2)))
        .Times(2)
        .WillOnce(Return(header2LastModifiedTime))
        .WillOnce(Return(header2LastModifiedTime2));
    EXPECT_CALL(fileSystem, lastModified(Eq(source))).Times(2).WillRepeatedly(Return(sourceLastModifiedTime));
    EXPECT_CALL(fileSystem, lastModified(Eq(source2)))
        .Times(2)
        .WillRepeatedly(Return(source2LastModifiedTime));
    cache.modified(entries);

    cache.modified(entries);

    ASSERT_THAT(cache.lastModifiedTime(header), headerLastModifiedTime2);
}

} // namespace
