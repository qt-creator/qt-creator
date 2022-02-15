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
#include "projectstorage.h"
#include "qmldocumentparserinterface.h"
#include "qmltypesparserinterface.h"
#include "sourcepath.h"
#include "sourcepathcache.h"

#include <sqlitedatabase.h>

#include <algorithm>
#include <functional>

namespace QmlDesigner {
namespace {

ComponentReferences createComponentReferences(const QMultiHash<QString, QmlDirParser::Component> &components)
{
    ComponentReferences componentReferences;
    componentReferences.reserve(static_cast<std::size_t>(components.size()));

    for (const QmlDirParser::Component &component : components)
        componentReferences.push_back(std::cref(component));

    return componentReferences;
}

SourceIds filterNotUpdatedSourceIds(SourceIds updatedSourceIds, SourceIds notUpdatedSourceIds)
{
    std::sort(updatedSourceIds.begin(), updatedSourceIds.end());
    std::sort(notUpdatedSourceIds.begin(), notUpdatedSourceIds.end());

    SourceIds filteredUpdatedSourceIds;
    filteredUpdatedSourceIds.reserve(updatedSourceIds.size());

    std::set_difference(updatedSourceIds.cbegin(),
                        updatedSourceIds.cend(),
                        notUpdatedSourceIds.cbegin(),
                        notUpdatedSourceIds.cend(),
                        std::back_inserter(filteredUpdatedSourceIds));

    filteredUpdatedSourceIds.erase(std::unique(filteredUpdatedSourceIds.begin(),
                                               filteredUpdatedSourceIds.end()),
                                   filteredUpdatedSourceIds.end());

    return filteredUpdatedSourceIds;
}

void addSourceIds(SourceIds &sourceIds, const Storage::ProjectDatas &projectDatas)
{
    for (const auto &projectData : projectDatas)
        sourceIds.push_back(projectData.sourceId);
}

Storage::Version convertVersion(LanguageUtils::ComponentVersion version)
{
    return Storage::Version{version.majorVersion(), version.minorVersion()};
}

Storage::IsAutoVersion convertToIsAutoVersion(QmlDirParser::Import::Flags flags)
{
    if (flags & QmlDirParser::Import::Flag::Auto)
        return Storage::IsAutoVersion::Yes;
    return Storage::IsAutoVersion::No;
}

void addDependencies(Storage::Imports &dependencies,
                     SourceId sourceId,
                     const QList<QmlDirParser::Import> &qmldirDependencies,
                     ProjectStorageInterface &projectStorage)
{
    for (const QmlDirParser::Import &qmldirDependency : qmldirDependencies) {
        ModuleId moduleId = projectStorage.moduleId(Utils::PathString{qmldirDependency.module}
                                                    + "-cppnative");
        dependencies.emplace_back(moduleId, Storage::Version{}, sourceId);
    }
}

void addModuleExportedImports(Storage::ModuleExportedImports &imports,
                              ModuleId moduleId,
                              const QList<QmlDirParser::Import> &qmldirImports,
                              ProjectStorageInterface &projectStorage)
{
    for (const QmlDirParser::Import &qmldirImport : qmldirImports) {
        if (qmldirImport.flags & QmlDirParser::Import::Flag::Optional)
            continue;

        ModuleId exportedModuleId = projectStorage.moduleId(Utils::PathString{qmldirImport.module});
        imports.emplace_back(moduleId,
                             exportedModuleId,
                             convertVersion(qmldirImport.version),
                             convertToIsAutoVersion(qmldirImport.flags));

        ModuleId exportedCppModuleId = projectStorage.moduleId(
            Utils::PathString{qmldirImport.module} + "-cppnative");
        imports.emplace_back(moduleId,
                             exportedCppModuleId,
                             Storage::Version{},
                             Storage::IsAutoVersion::No);
    }
}

} // namespace

void ProjectStorageUpdater::update(QStringList qmlDirs, QStringList qmlTypesPaths)
{
    Storage::SynchronizationPackage package;

    SourceIds notUpdatedFileStatusSourceIds;
    SourceIds notUpdatedSourceIds;

    updateQmldirs(qmlDirs, package, notUpdatedFileStatusSourceIds, notUpdatedSourceIds);
    updateQmlTypes(qmlTypesPaths, package, notUpdatedFileStatusSourceIds, notUpdatedSourceIds);

    package.updatedSourceIds = filterNotUpdatedSourceIds(std::move(package.updatedSourceIds),
                                                         std::move(notUpdatedSourceIds));
    package.updatedFileStatusSourceIds = filterNotUpdatedSourceIds(
        std::move(package.updatedFileStatusSourceIds), std::move(notUpdatedFileStatusSourceIds));

    m_projectStorage.synchronize(std::move(package));
}

void ProjectStorageUpdater::updateQmlTypes(const QStringList &qmlTypesPaths,
                                           Storage::SynchronizationPackage &package,
                                           SourceIds &notUpdatedFileStatusSourceIds,
                                           SourceIds &notUpdatedSourceIds)
{
    if (qmlTypesPaths.empty())
        return;

    ModuleId moduleId = m_projectStorage.moduleId("QML-cppnative");

    for (const QString &qmlTypesPath : qmlTypesPaths) {
        SourceId sourceId = m_pathCache.sourceId(SourcePath{qmlTypesPath});

        Storage::ProjectData projectData{SourceId{}, sourceId, moduleId, Storage::FileType::QmlTypes};

        FileState state = parseTypeInfo(projectData,
                                        Utils::PathString{qmlTypesPath},
                                        package,
                                        notUpdatedFileStatusSourceIds,
                                        notUpdatedSourceIds);

        if (state == FileState::Changed)
            package.projectDatas.push_back(std::move(projectData));
    }
}

void ProjectStorageUpdater::updateQmldirs(const QStringList &qmlDirs,
                                          Storage::SynchronizationPackage &package,
                                          SourceIds &notUpdatedFileStatusSourceIds,
                                          SourceIds &notUpdatedSourceIds)
{
    for (const QString &qmldirPath : qmlDirs) {
        SourcePath qmldirSourcePath{qmldirPath};
        SourceId qmlDirSourceId = m_pathCache.sourceId(qmldirSourcePath);

        auto state = fileState(qmlDirSourceId,
                               package.fileStatuses,
                               package.updatedFileStatusSourceIds,
                               notUpdatedFileStatusSourceIds);
        switch (state) {
        case FileState::Changed: {
            QmlDirParser parser;
            parser.parse(m_fileSystem.contentAsQString(qmldirPath));

            package.updatedSourceIds.push_back(qmlDirSourceId);

            SourceContextId directoryId = m_pathCache.sourceContextId(qmlDirSourceId);

            Utils::PathString moduleName{parser.typeNamespace()};
            ModuleId moduleId = m_projectStorage.moduleId(moduleName);

            addModuleExportedImports(package.moduleExportedImports,
                                     moduleId,
                                     parser.imports(),
                                     m_projectStorage);
            package.updatedModuleIds.push_back(moduleId);

            const auto qmlProjectDatas = m_projectStorage.fetchProjectDatas(qmlDirSourceId);
            addSourceIds(package.updatedSourceIds, qmlProjectDatas);
            addSourceIds(package.updatedFileStatusSourceIds, qmlProjectDatas);

            if (!parser.typeInfos().isEmpty()) {
                ModuleId cppModuleId = m_projectStorage.moduleId(moduleName + "-cppnative");
                parseTypeInfos(parser.typeInfos(),
                               parser.dependencies(),
                               parser.imports(),
                               qmlDirSourceId,
                               directoryId,
                               cppModuleId,
                               package,
                               notUpdatedFileStatusSourceIds,
                               notUpdatedSourceIds);
            }
            parseQmlComponents(createComponentReferences(parser.components()),
                               qmlDirSourceId,
                               directoryId,
                               moduleId,
                               package,
                               notUpdatedFileStatusSourceIds);
            package.updatedProjectSourceIds.push_back(qmlDirSourceId);
            break;
        }
        case FileState::NotChanged: {
            const auto qmlProjectDatas = m_projectStorage.fetchProjectDatas(qmlDirSourceId);
            parseTypeInfos(qmlProjectDatas, package, notUpdatedFileStatusSourceIds, notUpdatedSourceIds);
            parseQmlComponents(qmlProjectDatas, package, notUpdatedFileStatusSourceIds);
            break;
        }
        case FileState::NotExists: {
            package.updatedSourceIds.push_back(qmlDirSourceId);
            auto qmlProjectDatas = m_projectStorage.fetchProjectDatas(qmlDirSourceId);
            for (const Storage::ProjectData &projectData : qmlProjectDatas) {
                package.updatedSourceIds.push_back(projectData.sourceId);
            }

            break;
        }
        }
    }
}

void ProjectStorageUpdater::pathsWithIdsChanged(const std::vector<IdPaths> &idPaths) {}

void ProjectStorageUpdater::parseTypeInfos(const QStringList &typeInfos,
                                           const QList<QmlDirParser::Import> &qmldirDependencies,
                                           const QList<QmlDirParser::Import> &qmldirImports,
                                           SourceId qmldirSourceId,
                                           SourceContextId directoryId,
                                           ModuleId moduleId,
                                           Storage::SynchronizationPackage &package,
                                           SourceIds &notUpdatedFileStatusSourceIds,
                                           SourceIds &notUpdatedSourceIds)
{
    const Utils::PathString directory{m_pathCache.sourceContextPath(directoryId)};

    for (const QString &typeInfo : typeInfos) {
        Utils::PathString qmltypesPath = Utils::PathString::join(
            {directory, "/", Utils::SmallString{typeInfo}});
        SourceId sourceId = m_pathCache.sourceId(SourcePathView{qmltypesPath});

        addDependencies(package.moduleDependencies, sourceId, qmldirDependencies, m_projectStorage);
        addDependencies(package.moduleDependencies, sourceId, qmldirImports, m_projectStorage);
        package.updatedModuleDependencySourceIds.push_back(sourceId);

        auto projectData = package.projectDatas.emplace_back(qmldirSourceId,
                                                             sourceId,
                                                             moduleId,
                                                             Storage::FileType::QmlTypes);

        parseTypeInfo(projectData,
                      qmltypesPath,
                      package,
                      notUpdatedFileStatusSourceIds,
                      notUpdatedSourceIds);
    }
}

void ProjectStorageUpdater::parseTypeInfos(const Storage::ProjectDatas &projectDatas,
                                           Storage::SynchronizationPackage &package,
                                           SourceIds &notUpdatedFileStatusSourceIds,
                                           SourceIds &notUpdatedSourceIds)
{
    for (const Storage::ProjectData &projectData : projectDatas) {
        if (projectData.fileType != Storage::FileType::QmlTypes)
            continue;

        auto qmltypesPath = m_pathCache.sourcePath(projectData.sourceId);

        parseTypeInfo(projectData,
                      qmltypesPath,
                      package,
                      notUpdatedFileStatusSourceIds,
                      notUpdatedSourceIds);
    }
}

auto ProjectStorageUpdater::parseTypeInfo(const Storage::ProjectData &projectData,
                                          Utils::SmallStringView qmltypesPath,
                                          Storage::SynchronizationPackage &package,
                                          SourceIds &notUpdatedFileStatusSourceIds,
                                          SourceIds &notUpdatedSourceIds) -> FileState
{
    auto state = fileState(projectData.sourceId,
                           package.fileStatuses,
                           package.updatedFileStatusSourceIds,
                           notUpdatedFileStatusSourceIds);
    switch (state) {
    case FileState::Changed: {
        package.updatedSourceIds.push_back(projectData.sourceId);

        const auto content = m_fileSystem.contentAsQString(QString{qmltypesPath});
        m_qmlTypesParser.parse(content, package.imports, package.types, projectData);
        break;
    }
    case FileState::NotChanged: {
        notUpdatedSourceIds.push_back(projectData.sourceId);
        break;
    }
    case FileState::NotExists:
        break;
    }

    return state;
}

void ProjectStorageUpdater::parseQmlComponent(Utils::SmallStringView relativeFilePath,
                                              Utils::SmallStringView directory,
                                              Storage::ExportedTypes exportedTypes,
                                              ModuleId moduleId,
                                              SourceId qmldirSourceId,
                                              Storage::SynchronizationPackage &package,
                                              SourceIds &notUpdatedFileStatusSourceIds)
{
    if (std::find(relativeFilePath.begin(), relativeFilePath.end(), '+') != relativeFilePath.end())
        return;

    Utils::PathString qmlFilePath = Utils::PathString::join({directory, "/", relativeFilePath});
    SourceId sourceId = m_pathCache.sourceId(SourcePathView{qmlFilePath});

    Storage::Type type;

    auto state = fileState(sourceId,
                           package.fileStatuses,
                           package.updatedFileStatusSourceIds,
                           notUpdatedFileStatusSourceIds);
    switch (state) {
    case FileState::NotChanged:
        type.changeLevel = Storage::ChangeLevel::Minimal;
        break;
    case FileState::NotExists:
        throw CannotParseQmlDocumentFile{};
    case FileState::Changed:
        const auto content = m_fileSystem.contentAsQString(QString{qmlFilePath});
        type = m_qmlDocumentParser.parse(content, package.imports, sourceId);
        break;
    }

    package.projectDatas.emplace_back(qmldirSourceId, sourceId, moduleId, Storage::FileType::QmlDocument);

    package.updatedSourceIds.push_back(sourceId);

    type.typeName = SourcePath{qmlFilePath}.name();
    type.accessSemantics = Storage::TypeAccessSemantics::Reference;
    type.sourceId = sourceId;
    type.exportedTypes = std::move(exportedTypes);

    package.types.push_back(std::move(type));
}

void ProjectStorageUpdater::parseQmlComponent(Utils::SmallStringView fileName,
                                              Utils::SmallStringView filePath,
                                              SourceId sourceId,
                                              Storage::SynchronizationPackage &package,
                                              SourceIds &notUpdatedFileStatusSourceIds)
{
    auto state = fileState(sourceId,
                           package.fileStatuses,
                           package.updatedFileStatusSourceIds,
                           notUpdatedFileStatusSourceIds);
    if (state != FileState::Changed)
        return;

    package.updatedSourceIds.push_back(sourceId);

    SourcePath sourcePath{filePath};

    const auto content = m_fileSystem.contentAsQString(QString{filePath});
    auto type = m_qmlDocumentParser.parse(content, package.imports, sourceId);

    type.typeName = fileName;
    type.accessSemantics = Storage::TypeAccessSemantics::Reference;
    type.sourceId = sourceId;
    type.changeLevel = Storage::ChangeLevel::ExcludeExportedTypes;

    package.types.push_back(std::move(type));
}

namespace {

class ComponentReferencesRange
{
public:
    using const_iterator = ComponentReferences::const_iterator;

    ComponentReferencesRange(const_iterator begin, const_iterator end)
        : m_begin{begin}
        , m_end{end}
    {}

    std::size_t size() const { return static_cast<std::size_t>(std::distance(m_begin, m_end)); }

    const_iterator begin() const { return m_begin; }
    const_iterator end() const { return m_end; }

private:
    const_iterator m_begin;
    const_iterator m_end;
};

template<typename Callback>
void partitionForTheSameFileName(const ComponentReferences &components, Callback callback)
{
    auto current = components.begin();
    const auto end = components.end();

    while (current != end) {
        auto nextType = std::partition_point(current, end, [&](const auto &component) {
            return component.get().fileName == current->get().fileName;
        });

        callback(ComponentReferencesRange{current, nextType});

        current = nextType;
    }
}

Storage::ExportedTypes filterExportedTypes(ComponentReferencesRange components, ModuleId moduleId)
{
    return Utils::transform<Storage::ExportedTypes>(components, [&](ComponentReference component) {
        return Storage::ExportedType{moduleId,
                                     Utils::SmallString{component.get().typeName},
                                     Storage::Version{component.get().majorVersion,
                                                      component.get().minorVersion}};
    });
}

} // namespace

void ProjectStorageUpdater::parseQmlComponents(ComponentReferences components,
                                               SourceId qmldirSourceId,
                                               SourceContextId directoryId,
                                               ModuleId moduleId,
                                               Storage::SynchronizationPackage &package,
                                               SourceIds &notUpdatedFileStatusSourceIds)
{
    std::sort(components.begin(), components.end(), [](auto &&first, auto &&second) {
        return std::tie(first.get().fileName,
                        first.get().typeName,
                        first.get().majorVersion,
                        first.get().minorVersion)
               > std::tie(first.get().fileName,
                          second.get().typeName,
                          second.get().majorVersion,
                          second.get().minorVersion);
    });

    auto directory = m_pathCache.sourceContextPath(directoryId);

    auto callback = [&](ComponentReferencesRange componentsWithSameFileName) {
        const auto &firstComponent = *componentsWithSameFileName.begin();
        parseQmlComponent(Utils::SmallString{firstComponent.get().fileName},
                          directory,
                          filterExportedTypes(componentsWithSameFileName, moduleId),
                          moduleId,
                          qmldirSourceId,
                          package,
                          notUpdatedFileStatusSourceIds);
    };

    partitionForTheSameFileName(components, callback);
}

void ProjectStorageUpdater::parseQmlComponents(const Storage::ProjectDatas &projectDatas,
                                               Storage::SynchronizationPackage &package,
                                               SourceIds &notUpdatedFileStatusSourceIds)
{
    for (const Storage::ProjectData &projectData : projectDatas) {
        if (projectData.fileType != Storage::FileType::QmlDocument)
            continue;

        SourcePath qmlDocumentPath = m_pathCache.sourcePath(projectData.sourceId);

        parseQmlComponent(qmlDocumentPath.name(),
                          qmlDocumentPath,
                          projectData.sourceId,
                          package,
                          notUpdatedFileStatusSourceIds);
    }
}

ProjectStorageUpdater::FileState ProjectStorageUpdater::fileState(SourceId sourceId,
                                                                  FileStatuses &fileStatuses,
                                                                  SourceIds &updatedSourceIds,
                                                                  SourceIds &notUpdatedSourceIds) const
{
    auto currentFileStatus = m_fileStatusCache.find(sourceId);

    if (!currentFileStatus.isValid())
        return FileState::NotExists;

    auto projectStorageFileStatus = m_projectStorage.fetchFileStatus(sourceId);

    if (!projectStorageFileStatus.isValid() || projectStorageFileStatus != currentFileStatus) {
        fileStatuses.push_back(currentFileStatus);
        updatedSourceIds.push_back(currentFileStatus.sourceId);
        return FileState::Changed;
    }

    notUpdatedSourceIds.push_back(currentFileStatus.sourceId);
    return FileState::NotChanged;
}

} // namespace QmlDesigner
