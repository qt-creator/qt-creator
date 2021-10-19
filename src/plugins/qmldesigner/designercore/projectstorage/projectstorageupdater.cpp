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

#include "projectstorageupdater.h"

#include "filestatuscache.h"
#include "filesysteminterface.h"
#include "projectmanagerinterface.h"
#include "projectstorage.h"
#include "qmldocumentparserinterface.h"
#include "qmltypesparserinterface.h"
#include "sourcepathcache.h"

#include <sqlitedatabase.h>

#include <functional>

namespace QmlDesigner {

ComponentReferences createComponentReferences(const QMultiHash<QString, QmlDirParser::Component> &components)
{
    ComponentReferences componentReferences;
    componentReferences.reserve(static_cast<std::size_t>(components.size()));

    for (const QmlDirParser::Component &component : components)
        componentReferences.push_back(std::cref(component));

    return componentReferences;
}

void ProjectUpdater::update()
{
    Storage::Imports imports;
    Storage::Types types;
    SourceIds sourceIds;
    FileStatuses fileStatuses;

    for (const QString &qmldirPath : m_projectManager.qtQmlDirs()) {
        SourcePath qmldirSourcePath{qmldirPath};
        SourceId qmlDirSourceId = m_pathCache.sourceId(qmldirSourcePath);

        switch (fileState(qmlDirSourceId, fileStatuses)) {
        case FileState::Changed: {
            QmlDirParser parser;
            parser.parse(m_fileSystem.contentAsQString(qmldirPath));

            sourceIds.push_back(qmlDirSourceId);

            SourceContextId directoryId = m_pathCache.sourceContextId(qmlDirSourceId);

            Utils::PathString moduleName{parser.typeNamespace()};
            ModuleId moduleId = m_projectStorage.moduleId(moduleName);

            parseTypeInfos(parser.typeInfos(), directoryId, imports, types, sourceIds, fileStatuses);
            parseQmlComponents(createComponentReferences(parser.components()),
                               directoryId,
                               moduleId,
                               imports,
                               types,
                               sourceIds,
                               fileStatuses);
            break;
        }
        case FileState::NotChanged: {
            auto qmlProjectDatas = m_projectStorage.fetchProjectDatas(qmlDirSourceId);
            parseTypeInfos(qmlProjectDatas, imports, types, sourceIds, fileStatuses);
            break;
        }
        case FileState::NotExists: {
            // sourceIds.push_back(qmlDirSourceId);
            break;
        }
        }
    }

    m_projectStorage.synchronize(std::move(imports),
                                 std::move(types),
                                 std::move(sourceIds),
                                 std::move(fileStatuses));
}

void ProjectUpdater::pathsWithIdsChanged(const std::vector<IdPaths> &idPaths) {}

void ProjectUpdater::parseTypeInfos(const QStringList &typeInfos,
                                    SourceContextId directoryId,
                                    Storage::Imports &imports,
                                    Storage::Types &types,
                                    SourceIds &sourceIds,
                                    FileStatuses &fileStatuses)
{
    QString directory{m_pathCache.sourceContextPath(directoryId)};

    for (const QString &typeInfo : typeInfos) {
        SourceId sourceId = m_pathCache.sourceId(directoryId, Utils::SmallString{typeInfo});
        QString qmltypesPath = directory + "/" + typeInfo;

        Storage::ProjectData projectData{ModuleId{}, sourceId};

        parseTypeInfo(projectData, qmltypesPath, imports, types, sourceIds, fileStatuses);
    }
}

void ProjectUpdater::parseTypeInfos(const Storage::ProjectDatas &projectDatas,
                                    Storage::Imports &imports,
                                    Storage::Types &types,
                                    SourceIds &sourceIds,
                                    FileStatuses &fileStatuses)
{
    for (const Storage::ProjectData &projectData : projectDatas) {
        QString qmltypesPath = m_pathCache.sourcePath(projectData.sourceId).toQString();

        parseTypeInfo(projectData, qmltypesPath, imports, types, sourceIds, fileStatuses);
    }
}

void ProjectUpdater::parseTypeInfo(const Storage::ProjectData &projectData,
                                   const QString &qmltypesPath,
                                   Storage::Imports &imports,
                                   Storage::Types &types,
                                   SourceIds &sourceIds,
                                   FileStatuses &fileStatuses)
{
    if (fileState(projectData.sourceId, fileStatuses) == FileState::Changed) {
        sourceIds.push_back(projectData.sourceId);
        const auto content = m_fileSystem.contentAsQString(qmltypesPath);
        m_qmlTypesParser.parse(content, imports, types, projectData);
    }
}

void ProjectUpdater::parseQmlComponents(ComponentReferences components,
                                        SourceContextId directoryId,
                                        ModuleId moduleId,
                                        Storage::Imports &imports,
                                        Storage::Types &types,
                                        SourceIds &sourceIds,
                                        FileStatuses &fileStatuses)
{
    std::sort(components.begin(), components.end(), [](auto &&first, auto &&second) {
        return std::tie(first.get().typeName, first.get().majorVersion, first.get().minorVersion)
               > std::tie(second.get().typeName, second.get().majorVersion, second.get().minorVersion);
    });

    auto newEnd = std::unique(components.begin(), components.end(), [](auto &&first, auto &&second) {
        return first.get().typeName == second.get().typeName
               && first.get().majorVersion == second.get().majorVersion;
    });

    components.erase(newEnd, components.end());

    QString directory{m_pathCache.sourceContextPath(directoryId)};

    for (const QmlDirParser::Component &component : components) {
        Utils::SmallString fileName{component.fileName};
        SourceId sourceId = m_pathCache.sourceId(directoryId, fileName);

        if (fileState(sourceId, fileStatuses) != FileState::Changed)
            continue;

        sourceIds.push_back(sourceId);

        const auto content = m_fileSystem.contentAsQString(directory + "/" + component.fileName);
        auto type = m_qmlDocumentParser.parse(content, imports);

        type.typeName = fileName;
        type.accessSemantics = Storage::TypeAccessSemantics::Reference;
        type.sourceId = sourceId;
        type.exportedTypes.push_back(
            Storage::ExportedType{moduleId,
                                  Utils::SmallString{component.typeName},
                                  Storage::Version{component.majorVersion, component.minorVersion}});

        types.push_back(std::move(type));
    }
}

ProjectUpdater::FileState ProjectUpdater::fileState(SourceId sourceId, FileStatuses &fileStatuses) const
{
    auto currentFileStatus = m_fileStatusCache.find(sourceId);

    if (!currentFileStatus.isValid())
        return FileState::NotExists;

    auto projectStorageFileStatus = m_projectStorage.fetchFileStatus(sourceId);

    if (!projectStorageFileStatus.isValid() || projectStorageFileStatus != currentFileStatus) {
        fileStatuses.push_back(currentFileStatus);
        return FileState::Changed;
    }

    return FileState::NotChanged;
}

} // namespace QmlDesigner
