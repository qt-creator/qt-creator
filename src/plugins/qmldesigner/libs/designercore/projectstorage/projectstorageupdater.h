// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "filestatus.h"
#include "projectstorageerrornotifierinterface.h"
#include "projectstorageids.h"
#include "projectstoragepathwatchernotifierinterface.h"
#include "projectstoragepathwatchertypes.h"
#include "projectstoragetracing.h"
#include "projectstoragetypes.h"
#include "sourcepathstorage/nonlockingmutex.h"
#include "sourcepathstorage/sourcepath.h"

#include <modelfwd.h>
#include <modulesstorage/modulesstorage.h>

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
                          ModulesStorage &modulesStorage,
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
        , m_modulesStorage{modulesStorage}
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
        }

        SourceIds fileStatusSourceIds;
    };

    struct ProjectChunkSourceIds
    {
        ProjectChunkSourceIds()
        {
            directory.reserve(32);
            qmldir.reserve(32);
            qmlDocument.reserve(128);
            qmltypes.reserve(32);
        }

        void clear()
        {
            directory.clear();
            qmldir.clear();
            qmlDocument.clear();
            qmltypes.clear();
        }

        SourceIds directory;
        SourceIds qmldir;
        SourceIds qmlDocument;
        SourceIds qmltypes;
    };

private:
    using ParsedTypeInfoSourceIds = SmallSourceIds<1024>;
    using IsInsideProject = Storage::IsInsideProject;
    void updateDirectories(const QStringList &directories,
                           Storage::Synchronization::SynchronizationPackage &package,
                           NotUpdatedSourceIds &notUpdatedSourceIds,
                           WatchedSourceIds &watchedSourceIds,
                           ParsedTypeInfoSourceIds &parsedTypeInfos);
    void updateDirectory(Utils::SmallStringView,
                         const DirectoryPathIds &subdirecoriesToIgnore,
                         Storage::Synchronization::SynchronizationPackage &package,
                         NotUpdatedSourceIds &notUpdatedSourceIds,
                         WatchedSourceIds &watchedSourceIds,
                         ParsedTypeInfoSourceIds &parsedTypeInfos,
                         IsInsideProject isInsideProject);
    void updateQmldir(DirectoryPathId directoryId,
                      Utils::SmallStringView directoryPath,
                      Storage::Synchronization::SynchronizationPackage &package,
                      NotUpdatedSourceIds &notUpdatedSourceIds,
                      WatchedSourceIds &watchedSourceIds,
                      ParsedTypeInfoSourceIds &parsedTypeInfos,
                      IsInsideProject isInsideProject);
    void updateSubdirectories(Utils::SmallStringView directory,
                              DirectoryPathId directoryId,
                              FileState directoryFileState,
                              const DirectoryPathIds &subdirecoriesToIgnore,
                              Storage::Synchronization::SynchronizationPackage &package,
                              NotUpdatedSourceIds &notUpdatedSourceIds,
                              WatchedSourceIds &watchedSourceIds,
                              ParsedTypeInfoSourceIds &parsedTypeInfos,
                              IsInsideProject isInsideProject);
    void updateDirectoryChanged(Utils::SmallStringView directoryPath,
                                FileState directoryState,
                                DirectoryPathId directoryId,
                                Storage::Synchronization::SynchronizationPackage &package,
                                NotUpdatedSourceIds &notUpdatedSourceIds,
                                WatchedSourceIds &watchedSourceIds,
                                IsInsideProject isInsideProject,
                                NanotraceHR::Tracer<ProjectStorageTracing::Category> &tracer);
    void updateQmldirChanged(const Storage::Synchronization::ProjectEntryInfos &qmltypesProjectEntryInfos,
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
                             IsInsideProject isInsideProject);
    void updateAnnotationDirectory(Utils::SmallStringView directoryPath,
                                   ModuleId moduleId,
                                   DirectoryPathId directoryId,
                                   Storage::Synchronization::SynchronizationPackage &package,
                                   NotUpdatedSourceIds &notUpdatedSourceIds,
                                   IsInsideProject isInsideProject);
    void removeAnnotationDirectory(Utils::SmallStringView directoryPath,
                                   Storage::Synchronization::SynchronizationPackage &package,
                                   NotUpdatedSourceIds &notUpdatedSourceIds,
                                   IsInsideProject isInsideProject);
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
                        SourceId qmldirSourceId,
                        const QString &directoryPath,
                        FileState qmldirState,
                        ModuleId moduleId,
                        Storage::Synchronization::SynchronizationPackage &package,
                        NotUpdatedSourceIds &notUpdatedSourceIds,
                        WatchedSourceIds &watchedSourceIds,
                        ParsedTypeInfoSourceIds &parsedTypeInfos,
                        IsInsideProject isInsideProject);
    void parseProjectEntryInfos(const Storage::Synchronization::ProjectEntryInfos &projectEntryInfos,
                                Storage::Synchronization::SynchronizationPackage &package,
                                NotUpdatedSourceIds &notUpdatedSourceIds,
                                WatchedSourceIds &watchedSourceIds,
                                ParsedTypeInfoSourceIds &parsedTypeInfos,
                                IsInsideProject isInsideProject);

    void parseQmltypesProjectEntryInfos(const Storage::Synchronization::ProjectEntryInfos &projectEntryInfos,
                                        Storage::Synchronization::SynchronizationPackage &package,
                                        NotUpdatedSourceIds &notUpdatedSourceIds,
                                        WatchedSourceIds &watchedSourceIds,
                                        ParsedTypeInfoSourceIds &parsedTypeInfos,
                                        IsInsideProject isInsideProject);
    void parseTypeInfo(const Storage::Synchronization::ProjectEntryInfo &projectEntryInfo,
                       const QString &qmltypesPath,
                       Storage::Synchronization::SynchronizationPackage &package,
                       NotUpdatedSourceIds &notUpdatedSourceIds,
                       ParsedTypeInfoSourceIds &parsedTypeInfos,
                       IsInsideProject isInsideProject);
    void parseQmlDocuments(const QStringList &qmlFileNames,
                           Utils::SmallStringView directory,
                           DirectoryPathId directoryId,
                           Storage::Synchronization::SynchronizationPackage &package,
                           NotUpdatedSourceIds &notUpdatedSourceIds,
                           WatchedSourceIds &watchedSourceIds,
                           FileState directoryState,
                           IsInsideProject isInsideProject);
    void parseQmlDocument(const QString &qmlFileName,
                          Utils::SmallStringView directory,
                          DirectoryPathId directoryId,
                          Storage::Synchronization::SynchronizationPackage &package,
                          NotUpdatedSourceIds &notUpdatedSourceIds,
                          WatchedSourceIds &watchedSourceIds,
                          FileState directoryState,
                          IsInsideProject isInsideProject);
    void parseQmlDocument(SourceId sourceId,
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
    ProjectChunkSourceIds m_qtSourceIds;
    ProjectChunkSourceIds m_projectSourceIds;
    FileSystemInterface &m_fileSystem;
    ProjectStorageType &m_projectStorage;
    FileStatusCache &m_fileStatusCache;
    PathCacheType &m_pathCache;
    ModulesStorage &m_modulesStorage;
    QmlDocumentParserInterface &m_qmlDocumentParser;
    QmlTypesParserInterface &m_qmlTypesParser;
    ProjectStoragePathWatcherInterface &m_pathWatcher;
    ProjectStorageErrorNotifierInterface &m_errorNotifier;
    ProjectPartId m_projectPartId;
    ProjectPartId m_qtPartId;
};

} // namespace QmlDesigner
