// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectstorageupdater.h"

#include "filestatuscache.h"
#include "filesysteminterface.h"
#include "projectstorage.h"
#include "projectstoragepathwatcherinterface.h"
#include "projectstoragetracing.h"
#include "qmldocumentparserinterface.h"
#include "qmltypesparserinterface.h"
#include "sourcepathstorage/sourcepath.h"
#include "sourcepathstorage/sourcepathcache.h"
#include "typeannotationreader.h"

#include <functional.h>
#include <sqlitedatabase.h>
#include <utils/algorithm.h>
#include <utils/set_algorithm.h>

#include <QDirIterator>
#include <QRegularExpression>

#include <private/qqmldirparser_p.h>

#include <algorithm>
#include <filesystem>
#include <functional>

namespace QmlDesigner {
using NanotraceHR::keyValue;
using ProjectStorageTracing::category;
using Tracer = NanotraceHR::Tracer<ProjectStorageTracing::Category>;
using namespace Qt::StringLiterals;

template<typename String>
void convertToString(String &string, const ProjectStorageUpdater::FileState &state)
{
    switch (state) {
    case ProjectStorageUpdater::FileState::Changed:
        convertToString(string, "Changed");
        break;
    case ProjectStorageUpdater::FileState::Unchanged:
        convertToString(string, "Unchanged");
        break;
    case ProjectStorageUpdater::FileState::NotExists:
        convertToString(string, "NotExists");
        break;
    case ProjectStorageUpdater::FileState::NotExistsUnchanged:
        convertToString(string, "NotExistsUnchanged");
        break;
    case ProjectStorageUpdater::FileState::Added:
        convertToString(string, "Added");
        break;
    case ProjectStorageUpdater::FileState::Removed:
        convertToString(string, "Removed");
        break;
    }
}

namespace {

void filterMultipleEntries(QStringList &qmlTypes)
{
    std::ranges::sort(qmlTypes);
    auto removedRange = std::ranges::unique(qmlTypes);
    qmlTypes.erase(removedRange.begin(), removedRange.end());
}

QStringList prependDirectory(QStringView directory, const QStringList &qmlTypes)
{
    QStringList paths;
    paths.reserve(qmlTypes.size());

    for (QStringView qmlType : qmlTypes)
        paths += directory + '/' + qmlType;

    return paths;
}

void addProjectEntryInfoPaths(QStringList &paths,
                           const Storage::Synchronization::ProjectEntryInfos &projectEntryInfos,
                           PathCacheType &pathCache)
{
    for (const auto &projectEntryInfo : projectEntryInfos)
        paths += pathCache.sourcePath(projectEntryInfo.sourceId).toQString();
}

QList<QQmlDirParser::Import> filterMultipleEntries(QList<QQmlDirParser::Import> imports)
{
    std::ranges::stable_sort(imports, {}, &QQmlDirParser::Import::module);
    auto removedRange = std::ranges::unique(imports, {}, &QQmlDirParser::Import::module);
    imports.erase(removedRange.begin(), removedRange.end());

    return imports;
}

std::vector<Utils::PathString> toImportNames(const QList<QQmlDirParser::Import> &imports)
{
    return Utils::transform<std::vector<Utils::PathString>>(imports, &QQmlDirParser::Import::module);
}

std::vector<Utils::PathString> joinImports(const std::vector<Utils::PathString> &firstImports,
                                           const std::vector<Utils::PathString> &secondImports)
{
    std::vector<Utils::PathString> imports;
    imports.reserve(firstImports.size() + secondImports.size());
    std::ranges::set_union(firstImports, secondImports, std::back_inserter(imports));

    return imports;
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

void addSourceIds(SourceIds &sourceIds,
                  const Storage::Synchronization::ProjectEntryInfos &projectEntryInfos,
                  TracerLiteral message,
                  Tracer &tracer)
{
    for (const auto &projectEntryInfo : projectEntryInfos) {
        tracer.tick(message, keyValue("source id", projectEntryInfo.sourceId));
        sourceIds.push_back(projectEntryInfo.sourceId);
    }
}

Storage::Version convertVersion(QTypeRevision version)
{
    using Storage::Version;
    using Storage::VersionNumber;

    return Version{version.hasMajorVersion()
                       ? VersionNumber::convertFromSignedInteger(version.majorVersion())
                       : VersionNumber::noVersionNumber(),
                   version.hasMinorVersion()
                       ? VersionNumber::convertFromSignedInteger(version.minorVersion())
                       : VersionNumber::noVersionNumber()};
}

Storage::Synchronization::IsAutoVersion convertToIsAutoVersion(QQmlDirParser::Import::Flags flags,
                                                               QTypeRevision version)
{
    if (flags & QQmlDirParser::Import::Flag::Auto)
        return Storage::Synchronization::IsAutoVersion::Yes;

    if (!version.isValid())
        return Storage::Synchronization::IsAutoVersion::Yes;

    return Storage::Synchronization::IsAutoVersion::No;
}

void addExportedTypesFromQmldir(const QMultiHash<QString, QQmlDirParser::Component> &qmlDirParserComponents,
                                Utils::PathString directory,
                                ModuleId moduleId,
                                PathCacheType &pathCache,
                                Storage::Synchronization::ExportedTypes &exportedTypes,
                                SourceId qmldirSourceId)
{
    NanotraceHR::Tracer tracer{"add exported types from qmldir",
                               category(),
                               keyValue("directory", directory),
                               keyValue("module id", moduleId)};

    for (const QQmlDirParser::Component &qmlDirParserComponent : qmlDirParserComponents) {
        if (std::ranges::find(qmlDirParserComponent.fileName, '+')
            != qmlDirParserComponent.fileName.end()) {
            return;
        }

        auto filePath = Utils::PathString::join(
            {directory, "/", Utils::PathString{qmlDirParserComponent.fileName}});

        auto normalizedPath = std::filesystem::path{std::string_view{filePath}}
                                  .lexically_normal()
                                  .generic_string();

        SourcePathView sourcePath{normalizedPath};

        auto typeSourceId = pathCache.sourceId(sourcePath);
        Utils::SmallString typeName{qmlDirParserComponent.typeName};
        auto typeIdName = sourcePath.name();
        tracer.tick("add exported type",
                    keyValue("type name", typeName),
                    keyValue("type id name", typeIdName),
                    keyValue("source id", typeSourceId));
        exportedTypes.emplace_back(qmldirSourceId,
                                   moduleId,
                                   std::move(typeName),
                                   convertVersion(qmlDirParserComponent.version),
                                   typeSourceId,
                                   typeIdName);
    }
}

void addExportedTypesFromDirectory(const QStringList &qmlFileNames,
                                   DirectoryPathId directoryId,
                                   ModuleId pathModuleId,
                                   PathCacheType &pathCache,
                                   Storage::Synchronization::ExportedTypes &exportedTypes)
{
    for (QStringView qmlFileName : qmlFileNames) {
        SourceId directorySourceId = SourceId::create(directoryId);
        Utils::PathString fileName{qmlFileName};
        SourceId typeSourceId = pathCache.sourceId(directoryId, fileName);
        Utils::SmallString typeName{
            QStringView{qmlFileName.begin(), std::ranges::find(qmlFileName, '.')}};
        exportedTypes.emplace_back(directorySourceId,
                                   pathModuleId,
                                   std::move(typeName),
                                   Storage::Version{},
                                   typeSourceId,
                                   fileName);
    }
}

void addDependencies(Storage::Imports &dependencies,
                     SourceId sourceId,
                     SourceId contextSourceId,
                     const std::vector<Utils::PathString> &qmldirDependencies,
                     ModulesStorage &modulesStorage,
                     TracerLiteral message,
                     Tracer &tracer)
{
    for (std::string_view qmldirDependency : qmldirDependencies) {
        ModuleId moduleId = modulesStorage.moduleId(qmldirDependency, Storage::ModuleKind::CppLibrary);
        auto &import = dependencies.emplace_back(moduleId, Storage::Version{}, sourceId, contextSourceId);
        tracer.tick(message, keyValue("import", import));
    }
}

void addModuleExportedImport(Storage::Synchronization::ModuleExportedImports &imports,
                             ModuleId moduleId,
                             ModuleId exportedModuleId,
                             Storage::Version version,
                             Storage::Synchronization::IsAutoVersion isAutoVersion,
                             std::string_view moduleName,
                             Storage::ModuleKind moduleKind,
                             std::string_view exportedModuleName)
{
    NanotraceHR::Tracer tracer{"add module exported imports",
                               category(),
                               keyValue("module id", moduleId),
                               keyValue("exported module id", exportedModuleId),
                               keyValue("version", version),
                               keyValue("is auto version", isAutoVersion),
                               keyValue("module name", moduleName),
                               keyValue("module kind", moduleKind),
                               keyValue("exported module name", exportedModuleName)};

    imports.emplace_back(moduleId, exportedModuleId, version, isAutoVersion);
}

bool isOptionalImport(QQmlDirParser::Import::Flags flags)
{
    return flags & QQmlDirParser::Import::Optional
           && !(flags & QQmlDirParser::Import::OptionalDefault);
}

void addModuleExportedImports(Storage::Synchronization::ModuleExportedImports &imports,
                              ModuleId moduleId,
                              ModuleId cppModuleId,
                              std::string_view moduleName,
                              const QList<QQmlDirParser::Import> &qmldirImports,
                              ModulesStorage &modulesStorage)
{
    NanotraceHR::Tracer tracer{"add module exported imports",
                               category(),
                               keyValue("cpp module id", cppModuleId),
                               keyValue("module id", moduleId)};

    for (const QQmlDirParser::Import &qmldirImport : qmldirImports) {
        if (isOptionalImport(qmldirImport.flags))
            continue;

        Utils::PathString exportedModuleName{qmldirImport.module};
        using Storage::ModuleKind;
        ModuleId exportedModuleId = modulesStorage.moduleId(exportedModuleName,
                                                            ModuleKind::QmlLibrary);
        addModuleExportedImport(imports,
                                moduleId,
                                exportedModuleId,
                                convertVersion(qmldirImport.version),
                                convertToIsAutoVersion(qmldirImport.flags, qmldirImport.version),
                                moduleName,
                                ModuleKind::QmlLibrary,
                                exportedModuleName);

        ModuleId exportedCppModuleId = modulesStorage.moduleId(exportedModuleName,
                                                               ModuleKind::CppLibrary);
        addModuleExportedImport(imports,
                                cppModuleId,
                                exportedCppModuleId,
                                Storage::Version{},
                                Storage::Synchronization::IsAutoVersion::No,
                                moduleName,
                                ModuleKind::CppLibrary,
                                exportedModuleName);
    }
}

using Storage::IsInsideProject;

using WatchedSourceIds = ProjectStorageUpdater::WatchedSourceIds;

void appendIdPaths(WatchedSourceIds watchedSourceIds, ProjectPartId id, std::vector<IdPaths> &idPaths)
{
    if (watchedSourceIds.directoryIds.size())
        idPaths.emplace_back(id, SourceType::Directory, std::move(watchedSourceIds.directoryIds));

    if (watchedSourceIds.qmldirSourceIds.size())
        idPaths.emplace_back(id, SourceType::QmlDir, std::move(watchedSourceIds.qmldirSourceIds));

    if (watchedSourceIds.qmlSourceIds.size())
        idPaths.emplace_back(id, SourceType::Qml, std::move(watchedSourceIds.qmlSourceIds));

    if (watchedSourceIds.qmltypesSourceIds.size())
        idPaths.emplace_back(id, SourceType::QmlTypes, std::move(watchedSourceIds.qmltypesSourceIds));
}

void removeDuplicates(Storage::Synchronization::ExportedTypes &exportedTypes)
{
    std::ranges::sort(exportedTypes);

    auto duplicateExportedTypes = std::ranges::unique(exportedTypes);

    exportedTypes.erase(duplicateExportedTypes.begin(), duplicateExportedTypes.end());
}

} // namespace

void ProjectStorageUpdater::update(Update update)
{
    QStringList qtDirectories = std::move(update.qtDirectories);
    const QString &propertyEditorResourcesPath = update.propertyEditorResourcesPath;
    const QStringList &typeAnnotationPaths = update.typeAnnotationPaths;
    const Utils::PathString projectDirectory = update.projectDirectory;

    NanotraceHR::Tracer tracer{"update",
                               category(),
                               keyValue("Qt directories", qtDirectories),
                               keyValue("project directory", projectDirectory)};

    Storage::Synchronization::SynchronizationPackage package;
    WatchedSourceIds watchedQtSourceIds{32};
    WatchedSourceIds watchedProjectSourceIds{32};
    NotUpdatedSourceIds notUpdatedSourceIds{1024};
    DirectoryPathIds removedDirectoryIds;

    ParsedTypeInfoSourceIds parsedTypeInfos;
    updateDirectories(qtDirectories,
                      package,
                      notUpdatedSourceIds,
                      watchedQtSourceIds,
                      parsedTypeInfos,
                      removedDirectoryIds);
    if (projectDirectory.size()) {
        updateDirectory(projectDirectory,
                        {},
                        package,
                        notUpdatedSourceIds,
                        watchedProjectSourceIds,
                        parsedTypeInfos,
                        removedDirectoryIds,
                        IsInsideProject::Yes);
    }
    updatePropertyEditorPaths(propertyEditorResourcesPath, package, notUpdatedSourceIds);
    updateTypeAnnotations(typeAnnotationPaths, package, notUpdatedSourceIds);

    package.updatedFileStatusSourceIds = filterNotUpdatedSourceIds(
        std::move(package.updatedFileStatusSourceIds),
        std::move(notUpdatedSourceIds.fileStatusSourceIds));

    removeDuplicates(package.exportedTypes);

    m_projectStorage.synchronize(std::move(package));

    std::vector<IdPaths> idPaths;
    idPaths.reserve(8);
    appendIdPaths(std::move(watchedQtSourceIds), m_qtPartId, idPaths);
    appendIdPaths(std::move(watchedProjectSourceIds), m_projectPartId, idPaths);
    m_pathWatcher.updateIdPaths(idPaths);

    std::ranges::sort(removedDirectoryIds);
    m_fileStatusCache.remove(removedDirectoryIds);
}

namespace {

bool isChanged(ProjectStorageUpdater::FileState state)
{
    using FileState = ProjectStorageUpdater::FileState;
    switch (state) {
    case FileState::Changed:
    case FileState::Added:
    case FileState::Removed:
        return true;
    case FileState::NotExists:
    case FileState::NotExistsUnchanged:
    case FileState::Unchanged:
        break;
    }
    return false;
}

bool isUnchanged(ProjectStorageUpdater::FileState state)
{
    return !isChanged(state);
}

bool isChangedOrAdded(ProjectStorageUpdater::FileState state)
{
    using FileState = ProjectStorageUpdater::FileState;
    switch (state) {
    case FileState::Changed:
    case FileState::Added:
        return true;
    case FileState::Removed:
    case FileState::NotExists:
    case FileState::NotExistsUnchanged:
    case FileState::Unchanged:
        break;
    }
    return false;
}

bool isExisting(ProjectStorageUpdater::FileState state)
{
    using FileState = ProjectStorageUpdater::FileState;
    switch (state) {
    case FileState::Changed:
    case FileState::Added:
    case FileState::Unchanged:
        return true;
    case FileState::NotExistsUnchanged:
    case FileState::NotExists:
    case FileState::Removed:
        break;
    }
    return false;
}

bool everExisted(ProjectStorageUpdater::FileState state)
{
    using FileState = ProjectStorageUpdater::FileState;
    switch (state) {
    case FileState::Changed:
    case FileState::Added:
    case FileState::Unchanged:
    case FileState::Removed:
        return true;
    case FileState::NotExistsUnchanged:
    case FileState::NotExists:
        break;
    }
    return false;
}

bool isNotExisting(ProjectStorageUpdater::FileState state)
{
    return !isExisting(state);
}

std::string_view directoryName(std::string_view directoryPath)
{
    auto last = directoryPath.rfind('/');
    if (last == std::string_view::npos)
        return {""};
    return directoryPath.substr(last + 1);
}

void addRemovedImportAndTypeSourceIds(const Storage::Synchronization::ProjectEntryInfos &projectEntryInfos,
                                      const QStringList &qmlFileNames,
                                      DirectoryPathId directoryId,
                                      PathCacheType &pathCache,
                                      auto &updatedImportSourceIds,
                                      auto &updatedTypeSourceIds)
{
    auto qmlDocumentSourceIds = Utils::transform<SmallSourceIds<256>>(qmlFileNames, [&](const auto &fileName) {
        return SourceId::create(directoryId, pathCache.fileNameId(Utils::PathString{fileName}));
    });

    std::ranges::sort(qmlDocumentSourceIds);

    auto isQmlDocument = [](auto &&entry) {
        return entry.fileType == Storage::Synchronization::FileType::QmlDocument;
    };

    auto qmlDocumentProjectEntryInfoSourceIds = projectEntryInfos | std::views::filter(isQmlDocument)
                                             | std::views::transform(
                                                 &Storage::Synchronization::ProjectEntryInfo::sourceId);

    Utils::set_difference(qmlDocumentProjectEntryInfoSourceIds,
                          qmlDocumentSourceIds,
                          [&](SourceId sourceId) {
                              updatedImportSourceIds.push_back(sourceId);
                              updatedTypeSourceIds.push_back(sourceId);
                          });
}

} // namespace

void ProjectStorageUpdater::updateDirectoryChanged(Utils::SmallStringView directoryPath,
                                                   FileState directoryState,
                                                   DirectoryPathId directoryId,
                                                   Storage::Synchronization::SynchronizationPackage &package,
                                                   NotUpdatedSourceIds &notUpdatedSourceIds,
                                                   WatchedSourceIds &watchedSourceIds,
                                                   IsInsideProject isInsideProject,
                                                   Tracer &tracer)
{
    using Storage::ModuleKind;
    ModuleId pathModuleId = m_modulesStorage.moduleId(directoryPath, ModuleKind::PathLibrary);

    auto qmlProjectEntryInfos = m_projectStorage.fetchProjectEntryInfos(
        SourceId::create(directoryId), Storage::Synchronization::FileType::QmlDocument);
    std::ranges::sort(qmlProjectEntryInfos, {}, &Storage::Synchronization::ProjectEntryInfo::sourceId);
    addSourceIds(package.updatedFileStatusSourceIds,
                 qmlProjectEntryInfos,
                 "append updated file status source id",
                 tracer);

    QString directoryPathAsQString{directoryPath};

    auto qmlFileNames = m_fileSystem.fileNames(directoryPathAsQString, {"*.qml"});

    addExportedTypesFromDirectory(qmlFileNames,
                                  directoryId,
                                  pathModuleId,
                                  m_pathCache,
                                  package.exportedTypes);
    package.updatedExportedTypeSourceIds.push_back(SourceId::create(directoryId));

    addRemovedImportAndTypeSourceIds(qmlProjectEntryInfos,
                                     qmlFileNames,
                                     directoryId,
                                     m_pathCache,
                                     package.updatedImportSourceIds,
                                     package.updatedTypeSourceIds);

    parseQmlDocuments(qmlFileNames,
                      directoryPath,
                      directoryId,
                      package,
                      notUpdatedSourceIds,
                      watchedSourceIds,
                      directoryState,
                      isInsideProject);
    if (isChanged(directoryState)) {
        tracer.tick("append updated directory source id", keyValue("directory id", directoryId));
        package.updatedProjectEntryInfoSourceIds.push_back(SourceId::create(directoryId));
    }
}

void ProjectStorageUpdater::updateDirectories(const QStringList &directories,
                                              Storage::Synchronization::SynchronizationPackage &package,
                                              NotUpdatedSourceIds &notUpdatedSourceIds,
                                              WatchedSourceIds &watchedSourceIds,
                                              ParsedTypeInfoSourceIds &parsedTypeInfos,
                                              DirectoryPathIds &removedDirectoryIds)
{
    NanotraceHR::Tracer tracer{"update directories", category()};

    for (const QString &directory : directories) {
        updateDirectory(Utils::PathString{directory},
                        {},
                        package,
                        notUpdatedSourceIds,
                        watchedSourceIds,
                        parsedTypeInfos,
                        removedDirectoryIds,
                        IsInsideProject::No);
    }
}

void ProjectStorageUpdater::updateSubdirectories(Utils::SmallStringView directoryPath,
                                                 DirectoryPathId directoryId,
                                                 FileState directoryState,
                                                 const DirectoryPathIds &subdirectoriesToIgnore,
                                                 Storage::Synchronization::SynchronizationPackage &package,
                                                 NotUpdatedSourceIds &notUpdatedSourceIds,
                                                 WatchedSourceIds &watchedSourceIds,
                                                 ParsedTypeInfoSourceIds &parsedTypeInfos,
                                                 DirectoryPathIds &removedDirectoryIds,
                                                 IsInsideProject isInsideProject)
{
    struct Directory
    {
        Directory(Utils::SmallStringView path, DirectoryPathId directoryPathId)
            : path{path}
            , directoryPathId{directoryPathId}
        {}

        Utils::PathString path;
        DirectoryPathId directoryPathId;
    };

    using Directories = QVarLengthArray<Directory, 32>;

    auto subdirectoryIds = m_projectStorage.fetchSubdirectoryIds(directoryId);
    auto subdirectories = Utils::transform<Directories>(
        subdirectoryIds, [&](DirectoryPathId directoryPathId) -> Directory {
            auto subdirectoryPath = m_pathCache.directoryPath(directoryPathId);
            return {subdirectoryPath, directoryPathId};
        });

    auto exisitingSubdirectoryPaths = m_fileSystem.subdirectories(QString{directoryPath});
    Directories existingSubdirecories;

    auto skipDirectory = [&](std::string_view subdirectoryPath, DirectoryPathId directoryPathId) {
        if (subdirectoryPath.ends_with("designer"))
            return true;

        if (isInsideProject == IsInsideProject::Yes) {
            static FileNameId ignoreInQdsFileNameId = m_pathCache.fileNameId("ignore-in-qds");

            SourceId ignoreInQdsSourceId = SourceId::create(directoryPathId, ignoreInQdsFileNameId);

            auto ignoreInQdsState = fileState(ignoreInQdsSourceId, package, notUpdatedSourceIds);

            if (isExisting(ignoreInQdsState))
                return true;
        } else {
            if (subdirectoryPath.ends_with("/QtQuick/Scene2D")
                || subdirectoryPath.ends_with("/QtQuick/Scene3D"))
                return true;
        }

        return false;
    };

    for (Utils::PathString subdirectoryPath : exisitingSubdirectoryPaths) {
        DirectoryPathId directoryPathId = m_pathCache.directoryPathId(subdirectoryPath);

        if (skipDirectory(subdirectoryPath, directoryPathId))
            continue;

        subdirectories.emplace_back(subdirectoryPath, directoryPathId);
        existingSubdirecories.emplace_back(subdirectoryPath, directoryPathId);
    }

    std::ranges::sort(subdirectories, {}, &Directory::directoryPathId);
    auto removed = std::ranges::unique(subdirectories, {}, &Directory::directoryPathId);
    subdirectories.erase(removed.begin(), removed.end());

    auto updateDirectory = [&](const Directory &subdirectory) {
        this->updateDirectory(subdirectory.path,
                              subdirectoriesToIgnore,
                              package,
                              notUpdatedSourceIds,
                              watchedSourceIds,
                              parsedTypeInfos,
                              removedDirectoryIds,
                              isInsideProject);
    };

    Utils::set_greedy_difference(subdirectories,
                                 subdirectoriesToIgnore,
                                 updateDirectory,
                                 {},
                                 &Directory::directoryPathId);

    if (isChangedOrAdded(directoryState)) {
        for (const auto &[subdirectoryPath, subdirectoryId] : existingSubdirecories) {
            package.projectEntryInfos.emplace_back(SourceId::create(directoryId),
                                                SourceId::create(subdirectoryId),
                                                ModuleId{},
                                                Storage::Synchronization::FileType::Directory);
        }
    }
}

void ProjectStorageUpdater::annotationDirectoryChanged(
    Utils::SmallStringView directoryPath,
    DirectoryPathId directoryId,
    DirectoryPathId annotationDirectoryId,
    ModuleId moduleId,
    Storage::Synchronization::SynchronizationPackage &package)
{
    NanotraceHR::Tracer tracer{"annotation directory changed",
                               category(),
                               keyValue("directory path", directoryPath),
                               keyValue("directory id", directoryId)};

    package.updatedPropertyEditorQmlPathDirectoryIds.push_back(annotationDirectoryId);

    updatePropertyEditorFiles(directoryPath, annotationDirectoryId, moduleId, package);
}

void ProjectStorageUpdater::updatePropertyEditorFiles(
    Utils::SmallStringView directoryPath,
    DirectoryPathId directoryId,
    ModuleId moduleId,
    Storage::Synchronization::SynchronizationPackage &package)
{
    NanotraceHR::Tracer tracer{"update property editor files",
                               category(),
                               keyValue("directory path", directoryPath),
                               keyValue("directory id", directoryId)};

    const auto fileNames = m_fileSystem.fileNames(QString{directoryPath},
                                                  {"*Pane.qml",
                                                   "*Specifics.qml",
                                                   "*SpecificsDynamic.qml"});
    for (const QString &fileName : fileNames)
        updatePropertyEditorFile(fileName, directoryId, moduleId, package);
}

void ProjectStorageUpdater::updatePropertyEditorFile(
    const QString &fileName,
    DirectoryPathId directoryId,
    ModuleId moduleId,
    Storage::Synchronization::SynchronizationPackage &package)
{
    Utils::PathString propertyEditorFileName{fileName};
    auto propertyEditorSourceId = m_pathCache.sourceId(directoryId, propertyEditorFileName);

    QRegularExpression regex{R"xo((\w+)(Specifics|Pane|SpecificsDynamic).qml)xo"};
    auto match = regex.match(fileName);

    Storage::TypeNameString typeName{match.capturedView(1)};

    package.propertyEditorQmlPaths.emplace_back(moduleId, typeName, propertyEditorSourceId, directoryId);
}

namespace {

void addUpdateSourceIds(Storage::Synchronization::SynchronizationPackage &package,
                        const Storage::Synchronization::ProjectEntryInfos &projectEntryInfos)
{
    for (const Storage::Synchronization::ProjectEntryInfo &projectEntryInfo : projectEntryInfos) {
        package.updatedTypeSourceIds.push_back(projectEntryInfo.sourceId);
        package.updatedFileStatusSourceIds.push_back(projectEntryInfo.sourceId);
        package.updatedImportSourceIds.push_back(projectEntryInfo.sourceId);
    }
}

} // namespace

void ProjectStorageUpdater::updateDirectory(Utils::SmallStringView directoryPath,
                                            const DirectoryPathIds &subdirectoriesToIgnore,
                                            Storage::Synchronization::SynchronizationPackage &package,
                                            NotUpdatedSourceIds &notUpdatedSourceIds,
                                            WatchedSourceIds &watchedSourceIds,
                                            ParsedTypeInfoSourceIds &parsedTypeInfos,
                                            DirectoryPathIds &removedDirectoryIds,
                                            IsInsideProject isInsideProject)
{
    NanotraceHR::Tracer tracer{"update directory", category(), keyValue("directory", directoryPath)};

    DirectoryPathId directoryId = m_pathCache.directoryPathId(directoryPath);

    auto directoryState = fileState(directoryId, package, notUpdatedSourceIds);
    if (isExisting(directoryState))
        watchedSourceIds.directoryIds.push_back(SourceId::create(directoryId));

    switch (directoryState) {
    case FileState::Added:
    case FileState::Changed:
        tracer.tick("update directory changed");
        updateDirectoryChanged(directoryPath,
                               directoryState,
                               directoryId,
                               package,
                               notUpdatedSourceIds,
                               watchedSourceIds,
                               isInsideProject,
                               tracer);
        break;
    case FileState::Unchanged:
        tracer.tick("update directory not changed");

        parseProjectEntryInfos(
            m_projectStorage.fetchProjectEntryInfos(SourceId::create(directoryId),
                                                 Storage::Synchronization::FileType::QmlDocument),
            package,
            notUpdatedSourceIds,
            watchedSourceIds,
            parsedTypeInfos,
            isInsideProject);
        break;
    case FileState::Removed: {
        tracer.tick("update directory don't exits");

        removedDirectoryIds.push_back(directoryId);

        package.updatedExportedTypeSourceIds.push_back(SourceId::create(directoryId));
        package.updatedProjectEntryInfoSourceIds.push_back(SourceId::create(directoryId));
        auto qmlDocumentProjectEntryInfos = m_projectStorage.fetchProjectEntryInfos(
            SourceId::create(directoryId), Storage::Synchronization::FileType::QmlDocument);

        addUpdateSourceIds(package, qmlDocumentProjectEntryInfos);
        break;
    }
    case FileState::NotExists:
    case FileState::NotExistsUnchanged:
        break;
    }

    updateQmldir(directoryId,
                 directoryPath,
                 package,
                 notUpdatedSourceIds,
                 watchedSourceIds,
                 parsedTypeInfos,
                 isInsideProject);

    updateSubdirectories(directoryPath,
                         directoryId,
                         directoryState,
                         subdirectoriesToIgnore,
                         package,
                         notUpdatedSourceIds,
                         watchedSourceIds,
                         parsedTypeInfos,
                         removedDirectoryIds,
                         isInsideProject);

    tracer.end(keyValue("directory state", directoryState));
}

void ProjectStorageUpdater::updateQmldirChanged(
    const Storage::Synchronization::ProjectEntryInfos &qmltypesProjectEntryInfos,
    Utils::SmallStringView directoryPath,
    DirectoryPathId directoryId,
    SourceId qmldirSourceId,
    Utils::SmallStringView qmldirPath,
    FileState qmldirState,
    PathCacheType &pathCache,
    Storage::Synchronization::SynchronizationPackage &package,
    NotUpdatedSourceIds &notUpdatedSourceIds,
    WatchedSourceIds &watchedSourceIds,
    ParsedTypeInfoSourceIds &parsedTypeInfos,
    IsInsideProject isInsideProject)
{
    NanotraceHR::Tracer tracer{"update qmldir changed", category()};

    QQmlDirParser parser;
    parser.parse(m_fileSystem.contentAsQString(QString{qmldirPath}));

    using Storage::ModuleKind;
    Utils::PathString moduleName{parser.typeNamespace()};
    if (moduleName.empty())
        moduleName = directoryName(directoryPath);
    ModuleId moduleId = m_modulesStorage.moduleId(moduleName, ModuleKind::QmlLibrary);
    ModuleId cppModuleId = m_modulesStorage.moduleId(moduleName, ModuleKind::CppLibrary);

    addExportedTypesFromQmldir(parser.components(),
                               directoryPath,
                               moduleId,
                               pathCache,
                               package.exportedTypes,
                               qmldirSourceId);

    package.updatedExportedTypeSourceIds.push_back(qmldirSourceId);

    auto imports = filterMultipleEntries(parser.imports());

    addModuleExportedImports(
        package.moduleExportedImports, moduleId, cppModuleId, moduleName, imports, m_modulesStorage);
    tracer.tick("append updated module id", keyValue("module id", moduleId));
    package.updatedModuleIds.push_back(moduleId);
    package.updatedModuleIds.push_back(cppModuleId);

    addSourceIds(package.updatedFileStatusSourceIds,
                 qmltypesProjectEntryInfos,
                 "append updated file status source id",
                 tracer);

    auto qmlTypes = prependDirectory(QString{directoryPath}, parser.typeInfos());
    addProjectEntryInfoPaths(qmlTypes, qmltypesProjectEntryInfos, pathCache);

    filterMultipleEntries(qmlTypes);

    QString directoryPathAsQString{directoryPath};

    if (!qmlTypes.isEmpty()) {
        parseTypeInfos(qmlTypes,
                       toImportNames(filterMultipleEntries(parser.dependencies())),
                       toImportNames(imports),
                       qmldirSourceId,
                       directoryPathAsQString,
                       qmldirState,
                       cppModuleId,
                       package,
                       notUpdatedSourceIds,
                       watchedSourceIds,
                       parsedTypeInfos,
                       isInsideProject);
        package.updatedProjectEntryInfoSourceIds.push_back(qmldirSourceId);
    }

    updateAnnotationDirectory(
        directoryPath, moduleId, directoryId, package, notUpdatedSourceIds, isInsideProject);
}

void ProjectStorageUpdater::updateAnnotationDirectory(
    Utils::SmallStringView directoryPath,
    ModuleId moduleId,
    DirectoryPathId directoryId,
    Storage::Synchronization::SynchronizationPackage &package,
    NotUpdatedSourceIds &notUpdatedSourceIds,
    IsInsideProject isInsideProject)
{
    if (isInsideProject == IsInsideProject::Yes) {
        SourcePath annotationDirectoryPath{Utils::PathString{directoryPath} + "/designer"};
        auto annotationDirectoryId = m_pathCache.directoryPathId(annotationDirectoryPath);
        auto annotationDirectoryState = fileState(annotationDirectoryId, package, notUpdatedSourceIds);

        if (isExisting(annotationDirectoryState)) {
            annotationDirectoryChanged(annotationDirectoryPath,
                                       directoryId,
                                       annotationDirectoryId,
                                       moduleId,
                                       package);
        } else if (annotationDirectoryState == FileState::Removed) {
            package.updatedPropertyEditorQmlPathDirectoryIds.push_back(annotationDirectoryId);
        }
    }
}

void ProjectStorageUpdater::removeAnnotationDirectory(
    Utils::SmallStringView directoryPath,
    Storage::Synchronization::SynchronizationPackage &package,
    NotUpdatedSourceIds &notUpdatedSourceIds,
    IsInsideProject isInsideProject)
{
    if (isInsideProject == IsInsideProject::Yes) {
        SourcePath annotationDirectoryPath{Utils::PathString{directoryPath} + "/designer"};
        auto annotationDirectoryId = m_pathCache.directoryPathId(annotationDirectoryPath);
        auto annotationDirectoryState = fileState(annotationDirectoryId, package, notUpdatedSourceIds);
        if (everExisted(annotationDirectoryState))
            package.updatedPropertyEditorQmlPathDirectoryIds.push_back(annotationDirectoryId);
    }
}

void ProjectStorageUpdater::updateQmldir(DirectoryPathId directoryId,
                                         Utils::SmallStringView directoryPath,
                                         Storage::Synchronization::SynchronizationPackage &package,
                                         NotUpdatedSourceIds &notUpdatedSourceIds,
                                         WatchedSourceIds &watchedSourceIds,
                                         ParsedTypeInfoSourceIds &parsedTypeInfos,
                                         IsInsideProject isInsideProject)
{
    NanotraceHR::Tracer tracer{"update qmldir", category(), keyValue("directory", directoryPath)};

    FileNameId qmldirFileNameId = m_pathCache.fileNameId("qmldir");
    SourceId qmldirSourceId = SourceId::create(directoryId, qmldirFileNameId);
    SourcePath qmldirPath{directoryPath, "qmldir"};

    auto qmldirState = fileState(qmldirSourceId, package, notUpdatedSourceIds);
    if (isExisting(qmldirState))
        watchedSourceIds.qmldirSourceIds.push_back(qmldirSourceId);

    auto qmltypesProjectEntryInfos = m_projectStorage.fetchProjectEntryInfos(
        qmldirSourceId, Storage::Synchronization::FileType::QmlTypes);

    switch (qmldirState) {
    case FileState::Added:
    case FileState::Changed:
        tracer.tick("update qmldir changed");

        updateQmldirChanged(qmltypesProjectEntryInfos,
                            directoryPath,
                            directoryId,
                            qmldirSourceId,
                            qmldirPath,
                            qmldirState,
                            m_pathCache,
                            package,
                            notUpdatedSourceIds,
                            watchedSourceIds,
                            parsedTypeInfos,
                            isInsideProject);
        break;
    case FileState::Unchanged:
        tracer.tick("update qmldir not changed");

        parseQmltypesProjectEntryInfos(qmltypesProjectEntryInfos,
                                    package,
                                    notUpdatedSourceIds,
                                    watchedSourceIds,
                                    parsedTypeInfos,
                                    isInsideProject);
        break;
    case FileState::Removed: {
        tracer.tick("update qmldir don't exits");

        package.updatedProjectEntryInfoSourceIds.push_back(qmldirSourceId);
        package.updatedExportedTypeSourceIds.push_back(qmldirSourceId);
        for (const Storage::Synchronization::ProjectEntryInfo &projectEntryInfo : qmltypesProjectEntryInfos) {
            tracer.tick("append updated source id", keyValue("source id", projectEntryInfo.sourceId));
            package.updatedTypeSourceIds.push_back(projectEntryInfo.sourceId);
            tracer.tick("append updated file status source id",
                        keyValue("source id", projectEntryInfo.sourceId));
            package.updatedFileStatusSourceIds.push_back(projectEntryInfo.sourceId);
            package.updatedImportSourceIds.push_back(projectEntryInfo.sourceId);

            parsedTypeInfos.push_back(projectEntryInfo.sourceId);
        }

        removeAnnotationDirectory(directoryPath, package, notUpdatedSourceIds, isInsideProject);
        break;
    }
    case FileState::NotExists:
    case FileState::NotExistsUnchanged:
        break;
    }
}

void ProjectStorageUpdater::updatePropertyEditorPaths(
    const QString &propertyEditorResourcesPath,
    Storage::Synchronization::SynchronizationPackage &package,
    NotUpdatedSourceIds &notUpdatedSourceIds)
{
    NanotraceHR::Tracer tracer{"update property editor paths",
                               category(),
                               keyValue("property editor resources path", propertyEditorResourcesPath)};

    if (propertyEditorResourcesPath.isEmpty())
        return;

    QDirIterator dirIterator{QDir::cleanPath(propertyEditorResourcesPath),
                             QDir::Dirs | QDir::NoDotAndDotDot,
                             QDirIterator::Subdirectories};

    while (dirIterator.hasNext()) {
        auto pathInfo = dirIterator.nextFileInfo();

        DirectoryPathId directoryId = m_pathCache.directoryPathId(
            Utils::PathString{pathInfo.filePath()});

        auto state = fileState(directoryId, package, notUpdatedSourceIds);

        if (isChanged(state)) {
            updatePropertyEditorPath(pathInfo.filePath(),
                                     package,
                                     directoryId,
                                     propertyEditorResourcesPath.size() + 1);
        }
    }
}

namespace {

template<typename Result = SmallSourceIds<16>>
Result mergedSourceIds(Utils::span<SourceId> sourceIds1, Utils::span<SourceId> sourceIds2)
{
    Result mergedSourceIds;
    mergedSourceIds.reserve(static_cast<Result::size_type>(sourceIds1.size() + sourceIds2.size()));

    std::ranges::set_union(sourceIds1, sourceIds2, std::back_inserter(mergedSourceIds));

    return mergedSourceIds;
}
} // namespace

void ProjectStorageUpdater::updateTypeAnnotations(const QStringList &directoryPaths,
                                                  Storage::Synchronization::SynchronizationPackage &package,
                                                  NotUpdatedSourceIds &notUpdatedSourceIds)
{
    NanotraceHR::Tracer tracer("update type annotations", category());

    std::map<DirectoryPathId, SmallSourceIds<16>> updatedSourceIdsDictonary;

    for (DirectoryPathId directoryId : m_projectStorage.typeAnnotationDirectoryIds())
        updatedSourceIdsDictonary[directoryId] = {};

    for (const auto &directoryPath : directoryPaths)
        updateTypeAnnotations(directoryPath, package, notUpdatedSourceIds, updatedSourceIdsDictonary);

    updateTypeAnnotationDirectories(package, notUpdatedSourceIds, updatedSourceIdsDictonary);
}

void ProjectStorageUpdater::updateTypeAnnotations(
    const QString &rootDirectoryPath,
    Storage::Synchronization::SynchronizationPackage &package,
    NotUpdatedSourceIds &notUpdatedSourceIds,
    std::map<DirectoryPathId, SmallSourceIds<16>> &updatedSourceIdsDictonary)
{
    NanotraceHR::Tracer tracer("update type annotation directory",
                               category(),
                               keyValue("path", rootDirectoryPath));

    if (rootDirectoryPath.isEmpty())
        return;

    QDirIterator directoryIterator{rootDirectoryPath,
                                   {"*.metainfo"},
                                   QDir::NoDotAndDotDot | QDir::Files,
                                   QDirIterator::Subdirectories};

    while (directoryIterator.hasNext()) {
        auto fileInfo = directoryIterator.nextFileInfo();
        auto filePath = fileInfo.filePath();
        SourceId sourceId = m_pathCache.sourceId(SourcePath{filePath});

        auto directoryPath = fileInfo.canonicalPath();

        DirectoryPathId directoryId = m_pathCache.directoryPathId(Utils::PathString{directoryPath});

        auto state = fileState(sourceId, package, notUpdatedSourceIds);
        if (isChangedOrAdded(state))
            updateTypeAnnotation(directoryPath, fileInfo.filePath(), sourceId, directoryId, package);

        if (isChanged(state))
            updatedSourceIdsDictonary[directoryId].push_back(sourceId);
    }
}

void ProjectStorageUpdater::updateTypeAnnotationDirectories(
    Storage::Synchronization::SynchronizationPackage &package,
    NotUpdatedSourceIds &notUpdatedSourceIds,
    std::map<DirectoryPathId, SmallSourceIds<16>> &updatedSourceIdsDictonary)
{
    for (auto &[directoryId, updatedSourceIds] : updatedSourceIdsDictonary) {
        auto directoryState = fileState(directoryId, package, notUpdatedSourceIds);

        if (isChanged(directoryState)) {
            auto storedTypeAnnotationSourceIds = m_projectStorage.typeAnnotationSourceIds(directoryId);
            storedTypeAnnotationSourceIds.removeIf(
                [&](SourceId sourceId) { return m_fileStatusCache.find(sourceId).isExisting(); });

            std::ranges::sort(updatedSourceIds);

            auto changedSourceIds = mergedSourceIds(storedTypeAnnotationSourceIds, updatedSourceIds);
            package.updatedTypeAnnotationSourceIds.insert(package.updatedTypeAnnotationSourceIds.end(),
                                                          changedSourceIds.begin(),
                                                          changedSourceIds.end());
        } else {
            package.updatedTypeAnnotationSourceIds.insert(package.updatedTypeAnnotationSourceIds.end(),
                                                          updatedSourceIds.begin(),
                                                          updatedSourceIds.end());
        }
    }
}

namespace {
QString contentFromFile(const QString &path)
{
    QFile file{path};
    if (file.open(QIODevice::ReadOnly))
        return QString::fromUtf8(file.readAll());

    return {};
}
} // namespace

void ProjectStorageUpdater::updateTypeAnnotation(const QString &directoryPath,
                                                 const QString &filePath,
                                                 SourceId sourceId,
                                                 DirectoryPathId directoryId,
                                                 Storage::Synchronization::SynchronizationPackage &package)
{
    NanotraceHR::Tracer tracer{"update type annotation path",
                               category(),
                               keyValue("path", filePath),
                               keyValue("directory path", directoryPath)};

    Storage::TypeAnnotationReader reader{m_modulesStorage};

    auto annotations = reader.parseTypeAnnotation(contentFromFile(filePath),
                                                  directoryPath,
                                                  sourceId,
                                                  directoryId);
    auto &typeAnnotations = package.typeAnnotations;
    package.typeAnnotations.insert(typeAnnotations.end(),
                                   std::make_move_iterator(annotations.begin()),
                                   std::make_move_iterator(annotations.end()));
}

void ProjectStorageUpdater::updatePropertyEditorPath(
    const QString &directoryPath,
    Storage::Synchronization::SynchronizationPackage &package,
    DirectoryPathId directoryId,
    long long pathOffset)
{
    NanotraceHR::Tracer tracer{"update property editor path",
                               category(),
                               keyValue("directory path", directoryPath),
                               keyValue("directory id", directoryId)};

    tracer.tick("append updated property editor qml path source id",
                keyValue("source id", directoryId));
    package.updatedPropertyEditorQmlPathDirectoryIds.push_back(directoryId);
    auto dir = QDir{directoryPath};
    const auto fileInfos = dir.entryInfoList({"*Pane.qml", "*Specifics.qml"}, QDir::Files);
    for (const auto &fileInfo : fileInfos)
        updatePropertyEditorFilePath(fileInfo.filePath(), package, directoryId, pathOffset);
}

void ProjectStorageUpdater::updatePropertyEditorFilePath(
    const QString &path,
    Storage::Synchronization::SynchronizationPackage &package,
    DirectoryPathId directoryId,
    long long pathOffset)
{
    NanotraceHR::Tracer tracer{"update property editor file path",
                               category(),
                               keyValue("directory path", path),
                               keyValue("directory id", directoryId)};

    QRegularExpression regex{R"xo((.+)\/(\w+)(Specifics|Pane).qml)xo"};
    auto typePath = QStringView{path}.mid(pathOffset);
    auto match = regex.matchView(typePath);
    QString oldModuleName;
    ModuleId moduleId;
    if (match.hasMatch()) {
        auto moduleName = match.capturedView(1).toString();
        moduleName.replace('/', '.');
        if (oldModuleName != moduleName) {
            oldModuleName = moduleName;
            moduleId = m_modulesStorage.moduleId(Utils::SmallString{moduleName},
                                                 Storage::ModuleKind::QmlLibrary);
        }
        Storage::TypeNameString typeName{match.capturedView(2)};
        SourceId pathId = m_pathCache.sourceId(SourcePath{path});
        const auto &paths = package.propertyEditorQmlPaths.emplace_back(moduleId,
                                                                        typeName,
                                                                        pathId,
                                                                        directoryId);
        tracer.tick("append property editor qml paths", keyValue("property editor qml paths", paths));
    }
}

namespace {

template<typename Container, typename Id>
bool contains(const Container &container, Id id)
{
    return std::find(container.begin(), container.end(), id) != container.end();
}

struct ProjectChunkSourceIds
{
    ProjectChunkSourceIds()
    {
        directory.reserve(32);
        qmldir.reserve(32);
        qmlDocument.reserve(128);
        qmltypes.reserve(32);
    }

    SourceIds directory;
    SourceIds qmldir;
    SourceIds qmlDocument;
    SourceIds qmltypes;
};

void appendProjectChunkSourceIds(ProjectStorageUpdater::ProjectChunkSourceIds &ids,
                                 const ProjectChunkId &projectChunkId,
                                 SourceIds sourceIds)
{
    std::ranges::sort(sourceIds);
    auto removed = std::ranges::unique(sourceIds);
    sourceIds.erase(removed.begin(), removed.end());

    switch (projectChunkId.sourceType) {
    case SourceType::Directory:
        ids.directory = mergedSourceIds<SourceIds>(ids.directory, sourceIds);
        break;
    case SourceType::QmlDir:
        ids.qmldir = mergedSourceIds<SourceIds>(ids.qmldir, sourceIds);
        break;
    case SourceType::Qml:
        ids.qmlDocument = mergedSourceIds<SourceIds>(ids.qmlDocument, sourceIds);
        break;
    case SourceType::QmlTypes:
        ids.qmltypes = mergedSourceIds<SourceIds>(ids.qmltypes, sourceIds);
        break;
    }
}

} // namespace

void ProjectStorageUpdater::pathsWithIdsChanged(const std::vector<IdPaths> &changedIdPaths)
{
    NanotraceHR::Tracer tracer{"paths with ids changed",
                               category(),
                               keyValue("id paths", changedIdPaths)};

    try {
        Storage::Synchronization::SynchronizationPackage package;

        WatchedSourceIds watchedQtSourceIds{10};
        WatchedSourceIds watchedProjectSourceIds{10};
        NotUpdatedSourceIds notUpdatedSourceIds{10};
        DirectoryPathIds removedDirectoryIds;
        std::vector<IdPaths> idPaths;
        idPaths.reserve(4);

        auto &project = m_projectSourceIds;
        auto &qt = m_qtSourceIds;

        for (const auto &[projectChunkId, sourceIds] : changedIdPaths) {
            if (projectChunkId.id == m_projectPartId)
                appendProjectChunkSourceIds(project, projectChunkId, sourceIds);
            else if (projectChunkId.id == m_qtPartId)
                appendProjectChunkSourceIds(qt, projectChunkId, sourceIds);
        }

        ParsedTypeInfoSourceIds parsedTypeInfos;

        auto updateDirectory = [&](const SourceIds &directorySourceIds,
                                   WatchedSourceIds &watchedSourceIds,
                                   ParsedTypeInfoSourceIds &parsedTypeInfos,
                                   IsInsideProject isInsideProject) {
            auto directoryIds = Utils::transform(directorySourceIds, &SourceId::directoryPathId);
            for (auto directoryId : directoryIds) {
                Utils::PathString directory = m_pathCache.directoryPath(directoryId);
                this->updateDirectory(directory,
                                      directoryIds,
                                      package,
                                      notUpdatedSourceIds,
                                      watchedSourceIds,
                                      parsedTypeInfos,
                                      removedDirectoryIds,
                                      isInsideProject);
            }

            return directoryIds;
        };

        auto qtDirectoryIds = updateDirectory(qt.directory,
                                              watchedQtSourceIds,
                                              parsedTypeInfos,
                                              IsInsideProject::No);
        auto projectDirectoryIds = updateDirectory(project.directory,
                                                   watchedProjectSourceIds,
                                                   parsedTypeInfos,
                                                   IsInsideProject::Yes);

        auto updateQmldir = [&](const SourceIds &qmldirSourceIds,
                                const DirectoryPathIds &directoryIds,
                                WatchedSourceIds &watchedSourceIds,
                                ParsedTypeInfoSourceIds &parsedTypeInfos,
                                IsInsideProject isInsideProject) {
            for (auto qmldirSourceId : qmldirSourceIds) {
                if (contains(directoryIds, qmldirSourceId.directoryPathId()))
                    continue;

                Utils::PathString directory = m_pathCache.directoryPath(
                    qmldirSourceId.directoryPathId());
                this->updateQmldir(qmldirSourceId.directoryPathId(),
                                   directory,
                                   package,
                                   notUpdatedSourceIds,
                                   watchedSourceIds,
                                   parsedTypeInfos,
                                   isInsideProject);
            }
        };

        updateQmldir(qt.qmldir, qtDirectoryIds, watchedQtSourceIds, parsedTypeInfos, IsInsideProject::No);
        updateQmldir(project.qmldir,
                     projectDirectoryIds,
                     watchedProjectSourceIds,
                     parsedTypeInfos,
                     IsInsideProject::Yes);

        auto parseQmlDocument = [&](const SourceIds &qmlDocumentSourceIds,
                                    const DirectoryPathIds &directoryIds,
                                    IsInsideProject isInsideProject) {
            for (SourceId sourceId : qmlDocumentSourceIds) {
                if (!contains(directoryIds, sourceId.directoryPathId()))
                    this->parseQmlDocument(sourceId, package, notUpdatedSourceIds, isInsideProject);
            }
        };

        parseQmlDocument(qt.qmlDocument, qtDirectoryIds, IsInsideProject::No);
        parseQmlDocument(project.qmlDocument, projectDirectoryIds, IsInsideProject::Yes);

        auto parseTypeInfo = [&](const SourceIds &qmltypesSourceIds,
                                 ParsedTypeInfoSourceIds &parsedTypeInfos,
                                 IsInsideProject isInsideProject) {
            for (SourceId sourceId : qmltypesSourceIds) {
                if (parsedTypeInfos.contains(sourceId))
                    continue;

                QString qmltypesPath{m_pathCache.sourcePath(sourceId)};
                auto projectEntryInfo = m_projectStorage.fetchProjectEntryInfo(sourceId);
                if (projectEntryInfo)
                    this->parseTypeInfo(*projectEntryInfo,
                                        qmltypesPath,
                                        package,
                                        notUpdatedSourceIds,
                                        parsedTypeInfos,
                                        isInsideProject);
            }
        };

        parseTypeInfo(qt.qmltypes, parsedTypeInfos, IsInsideProject::No);
        parseTypeInfo(project.qmltypes, parsedTypeInfos, IsInsideProject::Yes);

        package.updatedFileStatusSourceIds = filterNotUpdatedSourceIds(
            std::move(package.updatedFileStatusSourceIds),
            std::move(notUpdatedSourceIds.fileStatusSourceIds));

        removeDuplicates(package.exportedTypes);

        m_projectStorage.synchronize(std::move(package));

        auto directoryIdsSize = projectDirectoryIds.size() + qtDirectoryIds.size();
        DirectoryPathIds directoryIds;
        std::vector<IdPaths> newIdPaths;
        idPaths.reserve(8);
        appendIdPaths(std::move(watchedQtSourceIds), m_qtPartId, newIdPaths);
        appendIdPaths(std::move(watchedProjectSourceIds), m_projectPartId, newIdPaths);

        directoryIds.reserve(directoryIdsSize);
        std::ranges::merge(projectDirectoryIds, qtDirectoryIds, std::back_inserter(directoryIds));
        if (directoryIdsSize > 0 or newIdPaths.size() > 0)
            m_pathWatcher.updateContextIdPaths(newIdPaths, directoryIds);

        std::ranges::sort(removedDirectoryIds);
        m_fileStatusCache.remove(removedDirectoryIds);

        m_projectSourceIds.clear();
        m_qtSourceIds.clear();

    } catch (const Sqlite::Exception &exception) {
        tracer.tick("sqlite exception thrown",
                    keyValue("what", exception.what()),
                    keyValue("location", exception.location()));
    } catch (const std::exception &exception) {
        tracer.tick("standard exception thrown", keyValue("what", exception.what()));
    } catch (...) {
        tracer.tick("unkown exception thrown");
    }
}

void ProjectStorageUpdater::pathsChanged(const SourceIds &) {}

void ProjectStorageUpdater::parseTypeInfos(const QStringList &typeInfos,
                                           const std::vector<Utils::PathString> &qmldirDependencies,
                                           const std::vector<Utils::PathString> &qmldirImports,
                                           SourceId qmldirSourceId,
                                           const QString &directoryPath,
                                           FileState qmldirState,
                                           ModuleId moduleId,
                                           Storage::Synchronization::SynchronizationPackage &package,
                                           NotUpdatedSourceIds &notUpdatedSourceIds,
                                           WatchedSourceIds &watchedSourceIds,
                                           ParsedTypeInfoSourceIds &parsedTypeInfos,
                                           IsInsideProject isInsideProject)
{
    NanotraceHR::Tracer tracer{"parse type infos",
                               category(),
                               keyValue("qmldir source id", qmldirSourceId),
                               keyValue("directory path", directoryPath),
                               keyValue("module id", moduleId)};

    for (const QString &qmltypesPath : typeInfos) {
        NanotraceHR::Tracer tracer{"parse type info", category(), keyValue("type info", qmltypesPath)};

        SourceId sourceId = m_pathCache.sourceId(SourcePath{qmltypesPath});

        if (isChanged(qmldirState)) {
            tracer.tick("append module dependencies", keyValue("source id", sourceId));
            package.updatedModuleDependencySourceIds.push_back(sourceId);
            addDependencies(package.moduleDependencies,
                            sourceId,
                            qmldirSourceId,
                            joinImports(qmldirDependencies, qmldirImports),
                            m_modulesStorage,
                            "append module dependency",
                            tracer);
        }

        tracer.tick("append qmltypes source id", keyValue("source id", sourceId));
        watchedSourceIds.qmltypesSourceIds.push_back(sourceId);

        const Storage::Synchronization::ProjectEntryInfo projectEntryInfo{
            qmldirSourceId, sourceId, moduleId, Storage::Synchronization::FileType::QmlTypes};

        parseTypeInfo(projectEntryInfo,
                      qmltypesPath,
                      package,
                      notUpdatedSourceIds,
                      parsedTypeInfos,
                      isInsideProject);

        if (isChanged(qmldirState)) {
            package.projectEntryInfos.push_back(projectEntryInfo);
            tracer.tick("append directory info", keyValue("source id", projectEntryInfo.sourceId));
        }
    }
}

void ProjectStorageUpdater::parseProjectEntryInfos(
    const Storage::Synchronization::ProjectEntryInfos &projectEntryInfos,
    Storage::Synchronization::SynchronizationPackage &package,
    NotUpdatedSourceIds &notUpdatedSourceIds,
    WatchedSourceIds &watchedSourceIds,
    ParsedTypeInfoSourceIds &parsedTypeInfos,
    IsInsideProject isInsideProject)
{
    NanotraceHR::Tracer tracer{"parse project datas", category()};

    for (const Storage::Synchronization::ProjectEntryInfo &projectEntryInfo : projectEntryInfos) {
        switch (projectEntryInfo.fileType) {
        case Storage::Synchronization::FileType::QmlTypes: {
            watchedSourceIds.qmltypesSourceIds.push_back(projectEntryInfo.sourceId);

            QString qmltypesPath{m_pathCache.sourcePath(projectEntryInfo.sourceId)};
            parseTypeInfo(projectEntryInfo,
                          qmltypesPath,
                          package,
                          notUpdatedSourceIds,
                          parsedTypeInfos,
                          isInsideProject);
            break;
        }
        case Storage::Synchronization::FileType::QmlDocument: {
            watchedSourceIds.qmlSourceIds.push_back(projectEntryInfo.sourceId);

            parseQmlDocument(projectEntryInfo.sourceId, package, notUpdatedSourceIds, isInsideProject);
            break;
        }
        case Storage::Synchronization::FileType::Directory:
            break;
        }
    }
}

void ProjectStorageUpdater::parseQmltypesProjectEntryInfos(
    const Storage::Synchronization::ProjectEntryInfos &projectEntryInfos,
    Storage::Synchronization::SynchronizationPackage &package,
    NotUpdatedSourceIds &notUpdatedSourceIds,
    WatchedSourceIds &watchedSourceIds,
    ParsedTypeInfoSourceIds &parsedTypeInfos,
    IsInsideProject isInsideProject)
{
    NanotraceHR::Tracer tracer{"parse project entries", category()};

    for (const Storage::Synchronization::ProjectEntryInfo &projectEntryInfo : projectEntryInfos) {
        watchedSourceIds.qmltypesSourceIds.push_back(projectEntryInfo.sourceId);

        QString qmltypesPath{m_pathCache.sourcePath(projectEntryInfo.sourceId)};
        parseTypeInfo(projectEntryInfo,
                      qmltypesPath,
                      package,
                      notUpdatedSourceIds,
                      parsedTypeInfos,
                      isInsideProject);
    }
}

void ProjectStorageUpdater::parseTypeInfo(const Storage::Synchronization::ProjectEntryInfo &projectEntryInfo,
                                          const QString &qmltypesPath,
                                          Storage::Synchronization::SynchronizationPackage &package,
                                          NotUpdatedSourceIds &notUpdatedSourceIds,
                                          ParsedTypeInfoSourceIds &parsedTypeInfos,
                                          IsInsideProject isInsideProject)
{
    NanotraceHR::Tracer tracer{"parse type info", category(), keyValue("qmltypes path", qmltypesPath)};

    parsedTypeInfos.push_back(projectEntryInfo.sourceId);

    auto state = fileState(projectEntryInfo.sourceId, package, notUpdatedSourceIds);
    switch (state) {
    case FileState::Added:
    case FileState::Changed: {
        tracer.tick("append updated source ids", keyValue("source id", projectEntryInfo.sourceId));
        package.updatedTypeSourceIds.push_back(projectEntryInfo.sourceId);
        package.updatedImportSourceIds.push_back(projectEntryInfo.sourceId);
        package.updatedExportedTypeSourceIds.push_back(projectEntryInfo.sourceId);

        const auto content = m_fileSystem.contentAsQString(qmltypesPath);
        m_qmlTypesParser.parse(content,
                               package.imports,
                               package.types,
                               package.exportedTypes,
                               projectEntryInfo,
                               isInsideProject);

        break;
    }
    case FileState::Unchanged:
        break;
    case FileState::Removed:
    case FileState::NotExists:
    case FileState::NotExistsUnchanged:
        tracer.tick("append updated source ids", keyValue("source id", projectEntryInfo.sourceId));
        package.updatedTypeSourceIds.push_back(projectEntryInfo.sourceId);
        package.updatedImportSourceIds.push_back(projectEntryInfo.sourceId);
        package.updatedExportedTypeSourceIds.push_back(projectEntryInfo.sourceId);

        m_errorNotifier.qmltypesFileMissing(qmltypesPath);
    }

    tracer.end(keyValue("state", state));
}

void ProjectStorageUpdater::parseQmlDocument(const QString &qmlFileName,
                                             Utils::SmallStringView directoryPath,
                                             DirectoryPathId directoryId,
                                             Storage::Synchronization::SynchronizationPackage &package,
                                             NotUpdatedSourceIds &notUpdatedSourceIds,
                                             WatchedSourceIds &watchedSourceIds,
                                             FileState,
                                             IsInsideProject isInsideProject)
{
    Utils::PathString fileName{qmlFileName};
    NanotraceHR::Tracer tracer{"parse qml document", category(), keyValue("file name", fileName)};

    SourceId sourceId = m_pathCache.sourceId(directoryId, Utils::PathString{qmlFileName});

    Storage::Synchronization::Type type;
    auto state = fileState(sourceId, package, notUpdatedSourceIds);

    if (isExisting(state)) {
        tracer.tick("append watched qml source id", keyValue("source id", sourceId));
        watchedSourceIds.qmlSourceIds.push_back(sourceId);
    }
    switch (state) {
    case FileState::Unchanged: {
        tracer.tick("append not updated source id", keyValue("source id", sourceId));

        const auto &projectEntryInfo = package.projectEntryInfos.emplace_back(
            SourceId::create(directoryId),
            sourceId,
            ModuleId{},
            Storage::Synchronization::FileType::QmlDocument);
        tracer.tick("append project data", keyValue("project data", projectEntryInfo));

        break;
    }
    case FileState::NotExists:
    case FileState::Removed:
        tracer.tick("file does not exits", keyValue("source id", sourceId));
        package.updatedTypeSourceIds.push_back(sourceId);
        package.updatedImportSourceIds.push_back(sourceId);
        break;
    case FileState::NotExistsUnchanged:
        break;
    case FileState::Added:
    case FileState::Changed:
        tracer.tick("update from qml document", keyValue("source id", sourceId));
        QString qmlFilePath;
        qmlFilePath.reserve(std::ssize(directoryPath) + std::ssize(qmlFileName) + 1);
        qmlFilePath.append(directoryPath);
        qmlFilePath.append('/');
        qmlFilePath.append(qmlFileName);
        const auto content = m_fileSystem.contentAsQString(qmlFilePath);
        package.updatedImportSourceIds.push_back(sourceId);
        type = m_qmlDocumentParser.parse(content, package.imports, sourceId, directoryPath, isInsideProject);

        const auto &projectEntryInfo = package.projectEntryInfos.emplace_back(
            SourceId::create(directoryId),
            sourceId,
            ModuleId{},
            Storage::Synchronization::FileType::QmlDocument);
        tracer.tick("append directory info", keyValue("project data", projectEntryInfo));

        tracer.tick("append updated source id", keyValue("source id", sourceId));
        package.updatedTypeSourceIds.push_back(sourceId);

        type.typeName = fileName;
        type.sourceId = sourceId;

        tracer.end(keyValue("type", type));

        package.types.push_back(std::move(type));
    }
}

void ProjectStorageUpdater::parseQmlDocument(SourceId sourceId,
                                             Storage::Synchronization::SynchronizationPackage &package,
                                             NotUpdatedSourceIds &notUpdatedSourceIds,
                                             IsInsideProject isInsideProject)
{
    NanotraceHR::Tracer tracer{"parse qml component", category(), keyValue("source id", sourceId)};

    auto state = fileState(sourceId, package, notUpdatedSourceIds);
    if (isUnchanged(state))
        return;

    tracer.tick("append updated source id", keyValue("source id", sourceId));
    package.updatedImportSourceIds.push_back(sourceId);
    package.updatedTypeSourceIds.push_back(sourceId);

    if (isNotExisting(state))
        return;

    SourcePath sourcePath = m_pathCache.sourcePath(sourceId);

    const auto content = m_fileSystem.contentAsQString(QString{sourcePath});
    auto type = m_qmlDocumentParser.parse(content,
                                          package.imports,
                                          sourceId,
                                          sourcePath.directory(),
                                          isInsideProject);

    type.typeName = sourcePath.name();
    type.sourceId = sourceId;

    tracer.end(keyValue("type", type));

    package.types.push_back(std::move(type));
}

void ProjectStorageUpdater::parseQmlDocuments(const QStringList &qmlFileNames,
                                              Utils::SmallStringView directoryPath,
                                              DirectoryPathId directoryId,
                                              Storage::Synchronization::SynchronizationPackage &package,
                                              NotUpdatedSourceIds &notUpdatedSourceIds,
                                              WatchedSourceIds &watchedSourceIds,
                                              FileState directoryState,
                                              IsInsideProject isInsideProject)
{
    for (const QString &qmlFileName : qmlFileNames) {
        parseQmlDocument(qmlFileName,
                         directoryPath,
                         directoryId,
                         package,
                         notUpdatedSourceIds,
                         watchedSourceIds,
                         directoryState,
                         isInsideProject);
    }
}

ProjectStorageUpdater::FileState ProjectStorageUpdater::fileState(
    SourceId sourceId,
    Storage::Synchronization::SynchronizationPackage &package,
    NotUpdatedSourceIds &notUpdatedSourceIds) const
{
    NanotraceHR::Tracer tracer{"update property editor paths",
                               category(),
                               keyValue("source id", sourceId)};

    auto currentFileStatus = m_fileStatusCache.find(sourceId);
    auto projectStorageFileStatus = m_projectStorage.fetchFileStatus(sourceId);

    if (!currentFileStatus.isExisting()) {
        if (projectStorageFileStatus.isExisting()) {
            package.updatedFileStatusSourceIds.push_back(sourceId);
            return FileState::Removed;
        }

        if (projectStorageFileStatus) {
            notUpdatedSourceIds.fileStatusSourceIds.push_back(sourceId);
            return FileState::NotExistsUnchanged;
        }

        package.fileStatuses.push_back(currentFileStatus);
        package.updatedFileStatusSourceIds.push_back(sourceId);

        return FileState::NotExists;
    }

    if (!projectStorageFileStatus) {
        package.fileStatuses.push_back(currentFileStatus);
        package.updatedFileStatusSourceIds.push_back(sourceId);

        return FileState::Added;
    }

    if (projectStorageFileStatus != currentFileStatus) {
        package.fileStatuses.push_back(currentFileStatus);
        package.updatedFileStatusSourceIds.push_back(sourceId);

        return FileState::Changed;
    }

    notUpdatedSourceIds.fileStatusSourceIds.push_back(sourceId);

    return FileState::Unchanged;
}

ProjectStorageUpdater::FileState ProjectStorageUpdater::fileState(
    DirectoryPathId directoryPathId,
    Storage::Synchronization::SynchronizationPackage &package,
    NotUpdatedSourceIds &notUpdatedSourceIds) const
{
    auto sourceId = SourceId::create(directoryPathId);
    return fileState(sourceId, package, notUpdatedSourceIds);
}

} // namespace QmlDesigner
