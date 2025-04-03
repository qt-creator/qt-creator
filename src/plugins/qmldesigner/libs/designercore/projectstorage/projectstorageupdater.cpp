// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectstorageupdater.h"

#include "filestatuscache.h"
#include "filesysteminterface.h"
#include "projectstorage.h"
#include "projectstoragepathwatcherinterface.h"
#include "qmldocumentparserinterface.h"
#include "qmltypesparserinterface.h"
#include "sourcepathstorage/sourcepath.h"
#include "sourcepathstorage/sourcepathcache.h"
#include "typeannotationreader.h"

#include <functional.h>
#include <sqlitedatabase.h>
#include <tracing/qmldesignertracing.h>
#include <utils/algorithm.h>
#include <utils/set_algorithm.h>

#include <QDirIterator>
#include <QRegularExpression>

#include <private/qqmldirparser_p.h>

#include <algorithm>
#include <functional>

namespace QmlDesigner {
constexpr auto category = ProjectStorageTracing::projectStorageUpdaterCategory;
using NanotraceHR::keyValue;
using Tracer = ProjectStorageTracing::Category::TracerType;

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

QStringList filterMultipleEntries(QStringList qmlTypes)
{
    std::sort(qmlTypes.begin(), qmlTypes.end());
    qmlTypes.erase(std::unique(qmlTypes.begin(), qmlTypes.end()), qmlTypes.end());

    return qmlTypes;
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

ProjectStorageUpdater::Components createComponents(
    const QMultiHash<QString, QQmlDirParser::Component> &qmlDirParserComponents,
    ModuleId moduleId,
    ModuleId pathModuleId,
    FileSystemInterface &fileSystem,
    const QString &directory)
{
    ProjectStorageUpdater::Components components;

    auto qmlFileNames = fileSystem.fileNames(directory, {"*.qml"});

    components.reserve(static_cast<std::size_t>(qmlDirParserComponents.size() + qmlFileNames.size()));

    for (const QString &qmlFileName : qmlFileNames) {
        Utils::PathString fileName{qmlFileName};
        Utils::PathString typeName{fileName.begin(), std::find(fileName.begin(), fileName.end(), '.')};
        components.push_back(
            ProjectStorageUpdater::Component{fileName, typeName, pathModuleId, -1, -1});
    }

    for (const QQmlDirParser::Component &qmlDirParserComponent : qmlDirParserComponents) {
        if (qmlDirParserComponent.fileName.contains('/'))
            continue;
        components.push_back({qmlDirParserComponent.fileName,
                              qmlDirParserComponent.typeName,
                              moduleId,
                              qmlDirParserComponent.version.majorVersion(),
                              qmlDirParserComponent.version.minorVersion()});
    }

    return components;
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
                  const Storage::Synchronization::DirectoryInfos &directoryInfos,
                  TracerLiteral message,
                  Tracer &tracer)
{
    for (const auto &directoryInfo : directoryInfos) {
        tracer.tick(message, keyValue("source id", directoryInfo.sourceId));
        sourceIds.push_back(directoryInfo.sourceId);
    }
}

Storage::Version convertVersion(QTypeRevision version)
{
    return Storage::Version{version.hasMajorVersion() ? version.majorVersion() : -1,
                            version.hasMinorVersion() ? version.minorVersion() : -1};
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

void addDependencies(Storage::Imports &dependencies,
                     SourceId sourceId,
                     const std::vector<Utils::PathString> &qmldirDependencies,
                     ProjectStorageInterface &projectStorage,
                     TracerLiteral message,
                     Tracer &tracer)
{
    for (std::string_view qmldirDependency : qmldirDependencies) {
        ModuleId moduleId = projectStorage.moduleId(qmldirDependency, Storage::ModuleKind::CppLibrary);
        auto &import = dependencies.emplace_back(moduleId, Storage::Version{}, sourceId);
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
                              ProjectStorageInterface &projectStorage)
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
        ModuleId exportedModuleId = projectStorage.moduleId(exportedModuleName,
                                                            ModuleKind::QmlLibrary);
        addModuleExportedImport(imports,
                                moduleId,
                                exportedModuleId,
                                convertVersion(qmldirImport.version),
                                convertToIsAutoVersion(qmldirImport.flags, qmldirImport.version),
                                moduleName,
                                ModuleKind::QmlLibrary,
                                exportedModuleName);

        ModuleId exportedCppModuleId = projectStorage.moduleId(exportedModuleName,
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

    updateDirectories(qtDirectories, package, notUpdatedSourceIds, watchedQtSourceIds);
    if (projectDirectory.size()) {
        updateDirectory(projectDirectory,
                        {},
                        package,
                        notUpdatedSourceIds,
                        watchedProjectSourceIds,
                        IsInsideProject::Yes);
    }
    updatePropertyEditorPaths(propertyEditorResourcesPath, package, notUpdatedSourceIds);
    updateTypeAnnotations(typeAnnotationPaths, package, notUpdatedSourceIds);

    package.updatedSourceIds = filterNotUpdatedSourceIds(std::move(package.updatedSourceIds),
                                                         std::move(notUpdatedSourceIds.sourceIds));
    package.updatedFileStatusSourceIds = filterNotUpdatedSourceIds(
        std::move(package.updatedFileStatusSourceIds),
        std::move(notUpdatedSourceIds.fileStatusSourceIds));

    m_projectStorage.synchronize(std::move(package));

    std::vector<IdPaths> idPaths;
    idPaths.reserve(8);
    appendIdPaths(std::move(watchedQtSourceIds), m_qtPartId, idPaths);
    appendIdPaths(std::move(watchedProjectSourceIds), m_projectPartId, idPaths);
    m_pathWatcher.updateIdPaths(idPaths);
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

template<typename... FileStates>
ProjectStorageUpdater::FileState combineState(FileStates... fileStates)
{
    if (((fileStates == ProjectStorageUpdater::FileState::Removed) && ...))
        return ProjectStorageUpdater::FileState::Removed;

    if (((fileStates == ProjectStorageUpdater::FileState::Added) || ...))
        return ProjectStorageUpdater::FileState::Added;

    if ((isChanged(fileStates) || ...))
        return ProjectStorageUpdater::FileState::Changed;

    if (((fileStates == ProjectStorageUpdater::FileState::Unchanged) || ...))
        return ProjectStorageUpdater::FileState::Unchanged;

    return ProjectStorageUpdater::FileState::NotExists;
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

} // namespace

void ProjectStorageUpdater::updateDirectoryChanged(Utils::SmallStringView directoryPath,
                                                   Utils::SmallStringView annotationDirectoryPath,
                                                   FileState qmldirState,
                                                   FileState annotationDirectoryState,
                                                   SourcePath qmldirSourcePath,
                                                   SourceId qmldirSourceId,
                                                   SourceContextId directoryId,
                                                   SourceContextId annotationDirectoryId,
                                                   Storage::Synchronization::SynchronizationPackage &package,
                                                   NotUpdatedSourceIds &notUpdatedSourceIds,
                                                   WatchedSourceIds &WatchedSourceIds,
                                                   IsInsideProject isInsideProject,
                                                   Tracer &tracer)
{
    QQmlDirParser parser;
    if (isExisting(qmldirState))
        parser.parse(m_fileSystem.contentAsQString(QString{qmldirSourcePath}));

    if (isChanged(qmldirState)) {
        tracer.tick("append updated source id", keyValue("module id", qmldirSourceId));
        package.updatedSourceIds.push_back(qmldirSourceId);
    }

    using Storage::ModuleKind;
    Utils::PathString moduleName{parser.typeNamespace()};
    if (moduleName.empty())
        moduleName = directoryName(directoryPath);
    ModuleId moduleId = m_projectStorage.moduleId(moduleName, ModuleKind::QmlLibrary);
    ModuleId cppModuleId = m_projectStorage.moduleId(moduleName, ModuleKind::CppLibrary);
    ModuleId pathModuleId = m_projectStorage.moduleId(directoryPath, ModuleKind::PathLibrary);

    auto imports = filterMultipleEntries(parser.imports());

    addModuleExportedImports(
        package.moduleExportedImports, moduleId, cppModuleId, moduleName, imports, m_projectStorage);
    tracer.tick("append updated module id", keyValue("module id", moduleId));
    package.updatedModuleIds.push_back(moduleId);

    const auto qmlDirectoryInfos = m_projectStorage.fetchDirectoryInfos(directoryId);
    addSourceIds(package.updatedSourceIds, qmlDirectoryInfos, "append updated source id", tracer);
    addSourceIds(package.updatedFileStatusSourceIds,
                 qmlDirectoryInfos,
                 "append updated file status source id",
                 tracer);

    auto qmlTypes = filterMultipleEntries(parser.typeInfos());

    QString directoryPathAsQString{directoryPath};

    if (!qmlTypes.isEmpty()) {
        parseTypeInfos(qmlTypes,
                       toImportNames(filterMultipleEntries(parser.dependencies())),
                       toImportNames(imports),
                       directoryId,
                       directoryPathAsQString,
                       cppModuleId,
                       package,
                       notUpdatedSourceIds,
                       WatchedSourceIds,
                       isInsideProject);
    }
    parseQmlComponents(createComponents(parser.components(),
                                        moduleId,
                                        pathModuleId,
                                        m_fileSystem,
                                        directoryPathAsQString),
                       directoryId,
                       package,
                       notUpdatedSourceIds,
                       WatchedSourceIds,
                       qmldirState,
                       qmldirSourceId,
                       isInsideProject);
    tracer.tick("append updated project source id", keyValue("module id", moduleId));
    package.updatedDirectoryInfoDirectoryIds.push_back(directoryId);

    if (isInsideProject == IsInsideProject::Yes) {
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

void ProjectStorageUpdater::updateDirectories(const QStringList &directories,
                                              Storage::Synchronization::SynchronizationPackage &package,
                                              NotUpdatedSourceIds &notUpdatedSourceIds,
                                              WatchedSourceIds &WatchedSourceIds)
{
    NanotraceHR::Tracer tracer{"update directories", category()};

    for (const QString &directory : directories)
        updateDirectory(
            {directory}, {}, package, notUpdatedSourceIds, WatchedSourceIds, IsInsideProject::No);
}

void ProjectStorageUpdater::updateSubdirectories(const Utils::PathString &directoryPath,
                                                 SourceContextId directoryId,
                                                 FileState directoryState,
                                                 const SourceContextIds &subdirectoriesToIgnore,
                                                 Storage::Synchronization::SynchronizationPackage &package,
                                                 NotUpdatedSourceIds &notUpdatedSourceIds,
                                                 WatchedSourceIds &WatchedSourceIds,
                                                 IsInsideProject isInsideProject)
{
    struct Directory
    {
        Directory(Utils::SmallStringView path, SourceContextId sourceContextId)
            : path{path}
            , sourceContextId{sourceContextId}
        {}

        Utils::PathString path;
        SourceContextId sourceContextId;
    };

    using Directories = QVarLengthArray<Directory, 32>;

    auto subdirectoryIds = m_projectStorage.fetchSubdirectoryIds(directoryId);
    auto subdirectories = Utils::transform<Directories>(
        subdirectoryIds, [&](SourceContextId sourceContextId) -> Directory {
            auto subdirectoryPath = m_pathCache.sourceContextPath(sourceContextId);
            return {subdirectoryPath, sourceContextId};
        });

    auto exisitingSubdirectoryPaths = m_fileSystem.subdirectories(directoryPath.toQString());
    Directories existingSubdirecories;

    auto skipDirectory = [&](std::string_view subdirectoryPath, SourceContextId sourceContextId) {
        if (subdirectoryPath.ends_with("designer"))
            return true;

        if (isInsideProject == IsInsideProject::Yes) {
            static SourceNameId ignoreInQdsSourceNameId = m_pathCache.sourceNameId("ignore-in-qds");

            SourceId ignoreInQdsSourceId = SourceId::create(ignoreInQdsSourceNameId, sourceContextId);

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
        SourceContextId sourceContextId = m_pathCache.sourceContextId(subdirectoryPath);

        if (skipDirectory(subdirectoryPath, sourceContextId))
            continue;

        subdirectories.emplace_back(subdirectoryPath, sourceContextId);
        existingSubdirecories.emplace_back(subdirectoryPath, sourceContextId);
    }

    std::ranges::sort(subdirectories, {}, &Directory::sourceContextId);
    auto removed = std::ranges::unique(subdirectories, {}, &Directory::sourceContextId);
    subdirectories.erase(removed.begin(), removed.end());

    auto updateDirectory = [&](const Directory &subdirectory) {
        this->updateDirectory(subdirectory.path,
                              subdirectoriesToIgnore,
                              package,
                              notUpdatedSourceIds,
                              WatchedSourceIds,
                              isInsideProject);
    };

    Utils::set_greedy_difference(subdirectories,
                                 subdirectoriesToIgnore,
                                 updateDirectory,
                                 {},
                                 &Directory::sourceContextId);

    if (isChanged(directoryState)) {
        for (const auto &[subdirectoryPath, subdirectoryId] : existingSubdirecories) {
            package.directoryInfos.emplace_back(directoryId,
                                                SourceId::create(SourceNameId{}, subdirectoryId),
                                                ModuleId{},
                                                Storage::Synchronization::FileType::Directory);
        }
    }
}

void ProjectStorageUpdater::annotationDirectoryChanged(
    Utils::SmallStringView directoryPath,
    SourceContextId directoryId,
    SourceContextId annotationDirectoryId,
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
    SourceContextId directoryId,
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
    SourceContextId directoryId,
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

void ProjectStorageUpdater::updateDirectory(const Utils::PathString &directoryPath,
                                            const SourceContextIds &subdirectoriesToIgnore,
                                            Storage::Synchronization::SynchronizationPackage &package,
                                            NotUpdatedSourceIds &notUpdatedSourceIds,
                                            WatchedSourceIds &WatchedSourceIds,
                                            IsInsideProject isInsideProject)
{
    NanotraceHR::Tracer tracer{"update directory", category(), keyValue("directory", directoryPath)};

    SourcePath qmldirPath{directoryPath + "/qmldir"};
    SourceId qmldirSourceId = m_pathCache.sourceId(qmldirPath);
    SourceContextId directoryId = qmldirSourceId.contextId();

    auto directoryState = fileState(directoryId, package, notUpdatedSourceIds);
    if (isExisting(directoryState))
        WatchedSourceIds.directoryIds.push_back(SourceId::create(SourceNameId{}, directoryId));

    auto qmldirState = fileState(qmldirSourceId, package, notUpdatedSourceIds);
    if (isExisting(qmldirState))
        WatchedSourceIds.qmldirSourceIds.push_back(qmldirSourceId);

    SourcePath annotationDirectoryPath{directoryPath + "/designer"};
    auto annotationDirectoryId = m_pathCache.sourceContextId(annotationDirectoryPath);
    auto annotationDirectoryState = fileState(annotationDirectoryId, package, notUpdatedSourceIds);

    switch (combineState(directoryState, qmldirState, annotationDirectoryState)) {
    case FileState::Added:
    case FileState::Changed:
        tracer.tick("update directory changed");
        updateDirectoryChanged(directoryPath,
                               annotationDirectoryPath,
                               qmldirState,
                               annotationDirectoryState,
                               qmldirPath,
                               qmldirSourceId,
                               directoryId,
                               annotationDirectoryId,
                               package,
                               notUpdatedSourceIds,
                               WatchedSourceIds,
                               isInsideProject,
                               tracer);
        break;
    case FileState::Unchanged:
        tracer.tick("update directory not changed");

        parseDirectoryInfos(m_projectStorage.fetchDirectoryInfos(directoryId),
                            package,
                            notUpdatedSourceIds,
                            WatchedSourceIds,
                            isInsideProject);
        break;
    case FileState::Removed: {
        tracer.tick("update directory don't exits");

        package.updatedDirectoryInfoDirectoryIds.push_back(directoryId);
        package.updatedSourceIds.push_back(qmldirSourceId);
        auto qmlDirectoryInfos = m_projectStorage.fetchDirectoryInfos(directoryId);
        for (const Storage::Synchronization::DirectoryInfo &directoryInfo : qmlDirectoryInfos) {
            tracer.tick("append updated source id", keyValue("source id", directoryInfo.sourceId));
            package.updatedSourceIds.push_back(directoryInfo.sourceId);
            tracer.tick("append updated file status source id",
                        keyValue("source id", directoryInfo.sourceId));
            package.updatedFileStatusSourceIds.push_back(directoryInfo.sourceId);
        }
        break;
    }
    case FileState::NotExists:
    case FileState::NotExistsUnchanged:
        break;
    }

    updateSubdirectories(directoryPath,
                         directoryId,
                         directoryState,
                         subdirectoriesToIgnore,
                         package,
                         notUpdatedSourceIds,
                         WatchedSourceIds,
                         isInsideProject);

    tracer.end(keyValue("qmldir source path", qmldirPath),
               keyValue("directory id", directoryId),
               keyValue("qmldir source id", qmldirSourceId),
               keyValue("qmldir state", qmldirState),
               keyValue("directory state", directoryState));
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

        SourceContextId directoryId = m_pathCache.sourceContextId(
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

template<typename SourceIds1, typename SourceIds2>
SmallSourceIds<16> mergedSourceIds(const SourceIds1 &sourceIds1, const SourceIds2 &sourceIds2)
{
    SmallSourceIds<16> mergedSourceIds;

    std::ranges::set_union(sourceIds1, sourceIds2, std::back_inserter(mergedSourceIds));

    return mergedSourceIds;
}
} // namespace

void ProjectStorageUpdater::updateTypeAnnotations(const QStringList &directoryPaths,
                                                  Storage::Synchronization::SynchronizationPackage &package,
                                                  NotUpdatedSourceIds &notUpdatedSourceIds)
{
    NanotraceHR::Tracer tracer("update type annotations", category());

    std::map<SourceContextId, SmallSourceIds<16>> updatedSourceIdsDictonary;

    for (SourceContextId directoryId : m_projectStorage.typeAnnotationDirectoryIds())
        updatedSourceIdsDictonary[directoryId] = {};

    for (const auto &directoryPath : directoryPaths)
        updateTypeAnnotations(directoryPath, package, notUpdatedSourceIds, updatedSourceIdsDictonary);

    updateTypeAnnotationDirectories(package, notUpdatedSourceIds, updatedSourceIdsDictonary);
}

void ProjectStorageUpdater::updateTypeAnnotations(
    const QString &rootDirectoryPath,
    Storage::Synchronization::SynchronizationPackage &package,
    NotUpdatedSourceIds &notUpdatedSourceIds,
    std::map<SourceContextId, SmallSourceIds<16>> &updatedSourceIdsDictonary)
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

        SourceContextId directoryId = m_pathCache.sourceContextId(Utils::PathString{directoryPath});

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
    std::map<SourceContextId, SmallSourceIds<16>> &updatedSourceIdsDictonary)
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
                                                 SourceContextId directoryId,
                                                 Storage::Synchronization::SynchronizationPackage &package)
{
    NanotraceHR::Tracer tracer{"update type annotation path",
                               category(),
                               keyValue("path", filePath),
                               keyValue("directory path", directoryPath)};

    Storage::TypeAnnotationReader reader{m_projectStorage};

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
    SourceContextId directoryId,
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
    SourceContextId directoryId,
    long long pathOffset)
{
    NanotraceHR::Tracer tracer{"update property editor file path",
                               category(),
                               keyValue("directory path", path),
                               keyValue("directory id", directoryId)};

    QRegularExpression regex{R"xo((.+)\/(\w+)(Specifics|Pane).qml)xo"};
    auto match = regex.match(QStringView{path}.mid(pathOffset));
    QString oldModuleName;
    ModuleId moduleId;
    if (match.hasMatch()) {
        auto moduleName = match.capturedView(1).toString();
        moduleName.replace('/', '.');
        if (oldModuleName != moduleName) {
            oldModuleName = moduleName;
            moduleId = m_projectStorage.moduleId(Utils::SmallString{moduleName},
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
SourceContextIds filterUniqueSourceContextIds(const SourceIds &sourceIds)
{
    auto sourceContextIds = Utils::transform(sourceIds, &SourceId::contextId);

    std::sort(sourceContextIds.begin(), sourceContextIds.end());
    auto newEnd = std::unique(sourceContextIds.begin(), sourceContextIds.end());
    sourceContextIds.erase(newEnd, sourceContextIds.end());

    return sourceContextIds;
}

SourceIds filterUniqueSourceIds(SourceIds sourceIds)
{
    std::sort(sourceIds.begin(), sourceIds.end());
    auto newEnd = std::unique(sourceIds.begin(), sourceIds.end());
    sourceIds.erase(newEnd, sourceIds.end());

    return sourceIds;
}

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
        qmlDocument.reserve(128);
        qmltypes.reserve(32);
    }

    SourceIds directory;
    SourceIds qmlDocument;
    SourceIds qmltypes;
};

void appendProjectChunkSourceIds(ProjectChunkSourceIds &ids,
                                 const ProjectChunkId &projectChunkId,
                                 const SourceIds &sourceIds)
{
    switch (projectChunkId.sourceType) {
    case SourceType::Directory:
    case SourceType::QmlDir:
        ids.directory.insert(ids.directory.end(), sourceIds.begin(), sourceIds.end());
        break;
    case SourceType::Qml:
    case SourceType::QmlUi:
        ids.qmlDocument.insert(ids.qmlDocument.end(), sourceIds.begin(), sourceIds.end());
        break;
    case SourceType::QmlTypes:
        ids.qmltypes.insert(ids.qmltypes.end(), sourceIds.begin(), sourceIds.end());
        break;
    }
}

} // namespace

void ProjectStorageUpdater::pathsWithIdsChanged(const std::vector<IdPaths> &changedIdPaths)
{
    NanotraceHR::Tracer tracer{"paths with ids changed",
                               category(),
                               keyValue("id paths", changedIdPaths)};

    m_changedIdPaths.insert(m_changedIdPaths.end(), changedIdPaths.begin(), changedIdPaths.end());

    Storage::Synchronization::SynchronizationPackage package;

    WatchedSourceIds watchedQtSourceIds{10};
    WatchedSourceIds watchedProjectSourceIds{10};
    NotUpdatedSourceIds notUpdatedSourceIds{10};
    std::vector<IdPaths> idPaths;
    idPaths.reserve(4);

    ProjectChunkSourceIds project;
    ProjectChunkSourceIds qt;

    for (const auto &[projectChunkId, sourceIds] : m_changedIdPaths) {
        if (projectChunkId.id == m_projectPartId)
            appendProjectChunkSourceIds(project, projectChunkId, sourceIds);
        else if (projectChunkId.id == m_qtPartId)
            appendProjectChunkSourceIds(qt, projectChunkId, sourceIds);
    }

    auto updateDirectory = [&](const SourceIds &directorySourceIds,
                               WatchedSourceIds &watchedSourceIds) {
        auto directoryIds = filterUniqueSourceContextIds(directorySourceIds);
        for (auto directoryId : directoryIds) {
            Utils::PathString directory = m_pathCache.sourceContextPath(directoryId);
            this->updateDirectory(directory,
                                  directoryIds,
                                  package,
                                  notUpdatedSourceIds,
                                  watchedSourceIds,
                                  IsInsideProject::No);
        }

        return directoryIds;
    };

    auto qtDirectoryIds = updateDirectory(qt.directory, watchedQtSourceIds);
    auto projectDirectoryIds = updateDirectory(project.directory, watchedProjectSourceIds);

    auto parseQmlComponent = [&](SourceIds qmlDocumentSourceIds,
                                 const SourceContextIds &directoryIds,
                                 IsInsideProject isInsideProject) {
        for (SourceId sourceId : filterUniqueSourceIds(std::move(qmlDocumentSourceIds))) {
            if (!contains(directoryIds, sourceId.contextId()))
                this->parseQmlComponent(sourceId, package, notUpdatedSourceIds, isInsideProject);
        }
    };

    parseQmlComponent(std::move(qt.qmlDocument), qtDirectoryIds, IsInsideProject::No);
    parseQmlComponent(std::move(project.qmlDocument), projectDirectoryIds, IsInsideProject::Yes);

    auto parseTypeInfo = [&](SourceIds qmltypesSourceIds,
                             const SourceContextIds &directoryIds,
                             IsInsideProject isInsideProject) {
        for (SourceId sourceId : filterUniqueSourceIds(std::move(qmltypesSourceIds))) {
            if (!contains(directoryIds, sourceId.contextId())) {
                QString qmltypesPath{m_pathCache.sourcePath(sourceId)};
                auto directoryInfo = m_projectStorage.fetchDirectoryInfo(sourceId);
                if (directoryInfo)
                    this->parseTypeInfo(*directoryInfo,
                                        qmltypesPath,
                                        package,
                                        notUpdatedSourceIds,
                                        isInsideProject);
            }
        }
    };

    try {
        parseTypeInfo(std::move(qt.qmltypes), qtDirectoryIds, IsInsideProject::No);
        parseTypeInfo(std::move(project.qmltypes), projectDirectoryIds, IsInsideProject::Yes);
    } catch (const QmlDesigner::CannotParseQmlTypesFile &) {
        return;
    }

    package.updatedSourceIds = filterNotUpdatedSourceIds(std::move(package.updatedSourceIds),
                                                         std::move(notUpdatedSourceIds.sourceIds));
    package.updatedFileStatusSourceIds = filterNotUpdatedSourceIds(
        std::move(package.updatedFileStatusSourceIds),
        std::move(notUpdatedSourceIds.fileStatusSourceIds));

    try {
        m_projectStorage.synchronize(std::move(package));
    } catch (const ProjectStorageError &) {
        return;
    }

    auto directoryIdsSize = projectDirectoryIds.size() + qtDirectoryIds.size();
    if (directoryIdsSize > 0) {
        SourceContextIds directoryIds;
        std::vector<IdPaths> newIdPaths;
        idPaths.reserve(8);
        appendIdPaths(std::move(watchedQtSourceIds), m_qtPartId, newIdPaths);
        appendIdPaths(std::move(watchedProjectSourceIds), m_projectPartId, newIdPaths);

        directoryIds.reserve(directoryIdsSize);
        std::ranges::merge(projectDirectoryIds, qtDirectoryIds, std::back_inserter(directoryIds));
        m_pathWatcher.updateContextIdPaths(newIdPaths, directoryIds);
    }

    m_changedIdPaths.clear();
}

void ProjectStorageUpdater::pathsChanged(const SourceIds &) {}

void ProjectStorageUpdater::parseTypeInfos(const QStringList &typeInfos,
                                           const std::vector<Utils::PathString> &qmldirDependencies,
                                           const std::vector<Utils::PathString> &qmldirImports,
                                           SourceContextId directoryId,
                                           const QString &directoryPath,
                                           ModuleId moduleId,
                                           Storage::Synchronization::SynchronizationPackage &package,
                                           NotUpdatedSourceIds &notUpdatedSourceIds,
                                           WatchedSourceIds &watchedSourceIds,
                                           IsInsideProject isInsideProject)
{
    NanotraceHR::Tracer tracer{"parse type infos",
                               category(),
                               keyValue("directory id", directoryId),
                               keyValue("directory path", directoryPath),
                               keyValue("module id", moduleId)};

    for (const QString &typeInfo : typeInfos) {
        NanotraceHR::Tracer tracer{"parse type info", category(), keyValue("type info", typeInfo)};

        Utils::PathString qmltypesFileName = typeInfo;
        SourceId sourceId = m_pathCache.sourceId(directoryId, qmltypesFileName);

        tracer.tick("append qmltypes source id", keyValue("source id", sourceId));
        watchedSourceIds.qmltypesSourceIds.push_back(sourceId);

        addDependencies(package.moduleDependencies,
                        sourceId,
                        joinImports(qmldirDependencies, qmldirImports),
                        m_projectStorage,
                        "append module dependency",
                        tracer);

        tracer.tick("append module dependenct source source id", keyValue("source id", sourceId));
        package.updatedModuleDependencySourceIds.push_back(sourceId);

        const auto &directoryInfo = package.directoryInfos.emplace_back(
            directoryId, sourceId, moduleId, Storage::Synchronization::FileType::QmlTypes);
        tracer.tick("append project data", keyValue("source id", sourceId));

        const QString qmltypesPath = directoryPath + "/" + typeInfo;

        parseTypeInfo(directoryInfo, qmltypesPath, package, notUpdatedSourceIds, isInsideProject);
    }
}

void ProjectStorageUpdater::parseDirectoryInfos(
    const Storage::Synchronization::DirectoryInfos &directoryInfos,
    Storage::Synchronization::SynchronizationPackage &package,
    NotUpdatedSourceIds &notUpdatedSourceIds,
    WatchedSourceIds &watchedSourceIds,
    IsInsideProject isInsideProject)
{
    NanotraceHR::Tracer tracer{"parse project datas", category()};

    for (const Storage::Synchronization::DirectoryInfo &directoryInfo : directoryInfos) {
        switch (directoryInfo.fileType) {
        case Storage::Synchronization::FileType::QmlTypes: {
            watchedSourceIds.qmltypesSourceIds.push_back(directoryInfo.sourceId);

            QString qmltypesPath{m_pathCache.sourcePath(directoryInfo.sourceId)};
            parseTypeInfo(directoryInfo, qmltypesPath, package, notUpdatedSourceIds, isInsideProject);
            break;
        }
        case Storage::Synchronization::FileType::QmlDocument: {
            watchedSourceIds.qmlSourceIds.push_back(directoryInfo.sourceId);

            parseQmlComponent(directoryInfo.sourceId, package, notUpdatedSourceIds, isInsideProject);
            break;
        }
        case Storage::Synchronization::FileType::Directory:
            break;
        }
    }
}

auto ProjectStorageUpdater::parseTypeInfo(const Storage::Synchronization::DirectoryInfo &directoryInfo,
                                          const QString &qmltypesPath,
                                          Storage::Synchronization::SynchronizationPackage &package,
                                          NotUpdatedSourceIds &notUpdatedSourceIds,
                                          IsInsideProject isInsideProject) -> FileState
{
    NanotraceHR::Tracer tracer{"parse type info", category(), keyValue("qmltypes path", qmltypesPath)};

    auto state = fileState(directoryInfo.sourceId, package, notUpdatedSourceIds);
    switch (state) {
    case FileState::Added:
    case FileState::Changed: {
        tracer.tick("append updated source ids", keyValue("source id", directoryInfo.sourceId));
        package.updatedSourceIds.push_back(directoryInfo.sourceId);

        const auto content = m_fileSystem.contentAsQString(qmltypesPath);
        m_qmlTypesParser.parse(content, package.imports, package.types, directoryInfo, isInsideProject);
        break;
    }
    case FileState::Unchanged: {
        tracer.tick("append not updated source ids", keyValue("source id", directoryInfo.sourceId));
        notUpdatedSourceIds.sourceIds.push_back(directoryInfo.sourceId);
        break;
    }
    case FileState::Removed:
    case FileState::NotExists:
    case FileState::NotExistsUnchanged:
        throw CannotParseQmlTypesFile{};
    }

    tracer.end(keyValue("state", state));

    return state;
}

void ProjectStorageUpdater::parseQmlComponent(Utils::SmallStringView relativeFilePath,
                                              Utils::SmallStringView directoryPath,
                                              Storage::Synchronization::ExportedTypes exportedTypes,
                                              SourceContextId directoryId,
                                              Storage::Synchronization::SynchronizationPackage &package,
                                              NotUpdatedSourceIds &notUpdatedSourceIds,
                                              WatchedSourceIds &watchedSourceIds,
                                              FileState qmldirState,
                                              SourceId qmldirSourceId,
                                              IsInsideProject isInsideProject)
{
    NanotraceHR::Tracer tracer{"parse qml component",
                               category(),
                               keyValue("relative file path", relativeFilePath),
                               keyValue("directory path", directoryPath),
                               keyValue("exported types", exportedTypes),
                               keyValue("directory  id", directoryId),
                               keyValue("qmldir state", qmldirState)};

    if (std::find(relativeFilePath.begin(), relativeFilePath.end(), '+') != relativeFilePath.end())
        return;

    Utils::PathString qmlFilePath = Utils::PathString::join({directoryPath, "/", relativeFilePath});
    SourceId sourceId = m_pathCache.sourceId(SourcePathView{qmlFilePath});

    Storage::Synchronization::Type type;
    auto state = fileState(sourceId, package, notUpdatedSourceIds);

    tracer.tick("append watched qml source id", keyValue("source id", sourceId));
    watchedSourceIds.qmlSourceIds.push_back(sourceId);

    switch (state) {
    case FileState::Unchanged:
        if (isNotExisting(qmldirState)) {
            tracer.tick("append not updated source id", keyValue("source id", sourceId));
            notUpdatedSourceIds.sourceIds.emplace_back(sourceId);

            const auto &directoryInfo = package.directoryInfos.emplace_back(
                directoryId, sourceId, ModuleId{}, Storage::Synchronization::FileType::QmlDocument);
            tracer.tick("append project data", keyValue("project data", directoryInfo));

            return;
        }
        type.changeLevel = Storage::Synchronization::ChangeLevel::Minimal;
        break;
    case FileState::NotExists:
    case FileState::Removed:
        tracer.tick("file does not exits", keyValue("source id", sourceId));
        for (const auto &exportedType : exportedTypes)
            m_errorNotifier.qmlDocumentDoesNotExistsForQmldirEntry(exportedType.name,
                                                                   exportedType.version,
                                                                   sourceId,
                                                                   qmldirSourceId);
        break;
    case FileState::NotExistsUnchanged:
        break;
    case FileState::Added:
    case FileState::Changed:
        tracer.tick("update from qml document", keyValue("source id", sourceId));
        const auto content = m_fileSystem.contentAsQString(QString{qmlFilePath});
        type = m_qmlDocumentParser.parse(content, package.imports, sourceId, directoryPath, isInsideProject);
        break;
    }

    const auto &directoryInfo = package.directoryInfos.emplace_back(
        directoryId, sourceId, ModuleId{}, Storage::Synchronization::FileType::QmlDocument);
    tracer.tick("append directory info", keyValue("project data", directoryInfo));

    tracer.tick("append updated source id", keyValue("source id", sourceId));
    package.updatedSourceIds.push_back(sourceId);

    type.typeName = SourcePath{qmlFilePath}.name();
    type.sourceId = sourceId;
    type.exportedTypes = std::move(exportedTypes);

    tracer.end(keyValue("type", type));

    package.types.push_back(std::move(type));
}

void ProjectStorageUpdater::parseQmlComponent(SourceId sourceId,
                                              Storage::Synchronization::SynchronizationPackage &package,
                                              NotUpdatedSourceIds &notUpdatedSourceIds,
                                              IsInsideProject isInsideProject)
{
    NanotraceHR::Tracer tracer{"parse qml component", category(), keyValue("source id", sourceId)};

    auto state = fileState(sourceId, package, notUpdatedSourceIds);
    if (isUnchanged(state))
        return;

    tracer.tick("append updated source id", keyValue("source id", sourceId));
    package.updatedSourceIds.push_back(sourceId);

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
    type.traits = Storage::TypeTraitsKind::Reference;
    type.sourceId = sourceId;
    type.changeLevel = Storage::Synchronization::ChangeLevel::ExcludeExportedTypes;

    tracer.end(keyValue("type", type));

    package.types.push_back(std::move(type));
}

namespace {

template<typename Callback>
void rangeForTheSameFileName(const ProjectStorageUpdater::Components &components, Callback callback)
{
    auto current = components.begin();
    const auto end = components.end();

    while (current != end) {
        auto nextType = std::find_if(current, end, [&](const auto &component) {
            return component.fileName != current->fileName;
        });

        callback(ProjectStorageUpdater::ComponentRange{current, nextType});

        current = nextType;
    }
}

namespace {

void removeDuplicates(Storage::Synchronization::ExportedTypes &exportedTypes)
{
    using Storage::Synchronization::ExportedType;

    auto compare = makeCompare(&ExportedType::name, &ExportedType::version);
    auto less = compare(std::ranges::less{});
    auto equal = compare(std::ranges::equal_to{});

    std::ranges::sort(exportedTypes, less);

    auto duplicateExportedTypes = std::ranges::unique(exportedTypes, equal);
    exportedTypes.erase(duplicateExportedTypes.begin(), duplicateExportedTypes.end());
}
} // namespace

Storage::Synchronization::ExportedTypes createExportedTypes(ProjectStorageUpdater::ComponentRange components)
{
    Storage::Synchronization::ExportedTypes exportedTypes;
    exportedTypes.reserve(components.size() + 1);

    for (const ProjectStorageUpdater::Component &component : components) {
        exportedTypes.emplace_back(component.moduleId,
                                   Utils::SmallString{component.typeName},
                                   Storage::Version{component.majorVersion, component.minorVersion});
    }

    removeDuplicates(exportedTypes);

    return exportedTypes;
}

} // namespace

void ProjectStorageUpdater::parseQmlComponents(Components components,
                                               SourceContextId directoryId,
                                               Storage::Synchronization::SynchronizationPackage &package,
                                               NotUpdatedSourceIds &notUpdatedSourceIds,
                                               WatchedSourceIds &WatchedSourceIds,
                                               FileState qmldirState,
                                               SourceId qmldirSourceId,
                                               IsInsideProject isInsideProject)
{
    NanotraceHR::Tracer tracer{"parse qml components",
                               category(),
                               keyValue("directory id", directoryId),
                               keyValue("qmldir state", qmldirState)};

    std::ranges::sort(components,
                      [](auto &&first, auto &&second) { return first.fileName < second.fileName; });

    auto directoryPath = m_pathCache.sourceContextPath(directoryId);

    auto callback = [&](ComponentRange componentsWithSameFileName) {
        const auto &firstComponent = *componentsWithSameFileName.begin();
        const Utils::SmallString fileName{firstComponent.fileName};
        parseQmlComponent(fileName,
                          directoryPath,
                          createExportedTypes(componentsWithSameFileName),
                          directoryId,
                          package,
                          notUpdatedSourceIds,
                          WatchedSourceIds,
                          qmldirState,
                          qmldirSourceId,
                          isInsideProject);
    };

    rangeForTheSameFileName(components, callback);
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
    SourceContextId sourceContextId,
    Storage::Synchronization::SynchronizationPackage &package,
    NotUpdatedSourceIds &notUpdatedSourceIds) const
{
    auto sourceId = SourceId::create(SourceNameId{}, sourceContextId);
    return fileState(sourceId, package, notUpdatedSourceIds);
}

} // namespace QmlDesigner
