// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <googletest.h>

#include <mocks/abstractviewmock.h>
#include <mocks/projectstoragemock.h>
#include <mocks/sourcepathcachemock.h>
#include <model/modelutils.h>
#include <nodemetainfo.h>
#include <nodeproperty.h>

namespace {
using QmlDesigner::ModelNode;
using QmlDesigner::ModelNodes;

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
    projectStorageMock.createType(moduleId,
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

TEST_F(ModelUtils, find_lowest_common_ancestor)
{
    auto child1 = model.createModelNode("Item");
    auto child2 = model.createModelNode("Item");
    model.rootModelNode().defaultNodeAbstractProperty().reparentHere(child1);
    model.rootModelNode().defaultNodeAbstractProperty().reparentHere(child2);
    ModelNodes nodes{child1, child2};

    auto commonAncestor = QmlDesigner::ModelUtils::lowestCommonAncestor(nodes);

    ASSERT_THAT(commonAncestor, model.rootModelNode());
}

TEST_F(ModelUtils, lowest_common_ancestor_return_invalid_node_if_argument_is_invalid)
{
    auto child1 = model.createModelNode("Item");
    auto child2 = ModelNode{};
    model.rootModelNode().defaultNodeAbstractProperty().reparentHere(child1);
    ModelNodes nodes{child1, child2};

    auto commonAncestor = QmlDesigner::ModelUtils::lowestCommonAncestor(nodes);

    ASSERT_THAT(commonAncestor, Not(IsValid()));
}

TEST_F(ModelUtils, find_lowest_common_ancestor_when_one_of_the_nodes_is_parent)
{
    auto parentNode = model.createModelNode("Item");
    auto childNode = model.createModelNode("Item");
    parentNode.defaultNodeAbstractProperty().reparentHere(childNode);
    model.rootModelNode().defaultNodeAbstractProperty().reparentHere(parentNode);
    ModelNodes nodes{childNode, parentNode};

    auto commonAncestor = QmlDesigner::ModelUtils::lowestCommonAncestor(nodes);

    ASSERT_THAT(commonAncestor, parentNode);
}

TEST_F(ModelUtils, lowest_common_ancestor_for_uncle_and_nephew_should_return_the_grandFather)
{
    auto grandFatherNode = model.createModelNode("Item");
    auto fatherNode = model.createModelNode("Item");
    auto uncleNode = model.createModelNode("Item");
    auto nephewNode = model.createModelNode("Item");
    fatherNode.defaultNodeAbstractProperty().reparentHere(nephewNode);
    grandFatherNode.defaultNodeAbstractProperty().reparentHere(fatherNode);
    grandFatherNode.defaultNodeAbstractProperty().reparentHere(uncleNode);
    model.rootModelNode().defaultNodeAbstractProperty().reparentHere(grandFatherNode);
    ModelNodes nodes{uncleNode, nephewNode};

    auto commonAncestor = QmlDesigner::ModelUtils::lowestCommonAncestor(nodes);

    ASSERT_THAT(commonAncestor, grandFatherNode);
}

} // namespace
