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

#include <filepathcaching.h>
#include <filestatuscache.h>
#include <refactoringdatabaseinitializer.h>

#include <QDateTime>
#include <QFileInfo>

#include <fstream>

namespace {

using ClangBackEnd::FilePathId;

class FileStatusCache : public testing::Test
{
protected:
    FilePathId filePathId(Utils::SmallStringView path) const
    {
        return filePathCache.filePathId(ClangBackEnd::FilePathView(path));
    }

    void touchFile(FilePathId filePathId)
    {
        std::ofstream ostream(std::string(filePathCache.filePath(filePathId)), std::ios::binary);
        ostream.write("\n", 1);
        ostream.close();
    }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> databaseInitializer{database};
    ClangBackEnd::FilePathCaching filePathCache{database};
    ClangBackEnd::FileStatusCache cache{filePathCache};
    FilePathId header{filePathId(TESTDATA_DIR "/filestatuscache_header.h")};
    FilePathId source{filePathId(TESTDATA_DIR "/filestatuscache_header.cpp")};
    long long headerLastModifiedTime = QFileInfo(TESTDATA_DIR "/filestatuscache_header.h").lastModified().toSecsSinceEpoch();
    long long sourceLastModifiedTime = QFileInfo(TESTDATA_DIR "/filestatuscache_header.cpp").lastModified().toSecsSinceEpoch();
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
    auto oldLastModified = cache.lastModifiedTime(header);
    touchFile(header);

    cache.update(header);

    ASSERT_THAT(cache.lastModifiedTime(header), Gt(oldLastModified));
}

TEST_F(FileStatusCache, UpdateFileDoesNotChangeEntryCount)
{
    cache.lastModifiedTime(header);
    touchFile(header);

    cache.update(header);

    ASSERT_THAT(cache, SizeIs(1));
}

TEST_F(FileStatusCache, UpdateFileForNonExistingEntry)
{
    touchFile(header);

    cache.update(header);

    ASSERT_THAT(cache, SizeIs(0));
}

}
