// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "filestatus.h"
#include "projectstorageerrornotifier.h"
#include "projectstorageids.h"
#include "projectstoragepathwatchernotifierinterface.h"
#include "projectstoragepathwatchertypes.h"
#include "projectstoragetypes.h"
#include "sourcepathstorage/nonlockingmutex.h"
#include "sourcepathstorage/sourcepath.h"

#include <modelfwd.h>

#include <tracing/qmldesignertracing.h>

#include <qmljs/parser/qmldirparser_p.h>

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
                          ProjectPartId projectPartId)
        : m_fileSystem{fileSystem}
        , m_projectStorage{projectStorage}
        , m_fileStatusCache{fileStatusCache}
        , m_pathCache{pathCache}
        , m_qmlDocumentParser{qmlDocumentParser}
        , m_qmlTypesParser{qmlTypesParser}
        , m_pathWatcher{pathWatcher}
        , m_errorNotifier{errorNotifier}
        , m_projectPartId{projectPartId}
    {}

    struct Update
    {
        QStringList directories = {};
        QStringList qmlTypesPaths = {};
        const QString propertyEditorResourcesPath = {};
        const QStringList typeAnnotationPaths = {};
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

    enum class FileState {
        NotChanged,
        Changed,
        NotExists,
    };

    struct WatchedSourceIdsIds
    {
        WatchedSourceIdsIds(std::size_t reserve)
        {
            directorySourceIds.reserve(reserve);
            qmldirSourceIds.reserve(reserve);
            qmlSourceIds.reserve(reserve * 30);
            qmltypesSourceIds.reserve(reserve * 30);
        }

        SourceIds directorySourceIds;
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
    void updateQmlTypes(const QStringList &qmlTypesPaths,
                        Storage::Synchronization::SynchronizationPackage &package,
                        NotUpdatedSourceIds &notUpdatedSourceIds,
                        WatchedSourceIdsIds &watchedSourceIdsIds);

    void updateDirectories(const QStringList &directories,
                           Storage::Synchronization::SynchronizationPackage &package,
                           NotUpdatedSourceIds &notUpdatedSourceIds,
                           WatchedSourceIdsIds &watchedSourceIdsIds);

    void updateDirectory(const Utils::PathString &directory,
                         const SourceContextIds &subdirecoriesToIgnore,
                         Storage::Synchronization::SynchronizationPackage &package,
                         NotUpdatedSourceIds &notUpdatedSourceIds,
                         WatchedSourceIdsIds &watchedSourceIdsIds);
    void updateSubdirectories(const Utils::PathString &directory,
                              SourceId directorySourceId,
                              FileState directoryFileState,
                              const SourceContextIds &subdirecoriesToIgnore,
                              Storage::Synchronization::SynchronizationPackage &package,
                              NotUpdatedSourceIds &notUpdatedSourceIds,
                              WatchedSourceIdsIds &watchedSourceIdsIds);
    void updateDirectoryChanged(std::string_view directoryPath,
                                FileState qmldirState,
                                SourcePath qmldirSourcePath,
                                SourceId qmldirSourceId,
                                SourceId directorySourceId,
                                SourceContextId directoryId,
                                Storage::Synchronization::SynchronizationPackage &package,
                                NotUpdatedSourceIds &notUpdatedSourceIds,
                                WatchedSourceIdsIds &watchedSourceIdsIds,
                                ProjectStorageTracing::Category::TracerType &tracer);

    void updatePropertyEditorPaths(const QString &propertyEditorResourcesPath,
                                   Storage::Synchronization::SynchronizationPackage &package,
                                   NotUpdatedSourceIds &notUpdatedSourceIds);
    void updateTypeAnnotations(const QString &directoryPath,
                               Storage::Synchronization::SynchronizationPackage &package,
                               NotUpdatedSourceIds &notUpdatedSourceIds,
                               std::map<SourceId, SmallSourceIds<16>> &updatedSourceIdsDictonary);
    void updateTypeAnnotationDirectories(Storage::Synchronization::SynchronizationPackage &package,
                                         NotUpdatedSourceIds &notUpdatedSourceIds,
                                         std::map<SourceId, SmallSourceIds<16>> &updatedSourceIdsDictonary);
    void updateTypeAnnotations(const QStringList &directoryPath,
                               Storage::Synchronization::SynchronizationPackage &package,
                               NotUpdatedSourceIds &notUpdatedSourceIds);
    void updateTypeAnnotation(const QString &directoryPath,
                              const QString &filePath,
                              SourceId sourceId,
                              SourceId directorySourceId,
                              Storage::Synchronization::SynchronizationPackage &package);
    void updatePropertyEditorPath(const QString &path,
                                  Storage::Synchronization::SynchronizationPackage &package,
                                  SourceId directorySourceId,
                                  long long pathOffset);
    void updatePropertyEditorFilePath(const QString &filePath,
                                      Storage::Synchronization::SynchronizationPackage &package,
                                      SourceId directorySourceId,
                                      long long pathOffset);
    void parseTypeInfos(const QStringList &typeInfos,
                        const QList<QmlDirParser::Import> &qmldirDependencies,
                        const QList<QmlDirParser::Import> &qmldirImports,
                        SourceId directorySourceId,
                        Utils::SmallStringView directoryPath,
                        ModuleId moduleId,
                        Storage::Synchronization::SynchronizationPackage &package,
                        NotUpdatedSourceIds &notUpdatedSourceIds,
                        WatchedSourceIdsIds &watchedSourceIdsIds);
    void parseDirectoryInfos(const Storage::Synchronization::DirectoryInfos &directoryInfos,
                             Storage::Synchronization::SynchronizationPackage &package,
                             NotUpdatedSourceIds &notUpdatedSourceIds,
                             WatchedSourceIdsIds &watchedSourceIdsIds);
    FileState parseTypeInfo(const Storage::Synchronization::DirectoryInfo &directoryInfo,
                            Utils::SmallStringView qmltypesPath,
                            Storage::Synchronization::SynchronizationPackage &package,
                            NotUpdatedSourceIds &notUpdatedSourceIds);
    void parseQmlComponents(Components components,
                            SourceId directorySourceId,
                            SourceContextId directoryId,
                            Storage::Synchronization::SynchronizationPackage &package,
                            NotUpdatedSourceIds &notUpdatedSourceIds,
                            WatchedSourceIdsIds &watchedSourceIdsIds,
                            FileState qmldirState,
                            SourceId qmldirSourceId);
    void parseQmlComponent(Utils::SmallStringView fileName,
                           Utils::SmallStringView directory,
                           Storage::Synchronization::ExportedTypes exportedTypes,
                           SourceId directorySourceId,
                           Storage::Synchronization::SynchronizationPackage &package,
                           NotUpdatedSourceIds &notUpdatedSourceIds,
                           WatchedSourceIdsIds &watchedSourceIdsIds,
                           FileState qmldirState,
                           SourceId qmldirSourceId);
    void parseQmlComponent(SourceId sourceId,
                           Storage::Synchronization::SynchronizationPackage &package,
                           NotUpdatedSourceIds &notUpdatedSourceIds);

    FileState fileState(SourceId sourceId,
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
};

} // namespace QmlDesigner
