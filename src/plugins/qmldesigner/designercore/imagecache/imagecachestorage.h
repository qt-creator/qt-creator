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
#include <QImageReader>
#include <QImageWriter>

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

    Entry fetchImage(Utils::SmallStringView name, Sqlite::TimeStamp minimumTimeStamp) const override
    {
        try {
            Sqlite::DeferredTransaction transaction{database};

            auto optionalBlob = selectImageStatement.template value<Sqlite::ByteArrayBlob>(
                name, minimumTimeStamp.value);

            transaction.commit();

            if (optionalBlob) {
                QBuffer buffer{&optionalBlob->byteArray};
                QImageReader reader{&buffer, "PNG"};

                return Entry{reader.read(), true};
            }

            return {};

        } catch (const Sqlite::StatementIsBusy &) {
            return fetchImage(name, minimumTimeStamp);
        }
    }

    Entry fetchIcon(Utils::SmallStringView name, Sqlite::TimeStamp minimumTimeStamp) const override
    {
        try {
            Sqlite::DeferredTransaction transaction{database};

            auto optionalBlob = selectIconStatement.template value<Sqlite::ByteArrayBlob>(
                name, minimumTimeStamp.value);

            transaction.commit();

            if (optionalBlob) {
                QBuffer buffer{&optionalBlob->byteArray};
                QImageReader reader{&buffer, "PNG"};

                return Entry{reader.read(), true};
            }

            return {};

        } catch (const Sqlite::StatementIsBusy &) {
            return fetchIcon(name, minimumTimeStamp);
        }
    }

    void storeImage(Utils::SmallStringView name, Sqlite::TimeStamp newTimeStamp, const QImage &image) override
    {
        try {
            Sqlite::ImmediateTransaction transaction{database};

            if (image.isNull()) {
                upsertImageStatement.write(name,
                                           newTimeStamp.value,
                                           Sqlite::NullValue{},
                                           Sqlite::NullValue{});
            } else {
                QSize iconSize = image.size().scaled(QSize{96, 96}.boundedTo(image.size()),
                                                     Qt::KeepAspectRatio);
                QImage icon = image.scaled(iconSize);
                upsertImageStatement.write(name,
                                           newTimeStamp.value,
                                           Sqlite::BlobView{createImageBuffer(image)->data()},
                                           Sqlite::BlobView{createImageBuffer(icon)->data()});
            }
            transaction.commit();

        } catch (const Sqlite::StatementIsBusy &) {
            return storeImage(name, newTimeStamp, image);
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
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("images");
            table.addColumn("id", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
            table.addColumn("name", Sqlite::ColumnType::Text, {Sqlite::NotNull{}, Sqlite::Unique{}});
            table.addColumn("mtime", Sqlite::ColumnType::Integer);
            table.addColumn("image", Sqlite::ColumnType::Blob);
            table.addColumn("icon", Sqlite::ColumnType::Blob);

            table.initialize(database);
        }
    };

    std::unique_ptr<QBuffer> createImageBuffer(const QImage &image)
    {
        auto buffer = std::make_unique<QBuffer>();
        buffer->open(QIODevice::WriteOnly);
        QImageWriter writer{buffer.get(), "PNG"};
        writer.write(image);

        return buffer;
    }

public:
    DatabaseType &database;
    Initializer initializer{database};
    Sqlite::ImmediateNonThrowingDestructorTransaction transaction{database};
    mutable ReadStatement selectImageStatement{
        "SELECT image FROM images WHERE name=?1 AND mtime >= ?2", database};
    mutable ReadStatement selectIconStatement{
        "SELECT icon FROM images WHERE name=?1 AND mtime >= ?2", database};
    WriteStatement upsertImageStatement{
        "INSERT INTO images(name, mtime, image, icon) VALUES (?1, ?2, ?3, ?4) ON "
        "CONFLICT(name) DO UPDATE SET mtime=excluded.mtime, image=excluded.image, "
        "icon=excluded.icon",
        database};
};

} // namespace QmlDesigner
