/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
    using ReadStatement = typename DatabaseType::ReadStatement;
    using WriteStatement = typename DatabaseType::WriteStatement;

    ImageCacheStorage(DatabaseType &database)
        : database(database)
    {
        transaction.commit();
    }

    ImageEntry fetchImage(Utils::SmallStringView name, Sqlite::TimeStamp minimumTimeStamp) const override
    {
        try {
            Sqlite::DeferredTransaction transaction{database};

            auto optionalBlob = selectImageStatement.template value<Sqlite::ByteArrayBlob>(
                name, minimumTimeStamp.value);

            transaction.commit();

            if (optionalBlob)
                return {readImage(optionalBlob->byteArray), true};

            return {};
        } catch (const Sqlite::StatementIsBusy &) {
            return fetchImage(name, minimumTimeStamp);
        }
    }

    ImageEntry fetchSmallImage(Utils::SmallStringView name,
                               Sqlite::TimeStamp minimumTimeStamp) const override
    {
        try {
            Sqlite::DeferredTransaction transaction{database};

            auto optionalBlob = selectSmallImageStatement.template value<Sqlite::ByteArrayBlob>(
                name, minimumTimeStamp.value);

            transaction.commit();

            if (optionalBlob)
                return ImageEntry{readImage(optionalBlob->byteArray), true};

            return {};

        } catch (const Sqlite::StatementIsBusy &) {
            return fetchSmallImage(name, minimumTimeStamp);
        }
    }

    IconEntry fetchIcon(Utils::SmallStringView name, Sqlite::TimeStamp minimumTimeStamp) const override
    {
        try {
            Sqlite::DeferredTransaction transaction{database};

            auto optionalBlob = selectIconStatement.template value<Sqlite::ByteArrayBlob>(
                name, minimumTimeStamp.value);

            transaction.commit();

            if (optionalBlob)
                return {readIcon(optionalBlob->byteArray), true};

            return {};

        } catch (const Sqlite::StatementIsBusy &) {
            return fetchIcon(name, minimumTimeStamp);
        }
    }

    void storeImage(Utils::SmallStringView name,
                    Sqlite::TimeStamp newTimeStamp,
                    const QImage &image,
                    const QImage &smallImage) override
    {
        try {
            Sqlite::ImmediateTransaction transaction{database};

            auto imageBuffer = createBuffer(image);
            auto smallImageBuffer = createBuffer(smallImage);
            upsertImageStatement.write(name,
                                       newTimeStamp.value,
                                       createBlobView(imageBuffer.get()),
                                       createBlobView(smallImageBuffer.get()));

            transaction.commit();

        } catch (const Sqlite::StatementIsBusy &) {
            return storeImage(name, newTimeStamp, image, smallImage);
        }
    }

    void storeIcon(Utils::SmallStringView name, Sqlite::TimeStamp newTimeStamp, const QIcon &icon) override
    {
        try {
            Sqlite::ImmediateTransaction transaction{database};

            auto iconBuffer = createBuffer(icon);
            upsertIconStatement.write(name, newTimeStamp.value, createBlobView(iconBuffer.get()));

            transaction.commit();

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

private:
    class Initializer
    {
    public:
        Initializer(DatabaseType &database)
        {
            if (!database.isInitialized()) {
                Sqlite::ExclusiveTransaction transaction{database};

                createImagesTable(database);

                transaction.commit();

                database.setIsInitialized(true);

                database.walCheckpointFull();
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
    Initializer initializer{database};
    Sqlite::ImmediateNonThrowingDestructorTransaction transaction{database};
    mutable ReadStatement selectImageStatement{
        "SELECT image FROM images WHERE name=?1 AND mtime >= ?2", database};
    mutable ReadStatement selectSmallImageStatement{
        "SELECT smallImage FROM images WHERE name=?1 AND mtime >= ?2", database};
    mutable ReadStatement selectIconStatement{
        "SELECT icon FROM icons WHERE name=?1 AND mtime >= ?2", database};
    WriteStatement upsertImageStatement{
        "INSERT INTO images(name, mtime, image, smallImage) VALUES (?1, ?2, ?3, ?4) ON "
        "CONFLICT(name) DO UPDATE SET mtime=excluded.mtime, image=excluded.image, "
        "smallImage=excluded.smallImage",
        database};
    WriteStatement upsertIconStatement{
        "INSERT INTO icons(name, mtime, icon) VALUES (?1, ?2, ?3) ON "
        "CONFLICT(name) DO UPDATE SET mtime=excluded.mtime, icon=excluded.icon",
        database};
};

} // namespace QmlDesigner
