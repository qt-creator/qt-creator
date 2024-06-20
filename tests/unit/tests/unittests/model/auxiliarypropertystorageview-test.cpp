// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <googletest.h>

#include <mocks/externaldependenciesmock.h>
#include <mocks/modelresourcemanagementmock.h>
#include <mocks/projectstoragemock.h>
#include <mocks/sourcepathcachemock.h>

#include <model/auxiliarypropertystorageview.h>

namespace {

using QmlDesigner::AuxiliaryDataKey;
using QmlDesigner::AuxiliaryDataType;
using QmlDesigner::ModelNode;

template<typename NameMatcher>
auto AuxiliaryProperty(AuxiliaryDataType type, const NameMatcher &nameMatcher, const QVariant &value)
{
    return Pair(AllOf(Field(&AuxiliaryDataKey::type, type),
                      Field(&AuxiliaryDataKey::name, nameMatcher)),
                value);
}

class AuxiliaryPropertyStorageView : public ::testing::Test
{
protected:
    struct StaticData
    {
        Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    };

    ~AuxiliaryPropertyStorageView() { view.resetForTestsOnly(); }

    static void SetUpTestSuite() { staticData = std::make_unique<StaticData>(); }

    static void TearDownTestSuite() { staticData.reset(); }

    inline static std::unique_ptr<StaticData> staticData;
    Sqlite::Database &database = staticData->database;
    NiceMock<SourcePathCacheMockWithPaths> pathCacheMock{"/path/foo.qml"};
    NiceMock<ProjectStorageMockWithQtQuick> projectStorageMock{pathCacheMock.sourceId, "/path"};
    NiceMock<ModelResourceManagementMock> resourceManagementMock;
    QmlDesigner::Imports imports = {QmlDesigner::Import::createLibraryImport("QtQuick")};
    QmlDesigner::Model model{{projectStorageMock, pathCacheMock},
                             "Item",
                             imports,
                             pathCacheMock.path.toQString(),
                             std::make_unique<ModelResourceManagementMockWrapper>(
                                 resourceManagementMock)};
    QmlDesigner::Model model2{{projectStorageMock, pathCacheMock},
                              "Item",
                              imports,
                              pathCacheMock.path.toQString(),
                              std::make_unique<ModelResourceManagementMockWrapper>(
                                  resourceManagementMock)};
    NiceMock<ExternalDependenciesMock> externalDependenciesMock;
    QmlDesigner::AuxiliaryPropertyStorageView view{database, externalDependenciesMock};
    ModelNode rootNode = model.rootModelNode();
    ModelNode rootNode2 = model2.rootModelNode();
    QmlDesigner::AuxiliaryDataKeyView persistentFooKey{AuxiliaryDataType::Persistent, "foo"};
    QmlDesigner::AuxiliaryDataKeyView temporaryFooKey{AuxiliaryDataType::Temporary, "foo"};
};

TEST_F(AuxiliaryPropertyStorageView, store_persistent_auxiliary_property)
{
    // arrange
    model.attachView(&view);
    rootNode.setIdWithoutRefactoring("root");
    rootNode2.setIdWithoutRefactoring("root");

    // act
    rootNode.setAuxiliaryData(persistentFooKey, "text");

    // assert
    model.detachView(&view);
    model2.attachView(&view);
    ASSERT_THAT(rootNode2.auxiliaryData(),
                Contains(AuxiliaryProperty(AuxiliaryDataType::Persistent, "foo", "text")));
}

TEST_F(AuxiliaryPropertyStorageView, store_color_auxiliary_property)
{
    // arrange
    model.attachView(&view);
    rootNode.setIdWithoutRefactoring("root");
    rootNode2.setIdWithoutRefactoring("root");
    QColor color{Qt::red};

    // act
    rootNode.setAuxiliaryData(persistentFooKey, color);

    // assert
    model.detachView(&view);
    model2.attachView(&view);
    ASSERT_THAT(rootNode2.auxiliaryData(),
                Contains(AuxiliaryProperty(AuxiliaryDataType::Persistent, "foo", color)));
}

TEST_F(AuxiliaryPropertyStorageView, do_not_store_temporary_auxiliary_property)
{
    // arrange
    model.attachView(&view);
    rootNode.setIdWithoutRefactoring("root");
    rootNode2.setIdWithoutRefactoring("root");

    // act
    rootNode.setAuxiliaryData(temporaryFooKey, "text");

    // assert
    model.detachView(&view);
    model2.attachView(&view);
    ASSERT_THAT(rootNode2.auxiliaryData(),
                Not(Contains(AuxiliaryProperty(AuxiliaryDataType::Temporary, "foo", "text"))));
}

TEST_F(AuxiliaryPropertyStorageView, do_not_load_node_without_id)
{
    // arrange
    model.attachView(&view);
    rootNode.setIdWithoutRefactoring("root");

    // act
    rootNode.setAuxiliaryData(persistentFooKey, "text");

    // assert
    model.detachView(&view);
    model2.attachView(&view);
    ASSERT_THAT(rootNode2.auxiliaryData(),
                Not(Contains(AuxiliaryProperty(AuxiliaryDataType::Persistent, "foo", "text"))));
}

TEST_F(AuxiliaryPropertyStorageView, do_not_store_node_without_id)
{
    // arrange
    model.attachView(&view);
    rootNode2.setIdWithoutRefactoring("root");

    // act
    rootNode.setAuxiliaryData(persistentFooKey, "text");

    // assert
    model.detachView(&view);
    model2.attachView(&view);
    ASSERT_THAT(rootNode2.auxiliaryData(),
                Not(Contains(AuxiliaryProperty(AuxiliaryDataType::Persistent, "foo", "text"))));
}

TEST_F(AuxiliaryPropertyStorageView, do_not_load_from_different_document)
{
    // arrange
    model2.setFileUrl(QUrl{"/path/foo2.qml"});
    model.attachView(&view);
    rootNode.setIdWithoutRefactoring("root");
    rootNode2.setIdWithoutRefactoring("root");

    // act
    rootNode.setAuxiliaryData(persistentFooKey, "text");

    // assert
    model.detachView(&view);
    model2.attachView(&view);
    ASSERT_THAT(rootNode2.auxiliaryData(),
                Not(Contains(AuxiliaryProperty(AuxiliaryDataType::Persistent, "foo", "text"))));
}

TEST_F(AuxiliaryPropertyStorageView, removing_node_erases_entries)
{
    // arrange
    auto node = model.createModelNode("Item");
    node.setIdWithoutRefactoring("node1");
    node.setAuxiliaryData(persistentFooKey, "some value");
    auto node2 = model2.createModelNode("Item");
    node2.setIdWithoutRefactoring("node1");
    model.attachView(&view);
    rootNode.setIdWithoutRefactoring("root");
    rootNode2.setIdWithoutRefactoring("root");

    // act
    node.destroy();

    // assert
    model.detachView(&view);
    model2.attachView(&view);
    ASSERT_THAT(node2.auxiliaryData(),
                Not(Contains(AuxiliaryProperty(AuxiliaryDataType::Persistent, "foo", "some value"))));
}

TEST_F(AuxiliaryPropertyStorageView, changing_node_id_is_saving_data_under_new_id)
{
    // arrange
    model.attachView(&view);
    rootNode.setIdWithoutRefactoring("root");
    rootNode2.setIdWithoutRefactoring("root2");
    rootNode.setAuxiliaryData(persistentFooKey, "text");
    model.detachView(&view);
    model.attachView(&view);

    // act
    rootNode.setIdWithoutRefactoring("root2");

    // assert
    model.detachView(&view);
    model2.attachView(&view);
    ASSERT_THAT(rootNode2.auxiliaryData(),
                Contains(AuxiliaryProperty(AuxiliaryDataType::Persistent, "foo", "text")));
}

TEST_F(AuxiliaryPropertyStorageView, changing_node_id_is_removing_data_from_old_id)
{
    // arrange
    model.attachView(&view);
    rootNode.setIdWithoutRefactoring("root");
    rootNode2.setIdWithoutRefactoring("root");
    rootNode.setAuxiliaryData(persistentFooKey, "text");
    model.detachView(&view);
    model.attachView(&view);

    // act
    rootNode.setIdWithoutRefactoring("root2");

    // assert
    model.detachView(&view);
    model2.attachView(&view);
    ASSERT_THAT(rootNode2.auxiliaryData(),
                Not(Contains(AuxiliaryProperty(AuxiliaryDataType::Persistent, "foo", "text"))));
}
} // namespace
