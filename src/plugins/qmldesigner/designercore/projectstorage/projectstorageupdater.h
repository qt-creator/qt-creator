// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "filestatus.h"
#include "nonlockingmutex.h"
#include "projectstorageids.h"
#include "projectstoragepathwatchernotifierinterface.h"
#include "projectstoragepathwatchertypes.h"
#include "projectstoragetypes.h"
#include "sourcepath.h"

#include <qmljs/parser/qmldirparser_p.h>

#include <QStringList>

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
template<typename Database>
class ProjectStorage;
class QmlDocumentParserInterface;
class QmlTypesParserInterface;

class ProjectStorageUpdater final : public ProjectStoragePathWatcherNotifierInterface
{
public:
    using PathCache = SourcePathCache<ProjectStorage<Sqlite::Database>, NonLockingMutex>;

    ProjectStorageUpdater(FileSystemInterface &fileSystem,
                          ProjectStorageInterface &projectStorage,
                          FileStatusCache &fileStatusCache,
                          PathCache &pathCache,
                          QmlDocumentParserInterface &qmlDocumentParser,
                          QmlTypesParserInterface &qmlTypesParser,
                          class ProjectStoragePathWatcherInterface &pathWatcher,
                          ProjectPartId projectPartId)
        : m_fileSystem{fileSystem}
        , m_projectStorage{projectStorage}
        , m_fileStatusCache{fileStatusCache}
        , m_pathCache{pathCache}
        , m_qmlDocumentParser{qmlDocumentParser}
        , m_qmlTypesParser{qmlTypesParser}
        , m_pathWatcher{pathWatcher}
        , m_projectPartId{projectPartId}
    {}

    void update(QStringList directories,
                QStringList qmlTypesPaths,
                const QString &propertyEditorResourcesPath);
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
                         Storage::Synchronization::SynchronizationPackage &package,
                         NotUpdatedSourceIds &notUpdatedSourceIds,
                         WatchedSourceIdsIds &watchedSourceIdsIds);

    void updatePropertyEditorPaths(const QString &propertyEditorResourcesPath,
                                   Storage::Synchronization::SynchronizationPackage &package,
                                   NotUpdatedSourceIds &notUpdatedSourceIds);
    void updatePropertyEditorPath(const QString &path,
                                  Storage::Synchronization::SynchronizationPackage &package,
                                  SourceId directorySourceId);
    void updatePropertyEditorFilePath(const QString &filePath,
                                      Storage::Synchronization::SynchronizationPackage &package,
                                      SourceId directorySourceId);
    void parseTypeInfos(const QStringList &typeInfos,
                        const QList<QmlDirParser::Import> &qmldirDependencies,
                        const QList<QmlDirParser::Import> &qmldirImports,
                        SourceId directorySourceId,
                        Utils::SmallStringView directoryPath,
                        ModuleId moduleId,
                        Storage::Synchronization::SynchronizationPackage &package,
                        NotUpdatedSourceIds &notUpdatedSourceIds,
                        WatchedSourceIdsIds &watchedSourceIdsIds);
    void parseProjectDatas(const Storage::Synchronization::ProjectDatas &projectDatas,
                           Storage::Synchronization::SynchronizationPackage &package,
                           NotUpdatedSourceIds &notUpdatedSourceIds,
                           WatchedSourceIdsIds &watchedSourceIdsIds);
    FileState parseTypeInfo(const Storage::Synchronization::ProjectData &projectData,
                            Utils::SmallStringView qmltypesPath,
                            Storage::Synchronization::SynchronizationPackage &package,
                            NotUpdatedSourceIds &notUpdatedSourceIds);
    void parseQmlComponents(Components components,
                            SourceId directorySourceId,
                            SourceContextId directoryId,
                            Storage::Synchronization::SynchronizationPackage &package,
                            NotUpdatedSourceIds &notUpdatedSourceIds,
                            WatchedSourceIdsIds &watchedSourceIdsIds,
                            FileState qmldirState);
    void parseQmlComponent(Utils::SmallStringView fileName,
                           Utils::SmallStringView directory,
                           Storage::Synchronization::ExportedTypes exportedTypes,
                           SourceId directorySourceId,
                           Storage::Synchronization::SynchronizationPackage &package,
                           NotUpdatedSourceIds &notUpdatedSourceIds,
                           WatchedSourceIdsIds &watchedSourceIdsIds,
                           FileState qmldirState);
    void parseQmlComponent(SourceId sourceId,
                           Storage::Synchronization::SynchronizationPackage &package,
                           NotUpdatedSourceIds &notUpdatedSourceIds);

    FileState fileState(SourceId sourceId,
                        Storage::Synchronization::SynchronizationPackage &package,
                        NotUpdatedSourceIds &notUpdatedSourceIds) const;

private:
    std::vector<IdPaths> m_changedIdPaths;
    FileSystemInterface &m_fileSystem;
    ProjectStorageInterface &m_projectStorage;
    FileStatusCache &m_fileStatusCache;
    PathCache &m_pathCache;
    QmlDocumentParserInterface &m_qmlDocumentParser;
    QmlTypesParserInterface &m_qmlTypesParser;
    ProjectStoragePathWatcherInterface &m_pathWatcher;
    ProjectPartId m_projectPartId;
};

} // namespace QmlDesigner
