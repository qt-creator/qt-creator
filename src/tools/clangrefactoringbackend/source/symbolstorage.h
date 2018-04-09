/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "symbolstorageinterface.h"

#include <sqliteexception.h>
#include <sqlitetransaction.h>
#include <filepathcachingfwd.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace ClangBackEnd {

template <typename StatementFactory>
class SymbolStorage final : public SymbolStorageInterface
{
    using ReadStatement = typename StatementFactory::ReadStatement;
    using WriteStatement = typename StatementFactory::WriteStatement;
    using Database = typename StatementFactory::Database;

public:
    SymbolStorage(StatementFactory &statementFactory,
                  FilePathCachingInterface &filePathCache)
        : m_statementFactory(statementFactory),
          m_filePathCache(filePathCache)
    {
    }

    void addSymbolsAndSourceLocations(const SymbolEntries &symbolEntries,
                                      const SourceLocationEntries &sourceLocations) override
    {
        fillTemporarySymbolsTable(symbolEntries);
        fillTemporaryLocationsTable(sourceLocations);
        addNewSymbolsToSymbols();
        syncNewSymbolsFromSymbols();
        syncSymbolsIntoNewLocations();
        deleteAllLocationsFromUpdatedFiles();
        insertNewLocationsInLocations();
        deleteNewSymbolsTable();
        deleteNewLocationsTable();
    }

    void insertOrUpdateProjectPart(Utils::SmallStringView projectPartName,
                                   const Utils::SmallStringVector &commandLineArguments,
                                   const CompilerMacros &compilerMacros,
                                   const Utils::SmallStringVector &includeSearchPaths) override
    {
        m_statementFactory.database.setLastInsertedRowId(-1);

        Utils::SmallString compilerArguementsAsJson = toJson(commandLineArguments);
        Utils::SmallString compilerMacrosAsJson = toJson(compilerMacros);
        Utils::SmallString includeSearchPathsAsJason = toJson(includeSearchPaths);

        WriteStatement &insertStatement = m_statementFactory.insertProjectPartStatement;
        insertStatement.write(projectPartName,
                              compilerArguementsAsJson,
                              compilerMacrosAsJson,
                              includeSearchPathsAsJason);

        if (m_statementFactory.database.lastInsertedRowId() == -1) {
            WriteStatement &updateStatement = m_statementFactory.updateProjectPartStatement;
            updateStatement.write(compilerArguementsAsJson,
                                  compilerMacrosAsJson,
                                  includeSearchPathsAsJason,
                                  projectPartName);
        }
    }

    Utils::optional<ProjectPartArtefact> fetchProjectPartArtefact(FilePathId sourceId) const override
    {
        ReadStatement &statement = m_statementFactory.getProjectPartArtefactsBySourceId;

        return statement.template value<ProjectPartArtefact, 4>(sourceId.filePathId);
    }

    Utils::optional<ProjectPartArtefact> fetchProjectPartArtefact(Utils::SmallStringView projectPartName) const override
    {
        ReadStatement &statement = m_statementFactory.getProjectPartArtefactsByProjectPartName;

        return statement.template value<ProjectPartArtefact, 4>(projectPartName);
    }

    long long fetchLowestLastModifiedTime(FilePathId sourceId) const override
    {
        ReadStatement &statement = m_statementFactory.getLowestLastModifiedTimeOfDependencies;

        return statement.template value<long long>(sourceId.filePathId).value_or(0);
    }

    void insertOrUpdateUsedMacros(const UsedMacros &usedMacros) override
    {
        WriteStatement &insertStatement = m_statementFactory.insertIntoNewUsedMacrosStatement;
        for (const UsedMacro &usedMacro : usedMacros)
            insertStatement.write(usedMacro.filePathId.filePathId, usedMacro.macroName);

        m_statementFactory.syncNewUsedMacrosStatement.execute();
        m_statementFactory.deleteOutdatedUsedMacrosStatement.execute();
        m_statementFactory.deleteNewUsedMacrosTableStatement.execute();
    }

    void insertOrUpdateSourceDependencies(const SourceDependencies &sourceDependencies) override
    {
        WriteStatement &insertStatement = m_statementFactory.insertIntoNewSourceDependenciesStatement;
        for (SourceDependency sourceDependency : sourceDependencies)
            insertStatement.write(sourceDependency.filePathId.filePathId,
                                  sourceDependency.dependencyFilePathId.filePathId);

        m_statementFactory.syncNewSourceDependenciesStatement.execute();
        m_statementFactory.deleteOutdatedSourceDependenciesStatement.execute();
        m_statementFactory.deleteNewSourceDependenciesStatement.execute();
    }

    void updateProjectPartSources(Utils::SmallStringView projectPartName,
                                  const FilePathIds &sourceFilePathIds) override
    {
        ReadStatement &getProjectPartIdStatement = m_statementFactory.getProjectPartIdStatement;
        int projectPartId = getProjectPartIdStatement.template value<int>(projectPartName).value();

        updateProjectPartSources(projectPartId, sourceFilePathIds);
    }

    void updateProjectPartSources(int projectPartId,
                                  const FilePathIds &sourceFilePathIds) override
    {
        WriteStatement &deleteStatement = m_statementFactory.deleteAllProjectPartsSourcesWithProjectPartIdStatement;
        deleteStatement.write(projectPartId);

        WriteStatement &insertStatement = m_statementFactory.insertProjectPartSourcesStatement;
        for (const FilePathId &sourceFilePathId : sourceFilePathIds)
            insertStatement.write(projectPartId, sourceFilePathId.filePathId);
    }

    void insertFileStatuses(const FileStatuses &fileStatuses)
    {
        WriteStatement &statement = m_statementFactory.insertFileStatuses;

        for (const FileStatus &fileStatus : fileStatuses)
            statement.write(fileStatus.filePathId.filePathId,
                            fileStatus.size,
                            fileStatus.lastModified,
                            fileStatus.isInPrecompiledHeader);
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

    void fillTemporarySymbolsTable(const SymbolEntries &symbolEntries)
    {
        WriteStatement &statement = m_statementFactory.insertSymbolsToNewSymbolsStatement;

        for (const auto &symbolEntry : symbolEntries) {
            statement.write(symbolEntry.first,
                            symbolEntry.second.usr,
                            symbolEntry.second.symbolName,
                            static_cast<uint>(symbolEntry.second.symbolKind));
        }
    }

    void fillTemporaryLocationsTable(const SourceLocationEntries &sourceLocations)
    {
        WriteStatement &statement = m_statementFactory.insertLocationsToNewLocationsStatement;

        for (const auto &locationEntry : sourceLocations) {
            statement.write(locationEntry.symbolId,
                            locationEntry.lineColumn.line,
                            locationEntry.lineColumn.column,
                            locationEntry.filePathId.filePathId,
                            int(locationEntry.kind));
        }
    }

    void addNewSymbolsToSymbols()
    {
        m_statementFactory.addNewSymbolsToSymbolsStatement.execute();
    }

    void syncNewSymbolsFromSymbols()
    {
        m_statementFactory.syncNewSymbolsFromSymbolsStatement.execute();
    }

    void syncSymbolsIntoNewLocations()
    {
        m_statementFactory.syncSymbolsIntoNewLocationsStatement.execute();
    }

    void deleteAllLocationsFromUpdatedFiles()
    {
        m_statementFactory.deleteAllLocationsFromUpdatedFilesStatement.execute();
    }

    void insertNewLocationsInLocations()
    {
        m_statementFactory.insertNewLocationsInLocationsStatement.execute();
    }

    void deleteNewSymbolsTable()
    {
        m_statementFactory.deleteNewSymbolsTableStatement.execute();
    }

    void deleteNewLocationsTable()
    {
        m_statementFactory.deleteNewLocationsTableStatement.execute();
    }

    Utils::optional<ProjectPartPch> fetchPrecompiledHeader(int projectPartId) const
    {
        return m_statementFactory.getPrecompiledHeader.template value<ProjectPartPch, 2>(projectPartId);
    }

    SourceLocationEntries sourceLocations() const
    {
        return SourceLocationEntries();
    }

private:
    StatementFactory &m_statementFactory;
    FilePathCachingInterface &m_filePathCache;
};

} // namespace ClangBackEnd
