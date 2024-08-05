// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "projectstorageupdater.h"

#include "filestatuscache.h"
#include "filesysteminterface.h"
#include "projectstorage.h"
#include "projectstoragepathwatcherinterface.h"
#include "qmldocumentparserinterface.h"
#include "qmltypesparserinterface.h"
#include "sourcepath.h"
#include "sourcepathcache.h"
#include "typeannotationreader.h"

#include <sqlitedatabase.h>
#include <tracing/qmldesignertracing.h>
#include <utils/set_algorithm.h>

#include <QDirIterator>
#include <QRegularExpression>

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
    case ProjectStorageUpdater::FileState::NotChanged:
        convertToString(string, "NotChanged");
        break;
    case ProjectStorageUpdater::FileState::NotExists:
        convertToString(string, "NotExists");
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

QList<QmlDirParser::Import> filterMultipleEntries(QList<QmlDirParser::Import> imports)
{
    std::stable_sort(imports.begin(), imports.end(), [](auto &&first, auto &&second) {
        return first.module < second.module;
    });
    imports.erase(std::unique(imports.begin(),
                              imports.end(),
                              [](auto &&first, auto &&second) {
                                  return first.module == second.module;
                              }),
                  imports.end());

    return imports;
}

QList<QmlDirParser::Import> joinImports(const QList<QmlDirParser::Import> &firstImports,
                                        const QList<QmlDirParser::Import> &secondImports)
{
    QList<QmlDirParser::Import> imports;
    imports.reserve(firstImports.size() + secondImports.size());
    imports.append(firstImports);
    imports.append(secondImports);
    imports = filterMultipleEntries(std::move(imports));

    return imports;
}

ProjectStorageUpdater::Components createComponents(
    const QMultiHash<QString, QmlDirParser::Component> &qmlDirParserComponents,
    ModuleId moduleId,
    ModuleId pathModuleId,
    FileSystemInterface &fileSystem,
    const Utils::PathString &directory)
{
    ProjectStorageUpdater::Components components;

    auto qmlFileNames = fileSystem.qmlFileNames(QString{directory});

    components.reserve(static_cast<std::size_t>(qmlDirParserComponents.size() + qmlFileNames.size()));

    for (const QString &qmlFileName : qmlFileNames) {
        Utils::PathString fileName{qmlFileName};
        Utils::PathString typeName{fileName.begin(), std::find(fileName.begin(), fileName.end(), '.')};
        components.push_back(
            ProjectStorageUpdater::Component{fileName, typeName, pathModuleId, -1, -1});
    }

    for (const QmlDirParser::Component &qmlDirParserComponent : qmlDirParserComponents) {
        if (qmlDirParserComponent.fileName.contains('/'))
            continue;
        components.push_back(ProjectStorageUpdater::Component{qmlDirParserComponent.fileName,
                                                              qmlDirParserComponent.typeName,
                                                              moduleId,
                                                              qmlDirParserComponent.majorVersion,
                                                              qmlDirParserComponent.minorVersion});
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

Storage::Version convertVersion(LanguageUtils::ComponentVersion version)
{
    return Storage::Version{version.majorVersion(), version.minorVersion()};
}

Storage::Synchronization::IsAutoVersion convertToIsAutoVersion(QmlDirParser::Import::Flags flags)
{
    if (flags & QmlDirParser::Import::Flag::Auto)
        return Storage::Synchronization::IsAutoVersion::Yes;
    return Storage::Synchronization::IsAutoVersion::No;
}

void addDependencies(Storage::Imports &dependencies,
                     SourceId sourceId,
                     const QList<QmlDirParser::Import> &qmldirDependencies,
                     ProjectStorageInterface &projectStorage,
                     TracerLiteral message,
                     Tracer &tracer)
{
    for (const QmlDirParser::Import &qmldirDependency : qmldirDependencies) {
        ModuleId moduleId = projectStorage.moduleId(Utils::PathString{qmldirDependency.module},
                                                    Storage::ModuleKind::CppLibrary);
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
    NanotraceHR::Tracer tracer{"add module exported imports"_t,
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

bool isOptionalImport(QmlDirParser::Import::Flags flags)
{
    return flags & QmlDirParser::Import::Optional && !(flags & QmlDirParser::Import::OptionalDefault);
}

void addModuleExportedImports(Storage::Synchronization::ModuleExportedImports &imports,
                              ModuleId moduleId,
                              ModuleId cppModuleId,
                              std::string_view moduleName,
                              const QList<QmlDirParser::Import> &qmldirImports,
                              ProjectStorageInterface &projectStorage)
{
    NanotraceHR::Tracer tracer{"add module exported imports"_t,
                               category(),
                               keyValue("cpp module id", cppModuleId),
                               keyValue("module id", moduleId)};

    for (const QmlDirParser::Import &qmldirImport : qmldirImports) {
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
                                convertToIsAutoVersion(qmldirImport.flags),
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

std::vector<IdPaths> createIdPaths(ProjectStorageUpdater::WatchedSourceIdsIds watchedSourceIds,
                                   ProjectPartId projectPartId)
{
    std::vector<IdPaths> idPaths;
    idPaths.reserve(4);

    idPaths.push_back(
        {projectPartId, SourceType::Directory, std::move(watchedSourceIds.directorySourceIds)});
    idPaths.push_back({projectPartId, SourceType::QmlDir, std::move(watchedSourceIds.qmldirSourceIds)});
    idPaths.push_back({projectPartId, SourceType::Qml, std::move(watchedSourceIds.qmlSourceIds)});
    idPaths.push_back(
        {projectPartId, SourceType::QmlTypes, std::move(watchedSourceIds.qmltypesSourceIds)});

    return idPaths;
}

} // namespace

void ProjectStorageUpdater::update(Update update)
{
    QStringList directories = std::move(update.directories);
    QStringList qmlTypesPaths = std::move(update.qmlTypesPaths);
    const QString &propertyEditorResourcesPath = update.propertyEditorResourcesPath;
    const QStringList &typeAnnotationPaths = update.typeAnnotationPaths;

    NanotraceHR::Tracer tracer{"update"_t,
                               category(),
                               keyValue("directories", directories),
                               keyValue("qml types paths", qmlTypesPaths)};

    Storage::Synchronization::SynchronizationPackage package;
    WatchedSourceIdsIds watchedSourceIds{Utils::span{directories}.size()};
    NotUpdatedSourceIds notUpdatedSourceIds{Utils::span{directories}.size()};

    updateDirectories(directories, package, notUpdatedSourceIds, watchedSourceIds);
    updateQmlTypes(qmlTypesPaths, package, notUpdatedSourceIds, watchedSourceIds);
    updatePropertyEditorPaths(propertyEditorResourcesPath, package, notUpdatedSourceIds);
    updateTypeAnnotations(typeAnnotationPaths, package, notUpdatedSourceIds);

    package.updatedSourceIds = filterNotUpdatedSourceIds(std::move(package.updatedSourceIds),
                                                         std::move(notUpdatedSourceIds.sourceIds));
    package.updatedFileStatusSourceIds = filterNotUpdatedSourceIds(
        std::move(package.updatedFileStatusSourceIds),
        std::move(notUpdatedSourceIds.fileStatusSourceIds));

    try {
        m_projectStorage.synchronize(std::move(package));
    } catch (const TypeNameDoesNotExists &exception) {
        qDebug() << "missing type: " << exception.what();
    } catch (...) {
        qWarning() << "Project storage could not been updated!";
    }

    m_pathWatcher.updateIdPaths(createIdPaths(watchedSourceIds, m_projectPartId));
}

void ProjectStorageUpdater::updateQmlTypes(const QStringList &qmlTypesPaths,
                                           Storage::Synchronization::SynchronizationPackage &package,
                                           NotUpdatedSourceIds &notUpdatedSourceIds,
                                           WatchedSourceIdsIds &watchedSourceIdsIds)
{
    if (qmlTypesPaths.empty())
        return;

    NanotraceHR::Tracer tracer{"update qmltypes file"_t, category()};

    ModuleId moduleId = m_projectStorage.moduleId("QML", Storage::ModuleKind::CppLibrary);

    for (const QString &qmlTypesPath : qmlTypesPaths) {
        SourceId sourceId = m_pathCache.sourceId(SourcePath{qmlTypesPath});
        watchedSourceIdsIds.qmltypesSourceIds.push_back(sourceId);
        tracer.tick("append watched qml types source id"_t,
                    keyValue("source id", sourceId),
                    keyValue("qml types path", qmlTypesPath));

        Storage::Synchronization::DirectoryInfo directoryInfo{
            sourceId, sourceId, moduleId, Storage::Synchronization::FileType::QmlTypes};

        FileState state = parseTypeInfo(directoryInfo,
                                        Utils::PathString{qmlTypesPath},
                                        package,
                                        notUpdatedSourceIds);

        if (state == FileState::Changed) {
            tracer.tick("append project data"_t, keyValue("project data", directoryInfo));
            package.directoryInfos.push_back(std::move(directoryInfo));
            tracer.tick("append updated project source ids"_t, keyValue("source id", sourceId));
            package.updatedDirectoryInfoSourceIds.push_back(sourceId);
        }
    }
}

namespace {
template<typename... FileStates>
ProjectStorageUpdater::FileState combineState(FileStates... fileStates)
{
    if (((fileStates == ProjectStorageUpdater::FileState::Changed) || ...))
        return ProjectStorageUpdater::FileState::Changed;

    if (((fileStates == ProjectStorageUpdater::FileState::NotChanged) || ...))
        return ProjectStorageUpdater::FileState::NotChanged;

    return ProjectStorageUpdater::FileState::NotExists;
}

} // namespace

void ProjectStorageUpdater::updateDirectoryChanged(std::string_view directoryPath,
                                                   FileState qmldirState,
                                                   SourcePath qmldirSourcePath,
                                                   SourceId qmldirSourceId,
                                                   SourceId directorySourceId,
                                                   SourceContextId directoryId,
                                                   Storage::Synchronization::SynchronizationPackage &package,
                                                   NotUpdatedSourceIds &notUpdatedSourceIds,
                                                   WatchedSourceIdsIds &watchedSourceIdsIds,
                                                   Tracer &tracer)
{
    QmlDirParser parser;
    if (qmldirState != FileState::NotExists)
        parser.parse(m_fileSystem.contentAsQString(QString{qmldirSourcePath}));

    if (qmldirState != FileState::NotChanged) {
        tracer.tick("append updated source id"_t, keyValue("module id", qmldirSourceId));
        package.updatedSourceIds.push_back(qmldirSourceId);
    }

    using Storage::ModuleKind;
    Utils::PathString moduleName{parser.typeNamespace()};
    ModuleId moduleId = m_projectStorage.moduleId(moduleName, ModuleKind::QmlLibrary);
    ModuleId cppModuleId = m_projectStorage.moduleId(moduleName, ModuleKind::CppLibrary);
    ModuleId pathModuleId = m_projectStorage.moduleId(directoryPath, ModuleKind::PathLibrary);

    auto imports = filterMultipleEntries(parser.imports());

    addModuleExportedImports(package.moduleExportedImports,
                             moduleId,
                             cppModuleId,
                             moduleName,
                             imports,
                             m_projectStorage);
    tracer.tick("append updated module id"_t, keyValue("module id", moduleId));
    package.updatedModuleIds.push_back(moduleId);

    const auto qmlDirectoryInfos = m_projectStorage.fetchDirectoryInfos(directorySourceId);
    addSourceIds(package.updatedSourceIds, qmlDirectoryInfos, "append updated source id"_t, tracer);
    addSourceIds(package.updatedFileStatusSourceIds,
                 qmlDirectoryInfos,
                 "append updated file status source id"_t,
                 tracer);

    auto qmlTypes = filterMultipleEntries(parser.typeInfos());

    if (!qmlTypes.isEmpty()) {
        parseTypeInfos(qmlTypes,
                       filterMultipleEntries(parser.dependencies()),
                       imports,
                       directorySourceId,
                       directoryPath,
                       cppModuleId,
                       package,
                       notUpdatedSourceIds,
                       watchedSourceIdsIds);
    }
    parseQmlComponents(
        createComponents(parser.components(), moduleId, pathModuleId, m_fileSystem, directoryPath),
        directorySourceId,
        directoryId,
        package,
        notUpdatedSourceIds,
        watchedSourceIdsIds,
        qmldirState);
    tracer.tick("append updated project source id"_t, keyValue("module id", moduleId));
    package.updatedDirectoryInfoSourceIds.push_back(directorySourceId);
}

void ProjectStorageUpdater::updateDirectories(const QStringList &directories,
                                              Storage::Synchronization::SynchronizationPackage &package,
                                              NotUpdatedSourceIds &notUpdatedSourceIds,
                                              WatchedSourceIdsIds &watchedSourceIdsIds)
{
    NanotraceHR::Tracer tracer{"update directories"_t, category()};

    for (const QString &directory : directories)
        updateDirectory({directory}, {}, package, notUpdatedSourceIds, watchedSourceIdsIds);
}

void ProjectStorageUpdater::updateSubdirectories(const Utils::PathString &directoryPath,
                                                 SourceId directorySourceId,
                                                 FileState directoryState,
                                                 const SourceContextIds &subdirectoriesToIgnore,
                                                 Storage::Synchronization::SynchronizationPackage &package,
                                                 NotUpdatedSourceIds &notUpdatedSourceIds,
                                                 WatchedSourceIdsIds &watchedSourceIdsIds)
{
    struct Directory
    {
        Directory(Utils::SmallStringView path, SourceId sourceId)
            : path{path}
            , sourceId{sourceId}
        {}

        bool operator<(const Directory &other) const
        {
            return sourceId.contextId() < other.sourceId.contextId();
        }

        bool operator==(const Directory &other) const
        {
            return sourceId.contextId() == other.sourceId.contextId();
        }

        Utils::PathString path;
        SourceId sourceId;
    };

    struct Compare
    {
        bool operator()(const Directory &first, const Directory &second) const
        {
            return first.sourceId.contextId() < second.sourceId.contextId();
        }

        bool operator()(const Directory &first, SourceContextId second) const
        {
            return first.sourceId.contextId() < second;
        }

        bool operator()(SourceContextId first, const Directory &second) const
        {
            return first < second.sourceId.contextId();
        }
    };

    using Directories = QVarLengthArray<Directory, 32>;

    auto subdirectorySourceIds = m_projectStorage.fetchSubdirectorySourceIds(directorySourceId);
    auto subdirectories = Utils::transform<Directories>(
        subdirectorySourceIds, [&](SourceId sourceId) -> Directory {
            auto sourceContextId = sourceId.contextId();
            auto subdirectoryPath = m_pathCache.sourceContextPath(sourceContextId);
            return {subdirectoryPath, sourceId};
        });

    auto exisitingSubdirectoryPaths = m_fileSystem.subdirectories(directoryPath.toQString());
    Directories existingSubdirecories;
    for (const QString &subdirectory : exisitingSubdirectoryPaths) {
        if (subdirectory.endsWith("/designer") || subdirectory.endsWith("/QtQuick/Scene2D")
            || subdirectory.endsWith("/QtQuick/Scene3D"))
            continue;
        Utils::PathString subdirectoryPath = subdirectory;
        SourceId sourceId = m_pathCache.sourceId(SourcePath{subdirectoryPath + "/."});
        subdirectories.emplace_back(subdirectoryPath, sourceId);
        existingSubdirecories.emplace_back(subdirectoryPath, sourceId);
    }

    std::sort(subdirectories.begin(), subdirectories.end());
    subdirectories.erase(std::unique(subdirectories.begin(), subdirectories.end()),
                         subdirectories.end());

    std::set_difference(subdirectories.begin(),
                        subdirectories.end(),
                        subdirectoriesToIgnore.begin(),
                        subdirectoriesToIgnore.end(),
                        Utils::make_iterator([&](const Directory &subdirectory) {
                            updateDirectory(subdirectory.path,
                                            subdirectoriesToIgnore,
                                            package,
                                            notUpdatedSourceIds,
                                            watchedSourceIdsIds);
                        }),
                        Compare{});

    if (directoryState == FileState::Changed) {
        for (const auto &[subdirectoryPath, subdirectorySourceId] : existingSubdirecories) {
            package.directoryInfos.emplace_back(directorySourceId,
                                                subdirectorySourceId,
                                                ModuleId{},
                                                Storage::Synchronization::FileType::Directory);
        }
    }
}

void ProjectStorageUpdater::updateDirectory(const Utils::PathString &directoryPath,
                                            const SourceContextIds &subdirectoriesToIgnore,
                                            Storage::Synchronization::SynchronizationPackage &package,
                                            NotUpdatedSourceIds &notUpdatedSourceIds,
                                            WatchedSourceIdsIds &watchedSourceIdsIds)
{
    NanotraceHR::Tracer tracer{"update directory"_t, category(), keyValue("directory", directoryPath)};

    SourcePath qmldirSourcePath{directoryPath + "/qmldir"};
    auto [directoryId, qmldirSourceId] = m_pathCache.sourceContextAndSourceId(qmldirSourcePath);

    SourcePath directorySourcePath{directoryPath + "/."};
    auto directorySourceId = m_pathCache.sourceId(directorySourcePath);
    auto directoryState = fileState(directorySourceId, package, notUpdatedSourceIds);
    if (directoryState != FileState::NotExists)
        watchedSourceIdsIds.directorySourceIds.push_back(directorySourceId);

    auto qmldirState = fileState(qmldirSourceId, package, notUpdatedSourceIds);
    if (qmldirState != FileState::NotExists)
        watchedSourceIdsIds.qmldirSourceIds.push_back(qmldirSourceId);

    switch (combineState(directoryState, qmldirState)) {
    case FileState::Changed: {
        tracer.tick("update directory changed"_t);
        updateDirectoryChanged(directoryPath,
                               qmldirState,
                               qmldirSourcePath,
                               qmldirSourceId,
                               directorySourceId,
                               directoryId,
                               package,
                               notUpdatedSourceIds,
                               watchedSourceIdsIds,
                               tracer);
        break;
    }
    case FileState::NotChanged: {
        tracer.tick("update directory not changed"_t);

        parseDirectoryInfos(m_projectStorage.fetchDirectoryInfos(directorySourceId),
                            package,
                            notUpdatedSourceIds,
                            watchedSourceIdsIds);
        break;
    }
    case FileState::NotExists: {
        tracer.tick("update directory don't exits"_t);

        package.updatedFileStatusSourceIds.push_back(directorySourceId);
        package.updatedFileStatusSourceIds.push_back(qmldirSourceId);
        package.updatedDirectoryInfoSourceIds.push_back(directorySourceId);
        package.updatedSourceIds.push_back(qmldirSourceId);
        auto qmlDirectoryInfos = m_projectStorage.fetchDirectoryInfos(directorySourceId);
        for (const Storage::Synchronization::DirectoryInfo &directoryInfo : qmlDirectoryInfos) {
            tracer.tick("append updated source id"_t, keyValue("source id", directoryInfo.sourceId));
            package.updatedSourceIds.push_back(directoryInfo.sourceId);
            tracer.tick("append updated file status source id"_t,
                        keyValue("source id", directoryInfo.sourceId));
            package.updatedFileStatusSourceIds.push_back(directoryInfo.sourceId);
        }

        break;
    }
    }

    updateSubdirectories(directoryPath,
                         directorySourceId,
                         directoryState,
                         subdirectoriesToIgnore,
                         package,
                         notUpdatedSourceIds,
                         watchedSourceIdsIds);

    tracer.end(keyValue("qmldir source path", qmldirSourcePath),
               keyValue("directory source path", directorySourcePath),
               keyValue("directory id", directoryId),
               keyValue("qmldir source id", qmldirSourceId),
               keyValue("directory source source id", directorySourceId),
               keyValue("qmldir state", qmldirState),
               keyValue("directory state", directoryState));
}

void ProjectStorageUpdater::updatePropertyEditorPaths(
    const QString &propertyEditorResourcesPath,
    Storage::Synchronization::SynchronizationPackage &package,
    NotUpdatedSourceIds &notUpdatedSourceIds)
{
    NanotraceHR::Tracer tracer{"update property editor paths"_t,
                               category(),
                               keyValue("property editor resources path", propertyEditorResourcesPath)};

    if (propertyEditorResourcesPath.isEmpty())
        return;

    QDirIterator dirIterator{QDir::cleanPath(propertyEditorResourcesPath),
                             QDir::Dirs | QDir::NoDotAndDotDot,
                             QDirIterator::Subdirectories};

    while (dirIterator.hasNext()) {
        auto pathInfo = dirIterator.nextFileInfo();

        SourceId directorySourceId = m_pathCache.sourceId(SourcePath{pathInfo.filePath() + "/."});

        auto state = fileState(directorySourceId, package, notUpdatedSourceIds);

        if (state == FileState::Changed) {
            updatePropertyEditorPath(pathInfo.filePath(),
                                     package,
                                     directorySourceId,
                                     propertyEditorResourcesPath.size() + 1);
        }
    }
}

namespace {

template<typename SourceIds1, typename SourceIds2>
SmallSourceIds<16> mergedSourceIds(const SourceIds1 &sourceIds1, const SourceIds2 &sourceIds2)
{
    SmallSourceIds<16> mergedSourceIds;

    std::set_union(sourceIds1.begin(),
                   sourceIds1.end(),
                   sourceIds2.begin(),
                   sourceIds2.end(),
                   std::back_inserter(mergedSourceIds));

    return mergedSourceIds;
}
} // namespace

void ProjectStorageUpdater::updateTypeAnnotations(const QStringList &directoryPaths,
                                                  Storage::Synchronization::SynchronizationPackage &package,
                                                  NotUpdatedSourceIds &notUpdatedSourceIds)
{
    NanotraceHR::Tracer tracer("update type annotations"_t, category());

    std::map<SourceId, SmallSourceIds<16>> updatedSourceIdsDictonary;

    for (SourceId directoryId : m_projectStorage.typeAnnotationDirectorySourceIds())
        updatedSourceIdsDictonary[directoryId] = {};

    for (const auto &directoryPath : directoryPaths)
        updateTypeAnnotations(directoryPath, package, notUpdatedSourceIds, updatedSourceIdsDictonary);

    updateTypeAnnotationDirectories(package, notUpdatedSourceIds, updatedSourceIdsDictonary);
}

void ProjectStorageUpdater::updateTypeAnnotations(
    const QString &rootDirectoryPath,
    Storage::Synchronization::SynchronizationPackage &package,
    NotUpdatedSourceIds &notUpdatedSourceIds,
    std::map<SourceId, SmallSourceIds<16>> &updatedSourceIdsDictonary)
{
    NanotraceHR::Tracer tracer("update type annotation directory"_t,
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

        SourceId directorySourceId = m_pathCache.sourceId(SourcePath{directoryPath + "/."});

        auto state = fileState(sourceId, package, notUpdatedSourceIds);
        if (state == FileState::Changed)
            updateTypeAnnotation(directoryPath, fileInfo.filePath(), sourceId, directorySourceId, package);

        if (state != FileState::NotChanged)
            updatedSourceIdsDictonary[directorySourceId].push_back(sourceId);
    }
}

void ProjectStorageUpdater::updateTypeAnnotationDirectories(
    Storage::Synchronization::SynchronizationPackage &package,
    NotUpdatedSourceIds &notUpdatedSourceIds,
    std::map<SourceId, SmallSourceIds<16>> &updatedSourceIdsDictonary)
{
    for (auto &[directorySourceId, updatedSourceIds] : updatedSourceIdsDictonary) {
        auto directoryState = fileState(directorySourceId, package, notUpdatedSourceIds);

        if (directoryState != FileState::NotChanged) {
            auto existingTypeAnnotationSourceIds = m_projectStorage.typeAnnotationSourceIds(
                directorySourceId);

            std::sort(updatedSourceIds.begin(), updatedSourceIds.end());

            auto changedSourceIds = mergedSourceIds(existingTypeAnnotationSourceIds, updatedSourceIds);
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
                                                 SourceId directorySourceId,
                                                 Storage::Synchronization::SynchronizationPackage &package)
{
    NanotraceHR::Tracer tracer{"update type annotation path"_t,
                               category(),
                               keyValue("path", filePath),
                               keyValue("directory path", directoryPath)};

    Storage::TypeAnnotationReader reader{m_projectStorage};

    auto annotations = reader.parseTypeAnnotation(contentFromFile(filePath),
                                                  directoryPath,
                                                  sourceId,
                                                  directorySourceId);
    auto &typeAnnotations = package.typeAnnotations;
    package.typeAnnotations.insert(typeAnnotations.end(),
                                   std::make_move_iterator(annotations.begin()),
                                   std::make_move_iterator(annotations.end()));
}

void ProjectStorageUpdater::updatePropertyEditorPath(
    const QString &directoryPath,
    Storage::Synchronization::SynchronizationPackage &package,
    SourceId directorySourceId,
    long long pathOffset)
{
    NanotraceHR::Tracer tracer{"update property editor path"_t,
                               category(),
                               keyValue("directory path", directoryPath),
                               keyValue("directory source id", directorySourceId)};

    tracer.tick("append updated property editor qml path source id"_t,
                keyValue("source id", directorySourceId));
    package.updatedPropertyEditorQmlPathSourceIds.push_back(directorySourceId);
    auto dir = QDir{directoryPath};
    const auto fileInfos = dir.entryInfoList({"*Pane.qml", "*Specifics.qml"}, QDir::Files);
    for (const auto &fileInfo : fileInfos)
        updatePropertyEditorFilePath(fileInfo.filePath(), package, directorySourceId, pathOffset);
}

void ProjectStorageUpdater::updatePropertyEditorFilePath(
    const QString &path,
    Storage::Synchronization::SynchronizationPackage &package,
    SourceId directorySourceId,
    long long pathOffset)
{
    NanotraceHR::Tracer tracer{"update property editor file path"_t,
                               category(),
                               keyValue("directory path", path),
                               keyValue("directory source id", directorySourceId)};

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
                                                                        directorySourceId);
        tracer.tick("append property editor qml paths"_t,
                    keyValue("property editor qml paths", paths));
    }
}

namespace {
SourceContextIds filterUniqueSourceContextIds(const SourceIds &sourceIds)
{
    auto sourceContextIds = Utils::transform(sourceIds,
                                             [](SourceId sourceId) { return sourceId.contextId(); });

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
} // namespace

void ProjectStorageUpdater::pathsWithIdsChanged(const std::vector<IdPaths> &changedIdPaths)
{
    NanotraceHR::Tracer tracer{"paths with ids changed"_t,
                               category(),
                               keyValue("id paths", changedIdPaths)};

    m_changedIdPaths.insert(m_changedIdPaths.end(), changedIdPaths.begin(), changedIdPaths.end());

    Storage::Synchronization::SynchronizationPackage package;

    WatchedSourceIdsIds watchedSourceIds{10};
    NotUpdatedSourceIds notUpdatedSourceIds{10};
    std::vector<IdPaths> idPaths;
    idPaths.reserve(4);

    SourceIds directorySourceIds;
    directorySourceIds.reserve(32);
    SourceIds qmlDocumentSourceIds;
    qmlDocumentSourceIds.reserve(128);
    SourceIds qmltypesSourceIds;
    qmltypesSourceIds.reserve(32);

    for (const auto &[projectChunkId, sourceIds] : m_changedIdPaths) {
        if (projectChunkId.id != m_projectPartId)
            continue;

        switch (projectChunkId.sourceType) {
        case SourceType::Directory:
        case SourceType::QmlDir:
            directorySourceIds.insert(directorySourceIds.end(), sourceIds.begin(), sourceIds.end());
            break;
        case SourceType::Qml:
        case SourceType::QmlUi:
            qmlDocumentSourceIds.insert(qmlDocumentSourceIds.end(), sourceIds.begin(), sourceIds.end());
            break;
        case SourceType::QmlTypes:
            qmltypesSourceIds.insert(qmltypesSourceIds.end(), sourceIds.begin(), sourceIds.end());
            break;
        }
    }

    auto directorySourceContextIds = filterUniqueSourceContextIds(directorySourceIds);

    for (auto sourceContextId : directorySourceContextIds) {
        Utils::PathString directory = m_pathCache.sourceContextPath(sourceContextId);
        updateDirectory(directory,
                        directorySourceContextIds,
                        package,
                        notUpdatedSourceIds,
                        watchedSourceIds);
    }

    for (SourceId sourceId : filterUniqueSourceIds(qmlDocumentSourceIds)) {
        if (!contains(directorySourceContextIds, sourceId.contextId()))
            parseQmlComponent(sourceId, package, notUpdatedSourceIds);
    }

    try {
        for (SourceId sourceId : filterUniqueSourceIds(std::move(qmltypesSourceIds))) {
            if (!contains(directorySourceContextIds, sourceId.contextId())) {
                auto qmltypesPath = m_pathCache.sourcePath(sourceId);
                auto directoryInfo = m_projectStorage.fetchDirectoryInfo(sourceId);
                if (directoryInfo)
                    parseTypeInfo(*directoryInfo, qmltypesPath, package, notUpdatedSourceIds);
            }
        }
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

    if (directorySourceContextIds.size()) {
        m_pathWatcher.updateContextIdPaths(createIdPaths(watchedSourceIds, m_projectPartId),
                                           directorySourceContextIds);
    }

    m_changedIdPaths.clear();
}

void ProjectStorageUpdater::pathsChanged(const SourceIds &) {}

void ProjectStorageUpdater::parseTypeInfos(const QStringList &typeInfos,
                                           const QList<QmlDirParser::Import> &qmldirDependencies,
                                           const QList<QmlDirParser::Import> &qmldirImports,
                                           SourceId directorySourceId,
                                           Utils::SmallStringView directoryPath,
                                           ModuleId moduleId,
                                           Storage::Synchronization::SynchronizationPackage &package,
                                           NotUpdatedSourceIds &notUpdatedSourceIds,
                                           WatchedSourceIdsIds &watchedSourceIds)
{
    NanotraceHR::Tracer tracer{"parse type infos"_t,
                               category(),
                               keyValue("directory source id", directorySourceId),
                               keyValue("directory path", directoryPath),
                               keyValue("module id", moduleId)};

    for (const QString &typeInfo : typeInfos) {
        NanotraceHR::Tracer tracer{"parse type info"_t, category(), keyValue("type info", typeInfo)};

        Utils::PathString qmltypesPath = Utils::PathString::join(
            {directoryPath, "/", Utils::SmallString{typeInfo}});
        SourceId sourceId = m_pathCache.sourceId(SourcePathView{qmltypesPath});

        tracer.tick("append qmltypes source id"_t, keyValue("source id", sourceId));
        watchedSourceIds.qmltypesSourceIds.push_back(sourceId);

        addDependencies(package.moduleDependencies,
                        sourceId,
                        joinImports(qmldirDependencies, qmldirImports),
                        m_projectStorage,
                        "append module dependency"_t,
                        tracer);

        tracer.tick("append module dependenct source source id"_t, keyValue("source id", sourceId));
        package.updatedModuleDependencySourceIds.push_back(sourceId);

        const auto &directoryInfo = package.directoryInfos.emplace_back(
            directorySourceId, sourceId, moduleId, Storage::Synchronization::FileType::QmlTypes);
        tracer.tick("append project data"_t, keyValue("source id", sourceId));

        parseTypeInfo(directoryInfo, qmltypesPath, package, notUpdatedSourceIds);
    }
}

void ProjectStorageUpdater::parseDirectoryInfos(
    const Storage::Synchronization::DirectoryInfos &directoryInfos,
    Storage::Synchronization::SynchronizationPackage &package,
    NotUpdatedSourceIds &notUpdatedSourceIds,
    WatchedSourceIdsIds &watchedSourceIds)
{
    NanotraceHR::Tracer tracer{"parse project datas"_t, category()};

    for (const Storage::Synchronization::DirectoryInfo &directoryInfo : directoryInfos) {
        switch (directoryInfo.fileType) {
        case Storage::Synchronization::FileType::QmlTypes: {
            watchedSourceIds.qmltypesSourceIds.push_back(directoryInfo.sourceId);

            auto qmltypesPath = m_pathCache.sourcePath(directoryInfo.sourceId);
            parseTypeInfo(directoryInfo, qmltypesPath, package, notUpdatedSourceIds);
            break;
        }
        case Storage::Synchronization::FileType::QmlDocument: {
            watchedSourceIds.qmlSourceIds.push_back(directoryInfo.sourceId);

            parseQmlComponent(directoryInfo.sourceId, package, notUpdatedSourceIds);
            break;
        }
        case Storage::Synchronization::FileType::Directory:
            break;
        }
    }
}

auto ProjectStorageUpdater::parseTypeInfo(const Storage::Synchronization::DirectoryInfo &directoryInfo,
                                          Utils::SmallStringView qmltypesPath,
                                          Storage::Synchronization::SynchronizationPackage &package,
                                          NotUpdatedSourceIds &notUpdatedSourceIds) -> FileState
{
    NanotraceHR::Tracer tracer{"parse type info"_t,
                               category(),
                               keyValue("qmltypes path", qmltypesPath)};

    auto state = fileState(directoryInfo.sourceId, package, notUpdatedSourceIds);
    switch (state) {
    case FileState::Changed: {
        tracer.tick("append updated source ids"_t, keyValue("source id", directoryInfo.sourceId));
        package.updatedSourceIds.push_back(directoryInfo.sourceId);

        const auto content = m_fileSystem.contentAsQString(QString{qmltypesPath});
        m_qmlTypesParser.parse(content, package.imports, package.types, directoryInfo);
        break;
    }
    case FileState::NotChanged: {
        tracer.tick("append not updated source ids"_t, keyValue("source id", directoryInfo.sourceId));
        notUpdatedSourceIds.sourceIds.push_back(directoryInfo.sourceId);
        break;
    }
    case FileState::NotExists:
        throw CannotParseQmlTypesFile{};
    }

    tracer.end(keyValue("state", state));

    return state;
}

void ProjectStorageUpdater::parseQmlComponent(Utils::SmallStringView relativeFilePath,
                                              Utils::SmallStringView directoryPath,
                                              Storage::Synchronization::ExportedTypes exportedTypes,
                                              SourceId directorySourceId,
                                              Storage::Synchronization::SynchronizationPackage &package,
                                              NotUpdatedSourceIds &notUpdatedSourceIds,
                                              WatchedSourceIdsIds &watchedSourceIds,
                                              FileState qmldirState)
{
    NanotraceHR::Tracer tracer{"parse qml component"_t,
                               category(),
                               keyValue("relative file path", relativeFilePath),
                               keyValue("directory path", directoryPath),
                               keyValue("exported types", exportedTypes),
                               keyValue("directory source id", directorySourceId),
                               keyValue("qmldir state", qmldirState)};

    if (std::find(relativeFilePath.begin(), relativeFilePath.end(), '+') != relativeFilePath.end())
        return;

    Utils::PathString qmlFilePath = Utils::PathString::join({directoryPath, "/", relativeFilePath});
    SourceId sourceId = m_pathCache.sourceId(SourcePathView{qmlFilePath});

    Storage::Synchronization::Type type;
    auto state = fileState(sourceId, package, notUpdatedSourceIds);

    tracer.tick("append watched qml source id"_t, keyValue("source id", sourceId));
    watchedSourceIds.qmlSourceIds.push_back(sourceId);

    switch (state) {
    case FileState::NotChanged:
        if (qmldirState == FileState::NotExists) {
            tracer.tick("append not updated source id"_t, keyValue("source id", sourceId));
            notUpdatedSourceIds.sourceIds.emplace_back(sourceId);

            const auto &directoryInfo = package.directoryInfos.emplace_back(
                directorySourceId, sourceId, ModuleId{}, Storage::Synchronization::FileType::QmlDocument);
            tracer.tick("append project data"_t, keyValue("project data", directoryInfo));

            return;
        }
        type.changeLevel = Storage::Synchronization::ChangeLevel::Minimal;
        break;
    case FileState::NotExists:
        throw CannotParseQmlDocumentFile{};
    case FileState::Changed:
        const auto content = m_fileSystem.contentAsQString(QString{qmlFilePath});
        type = m_qmlDocumentParser.parse(content, package.imports, sourceId, directoryPath);
        break;
    }

    const auto &directoryInfo = package.directoryInfos.emplace_back(
        directorySourceId, sourceId, ModuleId{}, Storage::Synchronization::FileType::QmlDocument);
    tracer.tick("append project data"_t, keyValue("project data", directoryInfo));

    tracer.tick("append updated source id"_t, keyValue("source id", sourceId));
    package.updatedSourceIds.push_back(sourceId);

    type.typeName = SourcePath{qmlFilePath}.name();
    type.sourceId = sourceId;
    type.exportedTypes = std::move(exportedTypes);

    tracer.end(keyValue("type", type));

    package.types.push_back(std::move(type));
}

void ProjectStorageUpdater::parseQmlComponent(SourceId sourceId,
                                              Storage::Synchronization::SynchronizationPackage &package,
                                              NotUpdatedSourceIds &notUpdatedSourceIds)
{
    NanotraceHR::Tracer tracer{"parse qml component"_t, category(), keyValue("source id", sourceId)};

    auto state = fileState(sourceId, package, notUpdatedSourceIds);
    if (state == FileState::NotChanged)
        return;

    tracer.tick("append updated source id"_t, keyValue("source id", sourceId));
    package.updatedSourceIds.push_back(sourceId);

    if (state == FileState::NotExists)
        return;

    SourcePath sourcePath = m_pathCache.sourcePath(sourceId);

    const auto content = m_fileSystem.contentAsQString(QString{sourcePath});
    auto type = m_qmlDocumentParser.parse(content, package.imports, sourceId, sourcePath.directory());

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

Storage::Synchronization::ExportedTypes createExportedTypes(ProjectStorageUpdater::ComponentRange components)
{
    Storage::Synchronization::ExportedTypes exportedTypes;
    exportedTypes.reserve(components.size() + 1);

    for (const ProjectStorageUpdater::Component &component : components) {
        exportedTypes.emplace_back(component.moduleId,
                                   Utils::SmallString{component.typeName},
                                   Storage::Version{component.majorVersion, component.minorVersion});
    }

    return exportedTypes;
}

} // namespace

void ProjectStorageUpdater::parseQmlComponents(Components components,
                                               SourceId directorySourceId,
                                               SourceContextId directoryId,
                                               Storage::Synchronization::SynchronizationPackage &package,
                                               NotUpdatedSourceIds &notUpdatedSourceIds,
                                               WatchedSourceIdsIds &watchedSourceIdsIds,
                                               FileState qmldirState)
{
    NanotraceHR::Tracer tracer{"parse qml components"_t,
                               category(),
                               keyValue("directory source id", directorySourceId),
                               keyValue("directory id", directoryId),
                               keyValue("qmldir state", qmldirState)};

    std::sort(components.begin(), components.end(), [](auto &&first, auto &&second) {
        return first.fileName < second.fileName;
    });

    auto directoryPath = m_pathCache.sourceContextPath(directoryId);

    auto callback = [&](ComponentRange componentsWithSameFileName) {
        const auto &firstComponent = *componentsWithSameFileName.begin();
        const Utils::SmallString fileName{firstComponent.fileName};
        parseQmlComponent(fileName,
                          directoryPath,
                          createExportedTypes(componentsWithSameFileName),
                          directorySourceId,
                          package,
                          notUpdatedSourceIds,
                          watchedSourceIdsIds,
                          qmldirState);
    };

    rangeForTheSameFileName(components, callback);
}

ProjectStorageUpdater::FileState ProjectStorageUpdater::fileState(
    SourceId sourceId,
    Storage::Synchronization::SynchronizationPackage &package,
    NotUpdatedSourceIds &notUpdatedSourceIds) const
{
    NanotraceHR::Tracer tracer{"update property editor paths"_t,
                               category(),
                               keyValue("source id", sourceId)};

    auto currentFileStatus = m_fileStatusCache.find(sourceId);

    if (!currentFileStatus.isValid()) {
        tracer.tick("append updated file status source id"_t, keyValue("source id", sourceId));
        package.updatedFileStatusSourceIds.push_back(sourceId);

        tracer.end(keyValue("state", FileState::NotExists));
        return FileState::NotExists;
    }

    auto projectStorageFileStatus = m_projectStorage.fetchFileStatus(sourceId);

    if (!projectStorageFileStatus.isValid() || projectStorageFileStatus != currentFileStatus) {
        tracer.tick("append file status"_t, keyValue("file status", sourceId));
        package.fileStatuses.push_back(currentFileStatus);

        tracer.tick("append updated file status source id"_t, keyValue("source id", sourceId));
        package.updatedFileStatusSourceIds.push_back(sourceId);

        tracer.end(keyValue("state", FileState::Changed));
        return FileState::Changed;
    }

    tracer.tick("append not updated file status source id"_t, keyValue("source id", sourceId));
    notUpdatedSourceIds.fileStatusSourceIds.push_back(sourceId);

    tracer.end(keyValue("state", FileState::NotChanged));
    return FileState::NotChanged;
}

} // namespace QmlDesigner
