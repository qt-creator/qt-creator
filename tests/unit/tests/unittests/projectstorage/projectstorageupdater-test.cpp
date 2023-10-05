// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include "../mocks/filesystemmock.h"
#include "../mocks/projectstoragemock.h"
#include "../mocks/projectstoragepathwatchermock.h"
#include "../mocks/qmldocumentparsermock.h"
#include "../mocks/qmltypesparsermock.h"

#include <projectstorage/filestatuscache.h>
#include <projectstorage/projectstorage.h>
#include <projectstorage/projectstorageupdater.h>
#include <projectstorage/sourcepathcache.h>
#include <sqlitedatabase.h>

namespace {

namespace Storage = QmlDesigner::Storage;

using QmlDesigner::FileStatus;
using QmlDesigner::ModuleId;
using QmlDesigner::SourceId;
namespace Storage = QmlDesigner::Storage;
using QmlDesigner::IdPaths;
using QmlDesigner::Storage::Import;
using QmlDesigner::Storage::Synchronization::FileType;
using QmlDesigner::Storage::Synchronization::IsAutoVersion;
using QmlDesigner::Storage::Synchronization::ModuleExportedImport;
using QmlDesigner::Storage::Synchronization::ProjectData;
using QmlDesigner::Storage::Synchronization::SynchronizationPackage;
using QmlDesigner::Storage::TypeTraits;
using QmlDesigner::Storage::Version;

MATCHER_P5(IsStorageType,
           typeName,
           prototype,
           traits,
           sourceId,
           changeLevel,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::Synchronization::Type(
                   typeName, prototype, traits, sourceId, changeLevel)))
{
    const Storage::Synchronization::Type &type = arg;

    return type.typeName == typeName && type.traits == traits && type.sourceId == sourceId
           && Storage::Synchronization::ImportedTypeName{prototype} == type.prototype
           && type.changeLevel == changeLevel;
}

MATCHER_P3(IsPropertyDeclaration,
           name,
           typeName,
           traits,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::Synchronization::PropertyDeclaration{name, typeName, traits}))
{
    const Storage::Synchronization::PropertyDeclaration &propertyDeclaration = arg;

    return propertyDeclaration.name == name
           && Storage::Synchronization::ImportedTypeName{typeName} == propertyDeclaration.typeName
           && propertyDeclaration.traits == traits;
}

MATCHER_P4(IsExportedType,
           moduleId,
           name,
           majorVersion,
           minorVersion,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::Synchronization::ExportedType{
                   moduleId, name, Storage::Version{majorVersion, minorVersion}}))
{
    const Storage::Synchronization::ExportedType &type = arg;

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

MATCHER_P4(IsProjectData,
           projectSourceId,
           sourceId,
           moduleId,
           fileType,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::Synchronization::ProjectData{
                   projectSourceId, sourceId, moduleId, fileType}))
{
    const Storage::Synchronization::ProjectData &projectData = arg;

    return compareInvalidAreTrue(projectData.projectSourceId, projectSourceId)
           && projectData.sourceId == sourceId
           && compareInvalidAreTrue(projectData.moduleId, moduleId)
           && projectData.fileType == fileType;
}

MATCHER(PackageIsEmpty, std::string(negation ? "isn't empty" : "is empty"))
{
    const Storage::Synchronization::SynchronizationPackage &package = arg;

    return package.imports.empty() && package.types.empty() && package.fileStatuses.empty()
           && package.updatedSourceIds.empty() && package.projectDatas.empty()
           && package.updatedFileStatusSourceIds.empty() && package.updatedProjectSourceIds.empty()
           && package.moduleDependencies.empty() && package.updatedModuleDependencySourceIds.empty()
           && package.moduleExportedImports.empty() && package.updatedModuleIds.empty()
           && package.propertyEditorQmlPaths.empty()
           && package.updatedPropertyEditorQmlPathSourceIds.empty();
}

template<typename ModuleIdMatcher, typename TypeNameMatcher, typename SourceIdMatcher>
auto IsPropertyEditorQmlPath(const ModuleIdMatcher &moduleIdMatcher,
                             const TypeNameMatcher &typeNameMatcher,
                             const SourceIdMatcher &pathIdMatcher)
{
    using QmlDesigner::Storage::Synchronization::PropertyEditorQmlPath;
    return AllOf(Field(&PropertyEditorQmlPath::moduleId, moduleIdMatcher),
                 Field(&PropertyEditorQmlPath::typeName, typeNameMatcher),
                 Field(&PropertyEditorQmlPath::pathId, pathIdMatcher));
}

class ProjectStorageUpdater : public testing::Test
{
public:
    ProjectStorageUpdater()
    {
        setFilesChanged({qmltypesPathSourceId,
                         qmltypes2PathSourceId,
                         qmlDirPathSourceId,
                         qmlDocumentSourceId1,
                         qmlDocumentSourceId2,
                         qmlDocumentSourceId3});

        setFilesDontChanged({directoryPathSourceId,
                             path1SourceId,
                             path2SourceId,
                             path3SourceId,
                             firstSourceId,
                             secondSourceId,
                             thirdSourceId,
                             qmltypes1SourceId,
                             qmltypes2SourceId});

        setFilesAdded({qmldir1SourceId, qmldir2SourceId, qmldir3SourceId});

        setContent(u"/path/qmldir", qmldirContent);

        setQmlFileNames(u"/path", {"First.qml", "First2.qml", "Second.qml"});

        ON_CALL(projectStorageMock, moduleId(_)).WillByDefault([&](const auto &name) {
            return storage.moduleId(name);
        });

        firstType.prototype = Storage::Synchronization::ImportedType{"Object"};
        secondType.prototype = Storage::Synchronization::ImportedType{"Object2"};
        thirdType.prototype = Storage::Synchronization::ImportedType{"Object3"};

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

        ON_CALL(qmlDocumentParserMock, parse(qmlDocument1, _, _, _))
            .WillByDefault([&](auto, auto &imports, auto, auto) {
                imports.push_back(import1);
                return firstType;
            });
        ON_CALL(qmlDocumentParserMock, parse(qmlDocument2, _, _, _))
            .WillByDefault([&](auto, auto &imports, auto, auto) {
                imports.push_back(import2);
                return secondType;
            });
        ON_CALL(qmlDocumentParserMock, parse(qmlDocument3, _, _, _))
            .WillByDefault([&](auto, auto &imports, auto, auto) {
                imports.push_back(import3);
                return thirdType;
            });
        ON_CALL(qmlTypesParserMock, parse(Eq(qmltypes1), _, _, _))
            .WillByDefault([&](auto, auto &imports, auto &types, auto) {
                types.push_back(objectType);
                imports.push_back(import4);
            });
        ON_CALL(qmlTypesParserMock, parse(Eq(qmltypes2), _, _, _))
            .WillByDefault([&](auto, auto &imports, auto &types, auto) {
                types.push_back(itemType);
                imports.push_back(import5);
            });
    }

    void setFilesDontChanged(const QmlDesigner::SourceIds &sourceIds)
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
            ON_CALL(fileSystemMock, fileStatus(Eq(sourceId))).WillByDefault(Return(FileStatus{}));
            ON_CALL(projectStorageMock, fetchFileStatus(Eq(sourceId)))
                .WillByDefault(Return(FileStatus{sourceId, 1, 21}));
        }
    }

    void setFilesDontExists(const QmlDesigner::SourceIds &sourceIds)
    {
        for (auto sourceId : sourceIds) {
            ON_CALL(fileSystemMock, fileStatus(Eq(sourceId))).WillByDefault(Return(FileStatus{}));
            ON_CALL(projectStorageMock, fetchFileStatus(Eq(sourceId)))
                .WillByDefault(Return(FileStatus{}));
        }
    }

    void setQmlFileNames(QStringView directoryPath, const QStringList &qmlFileNames)
    {
        ON_CALL(fileSystemMock, qmlFileNames(Eq(directoryPath))).WillByDefault(Return(qmlFileNames));
    }

    void setProjectDatas(SourceId directoryPathSourceId,
                         const QmlDesigner::Storage::Synchronization::ProjectDatas &projectDatas)
    {
        ON_CALL(projectStorageMock, fetchProjectDatas(Eq(directoryPathSourceId)))
            .WillByDefault(Return(projectDatas));
        for (const ProjectData &projectData : projectDatas) {
            ON_CALL(projectStorageMock, fetchProjectData(Eq(projectData.sourceId)))
                .WillByDefault(Return(std::optional{projectData}));
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

protected:
    NiceMock<FileSystemMock> fileSystemMock;
    NiceMock<ProjectStorageMock> projectStorageMock;
    NiceMock<QmlTypesParserMock> qmlTypesParserMock;
    NiceMock<QmlDocumentParserMock> qmlDocumentParserMock;
    QmlDesigner::FileStatusCache fileStatusCache{fileSystemMock};
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    QmlDesigner::ProjectStorage<Sqlite::Database> storage{database, database.isInitialized()};
    QmlDesigner::SourcePathCache<QmlDesigner::ProjectStorage<Sqlite::Database>> sourcePathCache{
        storage};
    NiceMock<ProjectStoragePathWatcherMock> patchWatcherMock;
    QmlDesigner::ProjectPartId projectPartId = QmlDesigner::ProjectPartId::create(1);
    QmlDesigner::ProjectPartId otherProjectPartId = QmlDesigner::ProjectPartId::create(0);
    QmlDesigner::ProjectStorageUpdater updater{fileSystemMock,
                                               projectStorageMock,
                                               fileStatusCache,
                                               sourcePathCache,
                                               qmlDocumentParserMock,
                                               qmlTypesParserMock,
                                               patchWatcherMock,
                                               projectPartId};
    SourceId qmltypesPathSourceId = sourcePathCache.sourceId("/path/example.qmltypes");
    SourceId qmltypes2PathSourceId = sourcePathCache.sourceId("/path/example2.qmltypes");
    SourceId qmlDirPathSourceId = sourcePathCache.sourceId("/path/qmldir");
    SourceId directoryPathSourceId = sourcePathCache.sourceId("/path/.");
    SourceId qmlDocumentSourceId1 = sourcePathCache.sourceId("/path/First.qml");
    SourceId qmlDocumentSourceId2 = sourcePathCache.sourceId("/path/First2.qml");
    SourceId qmlDocumentSourceId3 = sourcePathCache.sourceId("/path/Second.qml");
    ModuleId qmlModuleId{storage.moduleId("Qml")};
    ModuleId qmlCppNativeModuleId{storage.moduleId("Qml-cppnative")};
    ModuleId exampleModuleId{storage.moduleId("Example")};
    ModuleId exampleCppNativeModuleId{storage.moduleId("Example-cppnative")};
    ModuleId builtinModuleId{storage.moduleId("QML")};
    ModuleId builtinCppNativeModuleId{storage.moduleId("QML-cppnative")};
    ModuleId quickModuleId{storage.moduleId("Quick")};
    ModuleId quickCppNativeModuleId{storage.moduleId("Quick-cppnative")};
    ModuleId pathModuleId{storage.moduleId("/path")};
    ModuleId subPathQmlModuleId{storage.moduleId("/path/qml")};
    Storage::Synchronization::Type objectType{
        "QObject",
        Storage::Synchronization::ImportedType{},
        Storage::Synchronization::ImportedType{},
        Storage::TypeTraits::Reference,
        qmltypesPathSourceId,
        {Storage::Synchronization::ExportedType{exampleModuleId, "Object"},
         Storage::Synchronization::ExportedType{exampleModuleId, "Obj"}}};
    Storage::Synchronization::Type itemType{"QItem",
                                            Storage::Synchronization::ImportedType{},
                                            Storage::Synchronization::ImportedType{},
                                            Storage::TypeTraits::Reference,
                                            qmltypes2PathSourceId,
                                            {Storage::Synchronization::ExportedType{exampleModuleId,
                                                                                    "Item"}}};
    QString qmlDocument1{"First{}"};
    QString qmlDocument2{"Second{}"};
    QString qmlDocument3{"Third{}"};
    Storage::Synchronization::Type firstType;
    Storage::Synchronization::Type secondType;
    Storage::Synchronization::Type thirdType;
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
    QmlDesigner::ProjectChunkId directoryProjectChunkId{projectPartId,
                                                        QmlDesigner::SourceType::Directory};
    QmlDesigner::ProjectChunkId qmldirProjectChunkId{projectPartId, QmlDesigner::SourceType::QmlDir};
    QmlDesigner::ProjectChunkId qmlDocumentProjectChunkId{projectPartId, QmlDesigner::SourceType::Qml};
    QmlDesigner::ProjectChunkId qmltypesProjectChunkId{projectPartId,
                                                       QmlDesigner::SourceType::QmlTypes};
    QmlDesigner::ProjectChunkId otherDirectoryProjectChunkId{otherProjectPartId,
                                                             QmlDesigner::SourceType::Directory};
    QmlDesigner::ProjectChunkId otherQmldirProjectChunkId{otherProjectPartId,
                                                          QmlDesigner::SourceType::QmlDir};
    QmlDesigner::ProjectChunkId otherQmlDocumentProjectChunkId{otherProjectPartId,
                                                               QmlDesigner::SourceType::Qml};
    QmlDesigner::ProjectChunkId otherQmltypesProjectChunkId{otherProjectPartId,
                                                            QmlDesigner::SourceType::QmlTypes};
    SourceId path1SourceId = sourcePathCache.sourceId("/path/one/.");
    SourceId path2SourceId = sourcePathCache.sourceId("/path/two/.");
    SourceId path3SourceId = sourcePathCache.sourceId("/path/three/.");
    SourceId qmldir1SourceId = sourcePathCache.sourceId("/path/one/qmldir");
    SourceId qmldir2SourceId = sourcePathCache.sourceId("/path/two/qmldir");
    SourceId qmldir3SourceId = sourcePathCache.sourceId("/path/three/qmldir");
    SourceId firstSourceId = sourcePathCache.sourceId("/path/one/First.qml");
    SourceId secondSourceId = sourcePathCache.sourceId("/path/one/Second.qml");
    SourceId thirdSourceId = sourcePathCache.sourceId("/path/two/Third.qml");
    SourceId qmltypes1SourceId = sourcePathCache.sourceId("/path/one/example.qmltypes");
    SourceId qmltypes2SourceId = sourcePathCache.sourceId("/path/two/example2.qmltypes");
};

TEST_F(ProjectStorageUpdater, get_content_for_qml_dir_paths_if_file_status_is_different)
{
    SourceId qmlDir1PathSourceId = sourcePathCache.sourceId("/path/one/qmldir");
    SourceId qmlDir2PathSourceId = sourcePathCache.sourceId("/path/two/qmldir");
    SourceId qmlDir3PathSourceId = sourcePathCache.sourceId("/path/three/qmldir");
    SourceId path3SourceId = sourcePathCache.sourceId("/path/three/.");
    QStringList directories = {"/path/one", "/path/two", "/path/three"};
    setFilesChanged({qmlDir1PathSourceId, qmlDir2PathSourceId});
    setFilesDontChanged({qmlDir3PathSourceId, path3SourceId});

    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/one/qmldir"))));
    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/two/qmldir"))));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, request_file_status_from_file_system)
{
    EXPECT_CALL(fileSystemMock, fileStatus(Ne(directoryPathSourceId))).Times(AnyNumber());

    EXPECT_CALL(fileSystemMock, fileStatus(Eq(directoryPathSourceId)));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, get_content_for_qml_types)
{
    QString qmldir{R"(module Example
                      typeinfo example.qmltypes)"};
    setQmlFileNames(u"/path", {});
    setExpectedContent(u"/path/qmldir", qmldir);

    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/example.qmltypes"))));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, get_content_for_qml_types_if_project_storage_file_status_is_invalid)
{
    QString qmldir{R"(module Example
                      typeinfo example.qmltypes)"};
    setQmlFileNames(u"/path", {});
    setExpectedContent(u"/path/qmldir", qmldir);
    setFilesAdded({qmltypesPathSourceId});

    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/example.qmltypes"))));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, parse_qml_types)
{
    QString qmldir{R"(module Example
                      typeinfo example.qmltypes
                      typeinfo example2.qmltypes)"};
    setContent(u"/path/qmldir", qmldir);
    QString qmltypes{"Module {\ndependencies: []}"};
    QString qmltypes2{"Module {\ndependencies: [foo]}"};
    setContent(u"/path/example.qmltypes", qmltypes);
    setContent(u"/path/example2.qmltypes", qmltypes2);

    EXPECT_CALL(qmlTypesParserMock,
                parse(qmltypes, _, _, Field(&ProjectData::moduleId, exampleCppNativeModuleId)));
    EXPECT_CALL(qmlTypesParserMock,
                parse(qmltypes2, _, _, Field(&ProjectData::moduleId, exampleCppNativeModuleId)));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, synchronize_is_empty_for_no_change)
{
    setFilesDontChanged({qmltypesPathSourceId, qmltypes2PathSourceId, qmlDirPathSourceId});

    EXPECT_CALL(projectStorageMock, synchronize(PackageIsEmpty()));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, synchronize_qml_types)
{
    Storage::Import import{qmlModuleId, Storage::Version{2, 3}, qmltypesPathSourceId};
    QString qmltypes{"Module {\ndependencies: []}"};
    setQmlFileNames(u"/path", {});
    setContent(u"/path/example.qmltypes", qmltypes);
    ON_CALL(qmlTypesParserMock, parse(qmltypes, _, _, _))
        .WillByDefault([&](auto, auto &imports, auto &types, auto) {
            types.push_back(objectType);
            imports.push_back(import);
        });

    EXPECT_CALL(projectStorageMock, moduleId(Eq("Example")));
    EXPECT_CALL(projectStorageMock, moduleId(Eq("Example-cppnative")));
    EXPECT_CALL(projectStorageMock, moduleId(Eq("/path")));
    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field(&SynchronizationPackage::imports, ElementsAre(import)),
                          Field(&SynchronizationPackage::types, ElementsAre(Eq(objectType))),
                          Field(&SynchronizationPackage::updatedSourceIds,
                                UnorderedElementsAre(qmlDirPathSourceId, qmltypesPathSourceId)),
                          Field(&SynchronizationPackage::fileStatuses,
                                UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21),
                                                     IsFileStatus(qmltypesPathSourceId, 1, 21))),
                          Field(&SynchronizationPackage::projectDatas,
                                UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                                   qmltypesPathSourceId,
                                                                   exampleCppNativeModuleId,
                                                                   FileType::QmlTypes))),
                          Field(&SynchronizationPackage::updatedProjectSourceIds,
                                UnorderedElementsAre(directoryPathSourceId)))));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, synchronize_qml_types_throws_if_qmltpes_does_not_exists)
{
    Storage::Import import{qmlModuleId, Storage::Version{2, 3}, qmltypesPathSourceId};
    setFilesDontExists({qmltypesPathSourceId});

    ASSERT_THROW(updater.update(directories, {}, {}), QmlDesigner::CannotParseQmlTypesFile);
}

TEST_F(ProjectStorageUpdater, synchronize_qml_types_are_empty_if_file_does_not_changed)
{
    QString qmltypes{"Module {\ndependencies: []}"};
    setContent(u"/path/example.qmltypes", qmltypes);
    ON_CALL(qmlTypesParserMock, parse(qmltypes, _, _, _))
        .WillByDefault([&](auto, auto &, auto &types, auto) { types.push_back(objectType); });
    setFilesDontChanged({qmltypesPathSourceId, qmltypes2PathSourceId, qmlDirPathSourceId});

    EXPECT_CALL(projectStorageMock, synchronize(PackageIsEmpty()));

    updater.update(directories, {}, {});
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

    updater.update(directories, {}, {});
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

    EXPECT_CALL(qmlDocumentParserMock, parse(qmlDocument1, _, _, _));
    EXPECT_CALL(qmlDocumentParserMock, parse(qmlDocument2, _, _, _));
    EXPECT_CALL(qmlDocumentParserMock, parse(qmlDocument3, _, _, _));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, parse_qml_documents_with_non_existing_qml_document_throws)
{
    QString qmldir{R"(module Example
                      NonexitingType 1.0 NonexitingType.qml)"};
    setContent(u"/path/qmldir", qmldir);

    ASSERT_THROW(updater.update(directories, {}, {}), QmlDesigner::CannotParseQmlDocumentFile);
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
            Field(&SynchronizationPackage::imports, UnorderedElementsAre(import1, import2, import3)),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        Storage::Synchronization::ImportedType{"Object"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId1,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        Storage::Synchronization::ImportedType{"Object2"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId2,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))),
                    AllOf(IsStorageType("Second.qml",
                                        Storage::Synchronization::ImportedType{"Object3"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId3,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "SecondType", 2, 2),
                                                     IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId3, 1, 21))),
            Field(&SynchronizationPackage::updatedProjectSourceIds,
                  UnorderedElementsAre(directoryPathSourceId)),
            Field(&SynchronizationPackage::projectDatas,
                  UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId1,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId2,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId3,
                                                     ModuleId{},
                                                     FileType::QmlDocument))))));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, synchronize_add_only_qml_document_in_directory)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({directoryPathSourceId});
    setFilesDontChanged({qmlDirPathSourceId, qmlDocumentSourceId1});
    setFilesAdded({qmlDocumentSourceId2});
    setProjectDatas(directoryPathSourceId,
                    {{directoryPathSourceId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument}});
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(
                    Field(&SynchronizationPackage::imports, UnorderedElementsAre(import2)),
                    Field(&SynchronizationPackage::types,
                          UnorderedElementsAre(
                              AllOf(IsStorageType("First.qml",
                                                  Storage::Synchronization::ImportedType{},
                                                  TypeTraits::Reference,
                                                  qmlDocumentSourceId1,
                                                  Storage::Synchronization::ChangeLevel::Minimal),
                                    Field(&Storage::Synchronization::Type::exportedTypes,
                                          UnorderedElementsAre(
                                              IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                              IsExportedType(pathModuleId, "First", -1, -1)))),
                              AllOf(IsStorageType("First2.qml",
                                                  Storage::Synchronization::ImportedType{"Object2"},
                                                  TypeTraits::Reference,
                                                  qmlDocumentSourceId2,
                                                  Storage::Synchronization::ChangeLevel::Full),
                                    Field(&Storage::Synchronization::Type::exportedTypes,
                                          UnorderedElementsAre(
                                              IsExportedType(pathModuleId, "First2", -1, -1)))))),
                    Field(&SynchronizationPackage::updatedSourceIds,
                          UnorderedElementsAre(qmlDocumentSourceId1, qmlDocumentSourceId2)),
                    Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                          UnorderedElementsAre(directoryPathSourceId, qmlDocumentSourceId2)),
                    Field(&SynchronizationPackage::fileStatuses,
                          UnorderedElementsAre(IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                               IsFileStatus(directoryPathSourceId, 1, 21))),
                    Field(&SynchronizationPackage::updatedProjectSourceIds,
                          UnorderedElementsAre(directoryPathSourceId)),
                    Field(&SynchronizationPackage::projectDatas,
                          UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                             qmlDocumentSourceId1,
                                                             ModuleId{},
                                                             FileType::QmlDocument),
                                               IsProjectData(directoryPathSourceId,
                                                             qmlDocumentSourceId2,
                                                             ModuleId{},
                                                             FileType::QmlDocument))))));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, synchronize_removes_qml_document)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      )"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({qmlDirPathSourceId});
    setFilesDontChanged({qmlDocumentSourceId1, qmlDocumentSourceId2});
    setFilesRemoved({qmlDocumentSourceId3});
    setProjectDatas(directoryPathSourceId,
                    {{directoryPathSourceId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                     {directoryPathSourceId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument},
                     {directoryPathSourceId, qmlDocumentSourceId3, ModuleId{}, FileType::QmlDocument}});
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(
                    Field(&SynchronizationPackage::imports, IsEmpty()),
                    Field(&SynchronizationPackage::types,
                          UnorderedElementsAre(
                              AllOf(IsStorageType("First.qml",
                                                  Storage::Synchronization::ImportedType{},
                                                  TypeTraits::Reference,
                                                  qmlDocumentSourceId1,
                                                  Storage::Synchronization::ChangeLevel::Minimal),
                                    Field(&Storage::Synchronization::Type::exportedTypes,
                                          UnorderedElementsAre(
                                              IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                              IsExportedType(pathModuleId, "First", -1, -1)))),
                              AllOf(IsStorageType("First2.qml",
                                                  Storage::Synchronization::ImportedType{},
                                                  TypeTraits::Reference,
                                                  qmlDocumentSourceId2,
                                                  Storage::Synchronization::ChangeLevel::Minimal),
                                    Field(&Storage::Synchronization::Type::exportedTypes,
                                          UnorderedElementsAre(
                                              IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                              IsExportedType(pathModuleId, "First2", -1, -1)))))),
                    Field(&SynchronizationPackage::updatedSourceIds,
                          UnorderedElementsAre(qmlDirPathSourceId,
                                               qmlDocumentSourceId1,
                                               qmlDocumentSourceId2,
                                               qmlDocumentSourceId3)),
                    Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                          UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId3)),
                    Field(&SynchronizationPackage::fileStatuses,
                          UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21))),
                    Field(&SynchronizationPackage::updatedProjectSourceIds,
                          UnorderedElementsAre(directoryPathSourceId)),
                    Field(&SynchronizationPackage::projectDatas,
                          UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                             qmlDocumentSourceId1,
                                                             ModuleId{},
                                                             FileType::QmlDocument),
                                               IsProjectData(directoryPathSourceId,
                                                             qmlDocumentSourceId2,
                                                             ModuleId{},
                                                             FileType::QmlDocument))))));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, synchronize_removes_qml_document_in_qmldir_only)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      )"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({qmlDirPathSourceId});
    setFilesDontChanged({qmlDocumentSourceId1, qmlDocumentSourceId2});
    setProjectDatas(directoryPathSourceId,
                    {{directoryPathSourceId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                     {directoryPathSourceId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument}});
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports, IsEmpty()),
            Field(&SynchronizationPackage::types,
                  UnorderedElementsAre(
                      AllOf(IsStorageType("First.qml",
                                          Storage::Synchronization::ImportedType{},
                                          TypeTraits::Reference,
                                          qmlDocumentSourceId1,
                                          Storage::Synchronization::ChangeLevel::Minimal),
                            Field(&Storage::Synchronization::Type::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                       IsExportedType(pathModuleId, "First", -1, -1)))),
                      AllOf(IsStorageType("First2.qml",
                                          Storage::Synchronization::ImportedType{},
                                          TypeTraits::Reference,
                                          qmlDocumentSourceId2,
                                          Storage::Synchronization::ChangeLevel::Minimal),
                            Field(&Storage::Synchronization::Type::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(pathModuleId, "First2", -1, -1)))))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21))),
            Field(&SynchronizationPackage::updatedProjectSourceIds,
                  UnorderedElementsAre(directoryPathSourceId)),
            Field(&SynchronizationPackage::projectDatas,
                  UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId1,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId2,
                                                     ModuleId{},
                                                     FileType::QmlDocument))))));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, synchronize_add_qml_document_to_qmldir)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      )"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({qmlDirPathSourceId});
    setFilesDontChanged({qmlDocumentSourceId1, qmlDocumentSourceId2});
    setProjectDatas(directoryPathSourceId,
                    {{directoryPathSourceId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                     {directoryPathSourceId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument}});
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports, IsEmpty()),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId1,
                                        Storage::Synchronization::ChangeLevel::Minimal),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId2,
                                        Storage::Synchronization::ChangeLevel::Minimal),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21))),
            Field(&SynchronizationPackage::updatedProjectSourceIds,
                  UnorderedElementsAre(directoryPathSourceId)),
            Field(&SynchronizationPackage::projectDatas,
                  UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId1,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId2,
                                                     ModuleId{},
                                                     FileType::QmlDocument))))));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, synchronize_remove_qml_document_from_qmldir)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      )"};
    setContent(u"/path/qmldir", qmldir);
    setFilesDontChanged({qmlDocumentSourceId1, qmlDocumentSourceId2});
    setFilesChanged({qmlDirPathSourceId});
    setProjectDatas(directoryPathSourceId,
                    {{directoryPathSourceId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                     {directoryPathSourceId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument}});
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports, IsEmpty()),
            Field(&SynchronizationPackage::types,
                  UnorderedElementsAre(
                      AllOf(IsStorageType("First.qml",
                                          Storage::Synchronization::ImportedType{},
                                          TypeTraits::Reference,
                                          qmlDocumentSourceId1,
                                          Storage::Synchronization::ChangeLevel::Minimal),
                            Field(&Storage::Synchronization::Type::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                       IsExportedType(pathModuleId, "First", -1, -1)))),
                      AllOf(IsStorageType("First2.qml",
                                          Storage::Synchronization::ImportedType{},
                                          TypeTraits::Reference,
                                          qmlDocumentSourceId2,
                                          Storage::Synchronization::ChangeLevel::Minimal),
                            Field(&Storage::Synchronization::Type::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(pathModuleId, "First2", -1, -1)))))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21))),
            Field(&SynchronizationPackage::updatedProjectSourceIds,
                  UnorderedElementsAre(directoryPathSourceId)),
            Field(&SynchronizationPackage::projectDatas,
                  UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId1,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId2,
                                                     ModuleId{},
                                                     FileType::QmlDocument))))));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, synchronize_qml_documents_dont_update_if_up_to_date)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setFilesDontChanged({qmlDocumentSourceId3});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports, UnorderedElementsAre(import1, import2)),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        Storage::Synchronization::ImportedType{"Object"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId1,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        Storage::Synchronization::ImportedType{"Object2"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId2,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))),
                    AllOf(IsStorageType("Second.qml",
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId3,
                                        Storage::Synchronization::ChangeLevel::Minimal),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "SecondType", 2, 2),
                                                     IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21))),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field(&SynchronizationPackage::updatedProjectSourceIds,
                  UnorderedElementsAre(directoryPathSourceId)),
            Field(&SynchronizationPackage::projectDatas,
                  UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId1,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId2,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId3,
                                                     ModuleId{},
                                                     FileType::QmlDocument))))));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, synchroniz_if_qmldir_file_has_not_changed)
{
    setProjectDatas(
        directoryPathSourceId,
        {{directoryPathSourceId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathSourceId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathSourceId, qmlDocumentSourceId1, exampleModuleId, FileType::QmlDocument},
         {directoryPathSourceId, qmlDocumentSourceId2, exampleModuleId, FileType::QmlDocument}});
    setFilesDontChanged({qmlDirPathSourceId});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports,
                  UnorderedElementsAre(import1, import2, import4, import5)),
            Field(&SynchronizationPackage::types,
                  UnorderedElementsAre(
                      Eq(objectType),
                      Eq(itemType),
                      AllOf(IsStorageType("First.qml",
                                          Storage::Synchronization::ImportedType{"Object"},
                                          TypeTraits::Reference,
                                          qmlDocumentSourceId1,
                                          Storage::Synchronization::ChangeLevel::ExcludeExportedTypes),
                            Field(&Storage::Synchronization::Type::exportedTypes, IsEmpty())),
                      AllOf(IsStorageType("First2.qml",
                                          Storage::Synchronization::ImportedType{"Object2"},
                                          TypeTraits::Reference,
                                          qmlDocumentSourceId2,
                                          Storage::Synchronization::ChangeLevel::ExcludeExportedTypes),
                            Field(&Storage::Synchronization::Type::exportedTypes, IsEmpty())))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmltypesPathSourceId,
                                       qmltypes2PathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmltypesPathSourceId, 1, 21),
                                       IsFileStatus(qmltypes2PathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21))),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmltypesPathSourceId,
                                       qmltypes2PathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2)),
            Field(&SynchronizationPackage::projectDatas, IsEmpty()))));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, synchroniz_if_qmldir_file_has_not_changed_and_some_updated_files)
{
    setProjectDatas(
        directoryPathSourceId,
        {{directoryPathSourceId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathSourceId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathSourceId, qmlDocumentSourceId1, exampleModuleId, FileType::QmlDocument},
         {directoryPathSourceId, qmlDocumentSourceId2, exampleModuleId, FileType::QmlDocument}});
    setFilesDontChanged({qmlDirPathSourceId, qmltypes2PathSourceId, qmlDocumentSourceId2});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(
            AllOf(Field(&SynchronizationPackage::imports, UnorderedElementsAre(import1, import4)),
                  Field(&SynchronizationPackage::types,
                        UnorderedElementsAre(
                            Eq(objectType),
                            AllOf(IsStorageType("First.qml",
                                                Storage::Synchronization::ImportedType{"Object"},
                                                TypeTraits::Reference,
                                                qmlDocumentSourceId1,
                                                Storage::Synchronization::ChangeLevel::ExcludeExportedTypes),
                                  Field(&Storage::Synchronization::Type::exportedTypes, IsEmpty())))),
                  Field(&SynchronizationPackage::updatedSourceIds,
                        UnorderedElementsAre(qmltypesPathSourceId, qmlDocumentSourceId1)),
                  Field(&SynchronizationPackage::fileStatuses,
                        UnorderedElementsAre(IsFileStatus(qmltypesPathSourceId, 1, 21),
                                             IsFileStatus(qmlDocumentSourceId1, 1, 21))),
                  Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                        UnorderedElementsAre(qmltypesPathSourceId, qmlDocumentSourceId1)),
                  Field(&SynchronizationPackage::projectDatas, IsEmpty()))));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, synchroniz_if_qmldir_file_not_changed_and_some_removed_files)
{
    setQmlFileNames(u"/path", {"First2.qml"});
    setProjectDatas(
        directoryPathSourceId,
        {{directoryPathSourceId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathSourceId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathSourceId, qmlDocumentSourceId1, exampleModuleId, FileType::QmlDocument},
         {directoryPathSourceId, qmlDocumentSourceId2, exampleModuleId, FileType::QmlDocument}});
    setFilesDontChanged({qmlDirPathSourceId, qmltypes2PathSourceId, qmlDocumentSourceId2});
    setFilesRemoved({qmltypesPathSourceId, qmlDocumentSourceId1});

    ASSERT_THROW(updater.update(directories, {}, {}), QmlDesigner::CannotParseQmlTypesFile);
}

TEST_F(ProjectStorageUpdater, synchroniz_if_qmldir_file_has_changed_and_some_removed_files)
{
    QString qmldir{R"(module Example
                      FirstType 2.2 First2.qml
                      typeinfo example2.qmltypes)"};
    setContent(u"/path/qmldir", qmldir);
    setQmlFileNames(u"/path", {"First2.qml"});
    setProjectDatas(
        directoryPathSourceId,
        {{directoryPathSourceId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathSourceId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathSourceId, qmlDocumentSourceId1, exampleModuleId, FileType::QmlDocument},
         {directoryPathSourceId, qmlDocumentSourceId2, exampleModuleId, FileType::QmlDocument}});
    setFilesDontChanged({qmltypes2PathSourceId, qmlDocumentSourceId2});
    setFilesRemoved({qmltypesPathSourceId, qmlDocumentSourceId1});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports, IsEmpty()),
            Field(&SynchronizationPackage::types,
                  UnorderedElementsAre(AllOf(
                      IsStorageType("First2.qml",
                                    Storage::Synchronization::ImportedType{},
                                    TypeTraits::Reference,
                                    qmlDocumentSourceId2,
                                    Storage::Synchronization::ChangeLevel::Minimal),
                      Field(&Storage::Synchronization::Type::exportedTypes,
                            UnorderedElementsAre(IsExportedType(pathModuleId, "First2", -1, -1),
                                                 IsExportedType(exampleModuleId, "FirstType", 2, 2)))))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmltypesPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21))),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmltypesPathSourceId, qmlDocumentSourceId1)),
            Field(&SynchronizationPackage::projectDatas,
                  UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId2,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmltypes2PathSourceId,
                                                     exampleCppNativeModuleId,
                                                     FileType::QmlTypes))))));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, update_qml_types_files_is_empty)
{
    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field(&SynchronizationPackage::imports, IsEmpty()),
                          Field(&SynchronizationPackage::types, IsEmpty()),
                          Field(&SynchronizationPackage::updatedSourceIds, IsEmpty()),
                          Field(&SynchronizationPackage::fileStatuses, IsEmpty()),
                          Field(&SynchronizationPackage::updatedFileStatusSourceIds, IsEmpty()),
                          Field(&SynchronizationPackage::projectDatas, IsEmpty()),
                          Field(&SynchronizationPackage::updatedProjectSourceIds, IsEmpty()))));

    updater.update({}, {}, {});
}

TEST_F(ProjectStorageUpdater, update_qml_types_files)
{
    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(
                    Field(&SynchronizationPackage::imports, UnorderedElementsAre(import4, import5)),
                    Field(&SynchronizationPackage::types, UnorderedElementsAre(objectType, itemType)),
                    Field(&SynchronizationPackage::updatedSourceIds,
                          UnorderedElementsAre(qmltypesPathSourceId, qmltypes2PathSourceId)),
                    Field(&SynchronizationPackage::fileStatuses,
                          UnorderedElementsAre(IsFileStatus(qmltypesPathSourceId, 1, 21),
                                               IsFileStatus(qmltypes2PathSourceId, 1, 21))),
                    Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                          UnorderedElementsAre(qmltypesPathSourceId, qmltypes2PathSourceId)),
                    Field(&SynchronizationPackage::projectDatas,
                          UnorderedElementsAre(IsProjectData(qmltypesPathSourceId,
                                                             qmltypesPathSourceId,
                                                             builtinCppNativeModuleId,
                                                             FileType::QmlTypes),
                                               IsProjectData(qmltypes2PathSourceId,
                                                             qmltypes2PathSourceId,
                                                             builtinCppNativeModuleId,
                                                             FileType::QmlTypes))),
                    Field(&SynchronizationPackage::updatedProjectSourceIds,
                          UnorderedElementsAre(qmltypesPathSourceId, qmltypes2PathSourceId)))));

    updater.update({}, {"/path/example.qmltypes", "/path/example2.qmltypes"}, {});
}

TEST_F(ProjectStorageUpdater, dont_update_qml_types_files_if_unchanged)
{
    setFilesDontChanged({qmltypes2PathSourceId});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field(&SynchronizationPackage::imports, UnorderedElementsAre(import4)),
                          Field(&SynchronizationPackage::types, UnorderedElementsAre(objectType)),
                          Field(&SynchronizationPackage::updatedSourceIds,
                                UnorderedElementsAre(qmltypesPathSourceId)),
                          Field(&SynchronizationPackage::fileStatuses,
                                UnorderedElementsAre(IsFileStatus(qmltypesPathSourceId, 1, 21))),
                          Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                                UnorderedElementsAre(qmltypesPathSourceId)),
                          Field(&SynchronizationPackage::projectDatas,
                                UnorderedElementsAre(IsProjectData(qmltypesPathSourceId,
                                                                   qmltypesPathSourceId,
                                                                   builtinCppNativeModuleId,
                                                                   FileType::QmlTypes))),
                          Field(&SynchronizationPackage::updatedProjectSourceIds,
                                UnorderedElementsAre(qmltypesPathSourceId)))));

    updater.update({}, {"/path/example.qmltypes", "/path/example2.qmltypes"}, {});
}

TEST_F(ProjectStorageUpdater, synchronize_qml_documents_with_different_version_but_same_type_name_and_file_name)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 1.1 First.qml
                      FirstType 6.0 First.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setQmlFileNames(u"/path", {"First.qml"});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field(&SynchronizationPackage::imports, UnorderedElementsAre(import1)),
                          Field(&SynchronizationPackage::types,
                                UnorderedElementsAre(AllOf(
                                    IsStorageType("First.qml",
                                                  Storage::Synchronization::ImportedType{"Object"},
                                                  TypeTraits::Reference,
                                                  qmlDocumentSourceId1,
                                                  Storage::Synchronization::ChangeLevel::Full),
                                    Field(&Storage::Synchronization::Type::exportedTypes,
                                          UnorderedElementsAre(
                                              IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                              IsExportedType(exampleModuleId, "FirstType", 1, 1),
                                              IsExportedType(exampleModuleId, "FirstType", 6, 0),
                                              IsExportedType(pathModuleId, "First", -1, -1)))))),
                          Field(&SynchronizationPackage::updatedSourceIds,
                                UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1)),
                          Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                                UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1)),
                          Field(&SynchronizationPackage::fileStatuses,
                                UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21),
                                                     IsFileStatus(qmlDocumentSourceId1, 1, 21))),
                          Field(&SynchronizationPackage::updatedProjectSourceIds,
                                UnorderedElementsAre(directoryPathSourceId)),
                          Field(&SynchronizationPackage::projectDatas,
                                UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                                   qmlDocumentSourceId1,
                                                                   ModuleId{},
                                                                   FileType::QmlDocument))))));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, synchronize_qml_documents_with_different_type_name_but_same_version_and_file_name)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType2 1.0 First.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setQmlFileNames(u"/path", {"First.qml"});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field(&SynchronizationPackage::imports, UnorderedElementsAre(import1)),
                          Field(&SynchronizationPackage::types,
                                UnorderedElementsAre(AllOf(
                                    IsStorageType("First.qml",
                                                  Storage::Synchronization::ImportedType{"Object"},
                                                  TypeTraits::Reference,
                                                  qmlDocumentSourceId1,
                                                  Storage::Synchronization::ChangeLevel::Full),
                                    Field(&Storage::Synchronization::Type::exportedTypes,
                                          UnorderedElementsAre(
                                              IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                              IsExportedType(exampleModuleId, "FirstType2", 1, 0),
                                              IsExportedType(pathModuleId, "First", -1, -1)))))),
                          Field(&SynchronizationPackage::updatedSourceIds,
                                UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1)),
                          Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                                UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1)),
                          Field(&SynchronizationPackage::fileStatuses,
                                UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21),
                                                     IsFileStatus(qmlDocumentSourceId1, 1, 21))),
                          Field(&SynchronizationPackage::updatedProjectSourceIds,
                                UnorderedElementsAre(directoryPathSourceId)),
                          Field(&SynchronizationPackage::projectDatas,
                                UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                                   qmlDocumentSourceId1,
                                                                   ModuleId{},
                                                                   FileType::QmlDocument))))));

    updater.update(directories, {}, {});
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
                    Contains(Field(&Storage::Synchronization::Type::exportedTypes,
                                   Contains(IsExportedType(exampleModuleId, "FirstType", 1, 0))))))));

    updater.update(directories, {}, {});
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
            Field(&SynchronizationPackage::moduleDependencies,
                  UnorderedElementsAre(
                      Import{qmlCppNativeModuleId, Storage::Version{}, qmltypesPathSourceId},
                      Import{builtinCppNativeModuleId, Storage::Version{}, qmltypesPathSourceId},
                      Import{qmlCppNativeModuleId, Storage::Version{}, qmltypes2PathSourceId},
                      Import{builtinCppNativeModuleId, Storage::Version{}, qmltypes2PathSourceId})),
            Field(&SynchronizationPackage::updatedModuleDependencySourceIds,
                  UnorderedElementsAre(qmltypesPathSourceId, qmltypes2PathSourceId)))));

    updater.update(directories, {}, {});
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
            Field(&SynchronizationPackage::moduleDependencies,
                  UnorderedElementsAre(
                      Import{qmlCppNativeModuleId, Storage::Version{}, qmltypesPathSourceId},
                      Import{builtinCppNativeModuleId, Storage::Version{}, qmltypesPathSourceId},
                      Import{qmlCppNativeModuleId, Storage::Version{}, qmltypes2PathSourceId},
                      Import{builtinCppNativeModuleId, Storage::Version{}, qmltypes2PathSourceId})),
            Field(&SynchronizationPackage::updatedModuleDependencySourceIds,
                  UnorderedElementsAre(qmltypesPathSourceId, qmltypes2PathSourceId)))));

    updater.update(directories, {}, {});
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
            Field(&SynchronizationPackage::moduleDependencies,
                  UnorderedElementsAre(
                      Import{qmlCppNativeModuleId, Storage::Version{}, qmltypesPathSourceId},
                      Import{builtinCppNativeModuleId, Storage::Version{}, qmltypesPathSourceId},
                      Import{qmlCppNativeModuleId, Storage::Version{}, qmltypes2PathSourceId},
                      Import{builtinCppNativeModuleId, Storage::Version{}, qmltypes2PathSourceId})),
            Field(&SynchronizationPackage::updatedModuleDependencySourceIds,
                  UnorderedElementsAre(qmltypesPathSourceId, qmltypes2PathSourceId)))));

    updater.update(directories, {}, {});
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
                    AllOf(Field(&SynchronizationPackage::moduleDependencies, IsEmpty()),
                          Field(&SynchronizationPackage::updatedModuleDependencySourceIds,
                                UnorderedElementsAre(qmltypesPathSourceId, qmltypes2PathSourceId)))));

    updater.update(directories, {}, {});
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
                    AllOf(Field(&SynchronizationPackage::moduleExportedImports,
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
                                                                          IsAutoVersion::No},
                                                     ModuleExportedImport{exampleCppNativeModuleId,
                                                                          quickCppNativeModuleId,
                                                                          Storage::Version{},
                                                                          IsAutoVersion::No})),
                          Field(&SynchronizationPackage::updatedModuleIds,
                                ElementsAre(exampleModuleId)))));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, synchronize_qmldir_with_no_imports)
{
    QString qmldir{R"(module Example
                      )"};
    setContent(u"/path/qmldir", qmldir);

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field(&SynchronizationPackage::moduleExportedImports, IsEmpty()),
                                  Field(&SynchronizationPackage::updatedModuleIds,
                                        ElementsAre(exampleModuleId)))));

    updater.update(directories, {}, {});
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
                    AllOf(Field(&SynchronizationPackage::moduleExportedImports,
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
                                                                          IsAutoVersion::No},
                                                     ModuleExportedImport{exampleCppNativeModuleId,
                                                                          quickCppNativeModuleId,
                                                                          Storage::Version{},
                                                                          IsAutoVersion::No})),
                          Field(&SynchronizationPackage::updatedModuleIds,
                                ElementsAre(exampleModuleId)))));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, synchronize_qmldir_optional_imports)
{
    QString qmldir{R"(module Example
                      import Qml auto
                      import QML 2.1
                      optional import Quick
                      )"};
    setContent(u"/path/qmldir", qmldir);

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field(&SynchronizationPackage::moduleExportedImports,
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
                                                                          IsAutoVersion::No},
                                                     ModuleExportedImport{exampleCppNativeModuleId,
                                                                          quickCppNativeModuleId,
                                                                          Storage::Version{},
                                                                          IsAutoVersion::No})),
                          Field(&SynchronizationPackage::updatedModuleIds,
                                ElementsAre(exampleModuleId)))));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_directories)
{
    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{projectPartId,
                                               QmlDesigner::SourceType::Directory,
                                               {path1SourceId, path2SourceId, path3SourceId}})));

    updater.update(directories3, {}, {});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_directory_does_not_exists)
{
    setFilesDontExists({path2SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{projectPartId,
                                               QmlDesigner::SourceType::Directory,
                                               {path1SourceId, path3SourceId}})));

    updater.update(directories3, {}, {});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_directory_does_not_changed)
{
    setFilesDontChanged({qmldir1SourceId, qmldir2SourceId, path1SourceId, path2SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{projectPartId,
                                               QmlDesigner::SourceType::Directory,
                                               {path1SourceId, path2SourceId}})));

    updater.update(directories2, {}, {});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_directory_removed)
{
    setFilesRemoved({qmldir1SourceId, path1SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(
                    IdPaths{projectPartId, QmlDesigner::SourceType::Directory, {path2SourceId}})));

    updater.update(directories2, {}, {});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_qmldirs)
{
    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{projectPartId,
                                               QmlDesigner::SourceType::QmlDir,
                                               {qmldir1SourceId, qmldir2SourceId, qmldir3SourceId}})));

    updater.update(directories3, {}, {});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_qmldir_does_not_exists)
{
    setFilesDontExists({qmldir2SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{projectPartId,
                                               QmlDesigner::SourceType::QmlDir,
                                               {qmldir1SourceId, qmldir3SourceId}})));

    updater.update(directories3, {}, {});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_qmldir_does_not_changed)
{
    setFilesDontChanged({qmldir1SourceId, qmldir2SourceId, path1SourceId, path2SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{projectPartId,
                                               QmlDesigner::SourceType::QmlDir,
                                               {qmldir1SourceId, qmldir2SourceId}})));

    updater.update(directories2, {}, {});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_qmldir_removed)
{
    setFilesRemoved({qmldir1SourceId, path1SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(
                    IdPaths{projectPartId, QmlDesigner::SourceType::QmlDir, {qmldir2SourceId}})));

    updater.update(directories2, {}, {});
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
                updateIdPaths(Contains(IdPaths{projectPartId,
                                               QmlDesigner::SourceType::Qml,
                                               {firstSourceId, secondSourceId, thirdSourceId}})));

    updater.update(directories2, {}, {});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_only_qml_files_dont_changed)
{
    QString qmldir1{R"(module Example
                      FirstType 1.0 First.qml
                      Second 1.0 Second.qml)"};
    setQmlFileNames(u"/path/one", {"First.qml", "Second.qml"});
    setQmlFileNames(u"/path/two", {"Third.qml"});
    setContent(u"/path/one/qmldir", qmldir1);
    setFilesDontChanged({firstSourceId, secondSourceId, thirdSourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{projectPartId,
                                               QmlDesigner::SourceType::Qml,
                                               {firstSourceId, secondSourceId, thirdSourceId}})));

    updater.update(directories2, {}, {});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_only_qml_files_changed)
{
    setFilesDontChanged({qmldir1SourceId, qmldir2SourceId, path1SourceId, path2SourceId});
    setFilesChanged({firstSourceId, secondSourceId, thirdSourceId});
    setProjectDatas(path1SourceId,
                    {{path1SourceId, firstSourceId, exampleModuleId, FileType::QmlDocument},
                     {path1SourceId, secondSourceId, exampleModuleId, FileType::QmlDocument}});
    setProjectDatas(path2SourceId,
                    {{path2SourceId, thirdSourceId, ModuleId{}, FileType::QmlDocument}});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{projectPartId,
                                               QmlDesigner::SourceType::Qml,
                                               {firstSourceId, secondSourceId, thirdSourceId}})));

    updater.update(directories2, {}, {});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_qml_files_and_directories_dont_changed)
{
    setFilesDontChanged({qmldir1SourceId,
                         qmldir2SourceId,
                         path1SourceId,
                         path2SourceId,
                         firstSourceId,
                         secondSourceId,
                         thirdSourceId});
    setProjectDatas(path1SourceId,
                    {{path1SourceId, firstSourceId, exampleModuleId, FileType::QmlDocument},
                     {path1SourceId, secondSourceId, exampleModuleId, FileType::QmlDocument}});
    setProjectDatas(path2SourceId,
                    {{path2SourceId, thirdSourceId, ModuleId{}, FileType::QmlDocument}});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{projectPartId,
                                               QmlDesigner::SourceType::Qml,
                                               {firstSourceId, secondSourceId, thirdSourceId}})));

    updater.update(directories2, {}, {});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_qmltypes_files_in_qmldir)
{
    QString qmldir1{R"(module Example
                      typeinfo example.qmltypes)"};
    QString qmldir2{R"(module Example2
                      typeinfo example2.qmltypes)"};
    setContent(u"/path/one/qmldir", qmldir1);
    setContent(u"/path/two/qmldir", qmldir2);

    setFilesDontChanged({firstSourceId, secondSourceId, thirdSourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{projectPartId,
                                               QmlDesigner::SourceType::QmlTypes,
                                               {qmltypes1SourceId, qmltypes2SourceId}})));

    updater.update(directories2, {}, {});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_only_qmltypes_files_in_qmldir_dont_changed)
{
    QString qmldir1{R"(module Example
                      typeinfo example.qmltypes)"};
    QString qmldir2{R"(module Example2
                      typeinfo example2.qmltypes)"};
    setContent(u"/path/one/qmldir", qmldir1);
    setContent(u"/path/two/qmldir", qmldir2);
    setFilesDontChanged({qmltypes1SourceId, qmltypes2SourceId});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{projectPartId,
                                               QmlDesigner::SourceType::QmlTypes,
                                               {qmltypes1SourceId, qmltypes2SourceId}})));

    updater.update(directories2, {}, {});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_only_qmltypes_files_changed)
{
    setFilesDontChanged({qmldir1SourceId, qmldir2SourceId, path1SourceId, path2SourceId});
    setFilesChanged({qmltypes1SourceId, qmltypes2SourceId});
    setProjectDatas(path1SourceId,
                    {{path1SourceId, qmltypes1SourceId, exampleModuleId, FileType::QmlTypes}});
    setProjectDatas(path2SourceId,
                    {{path2SourceId, qmltypes2SourceId, exampleModuleId, FileType::QmlTypes}});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{projectPartId,
                                               QmlDesigner::SourceType::QmlTypes,
                                               {qmltypes1SourceId, qmltypes2SourceId}})));

    updater.update(directories2, {}, {});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_qmltypes_files_and_directories_dont_changed)
{
    setFilesDontChanged({qmldir1SourceId,
                         qmldir2SourceId,
                         path1SourceId,
                         path2SourceId,
                         qmltypes1SourceId,
                         qmltypes2SourceId});
    setProjectDatas(path1SourceId,
                    {{path1SourceId, qmltypes1SourceId, exampleModuleId, FileType::QmlTypes}});
    setProjectDatas(path2SourceId,
                    {{path2SourceId, qmltypes2SourceId, exampleModuleId, FileType::QmlTypes}});

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{projectPartId,
                                               QmlDesigner::SourceType::QmlTypes,
                                               {qmltypes1SourceId, qmltypes2SourceId}})));

    updater.update(directories2, {}, {});
}

TEST_F(ProjectStorageUpdater, update_path_watcher_builtin_qmltypes_files)
{
    QString builtinQmltyplesPath1{"/path/one/example.qmltypes"};
    QString builtinQmltyplesPath2{"/path/two/example2.qmltypes"};
    setContent(builtinQmltyplesPath1, qmltypes1);
    setContent(builtinQmltyplesPath2, qmltypes2);

    EXPECT_CALL(patchWatcherMock,
                updateIdPaths(Contains(IdPaths{projectPartId,
                                               QmlDesigner::SourceType::QmlTypes,
                                               {qmltypes1SourceId, qmltypes2SourceId}})));

    updater.update({}, {builtinQmltyplesPath1, builtinQmltyplesPath2}, {});
}

TEST_F(ProjectStorageUpdater, synchronize_qml_documents_without_qmldir)
{
    setFilesDontExists({qmlDirPathSourceId});
    setFilesChanged({directoryPathSourceId});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports, UnorderedElementsAre(import1, import2, import3)),
            Field(&SynchronizationPackage::types,
                  UnorderedElementsAre(
                      AllOf(IsStorageType("First.qml",
                                          Storage::Synchronization::ImportedType{"Object"},
                                          TypeTraits::Reference,
                                          qmlDocumentSourceId1,
                                          Storage::Synchronization::ChangeLevel::Full),
                            Field(&Storage::Synchronization::Type::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(pathModuleId, "First", -1, -1)))),
                      AllOf(IsStorageType("First2.qml",
                                          Storage::Synchronization::ImportedType{"Object2"},
                                          TypeTraits::Reference,
                                          qmlDocumentSourceId2,
                                          Storage::Synchronization::ChangeLevel::Full),
                            Field(&Storage::Synchronization::Type::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(pathModuleId, "First2", -1, -1)))),
                      AllOf(IsStorageType("Second.qml",
                                          Storage::Synchronization::ImportedType{"Object3"},
                                          TypeTraits::Reference,
                                          qmlDocumentSourceId3,
                                          Storage::Synchronization::ChangeLevel::Full),
                            Field(&Storage::Synchronization::Type::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(directoryPathSourceId,
                                       qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(directoryPathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId3, 1, 21))),
            Field(&SynchronizationPackage::updatedProjectSourceIds,
                  UnorderedElementsAre(directoryPathSourceId)),
            Field(&SynchronizationPackage::projectDatas,
                  UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId1,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId2,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId3,
                                                     ModuleId{},
                                                     FileType::QmlDocument))))));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, synchronize_qml_documents_without_qmldir_throws_if_qml_document_does_not_exists)
{
    setFilesDontExists({qmlDirPathSourceId, qmlDocumentSourceId1});
    setFilesAdded({directoryPathSourceId});

    ASSERT_THROW(updater.update(directories, {}, {}), QmlDesigner::CannotParseQmlDocumentFile);
}

TEST_F(ProjectStorageUpdater, synchronize_qml_documents_without_qmldir_throws_if_directory_does_not_exists)
{
    setFilesDontExists({qmlDirPathSourceId, directoryPathSourceId});
    setProjectDatas(directoryPathSourceId,
                    {{directoryPathSourceId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                     {directoryPathSourceId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument},
                     {directoryPathSourceId, qmlDocumentSourceId3, ModuleId{}, FileType::QmlDocument}});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field(&SynchronizationPackage::imports, IsEmpty()),
                                  Field(&SynchronizationPackage::types, IsEmpty()),
                                  Field(&SynchronizationPackage::updatedSourceIds,
                                        UnorderedElementsAre(qmlDirPathSourceId,
                                                             qmlDocumentSourceId1,
                                                             qmlDocumentSourceId2,
                                                             qmlDocumentSourceId3)),
                                  Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                                        UnorderedElementsAre(directoryPathSourceId,
                                                             qmlDirPathSourceId,
                                                             qmlDocumentSourceId1,
                                                             qmlDocumentSourceId2,
                                                             qmlDocumentSourceId3)),
                                  Field(&SynchronizationPackage::fileStatuses, IsEmpty()),
                                  Field(&SynchronizationPackage::updatedProjectSourceIds,
                                        UnorderedElementsAre(directoryPathSourceId)),
                                  Field(&SynchronizationPackage::projectDatas, IsEmpty()))));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, synchronize_qml_documents_without_qmldir_add_qml_document)
{
    setFilesDontExists({qmlDirPathSourceId});
    setFilesChanged({directoryPathSourceId});
    setFilesAdded({qmlDocumentSourceId3});
    setFilesDontChanged({qmlDocumentSourceId1, qmlDocumentSourceId2});
    setProjectDatas(directoryPathSourceId,
                    {{directoryPathSourceId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                     {directoryPathSourceId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument}});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports, UnorderedElementsAre(import3)),
            Field(&SynchronizationPackage::types,
                  UnorderedElementsAre(AllOf(
                      IsStorageType("Second.qml",
                                    Storage::Synchronization::ImportedType{"Object3"},
                                    TypeTraits::Reference,
                                    qmlDocumentSourceId3,
                                    Storage::Synchronization::ChangeLevel::Full),
                      Field(&Storage::Synchronization::Type::exportedTypes,
                            UnorderedElementsAre(IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId3)),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(directoryPathSourceId, qmlDirPathSourceId, qmlDocumentSourceId3)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(directoryPathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId3, 1, 21))),
            Field(&SynchronizationPackage::updatedProjectSourceIds,
                  UnorderedElementsAre(directoryPathSourceId)),
            Field(&SynchronizationPackage::projectDatas,
                  UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId1,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId2,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId3,
                                                     ModuleId{},
                                                     FileType::QmlDocument))))));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, synchronize_qml_documents_without_qmldir_removes_qml_document)
{
    setFilesDontExists({qmlDirPathSourceId});
    setFilesChanged({directoryPathSourceId});
    setFilesRemoved({qmlDocumentSourceId3});
    setFilesDontChanged({qmlDocumentSourceId1, qmlDocumentSourceId2});
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});
    setProjectDatas(directoryPathSourceId,
                    {{directoryPathSourceId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                     {directoryPathSourceId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument},
                     {directoryPathSourceId, qmlDocumentSourceId3, ModuleId{}, FileType::QmlDocument}});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field(&SynchronizationPackage::imports, IsEmpty()),
                          Field(&SynchronizationPackage::types, IsEmpty()),
                          Field(&SynchronizationPackage::updatedSourceIds,
                                UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId3)),
                          Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                                UnorderedElementsAre(directoryPathSourceId,
                                                     qmlDirPathSourceId,
                                                     qmlDocumentSourceId3)),
                          Field(&SynchronizationPackage::fileStatuses,
                                UnorderedElementsAre(IsFileStatus(directoryPathSourceId, 1, 21))),
                          Field(&SynchronizationPackage::updatedProjectSourceIds,
                                UnorderedElementsAre(directoryPathSourceId)),
                          Field(&SynchronizationPackage::projectDatas,
                                UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                                   qmlDocumentSourceId1,
                                                                   ModuleId{},
                                                                   FileType::QmlDocument),
                                                     IsProjectData(directoryPathSourceId,
                                                                   qmlDocumentSourceId2,
                                                                   ModuleId{},
                                                                   FileType::QmlDocument))))));

    updater.update(directories, {}, {});
}

TEST_F(ProjectStorageUpdater, watcher_updates_directories)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setFilesChanged({directoryPathSourceId});
    setFilesDontChanged({qmlDirPathSourceId});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports, UnorderedElementsAre(import1, import2, import3)),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        Storage::Synchronization::ImportedType{"Object"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId1,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        Storage::Synchronization::ImportedType{"Object2"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId2,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))),
                    AllOf(IsStorageType("Second.qml",
                                        Storage::Synchronization::ImportedType{"Object3"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId3,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "SecondType", 2, 2),
                                                     IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDocumentSourceId1, qmlDocumentSourceId2, qmlDocumentSourceId3)),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(directoryPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(directoryPathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId3, 1, 21))),
            Field(&SynchronizationPackage::updatedProjectSourceIds,
                  UnorderedElementsAre(directoryPathSourceId)),
            Field(&SynchronizationPackage::projectDatas,
                  UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId1,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId2,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId3,
                                                     ModuleId{},
                                                     FileType::QmlDocument))))));

    updater.pathsWithIdsChanged({{directoryProjectChunkId, {directoryPathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_removed_directory)
{
    setFilesRemoved({directoryPathSourceId,
                     qmlDirPathSourceId,
                     qmlDocumentSourceId1,
                     qmlDocumentSourceId2,
                     qmlDocumentSourceId3});
    setProjectDatas(directoryPathSourceId,
                    {{directoryPathSourceId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                     {directoryPathSourceId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument},
                     {directoryPathSourceId, qmlDocumentSourceId3, ModuleId{}, FileType::QmlDocument}});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field(&SynchronizationPackage::imports, IsEmpty()),
                                  Field(&SynchronizationPackage::types, IsEmpty()),
                                  Field(&SynchronizationPackage::updatedSourceIds,
                                        UnorderedElementsAre(qmlDirPathSourceId,
                                                             qmlDocumentSourceId1,
                                                             qmlDocumentSourceId2,
                                                             qmlDocumentSourceId3)),
                                  Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                                        UnorderedElementsAre(directoryPathSourceId,
                                                             qmlDirPathSourceId,
                                                             qmlDocumentSourceId1,
                                                             qmlDocumentSourceId2,
                                                             qmlDocumentSourceId3)),
                                  Field(&SynchronizationPackage::fileStatuses, UnorderedElementsAre()),
                                  Field(&SynchronizationPackage::updatedProjectSourceIds,
                                        UnorderedElementsAre(directoryPathSourceId)),
                                  Field(&SynchronizationPackage::projectDatas, IsEmpty()))));

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
    setFilesDontChanged({qmlDirPathSourceId});
    auto directorySourceContextId = sourcePathCache.sourceContextId(directoryPathSourceId);

    EXPECT_CALL(patchWatcherMock,
                updateContextIdPaths(
                    UnorderedElementsAre(
                        IdPaths{projectPartId, QmlDesigner::SourceType::Directory, {directoryPathSourceId}},
                        IdPaths{projectPartId, QmlDesigner::SourceType::QmlDir, {qmlDirPathSourceId}},
                        IdPaths{projectPartId,
                                QmlDesigner::SourceType::Qml,
                                {qmlDocumentSourceId1, qmlDocumentSourceId2, qmlDocumentSourceId3}},
                        IdPaths{projectPartId, QmlDesigner::SourceType::QmlTypes, {}}),
                    UnorderedElementsAre(directorySourceContextId)));

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
            Field(&SynchronizationPackage::imports, UnorderedElementsAre(import1, import2, import3)),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        Storage::Synchronization::ImportedType{"Object"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId1,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        Storage::Synchronization::ImportedType{"Object2"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId2,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))),
                    AllOf(IsStorageType("Second.qml",
                                        Storage::Synchronization::ImportedType{"Object3"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId3,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "SecondType", 2, 2),
                                                     IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(directoryPathSourceId,
                                       qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(directoryPathSourceId, 1, 21),
                                       IsFileStatus(qmlDirPathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId3, 1, 21))),
            Field(&SynchronizationPackage::updatedProjectSourceIds,
                  UnorderedElementsAre(directoryPathSourceId)),
            Field(&SynchronizationPackage::projectDatas,
                  UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId1,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId2,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId3,
                                                     ModuleId{},
                                                     FileType::QmlDocument))))));

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
    auto directorySourceContextId = sourcePathCache.sourceContextId(qmlDirPathSourceId);

    EXPECT_CALL(patchWatcherMock,
                updateContextIdPaths(
                    UnorderedElementsAre(
                        IdPaths{projectPartId, QmlDesigner::SourceType::Directory, {directoryPathSourceId}},
                        IdPaths{projectPartId, QmlDesigner::SourceType::QmlDir, {qmlDirPathSourceId}},
                        IdPaths{projectPartId,
                                QmlDesigner::SourceType::Qml,
                                {qmlDocumentSourceId1, qmlDocumentSourceId2, qmlDocumentSourceId3}},
                        IdPaths{projectPartId, QmlDesigner::SourceType::QmlTypes, {}}),
                    UnorderedElementsAre(directorySourceContextId)));

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
    setFilesDontChanged({qmlDirPathSourceId, qmlDocumentSourceId1});
    setFilesAdded({qmlDocumentSourceId2});
    setProjectDatas(directoryPathSourceId,
                    {{directoryPathSourceId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument}});
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(
                    Field(&SynchronizationPackage::imports, UnorderedElementsAre(import2)),
                    Field(&SynchronizationPackage::types,
                          UnorderedElementsAre(
                              AllOf(IsStorageType("First.qml",
                                                  Storage::Synchronization::ImportedType{},
                                                  TypeTraits::Reference,
                                                  qmlDocumentSourceId1,
                                                  Storage::Synchronization::ChangeLevel::Minimal),
                                    Field(&Storage::Synchronization::Type::exportedTypes,
                                          UnorderedElementsAre(
                                              IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                              IsExportedType(pathModuleId, "First", -1, -1)))),
                              AllOf(IsStorageType("First2.qml",
                                                  Storage::Synchronization::ImportedType{"Object2"},
                                                  TypeTraits::Reference,
                                                  qmlDocumentSourceId2,
                                                  Storage::Synchronization::ChangeLevel::Full),
                                    Field(&Storage::Synchronization::Type::exportedTypes,
                                          UnorderedElementsAre(
                                              IsExportedType(pathModuleId, "First2", -1, -1)))))),
                    Field(&SynchronizationPackage::updatedSourceIds,
                          UnorderedElementsAre(qmlDocumentSourceId1, qmlDocumentSourceId2)),
                    Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                          UnorderedElementsAre(directoryPathSourceId, qmlDocumentSourceId2)),
                    Field(&SynchronizationPackage::fileStatuses,
                          UnorderedElementsAre(IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                               IsFileStatus(directoryPathSourceId, 1, 21))),
                    Field(&SynchronizationPackage::updatedProjectSourceIds,
                          UnorderedElementsAre(directoryPathSourceId)),
                    Field(&SynchronizationPackage::projectDatas,
                          UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                             qmlDocumentSourceId1,
                                                             ModuleId{},
                                                             FileType::QmlDocument),
                                               IsProjectData(directoryPathSourceId,
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
    setFilesDontChanged({qmlDocumentSourceId1, qmlDocumentSourceId2});
    setFilesRemoved({qmlDocumentSourceId3});
    setProjectDatas(directoryPathSourceId,
                    {{directoryPathSourceId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                     {directoryPathSourceId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument},
                     {directoryPathSourceId, qmlDocumentSourceId3, ModuleId{}, FileType::QmlDocument}});
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(
                    Field(&SynchronizationPackage::imports, IsEmpty()),
                    Field(&SynchronizationPackage::types,
                          UnorderedElementsAre(
                              AllOf(IsStorageType("First.qml",
                                                  Storage::Synchronization::ImportedType{},
                                                  TypeTraits::Reference,
                                                  qmlDocumentSourceId1,
                                                  Storage::Synchronization::ChangeLevel::Minimal),
                                    Field(&Storage::Synchronization::Type::exportedTypes,
                                          UnorderedElementsAre(
                                              IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                              IsExportedType(pathModuleId, "First", -1, -1)))),
                              AllOf(IsStorageType("First2.qml",
                                                  Storage::Synchronization::ImportedType{},
                                                  TypeTraits::Reference,
                                                  qmlDocumentSourceId2,
                                                  Storage::Synchronization::ChangeLevel::Minimal),
                                    Field(&Storage::Synchronization::Type::exportedTypes,
                                          UnorderedElementsAre(
                                              IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                              IsExportedType(pathModuleId, "First2", -1, -1)))))),
                    Field(&SynchronizationPackage::updatedSourceIds,
                          UnorderedElementsAre(qmlDirPathSourceId,
                                               qmlDocumentSourceId1,
                                               qmlDocumentSourceId2,
                                               qmlDocumentSourceId3)),
                    Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                          UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId3)),
                    Field(&SynchronizationPackage::fileStatuses,
                          UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21))),
                    Field(&SynchronizationPackage::updatedProjectSourceIds,
                          UnorderedElementsAre(directoryPathSourceId)),
                    Field(&SynchronizationPackage::projectDatas,
                          UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                             qmlDocumentSourceId1,
                                                             ModuleId{},
                                                             FileType::QmlDocument),
                                               IsProjectData(directoryPathSourceId,
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
    setFilesDontChanged({qmlDocumentSourceId1, qmlDocumentSourceId2});
    setProjectDatas(directoryPathSourceId,
                    {{directoryPathSourceId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                     {directoryPathSourceId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument}});
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports, IsEmpty()),
            Field(&SynchronizationPackage::types,
                  UnorderedElementsAre(
                      AllOf(IsStorageType("First.qml",
                                          Storage::Synchronization::ImportedType{},
                                          TypeTraits::Reference,
                                          qmlDocumentSourceId1,
                                          Storage::Synchronization::ChangeLevel::Minimal),
                            Field(&Storage::Synchronization::Type::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                       IsExportedType(pathModuleId, "First", -1, -1)))),
                      AllOf(IsStorageType("First2.qml",
                                          Storage::Synchronization::ImportedType{},
                                          TypeTraits::Reference,
                                          qmlDocumentSourceId2,
                                          Storage::Synchronization::ChangeLevel::Minimal),
                            Field(&Storage::Synchronization::Type::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(pathModuleId, "First2", -1, -1)))))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21))),
            Field(&SynchronizationPackage::updatedProjectSourceIds,
                  UnorderedElementsAre(directoryPathSourceId)),
            Field(&SynchronizationPackage::projectDatas,
                  UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId1,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
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
    setFilesDontChanged({qmlDocumentSourceId1, qmlDocumentSourceId2});
    setProjectDatas(directoryPathSourceId,
                    {{directoryPathSourceId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                     {directoryPathSourceId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument}});
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports, IsEmpty()),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId1,
                                        Storage::Synchronization::ChangeLevel::Minimal),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId2,
                                        Storage::Synchronization::ChangeLevel::Minimal),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21))),
            Field(&SynchronizationPackage::updatedProjectSourceIds,
                  UnorderedElementsAre(directoryPathSourceId)),
            Field(&SynchronizationPackage::projectDatas,
                  UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId1,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
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
    setFilesDontChanged({qmlDocumentSourceId1, qmlDocumentSourceId2});
    setFilesChanged({qmlDirPathSourceId});
    setProjectDatas(directoryPathSourceId,
                    {{directoryPathSourceId, qmlDocumentSourceId1, ModuleId{}, FileType::QmlDocument},
                     {directoryPathSourceId, qmlDocumentSourceId2, ModuleId{}, FileType::QmlDocument}});
    setQmlFileNames(u"/path", {"First.qml", "First2.qml"});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports, IsEmpty()),
            Field(&SynchronizationPackage::types,
                  UnorderedElementsAre(
                      AllOf(IsStorageType("First.qml",
                                          Storage::Synchronization::ImportedType{},
                                          TypeTraits::Reference,
                                          qmlDocumentSourceId1,
                                          Storage::Synchronization::ChangeLevel::Minimal),
                            Field(&Storage::Synchronization::Type::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                       IsExportedType(pathModuleId, "First", -1, -1)))),
                      AllOf(IsStorageType("First2.qml",
                                          Storage::Synchronization::ImportedType{},
                                          TypeTraits::Reference,
                                          qmlDocumentSourceId2,
                                          Storage::Synchronization::ChangeLevel::Minimal),
                            Field(&Storage::Synchronization::Type::exportedTypes,
                                  UnorderedElementsAre(IsExportedType(pathModuleId, "First2", -1, -1)))))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21))),
            Field(&SynchronizationPackage::updatedProjectSourceIds,
                  UnorderedElementsAre(directoryPathSourceId)),
            Field(&SynchronizationPackage::projectDatas,
                  UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId1,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
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
    setFilesDontChanged({qmlDocumentSourceId3});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports, UnorderedElementsAre(import1, import2)),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        Storage::Synchronization::ImportedType{"Object"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId1,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        Storage::Synchronization::ImportedType{"Object2"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId2,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))),
                    AllOf(IsStorageType("Second.qml",
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId3,
                                        Storage::Synchronization::ChangeLevel::Minimal),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "SecondType", 2, 2),
                                                     IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21))),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field(&SynchronizationPackage::updatedProjectSourceIds,
                  UnorderedElementsAre(directoryPathSourceId)),
            Field(&SynchronizationPackage::projectDatas,
                  UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId1,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId2,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId3,
                                                     ModuleId{},
                                                     FileType::QmlDocument))))));

    updater.pathsWithIdsChanged({{directoryProjectChunkId, {directoryPathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_qmldirs_dont_update_qml_documents_if_up_to_date)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setFilesDontChanged({qmlDocumentSourceId3});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports, UnorderedElementsAre(import1, import2)),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        Storage::Synchronization::ImportedType{"Object"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId1,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        Storage::Synchronization::ImportedType{"Object2"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId2,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))),
                    AllOf(IsStorageType("Second.qml",
                                        Storage::Synchronization::ImportedType{},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId3,
                                        Storage::Synchronization::ChangeLevel::Minimal),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "SecondType", 2, 2),
                                                     IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21))),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field(&SynchronizationPackage::updatedProjectSourceIds,
                  UnorderedElementsAre(directoryPathSourceId)),
            Field(&SynchronizationPackage::projectDatas,
                  UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId1,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId2,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId3,
                                                     ModuleId{},
                                                     FileType::QmlDocument))))));

    updater.pathsWithIdsChanged({{qmldirProjectChunkId, {qmlDirPathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_directory_but_not_qmldir)
{
    setProjectDatas(
        directoryPathSourceId,
        {{directoryPathSourceId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathSourceId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathSourceId, qmlDocumentSourceId1, exampleModuleId, FileType::QmlDocument},
         {directoryPathSourceId, qmlDocumentSourceId2, exampleModuleId, FileType::QmlDocument}});
    setFilesDontChanged({qmlDirPathSourceId});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports,
                  UnorderedElementsAre(import1, import2, import4, import5)),
            Field(&SynchronizationPackage::types,
                  UnorderedElementsAre(
                      Eq(objectType),
                      Eq(itemType),
                      AllOf(IsStorageType("First.qml",
                                          Storage::Synchronization::ImportedType{"Object"},
                                          TypeTraits::Reference,
                                          qmlDocumentSourceId1,
                                          Storage::Synchronization::ChangeLevel::ExcludeExportedTypes),
                            Field(&Storage::Synchronization::Type::exportedTypes, IsEmpty())),
                      AllOf(IsStorageType("First2.qml",
                                          Storage::Synchronization::ImportedType{"Object2"},
                                          TypeTraits::Reference,
                                          qmlDocumentSourceId2,
                                          Storage::Synchronization::ChangeLevel::ExcludeExportedTypes),
                            Field(&Storage::Synchronization::Type::exportedTypes, IsEmpty())))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmltypesPathSourceId,
                                       qmltypes2PathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmltypesPathSourceId, 1, 21),
                                       IsFileStatus(qmltypes2PathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21))),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmltypesPathSourceId,
                                       qmltypes2PathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2)),
            Field(&SynchronizationPackage::projectDatas, IsEmpty()))));

    updater.pathsWithIdsChanged({{directoryProjectChunkId, {directoryPathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_qml_documents)
{
    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports, UnorderedElementsAre(import1, import2)),
            Field(&SynchronizationPackage::types,
                  UnorderedElementsAre(
                      AllOf(IsStorageType("First.qml",
                                          Storage::Synchronization::ImportedType{"Object"},
                                          TypeTraits::Reference,
                                          qmlDocumentSourceId1,
                                          Storage::Synchronization::ChangeLevel::ExcludeExportedTypes),
                            Field(&Storage::Synchronization::Type::exportedTypes, IsEmpty())),
                      AllOf(IsStorageType("First2.qml",
                                          Storage::Synchronization::ImportedType{"Object2"},
                                          TypeTraits::Reference,
                                          qmlDocumentSourceId2,
                                          Storage::Synchronization::ChangeLevel::ExcludeExportedTypes),
                            Field(&Storage::Synchronization::Type::exportedTypes, IsEmpty())))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21))),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field(&SynchronizationPackage::projectDatas, IsEmpty()))));

    updater.pathsWithIdsChanged(
        {{qmlDocumentProjectChunkId, {qmlDocumentSourceId1, qmlDocumentSourceId2}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_removed_qml_documents)
{
    setFilesRemoved({qmlDocumentSourceId2});

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(
                    Field(&SynchronizationPackage::imports, UnorderedElementsAre(import1)),
                    Field(&SynchronizationPackage::types,
                          UnorderedElementsAre(AllOf(
                              IsStorageType("First.qml",
                                            Storage::Synchronization::ImportedType{"Object"},
                                            TypeTraits::Reference,
                                            qmlDocumentSourceId1,
                                            Storage::Synchronization::ChangeLevel::ExcludeExportedTypes),
                              Field(&Storage::Synchronization::Type::exportedTypes, IsEmpty())))),
                    Field(&SynchronizationPackage::updatedSourceIds,
                          UnorderedElementsAre(qmlDocumentSourceId1, qmlDocumentSourceId2)),
                    Field(&SynchronizationPackage::fileStatuses,
                          UnorderedElementsAre(IsFileStatus(qmlDocumentSourceId1, 1, 21))),
                    Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                          UnorderedElementsAre(qmlDocumentSourceId1, qmlDocumentSourceId2)),
                    Field(&SynchronizationPackage::projectDatas, IsEmpty()))));

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
    setProjectDatas(
        directoryPathSourceId,
        {{directoryPathSourceId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathSourceId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes}});
    setFilesDontChanged(
        {directoryPathSourceId, qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2});

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field(&SynchronizationPackage::imports,
                                UnorderedElementsAre(import4, import5)),
                          Field(&SynchronizationPackage::types,
                                UnorderedElementsAre(Eq(objectType), Eq(itemType))),
                          Field(&SynchronizationPackage::updatedSourceIds,
                                UnorderedElementsAre(qmltypesPathSourceId, qmltypes2PathSourceId)),
                          Field(&SynchronizationPackage::fileStatuses,
                                UnorderedElementsAre(IsFileStatus(qmltypesPathSourceId, 1, 21),
                                                     IsFileStatus(qmltypes2PathSourceId, 1, 21))),
                          Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                                UnorderedElementsAre(qmltypesPathSourceId, qmltypes2PathSourceId)),
                          Field(&SynchronizationPackage::projectDatas, IsEmpty()))));

    updater.pathsWithIdsChanged(
        {{qmltypesProjectChunkId, {qmltypesPathSourceId, qmltypes2PathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_removed_qmltypes_without_updated_qmldir)
{
    setProjectDatas(
        directoryPathSourceId,
        {{directoryPathSourceId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathSourceId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes}});
    setFilesDontChanged(
        {directoryPathSourceId, qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2});
    setFilesRemoved({qmltypesPathSourceId});

    EXPECT_CALL(projectStorageMock, synchronize(_)).Times(0);

    updater.pathsWithIdsChanged(
        {{qmltypesProjectChunkId, {qmltypesPathSourceId, qmltypes2PathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_removed_qmltypes_with_updated_qmldir)
{
    setProjectDatas(
        directoryPathSourceId,
        {{directoryPathSourceId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathSourceId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes}});
    setFilesDontChanged(
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
                    AllOf(Field(&SynchronizationPackage::imports, UnorderedElementsAre(import5)),
                          Field(&SynchronizationPackage::types, UnorderedElementsAre(Eq(itemType))),
                          Field(&SynchronizationPackage::updatedSourceIds,
                                UnorderedElementsAre(qmlDirPathSourceId,
                                                     qmltypesPathSourceId,
                                                     qmltypes2PathSourceId)),
                          Field(&SynchronizationPackage::fileStatuses,
                                UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21),
                                                     IsFileStatus(qmltypes2PathSourceId, 1, 21))),
                          Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                                UnorderedElementsAre(qmlDirPathSourceId,
                                                     qmltypesPathSourceId,
                                                     qmltypes2PathSourceId)),
                          Field(&SynchronizationPackage::projectDatas,
                                UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                                   qmltypes2PathSourceId,
                                                                   exampleCppNativeModuleId,
                                                                   FileType::QmlTypes))))));

    updater.pathsWithIdsChanged({{qmldirProjectChunkId, {qmlDirPathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_dont_watches_directories_after_qmltypes_changes)
{
    setProjectDatas(
        directoryPathSourceId,
        {{directoryPathSourceId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathSourceId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes}});
    setFilesDontChanged(
        {directoryPathSourceId, qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2});

    EXPECT_CALL(patchWatcherMock, updateContextIdPaths(_, _)).Times(0);

    updater.pathsWithIdsChanged(
        {{qmltypesProjectChunkId, {qmltypesPathSourceId, qmltypes2PathSourceId}}});
}

TEST_F(ProjectStorageUpdater, watcher_dont_updates_qmltypes_for_other_projects)
{
    setProjectDatas(
        directoryPathSourceId,
        {{directoryPathSourceId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathSourceId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes}});
    setFilesDontChanged(
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
    setFilesDontChanged({qmlDirPathSourceId});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports, UnorderedElementsAre(import1, import2, import3)),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        Storage::Synchronization::ImportedType{"Object"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId1,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        Storage::Synchronization::ImportedType{"Object2"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId2,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))),
                    AllOf(IsStorageType("Second.qml",
                                        Storage::Synchronization::ImportedType{"Object3"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId3,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "SecondType", 2, 2),
                                                     IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDocumentSourceId1, qmlDocumentSourceId2, qmlDocumentSourceId3)),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(directoryPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(directoryPathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId3, 1, 21))),
            Field(&SynchronizationPackage::updatedProjectSourceIds,
                  UnorderedElementsAre(directoryPathSourceId)),
            Field(&SynchronizationPackage::projectDatas,
                  UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId1,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId2,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId3,
                                                     ModuleId{},
                                                     FileType::QmlDocument))))));

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
    setFilesDontChanged({directoryPathSourceId});
    setFilesChanged({qmlDirPathSourceId});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports, UnorderedElementsAre(import1, import2, import3)),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    AllOf(IsStorageType("First.qml",
                                        Storage::Synchronization::ImportedType{"Object"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId1,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        Storage::Synchronization::ImportedType{"Object2"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId2,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))),
                    AllOf(IsStorageType("Second.qml",
                                        Storage::Synchronization::ImportedType{"Object3"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId3,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "SecondType", 2, 2),
                                                     IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId3, 1, 21))),
            Field(&SynchronizationPackage::updatedProjectSourceIds,
                  UnorderedElementsAre(directoryPathSourceId)),
            Field(&SynchronizationPackage::projectDatas,
                  UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId1,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId2,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId3,
                                                     ModuleId{},
                                                     FileType::QmlDocument))))));

    updater.pathsWithIdsChanged({{qmldirProjectChunkId, {qmlDirPathSourceId}},
                                 {qmlDocumentProjectChunkId,
                                  {qmlDocumentSourceId1, qmlDocumentSourceId2, qmlDocumentSourceId3}}});
}

TEST_F(ProjectStorageUpdater, watcher_updates_qmldir_and_but_not_included_qmltypes)
{
    setProjectDatas(
        directoryPathSourceId,
        {{directoryPathSourceId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathSourceId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathSourceId, qmlDocumentSourceId1, exampleModuleId, FileType::QmlDocument},
         {directoryPathSourceId, qmlDocumentSourceId2, exampleModuleId, FileType::QmlDocument}});
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml
                      typeinfo example.qmltypes
                      typeinfo example2.qmltypes)"};
    setContent(u"/path/qmldir", qmldir);
    setFilesDontChanged({directoryPathSourceId});
    setFilesChanged({qmlDirPathSourceId,
                     qmltypesPathSourceId,
                     qmltypes2PathSourceId,
                     qmlDocumentSourceId1,
                     qmlDocumentSourceId2,
                     qmlDocumentSourceId3});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports,
                  UnorderedElementsAre(import1, import2, import3, import4, import5)),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    Eq(objectType),
                    Eq(itemType),
                    AllOf(IsStorageType("First.qml",
                                        Storage::Synchronization::ImportedType{"Object"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId1,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0),
                                                     IsExportedType(pathModuleId, "First", -1, -1)))),
                    AllOf(IsStorageType("First2.qml",
                                        Storage::Synchronization::ImportedType{"Object2"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId2,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2),
                                                     IsExportedType(pathModuleId, "First2", -1, -1)))),
                    AllOf(IsStorageType("Second.qml",
                                        Storage::Synchronization::ImportedType{"Object3"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId3,
                                        Storage::Synchronization::ChangeLevel::Full),
                          Field(&Storage::Synchronization::Type::exportedTypes,
                                UnorderedElementsAre(IsExportedType(exampleModuleId, "SecondType", 2, 2),
                                                     IsExportedType(pathModuleId, "Second", -1, -1)))))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmltypesPathSourceId,
                                       qmltypes2PathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmltypesPathSourceId,
                                       qmltypes2PathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 1, 21),
                                       IsFileStatus(qmltypesPathSourceId, 1, 21),
                                       IsFileStatus(qmltypes2PathSourceId, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId3, 1, 21))),
            Field(&SynchronizationPackage::updatedProjectSourceIds,
                  UnorderedElementsAre(directoryPathSourceId)),
            Field(&SynchronizationPackage::projectDatas,
                  UnorderedElementsAre(IsProjectData(directoryPathSourceId,
                                                     qmltypesPathSourceId,
                                                     exampleCppNativeModuleId,
                                                     FileType::QmlTypes),
                                       IsProjectData(directoryPathSourceId,
                                                     qmltypes2PathSourceId,
                                                     exampleCppNativeModuleId,
                                                     FileType::QmlTypes),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId1,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId2,
                                                     ModuleId{},
                                                     FileType::QmlDocument),
                                       IsProjectData(directoryPathSourceId,
                                                     qmlDocumentSourceId3,
                                                     ModuleId{},
                                                     FileType::QmlDocument))))));

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

    ON_CALL(projectStorageMock, synchronize(_)).WillByDefault(Throw(QmlDesigner::ProjectStorageError{}));

    ASSERT_NO_THROW(updater.pathsWithIdsChanged({{directoryProjectChunkId, {directoryPathSourceId}}}));
}

TEST_F(ProjectStorageUpdater, input_is_reused_next_call_if_an_error_happens)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First2.qml
                      SecondType 2.2 Second.qml)"};
    setContent(u"/path/qmldir", qmldir);
    setProjectDatas(
        directoryPathSourceId,
        {{directoryPathSourceId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathSourceId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes}});
    setFilesDontChanged({directoryPathSourceId, qmlDirPathSourceId});
    ON_CALL(projectStorageMock, synchronize(_)).WillByDefault(Throw(QmlDesigner::ProjectStorageError{}));
    updater.pathsWithIdsChanged(
        {{qmltypesProjectChunkId, {qmltypesPathSourceId, qmltypes2PathSourceId}}});
    ON_CALL(projectStorageMock, synchronize(_)).WillByDefault(Return());

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports,
                  UnorderedElementsAre(import1, import2, import4, import5)),
            Field(&SynchronizationPackage::types,
                  UnorderedElementsAre(
                      Eq(objectType),
                      Eq(itemType),
                      AllOf(IsStorageType("First.qml",
                                          Storage::Synchronization::ImportedType{"Object"},
                                          TypeTraits::Reference,
                                          qmlDocumentSourceId1,
                                          Storage::Synchronization::ChangeLevel::ExcludeExportedTypes),
                            Field(&Storage::Synchronization::Type::exportedTypes, IsEmpty())),
                      AllOf(IsStorageType("First2.qml",
                                          Storage::Synchronization::ImportedType{"Object2"},
                                          TypeTraits::Reference,
                                          qmlDocumentSourceId2,
                                          Storage::Synchronization::ChangeLevel::ExcludeExportedTypes),
                            Field(&Storage::Synchronization::Type::exportedTypes, IsEmpty())))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmltypesPathSourceId,
                                       qmltypes2PathSourceId)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                       IsFileStatus(qmltypesPathSourceId, 1, 21),
                                       IsFileStatus(qmltypes2PathSourceId, 1, 21))),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmltypesPathSourceId,
                                       qmltypes2PathSourceId)),
            Field(&SynchronizationPackage::projectDatas, IsEmpty()))));

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
    setProjectDatas(
        directoryPathSourceId,
        {{directoryPathSourceId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathSourceId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes}});
    setFilesDontChanged({directoryPathSourceId, qmlDirPathSourceId});
    ON_CALL(projectStorageMock, synchronize(_)).WillByDefault(Throw(QmlDesigner::ProjectStorageError{}));
    updater.pathsWithIdsChanged(
        {{qmltypesProjectChunkId, {qmltypesPathSourceId, qmltypes2PathSourceId}}});
    ON_CALL(projectStorageMock, synchronize(_)).WillByDefault(Return());

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports,
                  UnorderedElementsAre(import1, import2, import4, import5)),
            Field(
                &SynchronizationPackage::types,
                UnorderedElementsAre(
                    Eq(objectType),
                    Eq(itemType),
                    AllOf(IsStorageType("First.qml",
                                        Storage::Synchronization::ImportedType{"Object"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId1,
                                        Storage::Synchronization::ChangeLevel::ExcludeExportedTypes),
                          Field(&Storage::Synchronization::Type::exportedTypes, IsEmpty())),
                    AllOf(IsStorageType("First2.qml",
                                        Storage::Synchronization::ImportedType{"Object2"},
                                        TypeTraits::Reference,
                                        qmlDocumentSourceId2,
                                        Storage::Synchronization::ChangeLevel::ExcludeExportedTypes),
                          Field(&Storage::Synchronization::Type::exportedTypes, IsEmpty())))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmltypesPathSourceId,
                                       qmltypes2PathSourceId)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                       IsFileStatus(qmltypesPathSourceId, 1, 21),
                                       IsFileStatus(qmltypes2PathSourceId, 1, 21))),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmltypesPathSourceId,
                                       qmltypes2PathSourceId)),
            Field(&SynchronizationPackage::projectDatas, IsEmpty()))));

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
    setProjectDatas(
        directoryPathSourceId,
        {{directoryPathSourceId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathSourceId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathSourceId, qmlDocumentSourceId1, QmlDesigner::ModuleId{}, FileType::QmlDocument},
         {directoryPathSourceId, qmlDocumentSourceId1, QmlDesigner::ModuleId{}, FileType::QmlDocument}});
    setFilesDontChanged({directoryPathSourceId, qmlDirPathSourceId});
    ON_CALL(projectStorageMock, synchronize(_)).WillByDefault(Throw(QmlDesigner::ProjectStorageError{}));
    updater.pathsWithIdsChanged(
        {{qmlDocumentProjectChunkId, {qmlDocumentSourceId1, qmlDocumentSourceId2}}});
    ON_CALL(projectStorageMock, synchronize(_)).WillByDefault(Return());

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports,
                  UnorderedElementsAre(import1, import2, import4, import5)),
            Field(&SynchronizationPackage::types,
                  UnorderedElementsAre(
                      Eq(objectType),
                      Eq(itemType),
                      AllOf(IsStorageType("First.qml",
                                          Storage::Synchronization::ImportedType{"Object"},
                                          TypeTraits::Reference,
                                          qmlDocumentSourceId1,
                                          Storage::Synchronization::ChangeLevel::ExcludeExportedTypes),
                            Field(&Storage::Synchronization::Type::exportedTypes, IsEmpty())),
                      AllOf(IsStorageType("First2.qml",
                                          Storage::Synchronization::ImportedType{"Object2"},
                                          TypeTraits::Reference,
                                          qmlDocumentSourceId2,
                                          Storage::Synchronization::ChangeLevel::ExcludeExportedTypes),
                            Field(&Storage::Synchronization::Type::exportedTypes, IsEmpty())))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmltypesPathSourceId,
                                       qmltypes2PathSourceId)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21),
                                       IsFileStatus(qmltypesPathSourceId, 1, 21),
                                       IsFileStatus(qmltypes2PathSourceId, 1, 21))),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmltypesPathSourceId,
                                       qmltypes2PathSourceId)),
            Field(&SynchronizationPackage::projectDatas, IsEmpty()))));

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
    setProjectDatas(
        directoryPathSourceId,
        {{directoryPathSourceId, qmltypesPathSourceId, exampleModuleId, FileType::QmlTypes},
         {directoryPathSourceId, qmltypes2PathSourceId, exampleModuleId, FileType::QmlTypes}});
    setFilesDontChanged({directoryPathSourceId, qmlDirPathSourceId});
    updater.pathsWithIdsChanged(
        {{qmltypesProjectChunkId, {qmltypesPathSourceId, qmltypes2PathSourceId}}});

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports, UnorderedElementsAre(import1, import2)),
            Field(&SynchronizationPackage::types,
                  UnorderedElementsAre(
                      AllOf(IsStorageType("First.qml",
                                          Storage::Synchronization::ImportedType{"Object"},
                                          TypeTraits::Reference,
                                          qmlDocumentSourceId1,
                                          Storage::Synchronization::ChangeLevel::ExcludeExportedTypes),
                            Field(&Storage::Synchronization::Type::exportedTypes, IsEmpty())),
                      AllOf(IsStorageType("First2.qml",
                                          Storage::Synchronization::ImportedType{"Object2"},
                                          TypeTraits::Reference,
                                          qmlDocumentSourceId2,
                                          Storage::Synchronization::ChangeLevel::ExcludeExportedTypes),
                            Field(&Storage::Synchronization::Type::exportedTypes, IsEmpty())))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDocumentSourceId1, 1, 21),
                                       IsFileStatus(qmlDocumentSourceId2, 1, 21))),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field(&SynchronizationPackage::projectDatas, IsEmpty()))));

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
    auto directoryId = sourcePathCache.sourceId(
        QmlDesigner::SourcePath{propertyEditorQmlPath + "/QML/."});
    setFilesChanged({directoryId});
    auto qmlModuleId = storage.moduleId("QML");

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field(&SynchronizationPackage::fileStatuses,
                                UnorderedElementsAre(IsFileStatus(directoryId, 1, 21))),
                          Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                                UnorderedElementsAre(directoryId)),
                          Field(&SynchronizationPackage::propertyEditorQmlPaths,
                                Contains(IsPropertyEditorQmlPath(qmlModuleId, "QtObject", sourceId))),
                          Field(&SynchronizationPackage::updatedPropertyEditorQmlPathSourceIds,
                                ElementsAre(directoryId)))));

    updater.update({}, {}, propertyEditorQmlPath);
}

TEST_F(ProjectStorageUpdater, update_property_editor_specifics)
{
    ON_CALL(fileSystemMock, fileStatus(_)).WillByDefault([](SourceId sourceId) {
        return FileStatus{sourceId, 1, 21};
    });
    ON_CALL(projectStorageMock, fetchFileStatus(_)).WillByDefault([](SourceId sourceId) {
        return FileStatus{sourceId, 1, 21};
    });
    auto sourceId = sourcePathCache.sourceId(
        QmlDesigner::SourcePath{propertyEditorQmlPath + "/QtQuick/TextSpecifics.qml"});
    auto directoryId = sourcePathCache.sourceId(
        QmlDesigner::SourcePath{propertyEditorQmlPath + "/QtQuick/."});
    setFilesChanged({directoryId});
    auto qtQuickModuleId = storage.moduleId("QtQuick");

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field(&SynchronizationPackage::propertyEditorQmlPaths,
                                Contains(IsPropertyEditorQmlPath(qtQuickModuleId, "Text", sourceId))),
                          Field(&SynchronizationPackage::updatedPropertyEditorQmlPathSourceIds,
                                ElementsAre(directoryId)))));

    updater.update({}, {}, propertyEditorQmlPath);
}

TEST_F(ProjectStorageUpdater, update_property_editor_panes_is_empty_if_directory_has_not_changed)
{
    updater.update({}, {}, propertyEditorQmlPath);
    ON_CALL(fileSystemMock, fileStatus(_)).WillByDefault([](SourceId sourceId) {
        return FileStatus{sourceId, 1, 21};
    });
    ON_CALL(projectStorageMock, fetchFileStatus(_)).WillByDefault([](SourceId sourceId) {
        return FileStatus{sourceId, 1, 21};
    });

    EXPECT_CALL(projectStorageMock, synchronize(PackageIsEmpty()));

    updater.update({}, {}, propertyEditorQmlPath);
}

} // namespace
