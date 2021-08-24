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
using QmlDesigner::SourceId;
using QmlDesigner::Storage::TypeAccessSemantics;
namespace Storage = QmlDesigner::Storage;

MATCHER_P5(IsStorageType,
           module,
           typeName,
           prototype,
           accessSemantics,
           sourceId,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(Storage::Type{module, typeName, prototype, accessSemantics, sourceId}))
{
    const Storage::Type &type = arg;

    return type.module == module && type.typeName == typeName
           && type.accessSemantics == accessSemantics && type.sourceId == sourceId
           && Storage::TypeName{prototype} == type.prototype;
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
           && Storage::TypeName{typeName} == propertyDeclaration.typeName
           && propertyDeclaration.traits == traits;
}

MATCHER_P(IsExportedType,
          name,
          std::string(negation ? "isn't " : "is ") + PrintToString(Storage::ExportedType{name}))
{
    const Storage::ExportedType &type = arg;

    return type.name == name;
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

        ON_CALL(projectStorageMock, fetchSourceDependencieIds(Eq(qmlDirPathSourceId)))
            .WillByDefault(Return(QmlDesigner::SourceIds{qmltypesPathSourceId, qmltypes2PathSourceId}));

        QString qmldir{"module Example\ntypeinfo example.qmltypes\n"};
        ON_CALL(projectManagerMock, qtQmlDirs()).WillByDefault(Return(QStringList{"/path/qmldir"}));
        ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/qmldir"))))
            .WillByDefault(Return(qmldir));
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
    SourceId objectTypeSourceId{sourcePathCache.sourceId("/path/Object")};
    Storage::Type objectType{Storage::Module{"Qml", 2},
                             "QObject",
                             Storage::NativeType{},
                             Storage::TypeAccessSemantics::Reference,
                             objectTypeSourceId,
                             {Storage::ExportedType{"Object"}, Storage::ExportedType{"Obj"}}};
    SourceId qmltypesPathSourceId = sourcePathCache.sourceId("/path/example.qmltypes");
    SourceId qmltypes2PathSourceId = sourcePathCache.sourceId("/path/example2.qmltypes");
    SourceId qmlDirPathSourceId = sourcePathCache.sourceId("/path/qmldir");
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
    QString qmldir{"module Example\ntypeinfo example.qmltypes\n"};
    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/qmldir"))))
        .WillRepeatedly(Return(qmldir));

    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/example.qmltypes"))));

    updater.update();
}

TEST_F(ProjectStorageUpdater, GetContentForQmlTypesIfProjectStorageFileStatusIsInvalid)
{
    QString qmldir{"module Example\ntypeinfo example.qmltypes\n"};
    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/qmldir"))))
        .WillRepeatedly(Return(qmldir));
    ON_CALL(projectStorageMock, fetchFileStatus(Eq(qmltypesPathSourceId)))
        .WillByDefault(Return(FileStatus{}));

    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/example.qmltypes"))));

    updater.update();
}

TEST_F(ProjectStorageUpdater, DontGetContentForQmlTypesIfFileSystemFileStatusIsInvalid)
{
    QString qmldir{"module Example\ntypeinfo example.qmltypes\n"};
    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/qmldir"))))
        .WillRepeatedly(Return(qmldir));
    ON_CALL(fileSystemMock, fileStatus(Eq(qmltypesPathSourceId))).WillByDefault(Return(FileStatus{}));

    EXPECT_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/example.qmltypes")))).Times(0);

    updater.update();
}

TEST_F(ProjectStorageUpdater, ParseQmlTypes)
{
    QString qmldir{"module Example\ntypeinfo example.qmltypes\ntypeinfo example2.qmltypes\n"};
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

TEST_F(ProjectStorageUpdater, DISABLED_SynchronizeIsEmptyForNoChange)
{
    EXPECT_CALL(projectStorageMock,
                synchronize(IsEmpty(), IsEmpty(), IsEmpty(), IsEmpty(), IsEmpty()));

    updater.update();
}

TEST_F(ProjectStorageUpdater, SynchronizeQmlTypes)
{
    auto qmlDirPathSourceId = sourcePathCache.sourceId("/path/qmldir");
    auto qmltypesPathSourceId = sourcePathCache.sourceId("/path/example.qmltypes");
    QString qmltypes{"Module {\ndependencies: []}"};
    ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/example.qmltypes"))))
        .WillByDefault(Return(qmltypes));
    ON_CALL(qmlTypesParserMock, parse(qmltypes, _, _, _))
        .WillByDefault([&](auto, auto &moduleDependencies, auto &types, auto &sourceIds) {
            types.push_back(objectType);
        });

    EXPECT_CALL(projectStorageMock,
                synchronize(_,
                            _,
                            ElementsAre(Eq(objectType)),
                            UnorderedElementsAre(qmlDirPathSourceId, qmltypesPathSourceId),
                            _));

    updater.update();
}

TEST_F(ProjectStorageUpdater, SynchronizeQmlTypesAreEmptyIfFileDoesNotChanged)
{
    QString qmltypes{"Module {\ndependencies: []}"};
    ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/example.qmltypes"))))
        .WillByDefault(Return(qmltypes));
    ON_CALL(qmlTypesParserMock, parse(qmltypes, _, _, _))
        .WillByDefault([&](auto, auto &moduleDependencies, auto &types, auto &sourceIds) {
            types.push_back(objectType);
        });
    ON_CALL(fileSystemMock, fileStatus(Eq(qmltypesPathSourceId)))
        .WillByDefault(Return(FileStatus{qmltypesPathSourceId, 2, 421}));
    ON_CALL(fileSystemMock, fileStatus(Eq(qmltypes2PathSourceId)))
        .WillByDefault(Return(FileStatus{qmltypes2PathSourceId, 2, 421}));
    ON_CALL(fileSystemMock, fileStatus(Eq(qmlDirPathSourceId)))
        .WillByDefault(Return(FileStatus{qmlDirPathSourceId, 2, 421}));

    EXPECT_CALL(projectStorageMock,
                synchronize(IsEmpty(), IsEmpty(), IsEmpty(), IsEmpty(), IsEmpty()));

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
    QString qmldir{"module Example\nFirstType 1.0 First.qml\nFirstTypeV2 2.2 "
                   "First.2.qml\nSecondType 2.1 OldSecond.qml\nSecondType 2.2 Second.qml\n"};
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

    EXPECT_CALL(qmlDocumentParserMock, parse(qmlDocument1));
    EXPECT_CALL(qmlDocumentParserMock, parse(qmlDocument2));
    EXPECT_CALL(qmlDocumentParserMock, parse(qmlDocument3));

    updater.update();
}

TEST_F(ProjectStorageUpdater, SynchronizeQmlDocuments)
{
    QString qmldir{"module Example\nFirstType 1.0 First.qml\nFirstTypeV2 2.2 "
                   "First.2.qml\nSecondType 2.1 OldSecond.qml\nSecondType 2.2 Second.qml\n"};
    QString qmlDocument1{"First{}"};
    QString qmlDocument2{"Second{}"};
    QString qmlDocument3{"Third{}"};
    auto qmlDocumentSourceId1 = sourcePathCache.sourceId("/path/First.qml");
    auto qmlDocumentSourceId2 = sourcePathCache.sourceId("/path/First.2.qml");
    auto qmlDocumentSourceId3 = sourcePathCache.sourceId("/path/Second.qml");
    Storage::Type firstType;
    firstType.prototype = Storage::ExportedType{"Object"};
    Storage::Type secondType;
    secondType.prototype = Storage::ExportedType{"Object2"};
    Storage::Type thirdType;
    thirdType.prototype = Storage::ExportedType{"Object3"};
    auto firstQmlDocumentSourceId = sourcePathCache.sourceId("/path/First.qml");
    ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/qmldir")))).WillByDefault(Return(qmldir));
    ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/First.qml"))))
        .WillByDefault(Return(qmlDocument1));
    ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/First.2.qml"))))
        .WillByDefault(Return(qmlDocument2));
    ON_CALL(fileSystemMock, contentAsQString(Eq(QString("/path/Second.qml"))))
        .WillByDefault(Return(qmlDocument3));
    ON_CALL(qmlDocumentParserMock, parse(qmlDocument1)).WillByDefault(Return(firstType));
    ON_CALL(qmlDocumentParserMock, parse(qmlDocument2)).WillByDefault(Return(secondType));
    ON_CALL(qmlDocumentParserMock, parse(qmlDocument3)).WillByDefault(Return(thirdType));

    EXPECT_CALL(projectStorageMock,
                synchronize(_,
                            _,
                            Contains(AllOf(IsStorageType(Storage::Module{"Example", 1},
                                                         "First.qml",
                                                         Storage::ExportedType{"Object"},
                                                         TypeAccessSemantics::Reference,
                                                         firstQmlDocumentSourceId),
                                           Field(&Storage::Type::exportedTypes,
                                                 ElementsAre(IsExportedType("FirstType"))))),
                            UnorderedElementsAre(qmlDirPathSourceId,
                                                 qmlDocumentSourceId1,
                                                 qmlDocumentSourceId2,
                                                 qmlDocumentSourceId3),
                            _));

    updater.update();
}

} // namespace
