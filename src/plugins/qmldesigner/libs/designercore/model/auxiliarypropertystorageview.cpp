// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "auxiliarypropertystorageview.h"

#include <sqlitedatabase.h>

#include <QDataStream>
#include <QIODevice>

namespace QmlDesigner {

namespace {
struct Initializer
{
    Initializer(Sqlite::Database &database, bool isInitialized)
    {
        if (!isInitialized) {
            createAuxiliaryPropertiesTable(database);
        }
        database.setIsInitialized(true);
    }

    void createAuxiliaryPropertiesTable(Sqlite::Database &database)
    {
        Sqlite::StrictTable table;
        table.setUseIfNotExists(true);
        table.setUseWithoutRowId(true);
        table.setName("auxiliaryProperties");
        auto &filePathColumn = table.addColumn("filePath", Sqlite::StrictColumnType::Text);
        auto &idColumn = table.addColumn("id", Sqlite::StrictColumnType::Text);
        auto &keyColumn = table.addColumn("key", Sqlite::StrictColumnType::Text);
        table.addColumn("value", Sqlite::StrictColumnType::Blob);

        table.addPrimaryKeyContraint({filePathColumn, idColumn, keyColumn});

        table.initialize(database);
    }
};

struct Entry
{
    Entry(Utils::SmallStringView nodeId, Utils::SmallStringView key, Sqlite::BlobView value)
        : nodeId{nodeId}
        , key{key}
        , value{value}
    {}

    Utils::SmallStringView nodeId;
    Utils::SmallStringView key;
    Sqlite::BlobView value;
};

struct ChangeEntry
{
    ChangeEntry(Utils::SmallStringView id, Utils::SmallStringView name)
        : id{id}
        , name{name}
    {}

    friend bool operator<(const ChangeEntry &first, const ChangeEntry &second)
    {
        return std::tie(first.id, first.name) < std::tie(second.id, second.name);
    }

    Utils::SmallString id;
    Utils::SmallString name;
};

} // namespace

struct AuxiliaryPropertyStorageView::Data
{
    Data(Sqlite::Database &database)
        : database{database}
    {
        exclusiveTransaction.commit();
    }

    Sqlite::Database &database;
    Sqlite::ExclusiveNonThrowingDestructorTransaction<Sqlite::Database> exclusiveTransaction{database};
    Initializer initializer{database, database.isInitialized()};
    Sqlite::WriteStatement<4> upsertValue{
        "INSERT INTO auxiliaryProperties(filePath, id, key, value) "
        "VALUES(?1, ?2, ?3, ?4) "
        "ON CONFLICT DO UPDATE SET value=excluded.value",
        database};
    Sqlite::ReadStatement<3, 1> selectValues{
        "SELECT id, key, value FROM auxiliaryProperties WHERE filePath=?1", database};
    Sqlite::WriteStatement<0> removeValues{"DELETE FROM auxiliaryProperties", database};
    Sqlite::WriteStatement<3> removeValue{
        "DELETE FROM auxiliaryProperties WHERE filePath=?1 AND id=?2 AND key=?3", database};
    std::set<ChangeEntry, std::less<>> changeEntries;
};

AuxiliaryPropertyStorageView::AuxiliaryPropertyStorageView(
    Sqlite::Database &database, ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
    , d{std::make_unique<Data>(database)}
{}

AuxiliaryPropertyStorageView::~AuxiliaryPropertyStorageView() = default;

void AuxiliaryPropertyStorageView::modelAttached(Model *model)
{
    auto entryRange = d->selectValues.rangeWithTransaction<Entry>(
        Utils::PathString{model->fileUrl().path()});

    for (const auto entry : entryRange) {
        auto node = model->modelNodeForId(QString{entry.nodeId});

        if (!node)
            continue;

        auto array = QByteArray::fromRawData(entry.value.cdata(), entry.value.ssize());
        QDataStream dataStream{array};

        QVariant value;
        dataStream >> value;

        node.setAuxiliaryDataWithoutLock({AuxiliaryDataType::Persistent, entry.key}, value);
    }

    AbstractView::modelAttached(model);
}

void AuxiliaryPropertyStorageView::modelAboutToBeDetached(Model *model)
{
    Utils::PathString filePath{model->fileUrl().path()};

    auto update = [&] {
        for (const auto &[id, name] : d->changeEntries) {
            QByteArray array;
            QDataStream dataStream{&array, QIODevice::WriteOnly};
            auto node = model->modelNodeForId(id.toQString());

            auto value = node.auxiliaryData(AuxiliaryDataType::Persistent, name);
            if (value) {
                dataStream << *value;

                Sqlite::BlobView blob{array};

                d->upsertValue.write(filePath, id, name, blob);

            } else {
                d->removeValue.write(filePath, id, name);
            }
        }
    };

    if (d->changeEntries.size()) {
        Sqlite::withImmediateTransaction(d->database, update);

        d->changeEntries.clear();
    }

    AbstractView::modelAboutToBeDetached(model);
}

void AuxiliaryPropertyStorageView::nodeIdChanged(const ModelNode &node,
                                                 const QString &newIdArg,
                                                 const QString &oldIdArg)
{
    Utils::SmallString newId = newIdArg;
    Utils::SmallString oldId = oldIdArg;
    for (auto entry : node.auxiliaryData()) {
        if (entry.first.type != AuxiliaryDataType::Persistent)
            continue;

        d->changeEntries.emplace(oldId, entry.first.name);
        d->changeEntries.emplace(newId, entry.first.name);
    }
}

void AuxiliaryPropertyStorageView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    if (removedNode.hasId()) {
        Utils::SmallString id = removedNode.id();
        for (auto entry : removedNode.auxiliaryData()) {
            if (entry.first.type != AuxiliaryDataType::Persistent)
                continue;

            d->changeEntries.emplace(id, entry.first.name);
        }
    }
}

void AuxiliaryPropertyStorageView::auxiliaryDataChanged(const ModelNode &node,
                                                        AuxiliaryDataKeyView key,
                                                        const QVariant &)
{
    if (isAttached() && key.type == AuxiliaryDataType::Persistent)
        d->changeEntries.emplace(Utils::SmallString{node.id()}, key.name);
}

void AuxiliaryPropertyStorageView::resetForTestsOnly()
{
    Sqlite::withImmediateTransaction(d->database, [&] { d->removeValues.execute(); });
}

} // namespace QmlDesigner
