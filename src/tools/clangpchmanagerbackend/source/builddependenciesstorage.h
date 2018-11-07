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
        : m_transaction(database),
          m_database(database)
    {
        m_transaction.commit();
    }

    void updateSources(const SourceEntries &sourceEntries) override
    {
        for (const SourceEntry &entry : sourceEntries) {
            m_updateBuildDependencyTimeStampStatement.write(static_cast<long long>(entry.lastModified),
                                                            entry.sourceId.filePathId);
            m_updateSourceTypeStatement.write(static_cast<uchar>(entry.sourceType),
                                              entry.sourceId.filePathId);
        }
    }

    void insertFileStatuses(const FileStatuses &fileStatuses) override
    {
        WriteStatement &statement = m_insertFileStatusesStatement;

        for (const FileStatus &fileStatus : fileStatuses)
            statement.write(fileStatus.filePathId.filePathId,
                            fileStatus.size,
                            fileStatus.lastModified,
                            fileStatus.isInPrecompiledHeader);
    }

    long long fetchLowestLastModifiedTime(FilePathId sourceId) const override
    {
        ReadStatement &statement = m_getLowestLastModifiedTimeOfDependencies;

        return statement.template value<long long>(sourceId.filePathId).value_or(0);
    }

    void insertOrUpdateUsedMacros(const UsedMacros &usedMacros) override
    {
        WriteStatement &insertStatement = m_insertIntoNewUsedMacrosStatement;
        for (const UsedMacro &usedMacro : usedMacros)
            insertStatement.write(usedMacro.filePathId.filePathId, usedMacro.macroName);

        m_syncNewUsedMacrosStatement.execute();
        m_deleteOutdatedUsedMacrosStatement.execute();
        m_deleteNewUsedMacrosTableStatement.execute();
    }

    void insertOrUpdateSourceDependencies(const SourceDependencies &sourceDependencies) override
    {
        WriteStatement &insertStatement = m_insertIntoNewSourceDependenciesStatement;
        for (SourceDependency sourceDependency : sourceDependencies)
            insertStatement.write(sourceDependency.filePathId.filePathId,
                                  sourceDependency.dependencyFilePathId.filePathId);

        m_syncNewSourceDependenciesStatement.execute();
        m_deleteOutdatedSourceDependenciesStatement.execute();
        m_deleteNewSourceDependenciesStatement.execute();
    }

    SourceEntries fetchDependSources(FilePathId sourceId,
                                     Utils::SmallStringView projectPartName) const override
    {
        auto projectPartId = m_fetchProjectPartIdStatement.template value<int>(projectPartName);

        if (projectPartId) {
           return m_fetchSourceDependenciesStatement.template values<SourceEntry, 3>(
                       300,
                       sourceId.filePathId,
                       projectPartId.value());
        }
        return {};
    }

    UsedMacros fetchUsedMacros(FilePathId sourceId) const override
    {
        return m_fetchUsedMacrosStatement.template values<UsedMacro, 2>(128, sourceId.filePathId);
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

        table.initialize(m_database);

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

        table.initialize(m_database);

        return table;
    }

public:
    Sqlite::ImmediateNonThrowingDestructorTransaction m_transaction;
    Database &m_database;
    Sqlite::Table newUsedMacroTable{createNewUsedMacrosTable()};
    Sqlite::Table newNewSourceDependenciesTable{createNewSourceDependenciesTable()};
    WriteStatement m_insertIntoNewUsedMacrosStatement{
        "INSERT INTO newUsedMacros(sourceId, macroName) VALUES (?,?)",
        m_database
    };
    WriteStatement m_syncNewUsedMacrosStatement{
        "INSERT INTO usedMacros(sourceId, macroName) SELECT sourceId, macroName FROM newUsedMacros WHERE NOT EXISTS (SELECT sourceId FROM usedMacros WHERE usedMacros.sourceId == newUsedMacros.sourceId AND usedMacros.macroName == newUsedMacros.macroName)",
        m_database
    };
    WriteStatement m_deleteOutdatedUsedMacrosStatement{
        "DELETE FROM usedMacros WHERE sourceId IN (SELECT sourceId FROM newUsedMacros) AND NOT EXISTS (SELECT sourceId FROM newUsedMacros WHERE newUsedMacros.sourceId == usedMacros.sourceId AND newUsedMacros.macroName == usedMacros.macroName)",
        m_database
    };
    WriteStatement m_deleteNewUsedMacrosTableStatement{
        "DELETE FROM newUsedMacros",
        m_database
    };
    mutable ReadStatement m_getLowestLastModifiedTimeOfDependencies{
        "WITH RECURSIVE sourceIds(sourceId) AS (VALUES(?) UNION SELECT dependencySourceId FROM sourceDependencies, sourceIds WHERE sourceDependencies.sourceId = sourceIds.sourceId) SELECT min(lastModified) FROM fileStatuses, sourceIds WHERE fileStatuses.sourceId = sourceIds.sourceId",
        m_database
    };
    WriteStatement m_insertIntoNewSourceDependenciesStatement{
        "INSERT INTO newSourceDependencies(sourceId, dependencySourceId) VALUES (?,?)",
        m_database
    };
    WriteStatement m_insertFileStatusesStatement{
        "INSERT OR REPLACE INTO fileStatuses(sourceId, size, lastModified, isInPrecompiledHeader) VALUES (?,?,?,?)",
        m_database
    };
    WriteStatement m_syncNewSourceDependenciesStatement{
        "INSERT INTO sourceDependencies(sourceId, dependencySourceId) SELECT sourceId, dependencySourceId FROM newSourceDependencies WHERE NOT EXISTS (SELECT sourceId FROM sourceDependencies WHERE sourceDependencies.sourceId == newSourceDependencies.sourceId AND sourceDependencies.dependencySourceId == newSourceDependencies.dependencySourceId)",
        m_database
    };
    WriteStatement m_deleteOutdatedSourceDependenciesStatement{
        "DELETE FROM sourceDependencies WHERE sourceId IN (SELECT sourceId FROM newSourceDependencies) AND NOT EXISTS (SELECT sourceId FROM newSourceDependencies WHERE newSourceDependencies.sourceId == sourceDependencies.sourceId AND newSourceDependencies.dependencySourceId == sourceDependencies.dependencySourceId)",
        m_database
    };
    WriteStatement m_deleteNewSourceDependenciesStatement{
        "DELETE FROM newSourceDependencies",
        m_database
    };
    WriteStatement m_updateBuildDependencyTimeStampStatement{
        "UPDATE fileStatuses SET buildDependencyTimeStamp = ? WHERE sourceId == ?",
        m_database
    };
    WriteStatement m_updateSourceTypeStatement{
        "UPDATE projectPartsSources SET sourceType = ? WHERE sourceId == ?",
        m_database
    };
    mutable ReadStatement m_fetchSourceDependenciesStatement{
        "WITH RECURSIVE collectedDependencies(sourceId) AS (VALUES(?) UNION SELECT dependencySourceId FROM sourceDependencies, collectedDependencies WHERE sourceDependencies.sourceId == collectedDependencies.sourceId) SELECT sourceId, buildDependencyTimeStamp, sourceType FROM collectedDependencies NATURAL JOIN projectPartsSources NATURAL JOIN fileStatuses WHERE projectPartId = ?",
        m_database
    };
    mutable ReadStatement m_fetchProjectPartIdStatement{
        "SELECT projectPartId FROM projectParts WHERE projectPartName = ?",
        m_database
    };
    mutable ReadStatement m_fetchUsedMacrosStatement{
        "SELECT macroName, sourceId FROM usedMacros WHERE sourceId = ?",
        m_database
    };
};
}
