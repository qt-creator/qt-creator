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

    void updateSources(const SourceEntries &sourceEntries) override
    {
        for (const SourceEntry &entry : sourceEntries) {
            updateBuildDependencyTimeStampStatement.write(static_cast<long long>(entry.lastModified),
                                                            entry.sourceId.filePathId);
            updateSourceTypeStatement.write(static_cast<uchar>(entry.sourceType),
                                              entry.sourceId.filePathId);
        }
    }

    void insertFileStatuses(const FileStatuses &fileStatuses) override
    {
        WriteStatement &statement = insertFileStatusesStatement;

        for (const FileStatus &fileStatus : fileStatuses)
            statement.write(fileStatus.filePathId.filePathId,
                            fileStatus.size,
                            fileStatus.lastModified,
                            fileStatus.isInPrecompiledHeader);
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
            insertStatement.write(usedMacro.filePathId.filePathId, usedMacro.macroName);

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

    SourceEntries fetchDependSources(FilePathId sourceId,
                                     Utils::SmallStringView projectPartName) const override
    {
        auto projectPartId = fetchProjectPartIdStatement.template value<int>(projectPartName);

        if (projectPartId) {
           return fetchSourceDependenciesStatement.template values<SourceEntry, 3>(
                       300,
                       sourceId.filePathId,
                       projectPartId.value());
        }
        return {};
    }

    UsedMacros fetchUsedMacros(FilePathId sourceId) const override
    {
        return fetchUsedMacrosStatement.template values<UsedMacro, 2>(128, sourceId.filePathId);
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
    WriteStatement insertFileStatusesStatement{
        "INSERT OR REPLACE INTO fileStatuses(sourceId, size, lastModified, isInPrecompiledHeader) VALUES (?,?,?,?)",
        database
    };
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
    WriteStatement updateBuildDependencyTimeStampStatement{
        "UPDATE fileStatuses SET buildDependencyTimeStamp = ? WHERE sourceId == ?",
        database
    };
    WriteStatement updateSourceTypeStatement{
        "UPDATE projectPartsSources SET sourceType = ? WHERE sourceId == ?",
        database
    };
    mutable ReadStatement fetchSourceDependenciesStatement{
        "WITH RECURSIVE collectedDependencies(sourceId) AS (VALUES(?) UNION SELECT dependencySourceId FROM sourceDependencies, collectedDependencies WHERE sourceDependencies.sourceId == collectedDependencies.sourceId) SELECT sourceId, buildDependencyTimeStamp, sourceType FROM collectedDependencies NATURAL JOIN projectPartsSources NATURAL JOIN fileStatuses WHERE projectPartId = ?",
        database
    };
    mutable ReadStatement fetchProjectPartIdStatement{
        "SELECT projectPartId FROM projectParts WHERE projectPartName = ?",
        database
    };
    mutable ReadStatement fetchUsedMacrosStatement{
        "SELECT macroName, sourceId FROM usedMacros WHERE sourceId = ?",
        database
    };
};
}
