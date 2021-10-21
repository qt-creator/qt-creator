/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "googletest.h"

#include "filesystemmock.h"
#include "projectmanagermock.h"
#include "projectstoragemock.h"
#include "qmldocumentparsermock.h"
#include "qmltypesparsermock.h"

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
using QmlDesigner::Storage::TypeAccessSemantics;
namespace Storage = QmlDesigner::Storage;
using QmlDesigner::IdPaths;
using QmlDesigner::Storage::FileType;
using QmlDesigner::Storage::SynchronizationPackage;
using QmlDesigner::Storage::Version;

MATCHER_P5(IsStorageType,
           typeName,
           prototype,
           accessSemantics,
           sourceId,
           changeLevel,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::Type(typeName, prototype, accessSemantics, sourceId, changeLevel)))
{
    const Storage::Type &type = arg;

    return type.typeName == typeName && type.accessSemantics == accessSemantics
           && type.sourceId == sourceId && Storage::ImportedTypeName{prototype} == type.prototype
           && type.changeLevel == changeLevel;
}

MATCHER_P3(IsPropertyDeclaration,
           name,
           typeName,
           traits,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::PropertyDeclaration{name, typeName, traits}))
{
    const Storage::PropertyDeclaration &propertyDeclaration = arg;

    return propertyDeclaration.name == name
           && Storage::ImportedTypeName{typeName} == propertyDeclaration.typeName
           && propertyDeclaration.traits == traits;
}

MATCHER_P4(IsExportedType,
           moduleId,
           name,
           majorVersion,
           minorVersion,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::ExportedType{moduleId,
                                                     name,
                                                     Storage::Version{majorVersion, minorVersion}}))
{
    const Storage::ExportedType &type = arg;

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

MATCHER_P3(IsProjectData,
           moduleId,
           sourceId,
           fileType,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::ProjectData{moduleId, sourceId, fileType}))
{
    const Storage::ProjectData &projectData = arg;

    return projectData.sourceId == sourceId && projectData.extraModuleId == moduleId
           && projectData.fileType == fileType;
}

MATCHER(PackageIsEmpty, std::string(negation ? "isn't empty" : "is empty"))
{
    const Storage::SynchronizationPackage &package = arg;

    return package.imports.empty() && package.types.empty() && package.fileStatuses.empty()
           && package.updatedSourceIds.empty() && package.projectDatas.empty()
           && package.updatedFileStatusSourceIds.empty()
           && package.updatedProjectDataModuleIds.empty();
}

class ProjectStorageUpdater : public testing::Test
{
public:
    ProjectStorageUpdater()
    {
        ON_CALL(fileSystemMock, fileStatus(Eq(qmltypesPathSourceId)))
            .WillByDefault(Return(FileStatus{qmltypesPathSourceId, 21, 421}));
        ON_CALL(projectStorageMock, fetchFileStatus(Eq(qmltypesPathSourceId)))
            .WillByDefault(Return(FileStatus{qmltypesPathSourceId, 2, 421}));

        ON_CALL(fileSystemMock, fileStatus(Eq(qmltypes2PathSourceId)))
            .WillByDefault(Return(FileStatus{qmltypes2PathSourceId, 21, 421}));
        ON_CALL(projectStorageMock, fetchFileStatus(Eq(qmltypes2PathSourceId)))
            .WillByDefault(Return(FileStatus{qmltypes2PathSourceId, 2, 421}));

        ON_CALL(fileSystemMock, fileStatus(Eq(qmlDirPathSourceId)))
            .WillByDefault(Return(FileStatus{qmlDirPathSourceId, 21, 421}));
        ON_CALL(projectStorageMock, fetchFileStatus(Eq(qmlDirPathSourceId)))
            .WillByDefault(Return(FileStatus{qmlDirPathSourceId, 2, 421}));

        ON_CALL(projectManagerMock, qtQmlDirs()).WillByDefault(Return(QStringList{"/path/qmldir"}));
        ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/qmldir"))))
            .WillByDefault(Return(qmldirContent));

        ON_CALL(fileSystemMock, fileStatus(Eq(qmlDocumentSourceId1)))
            .WillByDefault(Return(FileStatus{qmlDocumentSourceId1, 22, 12}));
        ON_CALL(projectStorageMock, fetchFileStatus(Eq(qmlDocumentSourceId1)))
            .WillByDefault(Return(FileStatus{qmlDocumentSourceId1, 22, 2}));
        ON_CALL(fileSystemMock, fileStatus(Eq(qmlDocumentSourceId2)))
            .WillByDefault(Return(FileStatus{qmlDocumentSourceId2, 22, 13}));
        ON_CALL(projectStorageMock, fetchFileStatus(Eq(qmlDocumentSourceId2)))
            .WillByDefault(Return(FileStatus{qmlDocumentSourceId2, 22, 2}));
        ON_CALL(fileSystemMock, fileStatus(Eq(qmlDocumentSourceId3)))
            .WillByDefault(Return(FileStatus{qmlDocumentSourceId3, 22, 14}));
        ON_CALL(projectStorageMock, fetchFileStatus(Eq(qmlDocumentSourceId3)))
            .WillByDefault(Return(FileStatus{qmlDocumentSourceId3, 22, 2}));
        ON_CALL(projectStorageMock, moduleId(Eq("Example"))).WillByDefault(Return(exampleModuleId));
        ON_CALL(projectStorageMock, moduleId(Eq("Qml"))).WillByDefault(Return(qmlModuleId));

        firstType.prototype = Storage::ImportedType{"Object"};
        secondType.prototype = Storage::ImportedType{"Object2"};
        thirdType.prototype = Storage::ImportedType{"Object3"};

        ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/First.qml"))))
            .WillByDefault(Return(qmlDocument1));
        ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/First.2.qml"))))
            .WillByDefault(Return(qmlDocument2));
        ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/Second.qml"))))
            .WillByDefault(Return(qmlDocument3));
        ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/example.qmltypes"))))
            .WillByDefault(Return(qmltypes1));
        ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/example2.qmltypes"))))
            .WillByDefault(Return(qmltypes2));

        ON_CALL(qmlDocumentParserMock, parse(qmlDocument1, _)).WillByDefault([&](auto, auto &imports) {
            imports.push_back(import1);
            return firstType;
        });
        ON_CALL(qmlDocumentParserMock, parse(qmlDocument2, _)).WillByDefault([&](auto, auto &imports) {
            imports.push_back(import2);
            return secondType;
        });
        ON_CALL(qmlDocumentParserMock, parse(qmlDocument3, _)).WillByDefault([&](auto, auto &imports) {
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

protected:
    NiceMock<ProjectManagerMock> projectManagerMock;
    NiceMock<FileSystemMock> fileSystemMock;
    NiceMock<ProjectStorageMock> projectStorageMock;
    NiceMock<QmlTypesParserMock> qmlTypesParserMock;
    NiceMock<QmlDocumentParserMock> qmlDocumentParserMock;
    QmlDesigner::FileStatusCache fileStatusCache{fileSystemMock};
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    QmlDesigner::ProjectStorage<Sqlite::Database> storage{database, database.isInitialized()};
    QmlDesigner::SourcePathCache<QmlDesigner::ProjectStorage<Sqlite::Database>> sourcePathCache{
        storage};
    QmlDesigner::ProjectUpdater updater{projectManagerMock,
                                        fileSystemMock,
                                        projectStorageMock,
                                        fileStatusCache,
                                        sourcePathCache,
                                        qmlDocumentParserMock,
                                        qmlTypesParserMock};
    SourceId qmltypesPathSourceId = sourcePathCache.sourceId("/path/example.qmltypes");
    SourceId qmltypes2PathSourceId = sourcePathCache.sourceId("/path/example2.qmltypes");
    SourceId qmlDirPathSourceId = sourcePathCache.sourceId("/path/qmldir");
    SourceId qmlDocumentSourceId1 = sourcePathCache.sourceId("/path/First.qml");
    SourceId qmlDocumentSourceId2 = sourcePathCache.sourceId("/path/First.2.qml");
    SourceId qmlDocumentSourceId3 = sourcePathCache.sourceId("/path/Second.qml");
    ModuleId qmlModuleId{storage.moduleId("Qml")};
    ModuleId exampleModuleId{storage.moduleId("Example")};
    Storage::Type objectType{"QObject",
                             Storage::NativeType{},
                             Storage::TypeAccessSemantics::Reference,
                             qmltypesPathSourceId,
                             {Storage::ExportedType{exampleModuleId, "Object"},
                              Storage::ExportedType{exampleModuleId, "Obj"}}};
    Storage::Type itemType{"QItem",
                           Storage::NativeType{},
                           Storage::TypeAccessSemantics::Reference,
                           qmltypes2PathSourceId,
                           {Storage::ExportedType{exampleModuleId, "Item"}}};
    QString qmlDocument1{"First{}"};
    QString qmlDocument2{"Second{}"};
    QString qmlDocument3{"Third{}"};
    Storage::Type firstType;
    Storage::Type secondType;
    Storage::Type thirdType;
    Storage::Import import1{qmlModuleId, Storage::Version{2, 3}, qmlDocumentSourceId1};
    Storage::Import import2{qmlModuleId, Storage::Version{}, qmlDocumentSourceId2};
    Storage::Import import3{qmlModuleId, Storage::Version{2}, qmlDocumentSourceId3};
    Storage::Import import4{qmlModuleId, Storage::Version{2, 3}, qmltypesPathSourceId};
    Storage::Import import5{qmlModuleId, Storage::Version{2, 3}, qmltypes2PathSourceId};
    QString qmldirContent{"module Example\ntypeinfo example.qmltypes\n"};
    QString qmltypes1{"Module {\ndependencies: [module1]}"};
    QString qmltypes2{"Module {\ndependencies: [module2]}"};
};

TEST_F(ProjectStorageUpdater, GetContentForQmlDirPathsIfFileStatusIsDifferent)
{
    SourceId qmlDir1PathSourceId = sourcePathCache.sourceId("/path/one/qmldir");
    SourceId qmlDir2PathSourceId = sourcePathCache.sourceId("/path/two/qmldir");
    SourceId qmlDir3PathSourceId = sourcePathCache.sourceId("/path/three/qmldir");
    ON_CALL(projectManagerMock, qtQmlDirs())
        .WillByDefault(
            Return(QStringList{"/path/one/qmldir", "/path/two/qmldir", "/path/three/qmldir"}));
    ON_CALL(fileSystemMock, fileStatus(_)).WillByDefault([](auto sourceId) {
        return FileStatus{sourceId, 21, 421};
    });
    ON_CALL(projectStorageMock, fetchFileStatus(_)).WillByDefault([](auto sourceId) {
        return FileStatus{sourceId, 2, 421};
    });
    ON_CALL(fileSystemMock, fileStatus(Eq(qmlDir3PathSourceId)))
        .WillByDefault(Return(FileStatus{qmlDir3PathSourceId, 21, 421}));
    ON_CALL(projectStorageMock, fetchFileStatus(Eq(qmlDir3PathSourceId)))
        .WillByDefault(Return(FileStatus{qmlDir3PathSourceId, 21, 421}));

    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/one/qmldir"))));
    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/two/qmldir"))));

    updater.update();
}

TEST_F(ProjectStorageUpdater, RequestFileStatusFromFileSystem)
{
    EXPECT_CALL(fileSystemMock, fileStatus(Ne(qmlDirPathSourceId))).Times(AnyNumber());

    EXPECT_CALL(fileSystemMock, fileStatus(Eq(qmlDirPathSourceId)));

    updater.update();
}

TEST_F(ProjectStorageUpdater, GetContentForQmlTypes)
{
    QString qmldir{R"(module Example
                      typeinfo example.qmltypes)"};
    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/qmldir"))))
        .WillRepeatedly(Return(qmldir));

    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/example.qmltypes"))));

    updater.update();
}

TEST_F(ProjectStorageUpdater, GetContentForQmlTypesIfProjectStorageFileStatusIsInvalid)
{
    QString qmldir{R"(module Example
                      typeinfo example.qmltypes)"};
    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/qmldir"))))
        .WillRepeatedly(Return(qmldir));
    ON_CALL(projectStorageMock, fetchFileStatus(Eq(qmltypesPathSourceId)))
        .WillByDefault(Return(FileStatus{}));

    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/example.qmltypes"))));

    updater.update();
}

TEST_F(ProjectStorageUpdater, DontGetContentForQmlTypesIfFileSystemFileStatusIsInvalid)
{
    QString qmldir{R"(module Example
                      typeinfo example.qmltypes)"};
    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/qmldir"))))
        .WillRepeatedly(Return(qmldir));
    ON_CALL(fileSystemMock, fileStatus(Eq(qmltypesPathSourceId))).WillByDefault(Return(FileStatus{}));

    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/example.qmltypes")))).Times(0);

    updater.update();
}

TEST_F(ProjectStorageUpdater, ParseQmlTypes)
{
    QString qmldir{R"(module Example
                      typeinfo example.qmltypes
                      typeinfo example2.qmltypes)"};
    ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/qmldir")))).WillByDefault(Return(qmldir));
    QString qmltypes{"Module {\ndependencies: []}"};
    QString qmltypes2{"Module {\ndependencies: [foo]}"};
    ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/example.qmltypes"))))
        .WillByDefault(Return(qmltypes));
    ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/example2.qmltypes"))))
        .WillByDefault(Return(qmltypes2));

    EXPECT_CALL(qmlTypesParserMock, parse(qmltypes, _, _, _));
    EXPECT_CALL(qmlTypesParserMock, parse(qmltypes2, _, _, _));

    updater.update();
}

TEST_F(ProjectStorageUpdater, SynchronizeIsEmptyForNoChange)
{
    ON_CALL(projectStorageMock, fetchFileStatus(Eq(qmltypesPathSourceId)))
        .WillByDefault(Return(FileStatus{qmltypesPathSourceId, 21, 421}));
    ON_CALL(projectStorageMock, fetchFileStatus(Eq(qmltypes2PathSourceId)))
        .WillByDefault(Return(FileStatus{qmltypes2PathSourceId, 21, 421}));
    ON_CALL(projectStorageMock, fetchFileStatus(Eq(qmlDirPathSourceId)))
        .WillByDefault(Return(FileStatus{qmlDirPathSourceId, 21, 421}));

    EXPECT_CALL(projectStorageMock, synchronize(PackageIsEmpty()));

    updater.update();
}

TEST_F(ProjectStorageUpdater, SynchronizeQmlTypes)
{
    Storage::Import import{qmlModuleId, Storage::Version{2, 3}, qmltypesPathSourceId};
    QString qmltypes{"Module {\ndependencies: []}"};
    ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/example.qmltypes"))))
        .WillByDefault(Return(qmltypes));
    ON_CALL(qmlTypesParserMock, parse(qmltypes, _, _, _))
        .WillByDefault([&](auto, auto &imports, auto &types, auto) {
            types.push_back(objectType);
            imports.push_back(import);
        });

    EXPECT_CALL(projectStorageMock, moduleId(Eq("Example")));
    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field(&SynchronizationPackage::imports, ElementsAre(import)),
                          Field(&SynchronizationPackage::types, ElementsAre(Eq(objectType))),
                          Field(&SynchronizationPackage::updatedSourceIds,
                                UnorderedElementsAre(qmlDirPathSourceId, qmltypesPathSourceId)),
                          Field(&SynchronizationPackage::fileStatuses,
                                UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 21, 421),
                                                     IsFileStatus(qmltypesPathSourceId, 21, 421))),
                          Field(&SynchronizationPackage::projectDatas,
                                UnorderedElementsAre(IsProjectData(exampleModuleId,
                                                                   qmltypesPathSourceId,
                                                                   FileType::QmlTypes))),
                          Field(&SynchronizationPackage::updatedProjectDataModuleIds,
                                UnorderedElementsAre(exampleModuleId)))));

    updater.update();
}

TEST_F(ProjectStorageUpdater, SynchronizeQmlTypesAreEmptyIfFileDoesNotChanged)
{
    QString qmltypes{"Module {\ndependencies: []}"};
    ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/example.qmltypes"))))
        .WillByDefault(Return(qmltypes));
    ON_CALL(qmlTypesParserMock, parse(qmltypes, _, _, _))
        .WillByDefault([&](auto, auto &, auto &types, auto) { types.push_back(objectType); });
    ON_CALL(fileSystemMock, fileStatus(Eq(qmltypesPathSourceId)))
        .WillByDefault(Return(FileStatus{qmltypesPathSourceId, 2, 421}));
    ON_CALL(fileSystemMock, fileStatus(Eq(qmltypes2PathSourceId)))
        .WillByDefault(Return(FileStatus{qmltypes2PathSourceId, 2, 421}));
    ON_CALL(fileSystemMock, fileStatus(Eq(qmlDirPathSourceId)))
        .WillByDefault(Return(FileStatus{qmlDirPathSourceId, 2, 421}));

    EXPECT_CALL(projectStorageMock, synchronize(PackageIsEmpty()));

    updater.update();
}

TEST_F(ProjectStorageUpdater, GetContentForQmlDocuments)
{
    QString qmldir{"module Example\nFirstType 1.0 First.qml\nFirstTypeV2 2.2 "
                   "First.2.qml\nSecondType 2.1 OldSecond.qml\nSecondType 2.2 Second.qml\n"};
    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/qmldir"))))
        .WillRepeatedly(Return(qmldir));

    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/First.qml"))));
    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/First.2.qml"))));
    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/Second.qml"))));

    updater.update();
}

TEST_F(ProjectStorageUpdater, ParseQmlDocuments)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstTypeV2 2.2 First.2.qml
                      SecondType 2.2 Second.qml)"};
    QString qmlDocument1{"First{}"};
    QString qmlDocument2{"Second{}"};
    QString qmlDocument3{"Third{}"};
    ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/qmldir")))).WillByDefault(Return(qmldir));
    ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/First.qml"))))
        .WillByDefault(Return(qmlDocument1));
    ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/First.2.qml"))))
        .WillByDefault(Return(qmlDocument2));
    ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/Second.qml"))))
        .WillByDefault(Return(qmlDocument3));

    EXPECT_CALL(qmlDocumentParserMock, parse(qmlDocument1, _));
    EXPECT_CALL(qmlDocumentParserMock, parse(qmlDocument2, _));
    EXPECT_CALL(qmlDocumentParserMock, parse(qmlDocument3, _));

    updater.update();
}

TEST_F(ProjectStorageUpdater, ParseQmlDocumentsWithNonExistingQmlDocumentThrows)
{
    QString qmldir{R"(module Example
                      NonexitingType 1.0 NonexitingType.qml)"};
    ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/qmldir")))).WillByDefault(Return(qmldir));

    ASSERT_THROW(updater.update(), QmlDesigner::CannotParseQmlDocumentFile);
}

TEST_F(ProjectStorageUpdater, SynchronizeQmlDocuments)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First.2.qml
                      SecondType 2.2 Second.qml)"};
    ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/qmldir")))).WillByDefault(Return(qmldir));

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports, UnorderedElementsAre(import1, import2, import3)),
            Field(&SynchronizationPackage::types,
                  UnorderedElementsAre(
                      AllOf(IsStorageType("First.qml",
                                          Storage::ImportedType{"Object"},
                                          TypeAccessSemantics::Reference,
                                          qmlDocumentSourceId1,
                                          Storage::ChangeLevel::Full),
                            Field(&Storage::Type::exportedTypes,
                                  ElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0)))),
                      AllOf(IsStorageType("First.2.qml",
                                          Storage::ImportedType{"Object2"},
                                          TypeAccessSemantics::Reference,
                                          qmlDocumentSourceId2,
                                          Storage::ChangeLevel::Full),
                            Field(&Storage::Type::exportedTypes,
                                  ElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2)))),
                      AllOf(IsStorageType("Second.qml",
                                          Storage::ImportedType{"Object3"},
                                          TypeAccessSemantics::Reference,
                                          qmlDocumentSourceId3,
                                          Storage::ChangeLevel::Full),
                            Field(&Storage::Type::exportedTypes,
                                  ElementsAre(IsExportedType(exampleModuleId, "SecondType", 2, 2)))))),
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
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 21, 421),
                                       IsFileStatus(qmlDocumentSourceId1, 22, 12),
                                       IsFileStatus(qmlDocumentSourceId2, 22, 13),
                                       IsFileStatus(qmlDocumentSourceId3, 22, 14))),
            Field(&SynchronizationPackage::updatedProjectDataModuleIds,
                  UnorderedElementsAre(exampleModuleId)),
            Field(&SynchronizationPackage::projectDatas,
                  UnorderedElementsAre(
                      IsProjectData(exampleModuleId, qmlDocumentSourceId1, FileType::QmlDocument),
                      IsProjectData(exampleModuleId, qmlDocumentSourceId2, FileType::QmlDocument),
                      IsProjectData(exampleModuleId, qmlDocumentSourceId3, FileType::QmlDocument))))));

    updater.update();
}

TEST_F(ProjectStorageUpdater, SynchronizeRemoved)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First.2.qml
                      typeinfo example.qmltypes
                      typeinfo example2.qmltypes
                      )"};
    ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/qmldir")))).WillByDefault(Return(qmldir));
    ON_CALL(projectStorageMock, fetchFileStatus(Eq(qmlDocumentSourceId2)))
        .WillByDefault(Return(FileStatus{qmlDocumentSourceId2, 22, 13}));
    ON_CALL(projectStorageMock, fetchFileStatus(Eq(qmltypes2PathSourceId)))
        .WillByDefault(Return(FileStatus{qmltypes2PathSourceId, 21, 421}));
    ON_CALL(projectStorageMock, fetchProjectDatas(Eq(qmlDirPathSourceId)))
        .WillByDefault(Return(QmlDesigner::Storage::ProjectDatas{
            {exampleModuleId, qmltypesPathSourceId, FileType::QmlTypes},
            {exampleModuleId, qmltypes2PathSourceId, FileType::QmlTypes},
            {exampleModuleId, qmlDocumentSourceId1, FileType::QmlDocument},
            {exampleModuleId, qmlDocumentSourceId2, FileType::QmlDocument},
            {exampleModuleId, qmlDocumentSourceId3, FileType::QmlDocument}}));

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports, UnorderedElementsAre(import1, import4)),
            Field(&SynchronizationPackage::types,
                  UnorderedElementsAre(
                      Eq(objectType),
                      AllOf(IsStorageType("First.qml",
                                          Storage::ImportedType{"Object"},
                                          TypeAccessSemantics::Reference,
                                          qmlDocumentSourceId1,
                                          Storage::ChangeLevel::Full),
                            Field(&Storage::Type::exportedTypes,
                                  ElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0)))),
                      AllOf(IsStorageType("First.2.qml",
                                          Storage::NativeType{},
                                          TypeAccessSemantics::Reference,
                                          qmlDocumentSourceId2,
                                          Storage::ChangeLevel::Minimal),
                            Field(&Storage::Type::exportedTypes,
                                  ElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2)))))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmltypesPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmltypesPathSourceId,
                                       qmlDocumentSourceId3)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 21, 421),
                                       IsFileStatus(qmlDocumentSourceId1, 22, 12),
                                       IsFileStatus(qmltypesPathSourceId, 21, 421))),
            Field(&SynchronizationPackage::updatedProjectDataModuleIds,
                  UnorderedElementsAre(exampleModuleId)),
            Field(&SynchronizationPackage::projectDatas,
                  UnorderedElementsAre(
                      IsProjectData(exampleModuleId, qmlDocumentSourceId1, FileType::QmlDocument),
                      IsProjectData(exampleModuleId, qmlDocumentSourceId2, FileType::QmlDocument),
                      IsProjectData(exampleModuleId, qmltypesPathSourceId, FileType::QmlTypes),
                      IsProjectData(exampleModuleId, qmltypes2PathSourceId, FileType::QmlTypes))))));

    updater.update();
}

TEST_F(ProjectStorageUpdater, SynchronizeQmlDocumentsDontUpdateIfUpToDate)
{
    QString qmldir{R"(module Example
                      FirstType 1.0 First.qml
                      FirstType 2.2 First.2.qml
                      SecondType 2.2 Second.qml)"};
    ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/qmldir")))).WillByDefault(Return(qmldir));
    ON_CALL(projectStorageMock, fetchFileStatus(Eq(qmlDocumentSourceId3)))
        .WillByDefault(Return(FileStatus{qmlDocumentSourceId3, 22, 14}));

    EXPECT_CALL(
        projectStorageMock,
        synchronize(AllOf(
            Field(&SynchronizationPackage::imports, UnorderedElementsAre(import1, import2)),
            Field(&SynchronizationPackage::types,
                  UnorderedElementsAre(
                      AllOf(IsStorageType("First.qml",
                                          Storage::ImportedType{"Object"},
                                          TypeAccessSemantics::Reference,
                                          qmlDocumentSourceId1,
                                          Storage::ChangeLevel::Full),
                            Field(&Storage::Type::exportedTypes,
                                  ElementsAre(IsExportedType(exampleModuleId, "FirstType", 1, 0)))),
                      AllOf(IsStorageType("First.2.qml",
                                          Storage::ImportedType{"Object2"},
                                          TypeAccessSemantics::Reference,
                                          qmlDocumentSourceId2,
                                          Storage::ChangeLevel::Full),
                            Field(&Storage::Type::exportedTypes,
                                  ElementsAre(IsExportedType(exampleModuleId, "FirstType", 2, 2)))),
                      AllOf(IsStorageType("Second.qml",
                                          Storage::NativeType{},
                                          TypeAccessSemantics::Reference,
                                          qmlDocumentSourceId3,
                                          Storage::ChangeLevel::Minimal),
                            Field(&Storage::Type::exportedTypes,
                                  ElementsAre(IsExportedType(exampleModuleId, "SecondType", 2, 2)))))),
            Field(&SynchronizationPackage::updatedSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId,
                                       qmlDocumentSourceId1,
                                       qmlDocumentSourceId2,
                                       qmlDocumentSourceId3)),
            Field(&SynchronizationPackage::fileStatuses,
                  UnorderedElementsAre(IsFileStatus(qmlDirPathSourceId, 21, 421),
                                       IsFileStatus(qmlDocumentSourceId1, 22, 12),
                                       IsFileStatus(qmlDocumentSourceId2, 22, 13))),
            Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                  UnorderedElementsAre(qmlDirPathSourceId, qmlDocumentSourceId1, qmlDocumentSourceId2)),
            Field(&SynchronizationPackage::projectDatas,
                  UnorderedElementsAre(
                      IsProjectData(exampleModuleId, qmlDocumentSourceId1, FileType::QmlDocument),
                      IsProjectData(exampleModuleId, qmlDocumentSourceId2, FileType::QmlDocument),
                      IsProjectData(exampleModuleId, qmlDocumentSourceId3, FileType::QmlDocument))))));

    updater.update();
}

TEST_F(ProjectStorageUpdater, UpdateQmldirDocuments)
{
    QString qmldir{R"(module Example
                      FirstType 1.1 First.qml
                      FirstType 2.2 First.2.qml
                      SecondType 2.2 Second.qml)"};
    ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/qmldir")))).WillByDefault(Return(qmldir));
    ON_CALL(projectStorageMock, fetchFileStatus(Eq(qmlDocumentSourceId3)))
        .WillByDefault(Return(FileStatus{qmlDocumentSourceId3, 22, 14}));

    updater.pathsWithIdsChanged({});
}

TEST_F(ProjectStorageUpdater, AddSourceIdForForInvalidQmldirFileStatus)
{
    ON_CALL(projectStorageMock, fetchProjectDatas(Eq(qmlDirPathSourceId)))
        .WillByDefault(Return(QmlDesigner::Storage::ProjectDatas{
            {exampleModuleId, qmltypesPathSourceId, FileType::QmlTypes},
            {exampleModuleId, qmltypes2PathSourceId, FileType::QmlTypes}}));
    ON_CALL(fileSystemMock, fileStatus(Eq(qmlDirPathSourceId))).WillByDefault(Return(FileStatus{}));

    EXPECT_CALL(projectStorageMock,
                synchronize(AllOf(Field(&SynchronizationPackage::imports, IsEmpty()),
                                  Field(&SynchronizationPackage::types, IsEmpty()),
                                  Field(&SynchronizationPackage::updatedSourceIds,
                                        UnorderedElementsAre(qmlDirPathSourceId,
                                                             qmltypesPathSourceId,
                                                             qmltypes2PathSourceId)),
                                  Field(&SynchronizationPackage::fileStatuses, IsEmpty()),
                                  Field(&SynchronizationPackage::updatedFileStatusSourceIds, IsEmpty()),
                                  Field(&SynchronizationPackage::projectDatas, IsEmpty()))));
    updater.update();
}

TEST_F(ProjectStorageUpdater, SynchronizIfQmldirFileHasNotChanged)
{
    ON_CALL(projectStorageMock, fetchProjectDatas(Eq(qmlDirPathSourceId)))
        .WillByDefault(Return(QmlDesigner::Storage::ProjectDatas{
            {exampleModuleId, qmltypesPathSourceId, FileType::QmlTypes},
            {exampleModuleId, qmltypes2PathSourceId, FileType::QmlTypes},
            {exampleModuleId, qmlDocumentSourceId1, FileType::QmlDocument},
            {exampleModuleId, qmlDocumentSourceId2, FileType::QmlDocument}}));
    ON_CALL(projectStorageMock, fetchFileStatus(Eq(qmlDirPathSourceId)))
        .WillByDefault(Return(FileStatus{qmlDirPathSourceId, 21, 421}));

    EXPECT_CALL(projectStorageMock,
                synchronize(
                    AllOf(Field(&SynchronizationPackage::imports,
                                UnorderedElementsAre(import1, import2, import4, import5)),
                          Field(&SynchronizationPackage::types,
                                UnorderedElementsAre(
                                    Eq(objectType),
                                    Eq(itemType),
                                    AllOf(IsStorageType("First.qml",
                                                        Storage::ImportedType{"Object"},
                                                        TypeAccessSemantics::Reference,
                                                        qmlDocumentSourceId1,
                                                        Storage::ChangeLevel::ExcludeExportedTypes),
                                          Field(&Storage::Type::exportedTypes, IsEmpty())),
                                    AllOf(IsStorageType("First.2.qml",
                                                        Storage::ImportedType{"Object2"},
                                                        TypeAccessSemantics::Reference,
                                                        qmlDocumentSourceId2,
                                                        Storage::ChangeLevel::ExcludeExportedTypes),
                                          Field(&Storage::Type::exportedTypes, IsEmpty())))),
                          Field(&SynchronizationPackage::updatedSourceIds,
                                UnorderedElementsAre(qmltypesPathSourceId,
                                                     qmltypes2PathSourceId,
                                                     qmlDocumentSourceId1,
                                                     qmlDocumentSourceId2)),
                          Field(&SynchronizationPackage::fileStatuses,
                                UnorderedElementsAre(IsFileStatus(qmltypesPathSourceId, 21, 421),
                                                     IsFileStatus(qmltypes2PathSourceId, 21, 421),
                                                     IsFileStatus(qmlDocumentSourceId1, 22, 12),
                                                     IsFileStatus(qmlDocumentSourceId2, 22, 13))),
                          Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                                UnorderedElementsAre(qmltypesPathSourceId,
                                                     qmltypes2PathSourceId,
                                                     qmlDocumentSourceId1,
                                                     qmlDocumentSourceId2)),
                          Field(&SynchronizationPackage::projectDatas, IsEmpty()))));

    updater.update();
}

TEST_F(ProjectStorageUpdater, SynchronizIfQmldirFileHasNotChangedAndSomeUpdatedFiles)
{
    ON_CALL(projectStorageMock, fetchProjectDatas(Eq(qmlDirPathSourceId)))
        .WillByDefault(Return(QmlDesigner::Storage::ProjectDatas{
            {exampleModuleId, qmltypesPathSourceId, FileType::QmlTypes},
            {exampleModuleId, qmltypes2PathSourceId, FileType::QmlTypes},
            {exampleModuleId, qmlDocumentSourceId1, FileType::QmlDocument},
            {exampleModuleId, qmlDocumentSourceId2, FileType::QmlDocument}}));
    ON_CALL(projectStorageMock, fetchFileStatus(Eq(qmlDirPathSourceId)))
        .WillByDefault(Return(FileStatus{qmlDirPathSourceId, 21, 421}));
    ON_CALL(projectStorageMock, fetchFileStatus(Eq(qmltypes2PathSourceId)))
        .WillByDefault(Return(FileStatus{qmltypes2PathSourceId, 21, 421}));
    ON_CALL(projectStorageMock, fetchFileStatus(Eq(qmlDocumentSourceId2)))
        .WillByDefault(Return(FileStatus{qmlDocumentSourceId2, 22, 13}));

    EXPECT_CALL(
        projectStorageMock,
        synchronize(
            AllOf(Field(&SynchronizationPackage::imports, UnorderedElementsAre(import1, import4)),
                  Field(&SynchronizationPackage::types,
                        UnorderedElementsAre(Eq(objectType),
                                             AllOf(IsStorageType("First.qml",
                                                                 Storage::ImportedType{"Object"},
                                                                 TypeAccessSemantics::Reference,
                                                                 qmlDocumentSourceId1,
                                                                 Storage::ChangeLevel::ExcludeExportedTypes),
                                                   Field(&Storage::Type::exportedTypes, IsEmpty())))),
                  Field(&SynchronizationPackage::updatedSourceIds,
                        UnorderedElementsAre(qmltypesPathSourceId, qmlDocumentSourceId1)),
                  Field(&SynchronizationPackage::fileStatuses,
                        UnorderedElementsAre(IsFileStatus(qmltypesPathSourceId, 21, 421),
                                             IsFileStatus(qmlDocumentSourceId1, 22, 12))),
                  Field(&SynchronizationPackage::updatedFileStatusSourceIds,
                        UnorderedElementsAre(qmltypesPathSourceId, qmlDocumentSourceId1)),
                  Field(&SynchronizationPackage::projectDatas, IsEmpty()))));

    updater.update();
}

} // namespace
