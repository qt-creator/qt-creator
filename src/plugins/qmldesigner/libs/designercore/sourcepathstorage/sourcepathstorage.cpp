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
    mutable Sqlite::ReadStatement<1, 1> selectDirectoryPathIdFromDirectoryPathsByDirectoryPathStatement{
        "SELECT directoryPathId FROM directoryPaths WHERE directoryPath = ?", database};
    mutable Sqlite::ReadStatement<1, 1> selectDirectoryPathFromDirectoryPathsByDirectoryPathIdStatement{
        "SELECT directoryPath FROM directoryPaths WHERE directoryPathId = ?", database};
    mutable Sqlite::ReadStatement<2> selectAllDirectoryPathsStatement{
        "SELECT directoryPath, directoryPathId FROM directoryPaths", database};
    Sqlite::WriteStatement<1> insertIntoDirectoryPathsStatement{
        "INSERT INTO directoryPaths(directoryPath) VALUES (?)", database};
    mutable Sqlite::ReadStatement<1, 1> selectFileNameIdFromFileNamesByFileNameStatement{
        "SELECT fileNameId FROM fileNames WHERE fileName = ?", database};
    mutable Sqlite::ReadStatement<1, 1> selectFileNameFromFileNamesByFileNameIdStatement{
        "SELECT fileName FROM fileNames WHERE fileNameId = ?", database};
    Sqlite::WriteStatement<1> insertIntoSourcesStatement{
        "INSERT INTO fileNames(fileName) VALUES (?)", database};
    mutable Sqlite::ReadStatement<2> selectAllSourcesStatement{
        "SELECT fileName, fileNameId  FROM fileNames", database};
    Sqlite::WriteStatement<0> deleteAllFileNamesStatement{"DELETE FROM fileNames", database};
    Sqlite::WriteStatement<0> deleteAllDirectoryPathsStatement{"DELETE FROM directoryPaths", database};
};

class SourcePathStorage::Initializer
{
public:
    Initializer(Database &database, bool isInitialized)
    {
        if (!isInitialized) {
            createDirectoryPathsTable(database);
            createFileNamesTable(database);
        }
    }

    void createDirectoryPathsTable(Database &database)
    {
        Sqlite::Table table;
        table.setUseIfNotExists(true);
        table.setName("directoryPaths");
        table.addColumn("directoryPathId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
        const Sqlite::Column &directoryPathColumn = table.addColumn("directoryPath");

        table.addUniqueIndex({directoryPathColumn});

        table.initialize(database);
    }

    void createFileNamesTable(Database &database)
    {
        Sqlite::StrictTable table;
        table.setUseIfNotExists(true);
        table.setName("fileNames");
        table.addColumn("fileNameId", Sqlite::StrictColumnType::Integer, {Sqlite::PrimaryKey{}});
        const auto &fileNameColumn = table.addColumn("fileName", Sqlite::StrictColumnType::Text);
        table.addUniqueIndex({fileNameColumn});

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

DirectoryPathId SourcePathStorage::fetchDirectoryPathIdUnguarded(Utils::SmallStringView directoryPath)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch source context id unguarded", category()};

    auto directoryPathId = readDirectoryPathId(directoryPath);

    return directoryPathId ? directoryPathId : writeDirectoryPathId(directoryPath);
}

DirectoryPathId SourcePathStorage::fetchDirectoryPathId(Utils::SmallStringView directoryPath)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch source context id",
                               category(),
                               keyValue("source context path", directoryPath)};

    DirectoryPathId directoryPathId;
    try {
        directoryPathId = Sqlite::withDeferredTransaction(database, [&] {
            return fetchDirectoryPathIdUnguarded(directoryPath);
        });
    } catch (const Sqlite::ConstraintPreventsModification &) {
        directoryPathId = fetchDirectoryPathId(directoryPath);
    }

    tracer.end(keyValue("source context id", directoryPathId));

    return directoryPathId;
}

Utils::PathString SourcePathStorage::fetchDirectoryPath(DirectoryPathId directoryPathId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch source context path",
                               category(),
                               keyValue("source context id", directoryPathId)};

    auto path = Sqlite::withDeferredTransaction(database, [&] {
        auto optionalDirectoryPath = s->selectDirectoryPathFromDirectoryPathsByDirectoryPathIdStatement
                                             .optionalValue<Utils::PathString>(directoryPathId);

        if (!optionalDirectoryPath)
            throw DirectoryPathIdDoesNotExists();

        return std::move(*optionalDirectoryPath);
    });

    tracer.end(keyValue("source context path", path));

    return path;
}

Cache::DirectoryPaths SourcePathStorage::fetchAllDirectoryPaths() const
{
    NanotraceHR::Tracer tracer{"fetch all source contexts", category()};

    return s->selectAllDirectoryPathsStatement.valuesWithTransaction<Cache::DirectoryPath, 128>();
}

FileNameId SourcePathStorage::fetchFileNameId(Utils::SmallStringView fileName)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch source id", category(), keyValue("source name", fileName)};

    auto fileNameId = Sqlite::withDeferredTransaction(database, [&] {
        return fetchFileNameIdUnguarded(fileName);
    });

    tracer.end(keyValue("source name id", fileNameId));

    return fileNameId;
}

Utils::SmallString SourcePathStorage::fetchFileName(FileNameId fileNameId) const
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch source name and source context id",
                               category(),
                               keyValue("source name id", fileNameId)};

    auto fileName = s->selectFileNameFromFileNamesByFileNameIdStatement
                          .valueWithTransaction<Utils::SmallString>(fileNameId);

    if (fileName.empty())
        throw FileNameIdDoesNotExists();

    tracer.end(keyValue("source name", fileName));

    return fileName;
}

void SourcePathStorage::clearSources()
{
    Sqlite::withImmediateTransaction(database, [&] {
        s->deleteAllDirectoryPathsStatement.execute();
        s->deleteAllFileNamesStatement.execute();
    });
}

Cache::FileNames SourcePathStorage::fetchAllFileNames() const
{
    NanotraceHR::Tracer tracer{"fetch all sources", category()};

    return s->selectAllSourcesStatement.valuesWithTransaction<Cache::FileName, 1024>();
}

FileNameId SourcePathStorage::fetchFileNameIdUnguarded(Utils::SmallStringView fileName)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"fetch source id unguarded",
                               category(),
                               keyValue("source name", fileName)};

    auto sourceId = readFileNameId(fileName);

    if (!sourceId)
        sourceId = writeFileNameId(fileName);

    tracer.end(keyValue("source id", sourceId));

    return sourceId;
}

DirectoryPathId SourcePathStorage::readDirectoryPathId(Utils::SmallStringView directoryPath)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"read source context id",
                               category(),
                               keyValue("source context path", directoryPath)};

    auto directoryPathId = s->selectDirectoryPathIdFromDirectoryPathsByDirectoryPathStatement
                               .value<DirectoryPathId>(directoryPath);

    tracer.end(keyValue("source context id", directoryPathId));

    return directoryPathId;
}

DirectoryPathId SourcePathStorage::writeDirectoryPathId(Utils::SmallStringView directoryPath)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"write source context id",
                               category(),
                               keyValue("source context path", directoryPath)};

    s->insertIntoDirectoryPathsStatement.write(directoryPath);

    auto directoryPathId = DirectoryPathId::create(static_cast<int>(database.lastInsertedRowId()));

    tracer.end(keyValue("source context id", directoryPathId));

    return directoryPathId;
}

FileNameId SourcePathStorage::writeFileNameId(Utils::SmallStringView fileName)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"write source id", category(), keyValue("source name", fileName)};

    s->insertIntoSourcesStatement.write(fileName);

    auto fileNameId = FileNameId::create(static_cast<int>(database.lastInsertedRowId()));

    tracer.end(keyValue("source name id", fileNameId));

    return fileNameId;
}

FileNameId SourcePathStorage::readFileNameId(Utils::SmallStringView fileName)
{
    using NanotraceHR::keyValue;
    NanotraceHR::Tracer tracer{"read source id", category(), keyValue("source name", fileName)};

    auto fileNameId = s->selectFileNameIdFromFileNamesByFileNameStatement.value<FileNameId>(
        fileName);

    tracer.end(keyValue("source id", fileNameId));

    return fileNameId;
}

void SourcePathStorage::resetForTestsOnly()
{
    database.clearAllTablesForTestsOnly();
}
} // namespace QmlDesigner
