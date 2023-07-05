// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <googletest.h>

#include <mocks/projectstoragemock.h>
#include <mocks/sourcepathcachemock.h>

#include <model/modelutils.h>
#include <nodemetainfo.h>

namespace {

class ModelUtils : public ::testing::Test
{
protected:
    NiceMock<SourcePathCacheMockWithPaths> pathCacheMock{"/path/model.qml"};
    QmlDesigner::SourceId sourceId = pathCacheMock.createSourceId("/path/foo.qml");
    NiceMock<ProjectStorageMockWithQtQtuick> projectStorageMock{pathCacheMock.sourceId};
    QmlDesigner::ModuleId moduleId = projectStorageMock.moduleId("QtQuick");
    QmlDesigner::Model model{{projectStorageMock, pathCacheMock},
                             "Item",
                             {QmlDesigner::Import::createLibraryImport("QML"),
                              QmlDesigner::Import::createLibraryImport("QtQuick"),
                              QmlDesigner::Import::createLibraryImport("QtQml.Models")},
                             QUrl::fromLocalFile(pathCacheMock.path.toQString())};
};

TEST_F(ModelUtils, component_file_path)
{
    auto typeId = projectStorageMock.createType(moduleId,
                                                "Foo",
                                                QmlDesigner::Storage::TypeTraits::IsFileComponent,
                                                {},
                                                sourceId);
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    auto path = QmlDesigner::ModelUtils::componentFilePath(pathCacheMock, metaInfo);

    ASSERT_THAT(path, "/path/foo.qml");
}

TEST_F(ModelUtils, empty_component_file_path_for_non_file_component)
{
    auto typeId = projectStorageMock.createType(moduleId, "Foo", {}, {}, sourceId);
    QmlDesigner::NodeMetaInfo metaInfo{typeId, &projectStorageMock};

    auto path = QmlDesigner::ModelUtils::componentFilePath(pathCacheMock, metaInfo);

    ASSERT_THAT(path, IsEmpty());
}

TEST_F(ModelUtils, empty_component_file_path_for_invalid_meta_info)
{
    QmlDesigner::NodeMetaInfo metaInfo;

    auto path = QmlDesigner::ModelUtils::componentFilePath(pathCacheMock, metaInfo);

    ASSERT_THAT(path, IsEmpty());
}

TEST_F(ModelUtils, component_file_path_for_node)
{
    auto typeId = projectStorageMock.createType(moduleId,
                                                "Foo",
                                                QmlDesigner::Storage::TypeTraits::IsFileComponent,
                                                {},
                                                sourceId);
    projectStorageMock.createImportedTypeNameId(pathCacheMock.sourceId, "Foo", typeId);
    auto node = model.createModelNode("Foo");

    auto path = QmlDesigner::ModelUtils::componentFilePath(node);

    ASSERT_THAT(path, "/path/foo.qml");
}

TEST_F(ModelUtils, component_file_path_for_invalid_node_is_empty)
{
    auto path = QmlDesigner::ModelUtils::componentFilePath(QmlDesigner::ModelNode{});

    ASSERT_THAT(path, IsEmpty());
}

TEST_F(ModelUtils, component_file_path_for_node_without_metainfo_is_empty)
{
    auto typeId = projectStorageMock.createType(moduleId,
                                                "Foo",
                                                QmlDesigner::Storage::TypeTraits::IsFileComponent,
                                                {},
                                                sourceId);
    auto node = model.createModelNode("Foo");

    auto path = QmlDesigner::ModelUtils::componentFilePath(node);

    ASSERT_THAT(path, IsEmpty());
}

TEST_F(ModelUtils, component_file_path_for_non_file_component_node_is_empty)
{
    auto typeId = projectStorageMock.createType(moduleId, "Foo", {}, {}, sourceId);
    projectStorageMock.createImportedTypeNameId(pathCacheMock.sourceId, "Foo", typeId);
    auto node = model.createModelNode("Foo");

    auto path = QmlDesigner::ModelUtils::componentFilePath(node);

    ASSERT_THAT(path, IsEmpty());
}

} // namespace
