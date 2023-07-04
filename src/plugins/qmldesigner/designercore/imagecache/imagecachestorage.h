// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "imagecachestorageinterface.h"

#include <createtablesqlstatementbuilder.h>

#include <sqliteblob.h>
#include <sqlitereadstatement.h>
#include <sqlitetable.h>
#include <sqlitetransaction.h>
#include <sqlitewritestatement.h>

#include <QBuffer>
#include <QIcon>
#include <QImage>

namespace QmlDesigner {

template<typename DatabaseType>
class ImageCacheStorage : public ImageCacheStorageInterface
{
public:
    template<int ResultCount, int BindParameterCount = 0>
    using ReadStatement = typename DatabaseType::template ReadStatement<ResultCount, BindParameterCount>;
    template<int BindParameterCount>
    using WriteStatement = typename DatabaseType::template WriteStatement<BindParameterCount>;

    ImageCacheStorage(DatabaseType &database)
        : database(database)
    {
        transaction.commit();
        database.walCheckpointFull();
    }

    ImageEntry fetchImage(Utils::SmallStringView name, Sqlite::TimeStamp minimumTimeStamp) const override
    {
        try {
            auto optionalBlob = selectImageStatement.template optionalValueWithTransaction<
                Sqlite::ByteArrayBlob>(name, minimumTimeStamp.value);

            if (optionalBlob)
                return {readImage(optionalBlob->byteArray)};

            return {};
        } catch (const Sqlite::StatementIsBusy &) {
            return fetchImage(name, minimumTimeStamp);
        }
    }

    ImageEntry fetchMidSizeImage(Utils::SmallStringView name,
                                 Sqlite::TimeStamp minimumTimeStamp) const override
    {
        try {
            auto optionalBlob = selectMidSizeImageStatement.template optionalValueWithTransaction<
                Sqlite::ByteArrayBlob>(name, minimumTimeStamp.value);

            if (optionalBlob)
                return {readImage(optionalBlob->byteArray)};

            return {};
        } catch (const Sqlite::StatementIsBusy &) {
            return fetchMidSizeImage(name, minimumTimeStamp);
        }
    }

    ImageEntry fetchSmallImage(Utils::SmallStringView name,
                               Sqlite::TimeStamp minimumTimeStamp) const override
    {
        try {
            auto optionalBlob = selectSmallImageStatement.template optionalValueWithTransaction<
                Sqlite::ByteArrayBlob>(name, minimumTimeStamp.value);

            if (optionalBlob)
                return ImageEntry{readImage(optionalBlob->byteArray)};

            return {};

        } catch (const Sqlite::StatementIsBusy &) {
            return fetchSmallImage(name, minimumTimeStamp);
        }
    }

    IconEntry fetchIcon(Utils::SmallStringView name, Sqlite::TimeStamp minimumTimeStamp) const override
    {
        try {
            auto optionalBlob = selectIconStatement.template optionalValueWithTransaction<
                Sqlite::ByteArrayBlob>(name, minimumTimeStamp.value);

            if (optionalBlob)
                return {readIcon(optionalBlob->byteArray)};

            return {};

        } catch (const Sqlite::StatementIsBusy &) {
            return fetchIcon(name, minimumTimeStamp);
        }
    }

    void storeImage(Utils::SmallStringView name,
                    Sqlite::TimeStamp newTimeStamp,
                    const QImage &image,
                    const QImage &midSizeImage,
                    const QImage &smallImage) override
    {
        try {
            auto imageBuffer = createBuffer(image);
            auto midSizeImageBuffer = createBuffer(midSizeImage);
            auto smallImageBuffer = createBuffer(smallImage);
            Sqlite::withImmediateTransaction(database, [&] {
                upsertImageStatement.write(name,
                                           newTimeStamp.value,
                                           createBlobView(imageBuffer.get()),
                                           createBlobView(midSizeImageBuffer.get()),
                                           createBlobView(smallImageBuffer.get()));
            });
        } catch (const Sqlite::StatementIsBusy &) {
            return storeImage(name, newTimeStamp, image, midSizeImage, smallImage);
        }
    }

    void storeIcon(Utils::SmallStringView name, Sqlite::TimeStamp newTimeStamp, const QIcon &icon) override
    {
        try {
            auto iconBuffer = createBuffer(icon);
            Sqlite::withImmediateTransaction(database, [&] {
                upsertIconStatement.write(name, newTimeStamp.value, createBlobView(iconBuffer.get()));
            });

        } catch (const Sqlite::StatementIsBusy &) {
            return storeIcon(name, newTimeStamp, icon);
        }
    }

    void walCheckpointFull() override
    {
        try {
            database.walCheckpointFull();
        } catch (const Sqlite::StatementIsBusy &) {
            return walCheckpointFull();
        }
    }

    Sqlite::TimeStamp fetchModifiedImageTime(Utils::SmallStringView name) const override
    {
        return selectModifiedImageTimeStatement.template valueWithTransaction<Sqlite::TimeStamp>(name);
    }

    bool fetchHasImage(Utils::SmallStringView name) const override
    {
        return selectHasImageStatement.template valueWithTransaction<int>(name);
    }

private:
    class Initializer
    {
    public:
        Initializer(DatabaseType &database)
        {
            if (!database.isInitialized()) {
                createImagesTable(database);
                database.setVersion(1);

                database.setIsInitialized(true);
            } else if (database.version() < 1) {
                updateTableToVersion1(database);
            }
        }

        void createImagesTable(DatabaseType &database)
        {
            Sqlite::Table imageTable;
            imageTable.setUseIfNotExists(true);
            imageTable.setName("images");
            imageTable.addColumn("id", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
            imageTable.addColumn("name",
                                 Sqlite::ColumnType::Text,
                                 {Sqlite::NotNull{}, Sqlite::Unique{}});
            imageTable.addColumn("mtime", Sqlite::ColumnType::Integer);
            imageTable.addColumn("image", Sqlite::ColumnType::Blob);
            imageTable.addColumn("smallImage", Sqlite::ColumnType::Blob);
            imageTable.addColumn("midSizeImage", Sqlite::ColumnType::Blob);

            imageTable.initialize(database);

            Sqlite::Table iconTable;
            iconTable.setUseIfNotExists(true);
            iconTable.setName("icons");
            iconTable.addColumn("id", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
            iconTable.addColumn("name",
                                Sqlite::ColumnType::Text,
                                {Sqlite::NotNull{}, Sqlite::Unique{}});
            iconTable.addColumn("mtime", Sqlite::ColumnType::Integer);
            iconTable.addColumn("icon", Sqlite::ColumnType::Blob);

            iconTable.initialize(database);
        }

        void updateTableToVersion1(DatabaseType &database)
        {
            database.execute("DELETE FROM images");
            database.execute("ALTER TABLE images ADD COLUMN midSizeImage");
            database.setVersion(1);
        }
    };

    Sqlite::BlobView createBlobView(QBuffer *buffer)
    {
        if (buffer)
            return Sqlite::BlobView{buffer->data()};

        return {};
    }

    static std::unique_ptr<QBuffer> createBuffer(const QImage &image)
    {
        if (image.isNull())
            return {};

        auto buffer = std::make_unique<QBuffer>();
        buffer->open(QIODevice::WriteOnly);
        QDataStream out{buffer.get()};

        out << image;

        return buffer;
    }

    static std::unique_ptr<QBuffer> createBuffer(const QIcon &icon)
    {
        if (icon.isNull())
            return {};

        auto buffer = std::make_unique<QBuffer>();
        buffer->open(QIODevice::WriteOnly);
        QDataStream out{buffer.get()};

        out << icon;

        return buffer;
    }

    static QIcon readIcon(const QByteArray &byteArray)
    {
        QIcon icon;
        QBuffer buffer;
        buffer.setData(byteArray);
        buffer.open(QIODevice::ReadOnly);
        QDataStream in{&buffer};

        in >> icon;

        return icon;
    }

    static QImage readImage(const QByteArray &byteArray)
    {
        QImage image;
        QBuffer buffer;
        buffer.setData(byteArray);
        buffer.open(QIODevice::ReadOnly);
        QDataStream in{&buffer};

        in >> image;

        return image;
    }

public:
    DatabaseType &database;
    Sqlite::ExclusiveNonThrowingDestructorTransaction<DatabaseType> transaction{database};
    Initializer initializer{database};
    mutable ReadStatement<1, 2> selectImageStatement{
        "SELECT image FROM images WHERE name=?1 AND mtime >= ?2", database};
    mutable ReadStatement<1, 2> selectMidSizeImageStatement{
        "SELECT midSizeImage FROM images WHERE name=?1 AND mtime >= ?2", database};
    mutable ReadStatement<1, 2> selectSmallImageStatement{
        "SELECT smallImage FROM images WHERE name=?1 AND mtime >= ?2", database};
    mutable ReadStatement<1, 2> selectIconStatement{
        "SELECT icon FROM icons WHERE name=?1 AND mtime >= ?2", database};
    WriteStatement<5> upsertImageStatement{
        "INSERT INTO images(name, mtime, image, midSizeImage, smallImage) "
        "VALUES (?1, ?2, ?3, ?4, ?5) "
        "ON CONFLICT(name) DO UPDATE SET mtime=excluded.mtime, image=excluded.image, "
        "midSizeImage=excluded.midSizeImage, smallImage=excluded.smallImage",
        database};
    WriteStatement<3> upsertIconStatement{
        "INSERT INTO icons(name, mtime, icon) VALUES (?1, ?2, ?3) ON "
        "CONFLICT(name) DO UPDATE SET mtime=excluded.mtime, icon=excluded.icon",
        database};
    mutable ReadStatement<1, 1> selectModifiedImageTimeStatement{
        "SELECT mtime FROM images WHERE name=?1", database};
    mutable ReadStatement<1, 1> selectHasImageStatement{
        "SELECT image IS NOT NULL FROM images WHERE name=?1", database};
};

} // namespace QmlDesigner
