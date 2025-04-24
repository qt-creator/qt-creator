// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "filestatus.h"
#include "projectstorageerrornotifierinterface.h"
#include "projectstorageids.h"
#include "projectstoragepathwatchernotifierinterface.h"
#include "projectstoragepathwatchertypes.h"
#include "projectstoragetypes.h"
#include "sourcepathstorage/nonlockingmutex.h"
#include "sourcepathstorage/sourcepath.h"

#include <modelfwd.h>

#include <tracing/qmldesignertracing.h>

#include <QStringList>

#include <map>
#include <vector>

namespace Sqlite {
class Database;
}

namespace QmlDesigner {
class FileSystemInterface;
class ProjectStorageInterface;
template<typename ProjectStorage, typename Mutex>
class SourcePathCache;
class FileStatusCache;
class ProjectStorage;
class QmlDocumentParserInterface;
class QmlTypesParserInterface;
class SourcePathStorage;

class QMLDESIGNERCORE_EXPORT ProjectStorageUpdater final
    : public ProjectStoragePathWatcherNotifierInterface
{
public:
    ProjectStorageUpdater(FileSystemInterface &fileSystem,
                          ProjectStorageType &projectStorage,
                          FileStatusCache &fileStatusCache,
                          PathCacheType &pathCache,
                          QmlDocumentParserInterface &qmlDocumentParser,
                          QmlTypesParserInterface &qmlTypesParser,
                          class ProjectStoragePathWatcherInterface &pathWatcher,
                          ProjectStorageErrorNotifierInterface &errorNotifier,
                          ProjectPartId projectPartId,
                          ProjectPartId qtPartId)
        : m_fileSystem{fileSystem}
        , m_projectStorage{projectStorage}
        , m_fileStatusCache{fileStatusCache}
        , m_pathCache{pathCache}
        , m_qmlDocumentParser{qmlDocumentParser}
        , m_qmlTypesParser{qmlTypesParser}
        , m_pathWatcher{pathWatcher}
        , m_errorNotifier{errorNotifier}
        , m_projectPartId{projectPartId}
        , m_qtPartId{qtPartId}
    {}

    struct Update
    {
        QStringList qtDirectories = {};
        const QString propertyEditorResourcesPath = {};
        const QStringList typeAnnotationPaths = {};
        QString projectDirectory = {};
    };

    void update(Update update);
    void pathsWithIdsChanged(const std::vector<IdPaths> &idPaths) override;
    void pathsChanged(const SourceIds &filePathIds) override;

    struct Component
    {
        Utils::SmallString fileName;
        Utils::SmallString typeName;
        ModuleId moduleId;
        int majorVersion = -1;
        int minorVersion = -1;
    };

    using Components = std::vector<Component>;

    class ComponentRange
    {
    public:
        using const_iterator = Components::const_iterator;

        ComponentRange(const_iterator begin, const_iterator end)
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

    enum class FileState { Unchanged, Changed, NotExists, NotExistsUnchanged, Added, Removed };

    struct WatchedSourceIds
    {
        WatchedSourceIds(std::size_t reserve)
        {
            directoryIds.reserve(reserve);
            qmldirSourceIds.reserve(reserve);
            qmlSourceIds.reserve(reserve * 30);
            qmltypesSourceIds.reserve(reserve * 30);
        }

        SourceIds directoryIds;
        SourceIds qmldirSourceIds;
        SourceIds qmlSourceIds;
        SourceIds qmltypesSourceIds;
    };

    struct NotUpdatedSourceIds
    {
        NotUpdatedSourceIds(std::size_t reserve)
        {
            fileStatusSourceIds.reserve(reserve * 30);
            sourceIds.reserve(reserve * 30);
        }

        SourceIds fileStatusSourceIds;
        SourceIds sourceIds;
    };

private:
    using IsInsideProject = Storage::IsInsideProject;
    void updateDirectories(const QStringList &directories,
                           Storage::Synchronization::SynchronizationPackage &package,
                           NotUpdatedSourceIds &notUpdatedSourceIds,
                           WatchedSourceIds &WatchedSourceIds);
    void updateDirectory(const Utils::PathString &directory,
                         const DirectoryPathIds &subdirecoriesToIgnore,
                         Storage::Synchronization::SynchronizationPackage &package,
                         NotUpdatedSourceIds &notUpdatedSourceIds,
                         WatchedSourceIds &WatchedSourceIds,
                         IsInsideProject isInsideProject);
    void updateSubdirectories(const Utils::PathString &directory,
                              DirectoryPathId directoryId,
                              FileState directoryFileState,
                              const DirectoryPathIds &subdirecoriesToIgnore,
                              Storage::Synchronization::SynchronizationPackage &package,
                              NotUpdatedSourceIds &notUpdatedSourceIds,
                              WatchedSourceIds &WatchedSourceIds,
                              IsInsideProject isInsideProject);
    void updateDirectoryChanged(Utils::SmallStringView directoryPath,
                                Utils::SmallStringView annotationDirectoryPath,
                                FileState qmldirState,
                                FileState annotationDirectoryState,
                                SourcePath qmldirSourcePath,
                                SourceId qmldirSourceId,
                                DirectoryPathId directoryId,
                                DirectoryPathId annotationDirectoryId,
                                Storage::Synchronization::SynchronizationPackage &package,
                                NotUpdatedSourceIds &notUpdatedSourceIds,
                                WatchedSourceIds &WatchedSourceIds,
                                IsInsideProject isInsideProject,
                                ProjectStorageTracing::Category::TracerType &tracer);
    void annotationDirectoryChanged(Utils::SmallStringView directoryPath,
                                    DirectoryPathId directoryId,
                                    DirectoryPathId annotationDirectoryId,
                                    ModuleId moduleId,
                                    Storage::Synchronization::SynchronizationPackage &package);
    void updatePropertyEditorFiles(Utils::SmallStringView directyPath,
                                   DirectoryPathId directoryId,
                                   ModuleId moduleId,
                                   Storage::Synchronization::SynchronizationPackage &package);
    void updatePropertyEditorFile(const QString &fileName,
                                  DirectoryPathId directoryId,
                                  ModuleId moduleId,
                                  Storage::Synchronization::SynchronizationPackage &package);
    void updatePropertyEditorPaths(const QString &propertyEditorResourcesPath,
                                   Storage::Synchronization::SynchronizationPackage &package,
                                   NotUpdatedSourceIds &notUpdatedSourceIds);
    void updateTypeAnnotations(const QString &directoryPath,
                               Storage::Synchronization::SynchronizationPackage &package,
                               NotUpdatedSourceIds &notUpdatedSourceIds,
                               std::map<DirectoryPathId, SmallSourceIds<16>> &updatedSourceIdsDictonary);
    void updateTypeAnnotationDirectories(
        Storage::Synchronization::SynchronizationPackage &package,
        NotUpdatedSourceIds &notUpdatedSourceIds,
        std::map<DirectoryPathId, SmallSourceIds<16>> &updatedSourceIdsDictonary);
    void updateTypeAnnotations(const QStringList &directoryPath,
                               Storage::Synchronization::SynchronizationPackage &package,
                               NotUpdatedSourceIds &notUpdatedSourceIds);
    void updateTypeAnnotation(const QString &directoryPath,
                              const QString &filePath,
                              SourceId sourceId,
                              DirectoryPathId directoryId,
                              Storage::Synchronization::SynchronizationPackage &package);
    void updatePropertyEditorPath(const QString &path,
                                  Storage::Synchronization::SynchronizationPackage &package,
                                  DirectoryPathId directoryId,
                                  long long pathOffset);
    void updatePropertyEditorFilePath(const QString &filePath,
                                      Storage::Synchronization::SynchronizationPackage &package,
                                      DirectoryPathId directoryId,
                                      long long pathOffset);
    void parseTypeInfos(const QStringList &typeInfos,
                        const std::vector<Utils::PathString> &qmldirDependencies,
                        const std::vector<Utils::PathString> &qmldirImports,
                        DirectoryPathId directoryId,
                        const QString &directoryPath,
                        ModuleId moduleId,
                        Storage::Synchronization::SynchronizationPackage &package,
                        NotUpdatedSourceIds &notUpdatedSourceIds,
                        WatchedSourceIds &WatchedSourceIds,
                        IsInsideProject isInsideProject);
    void parseDirectoryInfos(const Storage::Synchronization::DirectoryInfos &directoryInfos,
                             Storage::Synchronization::SynchronizationPackage &package,
                             NotUpdatedSourceIds &notUpdatedSourceIds,
                             WatchedSourceIds &WatchedSourceIds,
                             IsInsideProject isInsideProject);
    FileState parseTypeInfo(const Storage::Synchronization::DirectoryInfo &directoryInfo,
                            const QString &qmltypesPath,
                            Storage::Synchronization::SynchronizationPackage &package,
                            NotUpdatedSourceIds &notUpdatedSourceIds,
                            IsInsideProject isInsideProject);
    void parseQmlComponents(Components components,
                            DirectoryPathId directoryId,
                            Storage::Synchronization::SynchronizationPackage &package,
                            NotUpdatedSourceIds &notUpdatedSourceIds,
                            WatchedSourceIds &WatchedSourceIds,
                            FileState qmldirState,
                            SourceId qmldirSourceId,
                            IsInsideProject isInsideProject);
    void parseQmlComponent(Utils::SmallStringView fileName,
                           Utils::SmallStringView directory,
                           Storage::Synchronization::ExportedTypes exportedTypes,
                           DirectoryPathId directoryId,
                           Storage::Synchronization::SynchronizationPackage &package,
                           NotUpdatedSourceIds &notUpdatedSourceIds,
                           WatchedSourceIds &WatchedSourceIds,
                           FileState qmldirState,
                           SourceId qmldirSourceId,
                           IsInsideProject isInsideProject);
    void parseQmlComponent(SourceId sourceId,
                           Storage::Synchronization::SynchronizationPackage &package,
                           NotUpdatedSourceIds &notUpdatedSourceIds,
                           IsInsideProject isInsideProject);

    FileState fileState(SourceId sourceId,
                        Storage::Synchronization::SynchronizationPackage &package,
                        NotUpdatedSourceIds &notUpdatedSourceIds) const;
    FileState fileState(DirectoryPathId directoryPathId,
                        Storage::Synchronization::SynchronizationPackage &package,
                        NotUpdatedSourceIds &notUpdatedSourceIds) const;

private:
    std::vector<IdPaths> m_changedIdPaths;
    FileSystemInterface &m_fileSystem;
    ProjectStorageType &m_projectStorage;
    FileStatusCache &m_fileStatusCache;
    PathCacheType &m_pathCache;
    QmlDocumentParserInterface &m_qmlDocumentParser;
    QmlTypesParserInterface &m_qmlTypesParser;
    ProjectStoragePathWatcherInterface &m_pathWatcher;
    ProjectStorageErrorNotifierInterface &m_errorNotifier;
    ProjectPartId m_projectPartId;
    ProjectPartId m_qtPartId;
};

} // namespace QmlDesigner
