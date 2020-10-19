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

#include "sqlitedatabasemock.h"

#include <imagecachestorage.h>
#include <sqlitedatabase.h>

namespace {

MATCHER_P2(IsEntry,
           image,
           hasEntry,
           std::string(negation ? "is't" : "is")
               + PrintToString(QmlDesigner::ImageCacheStorageInterface::Entry{image, hasEntry}))
{
    const QmlDesigner::ImageCacheStorageInterface::Entry &entry = arg;
    return entry.image == image && entry.hasEntry == hasEntry;
}

class ImageCacheStorageTest : public testing::Test
{
protected:
    QImage createImage()
    {
        QImage image{150, 150, QImage::Format_ARGB32};
        image.fill(QColor{128, 64, 0, 11});
        image.setPixelColor(75, 75, QColor{1, 255, 33, 196});

        return image;
    }

protected:
    using ReadStatement = QmlDesigner::ImageCacheStorage<SqliteDatabaseMock>::ReadStatement;
    using WriteStatement = QmlDesigner::ImageCacheStorage<SqliteDatabaseMock>::WriteStatement;

    NiceMock<SqliteDatabaseMock> databaseMock;
    QmlDesigner::ImageCacheStorage<SqliteDatabaseMock> storage{databaseMock};
    ReadStatement &selectImageStatement = storage.selectImageStatement;
    ReadStatement &selectIconStatement = storage.selectIconStatement;
    WriteStatement &upsertImageStatement = storage.upsertImageStatement;
    QImage image1{createImage()};
};

TEST_F(ImageCacheStorageTest, Initialize)
{
    InSequence s;

    EXPECT_CALL(databaseMock, exclusiveBegin());
    EXPECT_CALL(databaseMock,
                execute(Eq("CREATE TABLE IF NOT EXISTS images(id INTEGER PRIMARY KEY, name TEXT "
                           "NOT NULL UNIQUE, mtime INTEGER, image BLOB, icon BLOB)")));
    EXPECT_CALL(databaseMock, commit());
    EXPECT_CALL(databaseMock, immediateBegin());
    EXPECT_CALL(databaseMock, prepare(Eq(selectImageStatement.sqlStatement)));
    EXPECT_CALL(databaseMock, prepare(Eq(selectIconStatement.sqlStatement)));
    EXPECT_CALL(databaseMock, prepare(Eq(upsertImageStatement.sqlStatement)));
    EXPECT_CALL(databaseMock, commit());

    QmlDesigner::ImageCacheStorage<SqliteDatabaseMock> storage{databaseMock};
}

TEST_F(ImageCacheStorageTest, FetchImageCalls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectImageStatement,
                valueReturnBlob(TypedEq<Utils::SmallStringView>("/path/to/component"),
                                TypedEq<long long>(123)));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchImage("/path/to/component", {123});
}

TEST_F(ImageCacheStorageTest, FetchImageCallsIsBusy)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectImageStatement,
                valueReturnBlob(TypedEq<Utils::SmallStringView>("/path/to/component"),
                                TypedEq<long long>(123)))
        .WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(databaseMock, rollback());
    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectImageStatement,
                valueReturnBlob(TypedEq<Utils::SmallStringView>("/path/to/component"),
                                TypedEq<long long>(123)));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchImage("/path/to/component", {123});
}

TEST_F(ImageCacheStorageTest, FetchIconCalls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectIconStatement,
                valueReturnBlob(TypedEq<Utils::SmallStringView>("/path/to/component"),
                                TypedEq<long long>(123)));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchIcon("/path/to/component", {123});
}

TEST_F(ImageCacheStorageTest, FetchIconCallsIsBusy)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectIconStatement,
                valueReturnBlob(TypedEq<Utils::SmallStringView>("/path/to/component"),
                                TypedEq<long long>(123)))
        .WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(databaseMock, rollback());
    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectIconStatement,
                valueReturnBlob(TypedEq<Utils::SmallStringView>("/path/to/component"),
                                TypedEq<long long>(123)));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchIcon("/path/to/component", {123});
}

TEST_F(ImageCacheStorageTest, StoreImageCalls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, immediateBegin());
    EXPECT_CALL(upsertImageStatement,
                write(TypedEq<Utils::SmallStringView>("/path/to/component"),
                      TypedEq<long long>(123),
                      A<Sqlite::BlobView>(),
                      A<Sqlite::BlobView>()));
    EXPECT_CALL(databaseMock, commit());

    storage.storeImage("/path/to/component", {123}, image1);
}

TEST_F(ImageCacheStorageTest, StoreEmptyImageCalls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, immediateBegin());
    EXPECT_CALL(upsertImageStatement,
                write(TypedEq<Utils::SmallStringView>("/path/to/component"),
                      TypedEq<long long>(123),
                      A<Sqlite::NullValue>(),
                      A<Sqlite::NullValue>()));
    EXPECT_CALL(databaseMock, commit());

    storage.storeImage("/path/to/component", {123}, QImage{});
}

TEST_F(ImageCacheStorageTest, StoreImageCallsIsBusy)
{
    InSequence s;

    EXPECT_CALL(databaseMock, immediateBegin()).WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(databaseMock, immediateBegin());
    EXPECT_CALL(upsertImageStatement,
                write(TypedEq<Utils::SmallStringView>("/path/to/component"),
                      TypedEq<long long>(123),
                      A<Sqlite::NullValue>(),
                      A<Sqlite::NullValue>()));
    EXPECT_CALL(databaseMock, commit());

    storage.storeImage("/path/to/component", {123}, QImage{});
}

TEST_F(ImageCacheStorageTest, CallWalCheckointFull)
{
    EXPECT_CALL(databaseMock, walCheckpointFull());

    storage.walCheckpointFull();
}

TEST_F(ImageCacheStorageTest, CallWalCheckointFullIsBusy)
{
    InSequence s;

    EXPECT_CALL(databaseMock, walCheckpointFull()).WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(databaseMock, walCheckpointFull());

    storage.walCheckpointFull();
}

class ImageCacheStorageSlowTest : public testing::Test
{
protected:
    QImage createImage()
    {
        QImage image{150, 150, QImage::Format_ARGB32};
        image.fill(QColor{128, 64, 0, 11});
        image.setPixelColor(1, 1, QColor{1, 255, 33, 196});

        return image;
    }

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    QmlDesigner::ImageCacheStorage<Sqlite::Database> storage{database};
    QImage image1{createImage()};
    QImage image2{10, 10, QImage::Format_ARGB32};
    QImage icon1{image1.scaled(96, 96)};
};

TEST_F(ImageCacheStorageSlowTest, StoreImage)
{
    storage.storeImage("/path/to/component", {123}, image1);

    ASSERT_THAT(storage.fetchImage("/path/to/component", {123}), IsEntry(image1, true));
}

TEST_F(ImageCacheStorageSlowTest, StoreEmptyImageAfterEntry)
{
    storage.storeImage("/path/to/component", {123}, image1);

    storage.storeImage("/path/to/component", {123}, QImage{});

    ASSERT_THAT(storage.fetchImage("/path/to/component", {123}), IsEntry(QImage{}, true));
}

TEST_F(ImageCacheStorageSlowTest, StoreEmptyEntry)
{
    storage.storeImage("/path/to/component", {123}, QImage{});

    ASSERT_THAT(storage.fetchImage("/path/to/component", {123}), IsEntry(QImage{}, true));
}

TEST_F(ImageCacheStorageSlowTest, FetchNonExistingImageIsEmpty)
{
    auto image = storage.fetchImage("/path/to/component", {123});

    ASSERT_THAT(image, IsEntry(QImage{}, false));
}

TEST_F(ImageCacheStorageSlowTest, FetchSameTimeImage)
{
    storage.storeImage("/path/to/component", {123}, image1);

    auto image = storage.fetchImage("/path/to/component", {123});

    ASSERT_THAT(image, IsEntry(image1, true));
}

TEST_F(ImageCacheStorageSlowTest, DoNotFetchOlderImage)
{
    storage.storeImage("/path/to/component", {123}, image1);

    auto image = storage.fetchImage("/path/to/component", {124});

    ASSERT_THAT(image, IsEntry(QImage{}, false));
}

TEST_F(ImageCacheStorageSlowTest, FetchNewerImage)
{
    storage.storeImage("/path/to/component", {123}, image1);

    auto image = storage.fetchImage("/path/to/component", {122});

    ASSERT_THAT(image, IsEntry(image1, true));
}

TEST_F(ImageCacheStorageSlowTest, FetchNonExistingIconIsEmpty)
{
    auto image = storage.fetchIcon("/path/to/component", {123});

    ASSERT_THAT(image, IsEntry(QImage{}, false));
}

TEST_F(ImageCacheStorageSlowTest, FetchSameTimeIcon)
{
    storage.storeImage("/path/to/component", {123}, image1);

    auto image = storage.fetchIcon("/path/to/component", {123});

    ASSERT_THAT(image, IsEntry(icon1, true));
}

TEST_F(ImageCacheStorageSlowTest, DoNotFetchOlderIcon)
{
    storage.storeImage("/path/to/component", {123}, image1);

    auto image = storage.fetchIcon("/path/to/component", {124});

    ASSERT_THAT(image, IsEntry(QImage{}, false));
}

TEST_F(ImageCacheStorageSlowTest, FetchNewerIcon)
{
    storage.storeImage("/path/to/component", {123}, image1);

    auto image = storage.fetchIcon("/path/to/component", {122});

    ASSERT_THAT(image, IsEntry(icon1, true));
}

TEST_F(ImageCacheStorageSlowTest, DontScaleSmallerIcon)
{
    storage.storeImage("/path/to/component", {123}, image2);

    auto image = storage.fetchImage("/path/to/component", {122});

    ASSERT_THAT(image, IsEntry(image2, true));
}

} // namespace
