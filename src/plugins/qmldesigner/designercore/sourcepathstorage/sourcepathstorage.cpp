// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sourcepathstorage.h"

#include "sourcepathexceptions.h"

#include <tracing/qmldesignertracing.h>

namespace QmlDesigner {

using SourcePathStorageTracing::category;

struct SourcePathStorage::Statements
{
    Statements(Sqlite::Database &database)
        : database{database}
    {}

    Sqlite::Database &database;
    mutable Sqlite::ReadStatement<1, 1> selectSourceContextIdFromSourceContextsBySourceContextPathStatement{
        "SELECT sourceContextId FROM sourceContexts WHERE sourceContextPath = ?", database};
    mutable Sqlite::ReadStatement<1, 1> selectSourceContextPathFromSourceContextsBySourceContextIdStatement{
        "SELECT sourceContextPath FROM sourceContexts WHERE sourceContextId = ?", database};
    mutable Sqlite::ReadStatement<2> selectAllSourceContextsStatement{
        "SELECT sourceContextPath, sourceContextId FROM sourceContexts", database};
    Sqlite::WriteStatement<1> insertIntoSourceContextsStatement{
        "INSERT INTO sourceContexts(sourceContextPath) VALUES (?)", database};
    mutable Sqlite::ReadStatement<1, 1> selectSourceNameIdFromSourceNamesBySourceNameStatement{
        "SELECT sourceNameId FROM sourceNames WHERE sourceName = ?", database};
    mutable Sqlite::ReadStatement<1, 1> selectSourceNameFromSourceNamesBySourceNameIdStatement{
        "SELECT sourceName FROM sourceNames WHERE sourceNameId = ?", database};
    Sqlite::WriteStatement<1> insertIntoSourcesStatement{
        "INSERT INTO sourceNames(sourceName) VALUES (?)", database};
    mutable Sqlite::ReadStatement<2> selectAllSourcesStatement{
        "SELECT sourceName, sourceNameId  FROM sourceNames", database};
    Sqlite::WriteStatement<0> deleteAllSourceNamesStatement{"DELETE FROM sourceNames", database};
    Sqlite::WriteStatement<0> deleteAllSourceContextsStatement{"DELETE FROM sourceContexts", database};
};

class SourcePathStorage::Initializer
{
public:
    Initializer(Database &database, bool isInitialized)
    {
        if (!isInitialized) {
            createSourceContextsTable(database);
            createSourceNamesTable(database);
        }
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

    void createSourceNamesTable(Database &database)
    {
        Sqlite::StrictTable table;
        table.setUseIfNotExists(true);
        table.setName("sourceNames");
        table.addColumn("sourceNameId", Sqlite::StrictColumnType::Integer, {Sqlite::PrimaryKey{}});
        const auto &sourceNameColumn = table.addColumn("sourceName", Sqlite::StrictColumnType::Text);
        table.addUniqueIndex({sourceNameColumn});

        table.initialize(database);
    }
};

SourcePathStorage::SourcePathStorage(Database &database, bool isInitialized)
    : database{database}
    , exclusiveTransaction{database}
    , initializer{std::make_unique<SourcePathStorage::Initializer>(database, isInitialized)}
    , s{std::make_unique<SourcePathStorage::Statements>(database)}
{
    NanotraceHR::Tracer tracer{"initialize", category()};

    exclusiveTransaction.commit();
}

SourcePathStorage::~SourcePathStorage() = default;

SourceContextId SourcePathStorage::fetchSourceContextIdUnguarded(Utils::SmallStringView sourceContextPath)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch source context id unguarded", category()};

    auto sourceContextId = readSourceContextId(sourceContextPath);

    return sourceContextId ? sourceContextId : writeSourceContextId(sourceContextPath);
}

SourceContextId SourcePathStorage::fetchSourceContextId(Utils::SmallStringView sourceContextPath)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch source context id",
                               category(),
                               keyValue("source context path", sourceContextPath)};

    SourceContextId sourceContextId;
    try {
        sourceContextId = Sqlite::withDeferredTransaction(database, [&] {
            return fetchSourceContextIdUnguarded(sourceContextPath);
        });
    } catch (const Sqlite::ConstraintPreventsModification &) {
        sourceContextId = fetchSourceContextId(sourceContextPath);
    }

    tracer.end(keyValue("source context id", sourceContextId));

    return sourceContextId;
}

Utils::PathString SourcePathStorage::fetchSourceContextPath(SourceContextId sourceContextId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch source context path",
                               category(),
                               keyValue("source context id", sourceContextId)};

    auto path = Sqlite::withDeferredTransaction(database, [&] {
        auto optionalSourceContextPath = s->selectSourceContextPathFromSourceContextsBySourceContextIdStatement
                                             .optionalValue<Utils::PathString>(sourceContextId);

        if (!optionalSourceContextPath)
            throw SourceContextIdDoesNotExists();

        return std::move(*optionalSourceContextPath);
    });

    tracer.end(keyValue("source context path", path));

    return path;
}

Cache::SourceContexts SourcePathStorage::fetchAllSourceContexts() const
{
    NanotraceHR::Tracer tracer{"fetch all source contexts", category()};

    return s->selectAllSourceContextsStatement.valuesWithTransaction<Cache::SourceContext, 128>();
}

SourceNameId SourcePathStorage::fetchSourceNameId(Utils::SmallStringView sourceName)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch source id", category(), keyValue("source name", sourceName)};

    auto sourceNameId = Sqlite::withDeferredTransaction(database, [&] {
        return fetchSourceNameIdUnguarded(sourceName);
    });

    tracer.end(keyValue("source name id", sourceNameId));

    return sourceNameId;
}

Utils::SmallString SourcePathStorage::fetchSourceName(SourceNameId sourceNameId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch source name and source context id",
                               category(),
                               keyValue("source name id", sourceNameId)};

    auto sourceName = s->selectSourceNameFromSourceNamesBySourceNameIdStatement
                          .valueWithTransaction<Utils::SmallString>(sourceNameId);

    if (sourceName.empty())
        throw SourceNameIdDoesNotExists();

    tracer.end(keyValue("source name", sourceName));

    return sourceName;
}

void SourcePathStorage::clearSources()
{
    Sqlite::withImmediateTransaction(database, [&] {
        s->deleteAllSourceContextsStatement.execute();
        s->deleteAllSourceNamesStatement.execute();
    });
}

Cache::SourceNames SourcePathStorage::fetchAllSourceNames() const
{
    NanotraceHR::Tracer tracer{"fetch all sources", category()};

    return s->selectAllSourcesStatement.valuesWithTransaction<Cache::SourceName, 1024>();
}

SourceNameId SourcePathStorage::fetchSourceNameIdUnguarded(Utils::SmallStringView sourceName)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch source id unguarded",
                               category(),
                               keyValue("source name", sourceName)};

    auto sourceId = readSourceNameId(sourceName);

    if (!sourceId)
        sourceId = writeSourceNameId(sourceName);

    tracer.end(keyValue("source id", sourceId));

    return sourceId;
}

SourceContextId SourcePathStorage::readSourceContextId(Utils::SmallStringView sourceContextPath)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"read source context id",
                               category(),
                               keyValue("source context path", sourceContextPath)};

    auto sourceContextId = s->selectSourceContextIdFromSourceContextsBySourceContextPathStatement
                               .value<SourceContextId>(sourceContextPath);

    tracer.end(keyValue("source context id", sourceContextId));

    return sourceContextId;
}

SourceContextId SourcePathStorage::writeSourceContextId(Utils::SmallStringView sourceContextPath)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"write source context id",
                               category(),
                               keyValue("source context path", sourceContextPath)};

    s->insertIntoSourceContextsStatement.write(sourceContextPath);

    auto sourceContextId = SourceContextId::create(static_cast<int>(database.lastInsertedRowId()));

    tracer.end(keyValue("source context id", sourceContextId));

    return sourceContextId;
}

SourceNameId SourcePathStorage::writeSourceNameId(Utils::SmallStringView sourceName)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"write source id", category(), keyValue("source name", sourceName)};

    s->insertIntoSourcesStatement.write(sourceName);

    auto sourceNameId = SourceNameId::create(static_cast<int>(database.lastInsertedRowId()));

    tracer.end(keyValue("source name id", sourceNameId));

    return sourceNameId;
}

SourceNameId SourcePathStorage::readSourceNameId(Utils::SmallStringView sourceName)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"read source id", category(), keyValue("source name", sourceName)};

    auto sourceNameId = s->selectSourceNameIdFromSourceNamesBySourceNameStatement.value<SourceNameId>(
        sourceName);

    tracer.end(keyValue("source id", sourceNameId));

    return sourceNameId;
}

void SourcePathStorage::resetForTestsOnly()
{
    database.clearAllTablesForTestsOnly();
}
} // namespace QmlDesigner
