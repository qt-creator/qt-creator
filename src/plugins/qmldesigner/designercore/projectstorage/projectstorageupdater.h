// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

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

using ComponentReference = std::reference_wrapper<const QmlDirParser::Component>;
using ComponentReferences = std::vector<ComponentReference>;

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

    void update(QStringList qmlDirs, QStringList qmlTypesPaths);
    void pathsWithIdsChanged(const std::vector<IdPaths> &idPaths);

private:
    enum class FileState {
        NotChanged,
        Changed,
        NotExists,
    };

    void updateQmlTypes(const QStringList &qmlTypesPaths,
                        Storage::Synchronization::SynchronizationPackage &package,
                        SourceIds &notUpdatedFileStatusSourceIds,
                        SourceIds &notUpdatedSourceIds);
    void updateQmldirs(const QStringList &qmlDirs,
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
    void parseTypeInfos(const Storage::Synchronization::ProjectDatas &projectDatas,
                        Storage::Synchronization::SynchronizationPackage &package,
                        SourceIds &notUpdatedFileStatusSourceIds,
                        SourceIds &notUpdatedSourceIds);
    FileState parseTypeInfo(const Storage::Synchronization::ProjectData &projectData,
                            Utils::SmallStringView qmltypesPath,
                            Storage::Synchronization::SynchronizationPackage &package,
                            SourceIds &notUpdatedFileStatusSourceIds,
                            SourceIds &notUpdatedSourceIds);
    void parseQmlComponents(ComponentReferences components,
                            SourceId qmldirSourceId,
                            SourceContextId directoryId,
                            ModuleId moduleId,
                            ModuleId pathModuleId,
                            Storage::Synchronization::SynchronizationPackage &package,
                            SourceIds &notUpdatedFileStatusSourceIds);
    void parseQmlComponents(const Storage::Synchronization::ProjectDatas &projectDatas,
                            Storage::Synchronization::SynchronizationPackage &package,
                            SourceIds &notUpdatedFileStatusSourceIds,
                            Utils::SmallStringView directoryPath);
    void parseQmlComponent(Utils::SmallStringView fileName,
                           Utils::SmallStringView directory,
                           Storage::Synchronization::ExportedTypes exportedTypes,
                           ModuleId moduleId,
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
