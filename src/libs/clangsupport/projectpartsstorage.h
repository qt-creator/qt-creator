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

#include "clangsupportexceptions.h"
#include "projectpartsstorageinterface.h"

#include <sqliteexception.h>
#include <sqlitetransaction.h>

namespace ClangBackEnd {

template<typename Database = Sqlite::Database>
class ProjectPartsStorage final : public ProjectPartsStorageInterface
{
    using ReadStatement = typename Database::ReadStatement;
    using WriteStatement = typename Database::WriteStatement;

public:
    ProjectPartsStorage(Database &database)
        : transaction(database)
        , database(database)
    {
        transaction.commit();
    }

    ProjectPartContainers fetchProjectParts() const override
    {
        try {
            Sqlite::DeferredTransaction transaction{database};

            auto values = fetchProjectPartsStatement.template values<ProjectPartContainer, 8>(4096);

            transaction.commit();

            return values;
        } catch (const Sqlite::StatementIsBusy &) {
            return fetchProjectParts();
        }
    }

    FilePathIds fetchHeaders(ProjectPartId projectPartId) const
    {
        return fetchProjectPartsHeadersByIdStatement
            .template values<FilePathId>(1024, projectPartId.projectPathId);
    }

    FilePathIds fetchSources(ProjectPartId projectPartId) const
    {
        return fetchProjectPartsSourcesByIdStatement
            .template values<FilePathId>(1024, projectPartId.projectPathId);
    }

    ProjectPartContainers fetchProjectParts(const ProjectPartIds &projectPartIds) const override
    {
        try {
            ProjectPartContainers projectParts;
            projectParts.reserve(projectPartIds.size());

            Sqlite::DeferredTransaction transaction{database};

            for (ProjectPartId projectPartId : projectPartIds) {
                auto value = fetchProjectPartByIdStatement.template value<ProjectPartContainer, 8>(
                    projectPartId.projectPathId);
                if (value) {
                    value->headerPathIds = fetchHeaders(projectPartId);
                    value->sourcePathIds = fetchSources(projectPartId);
                    projectParts.push_back(*std::move(value));
                }
            }

            transaction.commit();

            return projectParts;
        } catch (const Sqlite::StatementIsBusy &) {
            return fetchProjectParts(projectPartIds);
        }
    }

    ProjectPartId fetchProjectPartId(Utils::SmallStringView projectPartName) const override
    {
        try {
            Sqlite::DeferredTransaction transaction{database};

            ProjectPartId projectPartId;
            auto optionalProjectPartId = fetchProjectPartIdStatement.template value<ProjectPartId>(
                projectPartName);

            if (optionalProjectPartId) {
                projectPartId = *optionalProjectPartId;
            } else {
                insertProjectPartNameStatement.write(projectPartName);

                projectPartId = static_cast<int>(database.lastInsertedRowId());
            }

            transaction.commit();

            return projectPartId;
        } catch (const Sqlite::StatementIsBusy &) {
            return fetchProjectPartId(projectPartName);
        }
    }

    Utils::PathString fetchProjectPartName(ProjectPartId projectPartId) const
    {
        try {
            Sqlite::DeferredTransaction transaction{database};

            auto optionalProjectPartName = fetchProjectPartNameStatement.template value<Utils::PathString>(
                projectPartId.projectPathId);

            transaction.commit();

            if (optionalProjectPartName)
                return *std::move(optionalProjectPartName);
        } catch (const Sqlite::StatementIsBusy &) {
            return fetchProjectPartName(projectPartId);
        }

        throwProjectPartDoesNotExists(projectPartId);
    }

    void insertHeaders(ProjectPartId projectPartId, const FilePathIds &headerPathIds)
    {
        deleteProjectPartsHeadersByIdStatement.write(projectPartId.projectPathId);
        for (FilePathId headerPathId : headerPathIds) {
            insertProjectPartsHeadersStatement.write(projectPartId.projectPathId,
                                                     headerPathId.filePathId);
        }
    }

    void insertSources(ProjectPartId projectPartId, const FilePathIds &sourcePathIds)
    {
        deleteProjectPartsSourcesByIdStatement.write(projectPartId.projectPathId);
        for (FilePathId sourcePathId : sourcePathIds) {
            insertProjectPartsSourcesStatement.write(projectPartId.projectPathId,
                                                     sourcePathId.filePathId);
        }
    }

    void updateProjectPart(ProjectPartId projectPartId,
                           const Utils::SmallStringVector &toolChainArguments,
                           const CompilerMacros &compilerMacros,
                           const IncludeSearchPaths &systemIncludeSearchPaths,
                           const IncludeSearchPaths &projectIncludeSearchPaths,
                           Utils::Language language,
                           Utils::LanguageVersion languageVersion,
                           Utils::LanguageExtension languageExtension) override
    {
        Utils::SmallString toolChainArgumentsAsJson = toJson(toolChainArguments);
        Utils::SmallString compilerMacrosAsJson = toJson(compilerMacros);
        Utils::SmallString systemIncludeSearchPathsAsJason = toJson(systemIncludeSearchPaths);
        Utils::SmallString projectIncludeSearchPathsAsJason = toJson(projectIncludeSearchPaths);

        updateProjectPartStatement.write(projectPartId.projectPathId,
                                         toolChainArgumentsAsJson,
                                         compilerMacrosAsJson,
                                         systemIncludeSearchPathsAsJason,
                                         projectIncludeSearchPathsAsJason,
                                         static_cast<int>(language),
                                         static_cast<int>(languageVersion),
                                         static_cast<int>(languageExtension));
    }

    void updateProjectPart(const ProjectPartContainer &projectPart)
    {
        Utils::SmallString toolChainArgumentsAsJson = toJson(projectPart.toolChainArguments);
        Utils::SmallString compilerMacrosAsJson = toJson(projectPart.compilerMacros);
        Utils::SmallString systemIncludeSearchPathsAsJason = toJson(
            projectPart.systemIncludeSearchPaths);
        Utils::SmallString projectIncludeSearchPathsAsJason = toJson(
            projectPart.projectIncludeSearchPaths);

        updateProjectPartStatement.write(projectPart.projectPartId.projectPathId,
                                         toolChainArgumentsAsJson,
                                         compilerMacrosAsJson,
                                         systemIncludeSearchPathsAsJason,
                                         projectIncludeSearchPathsAsJason,
                                         static_cast<int>(projectPart.language),
                                         static_cast<int>(projectPart.languageVersion),
                                         static_cast<int>(projectPart.languageExtension));

        insertHeaders(projectPart.projectPartId, projectPart.headerPathIds);
        insertSources(projectPart.projectPartId, projectPart.sourcePathIds);
    }

    void updateProjectParts(const ProjectPartContainers &projectParts) override
    {
        try {
            Sqlite::ImmediateTransaction transaction{database};

            for (const ProjectPartContainer &projectPart : projectParts)
                updateProjectPart(projectPart);

            transaction.commit();
        } catch (const Sqlite::StatementIsBusy &) {
            updateProjectParts(projectParts);
        }
    }

    Utils::optional<ProjectPartArtefact> fetchProjectPartArtefact(FilePathId sourceId) const override
    {
        ReadStatement &statement = getProjectPartArtefactsBySourceId;

        return statement.template value<ProjectPartArtefact, 8>(sourceId.filePathId);
    }

    Utils::optional<ProjectPartArtefact> fetchProjectPartArtefact(ProjectPartId projectPartId) const override
    {
        ReadStatement &statement = getProjectPartArtefactsByProjectPartId;

        return statement.template value<ProjectPartArtefact, 8>(projectPartId.projectPathId);
    }

    Sqlite::TransactionInterface &transactionBackend() override { return database; }

    static Utils::SmallString toJson(const Utils::SmallStringVector &strings)
    {
        QJsonDocument document;
        QJsonArray array;

        std::transform(strings.begin(),
                       strings.end(),
                       std::back_inserter(array),
                       [](const auto &string) { return QJsonValue(string.data()); });

        document.setArray(array);

        return document.toJson(QJsonDocument::Compact);
    }

    static Utils::SmallString toJson(const CompilerMacros &compilerMacros)
    {
        QJsonDocument document;
        QJsonArray array;

        for (const CompilerMacro &macro : compilerMacros)
            array.push_back(QJsonArray{{QString(macro.key), QString(macro.value), macro.index}});

        document.setArray(array);

        return document.toJson(QJsonDocument::Compact);
    }

    static Utils::SmallString toJson(const IncludeSearchPaths &includeSearchPaths)
    {
        QJsonDocument document;
        QJsonArray array;

        for (const IncludeSearchPath &path : includeSearchPaths)
            array.push_back(QJsonArray{{path.path.data(), path.index, int(path.type)}});

        document.setArray(array);

        return document.toJson(QJsonDocument::Compact);
    }

    [[noreturn]] void throwProjectPartDoesNotExists(ProjectPartId projectPartId) const
    {
        throw ProjectPartDoesNotExists("Try to fetch non existing project part id: ",
                                       Utils::SmallString::number(projectPartId.projectPathId));
    }

public:
    Sqlite::ImmediateNonThrowingDestructorTransaction transaction;
    Database &database;
    mutable ReadStatement fetchProjectPartIdStatement{
        "SELECT projectPartId FROM projectParts WHERE projectPartName = ?", database};
    mutable WriteStatement insertProjectPartNameStatement{
        "INSERT INTO projectParts(projectPartName) VALUES (?)", database};
    mutable ReadStatement fetchProjectPartNameStatement{
        "SELECT projectPartName FROM projectParts WHERE projectPartId = ?", database};
    mutable ReadStatement fetchProjectPartsStatement{
        "SELECT toolChainArguments, compilerMacros, systemIncludeSearchPaths, "
        "projectIncludeSearchPaths, projectPartId, language, languageVersion, languageExtension "
        "FROM projectParts",
        database};
    mutable ReadStatement fetchProjectPartByIdStatement{
        "SELECT toolChainArguments, compilerMacros, systemIncludeSearchPaths, "
        "projectIncludeSearchPaths, projectPartId, language, languageVersion, languageExtension "
        "FROM projectParts WHERE projectPartId = ?",
        database};
    WriteStatement updateProjectPartStatement{
        "UPDATE projectParts SET toolChainArguments=?002, compilerMacros=?003, "
        "systemIncludeSearchPaths=?004, projectIncludeSearchPaths=?005, language=?006, "
        "languageVersion=?007, languageExtension=?008 WHERE projectPartId = ?001",
        database};
    mutable ReadStatement getProjectPartArtefactsBySourceId{
        "SELECT toolChainArguments, compilerMacros, systemIncludeSearchPaths, "
        "projectIncludeSearchPaths, projectPartId, language, languageVersion, languageExtension "
        "FROM projectParts WHERE projectPartId = (SELECT "
        "projectPartId FROM projectPartsFiles WHERE sourceId = ?)",
        database};
    mutable ReadStatement getProjectPartArtefactsByProjectPartId{
        "SELECT toolChainArguments, compilerMacros, systemIncludeSearchPaths, "
        "projectIncludeSearchPaths, projectPartId, language, languageVersion, languageExtension "
        "FROM projectParts WHERE projectPartId = ?",
        database};
    WriteStatement deleteProjectPartsHeadersByIdStatement{
        "DELETE FROM projectPartsHeaders WHERE projectPartId = ?", database};
    WriteStatement deleteProjectPartsSourcesByIdStatement{
        "DELETE FROM projectPartsSources WHERE projectPartId = ?", database};
    WriteStatement insertProjectPartsHeadersStatement{
        "INSERT INTO projectPartsHeaders(projectPartId, sourceId) VALUES (?,?)", database};
    WriteStatement insertProjectPartsSourcesStatement{
        "INSERT INTO projectPartsSources(projectPartId, sourceId) VALUES (?,?)", database};
    mutable ReadStatement fetchProjectPartsHeadersByIdStatement{
        "SELECT sourceId FROM projectPartsHeaders WHERE projectPartId = ?", database};
    mutable ReadStatement fetchProjectPartsSourcesByIdStatement{
        "SELECT sourceId FROM projectPartsSources WHERE projectPartId = ?", database};
};
} // namespace ClangBackEnd
