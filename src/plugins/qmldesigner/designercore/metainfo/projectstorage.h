/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#pragma once

#include "projectstorageexceptions.h"
#include "projectstorageids.h"
#include "projectstoragetypes.h"

#include <sqlitetable.h>
#include <sqlitetransaction.h>

#include <utils/optional.h>

#include <tuple>

namespace QmlDesigner {

template<typename Database>
class ProjectStorage
{
public:
    template<int ResultCount>
    using ReadStatement = typename Database::template ReadStatement<ResultCount>;
    template<int ResultCount>
    using ReadWriteStatement = typename Database::template ReadWriteStatement<ResultCount>;
    using WriteStatement = typename Database::WriteStatement;

    ProjectStorage(Database &database, bool isInitialized)
        : database{database}
        , initializer{database, isInitialized}
    {}

    template<typename Container>
    TypeId upsertType(Utils::SmallStringView name,
                      TypeId prototype,
                      TypeAccessSemantics accessSemantics,
                      const Container &qualifiedNames)
    {
        Sqlite::ImmediateTransaction transaction{database};

        auto typeId = upsertTypeStatement.template value<TypeId>(name,
                                                                 static_cast<long long>(accessSemantics),
                                                                 &prototype);

        for (Utils::SmallStringView qualifiedName : qualifiedNames)
            upsertQualifiedTypeNameStatement.write(qualifiedName, &typeId);

        transaction.commit();

        return typeId;
    }

    PropertyDeclarationId upsertPropertyDeclaration(TypeId typeId,
                                                    Utils::SmallStringView name,
                                                    TypeId propertyTypeId)
    {
        Sqlite::ImmediateTransaction transaction{database};

        auto propertyDeclarationId = upsertPropertyDeclarationStatement
                                         .template value<PropertyDeclarationId>(&typeId,
                                                                                name,
                                                                                &propertyTypeId);

        transaction.commit();

        return propertyDeclarationId;
    }

    PropertyDeclarationId fetchPropertyDeclarationByTypeIdAndName(TypeId typeId,
                                                                  Utils::SmallStringView name)
    {
        return selectPropertyDeclarationByTypeIdAndNameStatement
            .template valueWithTransaction<PropertyDeclarationId>(&typeId, name);
    }

    TypeId fetchTypeIdByQualifiedName(Utils::SmallStringView name)
    {
        return selectTypeIdByQualifiedNameStatement.template valueWithTransaction<TypeId>(name);
    }

    bool fetchIsProtype(TypeId type, TypeId prototype)
    {
        return bool(
            selectPrototypeIdStatement.template valueWithTransaction<TypeId>(&type, &prototype));
    }

    auto fetchPrototypes(TypeId type)
    {
        return selectPrototypeIdsStatement.template rangeWithTransaction<TypeId>(&type);
    }

    SourceContextId fetchSourceContextIdUnguarded(Utils::SmallStringView sourceContextPath)
    {
        auto sourceContextId = readSourceContextId(sourceContextPath);

        return sourceContextId ? sourceContextId : writeSourceContextId(sourceContextPath);
    }

    SourceContextId fetchSourceContextId(Utils::SmallStringView sourceContextPath)
    {
        try {
            Sqlite::DeferredTransaction transaction{database};

            auto sourceContextId = fetchSourceContextIdUnguarded(sourceContextPath);

            transaction.commit();

            return sourceContextId;
        } catch (const Sqlite::ConstraintPreventsModification &) {
            return fetchSourceContextId(sourceContextPath);
        }
    }

    Utils::PathString fetchSourceContextPath(SourceContextId sourceContextId) const
    {
        Sqlite::DeferredTransaction transaction{database};

        auto optionalSourceContextPath = selectSourceContextPathFromSourceContextsBySourceContextIdStatement
                                             .template optionalValue<Utils::PathString>(
                                                 &sourceContextId);

        if (!optionalSourceContextPath)
            throw SourceContextIdDoesNotExists();

        transaction.commit();

        return std::move(*optionalSourceContextPath);
    }

    auto fetchAllSourceContexts() const
    {
        return selectAllSourceContextsStatement.template rangeWithTransaction<Sources::SourceContext>();
    }

    SourceId fetchSourceId(SourceContextId sourceContextId, Utils::SmallStringView sourceName)
    {
        Sqlite::DeferredTransaction transaction{database};

        auto sourceId = fetchSourceIdUnguarded(sourceContextId, sourceName);

        transaction.commit();

        return sourceId;
    }

    Sources::SourceNameAndSourceContextId fetchSourceNameAndSourceContextId(SourceId sourceId) const
    {
        auto value = selectSourceNameAndSourceContextIdFromSourcesBySourceIdStatement
                         .template valueWithTransaction<Sources::SourceNameAndSourceContextId>(
                             &sourceId);

        if (!value.sourceContextId)
            throw SourceIdDoesNotExists();

        return value;
    }

    SourceContextId fetchSourceContextId(SourceId sourceId) const
    {
        auto sourceContextId = selectSourceContextIdFromSourcesBySourceIdStatement
                                   .template valueWithTransaction<SourceContextId>(sourceId.id);

        if (!sourceContextId)
            throw SourceIdDoesNotExists();

        return sourceContextId;
    }

    auto fetchAllSources() const
    {
        return selectAllSourcesStatement.template rangeWithTransaction<Sources::Source>();
    }

    SourceId fetchSourceIdUnguarded(SourceContextId sourceContextId, Utils::SmallStringView sourceName)
    {
        auto sourceId = readSourceId(sourceContextId, sourceName);

        if (sourceId)
            return sourceId;

        return writeSourceId(sourceContextId, sourceName);
    }

private:
    SourceContextId readSourceContextId(Utils::SmallStringView sourceContextPath)
    {
        return selectSourceContextIdFromSourceContextsBySourceContextPathStatement
            .template value<SourceContextId>(sourceContextPath);
    }

    SourceContextId writeSourceContextId(Utils::SmallStringView sourceContextPath)
    {
        insertIntoSourceContextsStatement.write(sourceContextPath);

        return SourceContextId(database.lastInsertedRowId());
    }

    SourceId writeSourceId(SourceContextId sourceContextId, Utils::SmallStringView sourceName)
    {
        insertIntoSourcesStatement.write(&sourceContextId, sourceName);

        return SourceId(database.lastInsertedRowId());
    }

    SourceId readSourceId(SourceContextId sourceContextId, Utils::SmallStringView sourceName)
    {
        return selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatement
            .template value<SourceId>(&sourceContextId, sourceName);
    }

    class Initializer
    {
    public:
        Initializer(Database &database, bool isInitialized)
        {
            if (!isInitialized) {
                Sqlite::ExclusiveTransaction transaction{database};

                createTypesTable(database);
                createQualifiedTypeNamesTable(database);
                createPropertyDeclarationsTable(database);
                createEnumValuesTable(database);
                createMethodsTable(database);
                createSignalsTable(database);
                createSourceContextsTable(database);
                createSourcesTable(database);

                transaction.commit();

                database.walCheckpointFull();
            }
        }

        void createPropertyDeclarationsTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("propertyDeclarations");
            table.addColumn("propertyDeclarationId",
                            Sqlite::ColumnType::Integer,
                            {Sqlite::PrimaryKey{}});
            auto &typeIdColumn = table.addColumn("typeId");
            auto &nameColumn = table.addColumn("name");
            table.addColumn("propertyTypeId");

            table.addUniqueIndex({typeIdColumn, nameColumn});

            table.initialize(database);
        }

        void createTypesTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("types");
            table.addColumn("typeId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
            auto &nameColumn = table.addColumn("name");
            table.addColumn("accessSemantics");
            table.addColumn("prototype");
            table.addColumn("defaultProperty");

            table.addUniqueIndex({nameColumn});

            table.initialize(database);
        }

        void createQualifiedTypeNamesTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setUseWithoutRowId(true);
            table.setName("qualifiedTypeNames");
            auto &qualifiedNameColumn = table.addColumn("qualifiedName");
            table.addColumn("typeId");

            table.addPrimaryKeyContraint({qualifiedNameColumn});

            table.initialize(database);
        }

        void createEnumValuesTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("enumerationValues");
            table.addColumn("enumerationValueId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
            auto &enumIdColumn = table.addColumn("typeId");
            auto &nameColumn = table.addColumn("name");

            table.addUniqueIndex({enumIdColumn, nameColumn});

            table.initialize(database);
        }

        void createMethodsTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("methods");
            table.addColumn("methodId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
            auto &nameColumn = table.addColumn("name");

            table.addUniqueIndex({nameColumn});

            table.initialize(database);
        }

        void createSignalsTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("signals");
            table.addColumn("signalId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
            auto &nameColumn = table.addColumn("name");

            table.addUniqueIndex({nameColumn});

            table.initialize(database);
        }

        void createSourceContextsTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("sourceContexts");
            table.addColumn("sourceContextId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
            const Sqlite::Column &sourceContextPathColumn = table.addColumn("sourceContextPath");

            table.addUniqueIndex({sourceContextPathColumn});

            table.initialize(database);
        }

        void createSourcesTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("sources");
            table.addColumn("sourceId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
            const Sqlite::Column &sourceContextIdColumn = table.addColumn(
                "sourceContextId",
                Sqlite::ColumnType::Integer,
                {Sqlite::NotNull{},
                 Sqlite::ForeignKey{"sourceContexts",
                                    "sourceContextId",
                                    Sqlite::ForeignKeyAction::NoAction,
                                    Sqlite::ForeignKeyAction::Cascade}});
            const Sqlite::Column &sourceNameColumn = table.addColumn("sourceName",
                                                                     Sqlite::ColumnType::Text);
            table.addUniqueIndex({sourceContextIdColumn, sourceNameColumn});

            table.initialize(database);
        }
    };

public:
    Database &database;
    Initializer initializer;
    ReadWriteStatement<1> upsertTypeStatement{
        "INSERT INTO types(name,  accessSemantics, prototype) VALUES(?1, ?2, nullif(?3, -1)) ON "
        "CONFLICT DO UPDATE SET "
        "prototype=excluded.prototype, accessSemantics=excluded.accessSemantics RETURNING typeId",
        database};
    mutable ReadStatement<1> selectTypeIdByQualifiedNameStatement{
        "SELECT typeId FROM qualifiedTypeNames WHERE qualifiedName=?", database};
    mutable ReadStatement<1> selectPrototypeIdStatement{
        "WITH RECURSIVE "
        "  typeSelection(typeId) AS ("
        "      VALUES(?1) "
        "    UNION ALL "
        "      SELECT prototype FROM types JOIN typeSelection USING(typeId)) "
        "SELECT typeId FROM typeSelection WHERE typeId=?2 LIMIT 1",
        database};
    ReadWriteStatement<1> upsertPropertyDeclarationStatement{
        "INSERT INTO propertyDeclarations(typeId, name, propertyTypeId) VALUES(?1, ?2, ?3) ON "
        "CONFLICT DO UPDATE SET "
        "typeId=excluded.typeId, name=excluded.name, propertyTypeId=excluded.propertyTypeId  "
        "RETURNING propertyDeclarationId",
        database};
    mutable ReadStatement<1> selectPropertyDeclarationByTypeIdAndNameStatement{
        "WITH RECURSIVE "
        "  typeSelection(typeId) AS ("
        "      VALUES(?1) "
        "    UNION ALL "
        "      SELECT prototype FROM types JOIN typeSelection USING(typeId)) "
        "SELECT propertyDeclarationId FROM propertyDeclarations JOIN typeSelection USING(typeId) "
        "  WHERE name=?2 LIMIT 1",
        database};
    WriteStatement upsertQualifiedTypeNameStatement{
        "INSERT INTO qualifiedTypeNames(qualifiedName,  typeId) VALUES(?1, ?2) ON CONFLICT DO "
        "UPDATE SET typeId=excluded.typeId",
        database};
    mutable ReadStatement<1> selectAccessSemanticsStatement{
        "SELECT typeId FROM qualifiedTypeNames WHERE qualifiedName=?", database};
    mutable ReadStatement<1> selectPrototypeIdsStatement{
        "WITH RECURSIVE "
        "  typeSelection(typeId) AS ("
        "      VALUES(?1) "
        "    UNION ALL "
        "      SELECT prototype FROM types JOIN typeSelection USING(typeId)) "
        "SELECT typeId FROM typeSelection",
        database};
    mutable ReadStatement<1> selectSourceContextIdFromSourceContextsBySourceContextPathStatement{
        "SELECT sourceContextId FROM sourceContexts WHERE sourceContextPath = ?", database};
    mutable ReadStatement<1> selectSourceContextPathFromSourceContextsBySourceContextIdStatement{
        "SELECT sourceContextPath FROM sourceContexts WHERE sourceContextId = ?", database};
    mutable ReadStatement<2> selectAllSourceContextsStatement{
        "SELECT sourceContextPath, sourceContextId FROM sourceContexts", database};
    WriteStatement insertIntoSourceContextsStatement{
        "INSERT INTO sourceContexts(sourceContextPath) VALUES (?)", database};
    mutable ReadStatement<1> selectSourceIdFromSourcesBySourceContextIdAndSourceNameStatement{
        "SELECT sourceId FROM sources WHERE sourceContextId = ? AND sourceName = ?", database};
    mutable ReadStatement<2> selectSourceNameAndSourceContextIdFromSourcesBySourceIdStatement{
        "SELECT sourceName, sourceContextId FROM sources WHERE sourceId = ?", database};
    mutable ReadStatement<1> selectSourceContextIdFromSourcesBySourceIdStatement{
        "SELECT sourceContextId FROM sources WHERE sourceId = ?", database};
    WriteStatement insertIntoSourcesStatement{
        "INSERT INTO sources(sourceContextId, sourceName) VALUES (?,?)", database};
    mutable ReadStatement<3> selectAllSourcesStatement{
        "SELECT sourceName, sourceContextId, sourceId  FROM sources", database};
};

} // namespace QmlDesigner
