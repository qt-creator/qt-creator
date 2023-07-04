// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include "../mocks/sqlitedatabasemock.h"

#include <imagecachestorage.h>
#include <sqlitedatabase.h>

namespace {

MATCHER_P(IsIcon, icon, std::string(negation ? "is't" : "is") + PrintToString(icon))
{
    return arg.availableSizes() == icon.availableSizes();
}

TEST(ImageCacheStorageUpdateTest, check_version_if_database_is_already_initialized)
{
    NiceMock<SqliteDatabaseMock> databaseMock;
    ON_CALL(databaseMock, isInitialized()).WillByDefault(Return(true));

    EXPECT_CALL(databaseMock, version());

    QmlDesigner::ImageCacheStorage<SqliteDatabaseMock> storage{databaseMock};
}

TEST(ImageCacheStorageUpdateTest, add_column_mid_size_if_version_is_zero)
{
    NiceMock<SqliteDatabaseMock> databaseMock;
    ON_CALL(databaseMock, isInitialized()).WillByDefault(Return(true));
    EXPECT_CALL(databaseMock, execute(_)).Times(AnyNumber());

    EXPECT_CALL(databaseMock, execute(Eq("ALTER TABLE images ADD COLUMN midSizeImage")));

    QmlDesigner::ImageCacheStorage<SqliteDatabaseMock> storage{databaseMock};
}

TEST(ImageCacheStorageUpdateTest, delete_all_rows_before_adding_mid_size_column)
{
    NiceMock<SqliteDatabaseMock> databaseMock;
    ON_CALL(databaseMock, isInitialized()).WillByDefault(Return(true));
    InSequence s;

    EXPECT_CALL(databaseMock, execute(Eq("DELETE FROM images")));
    EXPECT_CALL(databaseMock, execute(Eq("ALTER TABLE images ADD COLUMN midSizeImage")));

    QmlDesigner::ImageCacheStorage<SqliteDatabaseMock> storage{databaseMock};
}

TEST(ImageCacheStorageUpdateTest, dont_call_add_column_mid_size_if_database_was_not_already_initialized)
{
    NiceMock<SqliteDatabaseMock> databaseMock;
    ON_CALL(databaseMock, isInitialized()).WillByDefault(Return(false));
    EXPECT_CALL(databaseMock, execute(_)).Times(2);

    EXPECT_CALL(databaseMock, execute(Eq("ALTER TABLE images ADD COLUMN midSizeImage"))).Times(0);

    QmlDesigner::ImageCacheStorage<SqliteDatabaseMock> storage{databaseMock};
}

TEST(ImageCacheStorageUpdateTest, set_version_to_one_if_version_is_zero)
{
    NiceMock<SqliteDatabaseMock> databaseMock;
    ON_CALL(databaseMock, isInitialized()).WillByDefault(Return(true));

    EXPECT_CALL(databaseMock, setVersion(Eq(1)));

    QmlDesigner::ImageCacheStorage<SqliteDatabaseMock> storage{databaseMock};
}

TEST(ImageCacheStorageUpdateTest, dont_set_version_if_version_is_one)
{
    NiceMock<SqliteDatabaseMock> databaseMock;
    ON_CALL(databaseMock, isInitialized()).WillByDefault(Return(true));
    ON_CALL(databaseMock, version()).WillByDefault(Return(1));

    EXPECT_CALL(databaseMock, setVersion(_)).Times(0);

    QmlDesigner::ImageCacheStorage<SqliteDatabaseMock> storage{databaseMock};
}

TEST(ImageCacheStorageUpdateTest, set_version_to_one_for_initialization)
{
    NiceMock<SqliteDatabaseMock> databaseMock;
    ON_CALL(databaseMock, isInitialized()).WillByDefault(Return(false));
    ON_CALL(databaseMock, version()).WillByDefault(Return(1));

    EXPECT_CALL(databaseMock, setVersion(Eq(1)));

    QmlDesigner::ImageCacheStorage<SqliteDatabaseMock> storage{databaseMock};
}

class ImageCacheStorageTest : public testing::Test
{
protected:
    template<int ResultCount, int BindParameterCount = 0>
    using ReadStatement = QmlDesigner::ImageCacheStorage<
        SqliteDatabaseMock>::ReadStatement<ResultCount, BindParameterCount>;
    template<int BindParameterCount>
    using WriteStatement = QmlDesigner::ImageCacheStorage<SqliteDatabaseMock>::WriteStatement<BindParameterCount>;

    NiceMock<SqliteDatabaseMock> databaseMock;
    QmlDesigner::ImageCacheStorage<SqliteDatabaseMock> storage{databaseMock};
    ReadStatement<1, 2> &selectImageStatement = storage.selectImageStatement;
    ReadStatement<1, 2> &selectMidSizeImageStatement = storage.selectMidSizeImageStatement;
    ReadStatement<1, 2> &selectSmallImageStatement = storage.selectSmallImageStatement;
    ReadStatement<1, 2> &selectIconStatement = storage.selectIconStatement;
    WriteStatement<5> &upsertImageStatement = storage.upsertImageStatement;
    WriteStatement<3> &upsertIconStatement = storage.upsertIconStatement;
    QImage image1{10, 10, QImage::Format_ARGB32};
    QImage midSizeImage1{5, 5, QImage::Format_ARGB32};
    QImage smallImage1{1, 1, QImage::Format_ARGB32};
    QIcon icon1{QPixmap::fromImage(image1)};
};

TEST_F(ImageCacheStorageTest, fetch_image_calls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectImageStatement,
                valueReturnBlob(TypedEq<Utils::SmallStringView>("/path/to/component"),
                                TypedEq<long long>(123)));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchImage("/path/to/component", {123});
}

TEST_F(ImageCacheStorageTest, fetch_image_calls_is_busy)
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

TEST_F(ImageCacheStorageTest, fetch_mid_size_image_calls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectMidSizeImageStatement,
                valueReturnBlob(TypedEq<Utils::SmallStringView>("/path/to/component"),
                                TypedEq<long long>(123)));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchMidSizeImage("/path/to/component", {123});
}

TEST_F(ImageCacheStorageTest, fetch_mid_size_image_calls_is_busy)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectMidSizeImageStatement,
                valueReturnBlob(TypedEq<Utils::SmallStringView>("/path/to/component"),
                                TypedEq<long long>(123)))
        .WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(databaseMock, rollback());
    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectMidSizeImageStatement,
                valueReturnBlob(TypedEq<Utils::SmallStringView>("/path/to/component"),
                                TypedEq<long long>(123)));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchMidSizeImage("/path/to/component", {123});
}

TEST_F(ImageCacheStorageTest, fetch_small_image_calls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectSmallImageStatement,
                valueReturnBlob(TypedEq<Utils::SmallStringView>("/path/to/component"),
                                TypedEq<long long>(123)));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchSmallImage("/path/to/component", {123});
}

TEST_F(ImageCacheStorageTest, fetch_small_image_calls_is_busy)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectSmallImageStatement,
                valueReturnBlob(TypedEq<Utils::SmallStringView>("/path/to/component"),
                                TypedEq<long long>(123)))
        .WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(databaseMock, rollback());
    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectSmallImageStatement,
                valueReturnBlob(TypedEq<Utils::SmallStringView>("/path/to/component"),
                                TypedEq<long long>(123)));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchSmallImage("/path/to/component", {123});
}

TEST_F(ImageCacheStorageTest, fetch_icon_calls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectIconStatement,
                valueReturnBlob(TypedEq<Utils::SmallStringView>("/path/to/component"),
                                TypedEq<long long>(123)));
    EXPECT_CALL(databaseMock, commit());

    storage.fetchIcon("/path/to/component", {123});
}

TEST_F(ImageCacheStorageTest, fetch_icon_calls_is_busy)
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

TEST_F(ImageCacheStorageTest, store_image_calls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, immediateBegin());
    EXPECT_CALL(upsertImageStatement,
                write(TypedEq<Utils::SmallStringView>("/path/to/component"),
                      TypedEq<long long>(123),
                      Not(IsEmpty()),
                      Not(IsEmpty()),
                      Not(IsEmpty())));
    EXPECT_CALL(databaseMock, commit());

    storage.storeImage("/path/to/component", {123}, image1, midSizeImage1, smallImage1);
}

TEST_F(ImageCacheStorageTest, store_empty_image_calls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, immediateBegin());
    EXPECT_CALL(upsertImageStatement,
                write(TypedEq<Utils::SmallStringView>("/path/to/component"),
                      TypedEq<long long>(123),
                      IsEmpty(),
                      IsEmpty(),
                      IsEmpty()));
    EXPECT_CALL(databaseMock, commit());

    storage.storeImage("/path/to/component", {123}, QImage{}, QImage{}, QImage{});
}

TEST_F(ImageCacheStorageTest, store_image_calls_is_busy)
{
    InSequence s;

    EXPECT_CALL(databaseMock, immediateBegin()).WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(databaseMock, immediateBegin());
    EXPECT_CALL(upsertImageStatement,
                write(TypedEq<Utils::SmallStringView>("/path/to/component"),
                      TypedEq<long long>(123),
                      IsEmpty(),
                      IsEmpty(),
                      IsEmpty()));
    EXPECT_CALL(databaseMock, commit());

    storage.storeImage("/path/to/component", {123}, QImage{}, QImage{}, QImage{});
}

TEST_F(ImageCacheStorageTest, store_icon_calls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, immediateBegin());
    EXPECT_CALL(upsertIconStatement,
                write(TypedEq<Utils::SmallStringView>("/path/to/component"),
                      TypedEq<long long>(123),
                      A<Sqlite::BlobView>()));
    EXPECT_CALL(databaseMock, commit());

    storage.storeIcon("/path/to/component", {123}, icon1);
}

TEST_F(ImageCacheStorageTest, store_empty_icon_calls)
{
    InSequence s;

    EXPECT_CALL(databaseMock, immediateBegin());
    EXPECT_CALL(upsertIconStatement,
                write(TypedEq<Utils::SmallStringView>("/path/to/component"),
                      TypedEq<long long>(123),
                      IsEmpty()));
    EXPECT_CALL(databaseMock, commit());

    storage.storeIcon("/path/to/component", {123}, QIcon{});
}

TEST_F(ImageCacheStorageTest, store_icon_calls_is_busy)
{
    InSequence s;

    EXPECT_CALL(databaseMock, immediateBegin()).WillOnce(Throw(Sqlite::StatementIsBusy("busy")));
    EXPECT_CALL(databaseMock, immediateBegin());
    EXPECT_CALL(upsertIconStatement,
                write(TypedEq<Utils::SmallStringView>("/path/to/component"),
                      TypedEq<long long>(123),
                      IsEmpty()));
    EXPECT_CALL(databaseMock, commit());

    storage.storeIcon("/path/to/component", {123}, QIcon{});
}

TEST_F(ImageCacheStorageTest, call_wal_checkoint_full)
{
    EXPECT_CALL(databaseMock, walCheckpointFull());

    storage.walCheckpointFull();
}

TEST_F(ImageCacheStorageTest, call_wal_checkoint_full_is_busy)
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
    QImage midSizeImage1{image1.scaled(96, 96)};
    QImage smallImage1{image1.scaled(48, 48)};
    QIcon icon1{QPixmap::fromImage(image1)};
};

TEST_F(ImageCacheStorageSlowTest, store_image)
{
    storage.storeImage("/path/to/component", {123}, image1, QImage{}, QImage{});

    ASSERT_THAT(storage.fetchImage("/path/to/component", {123}), Optional(image1));
}

TEST_F(ImageCacheStorageSlowTest, store_empty_image_after_entry)
{
    storage.storeImage("/path/to/component", {123}, image1, midSizeImage1, smallImage1);

    storage.storeImage("/path/to/component", {123}, QImage{}, midSizeImage1, smallImage1);

    ASSERT_THAT(storage.fetchImage("/path/to/component", {123}), Optional(QImage{}));
}

TEST_F(ImageCacheStorageSlowTest, store_mid_size_image)
{
    storage.storeImage("/path/to/component", {123}, QImage{}, midSizeImage1, QImage{});

    ASSERT_THAT(storage.fetchMidSizeImage("/path/to/component", {123}), Optional(midSizeImage1));
}

TEST_F(ImageCacheStorageSlowTest, store_empty_mid_size_image_after_entry)
{
    storage.storeImage("/path/to/component", {123}, image1, midSizeImage1, smallImage1);

    storage.storeImage("/path/to/component", {123}, image1, QImage{}, smallImage1);

    ASSERT_THAT(storage.fetchMidSizeImage("/path/to/component", {123}), Optional(QImage{}));
}

TEST_F(ImageCacheStorageSlowTest, store_small_image)
{
    storage.storeImage("/path/to/component", {123}, QImage{}, QImage{}, smallImage1);

    ASSERT_THAT(storage.fetchSmallImage("/path/to/component", {123}), Optional(smallImage1));
}

TEST_F(ImageCacheStorageSlowTest, store_empty_small_image_after_entry)
{
    storage.storeImage("/path/to/component", {123}, image1, midSizeImage1, smallImage1);

    storage.storeImage("/path/to/component", {123}, image1, midSizeImage1, QImage{});

    ASSERT_THAT(storage.fetchSmallImage("/path/to/component", {123}), Optional(QImage{}));
}

TEST_F(ImageCacheStorageSlowTest, store_empty_entry)
{
    storage.storeImage("/path/to/component", {123}, QImage{}, QImage{}, QImage{});

    ASSERT_THAT(storage.fetchImage("/path/to/component", {123}), Optional(QImage{}));
}

TEST_F(ImageCacheStorageSlowTest, fetch_non_existing_image_is_empty)
{
    auto image = storage.fetchImage("/path/to/component", {123});

    ASSERT_THAT(image, Eq(std::nullopt));
}

TEST_F(ImageCacheStorageSlowTest, fetch_same_time_image)
{
    storage.storeImage("/path/to/component", {123}, image1, midSizeImage1, smallImage1);

    auto image = storage.fetchImage("/path/to/component", {123});

    ASSERT_THAT(image, Optional(image1));
}

TEST_F(ImageCacheStorageSlowTest, do_not_fetch_older_image)
{
    storage.storeImage("/path/to/component", {123}, image1, midSizeImage1, smallImage1);

    auto image = storage.fetchImage("/path/to/component", {124});

    ASSERT_THAT(image, Eq(std::nullopt));
}

TEST_F(ImageCacheStorageSlowTest, fetch_newer_image)
{
    storage.storeImage("/path/to/component", {123}, image1, midSizeImage1, smallImage1);

    auto image = storage.fetchImage("/path/to/component", {122});

    ASSERT_THAT(image, Optional(image1));
}

TEST_F(ImageCacheStorageSlowTest, fetch_non_existing_mid_size_image_is_empty)
{
    auto image = storage.fetchMidSizeImage("/path/to/component", {123});

    ASSERT_THAT(image, Eq(std::nullopt));
}

TEST_F(ImageCacheStorageSlowTest, fetch_same_time_mid_size_image)
{
    storage.storeImage("/path/to/component", {123}, image1, midSizeImage1, smallImage1);

    auto image = storage.fetchMidSizeImage("/path/to/component", {123});

    ASSERT_THAT(image, Optional(midSizeImage1));
}

TEST_F(ImageCacheStorageSlowTest, do_not_fetch_older_mid_size_image)
{
    storage.storeImage("/path/to/component", {123}, image1, midSizeImage1, smallImage1);

    auto image = storage.fetchMidSizeImage("/path/to/component", {124});

    ASSERT_THAT(image, Eq(std::nullopt));
}

TEST_F(ImageCacheStorageSlowTest, fetch_newer_mid_size_image)
{
    storage.storeImage("/path/to/component", {123}, image1, midSizeImage1, smallImage1);

    auto image = storage.fetchMidSizeImage("/path/to/component", {122});

    ASSERT_THAT(image, Optional(midSizeImage1));
}

TEST_F(ImageCacheStorageSlowTest, fetch_non_existing_small_image_is_empty)
{
    auto image = storage.fetchSmallImage("/path/to/component", {123});

    ASSERT_THAT(image, Eq(std::nullopt));
}

TEST_F(ImageCacheStorageSlowTest, fetch_same_time_small_image)
{
    storage.storeImage("/path/to/component", {123}, image1, midSizeImage1, smallImage1);

    auto image = storage.fetchSmallImage("/path/to/component", {123});

    ASSERT_THAT(image, Optional(smallImage1));
}

TEST_F(ImageCacheStorageSlowTest, do_not_fetch_older_small_image)
{
    storage.storeImage("/path/to/component", {123}, image1, midSizeImage1, smallImage1);

    auto image = storage.fetchSmallImage("/path/to/component", {124});

    ASSERT_THAT(image, Eq(std::nullopt));
}

TEST_F(ImageCacheStorageSlowTest, fetch_newer_small_image)
{
    storage.storeImage("/path/to/component", {123}, image1, midSizeImage1, smallImage1);

    auto image = storage.fetchSmallImage("/path/to/component", {122});

    ASSERT_THAT(image, Optional(smallImage1));
}

TEST_F(ImageCacheStorageSlowTest, store_icon)
{
    storage.storeIcon("/path/to/component", {123}, icon1);

    ASSERT_THAT(storage.fetchIcon("/path/to/component", {123}), Optional(IsIcon(icon1)));
}

TEST_F(ImageCacheStorageSlowTest, store_empty_icon_after_entry)
{
    storage.storeIcon("/path/to/component", {123}, icon1);

    storage.storeIcon("/path/to/component", {123}, QIcon{});

    ASSERT_THAT(storage.fetchIcon("/path/to/component", {123}), Optional(IsIcon(QIcon{})));
}

TEST_F(ImageCacheStorageSlowTest, store_empty_icon_entry)
{
    storage.storeIcon("/path/to/component", {123}, QIcon{});

    ASSERT_THAT(storage.fetchIcon("/path/to/component", {123}), Optional(IsIcon(QIcon{})));
}

TEST_F(ImageCacheStorageSlowTest, fetch_non_existing_icon_is_empty)
{
    auto image = storage.fetchIcon("/path/to/component", {123});

    ASSERT_THAT(image, Eq(std::nullopt));
}

TEST_F(ImageCacheStorageSlowTest, fetch_same_time_icon)
{
    storage.storeIcon("/path/to/component", {123}, icon1);

    auto image = storage.fetchIcon("/path/to/component", {123});

    ASSERT_THAT(image, Optional(IsIcon(icon1)));
}

TEST_F(ImageCacheStorageSlowTest, do_not_fetch_older_icon)
{
    storage.storeIcon("/path/to/component", {123}, icon1);

    auto image = storage.fetchIcon("/path/to/component", {124});

    ASSERT_THAT(image, Eq(std::nullopt));
}

TEST_F(ImageCacheStorageSlowTest, fetch_newer_icon)
{
    storage.storeIcon("/path/to/component", {123}, icon1);

    auto image = storage.fetchIcon("/path/to/component", {122});

    ASSERT_THAT(image, Optional(IsIcon(icon1)));
}

TEST_F(ImageCacheStorageSlowTest, fetch_modified_image_time)
{
    storage.storeImage("/path/to/component", {123}, image1, midSizeImage1, smallImage1);

    auto timeStamp = storage.fetchModifiedImageTime("/path/to/component");

    ASSERT_THAT(timeStamp, Eq(Sqlite::TimeStamp{123}));
}

TEST_F(ImageCacheStorageSlowTest, fetch_invalid_modified_image_time_for_no_entry)
{
    storage.storeImage("/path/to/component2", {123}, image1, midSizeImage1, smallImage1);

    auto timeStamp = storage.fetchModifiedImageTime("/path/to/component");

    ASSERT_THAT(timeStamp, Eq(Sqlite::TimeStamp{}));
}

TEST_F(ImageCacheStorageSlowTest, fetch_has_image)
{
    storage.storeImage("/path/to/component", {123}, image1, midSizeImage1, smallImage1);

    auto hasImage = storage.fetchHasImage("/path/to/component");

    ASSERT_TRUE(hasImage);
}

TEST_F(ImageCacheStorageSlowTest, fetch_has_image_for_null_image)
{
    storage.storeImage("/path/to/component", {123}, QImage{}, QImage{}, QImage{});

    auto hasImage = storage.fetchHasImage("/path/to/component");

    ASSERT_FALSE(hasImage);
}

TEST_F(ImageCacheStorageSlowTest, fetch_has_image_for_no_entry)
{
    storage.storeImage("/path/to/component", {123}, QImage{}, QImage{}, QImage{});

    auto hasImage = storage.fetchHasImage("/path/to/component");

    ASSERT_FALSE(hasImage);
}
} // namespace
