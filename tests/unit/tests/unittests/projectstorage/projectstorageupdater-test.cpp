// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <filesystemmock.h>
#include <projectstorageerrornotifiermock.h>
#include <projectstoragemock.h>
#include <projectstoragepathwatchermock.h>
#include <qmldocumentparsermock.h>
#include <qmltypesparsermock.h>
#include <version-matcher.h>

#include <projectstorage-matcher.h>

#include <projectstorage/filestatuscache.h>
#include <projectstorage/projectstorage.h>
#include <projectstorage/projectstorageupdater.h>
#include <sourcepathstorage/sourcepathcache.h>
#include <sqlitedatabase.h>

namespace QmlDesigner {

static std::string toString(ProjectStorageUpdater::FileState state)
{
    using FileState = ProjectStorageUpdater::FileState;
    switch (state) {
    case FileState::Added:
        return "added";
    case FileState::Changed:
        return "changed";
    case FileState::Removed:
        return "removed";
    case FileState::NotExists:
        return "not_exists";
    case FileState::NotExistsUnchanged:
        return "not_exists_unchanged";
    case FileState::Unchanged:
        return "unchanged";
    }

    return "";
}

[[maybe_unused]] static std::ostream &operator<<(std::ostream &out,
                                                 ProjectStorageUpdater::FileState state)
{
    return out << toString(state);
}

[[maybe_unused]] static std::ostream &operator<<(std::ostream &out,
                                                 ProjectStorageUpdater::Update update)
{
    return out << "(" << update.projectDirectory << "," << update.qtDirectories << ","
               << update.propertyEditorResourcesPath << "," << update.typeAnnotationPaths << ")";
}
} // namespace QmlDesigner

namespace {

using namespace Qt::StringLiterals;

namespace Storage = QmlDesigner::Storage;

using QmlDesigner::DirectoryPathId;
using QmlDesigner::FileNameId;
using QmlDesigner::FileStatus;
using QmlDesigner::IdPaths;
using QmlDesigner::ModuleId;
using QmlDesigner::SourceId;
using QmlDesigner::SourceIds;
using QmlDesigner::SourceType;
using Storage::Import;
using Storage::IsInsideProject;
using Storage::ModuleKind;
using Storage::Synchronization::ChangeLevel;
using Storage::Synchronization::ProjectEntryInfo;
using Storage::Synchronization::ProjectEntryInfos;
using Storage::Synchronization::ExportedType;
using Storage::Synchronization::FileType;
using Storage::Synchronization::ImportedType;
using Storage::Synchronization::ImportedTypeName;
using Storage::Synchronization::IsAutoVersion;
using Storage::Synchronization::ModuleExportedImport;
using Storage::Synchronization::PropertyDeclaration;
using Storage::Synchronization::PropertyEditorQmlPath;
using Storage::Synchronization::SynchronizationPackage;
using Storage::Synchronization::Type;
using Storage::TypeTraits;
using Storage::TypeTraitsKind;
using Storage::Version;
using Storage::VersionNumber;
using FileState = QmlDesigner::ProjectStorageUpdater::FileState;
using Update = QmlDesigner::ProjectStorageUpdater::Update;
using IsInsideProject = QmlDesigner::QmlDocumentParserInterface::IsInsideProject;

MATCHER_P4(IsStorageType,
           typeName,
           prototype,
           traits,
           sourceId,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Type(typeName, prototype, traits, sourceId)))
{
    const Type &type = arg;

    return type.typeName == typeName && type.traits == traits && type.sourceId == sourceId
           && ImportedTypeName{prototype} == type.prototype;
}

MATCHER_P3(IsPropertyDeclaration,
           name,
           typeName,
           traits,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(PropertyDeclaration{name, typeName, traits}))
{
    const PropertyDeclaration &propertyDeclaration = arg;

    return propertyDeclaration.name == name
           && ImportedTypeName{typeName} == propertyDeclaration.typeName
           && propertyDeclaration.traits == traits;
}

MATCHER_P7(IsExportedType,
           moduleId,
           name,
           majorVersion,
           minorVersion,
           contextSourceId,
           typeIdName,
           typeSourceId,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(ExportedType{contextSourceId,
                                            moduleId,
                                            name,
                                            Storage::Version{majorVersion, minorVersion},
                                            typeSourceId,
                                            typeIdName}))
{
    const ExportedType &type = arg;

    return type.moduleId == moduleId && type.name == name
           && type.version == Storage::Version{majorVersion, minorVersion}
           && type.contextSourceId == contextSourceId && type.typeIdName == typeIdName
           && type.typeSourceId == typeSourceId;
}

MATCHER_P5(IsExportedType,
           moduleId,
           name,
           contextSourceId,
           typeIdName,
           typeSourceId,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(ExportedType{
                   contextSourceId, moduleId, name, Storage::Version{}, typeSourceId, typeIdName}))
{
    const ExportedType &type = arg;

    return type.moduleId == moduleId && type.name == name && type.version == Storage::Version{}
           && type.contextSourceId == contextSourceId && type.typeIdName == typeIdName
           && type.typeSourceId == typeSourceId;
}

auto IsFileStatus(SourceId sourceIdmatcher, long long size, long long lastModified)
{
    using file_time_type = std::filesystem::file_time_type;

    return AllOf(Field("FileStatus::sourceId", &FileStatus::sourceId, sourceIdmatcher),
                 Field("FileStatus::size", &FileStatus::size, size),
                 Field("FileStatus::lastModified",
                       &FileStatus::lastModified,
                       file_time_type{file_time_type::duration{lastModified}}));
}

auto IsNullFileStatus(const auto &sourceIdmatcher)
{
    return AllOf(Field("FileStatus::sourceId", &FileStatus::sourceId, sourceIdmatcher),
                 Field("FileStatus::size", &FileStatus::size, -1),
                 Field("FileStatus::lastModified", &FileStatus::lastModified, FileStatus::null));
}

MATCHER_P4(IsProjectEntryInfo,
           contextSourceId,
           sourceId,
           moduleId,
           fileType,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(ProjectEntryInfo{contextSourceId, sourceId, moduleId, fileType}))
{
    const ProjectEntryInfo &projectEntryInfo = arg;

    return compareInvalidAreTrue(projectEntryInfo.contextSourceId, contextSourceId)
           && projectEntryInfo.sourceId == sourceId
           && compareInvalidAreTrue(projectEntryInfo.moduleId, moduleId)
           && projectEntryInfo.fileType == fileType;
}

MATCHER(PackageIsEmpty, std::string(negation ? "isn't empty" : "is empty"))
{
    const SynchronizationPackage &package = arg;

    return package.imports.empty() && package.updatedImportSourceIds.empty()
           && package.exportedTypes.empty() && package.updatedExportedTypeSourceIds.empty()
           && package.types.empty() && package.updatedTypeSourceIds.empty()
           && package.fileStatuses.empty() && package.projectEntryInfos.empty()
           && package.updatedFileStatusSourceIds.empty()
           && package.updatedProjectEntryInfoSourceIds.empty() && package.moduleDependencies.empty()
           && package.updatedModuleDependencySourceIds.empty()
           && package.moduleExportedImports.empty() && package.updatedModuleIds.empty()
           && package.propertyEditorQmlPaths.empty()
           && package.updatedPropertyEditorQmlPathDirectoryIds.empty()
           && package.typeAnnotations.empty() && package.updatedTypeAnnotationSourceIds.empty();
}

template<typename ModuleIdMatcher, typename TypeNameMatcher, typename SourceIdMatcher, typename DirectoryPathIdMatcher>
auto IsPropertyEditorQmlPath(const ModuleIdMatcher &moduleIdMatcher,
                             const TypeNameMatcher &typeNameMatcher,
                             const SourceIdMatcher &pathIdMatcher,
                             const DirectoryPathIdMatcher &directoryIdMatcher)
{
    return AllOf(
        Field("PropertyEditorQmlPath::moduleId", &PropertyEditorQmlPath::moduleId, moduleIdMatcher),
        Field("PropertyEditorQmlPath::typeName", &PropertyEditorQmlPath::typeName, typeNameMatcher),
        Field("PropertyEditorQmlPath::pathId", &PropertyEditorQmlPath::pathId, pathIdMatcher),
        Field("PropertyEditorQmlPath::directoryId",
              &PropertyEditorQmlPath::directoryId,
              directoryIdMatcher));
}

constexpr QmlDesigner::ProjectPartId projectPartId = QmlDesigner::ProjectPartId::create(1);
constexpr QmlDesigner::ProjectPartId otherProjectPartId = QmlDesigner::ProjectPartId::create(2);
constexpr QmlDesigner::ProjectPartId qtPartId = QmlDesigner::ProjectPartId::create(10);

template<typename SourceIdMatcher>
auto IsIdPaths(QmlDesigner::ProjectPartId projectPartId,
               SourceType sourceType,
               const SourceIdMatcher &sourceIdMatcher)
{
    using QmlDesigner::ProjectChunkId;
    return AllOf(
        Field("IdPaths::id",
              &IdPaths::id,
              AllOf(Field("ProjectChunkId::id", &ProjectChunkId::id, projectPartId),
                    Field("ProjectChunkId::sourceType", &ProjectChunkId::sourceType, sourceType))),
        Field("IdPaths::sourceIds", &IdPaths::sourceIds, sourceIdMatcher));
}

auto IsImport(const auto &moduleIdMatcher,
              const auto &versionMatcher,
              const auto &sourceIdMatcher,
              const auto &contextSourceIdMatcher)
{
    using QmlDesigner::Storage::Import;
    return AllOf(Field("Import::sourceId", &Import::sourceId, sourceIdMatcher),
                 Field("Import::moduleId", &Import::moduleId, moduleIdMatcher),
                 Field("Import::version", &Import::version, versionMatcher),
                 Field("Import::contextSourceId", &Import::contextSourceId, contextSourceIdMatcher));
}

class BaseProjectStorageUpdater : public testing::Test
{
public:
    struct StaticData
    {
        Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
        Sqlite::Database sourcePathDatabase{":memory:", Sqlite::JournalMode::Memory};
        Sqlite::Database modulesDatabase{":memory:", Sqlite::JournalMode::Memory};
        QmlDesigner::ModulesStorage modulesStorage{modulesDatabase, modulesDatabase.isInitialized()};
        NiceMock<ProjectStorageErrorNotifierMock> errorNotifierMock;
        QmlDesigner::ProjectStorage storage{database,
                                            errorNotifierMock,
                                            modulesStorage,
                                            database.isInitialized()};
        QmlDesigner::SourcePathStorage sourcePathStorage{sourcePathDatabase,
                                                         sourcePathDatabase.isInitialized()};
    };

    static void SetUpTestSuite() { staticData = std::make_unique<StaticData>(); }

    static void TearDownTestSuite() { staticData.reset(); }

    BaseProjectStorageUpdater() = default;

    ~BaseProjectStorageUpdater()
    {
        storage.resetForTestsOnly();
    }

    void setFilesUnchanged(const QmlDesigner::SourceIds &sourceIds)
    {
        for (auto sourceId : sourceIds) {
            ON_CALL(fileSystemMock, fileStatus(Eq(sourceId)))
                .WillByDefault(Return(createFileStatus(sourceId, 2, 421)));
            ON_CALL(projectStorageMock, fetchFileStatus(Eq(sourceId)))
                .WillByDefault(Return(createFileStatus(sourceId, 2, 421)));
        }
    }

    void setFilesChanged(const QmlDesigner::SourceIds &sourceIds)
    {
        for (auto sourceId : sourceIds) {
            ON_CALL(fileSystemMock, fileStatus(Eq(sourceId)))
                .WillByDefault(Return(createFileStatus(sourceId, 1, 21)));
            ON_CALL(projectStorageMock, fetchFileStatus(Eq(sourceId)))
                .WillByDefault(Return(createFileStatus(sourceId, 2, 421)));
        }
    }

    void setFilesAdded(const QmlDesigner::SourceIds &sourceIds)
    {
        for (auto sourceId : sourceIds) {
            ON_CALL(fileSystemMock, fileStatus(Eq(sourceId)))
                .WillByDefault(Return(createFileStatus(sourceId, 1, 21)));
            ON_CALL(projectStorageMock, fetchFileStatus(Eq(sourceId)))
                .WillByDefault(Return(FileStatus{}));
        }
    }

    void setFilesRemoved(const QmlDesigner::SourceIds &sourceIds)
    {
        for (auto sourceId : sourceIds) {
            ON_CALL(fileSystemMock, fileStatus(Eq(sourceId))).WillByDefault(Return(FileStatus{sourceId}));
            ON_CALL(projectStorageMock, fetchFileStatus(Eq(sourceId)))
                .WillByDefault(Return(createFileStatus(sourceId, 1, 21)));
        }
    }

    void setFilesNotExists(const QmlDesigner::SourceIds &sourceIds)
    {
        for (auto sourceId : sourceIds) {
            ON_CALL(fileSystemMock, fileStatus(Eq(sourceId))).WillByDefault(Return(FileStatus{sourceId}));
            ON_CALL(projectStorageMock, fetchFileStatus(Eq(sourceId)))
                .WillByDefault(Return(FileStatus{SourceId{}}));
        }
    }

    void setFilesNotExistsUnchanged(const QmlDesigner::SourceIds &sourceIds)
    {
        for (auto sourceId : sourceIds) {
            ON_CALL(fileSystemMock, fileStatus(Eq(sourceId))).WillByDefault(Return(FileStatus{sourceId}));
            ON_CALL(projectStorageMock, fetchFileStatus(Eq(sourceId)))
                .WillByDefault(Return(FileStatus{sourceId}));
        }
    }

    void setFiles(FileState state, const QmlDesigner::SourceIds &sourceIds)
    {
        switch (state) {
        case FileState::Unchanged:
            setFilesUnchanged(sourceIds);
            break;
        case FileState::Changed:
            setFilesChanged(sourceIds);
            break;
        case FileState::Added:
            setFilesAdded(sourceIds);
            break;
        case FileState::Removed:
            setFilesRemoved(sourceIds);
            break;
        case FileState::NotExists:
            setFilesNotExists(sourceIds);
            break;
        case FileState::NotExistsUnchanged:
            setFilesNotExistsUnchanged(sourceIds);
        }
    }

    void setFileNames(QStringView directoryPath,
                      const QStringList &fileNames,
                      const QStringList &nameFilters)
    {
        ON_CALL(fileSystemMock, fileNames(Eq(directoryPath), UnorderedElementsAreArray(nameFilters)))
            .WillByDefault(Return(fileNames));
    }

    void setQmlFileNames(QStringView directoryPath, const QStringList &qmlFileNames)
    {
        setFileNames(directoryPath, qmlFileNames, {"*.qml"});
    }

    void setQmlTypesFileNames(QStringView directoryPath, const QStringList &qmlFileNames)
    {
        setFileNames(directoryPath, qmlFileNames, {"*.qmltypes"});
    }

    void setProjectEntryInfos(SourceId directorySourceId,
                           FileType fileType,
                           const ProjectEntryInfos &projectEntryInfos)
    {
        ON_CALL(projectStorageMock, fetchProjectEntryInfos(Eq(directorySourceId), fileType))
            .WillByDefault(Return(projectEntryInfos));
        for (const ProjectEntryInfo &projectEntryInfo : projectEntryInfos) {
            ON_CALL(projectStorageMock, fetchProjectEntryInfo(Eq(projectEntryInfo.sourceId)))
                .WillByDefault(Return(std::optional{projectEntryInfo}));
        }
    }

    void setQmlDocumentProjectEntryInfos(SourceId directorySourceId, const ProjectEntryInfos &projectEntryInfos)
    {
        setProjectEntryInfos(directorySourceId, FileType::QmlDocument, projectEntryInfos);
    }

    void setQmlTypesProjectEntryInfos(SourceId directorySourceId, const ProjectEntryInfos &projectEntryInfos)
    {
        setProjectEntryInfos(directorySourceId, FileType::QmlTypes, projectEntryInfos);
    }

    void setContent(QStringView path, const QString &content)
    {
        ON_CALL(fileSystemMock, contentAsQString(Eq(path))).WillByDefault(Return(content));
    }

    void setExpectedContent(QStringView path, const QString &content)
    {
        EXPECT_CALL(fileSystemMock, contentAsQString(Eq(path))).WillRepeatedly(Return(content));
    }

    void setFileSystemSubdirectories(QStringView directoryPath, const QStringList &subdirectoryPaths)
    {
        ON_CALL(fileSystemMock, subdirectories(Eq(directoryPath))).WillByDefault(Return(subdirectoryPaths));
    }

    void setStorageSubdirectories(DirectoryPathId directoryId,
                                  const QmlDesigner::SmallDirectoryPathIds<32> &subdirectoryIds)
    {
        ON_CALL(projectStorageMock, fetchSubdirectoryIds(Eq(directoryId)))
            .WillByDefault(Return(subdirectoryIds));
    }

    auto moduleId(Utils::SmallStringView name, ModuleKind kind) const
    {
        return modulesStorage.moduleId(name, kind);
    }

    SourceId createDirectorySourceId(Utils::SmallStringView path) const
    {
        auto directoryId = sourcePathCache.directoryPathId(path);
        return SourceId::create(directoryId, FileNameId{});
    }

    SourceId createDirectorySourceIdFromQString(const QString &path) const
    {
        return createDirectorySourceId(Utils::PathString{path});
    }

    static QmlDesigner::FileStatus createFileStatus(SourceId sourceId,
                                                    long long size,
                                                    long long modifiedTime)
    {
        using file_time_type = std::filesystem::file_time_type;

        return QmlDesigner::FileStatus{sourceId,
                                       size,
                                       file_time_type{file_time_type::duration{modifiedTime}}};
    }

protected:
    inline static std::unique_ptr<StaticData> staticData;
    Sqlite::Database &database = staticData->database;
    QmlDesigner::ProjectStorage &storage = staticData->storage;
    QmlDesigner::SourcePathCache<QmlDesigner::SourcePathStorage> sourcePathCache{
        staticData->sourcePathStorage};
    QmlDesigner::ModulesStorage &modulesStorage = staticData->modulesStorage;
    NiceMock<FileSystemMock> fileSystemMock;
    NiceMock<ProjectStorageMock> projectStorageMock{modulesStorage};
    NiceMock<QmlTypesParserMock> qmlTypesParserMock;
    NiceMock<QmlDocumentParserMock> qmlDocumentParserMock;
    QmlDesigner::FileStatusCache fileStatusCache{fileSystemMock};
    NiceMock<ProjectStoragePathWatcherMock> patchWatcherMock;
    NiceMock<ProjectStorageErrorNotifierMock> errorNotifierMock;
    QmlDesigner::ProjectStorageUpdater updater{fileSystemMock,
                                               projectStorageMock,
                                               fileStatusCache,
                                               sourcePathCache,
                                               modulesStorage,
                                               qmlDocumentParserMock,
                                               qmlTypesParserMock,
                                               patchWatcherMock,
                                               errorNotifierMock,
                                               projectPartId,
                                               qtPartId};
};

class ProjectStorageUpdater_get_content_for_qml_dir_paths : public BaseProjectStorageUpdater
{
public:
    ProjectStorageUpdater_get_content_for_qml_dir_paths()
    {
        setFilesChanged({qmlDir1PathSourceId});
        setFilesAdded({qmlDir2PathSourceId});
        setFilesUnchanged({qmlDir3PathSourceId, path3SourceId});
    }

    SourceId qmlDir1PathSourceId = sourcePathCache.sourceId("/path/one/qmldir");
    SourceId qmlDir2PathSourceId = sourcePathCache.sourceId("/path/two/qmldir");
    SourceId qmlDir3PathSourceId = sourcePathCache.sourceId("/path/three/qmldir");
    DirectoryPathId path3DirectoryPathId = sourcePathCache.directoryPathId("/path/three");
    SourceId path3SourceId = SourceId::create(path3DirectoryPathId, QmlDesigner::FileNameId{});
};

TEST_F(ProjectStorageUpdater_get_content_for_qml_dir_paths, file_status_is_different)
{
    QStringList directories = {"/path/one", "/path/two", "/path/three"};

    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/one/qmldir"))));
    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/two/qmldir"))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater_get_content_for_qml_dir_paths, file_status_is_different_for_subdirectories)
{
    QStringList directories = {"/path/one"};
    setFileSystemSubdirectories(u"/path/one", {"/path/two", "/path/three"});

    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/one/qmldir"))));
    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/two/qmldir"))));

    updater.update({.qtDirectories = directories});
}

class ProjectStorageUpdater_get_content_for_qml_types : public BaseProjectStorageUpdater
{
public:
    ProjectStorageUpdater_get_content_for_qml_types()
    {
        setQmlFileNames(u"/path", {});
        setExpectedContent(u"/path/qmldir", qmldir);
        setContent(u"/path/example.qmltypes", qmltypes);
        setFilesChanged({qmlDirPathSourceId, directoryPathSourceId});
    }

public:
    QString qmldir{R"(module Example
                      typeinfo example.qmltypes)"};
    QString qmltypes{"Module {\ndependencies: [module1]}"};
    SourceId qmltypesPathSourceId = sourcePathCache.sourceId("/path/example.qmltypes");
    SourceId qmlDirPathSourceId = sourcePathCache.sourceId("/path/qmldir");
    DirectoryPathId directoryPathId = qmlDirPathSourceId.directoryPathId();
    SourceId directoryPathSourceId = SourceId::create(directoryPathId, QmlDesigner::FileNameId{});
    ModuleId exampleModuleId = modulesStorage.moduleId("Example", ModuleKind::CppLibrary);
};

TEST_F(ProjectStorageUpdater_get_content_for_qml_types, added_qml_types_file_provides_content)
{
    setFilesAdded({qmlDirPathSourceId, qmltypesPathSourceId});

    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/example.qmltypes"))));

    updater.update({.qtDirectories = {"/path"}});
}

TEST_F(ProjectStorageUpdater_get_content_for_qml_types, changed_qml_types_file_provides_content)
{
    setQmlTypesProjectEntryInfos(
        qmlDirPathSourceId,
        {{qmlDirPathSourceId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes}});
    setFilesUnchanged({qmlDirPathSourceId, directoryPathSourceId});
    setFilesChanged({qmltypesPathSourceId});

    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/example.qmltypes"))));

    updater.update({.qtDirectories = {"/path"}});
}

TEST_F(ProjectStorageUpdater_get_content_for_qml_types, removed_qml_types_file_does_not_provide_content)
{
    EXPECT_CALL(fileSystemMock, contentAsQString(_)).Times(AnyNumber());
    setFilesRemoved({qmltypesPathSourceId});

    EXPECT_CALL(fileSystemMock, contentAsQString(Eq("/path/example.qmltypes"_L1))).Times(0);

    updater.update({.qtDirectories = {"/path"}});
}

TEST_F(ProjectStorageUpdater_get_content_for_qml_types, removed_qml_types_file_notifies_about_error)
{
    setFilesRemoved({qmltypesPathSourceId});

    EXPECT_CALL(errorNotifierMock, qmltypesFileMissing(Eq("/path/example.qmltypes"_L1)));

    updater.update({.qtDirectories = {"/path"}});
}

class ProjectStorageUpdater_get_content_for_qml_documents : public BaseProjectStorageUpdater
{
public:
    ProjectStorageUpdater_get_content_for_qml_documents()
    {
        setQmlFileNames(u"/path", {"First.qml", "First2.qml", "OldSecond.qml", "Second.qml"});
        setExpectedContent(u"/path/qmldir", qmldir);
        setContent(u"/path/First.qml", qmlDocument1);
        setContent(u"/path/First2.qml", qmlDocument1_2);
        setContent(u"/path/Second.qml", qmlDocument2);
        setContent(u"/path/OldSecond.qml", qmlDocument2Old);
    }

public:
    QString qmldir{R"(module Example
                      First 1.0 First.qml
                      First 2.2 First2.qml
                      Second 2.1 OldSecond.qml
                      Second 2.2 Second.qml)"};
    QString qmlDocument1{"First{}"};
    QString qmlDocument1_2{"First2{}"};
    QString qmlDocument2{"Second{}"};
    QString qmlDocument2Old{"SecondOld{}"};
    SourceId qmlDirPathSourceId = sourcePathCache.sourceId("/path/qmldir");
    DirectoryPathId directoryPathId = qmlDirPathSourceId.directoryPathId();
    SourceId directoryPathSourceId = SourceId::create(directoryPathId);
    SourceId qmlDocument1SourceId = sourcePathCache.sourceId("/path/First.qml");
    SourceId qmlDocument1_2SourceId = sourcePathCache.sourceId("/path/First2.qml");
    SourceId qmlDocument2SourceId = sourcePathCache.sourceId("/path/Second.qml");
    SourceId qmlDocument2OldSourceId = sourcePathCache.sourceId("/path/OldSecond.qml");
    ModuleId exampleModuleId = modulesStorage.moduleId("Example", ModuleKind::QmlLibrary);
};

TEST_F(ProjectStorageUpdater_get_content_for_qml_documents, add_qml_documents)
{
    setFilesChanged({qmlDirPathSourceId, directoryPathSourceId});
    setFilesAdded(
        {qmlDocument1SourceId, qmlDocument1_2SourceId, qmlDocument2SourceId, qmlDocument2OldSourceId});

    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/First.qml"))));
    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/First2.qml"))));
    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/OldSecond.qml"))));
    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/Second.qml"))));

    updater.update({.qtDirectories = {"/path"}});
}

TEST_F(ProjectStorageUpdater_get_content_for_qml_documents, changed_qml_documents)
{
    setQmlDocumentProjectEntryInfos(
        directoryPathSourceId,
        {{directoryPathSourceId, qmlDocument1SourceId, exampleModuleId, FileType::QmlDocument},
         {directoryPathSourceId, qmlDocument1_2SourceId, exampleModuleId, FileType::QmlDocument},
         {directoryPathSourceId, qmlDocument2SourceId, exampleModuleId, FileType::QmlDocument},
         {directoryPathSourceId, qmlDocument2OldSourceId, exampleModuleId, FileType::QmlDocument}});
    setFilesUnchanged({qmlDirPathSourceId, directoryPathSourceId, qmlDocument2SourceId});
    setFilesChanged({qmlDocument1SourceId, qmlDocument1_2SourceId, qmlDocument2OldSourceId});

    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/First.qml"))));
    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/First2.qml"))));
    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/OldSecond.qml"))));
    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/Second.qml")))).Times(0);

    updater.update({.qtDirectories = {"/path"}});
}

class ProjectStorageUpdater_parse_qml_types : public BaseProjectStorageUpdater
{
public:
    ProjectStorageUpdater_parse_qml_types()
    {
        QString qmldir{R"(module Example
                      typeinfo example.qmltypes
                      typeinfo example2.qmltypes)"};
        setContent(u"/root/path/qmldir", qmldir);
        setContent(u"/root/path/example.qmltypes", qmltypes);
        setContent(u"/root/path/example2.qmltypes", qmltypes2);
    }

public:
    QString qmltypes{"Module {\ndependencies: []}"};
    QString qmltypes2{"Module {\ndependencies: [foo]}"};
    ModuleId exampleCppNativeModuleId{modulesStorage.moduleId("Example", ModuleKind::CppLibrary)};
    SourceId qmltypesPathSourceId = sourcePathCache.sourceId("/root/path/example.qmltypes");
    SourceId qmltypes2PathSourceId = sourcePathCache.sourceId("/root/path/example2.qmltypes");
    SourceId qmlDirPathSourceId = sourcePathCache.sourceId("/root/path/qmldir");
    DirectoryPathId directoryPathId = qmlDirPathSourceId.directoryPathId();
    SourceId directoryPathSourceId = SourceId::create(directoryPathId, QmlDesigner::FileNameId{});
};

TEST_F(ProjectStorageUpdater_parse_qml_types, add_directory)
{
    setFilesAdded(
        {directoryPathSourceId, qmlDirPathSourceId, qmltypesPathSourceId, qmltypes2PathSourceId});

    EXPECT_CALL(qmlTypesParserMock,
                parse(qmltypes,
                      _,
                      _,
                      _,
                      Field("ProjectEntryInfo::moduleId", &ProjectEntryInfo::moduleId, exampleCppNativeModuleId),
                      IsInsideProject::No));
    EXPECT_CALL(qmlTypesParserMock,
                parse(qmltypes2,
                      _,
                      _,
                      _,
                      Field("ProjectEntryInfo::moduleId", &ProjectEntryInfo::moduleId, exampleCppNativeModuleId),
                      IsInsideProject::No));

    updater.update({.qtDirectories = {"/root/path"}});
}

TEST_F(ProjectStorageUpdater_parse_qml_types, add_qmltypes)
{
    setQmlTypesProjectEntryInfos(
        qmlDirPathSourceId,
        {{qmlDirPathSourceId, qmltypesPathSourceId, exampleCppNativeModuleId, FileType::QmlTypes},
         {qmlDirPathSourceId, qmltypes2PathSourceId, exampleCppNativeModuleId, FileType::QmlTypes}});
    setFilesUnchanged({directoryPathSourceId, qmlDirPathSourceId});
    setFilesAdded({qmltypesPathSourceId, qmltypes2PathSourceId});

    EXPECT_CALL(qmlTypesParserMock,
                parse(qmltypes,
                      _,
                      _,
                      _,
                      Field("ProjectEntryInfo::moduleId", &ProjectEntryInfo::moduleId, exampleCppNativeModuleId),
                      IsInsideProject::No));
    EXPECT_CALL(qmlTypesParserMock,
                parse(qmltypes2,
                      _,
                      _,
                      _,
                      Field("ProjectEntryInfo::moduleId", &ProjectEntryInfo::moduleId, exampleCppNativeModuleId),
                      IsInsideProject::No));

    updater.update({.qtDirectories = {"/root/path"}});
}

TEST_F(ProjectStorageUpdater_parse_qml_types, add_directory_inide_project)
{
    setFilesAdded(
        {directoryPathSourceId, qmlDirPathSourceId, qmltypesPathSourceId, qmltypes2PathSourceId});

    EXPECT_CALL(qmlTypesParserMock,
                parse(qmltypes,
                      _,
                      _,
                      _,
                      Field("ProjectEntryInfo::moduleId", &ProjectEntryInfo::moduleId, exampleCppNativeModuleId),
                      IsInsideProject::Yes));
    EXPECT_CALL(qmlTypesParserMock,
                parse(qmltypes2,
                      _,
                      _,
                      _,
                      Field("ProjectEntryInfo::moduleId", &ProjectEntryInfo::moduleId, exampleCppNativeModuleId),
                      IsInsideProject::Yes));

    updater.update({.projectDirectory = "/root/path"});
}

TEST_F(ProjectStorageUpdater_parse_qml_types, add_subdirectories)
{
    setFilesAdded(
        {directoryPathSourceId, qmlDirPathSourceId, qmltypesPathSourceId, qmltypes2PathSourceId});
    setFileSystemSubdirectories(u"/root", {"/root/path"});

    EXPECT_CALL(qmlTypesParserMock,
                parse(qmltypes,
                      _,
                      _,
                      _,
                      Field("ProjectEntryInfo::moduleId", &ProjectEntryInfo::moduleId, exampleCppNativeModuleId),
                      IsInsideProject::No));
    EXPECT_CALL(qmlTypesParserMock,
                parse(qmltypes2,
                      _,
                      _,
                      _,
                      Field("ProjectEntryInfo::moduleId", &ProjectEntryInfo::moduleId, exampleCppNativeModuleId),
                      IsInsideProject::No));

    updater.update({.qtDirectories = {"/root"}});
}

class ProjectStorageUpdater_parse_qml_documents : public BaseProjectStorageUpdater
{
public:
    ProjectStorageUpdater_parse_qml_documents()

    {
        setQmlFileNames(u"/path", {"First.qml", "First2.qml", "OldSecond.qml", "Second.qml"});
        setContent(u"/path/qmldir", qmldir);
        setContent(u"/path/First.qml", qmlDocument1);
        setContent(u"/path/First2.qml", qmlDocument1_2);
        setContent(u"/path/Second.qml", qmlDocument2);
    }

public:
    QString qmldir{R"(module Example
                      First 1.0 First.qml
                      First 2.2 First2.qml
                      Second 2.2 Second.qml)"};
    QString qmlDocument1{"First{}"};
    QString qmlDocument1_2{"First2{}"};
    QString qmlDocument2{"Second{}"};
    SourceId qmlDirPathSourceId = sourcePathCache.sourceId("/path/qmldir");
    DirectoryPathId directoryPathId = qmlDirPathSourceId.directoryPathId();
    SourceId directoryPathSourceId = SourceId::create(directoryPathId, QmlDesigner::FileNameId{});
    SourceId qmlDocument1SourceId = sourcePathCache.sourceId("/path/First.qml");
    SourceId qmlDocument1_2SourceId = sourcePathCache.sourceId("/path/First2.qml");
    SourceId qmlDocument2SourceId = sourcePathCache.sourceId("/path/Second.qml");
    ModuleId exampleModuleId = modulesStorage.moduleId("Example", ModuleKind::QmlLibrary);
};

TEST_F(ProjectStorageUpdater_parse_qml_documents, parse_added_qml_documents_in_qt)
{
    setFilesChanged({qmlDirPathSourceId, directoryPathSourceId});
    setFilesAdded({qmlDocument1SourceId, qmlDocument1_2SourceId, qmlDocument2SourceId});

    EXPECT_CALL(qmlDocumentParserMock,
                parse(qmlDocument1, _, qmlDocument1SourceId, Eq("/path"), IsInsideProject::No));
    EXPECT_CALL(qmlDocumentParserMock,
                parse(qmlDocument1_2, _, qmlDocument1_2SourceId, Eq("/path"), IsInsideProject::No));
    EXPECT_CALL(qmlDocumentParserMock,
                parse(qmlDocument2, _, qmlDocument2SourceId, Eq("/path"), IsInsideProject::No));

    updater.update({.qtDirectories = {"/path"}});
}

TEST_F(ProjectStorageUpdater_parse_qml_documents, parse_changed_qml_documents_in_project)
{
    setQmlDocumentProjectEntryInfos(
        directoryPathSourceId,
        {{directoryPathSourceId, qmlDocument1SourceId, exampleModuleId, FileType::QmlDocument},
         {directoryPathSourceId, qmlDocument1_2SourceId, exampleModuleId, FileType::QmlDocument},
         {directoryPathSourceId, qmlDocument2SourceId, exampleModuleId, FileType::QmlDocument}});
    setFilesUnchanged({qmlDirPathSourceId, directoryPathSourceId, qmlDocument2SourceId});
    setFilesChanged({qmlDocument1SourceId, qmlDocument1_2SourceId});

    EXPECT_CALL(qmlDocumentParserMock,
                parse(qmlDocument1, _, qmlDocument1SourceId, Eq("/path"), IsInsideProject::Yes));
    EXPECT_CALL(qmlDocumentParserMock,
                parse(qmlDocument1_2, _, qmlDocument1_2SourceId, Eq("/path"), IsInsideProject::Yes));
    EXPECT_CALL(qmlDocumentParserMock,
                parse(qmlDocument2, _, qmlDocument2SourceId, Eq("/path"), IsInsideProject::Yes))
        .Times(0);

    updater.update({.projectDirectory = "/path"});
}

TEST_F(ProjectStorageUpdater_parse_qml_documents, parse_added_qml_documents_in_project)
{
    setFilesChanged({qmlDirPathSourceId, directoryPathSourceId});
    setFilesAdded({qmlDocument1SourceId, qmlDocument1_2SourceId, qmlDocument2SourceId});

    EXPECT_CALL(qmlDocumentParserMock,
                parse(qmlDocument1, _, qmlDocument1SourceId, Eq("/path"), IsInsideProject::Yes));
    EXPECT_CALL(qmlDocumentParserMock,
                parse(qmlDocument1_2, _, qmlDocument1_2SourceId, Eq("/path"), IsInsideProject::Yes));
    EXPECT_CALL(qmlDocumentParserMock,
                parse(qmlDocument2, _, qmlDocument2SourceId, Eq("/path"), IsInsideProject::Yes));

    updater.update({.projectDirectory = "/path"});
}

TEST_F(ProjectStorageUpdater_parse_qml_documents, non_existing_qml_documents_are_ignored)
{
    setFilesChanged({qmlDirPathSourceId, directoryPathSourceId});
    setFilesAdded({qmlDocument1SourceId});
    setFilesNotExists({qmlDocument1_2SourceId});
    setFilesNotExistsUnchanged({qmlDocument2SourceId});

    EXPECT_CALL(qmlDocumentParserMock, parse(qmlDocument1, _, _, _, _));
    EXPECT_CALL(qmlDocumentParserMock, parse(qmlDocument1_2, _, _, _, _)).Times(0);
    EXPECT_CALL(qmlDocumentParserMock, parse(qmlDocument2, _, _, _, _)).Times(0);

    updater.update({.projectDirectory = "/path"});
}

class ProjectStorageUpdater_synchronize_empty : public BaseProjectStorageUpdater
{
public:
    ProjectStorageUpdater_synchronize_empty()
    {
        QString qmldir{R"(module Example
                      typeinfo example.qmltypes
                      typeinfo example2.qmltypes)"};
        setContent(u"/root/path/qmldir", qmldir);
        setContent(u"/root/path/example.qmltypes", qmltypes);
        setContent(u"/root/path/example2.qmltypes", qmltypes2);
        setFileSystemSubdirectories(u"/root", {"/root/path"});
    }

public:
    QString qmltypes{"Module {\ndependencies: []}"};
    QString qmltypes2{"Module {\ndependencies: [foo]}"};
    SourceId qmltypesPathSourceId = sourcePathCache.sourceId("/root/path/example.qmltypes");
    SourceId qmltypes2PathSourceId = sourcePathCache.sourceId("/root/path/example2.qmltypes");
    SourceId qmlDirPathSourceId = sourcePathCache.sourceId("/root/path/qmldir");
    DirectoryPathId directoryPathId = qmlDirPathSourceId.directoryPathId();
    SourceId directoryPathSourceId = SourceId::create(directoryPathId, QmlDesigner::FileNameId{});
    SourceId annotationDirectoryId = createDirectorySourceId("/root/path/designer");
    SourceId rootQmlDirPathSourceId = sourcePathCache.sourceId("/root/qmldir");
    SourceId rootDirectoryPathSourceId = createDirectorySourceId("/root");
    SourceId rootAnnotationDirectoryId = createDirectorySourceId("/root/designer");
    SourceId ignoreInQdsSourceId = sourcePathCache.sourceId("/root/path/ignore-in-qds");
};

TEST_F(ProjectStorageUpdater_synchronize_empty, for_no_change_for_qt)
{
    setFilesUnchanged(
        {qmltypesPathSourceId, qmltypes2PathSourceId, qmlDirPathSourceId, directoryPathSourceId});
    setFilesNotExistsUnchanged({annotationDirectoryId});

    EXPECT_CALL(projectStorageMock, synchronize(PackageIsEmpty()));

    updater.update({.qtDirectories = {"/root/path"}});
}

TEST_F(ProjectStorageUpdater_synchronize_empty, for_no_change_for_project)
{
    setFilesUnchanged(
        {qmltypesPathSourceId, qmltypes2PathSourceId, qmlDirPathSourceId, directoryPathSourceId});
    setFilesNotExistsUnchanged({annotationDirectoryId});

    EXPECT_CALL(projectStorageMock, synchronize(PackageIsEmpty()));

    updater.update({.projectDirectory = "/root/path"});
}

TEST_F(ProjectStorageUpdater_synchronize_empty, for_no_change_in_subdirectory)
{
    setFilesUnchanged({qmltypesPathSourceId,
                       qmltypes2PathSourceId,
                       qmlDirPathSourceId,
                       rootDirectoryPathSourceId,
                       directoryPathSourceId});
    setFilesNotExistsUnchanged(
        {annotationDirectoryId, rootQmlDirPathSourceId, rootAnnotationDirectoryId, ignoreInQdsSourceId});

    EXPECT_CALL(projectStorageMock, synchronize(PackageIsEmpty()));

    updater.update({.qtDirectories = {"/root"}});
}

TEST_F(ProjectStorageUpdater_synchronize_empty, for_ignored_subdirectory)
{
    setFilesChanged({qmltypesPathSourceId, qmltypes2PathSourceId, qmlDirPathSourceId});
    setFilesUnchanged({rootDirectoryPathSourceId, directoryPathSourceId, ignoreInQdsSourceId});
    setFilesNotExistsUnchanged(
        {rootAnnotationDirectoryId, annotationDirectoryId, rootQmlDirPathSourceId});

    EXPECT_CALL(projectStorageMock, synchronize(PackageIsEmpty()));

    updater.update({.projectDirectory = "/root"});
}

TEST_F(ProjectStorageUpdater_synchronize_empty, not_for_added_ignored_subdirectory)
{
    setFilesChanged({qmltypesPathSourceId, qmltypes2PathSourceId, qmlDirPathSourceId});
    setFilesUnchanged({rootDirectoryPathSourceId, directoryPathSourceId});
    setFilesAdded({ignoreInQdsSourceId});
    setFilesNotExistsUnchanged(
        {rootAnnotationDirectoryId, annotationDirectoryId, rootQmlDirPathSourceId});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(
            AllOf(Field("SynchronizationPackage::imports", &SynchronizationPackage::imports, IsEmpty()),
                  Field("SynchronizationPackage::updatedImportSourceIds",
                        &SynchronizationPackage::updatedImportSourceIds,
                        IsEmpty()),
                  Field("SynchronizationPackage::exportedTypes",
                        &SynchronizationPackage::exportedTypes,
                        IsEmpty()),
                  Field("SynchronizationPackage::updatedExportedTypeSourceIds",
                        &SynchronizationPackage::updatedExportedTypeSourceIds,
                        IsEmpty()),
                  Field("SynchronizationPackage::types", &SynchronizationPackage::types, IsEmpty()),
                  Field("SynchronizationPackage::updatedTypeSourceIds",
                        &SynchronizationPackage::updatedTypeSourceIds,
                        IsEmpty()),
                  Field("SynchronizationPackage::fileStatuses",
                        &SynchronizationPackage::fileStatuses,
                        UnorderedElementsAre(IsFileStatus(ignoreInQdsSourceId, 1, 21))),
                  Field("SynchronizationPackage::updatedFileStatusSourceIds",
                        &SynchronizationPackage::updatedFileStatusSourceIds,
                        UnorderedElementsAre(ignoreInQdsSourceId)))));

    updater.update({.projectDirectory = "/root"});
}

class ProjectStorageUpdater_synchronize_subdirectories : public BaseProjectStorageUpdater
{
public:
    ProjectStorageUpdater_synchronize_subdirectories()
    {
        setFileSystemSubdirectories(u"/root", {"/root/one", "/root/two"});
        setFileSystemSubdirectories(u"/root/one", {"/root/one/three"});
    }

public:
    SourceId rootDirectoryPathSourceId = createDirectorySourceId("/root");
    DirectoryPathId rootDirectoryPathId = rootDirectoryPathSourceId.directoryPathId();
    SourceId path1SourceId = createDirectorySourceId("/root/one");
    DirectoryPathId path1DirectoryPathId = path1SourceId.directoryPathId();
    SourceId path1DirectoryPathSourceId = SourceId::create(path1DirectoryPathId);
    SourceId path2SourceId = createDirectorySourceId("/root/two");
    DirectoryPathId path2DirectoryPathId = path2SourceId.directoryPathId();
    SourceId path2DirectoryPathSourceId = SourceId::create(path2DirectoryPathId);
    SourceId path3SourceId = createDirectorySourceId("/root/one/three");
    DirectoryPathId path3DirectoryPathId = path3SourceId.directoryPathId();
    SourceId path3DirectoryPathSourceId = SourceId::create(path3DirectoryPathId);
};

TEST_F(ProjectStorageUpdater_synchronize_subdirectories, added_qt_subdircectories)
{
    setFilesAdded({rootDirectoryPathSourceId, path1SourceId, path2SourceId, path3SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::projectEntryInfos",
                                &SynchronizationPackage::projectEntryInfos,
                                UnorderedElementsAre(IsProjectEntryInfo(rootDirectoryPathSourceId,
                                                                     path1SourceId,
                                                                     ModuleId{},
                                                                     FileType::Directory),
                                                     IsProjectEntryInfo(rootDirectoryPathSourceId,
                                                                     path2SourceId,
                                                                     ModuleId{},
                                                                     FileType::Directory),
                                                     IsProjectEntryInfo(path1DirectoryPathSourceId,
                                                                     path3SourceId,
                                                                     ModuleId{},
                                                                     FileType::Directory))),
                          Field("SynchronizationPackage::updatedProjectEntryInfoSourceIds",
                                &SynchronizationPackage::updatedProjectEntryInfoSourceIds,
                                UnorderedElementsAre(rootDirectoryPathSourceId,
                                                     path1DirectoryPathSourceId,
                                                     path2DirectoryPathSourceId,
                                                     path3DirectoryPathSourceId)))));

    updater.update({.qtDirectories = {"/root"}});
}

TEST_F(ProjectStorageUpdater_synchronize_subdirectories, changed_qt_subdircectories)
{
    setFilesChanged({rootDirectoryPathSourceId, path1SourceId, path2SourceId, path3SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::projectEntryInfos",
                                &SynchronizationPackage::projectEntryInfos,
                                UnorderedElementsAre(IsProjectEntryInfo(rootDirectoryPathSourceId,
                                                                     path1SourceId,
                                                                     ModuleId{},
                                                                     FileType::Directory),
                                                     IsProjectEntryInfo(rootDirectoryPathSourceId,
                                                                     path2SourceId,
                                                                     ModuleId{},
                                                                     FileType::Directory),
                                                     IsProjectEntryInfo(path1DirectoryPathSourceId,
                                                                     path3SourceId,
                                                                     ModuleId{},
                                                                     FileType::Directory))),
                          Field("SynchronizationPackage::updatedProjectEntryInfoSourceIds",
                                &SynchronizationPackage::updatedProjectEntryInfoSourceIds,
                                UnorderedElementsAre(rootDirectoryPathSourceId,
                                                     path1DirectoryPathSourceId,
                                                     path2DirectoryPathSourceId,
                                                     path3DirectoryPathSourceId)))));

    updater.update({.qtDirectories = {"/root"}});
}

TEST_F(ProjectStorageUpdater_synchronize_subdirectories, added_project_subdircectories)
{
    setFilesAdded({rootDirectoryPathSourceId, path1SourceId, path2SourceId, path3SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::projectEntryInfos",
                                &SynchronizationPackage::projectEntryInfos,
                                UnorderedElementsAre(IsProjectEntryInfo(rootDirectoryPathSourceId,
                                                                     path1SourceId,
                                                                     ModuleId{},
                                                                     FileType::Directory),
                                                     IsProjectEntryInfo(rootDirectoryPathSourceId,
                                                                     path2SourceId,
                                                                     ModuleId{},
                                                                     FileType::Directory),
                                                     IsProjectEntryInfo(path1DirectoryPathSourceId,
                                                                     path3SourceId,
                                                                     ModuleId{},
                                                                     FileType::Directory))),
                          Field("SynchronizationPackage::updatedProjectEntryInfoSourceIds",
                                &SynchronizationPackage::updatedProjectEntryInfoSourceIds,
                                UnorderedElementsAre(rootDirectoryPathSourceId,
                                                     path1DirectoryPathSourceId,
                                                     path2DirectoryPathSourceId,
                                                     path3DirectoryPathSourceId)))));

    updater.update({.projectDirectory = "/root"});
}

TEST_F(ProjectStorageUpdater_synchronize_subdirectories, changed_project_subdircectories)
{
    setFilesChanged({rootDirectoryPathSourceId, path1SourceId, path2SourceId, path3SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::projectEntryInfos",
                                &SynchronizationPackage::projectEntryInfos,
                                UnorderedElementsAre(IsProjectEntryInfo(rootDirectoryPathSourceId,
                                                                     path1SourceId,
                                                                     ModuleId{},
                                                                     FileType::Directory),
                                                     IsProjectEntryInfo(rootDirectoryPathSourceId,
                                                                     path2SourceId,
                                                                     ModuleId{},
                                                                     FileType::Directory),
                                                     IsProjectEntryInfo(path1DirectoryPathSourceId,
                                                                     path3SourceId,
                                                                     ModuleId{},
                                                                     FileType::Directory))),
                          Field("SynchronizationPackage::updatedProjectEntryInfoSourceIds",
                                &SynchronizationPackage::updatedProjectEntryInfoSourceIds,
                                UnorderedElementsAre(rootDirectoryPathSourceId,
                                                     path1DirectoryPathSourceId,
                                                     path2DirectoryPathSourceId,
                                                     path3DirectoryPathSourceId)))));

    updater.update({.projectDirectory = "/root"});
}

TEST_F(ProjectStorageUpdater_synchronize_subdirectories, qt_subdirectories_if_root_is_not_changed)
{
    setFilesChanged({path1SourceId, path2SourceId, path3SourceId});
    setFilesUnchanged({rootDirectoryPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::projectEntryInfos",
                                        &SynchronizationPackage::projectEntryInfos,
                                        UnorderedElementsAre(IsProjectEntryInfo(path1DirectoryPathSourceId,
                                                                             path3SourceId,
                                                                             ModuleId{},
                                                                             FileType::Directory))),
                                  Field("SynchronizationPackage::updatedProjectEntryInfoSourceIds",
                                        &SynchronizationPackage::updatedProjectEntryInfoSourceIds,
                                        UnorderedElementsAre(path1DirectoryPathSourceId,
                                                             path2DirectoryPathSourceId,
                                                             path3DirectoryPathSourceId)))));

    updater.update({.qtDirectories = {"/root"}});
}

TEST_F(ProjectStorageUpdater_synchronize_subdirectories, project_subdirectories_if_root_is_not_changed)
{
    setFilesChanged({path1SourceId, path2SourceId, path3SourceId});
    setFilesUnchanged({rootDirectoryPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::projectEntryInfos",
                                        &SynchronizationPackage::projectEntryInfos,
                                        UnorderedElementsAre(IsProjectEntryInfo(path1DirectoryPathSourceId,
                                                                             path3SourceId,
                                                                             ModuleId{},
                                                                             FileType::Directory))),
                                  Field("SynchronizationPackage::updatedProjectEntryInfoSourceIds",
                                        &SynchronizationPackage::updatedProjectEntryInfoSourceIds,
                                        UnorderedElementsAre(path1DirectoryPathSourceId,
                                                             path2DirectoryPathSourceId,
                                                             path3DirectoryPathSourceId)))));

    updater.update({.projectDirectory = "/root"});
}

TEST_F(ProjectStorageUpdater_synchronize_subdirectories, for_deleted_qt_subdirecties)
{
    setFilesChanged({rootDirectoryPathSourceId});
    setFilesRemoved({path1SourceId, path3SourceId});
    setFilesUnchanged({path2SourceId});
    setStorageSubdirectories(rootDirectoryPathId, {path1DirectoryPathId, path2DirectoryPathId});
    setStorageSubdirectories(path1DirectoryPathId, {path3DirectoryPathId});
    setFileSystemSubdirectories(u"/root", {"/root/two"});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::projectEntryInfos",
                                        &SynchronizationPackage::projectEntryInfos,
                                        UnorderedElementsAre(IsProjectEntryInfo(rootDirectoryPathSourceId,
                                                                             path2SourceId,
                                                                             ModuleId{},
                                                                             FileType::Directory))),
                                  Field("SynchronizationPackage::updatedProjectEntryInfoSourceIds",
                                        &SynchronizationPackage::updatedProjectEntryInfoSourceIds,
                                        UnorderedElementsAre(rootDirectoryPathSourceId,
                                                             path1DirectoryPathSourceId,
                                                             path3DirectoryPathSourceId)))));

    updater.update({.qtDirectories = {"/root"}});
}

TEST_F(ProjectStorageUpdater_synchronize_subdirectories, for_deleted_project_subdirecties)
{
    setFilesChanged({rootDirectoryPathSourceId});
    setFilesRemoved({path1SourceId, path3SourceId});
    setFilesUnchanged({path2SourceId});
    setStorageSubdirectories(rootDirectoryPathId, {path1DirectoryPathId, path2DirectoryPathId});
    setStorageSubdirectories(path1DirectoryPathId, {path3DirectoryPathId});
    setFileSystemSubdirectories(u"/root", {"/root/two"});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::projectEntryInfos",
                                        &SynchronizationPackage::projectEntryInfos,
                                        UnorderedElementsAre(IsProjectEntryInfo(rootDirectoryPathSourceId,
                                                                             path2SourceId,
                                                                             ModuleId{},
                                                                             FileType::Directory))),
                                  Field("SynchronizationPackage::updatedProjectEntryInfoSourceIds",
                                        &SynchronizationPackage::updatedProjectEntryInfoSourceIds,
                                        UnorderedElementsAre(rootDirectoryPathSourceId,
                                                             path1DirectoryPathSourceId,
                                                             path3DirectoryPathSourceId)))));

    updater.update({.projectDirectory = {"/root"}});
}

class synchronize_qml_types : public BaseProjectStorageUpdater
{
public:
    synchronize_qml_types()

    {
        setQmlFileNames(u"/path", {});

        QString qmltypes{"Module {\ndependencies: []}"};
        setContent(u"/path/example.qmltypes", qmltypes);
        setContent(u"/path/qmldir", "module Example\ntypeinfo example.qmltypes\n");
        ON_CALL(qmlTypesParserMock, parse(qmltypes, _, _, _, _, _))
            .WillByDefault([&](auto, auto &imports, auto &types, auto &exportedTypes, auto, auto) {
                types.push_back(objectType);
                imports.push_back(import);
                exportedTypes.emplace_back(qmltypesPathSourceId,
                                           exampleModuleId,
                                           "Object",
                                           Version{},
                                           qmltypesPathSourceId,
                                           "Object");
                exportedTypes.emplace_back(qmltypesPathSourceId,
                                           exampleModuleId,
                                           "Obj",
                                           Version{},
                                           qmltypesPathSourceId,
                                           "Obj");
            });
        setFilesNotExistsUnchanged({annotationDirectorySourceId});
    }

public:
    SourceId qmltypesPathSourceId = sourcePathCache.sourceId("/path/example.qmltypes");
    SourceId qmlDirPathSourceId = sourcePathCache.sourceId("/path/qmldir");
    DirectoryPathId directoryPathId = qmlDirPathSourceId.directoryPathId();
    SourceId directoryPathSourceId = SourceId::create(directoryPathId, QmlDesigner::FileNameId{});
    DirectoryPathId annotationDirectoryId = sourcePathCache.directoryPathId("/path/designer");
    SourceId annotationDirectorySourceId = SourceId::create(annotationDirectoryId, FileNameId{});
    ModuleId qmlModuleId{modulesStorage.moduleId("Qml", ModuleKind::QmlLibrary)};
    ModuleId exampleModuleId{modulesStorage.moduleId("Example", ModuleKind::QmlLibrary)};
    ModuleId exampleCppNativeModuleId{modulesStorage.moduleId("Example", ModuleKind::CppLibrary)};
    Storage::Import import{qmlModuleId, Storage::Version{2, 3}, qmltypesPathSourceId, qmlDirPathSourceId};
    Type objectType{"QObject",
                    ImportedType{},
                    ImportedType{},
                    Storage::TypeTraitsKind::Reference,
                    qmltypesPathSourceId};
};

using ChangeQmlTypesParameters = std::tuple<Update, FileState, FileState, FileState, FileState>;

class synchronize_changed_qml_types : public synchronize_qml_types,
                                      public testing::WithParamInterface<ChangeQmlTypesParameters>
{
public:
    synchronize_changed_qml_types()
        : state{std::get<1>(GetParam())}
        , directoryState{std::get<2>(GetParam())}
        , qmldirState{std::get<3>(GetParam())}
        , annotationDirectoryState{std::get<4>(GetParam())}
        , update{std::get<0>(GetParam())}

    {
        if (qmldirState == FileState::Unchanged) {
            setQmlTypesProjectEntryInfos(
                qmlDirPathSourceId,
                {{qmlDirPathSourceId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes}});
        }

        if (state == FileState::Added)
            directoryState = FileState::Changed;
    }

public:
    FileState state;
    FileState directoryState;
    FileState qmldirState;
    FileState annotationDirectoryState;
    const Update &update;
};

auto change_qml_types_parameters_printer =
    [](const testing::TestParamInfo<ChangeQmlTypesParameters> &info) {
        std::string name = toString(std::get<1>(info.param));

        name += "_qmltypes_file_in_";

        if (std::get<0>(info.param).projectDirectory.isEmpty())
            name += "qt_";
        else
            name += "project_";

        name += toString(std::get<2>(info.param));

        name += "_directory_";

        name += toString(std::get<3>(info.param));

        name += "_qmldir_";

        name += toString(std::get<4>(info.param));

        name += "_annotation";

        return name;
    };

INSTANTIATE_TEST_SUITE_P(ProjectStorageUpdater,
                         synchronize_changed_qml_types,
                         Combine(Values(Update{.qtDirectories = {"/path"}},
                                        Update{.projectDirectory = "/path"}),
                                 Values(FileState::Added, FileState::Changed),
                                 Values(FileState::Changed, FileState::Unchanged),
                                 Values(FileState::Changed, FileState::Unchanged),
                                 Values(FileState::NotExistsUnchanged,
                                        FileState::Added,
                                        FileState::Unchanged,
                                        FileState::Changed)),
                         change_qml_types_parameters_printer);

TEST_P(synchronize_changed_qml_types, from_qt_directory_update_imports)
{
    setFiles(directoryState, {directoryPathSourceId});
    setFiles(annotationDirectoryState, {annotationDirectorySourceId});
    setFiles(qmldirState, {qmlDirPathSourceId});
    setFiles(state, {qmltypesPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::imports",
                                        &SynchronizationPackage::imports,
                                        ElementsAre(import)),
                                  Field("SynchronizationPackage::updatedImportSourceIds",
                                        &SynchronizationPackage::updatedImportSourceIds,
                                        ElementsAre(qmltypesPathSourceId)))));

    updater.update(update);
}

TEST_P(synchronize_changed_qml_types, from_qt_directory_update_exported_types)
{
    setFiles(directoryState, {directoryPathSourceId});
    setFiles(annotationDirectoryState, {annotationDirectorySourceId});
    setFiles(qmldirState, {qmlDirPathSourceId});
    setFiles(state, {qmltypesPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::exportedTypes",
                                        &SynchronizationPackage::exportedTypes,
                                        UnorderedElementsAre(IsExportedType(exampleModuleId,
                                                                            "Object",
                                                                            qmltypesPathSourceId,
                                                                            "Object",
                                                                            qmltypesPathSourceId),
                                                             IsExportedType(exampleModuleId,
                                                                            "Obj",
                                                                            qmltypesPathSourceId,
                                                                            "Obj",
                                                                            qmltypesPathSourceId))),
                                  Field("SynchronizationPackage::updatedExportedTypeSourceIds",
                                        &SynchronizationPackage::updatedExportedTypeSourceIds,
                                        Contains(qmltypesPathSourceId)))));

    updater.update(update);
}

TEST_P(synchronize_changed_qml_types, from_qt_directory_update_types)
{
    setFiles(directoryState, {directoryPathSourceId});
    setFiles(annotationDirectoryState, {annotationDirectorySourceId});
    setFiles(qmldirState, {qmlDirPathSourceId});
    setFiles(state, {qmltypesPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::types",
                                        &SynchronizationPackage::types,
                                        ElementsAre(Eq(objectType))),
                                  Field("SynchronizationPackage::updatedTypeSourceIds",
                                        &SynchronizationPackage::updatedTypeSourceIds,
                                        Contains(qmltypesPathSourceId)))));

    updater.update(update);
}

TEST_P(synchronize_changed_qml_types, from_qt_directory_update_file_status)
{
    setFiles(directoryState, {directoryPathSourceId});
    setFiles(annotationDirectoryState, {annotationDirectorySourceId});
    setFiles(qmldirState, {qmlDirPathSourceId});
    setFiles(state, {qmltypesPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::fileStatuses",
                                        &SynchronizationPackage::fileStatuses,
                                        Contains(IsFileStatus(qmltypesPathSourceId, 1, 21))),
                                  Field("SynchronizationPackage::updatedFileStatusSourceIds",
                                        &SynchronizationPackage::updatedFileStatusSourceIds,
                                        Contains(qmltypesPathSourceId)))));

    updater.update(update);
}

TEST_P(synchronize_changed_qml_types, from_qt_directory_update_project_entry_infos)
{
    bool qmldirUnchanged = qmldirState == FileState::Unchanged;
    setFiles(directoryState, {directoryPathSourceId});
    setFiles(annotationDirectoryState, {annotationDirectorySourceId});
    setFiles(qmldirState, {qmlDirPathSourceId});
    setFiles(state, {qmltypesPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::projectEntryInfos",
                                        &SynchronizationPackage::projectEntryInfos,
                                        Contains(IsProjectEntryInfo(qmlDirPathSourceId,
                                                                 qmltypesPathSourceId,
                                                                 exampleCppNativeModuleId,
                                                                 FileType::QmlTypes))
                                            .Times(qmldirUnchanged ? 0 : 1)),
                                  Field("SynchronizationPackage::updatedProjectEntryInfoSourceIds",
                                        &SynchronizationPackage::updatedProjectEntryInfoSourceIds,
                                        Contains(qmlDirPathSourceId).Times(qmldirUnchanged ? 0 : 1)))));

    updater.update(update);
}

using NonExistingQmlTypesParameters = std::tuple<Update, FileState>;

class synchronize_non_existing_qml_types
    : public synchronize_qml_types,
      public testing::WithParamInterface<NonExistingQmlTypesParameters>
{
public:
    synchronize_non_existing_qml_types()
        : state{std::get<1>(GetParam())}
        , update{std::get<0>(GetParam())}

    {
        setQmlTypesProjectEntryInfos(
            qmlDirPathSourceId,
            {{qmlDirPathSourceId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes}});
    }

public:
    FileState state;
    const Update &update;
};

auto updateStateName = [](const testing::TestParamInfo<NonExistingQmlTypesParameters> &info) {
    std::string name = toString(std::get<1>(info.param));

    if (std::get<0>(info.param).projectDirectory.isEmpty())
        name += "_qt";
    else
        name += "_project";

    return name;
};

INSTANTIATE_TEST_SUITE_P(ProjectStorageUpdater,
                         synchronize_non_existing_qml_types,
                         Combine(Values(Update{.qtDirectories = {"/path"}},
                                        Update{.projectDirectory = "/path"}),
                                 Values(FileState::Removed, FileState::NotExists)),
                         updateStateName);

TEST_P(synchronize_non_existing_qml_types, notfies_error)
{
    setFilesUnchanged({directoryPathSourceId, qmlDirPathSourceId});
    setFiles(state, {qmltypesPathSourceId});

    EXPECT_CALL(errorNotifierMock, qmltypesFileMissing(Eq("/path/example.qmltypes"_L1)));

    updater.update(update);
}

TEST_P(synchronize_non_existing_qml_types, notfies_error_if_direcory_and_qmldir_file_is_changed)
{
    setFilesChanged({directoryPathSourceId, qmlDirPathSourceId});
    setFiles(state, {qmltypesPathSourceId});

    EXPECT_CALL(errorNotifierMock, qmltypesFileMissing(Eq("/path/example.qmltypes"_L1)));

    updater.update(update);
}

TEST_P(synchronize_non_existing_qml_types, updates_source_ids)
{
    setFilesUnchanged({directoryPathSourceId, qmlDirPathSourceId});
    setFiles(state, {qmltypesPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::updatedTypeSourceIds",
                                        &SynchronizationPackage::updatedTypeSourceIds,
                                        Contains(qmltypesPathSourceId)))));

    updater.update(update);
}

TEST_P(synchronize_non_existing_qml_types, updates_project_entry_info)
{
    setFilesChanged({directoryPathSourceId, qmlDirPathSourceId});
    setFiles(state, {qmltypesPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::projectEntryInfos",
                                        &SynchronizationPackage::projectEntryInfos,
                                        Contains(IsProjectEntryInfo(qmlDirPathSourceId,
                                                                 qmltypesPathSourceId,
                                                                 exampleCppNativeModuleId,
                                                                 FileType::QmlTypes))),
                                  Field("SynchronizationPackage::updatedProjectEntryInfoSourceIds",
                                        &SynchronizationPackage::updatedProjectEntryInfoSourceIds,
                                        Contains(qmlDirPathSourceId)))));

    updater.update(update);
}

class ProjectStorageUpdater_synchronize_qmldir_dependencies : public synchronize_qml_types
{
public:
    ProjectStorageUpdater_synchronize_qmldir_dependencies() {}

public:
    SourceId qmltypes2PathSourceId = sourcePathCache.sourceId("/path/example2.qmltypes");
    ModuleId qmlCppNativeModuleId{modulesStorage.moduleId("Qml", ModuleKind::CppLibrary)};
    ModuleId builtinCppNativeModuleId{modulesStorage.moduleId("QML", ModuleKind::CppLibrary)};
};

TEST_F(ProjectStorageUpdater_synchronize_qmldir_dependencies, with_multiple_qml_types)
{
    QString qmldir{R"(module Example
                      depends  Qml
                      depends  QML
                      typeinfo example.qmltypes
                      typeinfo example2.qmltypes
                      )"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({qmlDirPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::moduleDependencies",
                                &SynchronizationPackage::moduleDependencies,
                                UnorderedElementsAre(IsImport(qmlCppNativeModuleId,
                                                              Storage::Version{},
                                                              qmltypesPathSourceId,
                                                              qmlDirPathSourceId),
                                                     IsImport(builtinCppNativeModuleId,
                                                              Storage::Version{},
                                                              qmltypesPathSourceId,
                                                              qmlDirPathSourceId),
                                                     IsImport(qmlCppNativeModuleId,
                                                              Storage::Version{},
                                                              qmltypes2PathSourceId,
                                                              qmlDirPathSourceId),
                                                     IsImport(builtinCppNativeModuleId,
                                                              Storage::Version{},
                                                              qmltypes2PathSourceId,
                                                              qmlDirPathSourceId))),
                          Field("SynchronizationPackage::updatedModuleDependencySourceIds",
                                &SynchronizationPackage::updatedModuleDependencySourceIds,
                                UnorderedElementsAre(qmltypesPathSourceId, qmltypes2PathSourceId)))));

    updater.update({.qtDirectories = {"/path"}});
}

TEST_F(ProjectStorageUpdater_synchronize_qmldir_dependencies, with_double_entries)
{
    QString qmldir{R"(module Example
                      depends  Qml
                      depends  QML
                      depends  Qml
                      typeinfo example.qmltypes
                      )"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({qmlDirPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::moduleDependencies",
                                        &SynchronizationPackage::moduleDependencies,
                                        UnorderedElementsAre(IsImport(qmlCppNativeModuleId,
                                                                      Storage::Version{},
                                                                      qmltypesPathSourceId,
                                                                      qmlDirPathSourceId),
                                                             IsImport(builtinCppNativeModuleId,
                                                                      Storage::Version{},
                                                                      qmltypesPathSourceId,
                                                                      qmlDirPathSourceId))),
                                  Field("SynchronizationPackage::updatedModuleDependencySourceIds",
                                        &SynchronizationPackage::updatedModuleDependencySourceIds,
                                        UnorderedElementsAre(qmltypesPathSourceId)))));

    updater.update({.qtDirectories = {"/path"}});
}

TEST_F(ProjectStorageUpdater_synchronize_qmldir_dependencies, with_colliding_qmldir_imports)
{
    QString qmldir{R"(module Example
                      depends  Qml
                      depends  QML
                      import Qml
                      typeinfo example.qmltypes
                      )"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({qmlDirPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::moduleDependencies",
                                        &SynchronizationPackage::moduleDependencies,
                                        UnorderedElementsAre(IsImport(qmlCppNativeModuleId,
                                                                      Storage::Version{},
                                                                      qmltypesPathSourceId,
                                                                      qmlDirPathSourceId),
                                                             IsImport(builtinCppNativeModuleId,
                                                                      Storage::Version{},
                                                                      qmltypesPathSourceId,
                                                                      qmlDirPathSourceId))),
                                  Field("SynchronizationPackage::updatedModuleDependencySourceIds",
                                        &SynchronizationPackage::updatedModuleDependencySourceIds,
                                        UnorderedElementsAre(qmltypesPathSourceId)))));

    updater.update({.qtDirectories = {"/path"}});
}

TEST_F(ProjectStorageUpdater_synchronize_qmldir_dependencies, with_no_dependencies)
{
    QString qmldir{R"(module Example
                      typeinfo example.qmltypes
                      )"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({qmlDirPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::moduleDependencies",
                                        &SynchronizationPackage::moduleDependencies,
                                        IsEmpty()),
                                  Field("SynchronizationPackage::updatedModuleDependencySourceIds",
                                        &SynchronizationPackage::updatedModuleDependencySourceIds,
                                        UnorderedElementsAre(qmltypesPathSourceId)))));

    updater.update({.qtDirectories = {"/path"}});
}

class ProjectStorageUpdater_synchronize_qmldir_imports : public synchronize_qml_types
{
public:
    ProjectStorageUpdater_synchronize_qmldir_imports() {}

public:
    ModuleId qmlCppNativeModuleId{modulesStorage.moduleId("Qml", ModuleKind::CppLibrary)};
    ModuleId builtinModuleId{modulesStorage.moduleId("QML", ModuleKind::QmlLibrary)};
    ModuleId builtinCppNativeModuleId{modulesStorage.moduleId("QML", ModuleKind::CppLibrary)};
    ModuleId quickModuleId{modulesStorage.moduleId("Quick", ModuleKind::QmlLibrary)};
    ModuleId quickCppNativeModuleId{modulesStorage.moduleId("Quick", ModuleKind::CppLibrary)};
};

TEST_F(ProjectStorageUpdater_synchronize_qmldir_imports, imports)
{
    QString qmldir{R"(module Example
                      import Qml auto
                      import QML 2.1
                      import Quick
                      )"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({qmlDirPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::moduleExportedImports",
                                &SynchronizationPackage::moduleExportedImports,
                                UnorderedElementsAre(ModuleExportedImport{exampleModuleId,
                                                                          qmlModuleId,
                                                                          Storage::Version{},
                                                                          IsAutoVersion::Yes},
                                                     ModuleExportedImport{exampleCppNativeModuleId,
                                                                          qmlCppNativeModuleId,
                                                                          Storage::Version{},
                                                                          IsAutoVersion::No},
                                                     ModuleExportedImport{exampleModuleId,
                                                                          builtinModuleId,
                                                                          Storage::Version{2, 1},
                                                                          IsAutoVersion::No},
                                                     ModuleExportedImport{exampleCppNativeModuleId,
                                                                          builtinCppNativeModuleId,
                                                                          Storage::Version{},
                                                                          IsAutoVersion::No},
                                                     ModuleExportedImport{exampleModuleId,
                                                                          quickModuleId,
                                                                          Storage::Version{},
                                                                          IsAutoVersion::Yes},
                                                     ModuleExportedImport{exampleCppNativeModuleId,
                                                                          quickCppNativeModuleId,
                                                                          Storage::Version{},
                                                                          IsAutoVersion::No})),
                          Field("SynchronizationPackage::updatedModuleIds",
                                &SynchronizationPackage::updatedModuleIds,
                                ElementsAre(exampleModuleId, exampleCppNativeModuleId)))));

    updater.update({.qtDirectories = {"/path"}});
}

TEST_F(ProjectStorageUpdater_synchronize_qmldir_imports, with_no_imports)
{
    QString qmldir{R"(module Example
                      )"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({qmlDirPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::moduleExportedImports",
                                        &SynchronizationPackage::moduleExportedImports,
                                        IsEmpty()),
                                  Field("SynchronizationPackage::updatedModuleIds",
                                        &SynchronizationPackage::updatedModuleIds,
                                        ElementsAre(exampleModuleId, exampleCppNativeModuleId)))));

    updater.update({.qtDirectories = {"/path"}});
}

TEST_F(ProjectStorageUpdater_synchronize_qmldir_imports, with_double_entries)
{
    QString qmldir{R"(module Example
                      import Qml auto
                      import QML 2.1
                      import Quick
                      import Qml
                      )"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({qmlDirPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::moduleExportedImports",
                                &SynchronizationPackage::moduleExportedImports,
                                UnorderedElementsAre(ModuleExportedImport{exampleModuleId,
                                                                          qmlModuleId,
                                                                          Storage::Version{},
                                                                          IsAutoVersion::Yes},
                                                     ModuleExportedImport{exampleCppNativeModuleId,
                                                                          qmlCppNativeModuleId,
                                                                          Storage::Version{},
                                                                          IsAutoVersion::No},
                                                     ModuleExportedImport{exampleModuleId,
                                                                          builtinModuleId,
                                                                          Storage::Version{2, 1},
                                                                          IsAutoVersion::No},
                                                     ModuleExportedImport{exampleCppNativeModuleId,
                                                                          builtinCppNativeModuleId,
                                                                          Storage::Version{},
                                                                          IsAutoVersion::No},
                                                     ModuleExportedImport{exampleModuleId,
                                                                          quickModuleId,
                                                                          Storage::Version{},
                                                                          IsAutoVersion::Yes},
                                                     ModuleExportedImport{exampleCppNativeModuleId,
                                                                          quickCppNativeModuleId,
                                                                          Storage::Version{},
                                                                          IsAutoVersion::No})),
                          Field("SynchronizationPackage::updatedModuleIds",
                                &SynchronizationPackage::updatedModuleIds,
                                ElementsAre(exampleModuleId, exampleCppNativeModuleId)))));

    updater.update({.qtDirectories = {"/path"}});
}

TEST_F(ProjectStorageUpdater_synchronize_qmldir_imports, synchronize_qmldir_default_imports)
{
    QString qmldir{R"(module Example
                      import Qml auto
                      import QML 2.1
                      default import Quick
                      )"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({qmlDirPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::moduleExportedImports",
                                &SynchronizationPackage::moduleExportedImports,
                                UnorderedElementsAre(ModuleExportedImport{exampleModuleId,
                                                                          qmlModuleId,
                                                                          Storage::Version{},
                                                                          IsAutoVersion::Yes},
                                                     ModuleExportedImport{exampleCppNativeModuleId,
                                                                          qmlCppNativeModuleId,
                                                                          Storage::Version{},
                                                                          IsAutoVersion::No},
                                                     ModuleExportedImport{exampleModuleId,
                                                                          builtinModuleId,
                                                                          Storage::Version{2, 1},
                                                                          IsAutoVersion::No},
                                                     ModuleExportedImport{exampleCppNativeModuleId,
                                                                          builtinCppNativeModuleId,
                                                                          Storage::Version{},
                                                                          IsAutoVersion::No},
                                                     ModuleExportedImport{exampleModuleId,
                                                                          quickModuleId,
                                                                          Storage::Version{},
                                                                          IsAutoVersion::Yes},
                                                     ModuleExportedImport{exampleCppNativeModuleId,
                                                                          quickCppNativeModuleId,
                                                                          Storage::Version{},
                                                                          IsAutoVersion::No})),
                          Field("SynchronizationPackage::updatedModuleIds",
                                &SynchronizationPackage::updatedModuleIds,
                                ElementsAre(exampleModuleId, exampleCppNativeModuleId)))));

    updater.update({.qtDirectories = {"/path"}});
}

TEST_F(ProjectStorageUpdater_synchronize_qmldir_imports, do_not_synchronize_qmldir_optional_imports)
{
    QString qmldir{R"(module Example
                      import Qml auto
                      import QML 2.1
                      optional import Quick
                      )"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({qmlDirPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::moduleExportedImports",
                                &SynchronizationPackage::moduleExportedImports,
                                UnorderedElementsAre(ModuleExportedImport{exampleModuleId,
                                                                          qmlModuleId,
                                                                          Storage::Version{},
                                                                          IsAutoVersion::Yes},
                                                     ModuleExportedImport{exampleCppNativeModuleId,
                                                                          qmlCppNativeModuleId,
                                                                          Storage::Version{},
                                                                          IsAutoVersion::No},
                                                     ModuleExportedImport{exampleModuleId,
                                                                          builtinModuleId,
                                                                          Storage::Version{2, 1},
                                                                          IsAutoVersion::No},
                                                     ModuleExportedImport{exampleCppNativeModuleId,
                                                                          builtinCppNativeModuleId,
                                                                          Storage::Version{},
                                                                          IsAutoVersion::No})),
                          Field("SynchronizationPackage::updatedModuleIds",
                                &SynchronizationPackage::updatedModuleIds,
                                ElementsAre(exampleModuleId, exampleCppNativeModuleId)))));

    updater.update({.qtDirectories = {"/path"}});
}

class ProjectStorageUpdater_synchronize_qml_documents : public BaseProjectStorageUpdater
{
public:
    ProjectStorageUpdater_synchronize_qml_documents() {}

public:
    SourceId qmlDirPathSourceId = sourcePathCache.sourceId("/path/qmldir");
    DirectoryPathId directoryPathId = qmlDirPathSourceId.directoryPathId();
    DirectoryPathId rootDirectoryPathId = sourcePathCache.directoryPathId("/");
    SourceId directoryPathSourceId = SourceId::create(directoryPathId, QmlDesigner::FileNameId{});
    SourceId qmlDocument1SourceId = sourcePathCache.sourceId("/path/First.qml");
    SourceId qmlDocument1_2SourceId = sourcePathCache.sourceId("/path/First2.qml");
    SourceId qmlDocument2SourceId = sourcePathCache.sourceId("/path/Second.qml");
    ModuleId exampleModuleId = modulesStorage.moduleId("Example", ModuleKind::QmlLibrary);
    ModuleId qmlModuleId = modulesStorage.moduleId("QtQml", ModuleKind::QmlLibrary);
    ModuleId pathModuleId = modulesStorage.moduleId("/path", ModuleKind::PathLibrary);
    Storage::Import import1{qmlModuleId,
                            Storage::Version{2, 3},
                            qmlDocument1SourceId,
                            qmlDocument1SourceId};
    Storage::Import import1_2{qmlModuleId,
                              Storage::Version{},
                              qmlDocument1_2SourceId,
                              qmlDocument1_2SourceId};
    Storage::Import import2{qmlModuleId, Storage::Version{2}, qmlDocument2SourceId, qmlDocument2SourceId};
};

using ChangeQmlDocumentParameters = std::tuple<Update, FileState>;

class synchronize_changed_qml_documents
    : public ProjectStorageUpdater_synchronize_qml_documents,
      public testing::WithParamInterface<ChangeQmlDocumentParameters>
{
public:
    synchronize_changed_qml_documents()
        : update{std::get<0>(GetParam())}
        , state{std::get<1>(GetParam())}

    {
        setQmlFileNames(u"/path", {"First.qml", "First2.qml", "Second.qml"});

        QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
        setContent(u"/path/qmldir", qmldir);

        setContent(u"/path/First.qml", "First{}");
        setContent(u"/path/First2.qml", "First2{}");
        setContent(u"/path/Second.qml", "Second{}");

        ON_CALL(qmlDocumentParserMock, parse(_, _, qmlDocument1SourceId, _, _))
            .WillByDefault([&](auto, auto &imports, auto, auto, auto) {
                imports.push_back(import1);
                Type type;
                type.prototype = ImportedType{"Object"};
                type.traits = TypeTraitsKind::Reference;
                return type;
            });
        ON_CALL(qmlDocumentParserMock, parse(_, _, qmlDocument1_2SourceId, _, _))
            .WillByDefault([&](auto, auto &imports, auto, auto, auto) {
                imports.push_back(import1_2);
                Type type;
                type.prototype = ImportedType{"Object2"};
                type.traits = TypeTraitsKind::Reference;
                return type;
            });
        ON_CALL(qmlDocumentParserMock, parse(_, _, qmlDocument2SourceId, _, _))
            .WillByDefault([&](auto, auto &imports, auto, auto, auto) {
                imports.push_back(import2);
                Type type;
                type.prototype = ImportedType{"Object3"};
                type.traits = TypeTraitsKind::Reference;
                return type;
            });
    }

public:
    const Update &update;
    FileState state;
    bool isAdded = state == FileState::Added;
};

auto change_qml_document_parameters_printer =
    [](const testing::TestParamInfo<ChangeQmlDocumentParameters> &info) {
        std::string name = toString(std::get<1>(info.param));

        name += "_qml_documents_file_in_";

        if (std::get<0>(info.param).projectDirectory.isEmpty())
            name += "qt";
        else
            name += "project";
        return name;
    };

INSTANTIATE_TEST_SUITE_P(ProjectStorageUpdater,
                         synchronize_changed_qml_documents,
                         Combine(Values(Update{.qtDirectories = {"/path"}},
                                        Update{.projectDirectory = "/path"}),
                                 Values(FileState::Added, FileState::Changed)),
                         change_qml_document_parameters_printer);

TEST_P(synchronize_changed_qml_documents, imports)
{
    setFilesChanged({directoryPathSourceId, qmlDirPathSourceId});
    setFiles(state, {qmlDocument1SourceId, qmlDocument1_2SourceId, qmlDocument2SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::imports",
                                        &SynchronizationPackage::imports,
                                        UnorderedElementsAre(import1, import1_2, import2)),
                                  Field("SynchronizationPackage::updatedImportSourceIds",
                                        &SynchronizationPackage::updatedImportSourceIds,
                                        UnorderedElementsAre(qmlDocument1SourceId,
                                                             qmlDocument1_2SourceId,
                                                             qmlDocument2SourceId)))));

    updater.update(update);
}

TEST_P(synchronize_changed_qml_documents, types)
{
    setFilesChanged({directoryPathSourceId, qmlDirPathSourceId});
    setFiles(state, {qmlDocument1SourceId, qmlDocument1_2SourceId, qmlDocument2SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(AllOf(Field("SynchronizationPackage::types,",
                                      &SynchronizationPackage::types,
                                      UnorderedElementsAre(IsStorageType("First.qml",
                                                                         ImportedType{"Object"},
                                                                         TypeTraitsKind::Reference,
                                                                         qmlDocument1SourceId),
                                                           IsStorageType("First2.qml",
                                                                         ImportedType{"Object2"},
                                                                         TypeTraitsKind::Reference,
                                                                         qmlDocument1_2SourceId),
                                                           IsStorageType("Second.qml",
                                                                         ImportedType{"Object3"},
                                                                         TypeTraitsKind::Reference,
                                                                         qmlDocument2SourceId))),
                                Field("SynchronizationPackage::updatedTypeSourceIds",
                                      &SynchronizationPackage::updatedTypeSourceIds,
                                      UnorderedElementsAre(qmlDocument1SourceId,
                                                           qmlDocument1_2SourceId,
                                                           qmlDocument2SourceId))))));

    updater.update(update);
}

TEST_P(synchronize_changed_qml_documents, exported_types_from_qmldir)
{
    setFilesUnchanged({directoryPathSourceId});
    setFiles(state, {qmlDirPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::exportedTypes",
                                        &SynchronizationPackage::exportedTypes,
                                        UnorderedElementsAre(IsExportedType(exampleModuleId,
                                                                            "FirstType",
                                                                            1U,
                                                                            0U,
                                                                            qmlDirPathSourceId,
                                                                            "First.qml",
                                                                            qmlDocument1SourceId),
                                                             IsExportedType(exampleModuleId,
                                                                            "FirstType",
                                                                            2U,
                                                                            2U,
                                                                            qmlDirPathSourceId,
                                                                            "First2.qml",
                                                                            qmlDocument1_2SourceId),
                                                             IsExportedType(exampleModuleId,
                                                                            "SecondType",
                                                                            2U,
                                                                            2U,
                                                                            qmlDirPathSourceId,
                                                                            "Second.qml",
                                                                            qmlDocument2SourceId))),
                                  Field("SynchronizationPackage::updatedExportedTypeSourceIds",
                                        &SynchronizationPackage::updatedExportedTypeSourceIds,
                                        UnorderedElementsAre(qmlDirPathSourceId)))));

    updater.update(update);
}

TEST_P(synchronize_changed_qml_documents, exported_types_from_directory)
{
    setFilesUnchanged({qmlDirPathSourceId});
    setFiles(state, {directoryPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::exportedTypes",
                                        &SynchronizationPackage::exportedTypes,
                                        UnorderedElementsAre(IsExportedType(pathModuleId,
                                                                            "First",
                                                                            directoryPathSourceId,
                                                                            "First.qml",
                                                                            qmlDocument1SourceId),
                                                             IsExportedType(pathModuleId,
                                                                            "First2",
                                                                            directoryPathSourceId,
                                                                            "First2.qml",
                                                                            qmlDocument1_2SourceId),
                                                             IsExportedType(pathModuleId,
                                                                            "Second",
                                                                            directoryPathSourceId,
                                                                            "Second.qml",
                                                                            qmlDocument2SourceId))),
                                  Field("SynchronizationPackage::updatedExportedTypeSourceIds",
                                        &SynchronizationPackage::updatedExportedTypeSourceIds,
                                        UnorderedElementsAre(directoryPathSourceId)))));

    updater.update(update);
}

TEST_P(synchronize_changed_qml_documents, file_status)
{
    setFilesChanged({directoryPathSourceId, qmlDirPathSourceId});
    setFiles(state, {qmlDocument1SourceId, qmlDocument1_2SourceId, qmlDocument2SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::updatedFileStatusSourceIds",
                                        &SynchronizationPackage::updatedFileStatusSourceIds,
                                        IsSupersetOf({qmlDocument1SourceId,
                                                      qmlDocument1_2SourceId,
                                                      qmlDocument2SourceId})),
                                  Field("SynchronizationPackage::fileStatuses",
                                        &SynchronizationPackage::fileStatuses,
                                        IsSupersetOf({IsFileStatus(qmlDocument1SourceId, 1, 21),
                                                      IsFileStatus(qmlDocument1_2SourceId, 1, 21),
                                                      IsFileStatus(qmlDocument2SourceId, 1, 21)})))));

    updater.update(update);
}

TEST_P(synchronize_changed_qml_documents, project_entry_infos)
{
    setFilesChanged({directoryPathSourceId, qmlDirPathSourceId});
    setFiles(state, {qmlDocument1SourceId, qmlDocument1_2SourceId, qmlDocument2SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::updatedProjectEntryInfoSourceIds",
                                        &SynchronizationPackage::updatedProjectEntryInfoSourceIds,
                                        UnorderedElementsAre(directoryPathSourceId)),
                                  Field("SynchronizationPackage::projectEntryInfos",
                                        &SynchronizationPackage::projectEntryInfos,
                                        IsSupersetOf({IsProjectEntryInfo(directoryPathSourceId,
                                                                      qmlDocument1SourceId,
                                                                      ModuleId{},
                                                                      FileType::QmlDocument),
                                                      IsProjectEntryInfo(directoryPathSourceId,
                                                                      qmlDocument1_2SourceId,
                                                                      ModuleId{},
                                                                      FileType::QmlDocument),
                                                      IsProjectEntryInfo(directoryPathSourceId,
                                                                      qmlDocument2SourceId,
                                                                      ModuleId{},
                                                                      FileType::QmlDocument)})))));

    updater.update(update);
}

TEST_P(synchronize_changed_qml_documents, with_missing_module_name_directory_name_is_used)
{
    QString qmldir{R"(FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);
    ModuleId directoryNameModuleId{modulesStorage.moduleId("path", ModuleKind::QmlLibrary)};
    setFilesUnchanged({directoryPathSourceId});
    setFiles(state, {qmlDirPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(Field("SynchronizationPackage::exportedTypes",
                                  &SynchronizationPackage::exportedTypes,
                                  Each(Field("ExportedType::moduleId",
                                             &ExportedType::moduleId,
                                             directoryNameModuleId)))));

    updater.update(update);
}

TEST_P(synchronize_changed_qml_documents, skip_duplicate_exported_type_names)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 1.0 First.qml
                      FirstType 2.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setFileSystemSubdirectories(u"/path", {"/path/sub"});
    ModuleId subPathModuleId = modulesStorage.moduleId("/path/sub", ModuleKind::PathLibrary);
    SourceId qmlDocument1_subSourceId = sourcePathCache.sourceId("/path/sub/First.qml");
    SourceId subDirectoryPathSourceId = SourceId::create(qmlDocument1_subSourceId.directoryPathId());
    setQmlFileNames(u"/path/sub", {"First.qml"});
    setFilesChanged({directoryPathSourceId, qmlDirPathSourceId, subDirectoryPathSourceId});
    setFiles(state,
             {qmlDocument1SourceId,
              qmlDocument1_2SourceId,
              qmlDocument2SourceId,
              qmlDocument1_subSourceId});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(Field(
            "SynchronizationPackage::exportedTypes",
            &SynchronizationPackage::exportedTypes,
            UnorderedElementsAre(
                IsExportedType(exampleModuleId, "FirstType", 1U, 0U, qmlDirPathSourceId, "First.qml", qmlDocument1SourceId),
                IsExportedType(exampleModuleId, "FirstType", 2U, 0U, qmlDirPathSourceId, "First.qml", qmlDocument1SourceId),
                IsExportedType(exampleModuleId, "FirstType", 2U, 2U, qmlDirPathSourceId, "First2.qml", qmlDocument1_2SourceId),
                IsExportedType(exampleModuleId, "SecondType", 2U, 2U, qmlDirPathSourceId, "Second.qml", qmlDocument2SourceId),
                IsExportedType(pathModuleId, "First", directoryPathSourceId, "First.qml", qmlDocument1SourceId),
                IsExportedType(subPathModuleId, "First", subDirectoryPathSourceId, "First.qml", qmlDocument1_subSourceId),
                IsExportedType(pathModuleId, "First2", directoryPathSourceId, "First2.qml", qmlDocument1_2SourceId),
                IsExportedType(
                    pathModuleId, "Second", directoryPathSourceId, "Second.qml", qmlDocument2SourceId)))));

    updater.update({.projectDirectory = "/path"});
}

TEST_P(synchronize_changed_qml_documents, types_only_in_directory)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml)"};
    setContent(u"/path/qmldir", qmldir);
    if (isAdded)
        setFilesChanged({directoryPathSourceId});
    else
        setFilesUnchanged({directoryPathSourceId});
    setFilesUnchanged({qmlDirPathSourceId, qmlDocument1SourceId});
    setFiles(state, {qmlDocument1_2SourceId});
    if (isAdded) {
        setQmlDocumentProjectEntryInfos(
            directoryPathSourceId,
            {{directoryPathSourceId, qmlDocument1SourceId, ModuleId{}, FileType::QmlDocument}});
    } else {
        setQmlDocumentProjectEntryInfos(
            directoryPathSourceId,
            {{directoryPathSourceId, qmlDocument1SourceId, ModuleId{}, FileType::QmlDocument},
             {directoryPathSourceId, qmlDocument1_2SourceId, ModuleId{}, FileType::QmlDocument}});
    }
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::types",
                                        &SynchronizationPackage::types,
                                        ElementsAre(IsStorageType("First2.qml",
                                                                  ImportedType{"Object2"},
                                                                  TypeTraitsKind::Reference,
                                                                  qmlDocument1_2SourceId))),
                                  Field("SynchronizationPackage::updatedTypeSourceIds",
                                        &SynchronizationPackage::updatedTypeSourceIds,
                                        ElementsAre(qmlDocument1_2SourceId)))));

    updater.update(update);
}

TEST_P(synchronize_changed_qml_documents, project_entry_infos_for_one_document)
{
    if (isAdded)
        setFilesChanged({directoryPathSourceId});
    else
        setFilesUnchanged({directoryPathSourceId});
    if (isAdded)
        setQmlDocumentProjectEntryInfos(
            directoryPathSourceId,
            {{directoryPathSourceId, qmlDocument1SourceId, ModuleId{}, FileType::QmlDocument},
             {directoryPathSourceId, qmlDocument2SourceId, ModuleId{}, FileType::QmlDocument}});
    else {
        setQmlDocumentProjectEntryInfos(
            directoryPathSourceId,
            {{directoryPathSourceId, qmlDocument1SourceId, ModuleId{}, FileType::QmlDocument},
             {directoryPathSourceId, qmlDocument1_2SourceId, ModuleId{}, FileType::QmlDocument},
             {directoryPathSourceId, qmlDocument2SourceId, ModuleId{}, FileType::QmlDocument}});
    }
    setFilesUnchanged({qmlDocument1SourceId, qmlDocument2SourceId, qmlDirPathSourceId});
    setFiles(state, {qmlDocument1_2SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(
                    Field("SynchronizationPackage::projectEntryInfos",
                          &SynchronizationPackage::projectEntryInfos,
                          Conditional(isAdded,
                                      UnorderedElementsAre(IsProjectEntryInfo(directoryPathSourceId,
                                                                           qmlDocument1SourceId,
                                                                           ModuleId{},
                                                                           FileType::QmlDocument),
                                                           IsProjectEntryInfo(directoryPathSourceId,
                                                                           qmlDocument1_2SourceId,
                                                                           ModuleId{},
                                                                           FileType::QmlDocument),
                                                           IsProjectEntryInfo(directoryPathSourceId,
                                                                           qmlDocument2SourceId,
                                                                           ModuleId{},
                                                                           FileType::QmlDocument)),
                                      IsEmpty())),
                    Field("SynchronizationPackage::updatedProjectEntryInfoSourceIds",
                          &SynchronizationPackage::updatedProjectEntryInfoSourceIds,
                          Conditional(isAdded, ElementsAre(directoryPathSourceId), IsEmpty())))));

    updater.update(update);
}

TEST_P(synchronize_changed_qml_documents, file_status_for_one_document)
{
    if (isAdded)
        setFilesChanged({directoryPathSourceId});
    else
        setFilesUnchanged({directoryPathSourceId});
    if (isAdded)
        setQmlDocumentProjectEntryInfos(
            directoryPathSourceId,
            {{directoryPathSourceId, qmlDocument1SourceId, ModuleId{}, FileType::QmlDocument},
             {directoryPathSourceId, qmlDocument2SourceId, ModuleId{}, FileType::QmlDocument}});
    else {
        setQmlDocumentProjectEntryInfos(
            directoryPathSourceId,
            {{directoryPathSourceId, qmlDocument1SourceId, ModuleId{}, FileType::QmlDocument},
             {directoryPathSourceId, qmlDocument1_2SourceId, ModuleId{}, FileType::QmlDocument},
             {directoryPathSourceId, qmlDocument2SourceId, ModuleId{}, FileType::QmlDocument}});
    }
    setFilesUnchanged({qmlDocument1SourceId, qmlDocument2SourceId, qmlDirPathSourceId});
    setFiles(state, {qmlDocument1_2SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::fileStatuses",
                                &SynchronizationPackage::fileStatuses,
                                AllOf(Contains(IsFileStatus(directoryPathSourceId, 1, 21))
                                          .Times(isAdded ? 1 : 0),
                                      Contains(IsFileStatus(qmlDocument1_2SourceId, 1, 21)),
                                      Not(Contains(IsFileStatus(qmlDocument1SourceId, 1, 21))))),
                          Field("SynchronizationPackage::updatedFileStatusSourceIds",
                                &SynchronizationPackage::updatedFileStatusSourceIds,
                                AllOf(Contains(directoryPathSourceId).Times(isAdded ? 1 : 0),
                                      Contains(qmlDocument1_2SourceId),
                                      Not(Contains(qmlDocument1SourceId)))))));

    updater.update(update);
}

TEST_P(synchronize_changed_qml_documents, exported_types_in_qmldir_only)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml)"};
    setContent(u"/path/qmldir", qmldir);
    if (isAdded)
        setFilesChanged({directoryPathSourceId});
    else
        setFilesUnchanged({directoryPathSourceId});
    setFilesUnchanged({qmlDocument1SourceId});
    setFiles(state, {qmlDirPathSourceId});
    if (!isAdded) {
        setQmlDocumentProjectEntryInfos(
            directoryPathSourceId,
            {{directoryPathSourceId, qmlDocument1SourceId, ModuleId{}, FileType::QmlDocument}});
    }
    setQmlFileNames(u"/path", {"First.qml"});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(
                    Field("SynchronizationPackage::exportedTypes",
                          &SynchronizationPackage::exportedTypes,
                          Conditional(isAdded,
                                      UnorderedElementsAre(IsExportedType(pathModuleId,
                                                                          "First",
                                                                          directoryPathSourceId,
                                                                          "First.qml",
                                                                          qmlDocument1SourceId),
                                                           IsExportedType(exampleModuleId,
                                                                          "FirstType",
                                                                          1U,
                                                                          0U,
                                                                          qmlDirPathSourceId,
                                                                          "First.qml",
                                                                          qmlDocument1SourceId)),
                                      UnorderedElementsAre(IsExportedType(exampleModuleId,
                                                                          "FirstType",
                                                                          1U,
                                                                          0U,
                                                                          qmlDirPathSourceId,
                                                                          "First.qml",
                                                                          qmlDocument1SourceId)))),
                    Field("SynchronizationPackage::updatedExportedTypeSourceIds",
                          &SynchronizationPackage::updatedExportedTypeSourceIds,
                          Conditional(isAdded,
                                      UnorderedElementsAre(directoryPathSourceId, qmlDirPathSourceId),
                                      ElementsAre(qmlDirPathSourceId))))));

    updater.update(update);
}

TEST_P(synchronize_changed_qml_documents, file_status_in_qmldir_only)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml)"};
    setContent(u"/path/qmldir", qmldir);
    if (isAdded)
        setFilesChanged({directoryPathSourceId});
    else
        setFilesUnchanged({directoryPathSourceId});
    setFilesUnchanged({qmlDocument1SourceId});
    setFiles(state, {qmlDirPathSourceId});
    if (!isAdded) {
        setQmlDocumentProjectEntryInfos(
            directoryPathSourceId,
            {{directoryPathSourceId, qmlDocument1SourceId, ModuleId{}, FileType::QmlDocument}});
    }
    setQmlFileNames(u"/path", {"First.qml"});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::updatedFileStatusSourceIds",
                                        &SynchronizationPackage::updatedFileStatusSourceIds,
                                        Not(Contains(qmlDocument1SourceId))),
                                  Field("SynchronizationPackage::fileStatuses",
                                        &SynchronizationPackage::fileStatuses,
                                        Not(Contains(Field("FileStatus::sourceId",
                                                           &FileStatus::sourceId,
                                                           qmlDocument1SourceId)))))));
    updater.update(update);
}

TEST_P(synchronize_changed_qml_documents, project_entry_infos_in_qmldir_only)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml)"};
    setContent(u"/path/qmldir", qmldir);
    if (isAdded)
        setFilesChanged({directoryPathSourceId});
    else
        setFilesUnchanged({directoryPathSourceId});
    setFilesUnchanged({qmlDocument1SourceId});
    setFiles(state, {qmlDirPathSourceId});
    if (!isAdded) {
        setQmlDocumentProjectEntryInfos(
            directoryPathSourceId,
            {{directoryPathSourceId, qmlDocument1SourceId, ModuleId{}, FileType::QmlDocument}});
    }
    setQmlFileNames(u"/path", {"First.qml"});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::projectEntryInfos",
                                        &SynchronizationPackage::projectEntryInfos,
                                        Contains(IsProjectEntryInfo(directoryPathSourceId,
                                                                 qmlDocument1SourceId,
                                                                 ModuleId{},
                                                                 FileType::QmlDocument))
                                            .Times(isAdded ? 1 : 0)),
                                  Field("SynchronizationPackage::updatedProjectEntryInfoSourceIds",
                                        &SynchronizationPackage::updatedProjectEntryInfoSourceIds,
                                        Contains(directoryPathSourceId).Times(isAdded ? 1 : 0)))));

    updater.update(update);
}

TEST_P(synchronize_changed_qml_documents, types_without_qmldir)
{
    if (isAdded)
        setFilesChanged({directoryPathSourceId});
    else
        setFilesUnchanged({directoryPathSourceId});
    setFilesNotExistsUnchanged({qmlDirPathSourceId});
    setFilesUnchanged({qmlDocument1SourceId});
    setFiles(state, {qmlDocument1_2SourceId});
    if (isAdded) {
        setQmlDocumentProjectEntryInfos(
            directoryPathSourceId,
            {{directoryPathSourceId, qmlDocument1SourceId, ModuleId{}, FileType::QmlDocument}});
    } else {
        setQmlDocumentProjectEntryInfos(
            directoryPathSourceId,
            {{directoryPathSourceId, qmlDocument1SourceId, ModuleId{}, FileType::QmlDocument},
             {directoryPathSourceId, qmlDocument1_2SourceId, ModuleId{}, FileType::QmlDocument}});
    }
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::types",
                                        &SynchronizationPackage::types,
                                        ElementsAre(AllOf(IsStorageType("First2.qml",
                                                                        ImportedType{"Object2"},
                                                                        TypeTraitsKind::Reference,
                                                                        qmlDocument1_2SourceId)))),
                                  Field("SynchronizationPackage::updatedTypeSourceIds",
                                        &SynchronizationPackage::updatedTypeSourceIds,
                                        ElementsAre(qmlDocument1_2SourceId)))));

    updater.update(update);
}

TEST_P(synchronize_changed_qml_documents, exported_types_without_qmldir)
{
    if (isAdded)
        setFilesChanged({directoryPathSourceId});
    else
        setFilesUnchanged({directoryPathSourceId});
    setFilesNotExistsUnchanged({qmlDirPathSourceId});
    setFilesUnchanged({qmlDocument1SourceId});
    setFiles(state, {qmlDocument1_2SourceId});
    if (isAdded) {
        setQmlDocumentProjectEntryInfos(
            directoryPathSourceId,
            {{directoryPathSourceId, qmlDocument1SourceId, ModuleId{}, FileType::QmlDocument}});
    } else {
        setQmlDocumentProjectEntryInfos(
            directoryPathSourceId,
            {{directoryPathSourceId, qmlDocument1SourceId, ModuleId{}, FileType::QmlDocument},
             {directoryPathSourceId, qmlDocument1_2SourceId, ModuleId{}, FileType::QmlDocument}});
    }
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(
                    Field("SynchronizationPackage::exportedTypes",
                          &SynchronizationPackage::exportedTypes,
                          Conditional(isAdded,
                                      UnorderedElementsAre(IsExportedType(pathModuleId,
                                                                          "First",
                                                                          directoryPathSourceId,
                                                                          "First.qml",
                                                                          qmlDocument1SourceId),
                                                           IsExportedType(pathModuleId,
                                                                          "First2",
                                                                          directoryPathSourceId,
                                                                          "First2.qml",
                                                                          qmlDocument1_2SourceId)),
                                      IsEmpty())),
                    Field("SynchronizationPackage::updatedTypeSourceIds",
                          &SynchronizationPackage::updatedExportedTypeSourceIds,
                          Conditional(isAdded, ElementsAre(directoryPathSourceId), IsEmpty())))));

    updater.update(update);
}

TEST_P(synchronize_changed_qml_documents, project_entry_infos_without_qmldir)
{
    if (isAdded)
        setFilesChanged({directoryPathSourceId});
    else
        setFilesUnchanged({directoryPathSourceId});
    setFilesNotExistsUnchanged({qmlDirPathSourceId});
    setFilesUnchanged({qmlDocument1SourceId});
    setFiles(state, {qmlDocument1_2SourceId});
    if (isAdded) {
        setQmlDocumentProjectEntryInfos(
            directoryPathSourceId,
            {{directoryPathSourceId, qmlDocument1SourceId, ModuleId{}, FileType::QmlDocument}});
    } else {
        setQmlDocumentProjectEntryInfos(
            directoryPathSourceId,
            {{directoryPathSourceId, qmlDocument1SourceId, ModuleId{}, FileType::QmlDocument},
             {directoryPathSourceId, qmlDocument1_2SourceId, ModuleId{}, FileType::QmlDocument}});
    }
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::updatedProjectEntryInfoSourceIds",
                                &SynchronizationPackage::updatedProjectEntryInfoSourceIds,
                                Conditional(isAdded,
                                            UnorderedElementsAre(directoryPathSourceId),
                                            Not(Contains(directoryPathSourceId)))),
                          Field("SynchronizationPackage::projectEntryInfos",
                                &SynchronizationPackage::projectEntryInfos,
                                Conditional(isAdded,
                                            IsSupersetOf({IsProjectEntryInfo(directoryPathSourceId,
                                                                          qmlDocument1SourceId,
                                                                          ModuleId{},
                                                                          FileType::QmlDocument),
                                                          IsProjectEntryInfo(directoryPathSourceId,
                                                                          qmlDocument1_2SourceId,
                                                                          ModuleId{},
                                                                          FileType::QmlDocument)}),
                                            Not(Contains(Field("ProjectEntryInfo::contextSourceId",
                                                               &ProjectEntryInfo::contextSourceId,
                                                               directoryPathSourceId))))))));

    updater.update(update);
}

using NotExistingQmlDocumentParameters = std::tuple<Update, FileState>;

class synchronize_not_existing_qml_documents
    : public ProjectStorageUpdater_synchronize_qml_documents,
      public testing::WithParamInterface<NotExistingQmlDocumentParameters>
{
public:
    synchronize_not_existing_qml_documents()
        : update{std::get<0>(GetParam())}
        , state{std::get<1>(GetParam())}

    {
        QString qmldir{R"(module Example)"};
        setContent(u"/path/qmldir", qmldir);

        setFilesNotExistsUnchanged({createDirectorySourceId("/path/designer")});

        if (isRemoved) {
            setQmlDocumentProjectEntryInfos(
                directoryPathSourceId,
                {{directoryPathSourceId, qmlDocument1SourceId, ModuleId{}, FileType::QmlDocument},
                 {directoryPathSourceId, qmlDocument1_2SourceId, ModuleId{}, FileType::QmlDocument},
                 {directoryPathSourceId, qmlDocument2SourceId, ModuleId{}, FileType::QmlDocument}});
        }
    }

public:
    const Update &update;
    FileState state;
    bool isRemoved = state == FileState::Removed;
    bool isChanged = state != FileState::NotExistsUnchanged;
};

auto not_existing_qml_document_parameters_printer =
    [](const testing::TestParamInfo<NotExistingQmlDocumentParameters> &info) {
        std::string name = toString(std::get<1>(info.param));

        name += "_qml_documents_file_in_";

        if (std::get<0>(info.param).projectDirectory.isEmpty())
            name += "qt";
        else
            name += "project";
        return name;
    };

INSTANTIATE_TEST_SUITE_P(
    ProjectStorageUpdater,
    synchronize_not_existing_qml_documents,
    Combine(Values(Update{.qtDirectories = {"/path"}}, Update{.projectDirectory = "/path"}),
            Values(FileState::Removed, FileState::NotExists, FileState::NotExistsUnchanged)),
    not_existing_qml_document_parameters_printer);

TEST_P(synchronize_not_existing_qml_documents, imports)
{
    if (isRemoved)
        setFilesChanged({directoryPathSourceId, qmlDirPathSourceId});
    else
        setFilesUnchanged({directoryPathSourceId, qmlDirPathSourceId});
    setFiles(state, {qmlDocument1SourceId, qmlDocument1_2SourceId, qmlDocument2SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::imports",
                                        &SynchronizationPackage::imports,
                                        IsEmpty()),
                                  Field("SynchronizationPackage::updatedImportSourceIds",
                                        &SynchronizationPackage::updatedImportSourceIds,
                                        Conditional(isRemoved,
                                                    UnorderedElementsAre(qmlDocument1SourceId,
                                                                         qmlDocument1_2SourceId,
                                                                         qmlDocument2SourceId),
                                                    IsEmpty())))));

    updater.update(update);
}

TEST_P(synchronize_not_existing_qml_documents, types)
{
    if (isRemoved)
        setFilesChanged({directoryPathSourceId, qmlDirPathSourceId});
    else
        setFilesUnchanged({directoryPathSourceId, qmlDirPathSourceId});
    setFiles(state, {qmlDocument1SourceId, qmlDocument1_2SourceId, qmlDocument2SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::types",
                                        &SynchronizationPackage::types,
                                        IsEmpty()),
                                  Field("SynchronizationPackage::updatedTypeSourceIds",
                                        &SynchronizationPackage::updatedTypeSourceIds,
                                        Conditional(isRemoved,
                                                    UnorderedElementsAre(qmlDocument1SourceId,
                                                                         qmlDocument1_2SourceId,
                                                                         qmlDocument2SourceId),
                                                    IsEmpty())))));

    updater.update(update);
}

TEST_P(synchronize_not_existing_qml_documents, file_status)
{
    if (isRemoved)
        setFilesChanged({directoryPathSourceId, qmlDirPathSourceId});
    else
        setFilesUnchanged({directoryPathSourceId, qmlDirPathSourceId});
    setFiles(state, {qmlDocument1SourceId, qmlDocument1_2SourceId, qmlDocument2SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::updatedFileStatusSourceIds",
                                        &SynchronizationPackage::updatedFileStatusSourceIds,
                                        Conditional(isRemoved,
                                                    IsSupersetOf({qmlDocument1SourceId,
                                                                  qmlDocument1_2SourceId,
                                                                  qmlDocument2SourceId}),
                                                    Not(Contains(AnyOf(qmlDocument1SourceId,
                                                                       qmlDocument1_2SourceId,
                                                                       qmlDocument2SourceId))))),
                                  Field("SynchronizationPackage::fileStatuses",
                                        &SynchronizationPackage::fileStatuses,
                                        Not(Contains(Field("FileStatus::sourceId",
                                                           &FileStatus::sourceId,
                                                           AnyOf(qmlDocument1SourceId,
                                                                 qmlDocument1_2SourceId,
                                                                 qmlDocument2SourceId))))))));

    updater.update(update);
}

TEST_P(synchronize_not_existing_qml_documents, project_entry_infos)
{
    if (isRemoved)
        setFilesChanged({directoryPathSourceId, qmlDirPathSourceId});
    else
        setFilesUnchanged({directoryPathSourceId, qmlDirPathSourceId});
    setFiles(state, {qmlDocument1SourceId, qmlDocument1_2SourceId, qmlDocument2SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::updatedProjectEntryInfoSourceIds",
                                        &SynchronizationPackage::updatedProjectEntryInfoSourceIds,
                                        Contains(directoryPathSourceId).Times(isRemoved ? 1 : 0)),
                                  Field("SynchronizationPackage::projectEntryInfos",
                                        &SynchronizationPackage::projectEntryInfos,
                                        Not(Contains(Field("ProjectEntryInfo::sourceId",
                                                           &ProjectEntryInfo::sourceId,
                                                           AnyOf(qmlDocument1SourceId,
                                                                 qmlDocument1_2SourceId,
                                                                 qmlDocument2SourceId))))))));

    updater.update(update);
}

TEST_P(synchronize_not_existing_qml_documents, types_only_in_directory)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml)"};
    setContent(u"/path/qmldir", qmldir);
    if (isRemoved)
        setFilesChanged({directoryPathSourceId});
    else
        setFilesUnchanged({directoryPathSourceId});
    setFilesUnchanged({qmlDirPathSourceId, qmlDocument1SourceId});
    setFiles(state, {qmlDocument1_2SourceId});
    if (isRemoved) {
        setQmlDocumentProjectEntryInfos(
            directoryPathSourceId,
            {{directoryPathSourceId, qmlDocument1SourceId, ModuleId{}, FileType::QmlDocument},
             {directoryPathSourceId, qmlDocument1_2SourceId, ModuleId{}, FileType::QmlDocument}});
    } else {
        setQmlDocumentProjectEntryInfos(
            directoryPathSourceId,
            {{directoryPathSourceId, qmlDocument1SourceId, ModuleId{}, FileType::QmlDocument}});
    }
    setQmlFileNames(u"/path", {"First.qml"});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(
                    Field("SynchronizationPackage::types", &SynchronizationPackage::types, IsEmpty()),
                    Field("SynchronizationPackage::updatedTypeSourceIds",
                          &SynchronizationPackage::updatedTypeSourceIds,
                          Conditional(isRemoved, ElementsAre(qmlDocument1_2SourceId), IsEmpty())))));

    updater.update(update);
}

TEST_P(synchronize_not_existing_qml_documents, exported_types_for_directory)
{
    setQmlFileNames(u"/path", {"First.qml", "Second.qml"});
    if (isRemoved)
        setFilesChanged({directoryPathSourceId});
    else
        setFilesUnchanged({directoryPathSourceId});
    setFiles(state, {qmlDocument1_2SourceId});
    setFilesUnchanged({qmlDocument1SourceId, qmlDirPathSourceId, qmlDocument2SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(
                    Field("SynchronizationPackage::exportedTypes",
                          &SynchronizationPackage::exportedTypes,
                          Conditional(isRemoved,
                                      UnorderedElementsAre(IsExportedType(pathModuleId,
                                                                          "First",
                                                                          directoryPathSourceId,
                                                                          "First.qml",
                                                                          qmlDocument1SourceId),
                                                           IsExportedType(pathModuleId,
                                                                          "Second",
                                                                          directoryPathSourceId,
                                                                          "Second.qml",
                                                                          qmlDocument2SourceId)),
                                      IsEmpty())),
                    Field("SynchronizationPackage::updatedExportedTypeSourceIds",
                          &SynchronizationPackage::updatedExportedTypeSourceIds,
                          Conditional(isRemoved, ElementsAre(directoryPathSourceId), IsEmpty())))));

    updater.update(update);
}

TEST_P(synchronize_not_existing_qml_documents, exported_types_for_qmldir)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setQmlFileNames(u"/path", {"First2.qml"});
    setFilesUnchanged({directoryPathSourceId});
    setFilesUnchanged({qmlDocument1SourceId, qmlDocument1_2SourceId, qmlDocument2SourceId});
    setFiles(state, {qmlDirPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::exportedTypes",
                                &SynchronizationPackage::exportedTypes,
                                IsEmpty()),
                          Field("SynchronizationPackage::updatedTypeSourceIds",
                                &SynchronizationPackage::updatedExportedTypeSourceIds,
                                Conditional(isRemoved, ElementsAre(qmlDirPathSourceId), IsEmpty())))));

    updater.update(update);
}

TEST_P(synchronize_not_existing_qml_documents, file_status_in_qmldir_only)
{
    setQmlFileNames(u"/path", {"First.qml", "First2.qml", "Second.qml"});
    setFiles(state, {qmlDirPathSourceId});
    if (isRemoved)
        setFilesChanged({directoryPathSourceId});
    else
        setFilesUnchanged({directoryPathSourceId});
    setFilesUnchanged({qmlDocument1SourceId, qmlDocument1_2SourceId, qmlDocument2SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::updatedFileStatusSourceIds",
                                        &SynchronizationPackage::updatedFileStatusSourceIds,
                                        Not(Contains(AnyOf(qmlDocument1SourceId,
                                                           qmlDocument1_2SourceId,
                                                           qmlDocument2SourceId)))),
                                  Field("SynchronizationPackage::fileStatuses",
                                        &SynchronizationPackage::fileStatuses,
                                        Not(Contains(Field("FileStatus::sourceId",
                                                           &FileStatus::sourceId,
                                                           AnyOf(qmlDocument1SourceId,
                                                                 qmlDocument1_2SourceId,
                                                                 qmlDocument2SourceId))))))));

    updater.update(update);
}

TEST_P(synchronize_not_existing_qml_documents, types_if_directory_does_not_exists_too)
{
    setFiles(state,
             {qmlDirPathSourceId,
              directoryPathSourceId,
              qmlDocument1SourceId,
              qmlDocument1_2SourceId,
              qmlDocument2SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::types",
                                        &SynchronizationPackage::types,
                                        IsEmpty()),
                                  Field("SynchronizationPackage::updatedTypeSourceIds",
                                        &SynchronizationPackage::updatedTypeSourceIds,
                                        Conditional(isRemoved,
                                                    UnorderedElementsAre(qmlDocument1SourceId,
                                                                         qmlDocument1_2SourceId,
                                                                         qmlDocument2SourceId),
                                                    IsEmpty())))));

    updater.update(update);
}

TEST_P(synchronize_not_existing_qml_documents, file_status_if_directory_does_not_exists_too)
{
    setFiles(state,
             {qmlDirPathSourceId,
              directoryPathSourceId,
              qmlDocument1SourceId,
              qmlDocument1_2SourceId,
              qmlDocument2SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::fileStatuses",
                                        &SynchronizationPackage::fileStatuses,
                                        Not(Contains(Field("FileStatus::sourceId",
                                                           &FileStatus::sourceId,
                                                           AnyOf(qmlDocument1SourceId,
                                                                 qmlDocument1_2SourceId,
                                                                 qmlDocument2SourceId))))),
                                  Field("SynchronizationPackage::updatedFileStatusSourceIds",
                                        &SynchronizationPackage::updatedFileStatusSourceIds,
                                        Conditional(isRemoved,
                                                    IsSupersetOf({qmlDocument1SourceId,
                                                                  qmlDocument1_2SourceId,
                                                                  qmlDocument2SourceId}),
                                                    Not(Contains(AnyOf(qmlDocument1SourceId,
                                                                       qmlDocument1_2SourceId,
                                                                       qmlDocument2SourceId))))))));

    updater.update(update);
}

TEST_P(synchronize_not_existing_qml_documents, project_entry_infos_if_directory_does_not_exists_too)
{
    setFiles(state,
             {qmlDirPathSourceId,
              directoryPathSourceId,
              qmlDocument1SourceId,
              qmlDocument1_2SourceId,
              qmlDocument2SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::projectEntryInfos",
                                &SynchronizationPackage::projectEntryInfos,
                                IsEmpty()),
                          Field("SynchronizationPackage::updatedProjectEntryInfoSourceIds",
                                &SynchronizationPackage::updatedProjectEntryInfoSourceIds,
                                Conditional(isRemoved,
                                            ElementsAre(directoryPathSourceId, qmlDirPathSourceId),
                                            IsEmpty())))));

    updater.update(update);
}

TEST_P(synchronize_not_existing_qml_documents, types_without_qmldir)
{
    setFilesNotExistsUnchanged({qmlDirPathSourceId});
    if (isRemoved)
        setFilesChanged({directoryPathSourceId, qmlDirPathSourceId});
    else
        setFilesUnchanged({directoryPathSourceId, qmlDirPathSourceId});
    setFiles(state, {qmlDocument1SourceId, qmlDocument1_2SourceId, qmlDocument2SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field(&SynchronizationPackage::types, IsEmpty()),
                                  Field("SynchronizationPackage::updatedTypeSourceIds",
                                        &SynchronizationPackage::updatedTypeSourceIds,
                                        Conditional(isRemoved,
                                                    IsSupersetOf({qmlDocument1SourceId,
                                                                  qmlDocument1_2SourceId,
                                                                  qmlDocument2SourceId}),
                                                    Not(Contains(AnyOf(qmlDocument1SourceId,
                                                                       qmlDocument1_2SourceId,
                                                                       qmlDocument2SourceId))))))));

    updater.update(update);
}

TEST_P(synchronize_not_existing_qml_documents, project_entry_infos_without_qmldir)
{
    setFilesNotExistsUnchanged({qmlDirPathSourceId});
    if (isRemoved)
        setFilesChanged({directoryPathSourceId, qmlDirPathSourceId});
    else
        setFilesUnchanged({directoryPathSourceId, qmlDirPathSourceId});
    setFiles(state, {qmlDocument1SourceId, qmlDocument1_2SourceId, qmlDocument2SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::updatedProjectEntryInfoSourceIds",
                                        &SynchronizationPackage::updatedProjectEntryInfoSourceIds,
                                        Contains(directoryPathSourceId).Times(isRemoved ? 1 : 0)),
                                  Field("SynchronizationPackage::projectEntryInfos",
                                        &SynchronizationPackage::projectEntryInfos,
                                        Not(Contains(Field("ProjectEntryInfo::sourceId",
                                                           &ProjectEntryInfo::sourceId,
                                                           AnyOf(qmlDocument1SourceId,
                                                                 qmlDocument1_2SourceId,
                                                                 qmlDocument2SourceId))))))));

    updater.update(update);
}

TEST_F(ProjectStorageUpdater_synchronize_qml_documents,
       with_different_version_but_same_type_name_and_file_name)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 1.1 First.qml
                      FirstType 6.0 First.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({qmlDirPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(Field("SynchronizationPackage::exportedTypes",
                                  &SynchronizationPackage::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(exampleModuleId,
                                                                      "FirstType",
                                                                      1U,
                                                                      0U,
                                                                      qmlDirPathSourceId,
                                                                      "First.qml",
                                                                      qmlDocument1SourceId),
                                                       IsExportedType(exampleModuleId,
                                                                      "FirstType",
                                                                      1U,
                                                                      1U,
                                                                      qmlDirPathSourceId,
                                                                      "First.qml",
                                                                      qmlDocument1SourceId),
                                                       IsExportedType(exampleModuleId,
                                                                      "FirstType",
                                                                      6U,
                                                                      0U,
                                                                      qmlDirPathSourceId,
                                                                      "First.qml",
                                                                      qmlDocument1SourceId)))));

    updater.update({.projectDirectory = "/path"});
}

TEST_F(ProjectStorageUpdater_synchronize_qml_documents, with_relatives_paths)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 foo/../First.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({qmlDirPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(Field("SynchronizationPackage::exportedTypes",
                                  &SynchronizationPackage::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(exampleModuleId,
                                                                      "FirstType",
                                                                      1U,
                                                                      0U,
                                                                      qmlDirPathSourceId,
                                                                      "First.qml",
                                                                      qmlDocument1SourceId)))));

    updater.update({.projectDirectory = "/path"});
}

TEST_F(ProjectStorageUpdater_synchronize_qml_documents,
       with_different_type_name_but_same_version_and_file_name)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType2 1.0 First.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setQmlFileNames(u"/path", {"First.qml"});
    setFilesChanged({qmlDirPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::exportedTypes",
                                        &SynchronizationPackage::exportedTypes,
                                        UnorderedElementsAre(IsExportedType(exampleModuleId,
                                                                            "FirstType",
                                                                            1U,
                                                                            0U,
                                                                            qmlDirPathSourceId,
                                                                            "First.qml",
                                                                            qmlDocument1SourceId),
                                                             IsExportedType(exampleModuleId,
                                                                            "FirstType2",
                                                                            1U,
                                                                            0U,
                                                                            qmlDirPathSourceId,
                                                                            "First.qml",
                                                                            qmlDocument1SourceId))))));

    updater.update({.projectDirectory = "/path"});
}

TEST_F(ProjectStorageUpdater_synchronize_qml_documents, dont_synchronize_selectors)
{
    setContent(u"/path/+First.qml", "First{}");
    QString qmldir{R"(module Example
                      FirstType 1.0 +First.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({qmlDirPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(Field("SynchronizationPackage::exportedTypes",
                                  &SynchronizationPackage::exportedTypes,
                                  IsEmpty())));

    updater.update({.qtDirectories = {"/path"}});
}

class ProjectStorageUpdater_update_watcher : public BaseProjectStorageUpdater
{
public:
    ProjectStorageUpdater_update_watcher()
    {
        auto annotation1SourceId = createDirectorySourceId("/path/one/designer");
        auto annotation2SourceId = createDirectorySourceId("/path/one");
        auto annotation3SourceId = createDirectorySourceId("/path/one");
        setFilesNotExistsUnchanged({annotation1SourceId, annotation2SourceId, annotation3SourceId});
        setFilesNotExistsUnchanged({qmldir1SourceId, qmldir2SourceId, qmldir3SourceId});
    }

public:
    SourceId directoryPath1SourceId = createDirectorySourceId("/path/one");
    DirectoryPathId directoryPath1Id = directoryPath1SourceId.directoryPathId();
    SourceId directoryPath2SourceId = createDirectorySourceId("/path/two");
    DirectoryPathId directoryPath2Id = directoryPath2SourceId.directoryPathId();
    SourceId directoryPath3SourceId = createDirectorySourceId("/path/three");
    DirectoryPathId directoryPath3Id = directoryPath3SourceId.directoryPathId();
    SourceId qmldir1SourceId = sourcePathCache.sourceId("/path/one/qmldir");
    SourceId qmldir2SourceId = sourcePathCache.sourceId("/path/two/qmldir");
    SourceId qmldir3SourceId = sourcePathCache.sourceId("/path/three/qmldir");
    SourceId qmlDocument1SourceId = sourcePathCache.sourceId("/path/one/First.qml");
    SourceId qmlDocument2SourceId = sourcePathCache.sourceId("/path/one/Second.qml");
    SourceId qmlDocument3SourceId = sourcePathCache.sourceId("/path/two/Third.qml");
    ModuleId exampleModuleId = modulesStorage.moduleId("Example", ModuleKind::QmlLibrary);
    SourceId qmltypes1SourceId = sourcePathCache.sourceId("/path/one/example.qmltypes");
    SourceId qmltypes2SourceId = sourcePathCache.sourceId("/path/two/example2.qmltypes");
};

TEST_F(ProjectStorageUpdater_update_watcher, directories)
{
    setFilesChanged({directoryPath1SourceId, directoryPath2SourceId, directoryPath3SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(
                    IdPaths{qtPartId,
                            QmlDesigner::SourceType::Directory,
                            {directoryPath1SourceId, directoryPath2SourceId, directoryPath3SourceId}})));

    updater.update({.qtDirectories = {"/path/one", "/path/two", "/path/three"}});
}

TEST_F(ProjectStorageUpdater_update_watcher, directory_does_not_exists)
{
    setFilesChanged({directoryPath1SourceId, directoryPath3SourceId});
    setFilesNotExists({directoryPath2SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{qtPartId,
                                               QmlDesigner::SourceType::Directory,
                                               {directoryPath1SourceId, directoryPath3SourceId}})));

    updater.update({.qtDirectories = {"/path/one", "/path/two", "/path/three"}});
}

TEST_F(ProjectStorageUpdater_update_watcher, directory_does_not_changed)
{
    setFilesUnchanged({directoryPath1SourceId, directoryPath2SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{qtPartId,
                                               QmlDesigner::SourceType::Directory,
                                               {directoryPath1SourceId, directoryPath2SourceId}})));

    updater.update({.qtDirectories = {"/path/one", "/path/two"}});
}

TEST_F(ProjectStorageUpdater_update_watcher, directory_removed)
{
    setFilesUnchanged({directoryPath2SourceId});
    setFilesRemoved({directoryPath1SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(
                    IdPaths{qtPartId, QmlDesigner::SourceType::Directory, {directoryPath2SourceId}})));

    updater.update({.qtDirectories = {"/path/one", "/path/two"}});
}

TEST_F(ProjectStorageUpdater_update_watcher, qmldirs)
{
    setFilesUnchanged({directoryPath1SourceId, directoryPath2SourceId, directoryPath3SourceId});
    setFilesAdded({qmldir1SourceId, qmldir2SourceId, qmldir3SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{qtPartId,
                                               QmlDesigner::SourceType::QmlDir,
                                               {qmldir1SourceId, qmldir2SourceId, qmldir3SourceId}})));

    updater.update({.qtDirectories = {"/path/one", "/path/two", "/path/three"}});
}

TEST_F(ProjectStorageUpdater_update_watcher, qmldir_does_not_exists)
{
    setFilesUnchanged({directoryPath1SourceId, directoryPath2SourceId, directoryPath3SourceId});
    setFilesAdded({qmldir1SourceId, qmldir3SourceId});
    setFilesNotExists({qmldir2SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{qtPartId,
                                               QmlDesigner::SourceType::QmlDir,
                                               {qmldir1SourceId, qmldir3SourceId}})));

    updater.update({.qtDirectories = {"/path/one", "/path/two", "/path/three"}});
}

TEST_F(ProjectStorageUpdater_update_watcher, qmldir_does_not_changed)
{
    setFilesUnchanged(
        {directoryPath1SourceId, directoryPath2SourceId, qmldir1SourceId, qmldir2SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{qtPartId,
                                               QmlDesigner::SourceType::QmlDir,
                                               {qmldir1SourceId, qmldir2SourceId}})));

    updater.update({.qtDirectories = {"/path/one", "/path/two"}});
}

TEST_F(ProjectStorageUpdater_update_watcher, qmldir_removed)
{
    setFilesUnchanged(
        {directoryPath1SourceId, directoryPath2SourceId, directoryPath3SourceId, qmldir2SourceId});
    setFilesRemoved({qmldir1SourceId, qmldir3SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(
                    IdPaths{qtPartId, QmlDesigner::SourceType::QmlDir, {qmldir2SourceId}})));

    updater.update({.qtDirectories = {"/path/one", "/path/two", "/path/three"}});
}

TEST_F(ProjectStorageUpdater_update_watcher, qml_docments)
{
    setQmlFileNames(u"/path/one", {"First.qml", "Second.qml"});
    setQmlFileNames(u"/path/two", {"Third.qml"});
    setFilesChanged({directoryPath1SourceId, directoryPath2SourceId});
    setFilesAdded({qmlDocument1SourceId, qmlDocument2SourceId, qmlDocument3SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(
                    IdPaths{qtPartId,
                            QmlDesigner::SourceType::Qml,
                            {qmlDocument1SourceId, qmlDocument2SourceId, qmlDocument3SourceId}})));

    updater.update({.qtDirectories = {"/path/one", "/path/two"}});
}

TEST_F(ProjectStorageUpdater_update_watcher, directory_changed_but_qml_document_unchanged)
{
    setQmlFileNames(u"/path/one", {"First.qml", "Second.qml"});
    setQmlFileNames(u"/path/two", {"Third.qml"});
    setFilesChanged({directoryPath1SourceId, directoryPath2SourceId});
    setFilesUnchanged({qmlDocument1SourceId, qmlDocument2SourceId, qmlDocument3SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(
                    IdPaths{qtPartId,
                            QmlDesigner::SourceType::Qml,
                            {qmlDocument1SourceId, qmlDocument2SourceId, qmlDocument3SourceId}})));

    updater.update({.qtDirectories = {"/path/one", "/path/two"}});
}

TEST_F(ProjectStorageUpdater_update_watcher, only_qml_files_changed)
{
    setQmlFileNames(u"/path/one", {"First.qml", "Second.qml"});
    setQmlFileNames(u"/path/two", {"Third.qml"});
    setQmlDocumentProjectEntryInfos(
        directoryPath1SourceId,
        {{directoryPath1SourceId, qmlDocument1SourceId, exampleModuleId, FileType::QmlDocument},
         {directoryPath1SourceId, qmlDocument2SourceId, exampleModuleId, FileType::QmlDocument}});
    setQmlDocumentProjectEntryInfos(
        directoryPath2SourceId,
        {{directoryPath2SourceId, qmlDocument3SourceId, ModuleId{}, FileType::QmlDocument}});
    setFilesUnchanged({directoryPath1SourceId, directoryPath2SourceId});
    setFilesChanged({qmlDocument1SourceId, qmlDocument2SourceId, qmlDocument3SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(
                    IdPaths{qtPartId,
                            QmlDesigner::SourceType::Qml,
                            {qmlDocument1SourceId, qmlDocument2SourceId, qmlDocument3SourceId}})));

    updater.update({.qtDirectories = {"/path/one", "/path/two"}});
}

TEST_F(ProjectStorageUpdater_update_watcher, directories_and_qml_files_are_unchanged)
{
    setQmlFileNames(u"/path/one", {"First.qml", "Second.qml"});
    setQmlFileNames(u"/path/two", {"Third.qml"});
    setQmlDocumentProjectEntryInfos(
        directoryPath1SourceId,
        {{directoryPath1SourceId, qmlDocument1SourceId, exampleModuleId, FileType::QmlDocument},
         {directoryPath1SourceId, qmlDocument2SourceId, exampleModuleId, FileType::QmlDocument}});
    setQmlDocumentProjectEntryInfos(
        directoryPath2SourceId,
        {{directoryPath2SourceId, qmlDocument3SourceId, ModuleId{}, FileType::QmlDocument}});
    setFilesUnchanged({directoryPath1SourceId, directoryPath2SourceId});
    setFilesUnchanged({qmlDocument1SourceId, qmlDocument2SourceId, qmlDocument3SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(
                    IdPaths{qtPartId,
                            QmlDesigner::SourceType::Qml,
                            {qmlDocument1SourceId, qmlDocument2SourceId, qmlDocument3SourceId}})));

    updater.update({.qtDirectories = {"/path/one", "/path/two"}});
}

TEST_F(ProjectStorageUpdater_update_watcher, qmltypes_added)
{
    QString qmldir1{R"(module Example
                      typeinfo example.qmltypes)"};
    QString qmldir2{R"(module Example2
                      typeinfo example2.qmltypes)"};
    setContent(u"/path/one/qmldir", qmldir1);
    setContent(u"/path/two/qmldir", qmldir2);
    setFilesUnchanged({directoryPath1SourceId, directoryPath2SourceId});
    setFilesChanged({qmldir1SourceId, qmldir2SourceId});
    setFilesAdded({qmltypes1SourceId, qmltypes2SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{qtPartId,
                                               QmlDesigner::SourceType::QmlTypes,
                                               {qmltypes1SourceId, qmltypes2SourceId}})));

    updater.update({.qtDirectories = {"/path/one", "/path/two"}});
}

TEST_F(ProjectStorageUpdater_update_watcher, qmltypes_only_added_to_qmldir)
{
    QString qmldir1{R"(module Example
                      typeinfo example.qmltypes)"};
    QString qmldir2{R"(module Example2
                      typeinfo example2.qmltypes)"};
    setContent(u"/path/one/qmldir", qmldir1);
    setContent(u"/path/two/qmldir", qmldir2);
    setFilesUnchanged({directoryPath1SourceId, directoryPath2SourceId});
    setFilesChanged({qmldir1SourceId, qmldir2SourceId});
    setFilesUnchanged({qmltypes1SourceId, qmltypes2SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{qtPartId,
                                               QmlDesigner::SourceType::QmlTypes,
                                               {qmltypes1SourceId, qmltypes2SourceId}})));

    updater.update({.qtDirectories = {"/path/one", "/path/two"}});
}

TEST_F(ProjectStorageUpdater_update_watcher, only_qmltypes_changed)
{
    setQmlTypesProjectEntryInfos(
        qmldir1SourceId, {{qmldir1SourceId, qmltypes1SourceId, exampleModuleId, FileType::QmlTypes}});
    setQmlTypesProjectEntryInfos(
        qmldir2SourceId, {{qmldir2SourceId, qmltypes2SourceId, exampleModuleId, FileType::QmlTypes}});
    setFilesUnchanged(
        {directoryPath1SourceId, directoryPath2SourceId, qmldir1SourceId, qmldir2SourceId});
    setFilesChanged({qmltypes1SourceId, qmltypes2SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{qtPartId,
                                               QmlDesigner::SourceType::QmlTypes,
                                               {qmltypes1SourceId, qmltypes2SourceId}})));

    updater.update({.qtDirectories = {"/path/one", "/path/two"}});
}

TEST_F(ProjectStorageUpdater_update_watcher, qmltypes_and_directories_are_unchanged)
{
    setQmlTypesProjectEntryInfos(
        qmldir1SourceId, {{qmldir1SourceId, qmltypes1SourceId, exampleModuleId, FileType::QmlTypes}});
    setQmlTypesProjectEntryInfos(
        qmldir2SourceId, {{qmldir2SourceId, qmltypes2SourceId, exampleModuleId, FileType::QmlTypes}});
    setFilesUnchanged({directoryPath1SourceId,
                       directoryPath2SourceId,
                       qmldir1SourceId,
                       qmldir2SourceId,
                       qmltypes1SourceId,
                       qmltypes2SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{qtPartId,
                                               QmlDesigner::SourceType::QmlTypes,
                                               {qmltypes1SourceId, qmltypes2SourceId}})));

    updater.update({.qtDirectories = {"/path/one", "/path/two"}});
}

namespace Watcher {
struct ProjectPartId
{
    QmlDesigner::ProjectPartId projectPartId;
    std::string name;

    [[maybe_unused]] friend std::ostream &operator<<(std::ostream &out, const ProjectPartId &id)
    {
        return out << "ProjectPartId(.projectPartId=" << id.projectPartId.internalId() << ")";
    }
};

struct State
{
    FileState state;
    FileState directoryState;
    FileState qmldirState;
    std::string name;

    [[maybe_unused]] friend std::ostream &operator<<(std::ostream &out, const State &state)
    {
        return out << "State(.state=" << state.state << ", directoryState=" << state.directoryState
                   << ", qmldirState=" << state.qmldirState << ")";
    }
};

struct Subdirectory
{
    bool rootSubdirectory;

    [[maybe_unused]] friend std::ostream &operator<<(std::ostream &out,
                                                     const Subdirectory &subdirectory)
    {
        return out << "Subdirectory(.rootSubdirectory=" << subdirectory.rootSubdirectory << ")";
    }
};

struct Qmldir
{
    QString qmldir;
    bool noSecondTypeInQmldir;
    std::string name;

    [[maybe_unused]] friend std::ostream &operator<<(std::ostream &out, const Qmldir &qmldir)
    {
        return out << "Qmldir(.qmldir=" << qmldir.qmldir
                   << ", .noSecondTypeInQmldir=" << qmldir.noSecondTypeInQmldir << ")";
    }
};

enum class Error { No, Parser, Synchronize, MergeEntries };

[[maybe_unused]] std::ostream &operator<<(std::ostream &out, Error error)
{
    out << "Error::";
    switch (error) {
    case Error::No:
        out << "No";
        break;
    case Error::Parser:
        out << "Parser";
        break;
    case Error::Synchronize:
        out << "Synchronize";
        break;
    case Error::MergeEntries:
        out << "MergeEntries";
        break;
    }

    return out;
}

using Parameters = std::tuple<ProjectPartId, State, Qmldir, Subdirectory, Error>;

} // namespace Watcher

class watcher : public BaseProjectStorageUpdater,
                public testing::WithParamInterface<Watcher::Parameters>
{
public:
    static bool isChanged(FileState state)
    {
        switch (state) {
        case FileState::Changed:
        case FileState::Added:
        case FileState::Removed:
            return true;
        case FileState::Unchanged:
        case FileState::NotExistsUnchanged:
        case FileState::NotExists:
            break;
        }
        return false;
    }

    static bool isExisting(FileState state)
    {
        switch (state) {
        case FileState::Changed:
        case FileState::Added:
        case FileState::Unchanged:
            return true;
        case FileState::Removed:
        case FileState::NotExistsUnchanged:
        case FileState::NotExists:
            break;
        }
        return false;
    }

    testing::Matcher<IsInsideProject> createIsInsideProjectMatcher() const
    {
        if (partId == projectPartId)
            return Eq(IsInsideProject::Yes);

        if (partId == qtPartId)
            return Eq(IsInsideProject::No);

        return An<IsInsideProject>();
    }

    virtual void setupParser() = 0;

    void potentionallyThrowError()
    {
        // TODO add more error cases
        switch (error) {
        case Watcher::Error::No:
            break;
        case Watcher::Error::Parser: {
            ON_CALL(qmlDocumentParserMock, parse(_, _, _, _, _)).WillByDefault(Throw(std::exception{}));
            ON_CALL(qmlTypesParserMock, parse(_, _, _, _, _, _)).WillByDefault(Throw(std::exception{}));
            updater.pathsWithIdsChanged(idPaths);
            setupParser();
            break;
        }
        case Watcher::Error::Synchronize:
            ON_CALL(projectStorageMock, synchronize(_)).WillByDefault(Throw(std::exception{}));
            updater.pathsWithIdsChanged(idPaths);
            ON_CALL(projectStorageMock, synchronize(_)).WillByDefault(Return());
            break;
        case Watcher::Error::MergeEntries:
            if (idPaths.size() >= 1) {
                std::vector<IdPaths> errorIdPaths = {idPaths.front()};
                idPaths.erase(idPaths.begin());
                ON_CALL(projectStorageMock, synchronize(_)).WillByDefault(Throw(std::exception{}));
                updater.pathsWithIdsChanged(errorIdPaths);
                ON_CALL(projectStorageMock, synchronize(_)).WillByDefault(Return());
            }
            break;
        }
    }

public:
    SourceId qmlDirPathSourceId = sourcePathCache.sourceId("/root/path/qmldir");
    DirectoryPathId directoryPathId = qmlDirPathSourceId.directoryPathId();
    SourceId directoryPathSourceId = SourceId::create(directoryPathId, QmlDesigner::FileNameId{});
    DirectoryPathId rootDirectoryPathId = sourcePathCache.directoryPathId("/root");
    SourceId rootDirectoryPathSourceId = SourceId::create(rootDirectoryPathId,
                                                          QmlDesigner::FileNameId{});
    SourceId rootQmldirPathSourceId = sourcePathCache.sourceId("/root/qmldir");
    DirectoryPathId designerDirectoryPathId = sourcePathCache.directoryPathId(
        "/root/path/designer");
    SourceId designerDirectoryPathSourceId = SourceId::create(designerDirectoryPathId,
                                                              QmlDesigner::FileNameId{});
    QmlDesigner::ProjectPartId partId = std::get<0>(GetParam()).projectPartId;
    std::vector<IdPaths> idPaths;
    FileState state = std::get<1>(GetParam()).state;
    FileState directoryState = std::get<1>(GetParam()).directoryState;
    FileState qmldirState = std::get<1>(GetParam()).qmldirState;
    bool isIgnoredPartId = partId == otherProjectPartId;
    bool isDirectoryUnchanged = directoryState == FileState::Unchanged;
    bool isDirectoryRemoved = directoryState == FileState::Removed;
    bool isQmldirUnchanged = qmldirState == FileState::Unchanged;
    bool isQmldirNotExisting = qmldirState == FileState::NotExists
                               or qmldirState == FileState::NotExistsUnchanged;
    bool isQmldirRemoved = qmldirState == FileState::Removed;
    bool isDirectoryAndQmldirUnchanged = isDirectoryUnchanged and isQmldirUnchanged;
    bool useSubdirectory = std::get<3>(GetParam()).rootSubdirectory;
    QString qmldir = std::get<2>(GetParam()).qmldir;
    bool noSecondTypeInQmldir = std::get<2>(GetParam()).noSecondTypeInQmldir;
    Watcher::Error error = std::get<4>(GetParam());
};

class watcher_document : public watcher
{
public:
    watcher_document() { initializeIdPaths(); }

    void initializeIdPaths()
    {
        using QmlDesigner::SourceType;

        if (isChanged(directoryState)) {
            SourceIds directorySourceIds = {directoryPathSourceId};
            if (useSubdirectory)
                directorySourceIds.push_back(rootDirectoryPathSourceId);

            idPaths.emplace_back(partId, SourceType::Directory, std::move(directorySourceIds));
        }

        if (isChanged(qmldirState)) {
            SourceIds qmldirSourceIds = {qmlDirPathSourceId};
            if (useSubdirectory)
                qmldirSourceIds.push_back(rootQmldirPathSourceId);

            idPaths.emplace_back(partId, SourceType::QmlDir, std::move(qmldirSourceIds));
        }

        if (isChanged(state)) {
            idPaths.emplace_back(partId,
                                 SourceType::Qml,
                                 SourceIds{qmlDocument1SourceId, qmlDocument2SourceId});
        }
    }

public:
    SourceId qmlDocument1SourceId = sourcePathCache.sourceId("/root/path/First.qml");
    SourceId qmlDocument2SourceId = sourcePathCache.sourceId("/root/path/Second.qml");
    ModuleId exampleModuleId = modulesStorage.moduleId("Example", ModuleKind::QmlLibrary);
    ModuleId qmlModuleId = modulesStorage.moduleId("QtQml", ModuleKind::QmlLibrary);
    ModuleId pathModuleId = modulesStorage.moduleId("/root/path", ModuleKind::PathLibrary);
    Storage::Import import1{qmlModuleId,
                            Storage::Version{2, 3},
                            qmlDocument1SourceId,
                            qmlDocument1SourceId};
    Storage::Import import2{qmlModuleId, Storage::Version{2}, qmlDocument2SourceId, qmlDocument2SourceId};
    bool isDocumentUnchanged = state == FileState::Unchanged;
    bool isDocumentRemoved = state == FileState::Removed;
    bool isDocumentNonExisting = state == FileState::NotExists
                                 or state == FileState::NotExistsUnchanged;
    bool isDocumentAndQmldirRemoved = isQmldirRemoved and isDocumentRemoved;
};

class watcher_document_changes : public watcher_document
{
public:
    watcher_document_changes()
    {
        setupParser();

        if (state != FileState::Added) {
            setQmlDocumentProjectEntryInfos(
                directoryPathSourceId,
                {{directoryPathSourceId, qmlDocument1SourceId, ModuleId{}, FileType::QmlDocument},
                 {directoryPathSourceId, qmlDocument2SourceId, ModuleId{}, FileType::QmlDocument}});
        }

        setContent(u"/root/path/qmldir", qmldir);
        setQmlFileNames(u"/root/path", {"First.qml", "Second.qml"});
        setFilesNotExistsUnchanged({designerDirectoryPathSourceId});

        setFileSystemSubdirectories(u"/root", {"/root/path"});
        setStorageSubdirectories(rootDirectoryPathId, {directoryPathId});
    }

    void setupParser() final
    {
        ON_CALL(qmlDocumentParserMock, parse(_, _, qmlDocument1SourceId, _, _))
            .WillByDefault([&](auto, auto &imports, auto, auto, auto) {
                imports.push_back(import1);
                Type type;
                type.prototype = ImportedType{"Object"};
                type.traits = TypeTraitsKind::Reference;
                return type;
            });
        ON_CALL(qmlDocumentParserMock, parse(_, _, qmlDocument2SourceId, _, _))
            .WillByDefault([&](auto, auto &imports, auto, auto, auto) {
                imports.push_back(import2);
                Type type;
                type.prototype = ImportedType{"Object3"};
                type.traits = TypeTraitsKind::Reference;
                return type;
            });
    }
};

auto watcher_parameters_printer = [](const testing::TestParamInfo<Watcher::Parameters> &info) {
    std::string name = std::get<0>(info.param).name;
    name += '_';
    name += std::get<1>(info.param).name;

    name += '_';
    name += std::get<2>(info.param).name;

    if (std::get<3>(info.param).rootSubdirectory)
        name += "_subdirectory";

    switch (std::get<4>(info.param)) {
    case Watcher::Error::No:
        name += "_no_error";
        break;
    case Watcher::Error::Parser:
        name += "_parser_error";
        break;
    case Watcher::Error::Synchronize:
        name += "_synchronize_error";
        break;
    case Watcher::Error::MergeEntries:
        name += "_merge_entries";
        break;
    }

    return name;
};

INSTANTIATE_TEST_SUITE_P(
    ProjectStorageUpdater,
    watcher_document_changes,
    Combine(Values(Watcher::ProjectPartId{.projectPartId = qtPartId, .name = "qt_part"},
                   Watcher::ProjectPartId{.projectPartId = projectPartId, .name = "project_part"},
                   Watcher::ProjectPartId{.projectPartId = otherProjectPartId,
                                          .name = "other_project_part"}),
            Values(Watcher::State{.state = FileState::Added,
                                  .directoryState = FileState::Changed,
                                  .qmldirState = FileState::Changed,
                                  .name = "file_added_directory_changed_qmldir_changed"},
                   Watcher::State{.state = FileState::Added,
                                  .directoryState = FileState::Added,
                                  .qmldirState = FileState::Added,
                                  .name = "file_added_directory_added_qmldir_added"},
                   Watcher::State{.state = FileState::Changed,
                                  .directoryState = FileState::Unchanged,
                                  .qmldirState = FileState::Unchanged,
                                  .name = "file_changed_directory_unchanged_qmldir_unchanged"},
                   Watcher::State{.state = FileState::Changed,
                                  .directoryState = FileState::Unchanged,
                                  .qmldirState = FileState::Changed,
                                  .name = "file_changed_directory_unchanged_qmldir_changed"},
                   Watcher::State{.state = FileState::Changed,
                                  .directoryState = FileState::Changed,
                                  .qmldirState = FileState::Unchanged,
                                  .name = "file_changed_directory_changed_qmldir_unchanged"},
                   Watcher::State{.state = FileState::Unchanged,
                                  .directoryState = FileState::Unchanged,
                                  .qmldirState = FileState::Changed,
                                  .name = "file_unchanged_directory_unchanged_qmldir_changed"}),
            Values(Watcher::Qmldir{.qmldir = R"(module Example
                                                   FirstType 1.0 First.qml
                                                   SecondType 2.2 Second.qml)",
                                   .noSecondTypeInQmldir = false,
                                   .name = "qmldir_two_entries"},
                   Watcher::Qmldir{.qmldir = R"(module Example
                                                   FirstType 1.0 First.qml)",
                                   .noSecondTypeInQmldir = true,
                                   .name = "qmldir_one_entries"}),
            Values(Watcher::Subdirectory{.rootSubdirectory = true},
                   Watcher::Subdirectory{.rootSubdirectory = false}),
            Values(Watcher::Error::No,
                   Watcher::Error::Parser,
                   Watcher::Error::Synchronize,
                   Watcher::Error::MergeEntries)),
    watcher_parameters_printer);

TEST_P(watcher_document_changes, imports)
{
    setFiles(state, {qmlDocument1SourceId, qmlDocument2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::imports",
                                        &SynchronizationPackage::imports,
                                        Conditional(isIgnoredPartId or isDocumentUnchanged,
                                                    IsEmpty(),
                                                    UnorderedElementsAre(import1, import2))),
                                  Field("SynchronizationPackage::updatedImportSourceIds",
                                        &SynchronizationPackage::updatedImportSourceIds,
                                        Conditional(isIgnoredPartId or isDocumentUnchanged,
                                                    IsEmpty(),
                                                    UnorderedElementsAre(qmlDocument1SourceId,
                                                                         qmlDocument2SourceId))))));

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_document_changes, types)
{
    setFiles(state, {qmlDocument1SourceId, qmlDocument2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(
                    Field("SynchronizationPackage::types",
                          &SynchronizationPackage::types,
                          Conditional(isIgnoredPartId or isDocumentUnchanged,
                                      IsEmpty(),
                                      UnorderedElementsAre(IsStorageType("First.qml",
                                                                         ImportedType{"Object"},
                                                                         TypeTraitsKind::Reference,
                                                                         qmlDocument1SourceId),
                                                           IsStorageType("Second.qml",
                                                                         ImportedType{"Object3"},
                                                                         TypeTraitsKind::Reference,
                                                                         qmlDocument2SourceId)))),
                    Field("SynchronizationPackage::updatedTypeSourceIds",
                          &SynchronizationPackage::updatedTypeSourceIds,
                          Conditional(isIgnoredPartId or isDocumentUnchanged,
                                      IsEmpty(),
                                      UnorderedElementsAre(qmlDocument1SourceId,
                                                           qmlDocument2SourceId))))));

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_document_changes, exported_types)
{
    setFiles(state, {qmlDocument1SourceId, qmlDocument2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::exportedTypes",
                  &SynchronizationPackage::exportedTypes,
                  Conditional(isIgnoredPartId || isDirectoryAndQmldirUnchanged,
                              IsEmpty(),
                              AllOf(Contains(IsExportedType(exampleModuleId,
                                                            "FirstType",
                                                            1U,
                                                            0U,
                                                            qmlDirPathSourceId,
                                                            "First.qml",
                                                            qmlDocument1SourceId))
                                        .Times(isQmldirUnchanged ? 0 : 1),
                                    Contains(IsExportedType(pathModuleId,
                                                            "First",
                                                            directoryPathSourceId,
                                                            "First.qml",
                                                            qmlDocument1SourceId))
                                        .Times(isDirectoryUnchanged ? 0 : 1),
                                    Contains(IsExportedType(exampleModuleId,
                                                            "SecondType",
                                                            2U,
                                                            2U,
                                                            qmlDirPathSourceId,
                                                            "Second.qml",
                                                            qmlDocument2SourceId))
                                        .Times(isQmldirUnchanged or noSecondTypeInQmldir ? 0 : 1),
                                    Contains(IsExportedType(pathModuleId,
                                                            "Second",
                                                            directoryPathSourceId,
                                                            "Second.qml",
                                                            qmlDocument2SourceId))
                                        .Times(isDirectoryUnchanged ? 0 : 1)))),
            Field("SynchronizationPackage::updatedExportedTypeSourceIds",
                  &SynchronizationPackage::updatedExportedTypeSourceIds,
                  Conditional(isIgnoredPartId || isDirectoryAndQmldirUnchanged,
                              IsEmpty(),
                              AllOf(Contains(directoryPathSourceId).Times(isDirectoryUnchanged ? 0 : 1),
                                    Contains(qmlDirPathSourceId).Times(isQmldirUnchanged ? 0 : 1)))))));

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_document_changes, file_status)
{
    setFiles(state, {qmlDocument1SourceId, qmlDocument2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  Conditional(isIgnoredPartId,
                              IsEmpty(),
                              AllOf(Contains(IsFileStatus(directoryPathSourceId, 1, 21))
                                        .Times(isDirectoryUnchanged ? 0 : 1),
                                    Contains(IsFileStatus(qmlDirPathSourceId, 1, 21))
                                        .Times(isQmldirUnchanged ? 0 : 1),
                                    Contains(IsFileStatus(qmlDocument1SourceId, 1, 21))
                                        .Times(isDocumentUnchanged ? 0 : 1),
                                    Contains(IsFileStatus(qmlDocument2SourceId, 1, 21))
                                        .Times(isDocumentUnchanged ? 0 : 1)))),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  Conditional(
                      isIgnoredPartId,
                      IsEmpty(),
                      AllOf(Contains(directoryPathSourceId).Times(isDirectoryUnchanged ? 0 : 1),
                            Contains(qmlDirPathSourceId).Times(isQmldirUnchanged ? 0 : 1),
                            Contains(qmlDocument1SourceId).Times(isDocumentUnchanged ? 0 : 1),
                            Contains(qmlDocument2SourceId).Times(isDocumentUnchanged ? 0 : 1)))))));

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_document_changes, project_entry_infos)
{
    setFiles(state, {qmlDocument1SourceId, qmlDocument2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::projectEntryInfos",
                  &SynchronizationPackage::projectEntryInfos,
                  Conditional(isIgnoredPartId or isDirectoryUnchanged,
                              IsEmpty(),
                              AllOf(Contains(IsProjectEntryInfo(rootDirectoryPathSourceId,
                                                             directoryPathSourceId,
                                                             ModuleId{},
                                                             FileType::Directory))
                                        .Times(useSubdirectory ? 1 : 0),
                                    IsSupersetOf({IsProjectEntryInfo(directoryPathSourceId,
                                                                  qmlDocument1SourceId,
                                                                  ModuleId{},
                                                                  FileType::QmlDocument),
                                                  IsProjectEntryInfo(directoryPathSourceId,
                                                                  qmlDocument2SourceId,
                                                                  ModuleId{},
                                                                  FileType::QmlDocument)})))),
            Field("SynchronizationPackage::updatedProjectEntryInfoSourceIds",
                  &SynchronizationPackage::updatedProjectEntryInfoSourceIds,
                  Conditional(isIgnoredPartId or isDirectoryUnchanged,
                              IsEmpty(),
                              AllOf(Contains(rootDirectoryPathSourceId).Times(useSubdirectory ? 1 : 0),
                                    Contains(directoryPathSourceId)))))));

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_document_changes, qml_document_parses_inside_or_project)
{
    setFiles(state, {qmlDocument1SourceId, qmlDocument2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    auto isInsideProjectMatcher = createIsInsideProjectMatcher();
    potentionallyThrowError();

    EXPECT_CALL(qmlDocumentParserMock, parse(_, _, _, _, isInsideProjectMatcher))
        .Times(isIgnoredPartId ? Exactly(0) : AnyNumber());

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_document_changes, clear_id_paths_after_successful_commit)
{
    setFiles(state, {qmlDocument1SourceId, qmlDocument2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    auto isInsideProjectMatcher = createIsInsideProjectMatcher();
    potentionallyThrowError();
    updater.pathsWithIdsChanged(idPaths);

    EXPECT_CALL(projectStorageMock, synchronize(PackageIsEmpty()));

    updater.pathsWithIdsChanged({});
}

TEST_P(watcher_document_changes, dont_change_watcher_for_document_changes_only)
{
    setFiles(state, {qmlDocument1SourceId, qmlDocument2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(patchWatcherMock, updateContextIdPaths(_, _))
        .Times(isDirectoryAndQmldirUnchanged or isIgnoredPartId ? Exactly(0) : Exactly(1));

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_document_changes, update_watcher)
{
    setFiles(state, {qmlDocument1SourceId, qmlDocument2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(
        patchWatcherMock,
        updateContextIdPaths(
            AllOf(Contains(IsIdPaths(partId,
                                     SourceType::Directory,
                                     Conditional(useSubdirectory,
                                                 UnorderedElementsAre(rootDirectoryPathSourceId,
                                                                      directoryPathSourceId),
                                                 ElementsAre(directoryPathSourceId))))
                      .Times(isDirectoryUnchanged ? 0 : 1),
                  Contains(IsIdPaths(partId,
                                     SourceType::QmlDir,
                                     Conditional(useSubdirectory,
                                                 UnorderedElementsAre(rootQmldirPathSourceId,
                                                                      qmlDirPathSourceId),
                                                 ElementsAre(qmlDirPathSourceId))))
                      .Times(isDirectoryAndQmldirUnchanged ? 0 : 1),
                  Contains(IsIdPaths(partId,
                                     SourceType::Qml,
                                     UnorderedElementsAre(qmlDocument1SourceId, qmlDocument2SourceId)))
                      .Times(isDirectoryUnchanged ? 0 : 1)),
            AllOf(Contains(directoryPathId).Times(isDirectoryUnchanged ? 0 : 1),
                  Contains(rootDirectoryPathId).Times(!isDirectoryUnchanged and useSubdirectory ? 1 : 0))))
        .Times(isDirectoryAndQmldirUnchanged or isIgnoredPartId ? Exactly(0) : Exactly(1));

    updater.pathsWithIdsChanged(idPaths);
}

class watcher_document_not_existing : public watcher_document
{
public:
    watcher_document_not_existing()
    {
        setupParser();

        if (state == FileState::Removed) {
            setQmlDocumentProjectEntryInfos(
                directoryPathSourceId,
                {{directoryPathSourceId, qmlDocument1SourceId, ModuleId{}, FileType::QmlDocument},
                 {directoryPathSourceId, qmlDocument2SourceId, ModuleId{}, FileType::QmlDocument}});
        }

        setContent(u"/root/path/qmldir", qmldir);
        setFilesNotExistsUnchanged({designerDirectoryPathSourceId});

        setFileSystemSubdirectories(u"/root", {"/root/path"});
        setStorageSubdirectories(rootDirectoryPathId, {directoryPathId});
    }

    testing::Matcher<IsInsideProject> createIsInsideProjectMatcher() const
    {
        if (partId == projectPartId)
            return Eq(IsInsideProject::Yes);

        if (partId == qtPartId)
            return Eq(IsInsideProject::No);

        return An<IsInsideProject>();
    }

    void setupParser() final {}

public:
};

INSTANTIATE_TEST_SUITE_P(
    ProjectStorageUpdater,
    watcher_document_not_existing,
    Combine(Values(Watcher::ProjectPartId{.projectPartId = qtPartId, .name = "qt_part"},
                   Watcher::ProjectPartId{.projectPartId = projectPartId, .name = "project_part"},
                   Watcher::ProjectPartId{.projectPartId = otherProjectPartId,
                                          .name = "other_project_part"}),
            Values(Watcher::State{.state = FileState::Removed,
                                  .directoryState = FileState::Changed,
                                  .qmldirState = FileState::Changed,
                                  .name = "file_removed_directory_changed_qmldir_changed"},
                   Watcher::State{.state = FileState::Removed,
                                  .directoryState = FileState::Changed,
                                  .qmldirState = FileState::Removed,
                                  .name = "file_removed_directory_changed_qmldir_removed"},
                   Watcher::State{.state = FileState::Removed,
                                  .directoryState = FileState::Removed,
                                  .qmldirState = FileState::Removed,
                                  .name = "file_removed_directory_removed_qmldir_removed"}),
            Values(Watcher::Qmldir{.qmldir = R"(module Example
                                                FirstType 1.0 First.qml
                                                SecondType 2.2 Second.qml)",
                                   .noSecondTypeInQmldir = false,
                                   .name = "qmldir_two_entries"},
                   Watcher::Qmldir{.qmldir = R"(module Example
                                                FirstType 1.0 First.qml)",
                                   .noSecondTypeInQmldir = true,
                                   .name = "qmldir_one_entries"}),
            Values(Watcher::Subdirectory{.rootSubdirectory = true},
                   Watcher::Subdirectory{.rootSubdirectory = false}),
            Values(Watcher::Error::No, Watcher::Error::Parser, Watcher::Error::Synchronize)),
    watcher_parameters_printer);

TEST_P(watcher_document_not_existing, imports)
{
    setFiles(state, {qmlDocument1SourceId, qmlDocument2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::imports",
                                        &SynchronizationPackage::imports,
                                        IsEmpty()),
                                  Field("SynchronizationPackage::updatedImportSourceIds",
                                        &SynchronizationPackage::updatedImportSourceIds,
                                        Conditional(isIgnoredPartId or isDocumentUnchanged,
                                                    IsEmpty(),
                                                    UnorderedElementsAre(qmlDocument1SourceId,
                                                                         qmlDocument2SourceId))))));

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_document_not_existing, types)
{
    setFiles(state, {qmlDocument1SourceId, qmlDocument2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::types",
                  &SynchronizationPackage::types,
                  Conditional(isIgnoredPartId or isDocumentRemoved
                                  or (isDirectoryUnchanged and isDocumentNonExisting),
                              IsEmpty(),
                              UnorderedElementsAre(IsStorageType("First.qml",
                                                                 ImportedType{},
                                                                 TypeTraitsKind::None,
                                                                 qmlDocument1SourceId),
                                                   IsStorageType("Second.qml",
                                                                 ImportedType{},
                                                                 TypeTraitsKind::None,
                                                                 qmlDocument2SourceId)))),
            Field("SynchronizationPackage::updatedTypeSourceIds",
                  &SynchronizationPackage::updatedTypeSourceIds,
                  Conditional(isIgnoredPartId or (isDirectoryUnchanged and isDocumentNonExisting),
                              IsEmpty(),
                              UnorderedElementsAre(qmlDocument1SourceId, qmlDocument2SourceId))))));

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_document_not_existing, exported_types)
{
    setFiles(state, {qmlDocument1SourceId, qmlDocument2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::exportedTypes",
                  &SynchronizationPackage::exportedTypes,
                  Conditional(isIgnoredPartId or isQmldirRemoved or isQmldirUnchanged,
                              IsEmpty(),
                              Conditional(noSecondTypeInQmldir,
                                          ElementsAre(IsExportedType(exampleModuleId,
                                                                     "FirstType",
                                                                     1U,
                                                                     0U,
                                                                     qmlDirPathSourceId,
                                                                     "First.qml",
                                                                     qmlDocument1SourceId)),
                                          UnorderedElementsAre(IsExportedType(exampleModuleId,
                                                                              "FirstType",
                                                                              1U,
                                                                              0U,
                                                                              qmlDirPathSourceId,
                                                                              "First.qml",
                                                                              qmlDocument1SourceId),
                                                               IsExportedType(exampleModuleId,
                                                                              "SecondType",
                                                                              2U,
                                                                              2U,
                                                                              qmlDirPathSourceId,
                                                                              "Second.qml",
                                                                              qmlDocument2SourceId))))),
            Field("SynchronizationPackage::updatedExportedTypeSourceIds",
                  &SynchronizationPackage::updatedExportedTypeSourceIds,
                  Conditional(isIgnoredPartId or isDirectoryAndQmldirUnchanged,
                              IsEmpty(),
                              AllOf(Contains(rootQmldirPathSourceId)
                                        .Times(isQmldirUnchanged or not useSubdirectory ? 0 : 1),
                                    Contains(qmlDirPathSourceId).Times(isQmldirUnchanged ? 0 : 1),
                                    Contains(rootDirectoryPathSourceId)
                                        .Times(isDirectoryUnchanged or not useSubdirectory ? 0 : 1),
                                    Contains(directoryPathSourceId)
                                        .Times(isDirectoryUnchanged ? 0 : 1)))))));

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_document_not_existing, file_status)
{
    setFiles(state, {qmlDocument1SourceId, qmlDocument2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  Conditional(isIgnoredPartId,
                              IsEmpty(),
                              AllOf(Contains(IsFileStatus(directoryPathSourceId, 1, 21))
                                        .Times(isDirectoryUnchanged or isDirectoryRemoved ? 0 : 1),
                                    Contains(IsFileStatus(qmlDirPathSourceId, 1, 21))
                                        .Times(isQmldirUnchanged or isQmldirRemoved ? 0 : 1)))),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  Conditional(
                      isIgnoredPartId,
                      IsEmpty(),
                      AllOf(Contains(directoryPathSourceId).Times(isDirectoryUnchanged ? 0 : 1),
                            Contains(qmlDirPathSourceId).Times(isQmldirUnchanged ? 0 : 1),
                            Contains(qmlDocument1SourceId).Times(isDocumentNonExisting ? 0 : 1),
                            Contains(qmlDocument2SourceId).Times(isDocumentNonExisting ? 0 : 1)))))));

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_document_not_existing, project_entry_infos)
{
    setFiles(state, {qmlDocument1SourceId, qmlDocument2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::projectEntryInfos",
                                &SynchronizationPackage::projectEntryInfos,
                                Conditional(isIgnoredPartId or isDirectoryUnchanged or isDirectoryRemoved,
                                            IsEmpty(),
                                            AllOf(Contains(IsProjectEntryInfo(rootDirectoryPathSourceId,
                                                                           directoryPathSourceId,
                                                                           ModuleId{},
                                                                           FileType::Directory))
                                                      .Times(useSubdirectory ? 1 : 0)))),
                          Field("SynchronizationPackage::updatedProjectEntryInfoSourceIds",
                                &SynchronizationPackage::updatedProjectEntryInfoSourceIds,
                                Conditional(isIgnoredPartId or isDirectoryUnchanged,
                                            IsEmpty(),
                                            AllOf(Contains(rootDirectoryPathSourceId)
                                                      .Times(useSubdirectory ? 1 : 0),
                                                  Contains(directoryPathSourceId)))))));

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_document_not_existing, qml_document_parses_inside_or_project)
{
    setFiles(state, {qmlDocument1SourceId, qmlDocument2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    auto isInsideProjectMatcher = createIsInsideProjectMatcher();

    EXPECT_CALL(qmlDocumentParserMock, parse(_, _, _, _, isInsideProjectMatcher))
        .Times(isIgnoredPartId ? Exactly(0) : AnyNumber());

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_document_not_existing, update_watcher)
{
    setFiles(state, {qmlDocument1SourceId, qmlDocument2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(patchWatcherMock,
                updateContextIdPaths(
                    AllOf(Contains(IsIdPaths(partId,
                                             SourceType::Directory,
                                             Conditional(useSubdirectory,
                                                         UnorderedElementsAre(rootDirectoryPathSourceId,
                                                                              directoryPathSourceId),
                                                         ElementsAre(directoryPathSourceId))))
                              .Times(isDirectoryRemoved ? 0 : 1),
                          Contains(IsIdPaths(partId,
                                             SourceType::QmlDir,
                                             Conditional(useSubdirectory,
                                                         UnorderedElementsAre(rootQmldirPathSourceId,
                                                                              qmlDirPathSourceId),
                                                         ElementsAre(qmlDirPathSourceId))))
                              .Times(isQmldirNotExisting or isQmldirRemoved ? 0 : 1),
                          Contains(IsIdPaths(partId,
                                             SourceType::Qml,
                                             UnorderedElementsAre(qmlDocument1SourceId,
                                                                  qmlDocument2SourceId)))
                              .Times(isDocumentNonExisting or isDocumentRemoved ? 0 : 1)),
                    AllOf(Contains(directoryPathId),
                          Contains(rootDirectoryPathId).Times(useSubdirectory ? 1 : 0))))
        .Times(isIgnoredPartId ? Exactly(0) : Exactly(1));

    updater.pathsWithIdsChanged(idPaths);
}

class watcher_qmltypes : public watcher
{
public:
    watcher_qmltypes() { initializeIdPaths(); }

    void initializeIdPaths()
    {
        using QmlDesigner::SourceType;

        if (isChanged(directoryState)) {
            SourceIds directorySourceIds = {directoryPathSourceId};
            if (useSubdirectory)
                directorySourceIds.push_back(rootDirectoryPathSourceId);

            idPaths.emplace_back(partId, SourceType::Directory, std::move(directorySourceIds));
        }

        if (isChanged(qmldirState)) {
            SourceIds qmldirSourceIds = {qmlDirPathSourceId};
            if (useSubdirectory)
                qmldirSourceIds.push_back(rootQmldirPathSourceId);

            idPaths.emplace_back(partId, SourceType::QmlDir, std::move(qmldirSourceIds));
        }

        if (isChanged(state)) {
            idPaths.emplace_back(partId,
                                 SourceType::QmlTypes,
                                 SourceIds{qmltypes1SourceId, qmltypes2SourceId});
        }
    }

    static bool isChanged(FileState state)
    {
        switch (state) {
        case FileState::Changed:
        case FileState::Added:
        case FileState::Removed:
            return true;
        case FileState::Unchanged:
        case FileState::NotExistsUnchanged:
        case FileState::NotExists:
            break;
        }
        return false;
    }

public:
    SourceId qmltypes1SourceId = sourcePathCache.sourceId("/root/path/qmltypes1.qmltypes");
    SourceId qmltypes2SourceId = sourcePathCache.sourceId("/root/path/qmltypes2.qmltypes");
    QString qmltypes1Content{"Module {\ndependencies: [module1]}"};
    QString qmltypes2Content{"Module {\ndependencies: [module2]}"};
    ModuleId exampleCppModuleId = modulesStorage.moduleId("Example", ModuleKind::CppLibrary);
    ModuleId qmlModuleId = modulesStorage.moduleId("QtQml", ModuleKind::QmlLibrary);
    ModuleId pathModuleId = modulesStorage.moduleId("/root/path", ModuleKind::PathLibrary);
    Storage::Import import1{qmlModuleId, Storage::Version{2, 3}, qmltypes1SourceId, qmlDirPathSourceId};
    Storage::Import import2{qmlModuleId, Storage::Version{2}, qmltypes2SourceId, qmlDirPathSourceId};
    bool isQmltypesUnchanged = state == FileState::Unchanged;
    bool isQmltypesRemoved = state == FileState::Removed;
    bool isQmltypesNonExisting = state == FileState::NotExists
                                 or state == FileState::NotExistsUnchanged;
    bool isQmltypesOrQmldirRemoved = isQmldirRemoved or isQmltypesRemoved;
};

class watcher_qmltypes_changes : public watcher_qmltypes
{
public:
    watcher_qmltypes_changes()
    {
        setupParser();

        if (state != FileState::Added) {
            if (noSecondTypeInQmldir) {
                setQmlTypesProjectEntryInfos(
                    qmlDirPathSourceId,
                    {{qmlDirPathSourceId, qmltypes1SourceId, exampleCppModuleId, FileType::QmlTypes}});
            } else {
                setQmlTypesProjectEntryInfos(
                    qmlDirPathSourceId,
                    {{qmlDirPathSourceId, qmltypes1SourceId, exampleCppModuleId, FileType::QmlTypes},
                     {qmlDirPathSourceId, qmltypes2SourceId, exampleCppModuleId, FileType::QmlTypes}});
            }
        }

        setContent(u"/root/path/qmldir", qmldir);
        setQmlTypesFileNames(u"/root/path", {"qmltypes1.qmltypes", "qmltypes2.qmltypes"});
        if (isExisting(state)) {
            setContent(u"/root/path/qmltypes1.qmltypes", qmltypes1Content);
            setContent(u"/root/path/qmltypes2.qmltypes", qmltypes2Content);
        }
        setFilesNotExistsUnchanged({designerDirectoryPathSourceId});

        setFileSystemSubdirectories(u"/root", {"/root/path"});
        setStorageSubdirectories(rootDirectoryPathId, {directoryPathId});
    }

    void setupParser() final
    {
        Type objectType{"QObject",
                        ImportedType{},
                        ImportedType{},
                        Storage::TypeTraitsKind::Reference,
                        qmltypes1SourceId};
        Type itemType{"QItem",
                      ImportedType{},
                      ImportedType{},
                      Storage::TypeTraitsKind::Reference,
                      qmltypes2SourceId};

        ON_CALL(qmlTypesParserMock, parse(qmltypes1Content, _, _, _, _, _))
            .WillByDefault([&, objectType](auto, auto &imports, auto &types, auto, auto, auto) {
                imports.push_back(import1);
                types.push_back(objectType);
            });
        ON_CALL(qmlTypesParserMock, parse(qmltypes2Content, _, _, _, _, _))
            .WillByDefault([&, itemType](auto, auto &imports, auto &types, auto, auto, auto) {
                imports.push_back(import2);
                types.push_back(itemType);
            });
    }

    testing::Matcher<IsInsideProject> createIsInsideProjectMatcher() const
    {
        if (partId == projectPartId)
            return Eq(IsInsideProject::Yes);

        if (partId == qtPartId)
            return Eq(IsInsideProject::No);

        return An<IsInsideProject>();
    }
};

INSTANTIATE_TEST_SUITE_P(
    ProjectStorageUpdater,
    watcher_qmltypes_changes,
    Combine(Values(Watcher::ProjectPartId{.projectPartId = qtPartId, .name = "qt_part"},
                   Watcher::ProjectPartId{.projectPartId = projectPartId, .name = "project_part"},
                   Watcher::ProjectPartId{.projectPartId = otherProjectPartId,
                                          .name = "other_project_part"}),
            Values(Watcher::State{.state = FileState::Added,
                                  .directoryState = FileState::Changed,
                                  .qmldirState = FileState::Changed,
                                  .name = "file_added_directory_changed_qmldir_changed"},
                   Watcher::State{.state = FileState::Added,
                                  .directoryState = FileState::Added,
                                  .qmldirState = FileState::Added,
                                  .name = "file_added_directory_added_qmldir_added"},
                   Watcher::State{.state = FileState::Changed,
                                  .directoryState = FileState::Unchanged,
                                  .qmldirState = FileState::Unchanged,
                                  .name = "file_changed_directory_unchanged_qmldir_unchanged"},
                   Watcher::State{.state = FileState::Changed,
                                  .directoryState = FileState::Unchanged,
                                  .qmldirState = FileState::Changed,
                                  .name = "file_changed_directory_unchanged_qmldir_changed"},
                   Watcher::State{.state = FileState::Changed,
                                  .directoryState = FileState::Changed,
                                  .qmldirState = FileState::Unchanged,
                                  .name = "file_changed_directory_changed_qmldir_unchanged"},
                   Watcher::State{.state = FileState::Unchanged,
                                  .directoryState = FileState::Unchanged,
                                  .qmldirState = FileState::Changed,
                                  .name = "file_unchanged_directory_unchanged_qmldir_changed"}),
            Values(Watcher::Qmldir{.qmldir = R"(module Example
                                                typeinfo qmltypes1.qmltypes
                                                typeinfo qmltypes2.qmltypes)",
                                   .noSecondTypeInQmldir = false,
                                   .name = "qmldir_two_entries"},
                   Watcher::Qmldir{.qmldir = R"(module Example
                                                typeinfo qmltypes1.qmltypes)",
                                   .noSecondTypeInQmldir = true,
                                   .name = "qmldir_one_entries"}),
            Values(Watcher::Subdirectory{.rootSubdirectory = true},
                   Watcher::Subdirectory{.rootSubdirectory = false}),
            Values(Watcher::Error::No, Watcher::Error::Parser, Watcher::Error::Synchronize)),
    watcher_parameters_printer);

TEST_P(watcher_qmltypes_changes, imports)
{
    setFiles(state, {qmltypes1SourceId, qmltypes2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::imports",
                                &SynchronizationPackage::imports,
                                Conditional(isIgnoredPartId or isQmltypesUnchanged,
                                            IsEmpty(),
                                            Conditional(noSecondTypeInQmldir,
                                                        UnorderedElementsAre(import1),
                                                        UnorderedElementsAre(import1, import2)))),
                          Field("SynchronizationPackage::updatedImportSourceIds",
                                &SynchronizationPackage::updatedImportSourceIds,
                                Conditional(isIgnoredPartId or isQmltypesUnchanged,
                                            IsEmpty(),
                                            AllOf(Contains(qmltypes1SourceId),
                                                  Contains(qmltypes2SourceId)
                                                      .Times(noSecondTypeInQmldir ? 0 : 1)))))));

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_qmltypes_changes, types)
{
    setFiles(state, {qmltypes1SourceId, qmltypes2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::types",
                                &SynchronizationPackage::types,
                                Conditional(isIgnoredPartId or isQmltypesUnchanged,
                                            IsEmpty(),
                                            AllOf(Contains(IsStorageType("QObject",
                                                                         ImportedType{},
                                                                         TypeTraitsKind::Reference,
                                                                         qmltypes1SourceId)),
                                                  Contains(IsStorageType("QItem",
                                                                         ImportedType{},
                                                                         TypeTraitsKind::Reference,
                                                                         qmltypes2SourceId))
                                                      .Times(noSecondTypeInQmldir ? 0 : 1)))),
                          Field("SynchronizationPackage::updatedTypeSourceIds",
                                &SynchronizationPackage::updatedTypeSourceIds,
                                Conditional(isIgnoredPartId or isQmltypesUnchanged,
                                            Not(AnyOf(Contains(qmltypes1SourceId),
                                                      Contains(qmltypes2SourceId))),
                                            AllOf(Contains(qmltypes1SourceId),
                                                  Contains(qmltypes2SourceId)
                                                      .Times(noSecondTypeInQmldir ? 0 : 1)))))));

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_qmltypes_changes, file_status)
{
    // TODO!!!! check for the case that qmldir adds qmltypes entry but the qmltypes file already exists
    setFiles(state, {qmltypes1SourceId, qmltypes2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  Conditional(isIgnoredPartId,
                              IsEmpty(),
                              AllOf(Contains(IsFileStatus(directoryPathSourceId, 1, 21))
                                        .Times(isDirectoryUnchanged ? 0 : 1),
                                    Contains(IsFileStatus(qmlDirPathSourceId, 1, 21))
                                        .Times(isQmldirUnchanged ? 0 : 1),
                                    Contains(IsFileStatus(qmltypes1SourceId, 1, 21))
                                        .Times(isQmltypesUnchanged ? 0 : 1),
                                    Contains(IsFileStatus(qmltypes2SourceId, 1, 21))
                                        .Times(isQmltypesUnchanged or noSecondTypeInQmldir ? 0 : 1)))),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  Conditional(isIgnoredPartId,
                              IsEmpty(),
                              AllOf(Contains(directoryPathSourceId).Times(isDirectoryUnchanged ? 0 : 1),
                                    Contains(qmlDirPathSourceId).Times(isQmldirUnchanged ? 0 : 1),
                                    Contains(qmltypes1SourceId).Times(isQmltypesUnchanged ? 0 : 1),
                                    Contains(qmltypes2SourceId)
                                        .Times(isQmltypesUnchanged or noSecondTypeInQmldir ? 0 : 1)))))));

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_qmltypes_changes, project_entry_infos)
{
    setFiles(state, {qmltypes1SourceId, qmltypes2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::projectEntryInfos",
                  &SynchronizationPackage::projectEntryInfos,
                  Conditional(isIgnoredPartId or isDirectoryAndQmldirUnchanged,
                              IsEmpty(),
                              AllOf(Contains(IsProjectEntryInfo(rootDirectoryPathSourceId,
                                                             directoryPathSourceId,
                                                             ModuleId{},
                                                             FileType::Directory))
                                        .Times(useSubdirectory and not isDirectoryUnchanged ? 1 : 0),
                                    Contains(IsProjectEntryInfo(qmlDirPathSourceId,
                                                             qmltypes1SourceId,
                                                             exampleCppModuleId,
                                                             FileType::QmlTypes))
                                        .Times(isQmldirUnchanged ? 0 : 1),
                                    Contains(IsProjectEntryInfo(qmlDirPathSourceId,
                                                             qmltypes2SourceId,
                                                             exampleCppModuleId,
                                                             FileType::QmlTypes))
                                        .Times(noSecondTypeInQmldir or isQmldirUnchanged ? 0 : 1)))),
            Field("SynchronizationPackage::updatedProjectEntryInfoSourceIds",
                  &SynchronizationPackage::updatedProjectEntryInfoSourceIds,
                  Conditional(isIgnoredPartId or isDirectoryAndQmldirUnchanged,
                              IsEmpty(),
                              AllOf(Contains(rootDirectoryPathSourceId)
                                        .Times(useSubdirectory and not isDirectoryUnchanged ? 1 : 0),
                                    Contains(rootQmldirPathSourceId).Times(0),
                                    Contains(directoryPathSourceId).Times(isDirectoryUnchanged ? 0 : 1),
                                    Contains(qmlDirPathSourceId).Times(isQmldirUnchanged ? 0 : 1)))))));

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_qmltypes_changes, qml_document_parses_inside_or_project)
{
    setFiles(state, {qmltypes1SourceId, qmltypes2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    auto isInsideProjectMatcher = createIsInsideProjectMatcher();
    potentionallyThrowError();

    EXPECT_CALL(qmlTypesParserMock, parse(_, _, _, _, _, isInsideProjectMatcher))
        .Times(isIgnoredPartId ? Exactly(0) : AnyNumber());

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_qmltypes_changes, dont_change_watcher_for_qmltypes_changes_only)
{
    setFiles(state, {qmltypes1SourceId, qmltypes2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(patchWatcherMock, updateContextIdPaths(_, _))
        .Times(isDirectoryAndQmldirUnchanged or isIgnoredPartId ? Exactly(0) : Exactly(1));

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_qmltypes_changes, update_watcher)
{
    setFiles(state, {qmltypes1SourceId, qmltypes2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(patchWatcherMock,
                updateContextIdPaths(
                    AllOf(Contains(IsIdPaths(partId,
                                             SourceType::Directory,
                                             Conditional(useSubdirectory,
                                                         UnorderedElementsAre(rootDirectoryPathSourceId,
                                                                              directoryPathSourceId),
                                                         ElementsAre(directoryPathSourceId))))
                              .Times(isDirectoryUnchanged ? 0 : 1),
                          Contains(IsIdPaths(partId,
                                             SourceType::QmlDir,
                                             Conditional(useSubdirectory,
                                                         UnorderedElementsAre(rootQmldirPathSourceId,
                                                                              qmlDirPathSourceId),
                                                         ElementsAre(qmlDirPathSourceId))))
                              .Times(isDirectoryAndQmldirUnchanged ? 0 : 1),
                          Contains(IsIdPaths(partId,
                                             SourceType::QmlTypes,
                                             Conditional(noSecondTypeInQmldir,
                                                         ElementsAre(qmltypes1SourceId),
                                                         UnorderedElementsAre(qmltypes1SourceId,
                                                                              qmltypes2SourceId))))
                              .Times(isDirectoryAndQmldirUnchanged ? 0 : 1)),
                    AllOf(Contains(directoryPathId).Times(isDirectoryUnchanged ? 0 : 1),
                          Contains(rootDirectoryPathId)
                              .Times(not isDirectoryUnchanged and useSubdirectory ? 1 : 0))))
        .Times(isDirectoryAndQmldirUnchanged or isIgnoredPartId ? Exactly(0) : Exactly(1));

    updater.pathsWithIdsChanged(idPaths);
}

class watcher_qmltypes_not_existing : public watcher_qmltypes
{
public:
    watcher_qmltypes_not_existing()
    {
        setupParser();

        if (state == FileState::Removed) {
            if (noSecondTypeInQmldir) {
                setQmlTypesProjectEntryInfos(
                    qmlDirPathSourceId,
                    {{qmlDirPathSourceId, qmltypes1SourceId, exampleCppModuleId, FileType::QmlTypes}});
            } else {
                setQmlTypesProjectEntryInfos(
                    qmlDirPathSourceId,
                    {{qmlDirPathSourceId, qmltypes1SourceId, exampleCppModuleId, FileType::QmlTypes},
                     {qmlDirPathSourceId, qmltypes2SourceId, exampleCppModuleId, FileType::QmlTypes}});
            }
        }

        setContent(u"/root/path/qmldir", qmldir);
        setFilesNotExistsUnchanged({designerDirectoryPathSourceId});

        setFileSystemSubdirectories(u"/root", {"/root/path"});
        setStorageSubdirectories(rootDirectoryPathId, {directoryPathId});
    }

    testing::Matcher<IsInsideProject> createIsInsideProjectMatcher() const
    {
        if (partId == projectPartId)
            return Eq(IsInsideProject::Yes);

        if (partId == qtPartId)
            return Eq(IsInsideProject::No);

        return An<IsInsideProject>();
    }

    void setupParser() final {}

public:
};

INSTANTIATE_TEST_SUITE_P(
    ProjectStorageUpdater,
    watcher_qmltypes_not_existing,
    Combine(Values(Watcher::ProjectPartId{.projectPartId = qtPartId, .name = "qt_part"},
                   Watcher::ProjectPartId{.projectPartId = projectPartId, .name = "project_part"},
                   Watcher::ProjectPartId{.projectPartId = otherProjectPartId,
                                          .name = "other_project_part"}),
            Values(Watcher::State{.state = FileState::Removed,
                                  .directoryState = FileState::Changed,
                                  .qmldirState = FileState::Changed,
                                  .name = "file_removed_directory_changed_qmldir_changed"},
                   Watcher::State{.state = FileState::Removed,
                                  .directoryState = FileState::Changed,
                                  .qmldirState = FileState::Removed,
                                  .name = "file_removed_directory_changed_qmldir_removed"},
                   Watcher::State{.state = FileState::Removed,
                                  .directoryState = FileState::Removed,
                                  .qmldirState = FileState::Removed,
                                  .name = "file_removed_directory_removed_qmldir_removed"}),
            Values(Watcher::Qmldir{.qmldir = R"(module Example
                                                typeinfo qmltypes1.qmltypes
                                                typeinfo qmltypes2.qmltypes)",
                                   .noSecondTypeInQmldir = false,
                                   .name = "qmldir_two_entries"},
                   Watcher::Qmldir{.qmldir = R"(module Example
                                                typeinfo qmltypes1.qmltypes)",
                                   .noSecondTypeInQmldir = true,
                                   .name = "qmldir_one_entries"}),
            Values(Watcher::Subdirectory{.rootSubdirectory = true},
                   Watcher::Subdirectory{.rootSubdirectory = false}),
            Values(Watcher::Error::No, Watcher::Error::Parser, Watcher::Error::Synchronize)),
    watcher_parameters_printer);

TEST_P(watcher_qmltypes_not_existing, imports)
{
    setFiles(state, {qmltypes1SourceId, qmltypes2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports", &SynchronizationPackage::imports, IsEmpty()),
            Field("SynchronizationPackage::updatedImportSourceIds",
                  &SynchronizationPackage::updatedImportSourceIds,
                  Conditional(isIgnoredPartId or (isDirectoryAndQmldirUnchanged and isQmltypesNonExisting),
                              IsEmpty(),
                              AllOf(Contains(qmltypes1SourceId),
                                    Contains(qmltypes2SourceId).Times(noSecondTypeInQmldir ? 0 : 1)))))));

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_qmltypes_not_existing, types)
{
    setFiles(state, {qmltypes1SourceId, qmltypes2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::types",
                                &SynchronizationPackage::types,
                                Conditional(isIgnoredPartId or isQmltypesOrQmldirRemoved
                                                or isQmltypesNonExisting or isQmldirNotExisting,
                                            IsEmpty(),
                                            AllOf(Contains(IsStorageType("QObject",
                                                                         ImportedType{},
                                                                         TypeTraitsKind::Reference,
                                                                         qmltypes1SourceId)),
                                                  Contains(IsStorageType("QItem",
                                                                         ImportedType{},
                                                                         TypeTraitsKind::Reference,
                                                                         qmltypes2SourceId))
                                                      .Times(noSecondTypeInQmldir ? 0 : 1)))),
                          Field("SynchronizationPackage::updatedTypeSourceIds",
                                &SynchronizationPackage::updatedTypeSourceIds,
                                Conditional(isIgnoredPartId
                                                or (isDirectoryAndQmldirUnchanged
                                                    and isQmltypesNonExisting),
                                            IsEmpty(),
                                            AllOf(Contains(qmltypes1SourceId),
                                                  Contains(qmltypes2SourceId)
                                                      .Times(noSecondTypeInQmldir ? 0 : 1)))))));

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_qmltypes_not_existing, file_status)
{
    setFiles(state, {qmltypes1SourceId, qmltypes2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  Conditional(isIgnoredPartId,
                              IsEmpty(),
                              AllOf(Contains(IsFileStatus(directoryPathSourceId, 1, 21))
                                        .Times(isDirectoryUnchanged or isDirectoryRemoved ? 0 : 1),
                                    Contains(IsFileStatus(qmlDirPathSourceId, 1, 21))
                                        .Times(isQmldirUnchanged or isQmldirRemoved ? 0 : 1)))),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  Conditional(
                      isIgnoredPartId,
                      IsEmpty(),
                      AllOf(Contains(directoryPathSourceId).Times(isDirectoryUnchanged ? 0 : 1),
                            Contains(qmlDirPathSourceId).Times(isQmldirUnchanged ? 0 : 1),
                            Contains(qmltypes1SourceId).Times(isQmltypesNonExisting ? 0 : 1),
                            Contains(qmltypes2SourceId)
                                .Times(isQmltypesNonExisting or noSecondTypeInQmldir ? 0 : 1)))))));

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_qmltypes_not_existing, project_entry_infos)
{
    setFiles(state, {qmltypes1SourceId, qmltypes2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::projectEntryInfos",
                                &SynchronizationPackage::projectEntryInfos,
                                Conditional(isIgnoredPartId or isDirectoryUnchanged or isDirectoryRemoved,
                                            IsEmpty(),
                                            AllOf(Contains(IsProjectEntryInfo(rootDirectoryPathSourceId,
                                                                           directoryPathSourceId,
                                                                           ModuleId{},
                                                                           FileType::Directory))
                                                      .Times(useSubdirectory ? 1 : 0)))),
                          Field("SynchronizationPackage::updatedProjectEntryInfoSourceIds",
                                &SynchronizationPackage::updatedProjectEntryInfoSourceIds,
                                Conditional(isIgnoredPartId or isDirectoryUnchanged,
                                            IsEmpty(),
                                            AllOf(Contains(rootDirectoryPathSourceId)
                                                      .Times(useSubdirectory ? 1 : 0),
                                                  Contains(qmlDirPathSourceId)))))));

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_qmltypes_not_existing, qml_document_parses_inside_or_project)
{
    setFiles(state, {qmltypes1SourceId, qmltypes2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    auto isInsideProjectMatcher = createIsInsideProjectMatcher();
    potentionallyThrowError();

    EXPECT_CALL(qmlDocumentParserMock, parse(_, _, _, _, isInsideProjectMatcher))
        .Times(isIgnoredPartId ? Exactly(0) : AnyNumber());

    updater.pathsWithIdsChanged(idPaths);
}

TEST_P(watcher_qmltypes_not_existing, update_watcher)
{
    setFiles(state, {qmltypes1SourceId, qmltypes2SourceId});
    setFiles(directoryState, {rootDirectoryPathSourceId, directoryPathSourceId});
    setFiles(qmldirState, {rootQmldirPathSourceId, qmlDirPathSourceId});
    potentionallyThrowError();

    EXPECT_CALL(patchWatcherMock,
                updateContextIdPaths(
                    AllOf(Contains(IsIdPaths(partId,
                                             SourceType::Directory,
                                             Conditional(useSubdirectory,
                                                         UnorderedElementsAre(rootDirectoryPathSourceId,
                                                                              directoryPathSourceId),
                                                         ElementsAre(directoryPathSourceId))))
                              .Times(isDirectoryRemoved ? 0 : 1),
                          Contains(IsIdPaths(partId,
                                             SourceType::QmlDir,
                                             Conditional(useSubdirectory,
                                                         UnorderedElementsAre(rootQmldirPathSourceId,
                                                                              qmlDirPathSourceId),
                                                         ElementsAre(qmlDirPathSourceId))))
                              .Times(isQmldirNotExisting or isQmldirRemoved ? 0 : 1),
                          Contains(IsIdPaths(partId,
                                             SourceType::Qml,
                                             UnorderedElementsAre(qmltypes1SourceId, qmltypes2SourceId)))
                              .Times(isQmltypesNonExisting or isQmltypesRemoved ? 0 : 1)),
                    AllOf(Contains(directoryPathId),
                          Contains(rootDirectoryPathId).Times(useSubdirectory ? 1 : 0))))
        .Times(isIgnoredPartId ? Exactly(0) : Exactly(1));

    updater.pathsWithIdsChanged(idPaths);
}

class ProjectStorageUpdater_property_editor_panes : public BaseProjectStorageUpdater
{
public:
    ProjectStorageUpdater_property_editor_panes()
    {
        ON_CALL(fileSystemMock, fileStatus(_)).WillByDefault([](SourceId sourceId) {
            return createFileStatus(sourceId, 1, 21);
        });
        ON_CALL(projectStorageMock, fetchFileStatus(_)).WillByDefault([](SourceId sourceId) {
            return createFileStatus(sourceId, 1, 21);
        });
    }

public:
    const QString propertyEditorQmlPath = QDir::cleanPath(
        UNITTEST_DIR "/../../../../share/qtcreator/qmldesigner/propertyEditorQmlSources/");
    SourceId sourceId = sourcePathCache.sourceId(
        QmlDesigner::SourcePath{propertyEditorQmlPath + "/QML/QtObjectPane.qml"});
    DirectoryPathId directoryId = sourcePathCache.directoryPathId(
        QmlDesigner::SourcePath{propertyEditorQmlPath + "/QML"});
    SourceId directorySourceId = SourceId::create(directoryId, QmlDesigner::FileNameId{});
    SourceId textSourceId = sourcePathCache.sourceId(
        QmlDesigner::SourcePath{propertyEditorQmlPath + "/QtQuick/TextSpecifics.qml"});
    DirectoryPathId qtQuickDirectoryId = sourcePathCache.directoryPathId(
        QmlDesigner::SourcePath{propertyEditorQmlPath + "/QtQuick"});
    SourceId qtQuickDirectorySourceId = SourceId::create(qtQuickDirectoryId,
                                                         QmlDesigner::FileNameId{});
    SourceId buttonSourceId = sourcePathCache.sourceId(
        QmlDesigner::SourcePath{propertyEditorQmlPath + "/QtQuick/Controls/ButtonSpecifics.qml"});
    DirectoryPathId controlsDirectoryId = sourcePathCache.directoryPathId(
        QmlDesigner::SourcePath{propertyEditorQmlPath + "/QtQuick/Controls"});
    SourceId controlsDirectorySourceId = SourceId::create(controlsDirectoryId,
                                                          QmlDesigner::FileNameId{});
    ModuleId qtQuickModuleId = modulesStorage.moduleId("QtQuick", ModuleKind::QmlLibrary);
    ModuleId controlsModuleId = modulesStorage.moduleId("QtQuick.Controls", ModuleKind::QmlLibrary);
    ModuleId qmlModuleId = modulesStorage.moduleId("QML", ModuleKind::QmlLibrary);
    Update update = {.propertyEditorResourcesPath = propertyEditorQmlPath};
};

TEST_F(ProjectStorageUpdater_property_editor_panes, update)
{
    setFilesChanged({directorySourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::propertyEditorQmlPaths",
                                &SynchronizationPackage::propertyEditorQmlPaths,
                                Contains(IsPropertyEditorQmlPath(
                                    qmlModuleId, "QtObject", sourceId, directoryId))),
                          Field("SynchronizationPackage::updatedPropertyEditorQmlPathDirectoryIds",
                                &SynchronizationPackage::updatedPropertyEditorQmlPathDirectoryIds,
                                ElementsAre(directoryId)))));

    updater.update(update);
}

TEST_F(ProjectStorageUpdater_property_editor_panes, specifics)
{
    setFilesChanged({qtQuickDirectorySourceId, controlsDirectorySourceId});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(
            AllOf(Field("SynchronizationPackage::propertyEditorQmlPaths",
                        &SynchronizationPackage::propertyEditorQmlPaths,
                        IsSupersetOf(
                            {IsPropertyEditorQmlPath(qtQuickModuleId, "Text", textSourceId, qtQuickDirectoryId),
                             IsPropertyEditorQmlPath(
                                 controlsModuleId, "Button", buttonSourceId, controlsDirectoryId)})),
                  Field("SynchronizationPackage::updatedPropertyEditorQmlPathDirectoryIds",
                        &SynchronizationPackage::updatedPropertyEditorQmlPathDirectoryIds,
                        ElementsAre(qtQuickDirectoryId, controlsDirectoryId)))));

    updater.update(update);
}

TEST_F(ProjectStorageUpdater_property_editor_panes, is_empty_if_directory_has_not_changed)
{
    updater.update(update);

    EXPECT_CALL(projectStorageMock, synchronize(PackageIsEmpty()));

    updater.update(update);
}

class ProjectStorageUpdater_global_type_annotations : public BaseProjectStorageUpdater
{
public:
    ProjectStorageUpdater_global_type_annotations()
    {
        ON_CALL(fileSystemMock, fileStatus(_)).WillByDefault([](SourceId sourceId) {
            return createFileStatus(sourceId, 1, 21);
        });
        ON_CALL(projectStorageMock, fetchFileStatus(_)).WillByDefault([](SourceId sourceId) {
            return createFileStatus(sourceId, 1, 21);
        });

        ON_CALL(projectStorageMock, typeAnnotationDirectoryIds())
            .WillByDefault(Return(QmlDesigner::SmallDirectoryPathIds<64>{
                itemLibraryPathDirectoryPathId,
            }));
        ON_CALL(projectStorageMock, typeAnnotationSourceIds(itemLibraryPathDirectoryPathId))
            .WillByDefault(
                Return(QmlDesigner::SmallSourceIds<4>{qtQuickSourceId, qtQuickControlSourceId}));

        itemTraits.canBeContainer = QmlDesigner::FlagIs::True;
    }

public:
    const QString itemLibraryPath = QDir::cleanPath(
        UNITTEST_DIR "/../../../../share/qtcreator/qmldesigner/itemLibrary/");
    DirectoryPathId itemLibraryPathDirectoryPathId = sourcePathCache.directoryPathId(
        Utils::PathString{itemLibraryPath});
    SourceId itemLibraryPathSourceId = SourceId::create(itemLibraryPathDirectoryPathId, FileNameId{});
    const QString qmlImportsPath = QDir::cleanPath(UNITTEST_DIR "/projectstorage/data/qml");
    DirectoryPathId qmlImportsPathDirectoryPathId = sourcePathCache.directoryPathId(
        Utils::PathString{itemLibraryPath});
    SourceId qmlImportsPathSourceId = SourceId::create(qmlImportsPathDirectoryPathId, FileNameId{});
    SourceId qtQuickSourceId = sourcePathCache.sourceId(
        QmlDesigner::SourcePath{itemLibraryPath + "/quick.metainfo"});
    SourceId qtQuickControlSourceId = sourcePathCache.sourceId(
        QmlDesigner::SourcePath{itemLibraryPath + "/qtquickcontrols2.metainfo"});
    ModuleId qtQuickModuleId = moduleId("QtQuick", ModuleKind::QmlLibrary);
    ModuleId qtQuickControlsModuleId = moduleId("QtQuick.Controls.Basic", ModuleKind::QmlLibrary);
    QmlDesigner::Storage::TypeTraits itemTraits;
    Update update = {.typeAnnotationPaths = {itemLibraryPath}};
};

TEST_F(ProjectStorageUpdater_global_type_annotations, update)
{
    setFilesAdded({itemLibraryPathSourceId, qtQuickSourceId, qtQuickControlSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::typeAnnotations",
                                        &SynchronizationPackage::typeAnnotations,
                                        IsSupersetOf({IsTypeAnnotation(qtQuickSourceId,
                                                                       itemLibraryPathDirectoryPathId,
                                                                       "Item",
                                                                       qtQuickModuleId,
                                                                       StartsWith(itemLibraryPath),
                                                                       _,
                                                                       _,
                                                                       _),
                                                      IsTypeAnnotation(qtQuickControlSourceId,
                                                                       itemLibraryPathDirectoryPathId,
                                                                       "Button",
                                                                       qtQuickControlsModuleId,
                                                                       StartsWith(itemLibraryPath),
                                                                       _,
                                                                       _,
                                                                       _)})),
                                  Field("SynchronizationPackage::updatedTypeAnnotationSourceIds",
                                        &SynchronizationPackage::updatedTypeAnnotationSourceIds,
                                        IsSupersetOf({qtQuickSourceId, qtQuickControlSourceId})))));

    updater.update(update);
}

TEST_F(ProjectStorageUpdater_global_type_annotations, add)
{
    setFilesUnchanged({itemLibraryPathSourceId});
    setFilesAdded({qtQuickSourceId, qtQuickControlSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::typeAnnotations",
                                        &SynchronizationPackage::typeAnnotations,
                                        IsSupersetOf({IsTypeAnnotation(qtQuickSourceId,
                                                                       itemLibraryPathDirectoryPathId,
                                                                       "Item",
                                                                       qtQuickModuleId,
                                                                       StartsWith(itemLibraryPath),
                                                                       _,
                                                                       _,
                                                                       _),
                                                      IsTypeAnnotation(qtQuickControlSourceId,
                                                                       itemLibraryPathDirectoryPathId,
                                                                       "Button",
                                                                       qtQuickControlsModuleId,
                                                                       StartsWith(itemLibraryPath),
                                                                       _,
                                                                       _,
                                                                       _)})),
                                  Field("SynchronizationPackage::updatedTypeAnnotationSourceIds",
                                        &SynchronizationPackage::updatedTypeAnnotationSourceIds,
                                        IsSupersetOf({qtQuickSourceId, qtQuickControlSourceId})))));

    updater.update(update);
}

TEST_F(ProjectStorageUpdater_global_type_annotations, changed)
{
    setFilesUnchanged({itemLibraryPathSourceId});
    setFilesChanged({qtQuickSourceId, qtQuickControlSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::typeAnnotations",
                                        &SynchronizationPackage::typeAnnotations,
                                        IsSupersetOf({IsTypeAnnotation(qtQuickSourceId,
                                                                       itemLibraryPathDirectoryPathId,
                                                                       "Item",
                                                                       qtQuickModuleId,
                                                                       StartsWith(itemLibraryPath),
                                                                       _,
                                                                       _,
                                                                       _),
                                                      IsTypeAnnotation(qtQuickControlSourceId,
                                                                       itemLibraryPathDirectoryPathId,
                                                                       "Button",
                                                                       qtQuickControlsModuleId,
                                                                       StartsWith(itemLibraryPath),
                                                                       _,
                                                                       _,
                                                                       _)})),
                                  Field("SynchronizationPackage::updatedTypeAnnotationSourceIds",
                                        &SynchronizationPackage::updatedTypeAnnotationSourceIds,
                                        IsSupersetOf({qtQuickSourceId, qtQuickControlSourceId})))));

    updater.update(update);
}

TEST_F(ProjectStorageUpdater_global_type_annotations, remove)
{
    setFilesRemoved({itemLibraryPathSourceId, qtQuickSourceId, qtQuickControlSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::typeAnnotations",
                                        &SynchronizationPackage::typeAnnotations,
                                        IsEmpty()),
                                  Field("SynchronizationPackage::updatedTypeAnnotationSourceIds",
                                        &SynchronizationPackage::updatedTypeAnnotationSourceIds,
                                        IsSupersetOf({qtQuickSourceId, qtQuickControlSourceId})))));

    updater.update(update);
}

TEST_F(ProjectStorageUpdater_global_type_annotations, removed_meta_info_file)
{
    setFilesChanged({itemLibraryPathSourceId});
    setFilesRemoved({qtQuickSourceId});
    setFilesUnchanged({qtQuickControlSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::typeAnnotations",
                                        &SynchronizationPackage::typeAnnotations,
                                        IsEmpty()),
                                  Field("SynchronizationPackage::updatedTypeAnnotationSourceIds",
                                        &SynchronizationPackage::updatedTypeAnnotationSourceIds,
                                        AllOf(IsSupersetOf({qtQuickSourceId}),
                                              Not(Contains(qtQuickControlSourceId)))))));

    updater.update(update);
}

TEST_F(ProjectStorageUpdater_global_type_annotations, removed_directory)
{
    setFilesRemoved({itemLibraryPathSourceId, qtQuickSourceId, qtQuickControlSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::typeAnnotations",
                                        &SynchronizationPackage::typeAnnotations,
                                        IsEmpty()),
                                  Field("SynchronizationPackage::updatedTypeAnnotationSourceIds",
                                        &SynchronizationPackage::updatedTypeAnnotationSourceIds,
                                        IsSupersetOf({qtQuickSourceId, qtQuickControlSourceId})))));

    updater.update({.typeAnnotationPaths = {itemLibraryPath}});
}

class ProjectStorageUpdater_added_property_editor_qml_paths : public BaseProjectStorageUpdater
{
public:
    ProjectStorageUpdater_added_property_editor_qml_paths()
    {
        QString qmldir{R"(module Bar)"};
        setContent(u"/path/one/qmldir", qmldir);

        setFileNames(u"/path/one/designer",
                     {"FooSpecifics.qml", "HuoSpecificsDynamic.qml", "CaoPane.qml"},
                     {"*Pane.qml", "*Specifics.qml", "*SpecificsDynamic.qml"});
    }

public:
    DirectoryPathId path1DirectoryPathId = sourcePathCache.directoryPathId("/path/one");
    SourceId path1SourceId = SourceId::create(path1DirectoryPathId);
    DirectoryPathId path2DirectoryPathId = sourcePathCache.directoryPathId("/path/two");
    SourceId path2SourceId = SourceId::create(path2DirectoryPathId);
    SourceId qmldir1SourceId = sourcePathCache.sourceId("/path/one/qmldir");
    SourceId qmldir2SourceId = sourcePathCache.sourceId("/path/two/qmldir");
    DirectoryPathId designer1DirectoryId = sourcePathCache.directoryPathId("/path/one/designer");
    SourceId designer1SourceId = SourceId::create(designer1DirectoryId);
    DirectoryPathId designer2DirectoryId = sourcePathCache.directoryPathId("/path/two/designer");
    SourceId designer2SourceId = SourceId::create(designer2DirectoryId);
    SourceId propertyEditorSpecificSourceId = sourcePathCache.sourceId(
        "/path/one/designer/FooSpecifics.qml");
    SourceId propertyEditorSpecificDynamicSourceId = sourcePathCache.sourceId(
        "/path/one/designer/HuoSpecificsDynamic.qml");
    SourceId propertyEditorPaneSourceId = sourcePathCache.sourceId(
        "/path/one/designer/CaoPane.qml");
    ModuleId barModuleId = modulesStorage.moduleId("Bar", ModuleKind::QmlLibrary);
};

TEST_F(ProjectStorageUpdater_added_property_editor_qml_paths,
       synchronize_added_property_editor_qml_paths_directory)
{
    setFileSystemSubdirectories(u"/path/one", {"/path/one/designer"});
    setFilesUnchanged({path1SourceId});
    setFilesChanged({qmldir1SourceId});
    setFilesAdded({designer1SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::fileStatuses",
                                &SynchronizationPackage::fileStatuses,
                                IsSupersetOf({IsFileStatus(designer1SourceId, 1, 21)})),
                          Field("SynchronizationPackage::updatedFileStatusSourceIds",
                                &SynchronizationPackage::updatedFileStatusSourceIds,
                                IsSupersetOf({designer1SourceId})),
                          Field("SynchronizationPackage::propertyEditorQmlPaths",
                                &SynchronizationPackage::propertyEditorQmlPaths,
                                Each(Field("PropertyEditorQmlPath::directoryId",
                                           &PropertyEditorQmlPath::directoryId,
                                           designer1DirectoryId))),
                          Field("SynchronizationPackage::updatedPropertyEditorQmlPathDirectoryIds",
                                &SynchronizationPackage::updatedPropertyEditorQmlPathDirectoryIds,
                                UnorderedElementsAre(designer1DirectoryId)))));

    updater.update({.projectDirectory = "/path/one"});
}

TEST_F(ProjectStorageUpdater_added_property_editor_qml_paths,
       synchronize_changed_property_editor_qml_paths_directory)
{
    setFileSystemSubdirectories(u"/path/one", {"/path/one/designer"});
    setFilesUnchanged({path1SourceId});
    setFilesChanged({qmldir1SourceId});
    setFilesChanged({designer1SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::fileStatuses",
                                &SynchronizationPackage::fileStatuses,
                                IsSupersetOf({IsFileStatus(designer1SourceId, 1, 21)})),
                          Field("SynchronizationPackage::updatedFileStatusSourceIds",
                                &SynchronizationPackage::updatedFileStatusSourceIds,
                                IsSupersetOf({designer1SourceId})),
                          Field("SynchronizationPackage::propertyEditorQmlPaths",
                                &SynchronizationPackage::propertyEditorQmlPaths,
                                Each(Field("PropertyEditorQmlPath::directoryId",
                                           &PropertyEditorQmlPath::directoryId,
                                           designer1DirectoryId))),
                          Field("SynchronizationPackage::updatedPropertyEditorQmlPathDirectoryIds",
                                &SynchronizationPackage::updatedPropertyEditorQmlPathDirectoryIds,
                                UnorderedElementsAre(designer1DirectoryId)))));

    updater.update({.projectDirectory = "/path/one"});
}

TEST_F(ProjectStorageUpdater_added_property_editor_qml_paths,
       dont_synchronize_empty_property_editor_qml_paths_directory)
{
    setFileSystemSubdirectories(u"/path/two", {});
    setFilesChanged({path2SourceId});
    setFilesChanged({qmldir2SourceId});
    setFilesNotExists({designer2SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::fileStatuses",
                                &SynchronizationPackage::fileStatuses,
                                AllOf(Contains(IsFileStatus(path2SourceId, 1, 21)),
                                      Contains(IsNullFileStatus(designer2SourceId)))),
                          Field("SynchronizationPackage::updatedFileStatusSourceIds",
                                &SynchronizationPackage::updatedFileStatusSourceIds,
                                IsSupersetOf({designer2SourceId})),
                          Field("SynchronizationPackage::propertyEditorQmlPaths",
                                &SynchronizationPackage::propertyEditorQmlPaths,
                                IsEmpty()),
                          Field("SynchronizationPackage::updatedPropertyEditorQmlPathDirectoryIds",
                                &SynchronizationPackage::updatedPropertyEditorQmlPathDirectoryIds,
                                IsEmpty()),
                          Field("SynchronizationPackage::projectEntryInfos",
                                &SynchronizationPackage::projectEntryInfos,
                                IsEmpty()),
                          Field("SynchronizationPackage::updatedProjectEntryInfoSourceIds",
                                &SynchronizationPackage::updatedProjectEntryInfoSourceIds,
                                UnorderedElementsAre(path2SourceId)))));

    updater.update({.projectDirectory = "/path/two"});
}

TEST_F(ProjectStorageUpdater_added_property_editor_qml_paths,
       remove_property_editor_qml_paths_if_designer_directory_is_removed_and_qmldir_is_changed)
{
    setFilesUnchanged({path1SourceId});
    setFilesRemoved({qmldir1SourceId, designer1SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::fileStatuses",
                                &SynchronizationPackage::fileStatuses,
                                IsEmpty()),
                          Field("SynchronizationPackage::updatedFileStatusSourceIds",
                                &SynchronizationPackage::updatedFileStatusSourceIds,
                                IsSupersetOf({designer1SourceId})),
                          Field("SynchronizationPackage::propertyEditorQmlPaths",
                                &SynchronizationPackage::propertyEditorQmlPaths,
                                IsEmpty()),
                          Field("SynchronizationPackage::updatedPropertyEditorQmlPathDirectoryIds",
                                &SynchronizationPackage::updatedPropertyEditorQmlPathDirectoryIds,
                                UnorderedElementsAre(designer1DirectoryId)))));

    updater.update({.projectDirectory = "/path/one"});
}

TEST_F(ProjectStorageUpdater_added_property_editor_qml_paths,
       synchronize_property_editor_qml_paths_directory_if_designer_directory_and_qmldir_is_changed)
{
    setFileSystemSubdirectories(u"/path/one", {"/path/one/designer"});
    setFilesChanged({designer1SourceId, qmldir1SourceId});
    setFilesUnchanged({
        path1SourceId,
    });

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::propertyEditorQmlPaths",
                                &SynchronizationPackage::propertyEditorQmlPaths,
                                IsSupersetOf({IsPropertyEditorQmlPath(barModuleId,
                                                                      Eq("Foo"),
                                                                      propertyEditorSpecificSourceId,
                                                                      designer1DirectoryId),
                                              IsPropertyEditorQmlPath(barModuleId,
                                                                      Eq("Huo"),
                                                                      propertyEditorSpecificDynamicSourceId,
                                                                      designer1DirectoryId),
                                              IsPropertyEditorQmlPath(barModuleId,
                                                                      Eq("Cao"),
                                                                      propertyEditorPaneSourceId,
                                                                      designer1DirectoryId)})),
                          Field("SynchronizationPackage::updatedPropertyEditorQmlPathDirectoryIds",
                                &SynchronizationPackage::updatedPropertyEditorQmlPathDirectoryIds,
                                Contains(designer1DirectoryId)))));

    updater.update({.projectDirectory = "/path/one"});
}

} // namespace
