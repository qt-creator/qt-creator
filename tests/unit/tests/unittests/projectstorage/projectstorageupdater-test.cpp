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
using Storage::Import;
using Storage::IsInsideProject;
using Storage::ModuleKind;
using Storage::Synchronization::ChangeLevel;
using Storage::Synchronization::DirectoryInfo;
using Storage::Synchronization::DirectoryInfos;
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
using FileState = QmlDesigner::ProjectStorageUpdater::FileState;
using Update = QmlDesigner::ProjectStorageUpdater::Update;

MATCHER_P5(IsStorageType,
           typeName,
           prototype,
           traits,
           sourceId,
           changeLevel,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Type(typeName, prototype, traits, sourceId, changeLevel)))
{
    const Type &type = arg;

    return type.typeName == typeName && type.traits == traits && type.sourceId == sourceId
           && ImportedTypeName{prototype} == type.prototype && type.changeLevel == changeLevel;
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

MATCHER_P4(
    IsExportedType,
    moduleId,
    name,
    majorVersion,
    minorVersion,
    std::string(negation ? "isn't " : "is ")
        + PrintToString(ExportedType{moduleId, name, Storage::Version{majorVersion, minorVersion}}))
{
    const ExportedType &type = arg;

    return type.moduleId == moduleId && type.name == name
           && type.version == Storage::Version{majorVersion, minorVersion};
}

MATCHER_P3(IsFileStatus,
           sourceId,
           size,
           lastModified,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(FileStatus{sourceId, size, lastModified}))
{
    const FileStatus &fileStatus = arg;

    return fileStatus.sourceId == sourceId && fileStatus.size == size
           && fileStatus.lastModified == lastModified;
}

MATCHER_P4(IsDirectoryInfo,
           directoryId,
           sourceId,
           moduleId,
           fileType,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(DirectoryInfo{directoryId, sourceId, moduleId, fileType}))
{
    const DirectoryInfo &directoryInfo = arg;

    return compareInvalidAreTrue(directoryInfo.directoryId, directoryId)
           && directoryInfo.sourceId == sourceId
           && compareInvalidAreTrue(directoryInfo.moduleId, moduleId)
           && directoryInfo.fileType == fileType;
}

MATCHER(PackageIsEmpty, std::string(negation ? "isn't empty" : "is empty"))
{
    const SynchronizationPackage &package = arg;

    return package.imports.empty() && package.types.empty() && package.fileStatuses.empty()
           && package.updatedSourceIds.empty() && package.directoryInfos.empty()
           && package.updatedFileStatusSourceIds.empty()
           && package.updatedDirectoryInfoDirectoryIds.empty() && package.moduleDependencies.empty()
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

class BaseProjectStorageUpdater : public testing::Test
{
public:
    struct StaticData
    {
        Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
        Sqlite::Database sourcePathDatabase{":memory:", Sqlite::JournalMode::Memory};
        NiceMock<ProjectStorageErrorNotifierMock> errorNotifierMock;
        QmlDesigner::ProjectStorage storage{database, errorNotifierMock, database.isInitialized()};
        QmlDesigner::SourcePathStorage sourcePathStorage{sourcePathDatabase,
                                                         sourcePathDatabase.isInitialized()};
    };

    static void SetUpTestSuite() { staticData = std::make_unique<StaticData>(); }

    static void TearDownTestSuite() { staticData.reset(); }

    BaseProjectStorageUpdater()
    {
        ON_CALL(projectStorageMock, moduleId(_, _)).WillByDefault([&](const auto &name, const auto &kind) {
            return storage.moduleId(name, kind);
        });
    }

    ~BaseProjectStorageUpdater() { storage.resetForTestsOnly(); }

    void setFilesUnchanged(const QmlDesigner::SourceIds &sourceIds)
    {
        for (auto sourceId : sourceIds) {
            ON_CALL(fileSystemMock, fileStatus(Eq(sourceId)))
                .WillByDefault(Return(FileStatus{sourceId, 2, 421}));
            ON_CALL(projectStorageMock, fetchFileStatus(Eq(sourceId)))
                .WillByDefault(Return(FileStatus{sourceId, 2, 421}));
        }
    }

    void setFilesChanged(const QmlDesigner::SourceIds &sourceIds)
    {
        for (auto sourceId : sourceIds) {
            ON_CALL(fileSystemMock, fileStatus(Eq(sourceId)))
                .WillByDefault(Return(FileStatus{sourceId, 1, 21}));
            ON_CALL(projectStorageMock, fetchFileStatus(Eq(sourceId)))
                .WillByDefault(Return(FileStatus{sourceId, 2, 421}));
        }
    }

    void setFilesAdded(const QmlDesigner::SourceIds &sourceIds)
    {
        for (auto sourceId : sourceIds) {
            ON_CALL(fileSystemMock, fileStatus(Eq(sourceId)))
                .WillByDefault(Return(FileStatus{sourceId, 1, 21}));
            ON_CALL(projectStorageMock, fetchFileStatus(Eq(sourceId)))
                .WillByDefault(Return(FileStatus{}));
        }
    }

    void setFilesRemoved(const QmlDesigner::SourceIds &sourceIds)
    {
        for (auto sourceId : sourceIds) {
            ON_CALL(fileSystemMock, fileStatus(Eq(sourceId)))
                .WillByDefault(Return(FileStatus{sourceId, -1, -1}));
            ON_CALL(projectStorageMock, fetchFileStatus(Eq(sourceId)))
                .WillByDefault(Return(FileStatus{sourceId, 1, 21}));
        }
    }

    void setFilesNotExists(const QmlDesigner::SourceIds &sourceIds)
    {
        for (auto sourceId : sourceIds) {
            ON_CALL(fileSystemMock, fileStatus(Eq(sourceId)))
                .WillByDefault(Return(FileStatus{sourceId, -1, -1}));
            ON_CALL(projectStorageMock, fetchFileStatus(Eq(sourceId)))
                .WillByDefault(Return(FileStatus{SourceId{}, -1, -1}));
        }
    }

    void setFilesNotExistsUnchanged(const QmlDesigner::SourceIds &sourceIds)
    {
        for (auto sourceId : sourceIds) {
            ON_CALL(fileSystemMock, fileStatus(Eq(sourceId)))
                .WillByDefault(Return(FileStatus{sourceId, -1, -1}));
            ON_CALL(projectStorageMock, fetchFileStatus(Eq(sourceId)))
                .WillByDefault(Return(FileStatus{sourceId, -1, -1}));
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

    void setDirectoryInfos(DirectoryPathId directorySourceId, const DirectoryInfos &directoryInfos)
    {
        ON_CALL(projectStorageMock, fetchDirectoryInfos(Eq(directorySourceId)))
            .WillByDefault(Return(directoryInfos));
        for (const DirectoryInfo &directoryInfo : directoryInfos) {
            ON_CALL(projectStorageMock, fetchDirectoryInfo(Eq(directoryInfo.sourceId)))
                .WillByDefault(Return(std::optional{directoryInfo}));
        }
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
        return storage.moduleId(name, kind);
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

protected:
    NiceMock<FileSystemMock> fileSystemMock;
    NiceMock<ProjectStorageMock> projectStorageMock;
    NiceMock<QmlTypesParserMock> qmlTypesParserMock;
    NiceMock<QmlDocumentParserMock> qmlDocumentParserMock;
    QmlDesigner::FileStatusCache fileStatusCache{fileSystemMock};
    inline static std::unique_ptr<StaticData> staticData;
    Sqlite::Database &database = staticData->database;
    QmlDesigner::ProjectStorage &storage = staticData->storage;
    QmlDesigner::SourcePathCache<QmlDesigner::SourcePathStorage> sourcePathCache{
        staticData->sourcePathStorage};
    NiceMock<ProjectStoragePathWatcherMock> patchWatcherMock;
    NiceMock<ProjectStorageErrorNotifierMock> errorNotifierMock;
    QmlDesigner::ProjectPartId projectPartId = QmlDesigner::ProjectPartId::create(1);
    QmlDesigner::ProjectPartId otherProjectPartId = QmlDesigner::ProjectPartId::create(2);
    QmlDesigner::ProjectPartId qtPartId = QmlDesigner::ProjectPartId::create(10);
    QmlDesigner::ProjectStorageUpdater updater{fileSystemMock,
                                               projectStorageMock,
                                               fileStatusCache,
                                               sourcePathCache,
                                               qmlDocumentParserMock,
                                               qmlTypesParserMock,
                                               patchWatcherMock,
                                               errorNotifierMock,
                                               projectPartId,
                                               qtPartId};
};

class ProjectStorageUpdater : public BaseProjectStorageUpdater
{
public:
    ProjectStorageUpdater()
    {
        setFilesChanged({qmltypesPathSourceId, qmlDirPathSourceId, qmlDocumentSourceId1});
        setFilesAdded({qmltypes2PathSourceId, qmlDocumentSourceId2, qmlDocumentSourceId3});

        setFilesUnchanged({directoryPathSourceId,
                           path1SourceId,
                           path2SourceId,
                           path3SourceId,
                           firstSourceId,
                           secondSourceId,
                           thirdSourceId,
                           qmltypes1SourceId,
                           qmltypes2SourceId,
                           itemLibraryPathSourceId});
        setFilesNotExistsUnchanged({createDirectorySourceId("/path/designer"),
                                    createDirectorySourceId("/root/designer"),
                                    createDirectorySourceId("/path/one/designer"),
                                    createDirectorySourceId("/path/two/designer"),
                                    createDirectorySourceId("/path/three/designer")});

        setFilesAdded({qmldir1SourceId, qmldir2SourceId, qmldir3SourceId});

        setContent(u"/path/qmldir", qmldirContent);

        setQmlFileNames(u"/path", {"First.qml", "First2.qml", "Second.qml"});

        firstType.prototype = ImportedType{"Object"};
        firstType.traits = TypeTraitsKind::Reference;
        secondType.prototype = ImportedType{"Object2"};
        secondType.traits = TypeTraitsKind::Reference;
        thirdType.prototype = ImportedType{"Object3"};
        thirdType.traits = TypeTraitsKind::Reference;

        setContent(u"/path/First.qml", qmlDocument1);
        setContent(u"/path/First2.qml", qmlDocument2);
        setContent(u"/path/Second.qml", qmlDocument3);
        setContent(u"/path/example.qmltypes", qmltypes1);
        setContent(u"/path/example2.qmltypes", qmltypes2);
        setContent(u"/path/one/First.qml", qmlDocument1);
        setContent(u"/path/one/Second.qml", qmlDocument2);
        setContent(u"/path/two/Third.qml", qmlDocument3);
        setContent(u"/path/one/example.qmltypes", qmltypes1);
        setContent(u"/path/two/example2.qmltypes", qmltypes2);

        ON_CALL(qmlDocumentParserMock, parse(qmlDocument1, _, _, _, _))
            .WillByDefault([&](auto, auto &imports, auto, auto, auto) {
                imports.push_back(import1);
                return firstType;
            });
        ON_CALL(qmlDocumentParserMock, parse(qmlDocument2, _, _, _, _))
            .WillByDefault([&](auto, auto &imports, auto, auto, auto) {
                imports.push_back(import2);
                return secondType;
            });
        ON_CALL(qmlDocumentParserMock, parse(qmlDocument3, _, _, _, _))
            .WillByDefault([&](auto, auto &imports, auto, auto, auto) {
                imports.push_back(import3);
                return thirdType;
            });
        ON_CALL(qmlTypesParserMock, parse(Eq(qmltypes1), _, _, _, _))
            .WillByDefault([&](auto, auto &imports, auto &types, auto, auto) {
                types.push_back(objectType);
                imports.push_back(import4);
            });
        ON_CALL(qmlTypesParserMock, parse(Eq(qmltypes2), _, _, _, _))
            .WillByDefault([&](auto, auto &imports, auto &types, auto, auto) {
                types.push_back(itemType);
                imports.push_back(import5);
            });
    }

protected:
    SourceId qmltypesPathSourceId = sourcePathCache.sourceId("/path/example.qmltypes");
    SourceId qmltypes2PathSourceId = sourcePathCache.sourceId("/path/example2.qmltypes");
    SourceId qmlDirPathSourceId = sourcePathCache.sourceId("/path/qmldir");
    DirectoryPathId directoryPathId = qmlDirPathSourceId.directoryPathId();
    SourceId directoryPathSourceId = SourceId::create(directoryPathId, QmlDesigner::FileNameId{});
    SourceId annotationDirectorySourceId = createDirectorySourceId("/path/designer");
    SourceId qmlDocumentSourceId1 = sourcePathCache.sourceId("/path/First.qml");
    SourceId qmlDocumentSourceId2 = sourcePathCache.sourceId("/path/First2.qml");
    SourceId qmlDocumentSourceId3 = sourcePathCache.sourceId("/path/Second.qml");
    const QString itemLibraryPath = QDir::cleanPath(
        UNITTEST_DIR "/../../../../share/qtcreator/qmldesigner/itemLibrary/");
    DirectoryPathId itemLibraryPathDirectoryPathId = sourcePathCache.directoryPathId(
        Utils::PathString{itemLibraryPath});
    SourceId itemLibraryPathSourceId = SourceId::create(itemLibraryPathDirectoryPathId, FileNameId{});
    const QString qmlImportsPath = QDir::cleanPath(UNITTEST_DIR "/projectstorage/data/qml");
    DirectoryPathId qmlImportsPathDirectoryPathId = sourcePathCache.directoryPathId(
        Utils::PathString{itemLibraryPath});
    SourceId qmlImportsPathSourceId = SourceId::create(qmlImportsPathDirectoryPathId, FileNameId{});
    ModuleId qmlModuleId{storage.moduleId("Qml", ModuleKind::QmlLibrary)};
    ModuleId qmlCppNativeModuleId{storage.moduleId("Qml", ModuleKind::CppLibrary)};
    ModuleId exampleModuleId{storage.moduleId("Example", ModuleKind::QmlLibrary)};
    ModuleId exampleCppNativeModuleId{storage.moduleId("Example", ModuleKind::CppLibrary)};
    ModuleId builtinModuleId{storage.moduleId("QML", ModuleKind::QmlLibrary)};
    ModuleId builtinCppNativeModuleId{storage.moduleId("QML", ModuleKind::CppLibrary)};
    ModuleId quickModuleId{storage.moduleId("Quick", ModuleKind::QmlLibrary)};
    ModuleId quickCppNativeModuleId{storage.moduleId("Quick", ModuleKind::CppLibrary)};
    ModuleId pathModuleId{storage.moduleId("/path", ModuleKind::PathLibrary)};
    ModuleId subPathQmlModuleId{storage.moduleId("/path/qml", ModuleKind::PathLibrary)};
    Type objectType{"QObject",
                    ImportedType{},
                    ImportedType{},
                    Storage::TypeTraitsKind::Reference,
                    qmltypesPathSourceId,
                    {ExportedType{exampleModuleId, "Object"}, ExportedType{exampleModuleId, "Obj"}}};
    Type itemType{"QItem",
                  ImportedType{},
                  ImportedType{},
                  Storage::TypeTraitsKind::Reference,
                  qmltypes2PathSourceId,
                  {ExportedType{exampleModuleId, "Item"}}};
    QString qmlDocument1{"First{}"};
    QString qmlDocument2{"Second{}"};
    QString qmlDocument3{"Third{}"};
    Type firstType;
    Type secondType;
    Type thirdType;
    Storage::Import import1{qmlModuleId, Storage::Version{2, 3}, qmlDocumentSourceId1};
    Storage::Import import2{qmlModuleId, Storage::Version{}, qmlDocumentSourceId2};
    Storage::Import import3{qmlModuleId, Storage::Version{2}, qmlDocumentSourceId3};
    Storage::Import import4{qmlModuleId, Storage::Version{2, 3}, qmltypesPathSourceId};
    Storage::Import import5{qmlModuleId, Storage::Version{2, 3}, qmltypes2PathSourceId};
    QString qmldirContent{"module Example\ntypeinfo example.qmltypes\n"};
    QString qmltypes1{"Module {\ndependencies: [module1]}"};
    QString qmltypes2{"Module {\ndependencies: [module2]}"};
    QStringList directories = {"/path"};
    QStringList directories2 = {"/path/one", "/path/two"};
    QStringList directories3 = {"/path/one", "/path/two", "/path/three"};
    QmlDesigner::ProjectChunkId directoryProjectChunkId{qtPartId, QmlDesigner::SourceType::Directory};
    QmlDesigner::ProjectChunkId qmldirProjectChunkId{qtPartId, QmlDesigner::SourceType::QmlDir};
    QmlDesigner::ProjectChunkId qmlDocumentProjectChunkId{qtPartId, QmlDesigner::SourceType::Qml};
    QmlDesigner::ProjectChunkId qmltypesProjectChunkId{qtPartId, QmlDesigner::SourceType::QmlTypes};
    QmlDesigner::ProjectChunkId otherDirectoryProjectChunkId{otherProjectPartId,
                                                             QmlDesigner::SourceType::Directory};
    QmlDesigner::ProjectChunkId otherQmldirProjectChunkId{otherProjectPartId,
                                                          QmlDesigner::SourceType::QmlDir};
    QmlDesigner::ProjectChunkId otherQmlDocumentProjectChunkId{otherProjectPartId,
                                                               QmlDesigner::SourceType::Qml};
    QmlDesigner::ProjectChunkId otherQmltypesProjectChunkId{otherProjectPartId,
                                                            QmlDesigner::SourceType::QmlTypes};
    DirectoryPathId path1DirectoryPathId = sourcePathCache.directoryPathId("/path/one");
    SourceId path1SourceId = SourceId::create(path1DirectoryPathId, QmlDesigner::FileNameId{});
    DirectoryPathId path2DirectoryPathId = sourcePathCache.directoryPathId("/path/two");
    SourceId path2SourceId = SourceId::create(path2DirectoryPathId, QmlDesigner::FileNameId{});
    DirectoryPathId path3DirectoryPathId = sourcePathCache.directoryPathId("/path/three");
    SourceId path3SourceId = SourceId::create(path3DirectoryPathId, QmlDesigner::FileNameId{});
    SourceId qmldir1SourceId = sourcePathCache.sourceId("/path/one/qmldir");
    SourceId qmldir2SourceId = sourcePathCache.sourceId("/path/two/qmldir");
    SourceId qmldir3SourceId = sourcePathCache.sourceId("/path/three/qmldir");
    SourceId firstSourceId = sourcePathCache.sourceId("/path/one/First.qml");
    SourceId secondSourceId = sourcePathCache.sourceId("/path/one/Second.qml");
    SourceId thirdSourceId = sourcePathCache.sourceId("/path/two/Third.qml");
    SourceId qmltypes1SourceId = sourcePathCache.sourceId("/path/one/example.qmltypes");
    SourceId qmltypes2SourceId = sourcePathCache.sourceId("/path/two/example2.qmltypes");
};

class ProjectStorageUpdater_get_content_for_qml_dir_paths : public ProjectStorageUpdater
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
    SourceId path3SourceId = createDirectorySourceId("/path/three");
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
        setFilesUnchanged({directoryPathSourceId});
        setFilesChanged({qmlDirPathSourceId});
    }

public:
    QString qmldir{R"(module Example
                      typeinfo example.qmltypes)"};
    QString qmltypes{"Module {\ndependencies: [module1]}"};
    QStringList directories = {"/path"};
    SourceId qmltypesPathSourceId = sourcePathCache.sourceId("/path/example.qmltypes");
    SourceId qmlDirPathSourceId = sourcePathCache.sourceId("/path/qmldir");
    DirectoryPathId directoryPathId = qmlDirPathSourceId.directoryPathId();
    SourceId directoryPathSourceId = SourceId::create(directoryPathId, QmlDesigner::FileNameId{});
};

TEST_F(ProjectStorageUpdater_get_content_for_qml_types, added_qml_types_file_provides_content)
{
    setFilesAdded({qmltypesPathSourceId});

    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/example.qmltypes"))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater_get_content_for_qml_types, changed_qml_types_file_provides_content)
{
    setFilesChanged({qmltypesPathSourceId});

    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/example.qmltypes"))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater_get_content_for_qml_types, removed_qml_types_file_does_not_provide_content)
{
    EXPECT_CALL(fileSystemMock, contentAsQString(_)).Times(AnyNumber());
    setFilesRemoved({qmltypesPathSourceId});

    EXPECT_CALL(fileSystemMock, contentAsQString(Eq("/path/example.qmltypes"_L1))).Times(0);

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater_get_content_for_qml_types, removed_qml_types_file_notifies_about_error)
{
    setFilesRemoved({qmltypesPathSourceId});

    EXPECT_CALL(errorNotifierMock, qmltypesFileMissing(Eq("/path/example.qmltypes"_L1)));

    updater.update({.qtDirectories = directories});
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
    ModuleId exampleCppNativeModuleId{storage.moduleId("Example", ModuleKind::CppLibrary)};
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
                      Field("DirectoryInfo::moduleId", &DirectoryInfo::moduleId, exampleCppNativeModuleId),
                      IsInsideProject::No));
    EXPECT_CALL(qmlTypesParserMock,
                parse(qmltypes2,
                      _,
                      _,
                      Field("DirectoryInfo::moduleId", &DirectoryInfo::moduleId, exampleCppNativeModuleId),
                      IsInsideProject::No));

    updater.update({.qtDirectories = {"/root/path"}});
}

TEST_F(ProjectStorageUpdater_parse_qml_types, add_qmltypes)
{
    setDirectoryInfos(
        directoryPathId,
        {{directoryPathId, qmltypesPathSourceId, exampleCppNativeModuleId, FileType::QmlTypes},
         {directoryPathId, qmltypes2PathSourceId, exampleCppNativeModuleId, FileType::QmlTypes}});
    setFilesUnchanged({directoryPathSourceId, qmlDirPathSourceId});
    setFilesAdded({qmltypesPathSourceId, qmltypes2PathSourceId});

    EXPECT_CALL(qmlTypesParserMock,
                parse(qmltypes,
                      _,
                      _,
                      Field("DirectoryInfo::moduleId", &DirectoryInfo::moduleId, exampleCppNativeModuleId),
                      IsInsideProject::No));
    EXPECT_CALL(qmlTypesParserMock,
                parse(qmltypes2,
                      _,
                      _,
                      Field("DirectoryInfo::moduleId", &DirectoryInfo::moduleId, exampleCppNativeModuleId),
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
                      Field("DirectoryInfo::moduleId", &DirectoryInfo::moduleId, exampleCppNativeModuleId),
                      IsInsideProject::Yes));
    EXPECT_CALL(qmlTypesParserMock,
                parse(qmltypes2,
                      _,
                      _,
                      Field("DirectoryInfo::moduleId", &DirectoryInfo::moduleId, exampleCppNativeModuleId),
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
                      Field("DirectoryInfo::moduleId", &DirectoryInfo::moduleId, exampleCppNativeModuleId),
                      IsInsideProject::No));
    EXPECT_CALL(qmlTypesParserMock,
                parse(qmltypes2,
                      _,
                      _,
                      Field("DirectoryInfo::moduleId", &DirectoryInfo::moduleId, exampleCppNativeModuleId),
                      IsInsideProject::No));

    updater.update({.qtDirectories = {"/root"}});
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
                  Field("SynchronizationPackage::types", &SynchronizationPackage::types, IsEmpty()),
                  Field("SynchronizationPackage::updatedSourceIds",
                        &SynchronizationPackage::updatedSourceIds,
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
    SourceId path2SourceId = createDirectorySourceId("/root/two");
    DirectoryPathId path2DirectoryPathId = path2SourceId.directoryPathId();
    SourceId path3SourceId = createDirectorySourceId("/root/one/three");
    DirectoryPathId path3DirectoryPathId = path3SourceId.directoryPathId();
};

TEST_F(ProjectStorageUpdater_synchronize_subdirectories, added_qt_subdircectories)
{
    setFilesAdded({rootDirectoryPathSourceId, path1SourceId, path2SourceId, path3SourceId});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(
                      IsDirectoryInfo(rootDirectoryPathId, path1SourceId, ModuleId{}, FileType::Directory),
                      IsDirectoryInfo(rootDirectoryPathId, path2SourceId, ModuleId{}, FileType::Directory),
                      IsDirectoryInfo(path1DirectoryPathId, path3SourceId, ModuleId{}, FileType::Directory))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(rootDirectoryPathId,
                                       path1DirectoryPathId,
                                       path2DirectoryPathId,
                                       path3DirectoryPathId)))));

    updater.update({.qtDirectories = {"/root"}});
}

TEST_F(ProjectStorageUpdater_synchronize_subdirectories, changed_qt_subdircectories)
{
    setFilesChanged({rootDirectoryPathSourceId, path1SourceId, path2SourceId, path3SourceId});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(
                      IsDirectoryInfo(rootDirectoryPathId, path1SourceId, ModuleId{}, FileType::Directory),
                      IsDirectoryInfo(rootDirectoryPathId, path2SourceId, ModuleId{}, FileType::Directory),
                      IsDirectoryInfo(path1DirectoryPathId, path3SourceId, ModuleId{}, FileType::Directory))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(rootDirectoryPathId,
                                       path1DirectoryPathId,
                                       path2DirectoryPathId,
                                       path3DirectoryPathId)))));

    updater.update({.qtDirectories = {"/root"}});
}

TEST_F(ProjectStorageUpdater_synchronize_subdirectories, added_project_subdircectories)
{
    setFilesAdded({rootDirectoryPathSourceId, path1SourceId, path2SourceId, path3SourceId});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(
                      IsDirectoryInfo(rootDirectoryPathId, path1SourceId, ModuleId{}, FileType::Directory),
                      IsDirectoryInfo(rootDirectoryPathId, path2SourceId, ModuleId{}, FileType::Directory),
                      IsDirectoryInfo(path1DirectoryPathId, path3SourceId, ModuleId{}, FileType::Directory))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(rootDirectoryPathId,
                                       path1DirectoryPathId,
                                       path2DirectoryPathId,
                                       path3DirectoryPathId)))));

    updater.update({.projectDirectory = "/root"});
}

TEST_F(ProjectStorageUpdater_synchronize_subdirectories, changed_project_subdircectories)
{
    setFilesChanged({rootDirectoryPathSourceId, path1SourceId, path2SourceId, path3SourceId});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(
                      IsDirectoryInfo(rootDirectoryPathId, path1SourceId, ModuleId{}, FileType::Directory),
                      IsDirectoryInfo(rootDirectoryPathId, path2SourceId, ModuleId{}, FileType::Directory),
                      IsDirectoryInfo(path1DirectoryPathId, path3SourceId, ModuleId{}, FileType::Directory))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(rootDirectoryPathId,
                                       path1DirectoryPathId,
                                       path2DirectoryPathId,
                                       path3DirectoryPathId)))));

    updater.update({.projectDirectory = "/root"});
}

TEST_F(ProjectStorageUpdater_synchronize_subdirectories, qt_subdirectories_if_root_is_not_changed)
{
    setFilesChanged({path1SourceId, path2SourceId, path3SourceId});
    setFilesUnchanged({rootDirectoryPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::directoryInfos",
                                        &SynchronizationPackage::directoryInfos,
                                        UnorderedElementsAre(IsDirectoryInfo(path1DirectoryPathId,
                                                                             path3SourceId,
                                                                             ModuleId{},
                                                                             FileType::Directory))),
                                  Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                                        &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                                        UnorderedElementsAre(path1DirectoryPathId,
                                                             path2DirectoryPathId,
                                                             path3DirectoryPathId)))));

    updater.update({.qtDirectories = {"/root"}});
}

TEST_F(ProjectStorageUpdater_synchronize_subdirectories, project_subdirectories_if_root_is_not_changed)
{
    setFilesChanged({path1SourceId, path2SourceId, path3SourceId});
    setFilesUnchanged({rootDirectoryPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::directoryInfos",
                                        &SynchronizationPackage::directoryInfos,
                                        UnorderedElementsAre(IsDirectoryInfo(path1DirectoryPathId,
                                                                             path3SourceId,
                                                                             ModuleId{},
                                                                             FileType::Directory))),
                                  Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                                        &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                                        UnorderedElementsAre(path1DirectoryPathId,
                                                             path2DirectoryPathId,
                                                             path3DirectoryPathId)))));

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
                synchronize(AllOf(Field("SynchronizationPackage::directoryInfos",
                                        &SynchronizationPackage::directoryInfos,
                                        UnorderedElementsAre(IsDirectoryInfo(rootDirectoryPathId,
                                                                             path2SourceId,
                                                                             ModuleId{},
                                                                             FileType::Directory))),
                                  Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                                        &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                                        UnorderedElementsAre(rootDirectoryPathId,
                                                             path1DirectoryPathId,
                                                             path3DirectoryPathId)))));

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
                synchronize(AllOf(Field("SynchronizationPackage::directoryInfos",
                                        &SynchronizationPackage::directoryInfos,
                                        UnorderedElementsAre(IsDirectoryInfo(rootDirectoryPathId,
                                                                             path2SourceId,
                                                                             ModuleId{},
                                                                             FileType::Directory))),
                                  Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                                        &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                                        UnorderedElementsAre(rootDirectoryPathId,
                                                             path1DirectoryPathId,
                                                             path3DirectoryPathId)))));

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
        ON_CALL(qmlTypesParserMock, parse(qmltypes, _, _, _, _))
            .WillByDefault([&](auto, auto &imports, auto &types, auto, auto) {
                types.push_back(objectType);
                imports.push_back(import);
            });
        setFilesNotExistsUnchanged({annotationDirectorySourceId});
    }

public:
    SourceId qmltypesPathSourceId = sourcePathCache.sourceId("/path/example.qmltypes");
    SourceId qmlDirPathSourceId = sourcePathCache.sourceId("/path/qmldir");
    DirectoryPathId directoryPathId = qmlDirPathSourceId.directoryPathId();
    SourceId directoryPathSourceId = SourceId::create(directoryPathId, QmlDesigner::FileNameId{});
    SourceId annotationDirectorySourceId = createDirectorySourceId("/path/designer");
    ModuleId qmlModuleId{storage.moduleId("Qml", ModuleKind::QmlLibrary)};
    ModuleId exampleModuleId{storage.moduleId("Example", ModuleKind::QmlLibrary)};
    ModuleId exampleCppNativeModuleId{storage.moduleId("Example", ModuleKind::CppLibrary)};
    Storage::Import import{qmlModuleId, Storage::Version{2, 3}, qmltypesPathSourceId};
    Type objectType{"QObject",
                    ImportedType{},
                    ImportedType{},
                    Storage::TypeTraitsKind::Reference,
                    qmltypesPathSourceId,
                    {ExportedType{exampleModuleId, "Object"}, ExportedType{exampleModuleId, "Obj"}}};
};

using ChangeQmlTypesParameters = std::tuple<Update, FileState, FileState, FileState>;

class synchronize_changed_qml_types : public synchronize_qml_types,
                                      public testing::WithParamInterface<ChangeQmlTypesParameters>
{
public:
    synchronize_changed_qml_types()
        : state{std::get<1>(GetParam())}
        , directoryState{std::get<2>(GetParam())}
        , qmldirState{std::get<3>(GetParam())}
        , update{std::get<0>(GetParam())}

    {
        if (qmldirState == FileState::Unchanged) {
            setDirectoryInfos(
                directoryPathId,
                {{directoryPathId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes}});
        }

        if (state == FileState::Added)
            directoryState = FileState::Changed;
    }

public:
    FileState state;
    FileState directoryState;
    FileState qmldirState;
    const Update &update;
};

auto updateStatesName = [](const testing::TestParamInfo<ChangeQmlTypesParameters> &info) {
    std::string name = toString(std::get<1>(info.param));

    name += "_qmltypes_file_in_";

    if (std::get<0>(info.param).projectDirectory.isEmpty())
        name += "qt_";
    else
        name += "project_";

    name += toString(std::get<2>(info.param));

    name += "_directory_";

    name += toString(std::get<3>(info.param));

    name += "_qmldir";

    return name;
};

INSTANTIATE_TEST_SUITE_P(ProjectStorageUpdater,
                         synchronize_changed_qml_types,
                         testing::Combine(testing::Values(Update{.qtDirectories = {"/path"}},
                                                          Update{.projectDirectory = "/path"}),
                                          testing::Values(FileState::Added, FileState::Changed),
                                          testing::Values(FileState::Changed, FileState::Unchanged),
                                          testing::Values(FileState::Changed, FileState::Unchanged)),
                         updateStatesName);

TEST_P(synchronize_changed_qml_types, from_qt_directory_update_types)
{
    setFiles(directoryState, {directoryPathSourceId});
    setFiles(qmldirState, {qmlDirPathSourceId});
    setFiles(state, {qmltypesPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::imports",
                                        &SynchronizationPackage::imports,
                                        ElementsAre(import)),
                                  Field("SynchronizationPackage::types",
                                        &SynchronizationPackage::types,
                                        ElementsAre(Eq(objectType))),
                                  Field("SynchronizationPackage::updatedSourceIds",
                                        &SynchronizationPackage::updatedSourceIds,
                                        Contains(qmltypesPathSourceId)))));

    updater.update(update);
}

TEST_P(synchronize_changed_qml_types, from_qt_directory_update_file_status)
{
    setFiles(directoryState, {directoryPathSourceId});
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

TEST_P(synchronize_changed_qml_types, from_qt_directory_update_directory_infos)
{
    bool directoryUnchanged = directoryState == FileState::Unchanged
                              && qmldirState == FileState::Unchanged;
    setFiles(directoryState, {directoryPathSourceId});
    setFiles(qmldirState, {qmlDirPathSourceId});
    setFiles(state, {qmltypesPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::directoryInfos",
                                &SynchronizationPackage::directoryInfos,
                                Conditional(directoryUnchanged,
                                            IsEmpty(),
                                            Contains(IsDirectoryInfo(directoryPathId,
                                                                     qmltypesPathSourceId,
                                                                     exampleCppNativeModuleId,
                                                                     FileType::QmlTypes)))),
                          Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                                &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                                Conditional(directoryUnchanged, IsEmpty(), Contains(directoryPathId))))));

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
        setDirectoryInfos(directoryPathId,
                          {{directoryPathId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes}});
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
                         testing::Combine(testing::Values(Update{.qtDirectories = {"/path"}},
                                                          Update{.projectDirectory = "/path"}),
                                          testing::Values(FileState::Removed, FileState::NotExists)),
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
                synchronize(AllOf(Field("SynchronizationPackage::updatedSourceIds",
                                        &SynchronizationPackage::updatedSourceIds,
                                        Contains(qmltypesPathSourceId)))));

    updater.update(update);
}

TEST_P(synchronize_non_existing_qml_types, updates_directory_info)
{
    setFilesChanged({directoryPathSourceId, qmlDirPathSourceId});
    setFiles(state, {qmltypesPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::directoryInfos",
                                        &SynchronizationPackage::directoryInfos,
                                        Not(Contains(IsDirectoryInfo(directoryPathId,
                                                                     qmltypesPathSourceId,
                                                                     exampleCppNativeModuleId,
                                                                     FileType::QmlTypes)))),
                                  Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                                        &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                                        Contains(directoryPathId)))));

    updater.update(update);
}

TEST_F(ProjectStorageUpdater, synchronize_qml_types_are_empty_if_file_does_not_changed)
{
    QString qmltypes{"Module {\ndependencies: []}"};
    setContent(u"/path/example.qmltypes", qmltypes);
    ON_CALL(qmlTypesParserMock, parse(qmltypes, _, _, _, _))
        .WillByDefault([&](auto, auto &, auto &types, auto, auto) { types.push_back(objectType); });
    setFilesUnchanged({qmltypesPathSourceId, qmltypes2PathSourceId, qmlDirPathSourceId});

    EXPECT_CALL(projectStorageMock, synchronize(PackageIsEmpty()));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, get_content_for_qml_documents)
{
    SourceId oldSecondSourceId3 = sourcePathCache.sourceId("/path/OldSecond.qml");
    setQmlFileNames(u"/path", {"First.qml", "First2.qml", "OldSecond.qml", "Second.qml"});
    setFilesAdded({oldSecondSourceId3});
    setContent(u"/path/OldSecond.qml", qmlDocument3);
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstTypeV2 2.2 First2.qml
                      SecondType 2.1 OldSecond.qml
                      SecondType 2.2 Second.qml)"};
    setExpectedContent(u"/path/qmldir", qmldir);

    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/First.qml"))));
    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/First2.qml"))));
    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/OldSecond.qml"))));
    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/Second.qml"))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, parse_qml_documents)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstTypeV2 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    QString qmlDocument1{"First{}"};
    QString qmlDocument2{"Second{}"};
    QString qmlDocument3{"Third{}"};
    setContent(u"/path/qmldir", qmldir);
    setContent(u"/path/First.qml", qmlDocument1);
    setContent(u"/path/First2.qml", qmlDocument2);
    setContent(u"/path/Second.qml", qmlDocument3);

    EXPECT_CALL(qmlDocumentParserMock, parse(qmlDocument1, _, _, _, IsInsideProject::No));
    EXPECT_CALL(qmlDocumentParserMock, parse(qmlDocument2, _, _, _, IsInsideProject::No));
    EXPECT_CALL(qmlDocumentParserMock, parse(qmlDocument3, _, _, _, IsInsideProject::No));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, parse_qml_documents_inside_project)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml)"};
    QString qmlDocument1{"First{}"};
    setFileNames(u"/path", {"First.qml"}, {"*.qml"});
    setContent(u"/path/qmldir", qmldir);
    setContent(u"/path/First.qml", qmlDocument1);

    EXPECT_CALL(qmlDocumentParserMock, parse(qmlDocument1, _, _, _, IsInsideProject::Yes));

    updater.update({.projectDirectory = "/path"});
}

TEST_F(ProjectStorageUpdater, non_existing_qml_documents_are_ignored)
{
    QString qmldir{R"(module Example
                      NonexitingType 1.0 NonexitingType.qml
                      FirstType 1.0 First.qml
                      FirstTypeV2 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    QString qmlDocument1{"First{}"};
    QString qmlDocument2{"Second{}"};
    QString qmlDocument3{"Third{}"};
    setContent(u"/path/qmldir", qmldir);
    setContent(u"/path/First.qml", qmlDocument1);
    setContent(u"/path/First2.qml", qmlDocument2);
    setContent(u"/path/Second.qml", qmlDocument3);

    EXPECT_CALL(qmlDocumentParserMock, parse(qmlDocument1, _, _, _, _));
    EXPECT_CALL(qmlDocumentParserMock, parse(qmlDocument2, _, _, _, _));
    EXPECT_CALL(qmlDocumentParserMock, parse(qmlDocument3, _, _, _, _));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchronize_qml_documents)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports",
                  &SynchronizationPackage::imports,
                  UnorderedElementsAre(import1, import2, import3)),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        ImportedType{"Object"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId1,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        ImportedType{"Object2"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId2,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))),
                    AllOf(IsStorageType("Second.qml",
                                        ImportedType{"Object3"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId3,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "SecondType", 2, 2),
                                                     IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId3, 1, 21))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(
                          directoryPathId, qmlDocumentSourceId3, ModuleId{}, FileType::QmlDocument))))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchronize_qml_documents_with_missing_module_name)
{
    QString qmldir{R"(FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);
    ModuleId directoryNameModuleId{storage.moduleId("path", ModuleKind::QmlLibrary)};

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field(
                    &SynchronizationPackage::types,
                    Contains(AllOf(IsStorageType("First.qml",
                                                 ImportedType{"Object"},
                                                 TypeTraitsKind::Reference,
                                                 qmlDocumentSourceId1,
                                                 ChangeLevel::Full),
                                   Field("Type::exportedTypes",
                                         &Type::exportedTypes,
                                         UnorderedElementsAre(
                                             IsExportedType(directoryNameModuleId, "FirstType", 1, 0),
                                             IsExportedType(pathModuleId, "First", -1, -1)))))))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchronize_qml_documents_in_project)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports",
                  &SynchronizationPackage::imports,
                  UnorderedElementsAre(import1, import2, import3)),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        ImportedType{"Object"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId1,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        ImportedType{"Object2"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId2,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))),
                    AllOf(IsStorageType("Second.qml",
                                        ImportedType{"Object3"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId3,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "SecondType", 2, 2),
                                                     IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId3, 1, 21))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(
                          directoryPathId, qmlDocumentSourceId3, ModuleId{}, FileType::QmlDocument))))));

    updater.update({.projectDirectory = "/path"});
}

TEST_F(ProjectStorageUpdater, skip_duplicate_qmldir_entries)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 1.0 First.qml
                      FirstType 2.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports",
                  &SynchronizationPackage::imports,
                  UnorderedElementsAre(import1, import2, import3)),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        ImportedType{"Object"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId1,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(exampleModuleId, "FirstType", 2, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        ImportedType{"Object2"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId2,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))),
                    AllOf(IsStorageType("Second.qml",
                                        ImportedType{"Object3"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId3,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "SecondType", 2, 2),
                                                     IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId3, 1, 21))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(
                          directoryPathId, qmlDocumentSourceId3, ModuleId{}, FileType::QmlDocument))))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchronize_add_only_qml_document_in_directory)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({directoryPathSourceId});
    setFilesUnchanged({qmlDirPathSourceId, qmlDocumentSourceId1});
    setFilesAdded({qmlDocumentSourceId2});
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument}});
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::imports",
                                &SynchronizationPackage::imports,
                                UnorderedElementsAre(import2)),
                          Field("SynchronizationPackage::types",
                                &SynchronizationPackage::types,
                                UnorderedElementsAre(
                                    AllOf(IsStorageType("First.qml",
                                                        ImportedType{},
                                                        TypeTraitsKind::None,
                                                        qmlDocumentSourceId1,
                                                        ChangeLevel::Minimal),
                                          Field("Type::exportedTypes",
                                                &Type::exportedTypes,
                                                UnorderedElementsAre(
                                                    IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                    IsExportedType(pathModuleId, "First", -1, -1)))),
                                    AllOf(IsStorageType("First2.qml",
                                                        ImportedType{"Object2"},
                                                        TypeTraitsKind::Reference,
                                                        qmlDocumentSourceId2,
                                                        ChangeLevel::Full),
                                          Field("Type::exportedTypes",
                                                &Type::exportedTypes,
                                                UnorderedElementsAre(
                                                    IsExportedType(pathModuleId, "First2", -1, -1)))))),
                          Field("SynchronizationPackage::updatedSourceIds",
                                &SynchronizationPackage::updatedSourceIds,
                                UnorderedElementsAre(qmlDocumentSourceId1, qmlDocumentSourceId2)),
                          Field("SynchronizationPackage::updatedFileStatusSourceIds",
                                &SynchronizationPackage::updatedFileStatusSourceIds,
                                UnorderedElementsAre(directoryPathSourceId, qmlDocumentSourceId2)),
                          Field("SynchronizationPackage::fileStatuses",
                                &SynchronizationPackage::fileStatuses,
                                UnorderedElementsAre(IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                                     IsFileStatus(directoryPathSourceId, 1, 21))),
                          Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                                &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                                UnorderedElementsAre(directoryPathId)),
                          Field("SynchronizationPackage::directoryInfos",
                                &SynchronizationPackage::directoryInfos,
                                UnorderedElementsAre(IsDirectoryInfo(directoryPathId,
                                                                     qmlDocumentSourceId1,
                                                                     ModuleId{},
                                                                     FileType::QmlDocument),
                                                     IsDirectoryInfo(directoryPathId,
                                                                     qmlDocumentSourceId2,
                                                                     ModuleId{},
                                                                     FileType::QmlDocument))))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchronize_removes_qml_document)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      )"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({qmlDirPathSourceId});
    setFilesUnchanged({qmlDocumentSourceId1, qmlDocumentSourceId2});
    setFilesRemoved({qmlDocumentSourceId3});
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                       {directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument},
                       {directoryPathId, qmlDocumentSourceId3, ModuleId{}, FileType::QmlDocument}});
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports", &SynchronizationPackage::imports, IsEmpty()),
            Field(
                "SynchronizationPackage::types",
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        ImportedType{},
                                        TypeTraitsKind::None,
                                        qmlDocumentSourceId1,
                                        ChangeLevel::Minimal),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        ImportedType{},
                                        TypeTraitsKind::None,
                                        qmlDocumentSourceId2,
                                        ChangeLevel::Minimal),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId3)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(IsDirectoryInfo(directoryPathId,
                                                       qmlDocumentSourceId1,
                                                       ModuleId{},
                                                       FileType::QmlDocument),
                                       IsDirectoryInfo(directoryPathId,
                                                       qmlDocumentSourceId2,
                                                       ModuleId{},
                                                       FileType::QmlDocument))))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchronize_removes_qml_document_in_qmldir_only)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      )"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({qmlDirPathSourceId});
    setFilesUnchanged({qmlDocumentSourceId1, qmlDocumentSourceId2});
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                       {directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument}});
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports", &SynchronizationPackage::imports, IsEmpty()),
            Field("SynchronizationPackage::types",
                  &SynchronizationPackage::types,
                  UnorderedElementsAre(
                      AllOf(IsStorageType("First.qml",
                                          ImportedType{},
                                          TypeTraitsKind::None,
                                          qmlDocumentSourceId1,
                                          ChangeLevel::Minimal),
                            Field("Type::exportedTypes",
                                  &Type::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                       IsExportedType(pathModuleId, "First", -1, -1)))),
                      AllOf(IsStorageType("First2.qml",
                                          ImportedType{},
                                          TypeTraitsKind::None,
                                          qmlDocumentSourceId2,
                                          ChangeLevel::Minimal),
                            Field("Type::exportedTypes",
                                  &Type::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(pathModuleId, "First2", -1, -1)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(IsDirectoryInfo(directoryPathId,
                                                       qmlDocumentSourceId1,
                                                       ModuleId{},
                                                       FileType::QmlDocument),
                                       IsDirectoryInfo(directoryPathId,
                                                       qmlDocumentSourceId2,
                                                       ModuleId{},
                                                       FileType::QmlDocument))))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchronize_add_qml_document_to_qmldir)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      )"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({qmlDirPathSourceId});
    setFilesUnchanged({qmlDocumentSourceId1, qmlDocumentSourceId2});
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                       {directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument}});
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports", &SynchronizationPackage::imports, IsEmpty()),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        ImportedType{},
                                        TypeTraitsKind::None,
                                        qmlDocumentSourceId1,
                                        ChangeLevel::Minimal),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        ImportedType{},
                                        TypeTraitsKind::None,
                                        qmlDocumentSourceId2,
                                        ChangeLevel::Minimal),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(IsDirectoryInfo(directoryPathId,
                                                       qmlDocumentSourceId1,
                                                       ModuleId{},
                                                       FileType::QmlDocument),
                                       IsDirectoryInfo(directoryPathId,
                                                       qmlDocumentSourceId2,
                                                       ModuleId{},
                                                       FileType::QmlDocument))))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchronize_remove_qml_document_from_qmldir)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      )"};
    setContent(u"/path/qmldir", qmldir);
    setFilesUnchanged({qmlDocumentSourceId1, qmlDocumentSourceId2});
    setFilesChanged({qmlDirPathSourceId});
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                       {directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument}});
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports", &SynchronizationPackage::imports, IsEmpty()),
            Field("SynchronizationPackage::types",
                  &SynchronizationPackage::types,
                  UnorderedElementsAre(
                      AllOf(IsStorageType("First.qml",
                                          ImportedType{},
                                          TypeTraitsKind::None,
                                          qmlDocumentSourceId1,
                                          ChangeLevel::Minimal),
                            Field("Type::exportedTypes",
                                  &Type::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                       IsExportedType(pathModuleId, "First", -1, -1)))),
                      AllOf(IsStorageType("First2.qml",
                                          ImportedType{},
                                          TypeTraitsKind::None,
                                          qmlDocumentSourceId2,
                                          ChangeLevel::Minimal),
                            Field("Type::exportedTypes",
                                  &Type::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(pathModuleId, "First2", -1, -1)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(IsDirectoryInfo(directoryPathId,
                                                       qmlDocumentSourceId1,
                                                       ModuleId{},
                                                       FileType::QmlDocument),
                                       IsDirectoryInfo(directoryPathId,
                                                       qmlDocumentSourceId2,
                                                       ModuleId{},
                                                       FileType::QmlDocument))))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchronize_qml_documents_dont_update_if_up_to_date)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setFilesUnchanged({qmlDocumentSourceId3});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports",
                  &SynchronizationPackage::imports,
                  UnorderedElementsAre(import1, import2)),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        ImportedType{"Object"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId1,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        ImportedType{"Object2"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId2,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))),
                    AllOf(IsStorageType("Second.qml",
                                        ImportedType{},
                                        TypeTraitsKind::None,
                                        qmlDocumentSourceId3,
                                        ChangeLevel::Minimal),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "SecondType", 2, 2),
                                                     IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21))),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(
                          directoryPathId, qmlDocumentSourceId3, ModuleId{}, FileType::QmlDocument))))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchroniz_if_qmldir_file_has_not_changed_and_some_updated_files)
{
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
                       {directoryPathId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes},
                       {directoryPathId, qmlDocumentSourceId1, exampleModuleId, FileType::QmlDocument},
                       {directoryPathId, qmlDocumentSourceId2, exampleModuleId, FileType::QmlDocument}});
    setFilesUnchanged({qmlDirPathSourceId, qmltypes2PathSourceId, qmlDocumentSourceId2});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(
                    Field("SynchronizationPackage::imports",
                          &SynchronizationPackage::imports,
                          UnorderedElementsAre(import1, import4)),
                    Field("SynchronizationPackage::types",
                          &SynchronizationPackage::types,
                          UnorderedElementsAre(
                              Eq(objectType),
                              AllOf(IsStorageType("First.qml",
                                                  ImportedType{"Object"},
                                                  TypeTraitsKind::Reference,
                                                  qmlDocumentSourceId1,
                                                  ChangeLevel::ExcludeExportedTypes),
                                    Field("Type::exportedTypes", &Type::exportedTypes, IsEmpty())))),
                    Field("SynchronizationPackage::updatedSourceIds",
                          &SynchronizationPackage::updatedSourceIds,
                          UnorderedElementsAre(qmltypesPathSourceId, qmlDocumentSourceId1)),
                    Field("SynchronizationPackage::fileStatuses",
                          &SynchronizationPackage::fileStatuses,
                          UnorderedElementsAre(IsFileStatus(qmltypesPathSourceId, 1, 21),
                                               IsFileStatus(qmlDocumentSourceId1, 1, 21))),
                    Field("SynchronizationPackage::updatedFileStatusSourceIds",
                          &SynchronizationPackage::updatedFileStatusSourceIds,
                          UnorderedElementsAre(qmltypesPathSourceId, qmlDocumentSourceId1)),
                    Field("SynchronizationPackage::directoryInfos",
                          &SynchronizationPackage::directoryInfos,
                          IsEmpty()))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchroniz_if_qmldir_file_not_changed_and_some_removed_files)
{
    setQmlFileNames(u"/path", {"First2.qml"});
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
                       {directoryPathId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes},
                       {directoryPathId, qmlDocumentSourceId1, exampleModuleId, FileType::QmlDocument},
                       {directoryPathId, qmlDocumentSourceId2, exampleModuleId, FileType::QmlDocument}});
    setFilesUnchanged({qmlDirPathSourceId, qmltypes2PathSourceId, qmlDocumentSourceId2});
    setFilesRemoved({qmltypesPathSourceId, qmlDocumentSourceId1});

    EXPECT_CALL(errorNotifierMock, qmltypesFileMissing(Eq(u"/path/example.qmltypes"_s)));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchroniz_if_qmldir_file_has_changed_and_some_removed_files)
{
    QString qmldir{R"(module Example
                      FirstType 2.2 First2.qml
                      typeinfo example2.qmltypes)"};
    setContent(u"/path/qmldir", qmldir);
    setQmlFileNames(u"/path", {"First2.qml"});
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
                       {directoryPathId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes},
                       {directoryPathId, qmlDocumentSourceId1, exampleModuleId, FileType::QmlDocument},
                       {directoryPathId, qmlDocumentSourceId2, exampleModuleId, FileType::QmlDocument}});
    setFilesUnchanged({qmltypes2PathSourceId, qmlDocumentSourceId2});
    setFilesRemoved({qmltypesPathSourceId, qmlDocumentSourceId1});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports", &SynchronizationPackage::imports, IsEmpty()),
            Field("SynchronizationPackage::types",
                  &SynchronizationPackage::types,
                  UnorderedElementsAre(AllOf(
                      IsStorageType("First2.qml",
                                    ImportedType{},
                                    TypeTraitsKind::None,
                                    qmlDocumentSourceId2,
                                    ChangeLevel::Minimal),
                      Field("Type::exportedTypes",
                            &Type::exportedTypes,
                            UnorderedElementsAre(IsExportedType(pathModuleId, "First2", -1, -1),
                                                 IsExportedType(exampleModuleId, "FirstType", 2, 2)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmltypesPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21))),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmltypesPathSourceId, qmlDocumentSourceId1)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(IsDirectoryInfo(directoryPathId,
                                                       qmlDocumentSourceId2,
                                                       ModuleId{},
                                                       FileType::QmlDocument),
                                       IsDirectoryInfo(directoryPathId,
                                                       qmltypes2PathSourceId,
                                                       exampleCppNativeModuleId,
                                                       FileType::QmlTypes))))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, update_qml_types_files_is_empty)
{
    EXPECT_CALL(
        projectStorageMock,
        synchronize(
            AllOf(Field("SynchronizationPackage::imports", &SynchronizationPackage::imports, IsEmpty()),
                  Field("SynchronizationPackage::types", &SynchronizationPackage::types, IsEmpty()),
                  Field("SynchronizationPackage::updatedSourceIds",
                        &SynchronizationPackage::updatedSourceIds,
                        IsEmpty()),
                  Field("SynchronizationPackage::fileStatuses",
                        &SynchronizationPackage::fileStatuses,
                        IsEmpty()),
                  Field("SynchronizationPackage::updatedFileStatusSourceIds",
                        &SynchronizationPackage::updatedFileStatusSourceIds,
                        IsEmpty()),
                  Field("SynchronizationPackage::directoryInfos",
                        &SynchronizationPackage::directoryInfos,
                        IsEmpty()),
                  Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                        &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                        IsEmpty()))));

    updater.update({});
}

TEST_F(ProjectStorageUpdater, synchronize_qml_documents_with_different_version_but_same_type_name_and_file_name)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 1.1 First.qml
                      FirstType 6.0 First.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setQmlFileNames(u"/path", {"First.qml"});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports",
                  &SynchronizationPackage::imports,
                  UnorderedElementsAre(import1)),
            Field("SynchronizationPackage::types",
                  &SynchronizationPackage::types,
                  UnorderedElementsAre(AllOf(
                      IsStorageType("First.qml",
                                    ImportedType{"Object"},
                                    TypeTraitsKind::Reference,
                                    qmlDocumentSourceId1,
                                    ChangeLevel::Full),
                      Field("Type::exportedTypes",
                            &Type::exportedTypes,
                            UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                 IsExportedType(exampleModuleId, "FirstType", 1, 1),
                                                 IsExportedType(exampleModuleId, "FirstType", 6, 0),
                                                 IsExportedType(pathModuleId, "First", -1, -1)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1)),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(IsDirectoryInfo(
                      directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument))))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchronize_qml_documents_with_different_type_name_but_same_version_and_file_name)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType2 1.0 First.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setQmlFileNames(u"/path", {"First.qml"});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports",
                  &SynchronizationPackage::imports,
                  UnorderedElementsAre(import1)),
            Field("SynchronizationPackage::types",
                  &SynchronizationPackage::types,
                  UnorderedElementsAre(AllOf(
                      IsStorageType("First.qml",
                                    ImportedType{"Object"},
                                    TypeTraitsKind::Reference,
                                    qmlDocumentSourceId1,
                                    ChangeLevel::Full),
                      Field("Type::exportedTypes",
                            &Type::exportedTypes,
                            UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                 IsExportedType(exampleModuleId, "FirstType2", 1, 0),
                                                 IsExportedType(pathModuleId, "First", -1, -1)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1)),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(IsDirectoryInfo(
                      directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument))))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, dont_synchronize_selectors)
{
    setContent(u"/path/+First.qml", qmlDocument1);
    setContent(u"/path/qml/+First.qml", qmlDocument1);
    QString qmldir{R"(module Example
                      FirstType 1.0 +First.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setQmlFileNames(u"/path", {"First.qml"});

    EXPECT_CALL(projectStorageMock,
                synchronize(Not(Field(
                    &SynchronizationPackage::types,
                    Contains(Field("Type::exportedTypes",
                                   &Type::exportedTypes,
                                   Contains(IsExportedType(exampleModuleId, "FirstType", 1, 0))))))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchronize_qmldir_dependencies)
{
    QString qmldir{R"(module Example
                      depends  Qml
                      depends  QML
                      typeinfo example.qmltypes
                      typeinfo example2.qmltypes
                      )"};
    setContent(u"/path/qmldir", qmldir);

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::moduleDependencies",
                  &SynchronizationPackage::moduleDependencies,
                  UnorderedElementsAre(
                      Import{qmlCppNativeModuleId, Storage::Version{}, qmltypesPathSourceId},
                      Import{builtinCppNativeModuleId, Storage::Version{}, qmltypesPathSourceId},
                      Import{qmlCppNativeModuleId, Storage::Version{}, qmltypes2PathSourceId},
                      Import{builtinCppNativeModuleId, Storage::Version{}, qmltypes2PathSourceId})),
            Field("SynchronizationPackage::updatedModuleDependencySourceIds",
                  &SynchronizationPackage::updatedModuleDependencySourceIds,
                  UnorderedElementsAre(qmltypesPathSourceId, qmltypes2PathSourceId)))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchronize_qmldir_dependencies_with_double_entries)
{
    QString qmldir{R"(module Example
                      depends  Qml
                      depends  QML
                      depends  Qml
                      typeinfo example.qmltypes
                      typeinfo example2.qmltypes
                      )"};
    setContent(u"/path/qmldir", qmldir);

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::moduleDependencies",
                  &SynchronizationPackage::moduleDependencies,
                  UnorderedElementsAre(
                      Import{qmlCppNativeModuleId, Storage::Version{}, qmltypesPathSourceId},
                      Import{builtinCppNativeModuleId, Storage::Version{}, qmltypesPathSourceId},
                      Import{qmlCppNativeModuleId, Storage::Version{}, qmltypes2PathSourceId},
                      Import{builtinCppNativeModuleId, Storage::Version{}, qmltypes2PathSourceId})),
            Field("SynchronizationPackage::updatedModuleDependencySourceIds",
                  &SynchronizationPackage::updatedModuleDependencySourceIds,
                  UnorderedElementsAre(qmltypesPathSourceId, qmltypes2PathSourceId)))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchronize_qmldir_dependencies_with_colliding_imports)
{
    QString qmldir{R"(module Example
                      depends  Qml
                      depends  QML
                      import Qml
                      typeinfo example.qmltypes
                      typeinfo example2.qmltypes
                      )"};
    setContent(u"/path/qmldir", qmldir);

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::moduleDependencies",
                  &SynchronizationPackage::moduleDependencies,
                  UnorderedElementsAre(
                      Import{qmlCppNativeModuleId, Storage::Version{}, qmltypesPathSourceId},
                      Import{builtinCppNativeModuleId, Storage::Version{}, qmltypesPathSourceId},
                      Import{qmlCppNativeModuleId, Storage::Version{}, qmltypes2PathSourceId},
                      Import{builtinCppNativeModuleId, Storage::Version{}, qmltypes2PathSourceId})),
            Field("SynchronizationPackage::updatedModuleDependencySourceIds",
                  &SynchronizationPackage::updatedModuleDependencySourceIds,
                  UnorderedElementsAre(qmltypesPathSourceId, qmltypes2PathSourceId)))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchronize_qmldir_with_no_dependencies)
{
    QString qmldir{R"(module Example
                      typeinfo example.qmltypes
                      typeinfo example2.qmltypes
                      )"};
    setContent(u"/path/qmldir", qmldir);

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::moduleDependencies",
                                &SynchronizationPackage::moduleDependencies,
                                IsEmpty()),
                          Field("SynchronizationPackage::updatedModuleDependencySourceIds",
                                &SynchronizationPackage::updatedModuleDependencySourceIds,
                                UnorderedElementsAre(qmltypesPathSourceId, qmltypes2PathSourceId)))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchronize_qmldir_imports)
{
    QString qmldir{R"(module Example
                      import Qml auto
                      import QML 2.1
                      import Quick
                      )"};
    setContent(u"/path/qmldir", qmldir);

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
                                ElementsAre(exampleModuleId)))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchronize_qmldir_with_no_imports)
{
    QString qmldir{R"(module Example
                      )"};
    setContent(u"/path/qmldir", qmldir);

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field("SynchronizationPackage::moduleExportedImports",
                                        &SynchronizationPackage::moduleExportedImports,
                                        IsEmpty()),
                                  Field("SynchronizationPackage::updatedModuleIds",
                                        &SynchronizationPackage::updatedModuleIds,
                                        ElementsAre(exampleModuleId)))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchronize_qmldir_imports_with_double_entries)
{
    QString qmldir{R"(module Example
                      import Qml auto
                      import QML 2.1
                      import Quick
                      import Qml
                      )"};
    setContent(u"/path/qmldir", qmldir);

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
                                ElementsAre(exampleModuleId)))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchronize_qmldir_default_imports)
{
    QString qmldir{R"(module Example
                      import Qml auto
                      import QML 2.1
                      default import Quick
                      )"};
    setContent(u"/path/qmldir", qmldir);

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
                                ElementsAre(exampleModuleId)))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, do_not_synchronize_qmldir_optional_imports)
{
    QString qmldir{R"(module Example
                      import Qml auto
                      import QML 2.1
                      optional import Quick
                      )"};
    setContent(u"/path/qmldir", qmldir);

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
                                ElementsAre(exampleModuleId)))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_directories)
{
    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{qtPartId,
                                               QmlDesigner::SourceType::Directory,
                                               {path1SourceId, path2SourceId, path3SourceId}})));

    updater.update({.qtDirectories = directories3});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_directory_does_not_exists)
{
    setFilesNotExists({path2SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{qtPartId,
                                               QmlDesigner::SourceType::Directory,
                                               {path1SourceId, path3SourceId}})));

    updater.update({.qtDirectories = directories3});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_directory_does_not_changed)
{
    setFilesUnchanged({qmldir1SourceId, qmldir2SourceId, path1SourceId, path2SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{qtPartId,
                                               QmlDesigner::SourceType::Directory,
                                               {path1SourceId, path2SourceId}})));

    updater.update({.qtDirectories = directories2});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_directory_removed)
{
    setFilesRemoved({qmldir1SourceId, path1SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(
                    IdPaths{qtPartId, QmlDesigner::SourceType::Directory, {path2SourceId}})));

    updater.update({.qtDirectories = directories2});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_qmldirs)
{
    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{qtPartId,
                                               QmlDesigner::SourceType::QmlDir,
                                               {qmldir1SourceId, qmldir2SourceId, qmldir3SourceId}})));

    updater.update({.qtDirectories = directories3});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_qmldir_does_not_exists)
{
    setFilesNotExists({qmldir2SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{qtPartId,
                                               QmlDesigner::SourceType::QmlDir,
                                               {qmldir1SourceId, qmldir3SourceId}})));

    updater.update({.qtDirectories = directories3});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_qmldir_does_not_changed)
{
    setFilesUnchanged({qmldir1SourceId, qmldir2SourceId, path1SourceId, path2SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{qtPartId,
                                               QmlDesigner::SourceType::QmlDir,
                                               {qmldir1SourceId, qmldir2SourceId}})));

    updater.update({.qtDirectories = directories2});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_qmldir_removed)
{
    setFilesRemoved({qmldir1SourceId, path1SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(
                    IdPaths{qtPartId, QmlDesigner::SourceType::QmlDir, {qmldir2SourceId}})));

    updater.update({.qtDirectories = directories2});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_qml_files)
{
    QString qmldir1{R"(module Example
                      FirstType 1.0 First.qml
                      Second 1.0 Second.qml)"};
    setQmlFileNames(u"/path/one", {"First.qml", "Second.qml"});
    setQmlFileNames(u"/path/two", {"Third.qml"});
    setContent(u"/path/one/qmldir", qmldir1);

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{qtPartId,
                                               QmlDesigner::SourceType::Qml,
                                               {firstSourceId, secondSourceId, thirdSourceId}})));

    updater.update({.qtDirectories = directories2});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_only_qml_files_dont_changed)
{
    QString qmldir1{R"(module Example
                      FirstType 1.0 First.qml
                      Second 1.0 Second.qml)"};
    setQmlFileNames(u"/path/one", {"First.qml", "Second.qml"});
    setQmlFileNames(u"/path/two", {"Third.qml"});
    setContent(u"/path/one/qmldir", qmldir1);
    setFilesUnchanged({firstSourceId, secondSourceId, thirdSourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{qtPartId,
                                               QmlDesigner::SourceType::Qml,
                                               {firstSourceId, secondSourceId, thirdSourceId}})));

    updater.update({.qtDirectories = directories2});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_only_qml_files_changed)
{
    setFilesUnchanged({qmldir1SourceId, qmldir2SourceId, path1SourceId, path2SourceId});
    setFilesChanged({firstSourceId, secondSourceId, thirdSourceId});
    setDirectoryInfos(path1DirectoryPathId,
                      {{path1DirectoryPathId, firstSourceId, exampleModuleId, FileType::QmlDocument},
                       {path1DirectoryPathId, secondSourceId, exampleModuleId, FileType::QmlDocument}});
    setDirectoryInfos(path2DirectoryPathId,
                      {{path2DirectoryPathId, thirdSourceId, ModuleId{}, FileType::QmlDocument}});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{qtPartId,
                                               QmlDesigner::SourceType::Qml,
                                               {firstSourceId, secondSourceId, thirdSourceId}})));

    updater.update({.qtDirectories = directories2});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_qml_files_and_directories_dont_changed)
{
    setFilesUnchanged({qmldir1SourceId,
                       qmldir2SourceId,
                       path1SourceId,
                       path2SourceId,
                       firstSourceId,
                       secondSourceId,
                       thirdSourceId});
    setDirectoryInfos(path1DirectoryPathId,
                      {{path1DirectoryPathId, firstSourceId, exampleModuleId, FileType::QmlDocument},
                       {path1DirectoryPathId, secondSourceId, exampleModuleId, FileType::QmlDocument}});
    setDirectoryInfos(path2DirectoryPathId,
                      {{path2DirectoryPathId, thirdSourceId, ModuleId{}, FileType::QmlDocument}});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{qtPartId,
                                               QmlDesigner::SourceType::Qml,
                                               {firstSourceId, secondSourceId, thirdSourceId}})));

    updater.update({.qtDirectories = directories2});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_qmltypes_files_in_qmldir)
{
    QString qmldir1{R"(module Example
                      typeinfo example.qmltypes)"};
    QString qmldir2{R"(module Example2
                      typeinfo example2.qmltypes)"};
    setContent(u"/path/one/qmldir", qmldir1);
    setContent(u"/path/two/qmldir", qmldir2);

    setFilesUnchanged({firstSourceId, secondSourceId, thirdSourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{qtPartId,
                                               QmlDesigner::SourceType::QmlTypes,
                                               {qmltypes1SourceId, qmltypes2SourceId}})));

    updater.update({.qtDirectories = directories2});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_only_qmltypes_files_in_qmldir_dont_changed)
{
    QString qmldir1{R"(module Example
                      typeinfo example.qmltypes)"};
    QString qmldir2{R"(module Example2
                      typeinfo example2.qmltypes)"};
    setContent(u"/path/one/qmldir", qmldir1);
    setContent(u"/path/two/qmldir", qmldir2);
    setFilesUnchanged({qmltypes1SourceId, qmltypes2SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{qtPartId,
                                               QmlDesigner::SourceType::QmlTypes,
                                               {qmltypes1SourceId, qmltypes2SourceId}})));

    updater.update({.qtDirectories = directories2});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_only_qmltypes_files_changed)
{
    setFilesUnchanged({qmldir1SourceId, qmldir2SourceId, path1SourceId, path2SourceId});
    setFilesChanged({qmltypes1SourceId, qmltypes2SourceId});
    setDirectoryInfos(path1DirectoryPathId,
                      {{path1DirectoryPathId, qmltypes1SourceId, exampleModuleId, FileType::QmlTypes}});
    setDirectoryInfos(path2DirectoryPathId,
                      {{path2DirectoryPathId, qmltypes2SourceId, exampleModuleId, FileType::QmlTypes}});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{qtPartId,
                                               QmlDesigner::SourceType::QmlTypes,
                                               {qmltypes1SourceId, qmltypes2SourceId}})));

    updater.update({.qtDirectories = directories2});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_qmltypes_files_and_directories_dont_changed)
{
    setFilesUnchanged({qmldir1SourceId,
                       qmldir2SourceId,
                       path1SourceId,
                       path2SourceId,
                       qmltypes1SourceId,
                       qmltypes2SourceId});
    setDirectoryInfos(path1DirectoryPathId,
                      {{path1DirectoryPathId, qmltypes1SourceId, exampleModuleId, FileType::QmlTypes}});
    setDirectoryInfos(path2DirectoryPathId,
                      {{path2DirectoryPathId, qmltypes2SourceId, exampleModuleId, FileType::QmlTypes}});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{qtPartId,
                                               QmlDesigner::SourceType::QmlTypes,
                                               {qmltypes1SourceId, qmltypes2SourceId}})));

    updater.update({.qtDirectories = directories2});
}

TEST_F(ProjectStorageUpdater, synchronize_qml_documents_without_qmldir)
{
    setFilesNotExists({qmlDirPathSourceId});
    setFilesChanged({directoryPathSourceId});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports",
                  &SynchronizationPackage::imports,
                  UnorderedElementsAre(import1, import2, import3)),
            Field("SynchronizationPackage::types",
                  &SynchronizationPackage::types,
                  UnorderedElementsAre(
                      AllOf(IsStorageType("First.qml",
                                          ImportedType{"Object"},
                                          TypeTraitsKind::Reference,
                                          qmlDocumentSourceId1,
                                          ChangeLevel::Full),
                            Field("Type::exportedTypes",
                                  &Type::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(pathModuleId, "First", -1, -1)))),
                      AllOf(IsStorageType("First2.qml",
                                          ImportedType{"Object2"},
                                          TypeTraitsKind::Reference,
                                          qmlDocumentSourceId2,
                                          ChangeLevel::Full),
                            Field("Type::exportedTypes",
                                  &Type::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(pathModuleId, "First2", -1, -1)))),
                      AllOf(IsStorageType("Second.qml",
                                          ImportedType{"Object3"},
                                          TypeTraitsKind::Reference,
                                          qmlDocumentSourceId3,
                                          ChangeLevel::Full),
                            Field("Type::exportedTypes",
                                  &Type::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDocumentSourceId1, qmlDocumentSourceId2, qmlDocumentSourceId3)),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(directoryPathSourceId,
                                       qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(directoryPathSourceId, 1, 21),
                                       IsFileStatus(qmlDirPathSourceId, -1, -1),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId3, 1, 21))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(
                          directoryPathId, qmlDocumentSourceId3, ModuleId{}, FileType::QmlDocument))))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, warn_about_non_existing_qml_document)
{
    setFilesNotExists({qmlDocumentSourceId1});
    setFilesAdded({directoryPathSourceId});
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml)"};
    setQmlFileNames(u"/path", {"First2.qml"});
    setContent(u"/path/qmldir", qmldir);

    EXPECT_CALL(errorNotifierMock,
                qmlDocumentDoesNotExistsForQmldirEntry(Eq("FirstType"),
                                                       IsVersion(1, 0),
                                                       qmlDocumentSourceId1,
                                                       qmlDirPathSourceId));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater,
       synchronize_qml_documents_without_parsed_type_if_qml_document_does_not_exists)
{
    setFilesNotExists({qmlDocumentSourceId1});
    setFilesAdded({directoryPathSourceId, qmlDirPathSourceId, qmlDocumentSourceId2});
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml)"};
    setQmlFileNames(u"/path", {"First2.qml"});
    setContent(u"/path/qmldir", qmldir);

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports",
                  &SynchronizationPackage::imports,
                  UnorderedElementsAre(import2)),
            Field("SynchronizationPackage::types",
                  &SynchronizationPackage::types,
                  UnorderedElementsAre(
                      AllOf(IsStorageType("First.qml",
                                          ImportedType{},
                                          TypeTraitsKind::None,
                                          qmlDocumentSourceId1,
                                          ChangeLevel::Full),
                            Field("Type::exportedTypes",
                                  &Type::exportedTypes,
                                  UnorderedElementsAre(
                                      IsExportedType(exampleModuleId, "FirstType", 1, 0)))),
                      AllOf(IsStorageType("First2.qml",
                                          ImportedType{"Object2"},
                                          TypeTraitsKind::Reference,
                                          qmlDocumentSourceId2,
                                          ChangeLevel::Full),
                            Field("Type::exportedTypes",
                                  &Type::exportedTypes,
                                  UnorderedElementsAre(
                                      IsExportedType(pathModuleId, "First2", -1, -1),
                                      IsExportedType(exampleModuleId, "FirstType", 2, 2)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(directoryPathSourceId,
                                       qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(directoryPathSourceId, 1, 21),
                                       IsFileStatus(qmlDirPathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, -1, -1),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(IsDirectoryInfo(directoryPathId,
                                                       qmlDocumentSourceId1,
                                                       ModuleId{},
                                                       FileType::QmlDocument),
                                       IsDirectoryInfo(directoryPathId,
                                                       qmlDocumentSourceId2,
                                                       ModuleId{},
                                                       FileType::QmlDocument))))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchronize_qml_documents_without_qmldir_if_directory_is_removed)
{
    setFilesRemoved({qmlDirPathSourceId,
                     directoryPathSourceId,
                     annotationDirectorySourceId,
                     qmlDocumentSourceId1,
                     qmlDocumentSourceId2,
                     qmlDocumentSourceId3});
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                       {directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument},
                       {directoryPathId, qmlDocumentSourceId3, ModuleId{}, FileType::QmlDocument}});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(
            AllOf(Field("SynchronizationPackage::imports", &SynchronizationPackage::imports, IsEmpty()),
                  Field("SynchronizationPackage::types", &SynchronizationPackage::types, IsEmpty()),
                  Field("SynchronizationPackage::updatedSourceIds",
                        &SynchronizationPackage::updatedSourceIds,
                        UnorderedElementsAre(qmlDirPathSourceId,
                                             qmlDocumentSourceId1,
                                             qmlDocumentSourceId2,
                                             qmlDocumentSourceId3)),
                  Field("SynchronizationPackage::updatedFileStatusSourceIds",
                        &SynchronizationPackage::updatedFileStatusSourceIds,
                        UnorderedElementsAre(directoryPathSourceId,
                                             annotationDirectorySourceId,
                                             qmlDirPathSourceId,
                                             qmlDocumentSourceId1,
                                             qmlDocumentSourceId2,
                                             qmlDocumentSourceId3)),
                  Field("SynchronizationPackage::fileStatuses",
                        &SynchronizationPackage::fileStatuses,
                        IsEmpty()),
                  Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                        &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                        UnorderedElementsAre(directoryPathId)),
                  Field("SynchronizationPackage::directoryInfos",
                        &SynchronizationPackage::directoryInfos,
                        IsEmpty()))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchronize_qml_documents_without_qmldir_add_qml_document)
{
    setFilesNotExistsUnchanged({qmlDirPathSourceId});
    setFilesChanged({directoryPathSourceId});
    setFilesAdded({qmlDocumentSourceId3});
    setFilesUnchanged({qmlDocumentSourceId1, qmlDocumentSourceId2});
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                       {directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument}});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::imports",
                                &SynchronizationPackage::imports,
                                UnorderedElementsAre(import3)),
                          Field("SynchronizationPackage::types",
                                &SynchronizationPackage::types,
                                UnorderedElementsAre(
                                    AllOf(IsStorageType("Second.qml",
                                                        ImportedType{"Object3"},
                                                        TypeTraitsKind::Reference,
                                                        qmlDocumentSourceId3,
                                                        ChangeLevel::Full),
                                          Field("Type::exportedTypes",
                                                &Type::exportedTypes,
                                                UnorderedElementsAre(
                                                    IsExportedType(pathModuleId, "Second", -1, -1)))))),
                          Field("SynchronizationPackage::updatedSourceIds",
                                &SynchronizationPackage::updatedSourceIds,
                                UnorderedElementsAre(qmlDocumentSourceId3)),
                          Field("SynchronizationPackage::updatedFileStatusSourceIds",
                                &SynchronizationPackage::updatedFileStatusSourceIds,
                                UnorderedElementsAre(directoryPathSourceId, qmlDocumentSourceId3)),
                          Field("SynchronizationPackage::fileStatuses",
                                &SynchronizationPackage::fileStatuses,
                                UnorderedElementsAre(IsFileStatus(directoryPathSourceId, 1, 21),
                                                     IsFileStatus(qmlDocumentSourceId3, 1, 21))),
                          Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                                &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                                UnorderedElementsAre(directoryPathId)),
                          Field("SynchronizationPackage::directoryInfos",
                                &SynchronizationPackage::directoryInfos,
                                UnorderedElementsAre(IsDirectoryInfo(directoryPathId,
                                                                     qmlDocumentSourceId1,
                                                                     ModuleId{},
                                                                     FileType::QmlDocument),
                                                     IsDirectoryInfo(directoryPathId,
                                                                     qmlDocumentSourceId2,
                                                                     ModuleId{},
                                                                     FileType::QmlDocument),
                                                     IsDirectoryInfo(directoryPathId,
                                                                     qmlDocumentSourceId3,
                                                                     ModuleId{},
                                                                     FileType::QmlDocument))))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, synchronize_qml_documents_without_qmldir_removes_qml_document)
{
    setFilesNotExists({qmlDirPathSourceId});
    setFilesChanged({directoryPathSourceId});
    setFilesRemoved({qmlDocumentSourceId3});
    setFilesUnchanged({qmlDocumentSourceId1, qmlDocumentSourceId2});
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                       {directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument},
                       {directoryPathId, qmlDocumentSourceId3, ModuleId{}, FileType::QmlDocument}});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports", &SynchronizationPackage::imports, IsEmpty()),
            Field("SynchronizationPackage::types", &SynchronizationPackage::types, IsEmpty()),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDocumentSourceId3)),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(directoryPathSourceId, qmlDirPathSourceId, qmlDocumentSourceId3)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(directoryPathSourceId, 1, 21),
                                       IsFileStatus(qmlDirPathSourceId, -1, -1))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(IsDirectoryInfo(directoryPathId,
                                                       qmlDocumentSourceId1,
                                                       ModuleId{},
                                                       FileType::QmlDocument),
                                       IsDirectoryInfo(directoryPathId,
                                                       qmlDocumentSourceId2,
                                                       ModuleId{},
                                                       FileType::QmlDocument))))));

    updater.update({.qtDirectories = directories});
}

TEST_F(ProjectStorageUpdater, watcher_updates_directories)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({directoryPathSourceId});
    setFilesUnchanged({qmlDirPathSourceId});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports",
                  &SynchronizationPackage::imports,
                  UnorderedElementsAre(import1, import2, import3)),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        ImportedType{"Object"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId1,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        ImportedType{"Object2"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId2,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))),
                    AllOf(IsStorageType("Second.qml",
                                        ImportedType{"Object3"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId3,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "SecondType", 2, 2),
                                                     IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDocumentSourceId1, qmlDocumentSourceId2, qmlDocumentSourceId3)),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(directoryPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(directoryPathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId3, 1, 21))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(
                          directoryPathId, qmlDocumentSourceId3, ModuleId{}, FileType::QmlDocument))))));

    updater.pathsWithIdsChanged({{directoryProjectChunkId, {directoryPathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_subdirectories)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);
    DirectoryPathId rootPathId = sourcePathCache.directoryPathId("/root");
    SourceId rootPathSourceId = SourceId::create(rootPathId, QmlDesigner::FileNameId{});
    SourceId rootQmldirPathSourceId = sourcePathCache.sourceId("/root/qmldir");
    setFilesChanged({directoryPathSourceId, rootPathSourceId});
    setFilesUnchanged({qmlDirPathSourceId, rootQmldirPathSourceId});
    setFileSystemSubdirectories(u"/root", {"/path"});
    setStorageSubdirectories(rootPathId, {directoryPathId});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports",
                  &SynchronizationPackage::imports,
                  UnorderedElementsAre(import1, import2, import3)),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        ImportedType{"Object"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId1,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        ImportedType{"Object2"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId2,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))),
                    AllOf(IsStorageType("Second.qml",
                                        ImportedType{"Object3"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId3,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "SecondType", 2, 2),
                                                     IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDocumentSourceId1, qmlDocumentSourceId2, qmlDocumentSourceId3)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId3, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(rootPathId, directoryPathSourceId, ModuleId{}, FileType::Directory))))));

    updater.pathsWithIdsChanged({{directoryProjectChunkId, {rootPathSourceId, directoryPathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_removed_directory)
{
    auto annotationDirectorySourceId = createDirectorySourceId("/path/designer");
    setFilesRemoved({directoryPathSourceId,
                     annotationDirectorySourceId,
                     qmlDirPathSourceId,
                     qmlDocumentSourceId1,
                     qmlDocumentSourceId2,
                     qmlDocumentSourceId3});
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                       {directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument},
                       {directoryPathId, qmlDocumentSourceId3, ModuleId{}, FileType::QmlDocument}});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(
            AllOf(Field("SynchronizationPackage::imports", &SynchronizationPackage::imports, IsEmpty()),
                  Field("SynchronizationPackage::types", &SynchronizationPackage::types, IsEmpty()),
                  Field("SynchronizationPackage::updatedSourceIds",
                        &SynchronizationPackage::updatedSourceIds,
                        UnorderedElementsAre(qmlDirPathSourceId,
                                             qmlDocumentSourceId1,
                                             qmlDocumentSourceId2,
                                             qmlDocumentSourceId3)),
                  Field("SynchronizationPackage::updatedFileStatusSourceIds",
                        &SynchronizationPackage::updatedFileStatusSourceIds,
                        UnorderedElementsAre(directoryPathSourceId,
                                             annotationDirectorySourceId,
                                             qmlDirPathSourceId,
                                             qmlDocumentSourceId1,
                                             qmlDocumentSourceId2,
                                             qmlDocumentSourceId3)),
                  Field("SynchronizationPackage::fileStatuses",
                        &SynchronizationPackage::fileStatuses,
                        IsEmpty()),
                  Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                        &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                        UnorderedElementsAre(directoryPathId)),
                  Field("SynchronizationPackage::directoryInfos",
                        &SynchronizationPackage::directoryInfos,
                        IsEmpty()))));

    updater.pathsWithIdsChanged({{directoryProjectChunkId, {directoryPathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_watches_directories_after_directory_changes)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({directoryPathSourceId});
    setFilesUnchanged({qmlDirPathSourceId});
    auto directoryDirectoryPathId = directoryPathSourceId.directoryPathId();

    EXPECT_CALL(patchWatcherMock,
                updateContextIdPaths(
                    UnorderedElementsAre(
                        IdPaths{qtPartId, QmlDesigner::SourceType::Directory, {directoryPathSourceId}},
                        IdPaths{qtPartId, QmlDesigner::SourceType::QmlDir, {qmlDirPathSourceId}},
                        IdPaths{qtPartId,
                                QmlDesigner::SourceType::Qml,
                                {qmlDocumentSourceId1, qmlDocumentSourceId2, qmlDocumentSourceId3}}),
                    UnorderedElementsAre(directoryDirectoryPathId)));

    updater.pathsWithIdsChanged({{directoryProjectChunkId, {directoryPathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_dont_updates_directories_for_other_project)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);

    EXPECT_CALL(projectStorageMock, synchronize(PackageIsEmpty()));

    updater.pathsWithIdsChanged({{otherDirectoryProjectChunkId, {directoryPathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_directories_and_qmldir)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({directoryPathSourceId, qmlDirPathSourceId});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports",
                  &SynchronizationPackage::imports,
                  UnorderedElementsAre(import1, import2, import3)),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        ImportedType{"Object"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId1,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        ImportedType{"Object2"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId2,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))),
                    AllOf(IsStorageType("Second.qml",
                                        ImportedType{"Object3"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId3,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "SecondType", 2, 2),
                                                     IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(directoryPathSourceId,
                                       qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(directoryPathSourceId, 1, 21),
                                       IsFileStatus(qmlDirPathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId3, 1, 21))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(
                          directoryPathId, qmlDocumentSourceId3, ModuleId{}, FileType::QmlDocument))))));

    updater.pathsWithIdsChanged({{directoryProjectChunkId, {directoryPathSourceId}},
                                 {qmldirProjectChunkId, {qmlDirPathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_watches_directories_after_qmldir_changes)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);
    auto directoryDirectoryPathId = qmlDirPathSourceId.directoryPathId();

    EXPECT_CALL(patchWatcherMock,
                updateContextIdPaths(
                    UnorderedElementsAre(
                        IdPaths{qtPartId, QmlDesigner::SourceType::Directory, {directoryPathSourceId}},
                        IdPaths{qtPartId, QmlDesigner::SourceType::QmlDir, {qmlDirPathSourceId}},
                        IdPaths{qtPartId,
                                QmlDesigner::SourceType::Qml,
                                {qmlDocumentSourceId1, qmlDocumentSourceId2, qmlDocumentSourceId3}}),
                    UnorderedElementsAre(directoryDirectoryPathId)));

    updater.pathsWithIdsChanged({{qmldirProjectChunkId, {qmlDirPathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_dont_updates_qmldir_for_other_project)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);

    EXPECT_CALL(projectStorageMock, synchronize(PackageIsEmpty()));

    updater.pathsWithIdsChanged({{otherQmldirProjectChunkId, {qmlDirPathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_add_only_qml_document_in_directory)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({directoryPathSourceId});
    setFilesUnchanged({qmlDirPathSourceId, qmlDocumentSourceId1});
    setFilesAdded({qmlDocumentSourceId2});
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument}});
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::imports",
                                &SynchronizationPackage::imports,
                                UnorderedElementsAre(import2)),
                          Field("SynchronizationPackage::types",
                                &SynchronizationPackage::types,
                                UnorderedElementsAre(
                                    AllOf(IsStorageType("First.qml",
                                                        ImportedType{},
                                                        TypeTraitsKind::None,
                                                        qmlDocumentSourceId1,
                                                        ChangeLevel::Minimal),
                                          Field("Type::exportedTypes",
                                                &Type::exportedTypes,
                                                UnorderedElementsAre(
                                                    IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                    IsExportedType(pathModuleId, "First", -1, -1)))),
                                    AllOf(IsStorageType("First2.qml",
                                                        ImportedType{"Object2"},
                                                        TypeTraitsKind::Reference,
                                                        qmlDocumentSourceId2,
                                                        ChangeLevel::Full),
                                          Field("Type::exportedTypes",
                                                &Type::exportedTypes,
                                                UnorderedElementsAre(
                                                    IsExportedType(pathModuleId, "First2", -1, -1)))))),
                          Field("SynchronizationPackage::updatedSourceIds",
                                &SynchronizationPackage::updatedSourceIds,
                                UnorderedElementsAre(qmlDocumentSourceId1, qmlDocumentSourceId2)),
                          Field("SynchronizationPackage::updatedFileStatusSourceIds",
                                &SynchronizationPackage::updatedFileStatusSourceIds,
                                UnorderedElementsAre(directoryPathSourceId, qmlDocumentSourceId2)),
                          Field("SynchronizationPackage::fileStatuses",
                                &SynchronizationPackage::fileStatuses,
                                UnorderedElementsAre(IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                                     IsFileStatus(directoryPathSourceId, 1, 21))),
                          Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                                &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                                UnorderedElementsAre(directoryPathId)),
                          Field("SynchronizationPackage::directoryInfos",
                                &SynchronizationPackage::directoryInfos,
                                UnorderedElementsAre(IsDirectoryInfo(directoryPathId,
                                                                     qmlDocumentSourceId1,
                                                                     ModuleId{},
                                                                     FileType::QmlDocument),
                                                     IsDirectoryInfo(directoryPathId,
                                                                     qmlDocumentSourceId2,
                                                                     ModuleId{},
                                                                     FileType::QmlDocument))))));

    updater.pathsWithIdsChanged({{directoryProjectChunkId, {directoryPathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_removes_qml_document)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      )"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({qmlDirPathSourceId});
    setFilesUnchanged({qmlDocumentSourceId1, qmlDocumentSourceId2});
    setFilesRemoved({qmlDocumentSourceId3});
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                       {directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument},
                       {directoryPathId, qmlDocumentSourceId3, ModuleId{}, FileType::QmlDocument}});
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports", &SynchronizationPackage::imports, IsEmpty()),
            Field(
                "SynchronizationPackage::types",
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        ImportedType{},
                                        TypeTraitsKind::None,
                                        qmlDocumentSourceId1,
                                        ChangeLevel::Minimal),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        ImportedType{},
                                        TypeTraitsKind::None,
                                        qmlDocumentSourceId2,
                                        ChangeLevel::Minimal),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId3)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(IsDirectoryInfo(directoryPathId,
                                                       qmlDocumentSourceId1,
                                                       ModuleId{},
                                                       FileType::QmlDocument),
                                       IsDirectoryInfo(directoryPathId,
                                                       qmlDocumentSourceId2,
                                                       ModuleId{},
                                                       FileType::QmlDocument))))));

    updater.pathsWithIdsChanged({{directoryProjectChunkId, {directoryPathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_removes_qml_document_in_qmldir_only)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      )"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({qmlDirPathSourceId});
    setFilesUnchanged({qmlDocumentSourceId1, qmlDocumentSourceId2});
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                       {directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument}});
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports", &SynchronizationPackage::imports, IsEmpty()),
            Field("SynchronizationPackage::types",
                  &SynchronizationPackage::types,
                  UnorderedElementsAre(
                      AllOf(IsStorageType("First.qml",
                                          ImportedType{},
                                          TypeTraitsKind::None,
                                          qmlDocumentSourceId1,
                                          ChangeLevel::Minimal),
                            Field("Type::exportedTypes",
                                  &Type::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                       IsExportedType(pathModuleId, "First", -1, -1)))),
                      AllOf(IsStorageType("First2.qml",
                                          ImportedType{},
                                          TypeTraitsKind::None,
                                          qmlDocumentSourceId2,
                                          ChangeLevel::Minimal),
                            Field("Type::exportedTypes",
                                  &Type::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(pathModuleId, "First2", -1, -1)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(IsDirectoryInfo(directoryPathId,
                                                       qmlDocumentSourceId1,
                                                       ModuleId{},
                                                       FileType::QmlDocument),
                                       IsDirectoryInfo(directoryPathId,
                                                       qmlDocumentSourceId2,
                                                       ModuleId{},
                                                       FileType::QmlDocument))))));

    updater.pathsWithIdsChanged({{qmldirProjectChunkId, {qmlDirPathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_directories_add_qml_document_to_qmldir)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      )"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({qmlDirPathSourceId});
    setFilesUnchanged({qmlDocumentSourceId1, qmlDocumentSourceId2});
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                       {directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument}});
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports", &SynchronizationPackage::imports, IsEmpty()),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        ImportedType{},
                                        TypeTraitsKind::None,
                                        qmlDocumentSourceId1,
                                        ChangeLevel::Minimal),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        ImportedType{},
                                        TypeTraitsKind::None,
                                        qmlDocumentSourceId2,
                                        ChangeLevel::Minimal),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(IsDirectoryInfo(directoryPathId,
                                                       qmlDocumentSourceId1,
                                                       ModuleId{},
                                                       FileType::QmlDocument),
                                       IsDirectoryInfo(directoryPathId,
                                                       qmlDocumentSourceId2,
                                                       ModuleId{},
                                                       FileType::QmlDocument))))));

    updater.pathsWithIdsChanged({{qmldirProjectChunkId, {qmlDirPathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_directories_remove_qml_document_from_qmldir)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      )"};
    setContent(u"/path/qmldir", qmldir);
    setFilesUnchanged({qmlDocumentSourceId1, qmlDocumentSourceId2});
    setFilesChanged({qmlDirPathSourceId});
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                       {directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument}});
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports", &SynchronizationPackage::imports, IsEmpty()),
            Field("SynchronizationPackage::types",
                  &SynchronizationPackage::types,
                  UnorderedElementsAre(
                      AllOf(IsStorageType("First.qml",
                                          ImportedType{},
                                          TypeTraitsKind::None,
                                          qmlDocumentSourceId1,
                                          ChangeLevel::Minimal),
                            Field("Type::exportedTypes",
                                  &Type::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                       IsExportedType(pathModuleId, "First", -1, -1)))),
                      AllOf(IsStorageType("First2.qml",
                                          ImportedType{},
                                          TypeTraitsKind::None,
                                          qmlDocumentSourceId2,
                                          ChangeLevel::Minimal),
                            Field("Type::exportedTypes",
                                  &Type::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(pathModuleId, "First2", -1, -1)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(IsDirectoryInfo(directoryPathId,
                                                       qmlDocumentSourceId1,
                                                       ModuleId{},
                                                       FileType::QmlDocument),
                                       IsDirectoryInfo(directoryPathId,
                                                       qmlDocumentSourceId2,
                                                       ModuleId{},
                                                       FileType::QmlDocument))))));

    updater.pathsWithIdsChanged({{qmldirProjectChunkId, {qmlDirPathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_directories_dont_update_qml_documents_if_up_to_date)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setFilesUnchanged({qmlDocumentSourceId3});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports",
                  &SynchronizationPackage::imports,
                  UnorderedElementsAre(import1, import2)),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        ImportedType{"Object"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId1,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        ImportedType{"Object2"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId2,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))),
                    AllOf(IsStorageType("Second.qml",
                                        ImportedType{},
                                        TypeTraitsKind::None,
                                        qmlDocumentSourceId3,
                                        ChangeLevel::Minimal),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "SecondType", 2, 2),
                                                     IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21))),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(
                          directoryPathId, qmlDocumentSourceId3, ModuleId{}, FileType::QmlDocument))))));

    updater.pathsWithIdsChanged({{directoryProjectChunkId, {directoryPathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_qmldirs_dont_update_qml_documents_if_up_to_date)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setFilesUnchanged({qmlDocumentSourceId3});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports",
                  &SynchronizationPackage::imports,
                  UnorderedElementsAre(import1, import2)),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        ImportedType{"Object"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId1,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        ImportedType{"Object2"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId2,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))),
                    AllOf(IsStorageType("Second.qml",
                                        ImportedType{},
                                        TypeTraitsKind::None,
                                        qmlDocumentSourceId3,
                                        ChangeLevel::Minimal),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "SecondType", 2, 2),
                                                     IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21))),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(
                          directoryPathId, qmlDocumentSourceId3, ModuleId{}, FileType::QmlDocument))))));

    updater.pathsWithIdsChanged({{qmldirProjectChunkId, {qmlDirPathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_directory_but_not_qmldir)
{
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
                       {directoryPathId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes},
                       {directoryPathId, qmlDocumentSourceId1, exampleModuleId, FileType::QmlDocument},
                       {directoryPathId, qmlDocumentSourceId2, exampleModuleId, FileType::QmlDocument}});
    setFilesUnchanged({qmlDirPathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(
                    Field("SynchronizationPackage::imports",
                          &SynchronizationPackage::imports,
                          UnorderedElementsAre(import1, import2, import4, import5)),
                    Field("SynchronizationPackage::types",
                          &SynchronizationPackage::types,
                          UnorderedElementsAre(
                              Eq(objectType),
                              Eq(itemType),
                              AllOf(IsStorageType("First.qml",
                                                  ImportedType{"Object"},
                                                  TypeTraitsKind::Reference,
                                                  qmlDocumentSourceId1,
                                                  ChangeLevel::ExcludeExportedTypes),
                                    Field("Type::exportedTypes", &Type::exportedTypes, IsEmpty())),
                              AllOf(IsStorageType("First2.qml",
                                                  ImportedType{"Object2"},
                                                  TypeTraitsKind::Reference,
                                                  qmlDocumentSourceId2,
                                                  ChangeLevel::ExcludeExportedTypes),
                                    Field("Type::exportedTypes", &Type::exportedTypes, IsEmpty())))),
                    Field("SynchronizationPackage::updatedSourceIds",
                          &SynchronizationPackage::updatedSourceIds,
                          UnorderedElementsAre(qmltypesPathSourceId,
                                               qmltypes2PathSourceId,
                                               qmlDocumentSourceId1,
                                               qmlDocumentSourceId2)),
                    Field("SynchronizationPackage::fileStatuses",
                          &SynchronizationPackage::fileStatuses,
                          UnorderedElementsAre(IsFileStatus(qmltypesPathSourceId, 1, 21),
                                               IsFileStatus(qmltypes2PathSourceId, 1, 21),
                                               IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                               IsFileStatus(qmlDocumentSourceId2, 1, 21))),
                    Field("SynchronizationPackage::updatedFileStatusSourceIds",
                          &SynchronizationPackage::updatedFileStatusSourceIds,
                          UnorderedElementsAre(qmltypesPathSourceId,
                                               qmltypes2PathSourceId,
                                               qmlDocumentSourceId1,
                                               qmlDocumentSourceId2)),
                    Field("SynchronizationPackage::directoryInfos",
                          &SynchronizationPackage::directoryInfos,
                          IsEmpty()))));

    updater.pathsWithIdsChanged({{directoryProjectChunkId, {directoryPathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_qml_documents)
{
    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(
                    Field("SynchronizationPackage::imports",
                          &SynchronizationPackage::imports,
                          UnorderedElementsAre(import1, import2)),
                    Field("SynchronizationPackage::types",
                          &SynchronizationPackage::types,
                          UnorderedElementsAre(
                              AllOf(IsStorageType("First.qml",
                                                  ImportedType{"Object"},
                                                  TypeTraitsKind::Reference,
                                                  qmlDocumentSourceId1,
                                                  ChangeLevel::ExcludeExportedTypes),
                                    Field("Type::exportedTypes", &Type::exportedTypes, IsEmpty())),
                              AllOf(IsStorageType("First2.qml",
                                                  ImportedType{"Object2"},
                                                  TypeTraitsKind::Reference,
                                                  qmlDocumentSourceId2,
                                                  ChangeLevel::ExcludeExportedTypes),
                                    Field("Type::exportedTypes", &Type::exportedTypes, IsEmpty())))),
                    Field("SynchronizationPackage::updatedSourceIds",
                          &SynchronizationPackage::updatedSourceIds,
                          UnorderedElementsAre(qmlDocumentSourceId1, qmlDocumentSourceId2)),
                    Field("SynchronizationPackage::fileStatuses",
                          &SynchronizationPackage::fileStatuses,
                          UnorderedElementsAre(IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                               IsFileStatus(qmlDocumentSourceId2, 1, 21))),
                    Field("SynchronizationPackage::updatedFileStatusSourceIds",
                          &SynchronizationPackage::updatedFileStatusSourceIds,
                          UnorderedElementsAre(qmlDocumentSourceId1, qmlDocumentSourceId2)),
                    Field("SynchronizationPackage::directoryInfos",
                          &SynchronizationPackage::directoryInfos,
                          IsEmpty()))));

    updater.pathsWithIdsChanged(
        {{qmlDocumentProjectChunkId, {qmlDocumentSourceId1, qmlDocumentSourceId2}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_removed_qml_documents)
{
    setFilesNotExistsUnchanged({annotationDirectorySourceId, qmlDirPathSourceId});
    setFilesRemoved({qmlDocumentSourceId2});
    setFilesChanged({qmlDocumentSourceId1});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::imports",
                                &SynchronizationPackage::imports,
                                UnorderedElementsAre(import1)),
                          Field("SynchronizationPackage::types",
                                &SynchronizationPackage::types,
                                UnorderedElementsAre(AllOf(
                                    IsStorageType("First.qml",
                                                  Storage::Synchronization::ImportedType{"Object"},
                                                  TypeTraitsKind::Reference,
                                                  qmlDocumentSourceId1,
                                                  Storage::Synchronization::ChangeLevel::ExcludeExportedTypes),
                                    Field("exportedTypes",
                                          &Storage::Synchronization::Type::exportedTypes,
                                          IsEmpty())))),
                          Field("SynchronizationPackage::updatedSourceIds",
                                &SynchronizationPackage::updatedSourceIds,
                                UnorderedElementsAre(qmlDocumentSourceId1, qmlDocumentSourceId2)),
                          Field("SynchronizationPackage::fileStatuses",
                                &SynchronizationPackage::fileStatuses,
                                UnorderedElementsAre(IsFileStatus(qmlDocumentSourceId1, 1, 21))),
                          Field("SynchronizationPackage::updatedFileStatusSourceIds",
                                &SynchronizationPackage::updatedFileStatusSourceIds,
                                UnorderedElementsAre(qmlDocumentSourceId1, qmlDocumentSourceId2)),
                          Field("SynchronizationPackage::directoryInfos",
                                &SynchronizationPackage::directoryInfos,
                                IsEmpty()))));

    updater.pathsWithIdsChanged(
        {{qmlDocumentProjectChunkId, {qmlDocumentSourceId1, qmlDocumentSourceId2}}});
}

TEST_F(ProjectStorageUpdater, watcher_dont_watches_directories_after_qml_document_changes)
{
    EXPECT_CALL(patchWatcherMock, updateContextIdPaths(_, _)).Times(0);

    updater.pathsWithIdsChanged({{qmlDocumentProjectChunkId, {qmlDirPathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_dont_updates_qml_documents_for_other_projects)
{
    EXPECT_CALL(projectStorageMock, synchronize(PackageIsEmpty()));

    updater.pathsWithIdsChanged(
        {{otherQmlDocumentProjectChunkId, {qmlDocumentSourceId1, qmlDocumentSourceId2}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_qmltypes)
{
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
                       {directoryPathId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes}});
    setFilesUnchanged(
        {directoryPathSourceId, qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::imports",
                                &SynchronizationPackage::imports,
                                UnorderedElementsAre(import4, import5)),
                          Field("SynchronizationPackage::types",
                                &SynchronizationPackage::types,
                                UnorderedElementsAre(Eq(objectType), Eq(itemType))),
                          Field("SynchronizationPackage::updatedSourceIds",
                                &SynchronizationPackage::updatedSourceIds,
                                UnorderedElementsAre(qmltypesPathSourceId, qmltypes2PathSourceId)),
                          Field("SynchronizationPackage::fileStatuses",
                                &SynchronizationPackage::fileStatuses,
                                UnorderedElementsAre(IsFileStatus(qmltypesPathSourceId, 1, 21),
                                                     IsFileStatus(qmltypes2PathSourceId, 1, 21))),
                          Field("SynchronizationPackage::updatedFileStatusSourceIds",
                                &SynchronizationPackage::updatedFileStatusSourceIds,
                                UnorderedElementsAre(qmltypesPathSourceId, qmltypes2PathSourceId)),
                          Field("SynchronizationPackage::directoryInfos",
                                &SynchronizationPackage::directoryInfos,
                                IsEmpty()))));

    updater.pathsWithIdsChanged(
        {{qmltypesProjectChunkId, {qmltypesPathSourceId, qmltypes2PathSourceId}}});
}

TEST_F(ProjectStorageUpdater, DISABLED_watcher_updates_removed_qmltypes_without_updated_qmldir)
{
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
                       {directoryPathId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes}});
    setFilesUnchanged(
        {directoryPathSourceId, qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2});
    setFilesRemoved({qmltypesPathSourceId});

    EXPECT_CALL(projectStorageMock, synchronize(_)).Times(0);

    updater.pathsWithIdsChanged(
        {{qmltypesProjectChunkId, {qmltypesPathSourceId, qmltypes2PathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_removed_qmltypes_with_updated_qmldir)
{
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
                       {directoryPathId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes}});
    setFilesUnchanged(
        {directoryPathSourceId, qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2});
    setFilesRemoved({qmltypesPathSourceId});
    updater.pathsWithIdsChanged(
        {{qmltypesProjectChunkId, {qmltypesPathSourceId, qmltypes2PathSourceId}}});
    QString qmldir{R"(module Example
                      typeinfo example2.qmltypes)"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({qmlDirPathSourceId});
    setQmlFileNames(u"/path", {});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::imports",
                                &SynchronizationPackage::imports,
                                UnorderedElementsAre(import5)),
                          Field("SynchronizationPackage::types",
                                &SynchronizationPackage::types,
                                UnorderedElementsAre(Eq(itemType))),
                          Field("SynchronizationPackage::updatedSourceIds",
                                &SynchronizationPackage::updatedSourceIds,
                                UnorderedElementsAre(qmlDirPathSourceId,
                                                     qmltypesPathSourceId,
                                                     qmltypes2PathSourceId)),
                          Field("SynchronizationPackage::fileStatuses",
                                &SynchronizationPackage::fileStatuses,
                                UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21),
                                                     IsFileStatus(qmltypes2PathSourceId, 1, 21))),
                          Field("SynchronizationPackage::updatedFileStatusSourceIds",
                                &SynchronizationPackage::updatedFileStatusSourceIds,
                                UnorderedElementsAre(qmlDirPathSourceId,
                                                     qmltypesPathSourceId,
                                                     qmltypes2PathSourceId)),
                          Field("SynchronizationPackage::directoryInfos",
                                &SynchronizationPackage::directoryInfos,
                                UnorderedElementsAre(IsDirectoryInfo(directoryPathId,
                                                                     qmltypes2PathSourceId,
                                                                     exampleCppNativeModuleId,
                                                                     FileType::QmlTypes))))));

    updater.pathsWithIdsChanged({{qmldirProjectChunkId, {qmlDirPathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_dont_watches_directories_after_qmltypes_changes)
{
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
                       {directoryPathId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes}});
    setFilesUnchanged(
        {directoryPathSourceId, qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2});

    EXPECT_CALL(patchWatcherMock, updateContextIdPaths(_, _)).Times(0);

    updater.pathsWithIdsChanged(
        {{qmltypesProjectChunkId, {qmltypesPathSourceId, qmltypes2PathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_dont_updates_qmltypes_for_other_projects)
{
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
                       {directoryPathId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes}});
    setFilesUnchanged(
        {directoryPathSourceId, qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2});

    EXPECT_CALL(projectStorageMock, synchronize(PackageIsEmpty()));

    updater.pathsWithIdsChanged(
        {{otherQmltypesProjectChunkId, {qmltypesPathSourceId, qmltypes2PathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_directories_and_but_not_included_qml_document)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({directoryPathSourceId});
    setFilesUnchanged({qmlDirPathSourceId});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports",
                  &SynchronizationPackage::imports,
                  UnorderedElementsAre(import1, import2, import3)),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        ImportedType{"Object"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId1,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        ImportedType{"Object2"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId2,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))),
                    AllOf(IsStorageType("Second.qml",
                                        ImportedType{"Object3"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId3,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "SecondType", 2, 2),
                                                     IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDocumentSourceId1, qmlDocumentSourceId2, qmlDocumentSourceId3)),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(directoryPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(directoryPathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId3, 1, 21))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(
                          directoryPathId, qmlDocumentSourceId3, ModuleId{}, FileType::QmlDocument))))));

    updater.pathsWithIdsChanged({{directoryProjectChunkId, {directoryPathSourceId}},
                                 {qmlDocumentProjectChunkId,
                                  {qmlDocumentSourceId1, qmlDocumentSourceId2, qmlDocumentSourceId3}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_qmldir_and_but_not_included_qml_document)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setFilesUnchanged({directoryPathSourceId});
    setFilesChanged({qmlDirPathSourceId});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports",
                  &SynchronizationPackage::imports,
                  UnorderedElementsAre(import1, import2, import3)),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        ImportedType{"Object"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId1,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        ImportedType{"Object2"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId2,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))),
                    AllOf(IsStorageType("Second.qml",
                                        ImportedType{"Object3"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId3,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "SecondType", 2, 2),
                                                     IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId3, 1, 21))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(
                          directoryPathId, qmlDocumentSourceId3, ModuleId{}, FileType::QmlDocument))))));

    updater.pathsWithIdsChanged({{qmldirProjectChunkId, {qmlDirPathSourceId}},
                                 {qmlDocumentProjectChunkId,
                                  {qmlDocumentSourceId1, qmlDocumentSourceId2, qmlDocumentSourceId3}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_qmldir_and_but_not_included_qmltypes)
{
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
                       {directoryPathId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes},
                       {directoryPathId, qmlDocumentSourceId1, exampleModuleId, FileType::QmlDocument},
                       {directoryPathId, qmlDocumentSourceId2, exampleModuleId, FileType::QmlDocument}});
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml
                      typeinfo example.qmltypes
                      typeinfo example2.qmltypes)"};
    setContent(u"/path/qmldir", qmldir);
    setFilesUnchanged({directoryPathSourceId});
    setFilesChanged({qmlDirPathSourceId,
                     qmltypesPathSourceId,
                     qmltypes2PathSourceId,
                     qmlDocumentSourceId1,
                     qmlDocumentSourceId2,
                     qmlDocumentSourceId3});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field("SynchronizationPackage::imports",
                  &SynchronizationPackage::imports,
                  UnorderedElementsAre(import1, import2, import3, import4, import5)),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    Eq(objectType),
                    Eq(itemType),
                    AllOf(IsStorageType("First.qml",
                                        ImportedType{"Object"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId1,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        ImportedType{"Object2"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId2,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))),
                    AllOf(IsStorageType("Second.qml",
                                        ImportedType{"Object3"},
                                        TypeTraitsKind::Reference,
                                        qmlDocumentSourceId3,
                                        ChangeLevel::Full),
                          Field("Type::exportedTypes",
                                &Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "SecondType", 2, 2),
                                                     IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field("SynchronizationPackage::updatedSourceIds",
                  &SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmltypesPathSourceId,
                                       qmltypes2PathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field("SynchronizationPackage::updatedFileStatusSourceIds",
                  &SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmltypesPathSourceId,
                                       qmltypes2PathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field("SynchronizationPackage::fileStatuses",
                  &SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21),
                                       IsFileStatus(qmltypesPathSourceId, 1, 21),
                                       IsFileStatus(qmltypes2PathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId3, 1, 21))),
            Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                  &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                  UnorderedElementsAre(directoryPathId)),
            Field("SynchronizationPackage::directoryInfos",
                  &SynchronizationPackage::directoryInfos,
                  UnorderedElementsAre(
                      IsDirectoryInfo(directoryPathId,
                                      qmltypesPathSourceId,
                                      exampleCppNativeModuleId,
                                      FileType::QmlTypes),
                      IsDirectoryInfo(directoryPathId,
                                      qmltypes2PathSourceId,
                                      exampleCppNativeModuleId,
                                      FileType::QmlTypes),
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(directoryPathId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument),
                      IsDirectoryInfo(
                          directoryPathId, qmlDocumentSourceId3, ModuleId{}, FileType::QmlDocument))))));

    updater.pathsWithIdsChanged(
        {{qmldirProjectChunkId, {qmlDirPathSourceId}},
         {qmltypesProjectChunkId, {qmltypesPathSourceId, qmltypes2PathSourceId}}});
}

TEST_F(ProjectStorageUpdater, errors_for_watcher_updates_are_handled)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);

    ON_CALL(projectStorageMock, synchronize(_))
        .WillByDefault(Throw(QmlDesigner::TypeHasInvalidSourceId{}));

    ASSERT_NO_THROW(updater.pathsWithIdsChanged({{directoryProjectChunkId, {directoryPathSourceId}}}));
}

TEST_F(ProjectStorageUpdater, input_is_reused_next_call_if_an_error_happens)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
                       {directoryPathId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes}});
    setFilesUnchanged({directoryPathSourceId, qmlDirPathSourceId});
    ON_CALL(projectStorageMock, synchronize(_))
        .WillByDefault(Throw(QmlDesigner::TypeHasInvalidSourceId{}));
    updater.pathsWithIdsChanged(
        {{qmltypesProjectChunkId, {qmltypesPathSourceId, qmltypes2PathSourceId}}});
    ON_CALL(projectStorageMock, synchronize(_)).WillByDefault(Return());

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(
                    Field("SynchronizationPackage::imports",
                          &SynchronizationPackage::imports,
                          UnorderedElementsAre(import1, import2, import4, import5)),
                    Field("SynchronizationPackage::types",
                          &SynchronizationPackage::types,
                          UnorderedElementsAre(
                              Eq(objectType),
                              Eq(itemType),
                              AllOf(IsStorageType("First.qml",
                                                  ImportedType{"Object"},
                                                  TypeTraitsKind::Reference,
                                                  qmlDocumentSourceId1,
                                                  ChangeLevel::ExcludeExportedTypes),
                                    Field("Type::exportedTypes", &Type::exportedTypes, IsEmpty())),
                              AllOf(IsStorageType("First2.qml",
                                                  ImportedType{"Object2"},
                                                  TypeTraitsKind::Reference,
                                                  qmlDocumentSourceId2,
                                                  ChangeLevel::ExcludeExportedTypes),
                                    Field("Type::exportedTypes", &Type::exportedTypes, IsEmpty())))),
                    Field("SynchronizationPackage::updatedSourceIds",
                          &SynchronizationPackage::updatedSourceIds,
                          UnorderedElementsAre(qmlDocumentSourceId1,
                                               qmlDocumentSourceId2,
                                               qmltypesPathSourceId,
                                               qmltypes2PathSourceId)),
                    Field("SynchronizationPackage::fileStatuses",
                          &SynchronizationPackage::fileStatuses,
                          UnorderedElementsAre(IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                               IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                               IsFileStatus(qmltypesPathSourceId, 1, 21),
                                               IsFileStatus(qmltypes2PathSourceId, 1, 21))),
                    Field("SynchronizationPackage::updatedFileStatusSourceIds",
                          &SynchronizationPackage::updatedFileStatusSourceIds,
                          UnorderedElementsAre(qmlDocumentSourceId1,
                                               qmlDocumentSourceId2,
                                               qmltypesPathSourceId,
                                               qmltypes2PathSourceId)),
                    Field("SynchronizationPackage::directoryInfos",
                          &SynchronizationPackage::directoryInfos,
                          IsEmpty()))));

    updater.pathsWithIdsChanged(
        {{qmlDocumentProjectChunkId, {qmlDocumentSourceId1, qmlDocumentSourceId2}}});
}

TEST_F(ProjectStorageUpdater, input_is_reused_next_call_if_an_error_happens_and_qmltypes_duplicates_are_removed)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
                       {directoryPathId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes}});
    setFilesUnchanged({directoryPathSourceId, qmlDirPathSourceId});
    ON_CALL(projectStorageMock, synchronize(_))
        .WillByDefault(Throw(QmlDesigner::TypeHasInvalidSourceId{}));
    updater.pathsWithIdsChanged(
        {{qmltypesProjectChunkId, {qmltypesPathSourceId, qmltypes2PathSourceId}}});
    ON_CALL(projectStorageMock, synchronize(_)).WillByDefault(Return());

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(
                    Field("SynchronizationPackage::imports",
                          &SynchronizationPackage::imports,
                          UnorderedElementsAre(import1, import2, import4, import5)),
                    Field("SynchronizationPackage::types",
                          &SynchronizationPackage::types,
                          UnorderedElementsAre(
                              Eq(objectType),
                              Eq(itemType),
                              AllOf(IsStorageType("First.qml",
                                                  ImportedType{"Object"},
                                                  TypeTraitsKind::Reference,
                                                  qmlDocumentSourceId1,
                                                  ChangeLevel::ExcludeExportedTypes),
                                    Field("Type::exportedTypes", &Type::exportedTypes, IsEmpty())),
                              AllOf(IsStorageType("First2.qml",
                                                  ImportedType{"Object2"},
                                                  TypeTraitsKind::Reference,
                                                  qmlDocumentSourceId2,
                                                  ChangeLevel::ExcludeExportedTypes),
                                    Field("Type::exportedTypes", &Type::exportedTypes, IsEmpty())))),
                    Field("SynchronizationPackage::updatedSourceIds",
                          &SynchronizationPackage::updatedSourceIds,
                          UnorderedElementsAre(qmlDocumentSourceId1,
                                               qmlDocumentSourceId2,
                                               qmltypesPathSourceId,
                                               qmltypes2PathSourceId)),
                    Field("SynchronizationPackage::fileStatuses",
                          &SynchronizationPackage::fileStatuses,
                          UnorderedElementsAre(IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                               IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                               IsFileStatus(qmltypesPathSourceId, 1, 21),
                                               IsFileStatus(qmltypes2PathSourceId, 1, 21))),
                    Field("SynchronizationPackage::updatedFileStatusSourceIds",
                          &SynchronizationPackage::updatedFileStatusSourceIds,
                          UnorderedElementsAre(qmlDocumentSourceId1,
                                               qmlDocumentSourceId2,
                                               qmltypesPathSourceId,
                                               qmltypes2PathSourceId)),
                    Field("SynchronizationPackage::directoryInfos",
                          &SynchronizationPackage::directoryInfos,
                          IsEmpty()))));

    updater.pathsWithIdsChanged(
        {{qmltypesProjectChunkId, {qmltypesPathSourceId, qmltypes2PathSourceId}},
         {qmlDocumentProjectChunkId, {qmlDocumentSourceId1, qmlDocumentSourceId2}}});
}

TEST_F(ProjectStorageUpdater, input_is_reused_next_call_if_an_error_happens_and_qml_document_duplicates_are_removed)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setDirectoryInfos(
        directoryPathId,
        {{directoryPathId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathId, qmlDocumentSourceId1, QmlDesigner::ModuleId{}, FileType::QmlDocument},
         {directoryPathId, qmlDocumentSourceId1, QmlDesigner::ModuleId{}, FileType::QmlDocument}});
    setFilesUnchanged({directoryPathSourceId, qmlDirPathSourceId});
    ON_CALL(projectStorageMock, synchronize(_))
        .WillByDefault(Throw(QmlDesigner::TypeHasInvalidSourceId{}));
    updater.pathsWithIdsChanged(
        {{qmlDocumentProjectChunkId, {qmlDocumentSourceId1, qmlDocumentSourceId2}}});
    ON_CALL(projectStorageMock, synchronize(_)).WillByDefault(Return());

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(
                    Field("SynchronizationPackage::imports",
                          &SynchronizationPackage::imports,
                          UnorderedElementsAre(import1, import2, import4, import5)),
                    Field("SynchronizationPackage::types",
                          &SynchronizationPackage::types,
                          UnorderedElementsAre(
                              Eq(objectType),
                              Eq(itemType),
                              AllOf(IsStorageType("First.qml",
                                                  ImportedType{"Object"},
                                                  TypeTraitsKind::Reference,
                                                  qmlDocumentSourceId1,
                                                  ChangeLevel::ExcludeExportedTypes),
                                    Field("Type::exportedTypes", &Type::exportedTypes, IsEmpty())),
                              AllOf(IsStorageType("First2.qml",
                                                  ImportedType{"Object2"},
                                                  TypeTraitsKind::Reference,
                                                  qmlDocumentSourceId2,
                                                  ChangeLevel::ExcludeExportedTypes),
                                    Field("Type::exportedTypes", &Type::exportedTypes, IsEmpty())))),
                    Field("SynchronizationPackage::updatedSourceIds",
                          &SynchronizationPackage::updatedSourceIds,
                          UnorderedElementsAre(qmlDocumentSourceId1,
                                               qmlDocumentSourceId2,
                                               qmltypesPathSourceId,
                                               qmltypes2PathSourceId)),
                    Field("SynchronizationPackage::fileStatuses",
                          &SynchronizationPackage::fileStatuses,
                          UnorderedElementsAre(IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                               IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                               IsFileStatus(qmltypesPathSourceId, 1, 21),
                                               IsFileStatus(qmltypes2PathSourceId, 1, 21))),
                    Field("SynchronizationPackage::updatedFileStatusSourceIds",
                          &SynchronizationPackage::updatedFileStatusSourceIds,
                          UnorderedElementsAre(qmlDocumentSourceId1,
                                               qmlDocumentSourceId2,
                                               qmltypesPathSourceId,
                                               qmltypes2PathSourceId)),
                    Field("SynchronizationPackage::directoryInfos",
                          &SynchronizationPackage::directoryInfos,
                          IsEmpty()))));

    updater.pathsWithIdsChanged(
        {{qmltypesProjectChunkId, {qmltypesPathSourceId, qmltypes2PathSourceId}},
         {qmlDocumentProjectChunkId, {qmlDocumentSourceId1, qmlDocumentSourceId2}}});
}

TEST_F(ProjectStorageUpdater, input_is_cleared_after_successful_update)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setDirectoryInfos(directoryPathId,
                      {{directoryPathId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
                       {directoryPathId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes}});
    setFilesUnchanged({directoryPathSourceId, qmlDirPathSourceId});
    updater.pathsWithIdsChanged(
        {{qmltypesProjectChunkId, {qmltypesPathSourceId, qmltypes2PathSourceId}}});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(
                    Field("SynchronizationPackage::imports",
                          &SynchronizationPackage::imports,
                          UnorderedElementsAre(import1, import2)),
                    Field("SynchronizationPackage::types",
                          &SynchronizationPackage::types,
                          UnorderedElementsAre(
                              AllOf(IsStorageType("First.qml",
                                                  ImportedType{"Object"},
                                                  TypeTraitsKind::Reference,
                                                  qmlDocumentSourceId1,
                                                  ChangeLevel::ExcludeExportedTypes),
                                    Field("Type::exportedTypes", &Type::exportedTypes, IsEmpty())),
                              AllOf(IsStorageType("First2.qml",
                                                  ImportedType{"Object2"},
                                                  TypeTraitsKind::Reference,
                                                  qmlDocumentSourceId2,
                                                  ChangeLevel::ExcludeExportedTypes),
                                    Field("Type::exportedTypes", &Type::exportedTypes, IsEmpty())))),
                    Field("SynchronizationPackage::updatedSourceIds",
                          &SynchronizationPackage::updatedSourceIds,
                          UnorderedElementsAre(qmlDocumentSourceId1, qmlDocumentSourceId2)),
                    Field("SynchronizationPackage::fileStatuses",
                          &SynchronizationPackage::fileStatuses,
                          UnorderedElementsAre(IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                               IsFileStatus(qmlDocumentSourceId2, 1, 21))),
                    Field("SynchronizationPackage::updatedFileStatusSourceIds",
                          &SynchronizationPackage::updatedFileStatusSourceIds,
                          UnorderedElementsAre(qmlDocumentSourceId1, qmlDocumentSourceId2)),
                    Field("SynchronizationPackage::directoryInfos",
                          &SynchronizationPackage::directoryInfos,
                          IsEmpty()))));

    updater.pathsWithIdsChanged(
        {{qmlDocumentProjectChunkId, {qmlDocumentSourceId1, qmlDocumentSourceId2}}});
}

const QString propertyEditorQmlPath = QDir::cleanPath(
    UNITTEST_DIR "/../../../../share/qtcreator/qmldesigner/propertyEditorQmlSources/");

TEST_F(ProjectStorageUpdater, update_property_editor_panes)
{
    ON_CALL(fileSystemMock, fileStatus(_)).WillByDefault([](SourceId sourceId) {
        return FileStatus{sourceId, 1, 21};
    });
    ON_CALL(projectStorageMock, fetchFileStatus(_)).WillByDefault([](SourceId sourceId) {
        return FileStatus{sourceId, 1, 21};
    });
    auto sourceId = sourcePathCache.sourceId(
        QmlDesigner::SourcePath{propertyEditorQmlPath + "/QML/QtObjectPane.qml"});
    auto directoryId = sourcePathCache.directoryPathId(
        QmlDesigner::SourcePath{propertyEditorQmlPath + "/QML"});
    auto directorySourceId = SourceId::create(directoryId, QmlDesigner::FileNameId{});
    setFilesChanged({directorySourceId});
    auto qmlModuleId = storage.moduleId("QML", ModuleKind::QmlLibrary);

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::fileStatuses",
                                &SynchronizationPackage::fileStatuses,
                                UnorderedElementsAre(IsFileStatus(directorySourceId, 1, 21))),
                          Field("SynchronizationPackage::updatedFileStatusSourceIds",
                                &SynchronizationPackage::updatedFileStatusSourceIds,
                                UnorderedElementsAre(directorySourceId)),
                          Field("SynchronizationPackage::propertyEditorQmlPaths",
                                &SynchronizationPackage::propertyEditorQmlPaths,
                                Contains(IsPropertyEditorQmlPath(
                                    qmlModuleId, "QtObject", sourceId, directoryId))),
                          Field("SynchronizationPackage::updatedPropertyEditorQmlPathDirectoryIds",
                                &SynchronizationPackage::updatedPropertyEditorQmlPathDirectoryIds,
                                ElementsAre(directoryId)))));

    updater.update({.propertyEditorResourcesPath = propertyEditorQmlPath});
}

TEST_F(ProjectStorageUpdater, update_property_editor_specifics)
{
    ON_CALL(fileSystemMock, fileStatus(_)).WillByDefault([](SourceId sourceId) {
        return FileStatus{sourceId, 1, 21};
    });
    ON_CALL(projectStorageMock, fetchFileStatus(_)).WillByDefault([](SourceId sourceId) {
        return FileStatus{sourceId, 1, 21};
    });
    auto textSourceId = sourcePathCache.sourceId(
        QmlDesigner::SourcePath{propertyEditorQmlPath + "/QtQuick/TextSpecifics.qml"});
    auto qtQuickDirectoryId = sourcePathCache.directoryPathId(
        QmlDesigner::SourcePath{propertyEditorQmlPath + "/QtQuick"});
    auto qtQuickDirectorySourceId = SourceId::create(qtQuickDirectoryId, QmlDesigner::FileNameId{});
    auto buttonSourceId = sourcePathCache.sourceId(
        QmlDesigner::SourcePath{propertyEditorQmlPath + "/QtQuick/Controls/ButtonSpecifics.qml"});
    auto controlsDirectoryId = sourcePathCache.directoryPathId(
        QmlDesigner::SourcePath{propertyEditorQmlPath + "/QtQuick/Controls"});
    auto controlsDirectorySourceId = SourceId::create(controlsDirectoryId, QmlDesigner::FileNameId{});
    setFilesChanged({qtQuickDirectorySourceId, controlsDirectorySourceId});
    auto qtQuickModuleId = storage.moduleId("QtQuick", ModuleKind::QmlLibrary);
    auto controlsModuleId = storage.moduleId("QtQuick.Controls", ModuleKind::QmlLibrary);

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

    updater.update({.propertyEditorResourcesPath = propertyEditorQmlPath});
}

TEST_F(ProjectStorageUpdater, update_property_editor_panes_is_empty_if_directory_has_not_changed)
{
    updater.update({.propertyEditorResourcesPath = propertyEditorQmlPath});
    ON_CALL(fileSystemMock, fileStatus(_)).WillByDefault([](SourceId sourceId) {
        return FileStatus{sourceId, 1, 21};
    });
    ON_CALL(projectStorageMock, fetchFileStatus(_)).WillByDefault([](SourceId sourceId) {
        return FileStatus{sourceId, 1, 21};
    });

    EXPECT_CALL(projectStorageMock, synchronize(PackageIsEmpty()));

    updater.update({.propertyEditorResourcesPath = propertyEditorQmlPath});
}

TEST_F(ProjectStorageUpdater, update_type_annotations)
{
    auto qtQuickSourceId = sourcePathCache.sourceId(
        QmlDesigner::SourcePath{itemLibraryPath + "/quick.metainfo"});
    auto qtQuickControlSourceId = sourcePathCache.sourceId(
        QmlDesigner::SourcePath{itemLibraryPath + "/qtquickcontrols2.metainfo"});
    setFilesAdded({itemLibraryPathSourceId, qtQuickSourceId, qtQuickControlSourceId});
    auto qtQuickModuleId = moduleId("QtQuick", ModuleKind::QmlLibrary);
    auto qtQuickControlsModuleId = moduleId("QtQuick.Controls.Basic", ModuleKind::QmlLibrary);
    QmlDesigner::Storage::TypeTraits itemTraits;
    itemTraits.canBeContainer = QmlDesigner::FlagIs::True;

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

    updater.update({.typeAnnotationPaths = {itemLibraryPath}});
}

TEST_F(ProjectStorageUpdater, update_added_type_annotation)
{
    auto qtQuickSourceId = sourcePathCache.sourceId(
        QmlDesigner::SourcePath{itemLibraryPath + "/quick.metainfo"});
    auto qtQuickControlSourceId = sourcePathCache.sourceId(
        QmlDesigner::SourcePath{itemLibraryPath + "/qtquickcontrols2.metainfo"});
    setFilesUnchanged({itemLibraryPathSourceId});
    setFilesAdded({qtQuickSourceId, qtQuickControlSourceId});
    auto qtQuickModuleId = moduleId("QtQuick", ModuleKind::QmlLibrary);
    auto qtQuickControlsModuleId = moduleId("QtQuick.Controls.Basic", ModuleKind::QmlLibrary);
    QmlDesigner::Storage::TypeTraits itemTraits;
    itemTraits.canBeContainer = QmlDesigner::FlagIs::True;

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

    updater.update({.typeAnnotationPaths = {itemLibraryPath}});
}

TEST_F(ProjectStorageUpdater, update_changed_type_annotation)
{
    auto qtQuickSourceId = sourcePathCache.sourceId(
        QmlDesigner::SourcePath{itemLibraryPath + "/quick.metainfo"});
    auto qtQuickControlSourceId = sourcePathCache.sourceId(
        QmlDesigner::SourcePath{itemLibraryPath + "/qtquickcontrols2.metainfo"});
    setFilesUnchanged({itemLibraryPathSourceId});
    setFilesChanged({qtQuickSourceId, qtQuickControlSourceId});
    auto qtQuickModuleId = moduleId("QtQuick", ModuleKind::QmlLibrary);
    auto qtQuickControlsModuleId = moduleId("QtQuick.Controls.Basic", ModuleKind::QmlLibrary);
    QmlDesigner::Storage::TypeTraits itemTraits;
    itemTraits.canBeContainer = QmlDesigner::FlagIs::True;

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

    updater.update({.typeAnnotationPaths = {itemLibraryPath}});
}

TEST_F(ProjectStorageUpdater, update_removed_type_annotations)
{
    auto qtQuickSourceId = sourcePathCache.sourceId(
        QmlDesigner::SourcePath{itemLibraryPath + "/quick.metainfo"});
    auto qtQuickControlSourceId = sourcePathCache.sourceId(
        QmlDesigner::SourcePath{itemLibraryPath + "/qtquickcontrols2.metainfo"});
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

TEST_F(ProjectStorageUpdater, update_type_annotations_removed_meta_info_file)
{
    ON_CALL(fileSystemMock, fileStatus(_)).WillByDefault([](SourceId sourceId) {
        return FileStatus{sourceId, 1, 21};
    });
    ON_CALL(projectStorageMock, fetchFileStatus(_)).WillByDefault([](SourceId sourceId) {
        return FileStatus{sourceId, 1, 21};
    });
    auto qtQuickSourceId = sourcePathCache.sourceId(
        QmlDesigner::SourcePath{itemLibraryPath + "/quick.metainfo"});
    auto qtQuickControlSourceId = sourcePathCache.sourceId(
        QmlDesigner::SourcePath{itemLibraryPath + "/qtquickcontrols2.metainfo"});
    ON_CALL(projectStorageMock, typeAnnotationDirectoryIds())
        .WillByDefault(Return(QmlDesigner::SmallDirectoryPathIds<64>{itemLibraryPathDirectoryPathId}));
    ON_CALL(projectStorageMock, typeAnnotationSourceIds(itemLibraryPathDirectoryPathId))
        .WillByDefault(Return(QmlDesigner::SmallSourceIds<4>{qtQuickSourceId, qtQuickControlSourceId}));
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

    updater.update({.typeAnnotationPaths = {itemLibraryPath}});
}

TEST_F(ProjectStorageUpdater, update_type_annotations_removed_directory)
{
    ON_CALL(fileSystemMock, fileStatus(_)).WillByDefault([](SourceId sourceId) {
        return FileStatus{sourceId, 1, 21};
    });
    ON_CALL(projectStorageMock, fetchFileStatus(_)).WillByDefault([](SourceId sourceId) {
        return FileStatus{sourceId, 1, 21};
    });
    auto qtQuickSourceId = sourcePathCache.sourceId(
        QmlDesigner::SourcePath{itemLibraryPath + "/quick.metainfo"});
    auto qtQuickControlSourceId = sourcePathCache.sourceId(
        QmlDesigner::SourcePath{itemLibraryPath + "/qtquickcontrols2.metainfo"});
    ON_CALL(projectStorageMock, typeAnnotationDirectoryIds())
        .WillByDefault(Return(QmlDesigner::SmallDirectoryPathIds<64>{
            itemLibraryPathDirectoryPathId,
        }));
    ON_CALL(projectStorageMock, typeAnnotationSourceIds(itemLibraryPathDirectoryPathId))
        .WillByDefault(Return(QmlDesigner::SmallSourceIds<4>{qtQuickSourceId, qtQuickControlSourceId}));
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

TEST_F(ProjectStorageUpdater, synchronize_added_property_editor_qml_paths_directory)
{
    setFileSystemSubdirectories(u"/path/one", {"/path/one/designer"});
    DirectoryPathId designer1DirectoryId = sourcePathCache.directoryPathId("/path/one/designer");
    SourceId designer1SourceId = SourceId::create(designer1DirectoryId, QmlDesigner::FileNameId{});
    setFilesUnchanged({path1SourceId});
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
                                UnorderedElementsAre(designer1DirectoryId)),
                          Field("SynchronizationPackage::directoryInfos",
                                &SynchronizationPackage::directoryInfos,
                                IsEmpty()),
                          Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                                &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                                UnorderedElementsAre(path1DirectoryPathId)))));

    updater.update({.projectDirectory = "/path/one"});
}

TEST_F(ProjectStorageUpdater, synchronize_changed_property_editor_qml_paths_directory)
{
    setFileSystemSubdirectories(u"/path/one", {"/path/one/designer"});
    DirectoryPathId designer1DirectoryId = sourcePathCache.directoryPathId("/path/one/designer");
    SourceId designer1SourceId = SourceId::create(designer1DirectoryId, QmlDesigner::FileNameId{});
    setFilesUnchanged({path1SourceId});
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
                                UnorderedElementsAre(designer1DirectoryId)),
                          Field("SynchronizationPackage::directoryInfos",
                                &SynchronizationPackage::directoryInfos,
                                IsEmpty()),
                          Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                                &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                                UnorderedElementsAre(path1DirectoryPathId)))));

    updater.update({.projectDirectory = "/path/one"});
}

TEST_F(ProjectStorageUpdater, dont_synchronize_empty_property_editor_qml_paths_directory)
{
    setFileSystemSubdirectories(u"/path/two", {});
    DirectoryPathId designer2DirectoryId = sourcePathCache.directoryPathId("/path/two/designer");
    SourceId designer2SourceId = SourceId::create(designer2DirectoryId, QmlDesigner::FileNameId{});
    setFilesChanged({path2SourceId});
    setFilesNotExists({designer2SourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field("SynchronizationPackage::fileStatuses",
                                &SynchronizationPackage::fileStatuses,
                                IsSupersetOf({IsFileStatus(path2SourceId, 1, 21),
                                              IsFileStatus(designer2SourceId, -1, -1)})),
                          Field("SynchronizationPackage::updatedFileStatusSourceIds",
                                &SynchronizationPackage::updatedFileStatusSourceIds,
                                IsSupersetOf({designer2SourceId})),
                          Field("SynchronizationPackage::propertyEditorQmlPaths",
                                &SynchronizationPackage::propertyEditorQmlPaths,
                                IsEmpty()),
                          Field("SynchronizationPackage::updatedPropertyEditorQmlPathDirectoryIds",
                                &SynchronizationPackage::updatedPropertyEditorQmlPathDirectoryIds,
                                IsEmpty()),
                          Field("SynchronizationPackage::directoryInfos",
                                &SynchronizationPackage::directoryInfos,
                                IsEmpty()),
                          Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                                &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                                UnorderedElementsAre(path2DirectoryPathId)))));

    updater.update({.projectDirectory = "/path/two"});
}

TEST_F(ProjectStorageUpdater, remove_property_editor_qml_paths_if_designer_directory_is_removed)
{
    DirectoryPathId designer1DirectoryId = sourcePathCache.directoryPathId("/path/one/designer");
    SourceId designer1SourceId = SourceId::create(designer1DirectoryId, QmlDesigner::FileNameId{});
    setFilesUnchanged({path1SourceId, qmldir1SourceId});
    setFilesRemoved({designer1SourceId});

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
                                UnorderedElementsAre(designer1DirectoryId)),
                          Field("SynchronizationPackage::directoryInfos",
                                &SynchronizationPackage::directoryInfos,
                                IsEmpty()),
                          Field("SynchronizationPackage::updatedDirectoryInfoDirectoryIds",
                                &SynchronizationPackage::updatedDirectoryInfoDirectoryIds,
                                UnorderedElementsAre(path1DirectoryPathId)))));

    updater.update({.projectDirectory = "/path/one"});
}

TEST_F(ProjectStorageUpdater,
       synchronize_property_editor_qml_paths_directory_if_designer_directory_is_changed)
{
    setFileSystemSubdirectories(u"/path/one", {"/path/one/designer"});
    QString qmldir{R"(module Bar)"};
    setContent(u"/path/one/qmldir", qmldir);
    DirectoryPathId designer1DirectoryId = sourcePathCache.directoryPathId("/path/one/designer");
    SourceId propertyEditorSpecificSourceId = sourcePathCache.sourceId(
        "/path/one/designer/FooSpecifics.qml");
    SourceId propertyEditorSpecificDynamicSourceId = sourcePathCache.sourceId(
        "/path/one/designer/HuoSpecificsDynamic.qml");
    SourceId propertyEditorPaneSourceId = sourcePathCache.sourceId(
        "/path/one/designer/CaoPane.qml");
    SourceId designer1SourceId = SourceId::create(designer1DirectoryId, QmlDesigner::FileNameId{});
    setFilesChanged({designer1SourceId});
    setFilesUnchanged({path1SourceId, qmldir1SourceId});
    auto barModuleId = storage.moduleId("Bar", ModuleKind::QmlLibrary);
    setFileNames(u"/path/one/designer",
                 {"FooSpecifics.qml", "HuoSpecificsDynamic.qml", "CaoPane.qml"},
                 {"*Pane.qml", "*Specifics.qml", "*SpecificsDynamic.qml"});

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

TEST_F(ProjectStorageUpdater,
       synchronize_property_editor_qml_paths_directory_if_qml_directory_is_changed)
{
    setFileSystemSubdirectories(u"/path/one", {"/path/one/designer"});
    QString qmldir{R"(module Bar)"};
    setContent(u"/path/one/qmldir", qmldir);
    DirectoryPathId designer1DirectoryId = sourcePathCache.directoryPathId("/path/one/designer");
    SourceId propertyEditorSpecificSourceId = sourcePathCache.sourceId(
        "/path/one/designer/FooSpecifics.qml");
    SourceId propertyEditorSpecificDynamicSourceId = sourcePathCache.sourceId(
        "/path/one/designer/HuoSpecificsDynamic.qml");
    SourceId propertyEditorPaneSourceId = sourcePathCache.sourceId(
        "/path/one/designer/CaoPane.qml");
    SourceId designer1SourceId = SourceId::create(designer1DirectoryId, QmlDesigner::FileNameId{});
    setFilesChanged({path1SourceId});
    setFilesUnchanged({qmldir1SourceId, designer1SourceId});
    auto barModuleId = storage.moduleId("Bar", ModuleKind::QmlLibrary);
    setFileNames(u"/path/one/designer",
                 {"FooSpecifics.qml", "HuoSpecificsDynamic.qml", "CaoPane.qml"},
                 {"*Pane.qml", "*Specifics.qml", "*SpecificsDynamic.qml"});

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

TEST_F(ProjectStorageUpdater, synchronize_property_editor_qml_paths_directory_if_qmldir_is_changed)
{
    setFileSystemSubdirectories(u"/path/one", {"/path/one/designer"});
    QString qmldir{R"(module Bar)"};
    setContent(u"/path/one/qmldir", qmldir);
    DirectoryPathId designer1DirectoryId = sourcePathCache.directoryPathId("/path/one/designer");
    SourceId propertyEditorSpecificSourceId = sourcePathCache.sourceId(
        "/path/one/designer/FooSpecifics.qml");
    SourceId propertyEditorSpecificDynamicSourceId = sourcePathCache.sourceId(
        "/path/one/designer/HuoSpecificsDynamic.qml");
    SourceId propertyEditorPaneSourceId = sourcePathCache.sourceId(
        "/path/one/designer/CaoPane.qml");
    SourceId designer1SourceId = SourceId::create(designer1DirectoryId, QmlDesigner::FileNameId{});
    setFilesChanged({qmldir1SourceId});
    setFilesUnchanged({path1SourceId, designer1SourceId});
    auto barModuleId = storage.moduleId("Bar", ModuleKind::QmlLibrary);
    setFileNames(u"/path/one/designer",
                 {"FooSpecifics.qml", "HuoSpecificsDynamic.qml", "CaoPane.qml"},
                 {"*Pane.qml", "*Specifics.qml", "*SpecificsDynamic.qml"});

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
