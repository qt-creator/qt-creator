/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "builddependenciesstorageinterface.h"

#include <compilermacro.h>
#include <sqliteexception.h>
#include <sqlitetable.h>
#include <sqlitetransaction.h>

#include <utils/smallstringview.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace ClangBackEnd {

template<typename Database=Sqlite::Database>
class BuildDependenciesStorage final : public BuildDependenciesStorageInterface
{
    using ReadStatement = typename Database::ReadStatement;
    using WriteStatement = typename Database::WriteStatement;
public:
    BuildDependenciesStorage(Database &database)
        : transaction(database),
          database(database)
    {
        transaction.commit();
    }

    void insertOrUpdateSources(const SourceEntries &sourceEntries, ProjectPartId projectPartId) override
    {
        deleteAllProjectPartsFilesWithProjectPartNameStatement.write(projectPartId.projectPathId);

        for (const SourceEntry &entry : sourceEntries) {
            insertOrUpdateProjectPartsFilesStatement.write(entry.sourceId.filePathId,
                                                             projectPartId.projectPathId,
                                                             static_cast<uchar>(entry.sourceType),
                                                             static_cast<uchar>(
                                                                 entry.hasMissingIncludes));
        }
    }

    FilePathIds fetchPchSources(ProjectPartId projectPartId) const override
    {
        try {
            Sqlite::DeferredTransaction transaction{database};

            FilePathIds values = fetchPchSourcesStatement
                                     .template values<FilePathId>(1024, projectPartId.projectPathId);

            transaction.commit();

            return values;
        } catch (const Sqlite::StatementIsBusy &) {
            return fetchPchSources(projectPartId);
        }
    }

    FilePathIds fetchSources(ProjectPartId projectPartId) const override
    {
        return fetchSourcesStatement.template values<FilePathId>(1024, projectPartId.projectPathId);
    }

    void insertOrUpdateFileStatuses(const FileStatuses &fileStatuses) override
    {
        WriteStatement &statement = insertOrUpdateFileStatusesStatement;

        for (const FileStatus &fileStatus : fileStatuses)
            statement.write(fileStatus.filePathId.filePathId,
                            fileStatus.size,
                            fileStatus.lastModified);
    }

    long long fetchLowestLastModifiedTime(FilePathId sourceId) const override
    {
        ReadStatement &statement = getLowestLastModifiedTimeOfDependencies;

        return statement.template value<long long>(sourceId.filePathId).value_or(0);
    }

    void insertOrUpdateUsedMacros(const UsedMacros &usedMacros) override
    {
        WriteStatement &insertStatement = insertIntoNewUsedMacrosStatement;
        for (const UsedMacro &usedMacro : usedMacros)
            insertStatement.write(usedMacro.filePathId.filePathId,
                                  usedMacro.macroName);

        syncNewUsedMacrosStatement.execute();
        deleteOutdatedUsedMacrosStatement.execute();
        deleteNewUsedMacrosTableStatement.execute();
    }

    void insertOrUpdateSourceDependencies(const SourceDependencies &sourceDependencies) override
    {
        WriteStatement &insertStatement = insertIntoNewSourceDependenciesStatement;
        for (SourceDependency sourceDependency : sourceDependencies)
            insertStatement.write(sourceDependency.filePathId.filePathId,
                                  sourceDependency.dependencyFilePathId.filePathId);

        syncNewSourceDependenciesStatement.execute();
        deleteOutdatedSourceDependenciesStatement.execute();
        deleteNewSourceDependenciesStatement.execute();
    }

    ProjectPartId fetchProjectPartId(Utils::SmallStringView projectPartName) override
    {
        auto projectPartId = fetchProjectPartIdStatement.template value<ProjectPartId>(projectPartName);

        if (projectPartId)
            return *projectPartId;

        insertProjectPartNameStatement.write(projectPartName);

        return static_cast<int>(database.lastInsertedRowId());
    }

    SourceEntries fetchDependSources(FilePathId sourceId, ProjectPartId projectPartId) const override
    {
        return fetchSourceDependenciesStatement
            .template values<SourceEntry, 4>(300, sourceId.filePathId, projectPartId.projectPathId);
    }

    UsedMacros fetchUsedMacros(FilePathId sourceId) const override
    {
        return fetchUsedMacrosStatement.template values<UsedMacro, 2>(128, sourceId.filePathId);
    }

    void updatePchCreationTimeStamp(long long pchCreationTimeStamp, ProjectPartId projectPartId) override
    {
        Sqlite::ImmediateTransaction transaction{database};

        updatePchCreationTimeStampStatement.write(pchCreationTimeStamp, projectPartId.projectPathId);

        transaction.commit();
    }

    void insertOrUpdateIndexingTimeStamps(const FilePathIds &filePathIds,
                                          TimeStamp indexingTimeStamp) override
    {
        try {
            Sqlite::ImmediateTransaction transaction{database};

            for (FilePathId filePathId : filePathIds) {
                inserOrUpdateIndexingTimesStampStatement.write(filePathId.filePathId,
                                                               indexingTimeStamp.value);
            }

            transaction.commit();
        } catch (const Sqlite::StatementIsBusy &) {
            insertOrUpdateIndexingTimeStamps(filePathIds, indexingTimeStamp);
        }
    }

    void insertOrUpdateIndexingTimeStampsWithoutTransaction(const FilePathIds &filePathIds,
                                                            TimeStamp indexingTimeStamp) override
    {
        for (FilePathId filePathId : filePathIds) {
            inserOrUpdateIndexingTimesStampStatement.write(filePathId.filePathId,
                                                           indexingTimeStamp.value);
        }
    }

    SourceTimeStamps fetchIndexingTimeStamps() const override
    {
        try {
            Sqlite::DeferredTransaction transaction{database};

            auto timeStamps = fetchIndexingTimeStampsStatement.template values<SourceTimeStamp, 2>(
                1024);

            transaction.commit();

            return timeStamps;
        } catch (const Sqlite::StatementIsBusy &) {
            return fetchIndexingTimeStamps();
        }
    }

    SourceTimeStamps fetchIncludedIndexingTimeStamps(FilePathId sourcePathId) const override
    {
        try {
            Sqlite::DeferredTransaction transaction{database};

            auto timeStamps = fetchIncludedIndexingTimeStampsStatement
                                  .template values<SourceTimeStamp, 2>(1024, sourcePathId.filePathId);

            transaction.commit();

            return timeStamps;
        } catch (const Sqlite::StatementIsBusy &) {
            return fetchIncludedIndexingTimeStamps(sourcePathId);
        }
    }

    FilePathIds fetchDependentSourceIds(const FilePathIds &sourcePathIds) const override
    {
        try {
            FilePathIds dependentSourceIds;

            Sqlite::DeferredTransaction transaction{database};

            for (FilePathId sourcePathId : sourcePathIds) {
                FilePathIds newDependentSourceIds;
                newDependentSourceIds.reserve(dependentSourceIds.size() + 1024);

                auto newIds = fetchDependentSourceIdsStatement
                                  .template values<FilePathId>(1024, sourcePathId.filePathId);

                std::set_union(dependentSourceIds.begin(),
                               dependentSourceIds.end(),
                               newIds.begin(),
                               newIds.end(),
                               std::back_inserter(newDependentSourceIds));

                dependentSourceIds = std::move(newDependentSourceIds);
            }

            transaction.commit();

            return dependentSourceIds;
        } catch (const Sqlite::StatementIsBusy &) {
            return fetchDependentSourceIds(sourcePathIds);
        }
    }

    static Utils::SmallString toJson(const Utils::SmallStringVector &strings)
    {
        QJsonDocument document;
        QJsonArray array;

        std::transform(strings.begin(), strings.end(), std::back_inserter(array), [] (const auto &string) {
            return QJsonValue(string.data());
        });

        document.setArray(array);

        return document.toJson(QJsonDocument::Compact);
    }

    static Utils::SmallString toJson(const CompilerMacros &compilerMacros)
    {
        QJsonDocument document;
        QJsonObject object;

        for (const CompilerMacro &macro : compilerMacros)
            object.insert(QString(macro.key), QString(macro.value));

        document.setObject(object);

        return document.toJson(QJsonDocument::Compact);
    }

    Sqlite::Table createNewUsedMacrosTable() const
    {
        Sqlite::Table table;
        table.setName("newUsedMacros");
        table.setUseTemporaryTable(true);
        const Sqlite::Column &sourceIdColumn = table.addColumn("sourceId", Sqlite::ColumnType::Integer);
        const Sqlite::Column &macroNameColumn = table.addColumn("macroName", Sqlite::ColumnType::Text);
        table.addIndex({sourceIdColumn, macroNameColumn});

        table.initialize(database);

        return table;
    }

    Sqlite::Table createNewSourceDependenciesTable() const
    {
        Sqlite::Table table;
        table.setName("newSourceDependencies");
        table.setUseTemporaryTable(true);
        const Sqlite::Column &sourceIdColumn = table.addColumn("sourceId", Sqlite::ColumnType::Integer);
        const Sqlite::Column &dependencySourceIdColumn = table.addColumn("dependencySourceId", Sqlite::ColumnType::Text);
        table.addIndex({sourceIdColumn, dependencySourceIdColumn});

        table.initialize(database);

        return table;
    }

public:
    Sqlite::ImmediateNonThrowingDestructorTransaction transaction;
    Database &database;
    Sqlite::Table newUsedMacroTable{createNewUsedMacrosTable()};
    Sqlite::Table newNewSourceDependenciesTable{createNewSourceDependenciesTable()};
    WriteStatement insertIntoNewUsedMacrosStatement{
        "INSERT INTO newUsedMacros(sourceId, macroName) VALUES (?,?)",
        database
    };
    WriteStatement syncNewUsedMacrosStatement{
        "INSERT INTO usedMacros(sourceId, macroName) SELECT sourceId, macroName FROM newUsedMacros WHERE NOT EXISTS (SELECT sourceId FROM usedMacros WHERE usedMacros.sourceId == newUsedMacros.sourceId AND usedMacros.macroName == newUsedMacros.macroName)",
        database
    };
    WriteStatement deleteOutdatedUsedMacrosStatement{
        "DELETE FROM usedMacros WHERE sourceId IN (SELECT sourceId FROM newUsedMacros) AND NOT EXISTS (SELECT sourceId FROM newUsedMacros WHERE newUsedMacros.sourceId == usedMacros.sourceId AND newUsedMacros.macroName == usedMacros.macroName)",
        database
    };
    WriteStatement deleteNewUsedMacrosTableStatement{
        "DELETE FROM newUsedMacros",
        database
    };
    mutable ReadStatement getLowestLastModifiedTimeOfDependencies{
        "WITH RECURSIVE sourceIds(sourceId) AS (VALUES(?) UNION SELECT dependencySourceId FROM sourceDependencies, sourceIds WHERE sourceDependencies.sourceId = sourceIds.sourceId) SELECT min(lastModified) FROM fileStatuses, sourceIds WHERE fileStatuses.sourceId = sourceIds.sourceId",
        database
    };
    WriteStatement insertIntoNewSourceDependenciesStatement{
        "INSERT INTO newSourceDependencies(sourceId, dependencySourceId) VALUES (?,?)",
        database
    };
    WriteStatement insertOrUpdateFileStatusesStatement{
        "INSERT INTO fileStatuses(sourceId, size, lastModified) VALUES "
        "(?001,?002,?003) ON "
        "CONFLICT(sourceId) DO UPDATE SET size = ?002, lastModified = ?003",
        database};
    WriteStatement syncNewSourceDependenciesStatement{
        "INSERT INTO sourceDependencies(sourceId, dependencySourceId) SELECT sourceId, dependencySourceId FROM newSourceDependencies WHERE NOT EXISTS (SELECT sourceId FROM sourceDependencies WHERE sourceDependencies.sourceId == newSourceDependencies.sourceId AND sourceDependencies.dependencySourceId == newSourceDependencies.dependencySourceId)",
        database
    };
    WriteStatement deleteOutdatedSourceDependenciesStatement{
        "DELETE FROM sourceDependencies WHERE sourceId IN (SELECT sourceId FROM newSourceDependencies) AND NOT EXISTS (SELECT sourceId FROM newSourceDependencies WHERE newSourceDependencies.sourceId == sourceDependencies.sourceId AND newSourceDependencies.dependencySourceId == sourceDependencies.dependencySourceId)",
        database
    };
    WriteStatement deleteNewSourceDependenciesStatement{
        "DELETE FROM newSourceDependencies",
        database
    };
    WriteStatement insertOrUpdateProjectPartsFilesStatement{
        "INSERT INTO projectPartsFiles(sourceId, projectPartId, "
        "sourceType, hasMissingIncludes) VALUES (?001, ?002, ?003, ?004) ON "
        "CONFLICT(sourceId, projectPartId) DO UPDATE SET sourceType = ?003, "
        "hasMissingIncludes = ?004",
        database};
    mutable ReadStatement fetchPchSourcesStatement{
        "SELECT sourceId FROM projectPartsFiles WHERE projectPartId = ? AND sourceType IN (0, 1, "
        "3, 4) ORDER BY sourceId",
        database};
    mutable ReadStatement fetchSourcesStatement{
        "SELECT sourceId FROM projectPartsFiles WHERE projectPartId = ? ORDER BY sourceId", database};
    mutable ReadStatement fetchSourceDependenciesStatement{
        "WITH RECURSIVE collectedDependencies(sourceId) AS (VALUES(?) UNION "
        "SELECT dependencySourceId FROM sourceDependencies, "
        "collectedDependencies WHERE sourceDependencies.sourceId == "
        "collectedDependencies.sourceId) SELECT sourceId, "
        "pchCreationTimeStamp, sourceType, hasMissingIncludes FROM "
        "collectedDependencies NATURAL JOIN projectPartsFiles WHERE "
        "projectPartId = ? ORDER BY sourceId",
        database};
    mutable ReadStatement fetchProjectPartIdStatement{
        "SELECT projectPartId FROM projectParts WHERE projectPartName = ?",
        database
    };
    WriteStatement insertProjectPartNameStatement{
        "INSERT INTO projectParts(projectPartName) VALUES (?)", database};
    mutable ReadStatement fetchUsedMacrosStatement{
        "SELECT macroName, sourceId FROM usedMacros WHERE sourceId = ? ORDER BY sourceId, macroName",
        database
    };
    WriteStatement updatePchCreationTimeStampStatement{
        "UPDATE projectPartsFiles SET pchCreationTimeStamp = ?001 WHERE projectPartId = ?002",
        database};
    WriteStatement deleteAllProjectPartsFilesWithProjectPartNameStatement{
        "DELETE FROM projectPartsFiles WHERE projectPartId = ?", database};
    WriteStatement inserOrUpdateIndexingTimesStampStatement{
        "INSERT INTO fileStatuses(sourceId, indexingTimeStamp) VALUES (?001, ?002) ON "
        "CONFLICT(sourceId) DO UPDATE SET indexingTimeStamp = ?002",
        database};
    mutable ReadStatement fetchIncludedIndexingTimeStampsStatement{
        "WITH RECURSIVE collectedDependencies(sourceId) AS (VALUES(?) UNION SELECT "
        "dependencySourceId FROM sourceDependencies, collectedDependencies WHERE "
        "sourceDependencies.sourceId == collectedDependencies.sourceId) SELECT DISTINCT sourceId, "
        "indexingTimeStamp FROM collectedDependencies NATURAL LEFT JOIN fileStatuses ORDER BY "
        "sourceId",
        database};
    mutable ReadStatement fetchIndexingTimeStampsStatement{
        "SELECT sourceId, indexingTimeStamp FROM fileStatuses ORDER BY sourceId", database};
    mutable ReadStatement fetchDependentSourceIdsStatement{
        "WITH RECURSIVE collectedDependencies(sourceId) AS (VALUES(?) UNION SELECT "
        "sourceDependencies.sourceId FROM sourceDependencies, collectedDependencies WHERE "
        "sourceDependencies.dependencySourceId == collectedDependencies.sourceId) SELECT sourceId "
        "FROM collectedDependencies WHERE sourceId NOT IN (SELECT dependencySourceId FROM "
        "sourceDependencies) ORDER BY sourceId",
        database};
};
}
