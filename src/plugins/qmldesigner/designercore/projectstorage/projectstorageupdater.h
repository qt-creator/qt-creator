// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "filestatus.h"
#include "nonlockingmutex.h"
#include "projectstorageids.h"
#include "projectstoragepathwatchertypes.h"
#include "projectstoragetypes.h"

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

class ProjectStorageUpdater
{
public:
    using PathCache = SourcePathCache<ProjectStorage<Sqlite::Database>, NonLockingMutex>;

    ProjectStorageUpdater(FileSystemInterface &fileSystem,
                          ProjectStorageInterface &projectStorage,
                          FileStatusCache &fileStatusCache,
                          PathCache &pathCache,
                          QmlDocumentParserInterface &qmlDocumentParser,
                          QmlTypesParserInterface &qmlTypesParser)
        : m_fileSystem{fileSystem}
        , m_projectStorage{projectStorage}
        , m_fileStatusCache{fileStatusCache}
        , m_pathCache{pathCache}
        , m_qmlDocumentParser{qmlDocumentParser}
        , m_qmlTypesParser{qmlTypesParser}
    {}

    void update(QStringList directories, QStringList qmlTypesPaths);
    void pathsWithIdsChanged(const std::vector<IdPaths> &idPaths);

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

private:

    void updateQmlTypes(const QStringList &qmlTypesPaths,
                        Storage::Synchronization::SynchronizationPackage &package,
                        SourceIds &notUpdatedFileStatusSourceIds,
                        SourceIds &notUpdatedSourceIds);
    void updateDirectories(const QStringList &directories,
                           Storage::Synchronization::SynchronizationPackage &package,
                           SourceIds &notUpdatedFileStatusSourceIds,
                           SourceIds &notUpdatedSourceIds);

    void parseTypeInfos(const QStringList &typeInfos,
                        const QList<QmlDirParser::Import> &qmldirDependencies,
                        const QList<QmlDirParser::Import> &qmldirImports,
                        SourceId qmldirSourceId,
                        Utils::SmallStringView directoryPath,
                        ModuleId moduleId,
                        Storage::Synchronization::SynchronizationPackage &package,
                        SourceIds &notUpdatedFileStatusSourceIds,
                        SourceIds &notUpdatedSourceIds);
    void parseProjectDatas(const Storage::Synchronization::ProjectDatas &projectDatas,
                           Storage::Synchronization::SynchronizationPackage &package,
                           SourceIds &notUpdatedFileStatusSourceIds,
                           SourceIds &notUpdatedSourceIds,
                           Utils::SmallStringView directoryPath);
    FileState parseTypeInfo(const Storage::Synchronization::ProjectData &projectData,
                            Utils::SmallStringView qmltypesPath,
                            Storage::Synchronization::SynchronizationPackage &package,
                            SourceIds &notUpdatedFileStatusSourceIds,
                            SourceIds &notUpdatedSourceIds);
    void parseQmlComponents(Components components,
                            SourceId qmldirSourceId,
                            SourceContextId directoryId,
                            Storage::Synchronization::SynchronizationPackage &package,
                            SourceIds &notUpdatedFileStatusSourceIds);
    void parseQmlComponent(Utils::SmallStringView fileName,
                           Utils::SmallStringView directory,
                           Storage::Synchronization::ExportedTypes exportedTypes,
                           SourceId qmldirSourceId,
                           Storage::Synchronization::SynchronizationPackage &package,
                           SourceIds &notUpdatedFileStatusSourceIds);
    void parseQmlComponent(Utils::SmallStringView fileName,
                           Utils::SmallStringView filePath,
                           Utils::SmallStringView directoryPath,
                           SourceId sourceId,
                           Storage::Synchronization::SynchronizationPackage &package,
                           SourceIds &notUpdatedFileStatusSourceIds);

    FileState fileState(SourceId sourceId,
                        FileStatuses &fileStatuses,
                        SourceIds &updatedSourceIds,
                        SourceIds &notUpdatedSourceIds) const;

private:
    FileSystemInterface &m_fileSystem;
    ProjectStorageInterface &m_projectStorage;
    FileStatusCache &m_fileStatusCache;
    PathCache &m_pathCache;
    QmlDocumentParserInterface &m_qmlDocumentParser;
    QmlTypesParserInterface &m_qmlTypesParser;
};

} // namespace QmlDesigner
