// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <matchers/import-matcher.h>
#include <mocks/abstractviewmock.h>
#include <mocks/modelresourcemanagementmock.h>
#include <mocks/projectstoragemock.h>
#include <mocks/projectstorageobservermock.h>
#include <mocks/sourcepathcachemock.h>

#include <designercore/include/bindingproperty.h>
#include <designercore/include/itemlibraryentry.h>
#include <designercore/include/model.h>
#include <designercore/include/modelnode.h>
#include <designercore/include/nodeabstractproperty.h>
#include <designercore/include/nodelistproperty.h>
#include <designercore/include/nodemetainfo.h>
#include <designercore/include/nodeproperty.h>
#include <designercore/include/signalhandlerproperty.h>
#include <designercore/include/variantproperty.h>

namespace {
using QmlDesigner::AbstractProperty;
using QmlDesigner::ModelNode;
using QmlDesigner::ModelNodes;
using QmlDesigner::ModelResourceSet;
using QmlDesigner::Storage::ModuleKind;

MATCHER(IsSorted, std::string(negation ? "isn't sorted" : "is sorted"))
{
    using std::begin;
    using std::end;
    return std::is_sorted(begin(arg), end(arg));
}

template<typename PropertiesMatcher, typename ExtraFilePathsMatcher>
auto IsItemLibraryEntry(QmlDesigner::TypeId typeId,
                        QByteArrayView typeName,
                        QStringView name,
                        QStringView iconPath,
                        QStringView category,
                        QStringView import,
                        QStringView toolTip,
                        QStringView templatePath,
                        PropertiesMatcher propertiesMatcher,
                        ExtraFilePathsMatcher extraFilePathsMatcher)
{
    using QmlDesigner::ItemLibraryEntry;
    return AllOf(Property("typeId", &ItemLibraryEntry::typeId, typeId),
                 Property("typeName", &ItemLibraryEntry::typeName, typeName),
                 Property("name", &ItemLibraryEntry::name, name),
                 Property("libraryEntryIconPath", &ItemLibraryEntry::libraryEntryIconPath, iconPath),
                 Property("category", &ItemLibraryEntry::category, category),
                 Property("requiredImport", &ItemLibraryEntry::requiredImport, import),
                 Property("toolTip", &ItemLibraryEntry::toolTip, toolTip),
                 Property("qmlSource", &ItemLibraryEntry::qmlSource, templatePath),
                 Property("properties", &ItemLibraryEntry::properties, propertiesMatcher),
                 Property("extraFilePath", &ItemLibraryEntry::extraFilePaths, extraFilePathsMatcher));
}

MATCHER_P3(IsItemLibraryProperty,
           name,
           type,
           value,
           std::string(negation ? "isn't " : "is ")
               + PrintToString(QmlDesigner::PropertyContainer(name, type, value)))
{
    const QmlDesigner::PropertyContainer &property = arg;

    return property.name() == name && property.type() == type && property.value() == value;
}
class Model : public ::testing::Test
{
protected:
    Model()
    {
        model.attachView(&viewMock);
        rootNode = viewMock.rootModelNode();
        ON_CALL(resourceManagementMock, removeNodes(_, _)).WillByDefault([](auto nodes, auto) {
            return ModelResourceSet{std::move(nodes), {}, {}};
        });
        ON_CALL(resourceManagementMock, removeProperties(_, _))
            .WillByDefault([](auto properties, auto) {
                return ModelResourceSet{{}, std::move(properties), {}};
            });
    }

    ~Model() { model.detachView(&viewMock); }

    auto createNodeWithParent(const ModelNode &parentNode, const QString &id = {})
    {
        auto node = parentNode.view()->createModelNode("QtQuick.Item");
        node.setIdWithoutRefactoring(id);
        parentNode.defaultNodeAbstractProperty().reparentHere(node);

        return node;
    }

    auto createProperty(const ModelNode &parentNode, QmlDesigner::PropertyName name)
    {
        auto property = parentNode.variantProperty(name);
        property.setValue(4);
        return property;
    }

    auto createBindingProperty(const ModelNode &parentNode,
                               QmlDesigner::PropertyName name,
                               const QString &expression = "foo")
    {
        auto property = parentNode.bindingProperty(name);
        property.setExpression(expression);
        return property;
    }

protected:
    NiceMock<SourcePathCacheMockWithPaths> pathCacheMock{"/path/foo.qml"};
    NiceMock<ProjectStorageMockWithQtQuick> projectStorageMock{pathCacheMock.sourceId, "/path"};
    NiceMock<ModelResourceManagementMock> resourceManagementMock;
    QmlDesigner::Imports imports = {QmlDesigner::Import::createLibraryImport("QtQuick")};
    QmlDesigner::Model model{{projectStorageMock, pathCacheMock},
                             "Item",
                             imports,
                             QUrl::fromLocalFile(pathCacheMock.path.toQString()),
                             std::make_unique<ModelResourceManagementMockWrapper>(
                                 resourceManagementMock)};
    NiceMock<AbstractViewMock> viewMock;
    QmlDesigner::SourceId filePathId = pathCacheMock.sourceId;
    QmlDesigner::ModuleId qtQuickModuleId = projectStorageMock.moduleId("QtQuick",
                                                                        ModuleKind::QmlLibrary);
    QmlDesigner::TypeId itemTypeId = projectStorageMock.typeId(qtQuickModuleId,
                                                               "Item",
                                                               QmlDesigner::Storage::Version{});
    QmlDesigner::ImportedTypeNameId itemTypeNameId = projectStorageMock.createImportedTypeNameId(
        filePathId, "Item", itemTypeId);
    ModelNode rootNode;
};

class Model_Creation : public Model
{};

TEST_F(Model_Creation, root_node_has_item_type_name)
{
    auto model = QmlDesigner::Model::create({projectStorageMock, pathCacheMock},
                                            "Item",
                                            imports,
                                            QUrl::fromLocalFile(pathCacheMock.path.toQString()),
                                            std::make_unique<ModelResourceManagementMockWrapper>(
                                                resourceManagementMock));

    ASSERT_THAT(model->rootModelNode().type(), Eq("Item"));
}

TEST_F(Model_Creation, root_node_has_item_meta_info)
{
    auto model = QmlDesigner::Model::create({projectStorageMock, pathCacheMock},
                                            "Item",
                                            imports,
                                            QUrl::fromLocalFile(pathCacheMock.path.toQString()),
                                            std::make_unique<ModelResourceManagementMockWrapper>(
                                                resourceManagementMock));

    ASSERT_THAT(model->rootModelNode().metaInfo(), model->qtQuickItemMetaInfo());
}

TEST_F(Model_Creation, file_url)
{
    auto model = QmlDesigner::Model::create({projectStorageMock, pathCacheMock},
                                            "Item",
                                            imports,
                                            QUrl::fromLocalFile(pathCacheMock.path.toQString()),
                                            std::make_unique<ModelResourceManagementMockWrapper>(
                                                resourceManagementMock));

    ASSERT_THAT(model->fileUrl().toLocalFile(), Eq(pathCacheMock.path.toQString()));
}

TEST_F(Model_Creation, file_url_source_id)
{
    auto model = QmlDesigner::Model::create({projectStorageMock, pathCacheMock},
                                            "Item",
                                            imports,
                                            QUrl::fromLocalFile(pathCacheMock.path.toQString()),
                                            std::make_unique<ModelResourceManagementMockWrapper>(
                                                resourceManagementMock));

    ASSERT_THAT(model->fileUrlSourceId(), pathCacheMock.sourceId);
}

TEST_F(Model_Creation, imports)
{
    auto model = QmlDesigner::Model::create({projectStorageMock, pathCacheMock},
                                            "Item",
                                            imports,
                                            QUrl::fromLocalFile(pathCacheMock.path.toQString()),
                                            std::make_unique<ModelResourceManagementMockWrapper>(
                                                resourceManagementMock));

    ASSERT_THAT(model->imports(), UnorderedElementsAreArray(imports));
}

class Model_ResourceManagment : public Model
{};

TEST_F(Model_ResourceManagment, model_node_destroy_is_calling_model_resource_management_remove_node)
{
    auto node = createNodeWithParent(rootNode);

    EXPECT_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model));

    node.destroy();
}

TEST_F(Model_ResourceManagment,
       model_node_remove_propery_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.removeProperty("foo");
}

TEST_F(Model_ResourceManagment,
       node_abstract_property_reparent_here_is_calling_model_resource_management_remove_property)
{
    auto node = createNodeWithParent(rootNode);
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.nodeListProperty("foo").reparentHere(node);
}

TEST_F(Model_ResourceManagment,
       node_property_set_model_node_is_calling_model_resource_management_remove_property)
{
    auto node = createNodeWithParent(rootNode);
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.nodeProperty("foo").setModelNode(node);
}

TEST_F(Model_ResourceManagment,
       variant_property_set_value_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.variantProperty("foo").setValue(7);
}

TEST_F(Model_ResourceManagment,
       variant_property_set_dynamic_type_name_and_enumeration_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.variantProperty("foo").setDynamicTypeNameAndEnumeration("int", "Ha");
}

TEST_F(Model_ResourceManagment,
       variant_property_set_dynamic_type_name_and_value_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.variantProperty("foo").setDynamicTypeNameAndValue("int", 7);
}

TEST_F(Model_ResourceManagment,
       binding_property_set_expression_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.bindingProperty("foo").setExpression("blah");
}

TEST_F(Model_ResourceManagment,
       binding_property_set_dynamic_type_name_and_expression_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.bindingProperty("foo").setDynamicTypeNameAndExpression("int", "blah");
}

TEST_F(Model_ResourceManagment,
       signal_handler_property_set_source_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.signalHandlerProperty("foo").setSource("blah");
}

TEST_F(Model_ResourceManagment,
       signal_declaration_property_set_signature_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.signalDeclarationProperty("foo").setSignature("blah");
}

TEST_F(Model_ResourceManagment, model_node_destroy_is_calling_abstract_view_node_about_to_be_removed)
{
    auto node = createNodeWithParent(rootNode);
    auto node2 = createNodeWithParent(rootNode);
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node, node2}, {}, {}}));

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(Eq(node)));
    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(Eq(node2)));

    node.destroy();
}

TEST_F(Model_ResourceManagment, model_node_destroy_is_calling_abstract_view_node_removed)
{
    auto node = createNodeWithParent(rootNode);
    auto node2 = createNodeWithParent(rootNode);
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node, node2}, {}, {}}));

    EXPECT_CALL(viewMock, nodeRemoved(Eq(node), _, _));
    EXPECT_CALL(viewMock, nodeRemoved(Eq(node2), _, _));

    node.destroy();
}

TEST_F(Model_ResourceManagment,
       model_node_destroy_is_calling_abstract_view_node_removed_with_valid_nodes)
{
    auto node = createNodeWithParent(rootNode);
    auto node2 = createNodeWithParent(rootNode);
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node, node2, ModelNode{}}, {}, {}}));

    EXPECT_CALL(viewMock, nodeRemoved(Eq(node), _, _));
    EXPECT_CALL(viewMock, nodeRemoved(Eq(node2), _, _));

    node.destroy();
}

TEST_F(Model_ResourceManagment,
       model_node_destroy_is_calling_abstract_view_properties_about_to_be_removed)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createProperty(rootNode, "foo");
    auto property2 = createProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node}, {property, property2}, {}}));

    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(UnorderedElementsAre(property, property2)));

    node.destroy();
}

TEST_F(Model_ResourceManagment, model_node_destroy_is_calling_abstract_view_properties_removed)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createProperty(rootNode, "foo");
    auto property2 = createProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node}, {property, property2}, {}}));

    EXPECT_CALL(viewMock, propertiesRemoved(UnorderedElementsAre(property, property2)));

    node.destroy();
}

TEST_F(Model_ResourceManagment,
       model_node_destroy_is_calling_abstract_view_properties_removed_only_with_valid_properties)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createProperty(rootNode, "foo");
    auto property2 = createProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node}, {property, property2, {}}, {}}));

    EXPECT_CALL(viewMock, propertiesRemoved(UnorderedElementsAre(property, property2)));

    node.destroy();
}

TEST_F(Model_ResourceManagment,
       model_node_destroy_is_calling_abstract_view_binding_properties_about_to_be_changed)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createBindingProperty(rootNode, "foo");
    auto property2 = createBindingProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node}, {}, {{property, "yi"}, {property2, "er"}}}));

    EXPECT_CALL(viewMock,
                bindingPropertiesAboutToBeChanged(UnorderedElementsAre(property, property2)));

    node.destroy();
}

TEST_F(Model_ResourceManagment, model_node_destroy_is_calling_abstract_view_binding_properties_changed)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createBindingProperty(rootNode, "foo");
    auto property2 = createBindingProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node}, {}, {{property, "yi"}, {property2, "er"}}}));

    EXPECT_CALL(viewMock, bindingPropertiesChanged(UnorderedElementsAre(property, property2), _));

    node.destroy();
}

TEST_F(Model_ResourceManagment, model_node_destroy_is_changing_binding_property_expression)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createBindingProperty(rootNode, "foo");
    auto property2 = createBindingProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node}, {}, {{property, "yi"}, {property2, "er"}}}));

    node.destroy();

    ASSERT_THAT(property.expression(), "yi");
    ASSERT_THAT(property2.expression(), "er");
}

TEST_F(Model_ResourceManagment, model_node_destroy_is_only_changing_existing_binding_property)
{
    auto node = createNodeWithParent(rootNode);
    auto property = rootNode.bindingProperty("foo");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{}, {}, {{property, "yi"}}}));

    node.destroy();

    ASSERT_FALSE(rootNode.hasBindingProperty("foo"));
}

TEST_F(Model_ResourceManagment,
       model_node_destroy_is_calling_abstract_view_binding_properties_changed_only_with_existing_properties)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createBindingProperty(rootNode, "foo");
    auto property2 = rootNode.bindingProperty("bar");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(
            ModelResourceSet{{node}, {}, {{property, "yi"}, {property2, "er"}, {{}, "san"}}}));

    EXPECT_CALL(viewMock, bindingPropertiesChanged(UnorderedElementsAre(property), _));

    node.destroy();
}

TEST_F(Model_ResourceManagment,
       model_node_remove_property_is_calling_abstract_view_node_about_to_be_removed)
{
    auto property = createProperty(rootNode, "foo");
    auto node = createNodeWithParent(rootNode);
    auto node2 = createNodeWithParent(rootNode);
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(Return(ModelResourceSet{{node, node2}, {property}, {}}));

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(Eq(node)));
    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(Eq(node2)));

    rootNode.removeProperty("foo");
}

TEST_F(Model_ResourceManagment, model_node_remove_property_is_calling_abstract_view_node_removed)
{
    auto property = createProperty(rootNode, "foo");
    auto node = createNodeWithParent(rootNode);
    auto node2 = createNodeWithParent(rootNode);
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(Return(ModelResourceSet{{node, node2}, {property}, {}}));

    EXPECT_CALL(viewMock, nodeRemoved(Eq(node), _, _));
    EXPECT_CALL(viewMock, nodeRemoved(Eq(node2), _, _));

    rootNode.removeProperty("foo");
}

TEST_F(Model_ResourceManagment,
       model_node_remove_property_is_calling_abstract_view_node_removed_with_valid_nodes)
{
    auto property = createProperty(rootNode, "foo");
    auto node = createNodeWithParent(rootNode);
    auto node2 = createNodeWithParent(rootNode);
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(Return(ModelResourceSet{{node, node2, ModelNode{}}, {property}, {}}));

    EXPECT_CALL(viewMock, nodeRemoved(Eq(node), _, _));
    EXPECT_CALL(viewMock, nodeRemoved(Eq(node2), _, _));

    rootNode.removeProperty("foo");
}

TEST_F(Model_ResourceManagment,
       model_node_remove_property_is_calling_abstract_view_properties_about_to_be_removed)
{
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(Return(ModelResourceSet{{}, {property, property2}, {}}));

    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(UnorderedElementsAre(property, property2)));

    rootNode.removeProperty("yi");
}

TEST_F(Model_ResourceManagment, model_node_remove_property_is_calling_abstract_view_properties_removed)
{
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(Return(ModelResourceSet{{}, {property, property2}, {}}));

    EXPECT_CALL(viewMock, propertiesRemoved(UnorderedElementsAre(property, property2)));

    rootNode.removeProperty("yi");
}

TEST_F(Model_ResourceManagment,
       model_node_remove_property_is_calling_abstract_view_properties_removed_only_with_valid_properties)
{
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(Return(ModelResourceSet{{}, {property, property2, {}}, {}}));

    EXPECT_CALL(viewMock, propertiesRemoved(UnorderedElementsAre(property, property2)));

    rootNode.removeProperty("yi");
}

TEST_F(Model_ResourceManagment,
       model_node_remove_property_is_calling_abstract_view_binding_properties_about_to_be_changed)
{
    auto property = createProperty(rootNode, "yi");
    auto property1 = createBindingProperty(rootNode, "foo");
    auto property2 = createBindingProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(
            Return(ModelResourceSet{{}, {property}, {{property1, "yi"}, {property2, "er"}}}));

    EXPECT_CALL(viewMock, bindingPropertiesAboutToBeChanged(ElementsAre(property1, property2)));

    rootNode.removeProperty("yi");
}

TEST_F(Model_ResourceManagment,
       model_node_remove_property_is_calling_abstract_view_binding_properties_changed)
{
    auto property = createProperty(rootNode, "yi");
    auto property1 = createBindingProperty(rootNode, "foo");
    auto property2 = createBindingProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(
            Return(ModelResourceSet{{}, {property}, {{property1, "yi"}, {property2, "er"}}}));

    EXPECT_CALL(viewMock, bindingPropertiesChanged(ElementsAre(property1, property2), _));

    rootNode.removeProperty("yi");
}

TEST_F(Model_ResourceManagment,
       model_node_remove_property_is_calling_abstract_view_binding_properties_changed_only_with_valid_properties)
{
    auto property = createProperty(rootNode, "yi");
    auto property1 = createBindingProperty(rootNode, "foo");
    auto property2 = createBindingProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(
            Return(ModelResourceSet{{}, {property}, {{property1, "yi"}, {property2, "er"}, {}}}));

    EXPECT_CALL(viewMock, bindingPropertiesChanged(ElementsAre(property1, property2), _));

    rootNode.removeProperty("yi");
}

TEST_F(Model_ResourceManagment, by_default_remove_model_node_removes_node)
{
    QmlDesigner::Model newModel{{projectStorageMock, pathCacheMock}, "QtQuick.Item"};
    NiceMock<AbstractViewMock> viewMock;
    newModel.attachView(&viewMock);
    auto node = createNodeWithParent(viewMock.rootModelNode());

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(Eq(node)));

    node.destroy();
}

TEST_F(Model_ResourceManagment, by_default_remove_properties_removes_property)
{
    QmlDesigner::Model newModel{{projectStorageMock, pathCacheMock}, "QtQuick.Item"};
    NiceMock<AbstractViewMock> viewMock;
    newModel.attachView(&viewMock);
    rootNode = viewMock.rootModelNode();
    auto property = createProperty(rootNode, "yi");

    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(ElementsAre(Eq(property))));

    rootNode.removeProperty("yi");
}

TEST_F(Model_ResourceManagment, by_default_remove_model_node_in_factory_method_calls_removes_node)
{
    model.detachView(&viewMock);
    auto newModel = QmlDesigner::Model::create({projectStorageMock, pathCacheMock},
                                               "Item",
                                               imports,
                                               pathCacheMock.path.toQString());
    newModel->attachView(&viewMock);
    auto node = createNodeWithParent(viewMock.rootModelNode());

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(Eq(node)));

    node.destroy();
}

TEST_F(Model_ResourceManagment, by_default_remove_properties_in_factory_method_calls_remove_property)
{
    model.detachView(&viewMock);
    auto newModel = QmlDesigner::Model::create({projectStorageMock, pathCacheMock},
                                               "Item",
                                               imports,
                                               pathCacheMock.path.toQString());
    newModel->attachView(&viewMock);
    rootNode = viewMock.rootModelNode();
    auto property = createProperty(rootNode, "yi");

    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(ElementsAre(Eq(property))));

    rootNode.removeProperty("yi");
}

TEST_F(Model_ResourceManagment, remove_model_nodes)
{
    auto node = createNodeWithParent(rootNode, "yi");
    auto node2 = createNodeWithParent(rootNode, "er");

    EXPECT_CALL(resourceManagementMock,
                removeNodes(AllOf(UnorderedElementsAre(node, node2), IsSorted()), &model));

    model.removeModelNodes({node, node2});
}

TEST_F(Model_ResourceManagment, remove_model_nodes_filters_invalid_model_nodes)
{
    auto node = createNodeWithParent(rootNode, "yi");

    EXPECT_CALL(resourceManagementMock, removeNodes(UnorderedElementsAre(node), &model));

    model.removeModelNodes({{}, node});
}

TEST_F(Model_ResourceManagment, remove_model_nodes_for_only_invalid_model_nodes_does_nothing)
{
    EXPECT_CALL(resourceManagementMock, removeNodes(_, _)).Times(0);

    model.removeModelNodes({{}});
}

TEST_F(Model_ResourceManagment, remove_model_nodes_reverse)
{
    auto node = createNodeWithParent(rootNode, "yi");
    auto node2 = createNodeWithParent(rootNode, "er");

    EXPECT_CALL(resourceManagementMock,
                removeNodes(AllOf(UnorderedElementsAre(node, node2), IsSorted()), &model));

    model.removeModelNodes({node2, node});
}

TEST_F(Model_ResourceManagment, remove_model_nodes_calls_notifier)
{
    auto node = createNodeWithParent(rootNode, "yi");
    auto node2 = createNodeWithParent(rootNode, "er");
    auto property = createProperty(rootNode, "foo");
    auto property2 = createBindingProperty(rootNode, "bar");
    auto property3 = createProperty(rootNode, "oh");

    ON_CALL(resourceManagementMock,
            removeNodes(AllOf(UnorderedElementsAre(node, node2), IsSorted()), &model))
        .WillByDefault(
            Return(ModelResourceSet{{node, node2}, {property, property3}, {{property2, "bar"}}}));

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(node));
    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(node2));
    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(UnorderedElementsAre(property, property3)));
    EXPECT_CALL(viewMock, bindingPropertiesChanged(ElementsAre(property2), _));

    model.removeModelNodes({node, node2});
}

TEST_F(Model_ResourceManagment, remove_model_nodes_bypasses_model_resource_management)
{
    auto node = createNodeWithParent(rootNode, "yi");
    auto node2 = createNodeWithParent(rootNode, "er");
    auto property = createProperty(rootNode, "foo");
    auto property2 = createBindingProperty(rootNode, "bar");
    auto property3 = createProperty(rootNode, "oh");
    ON_CALL(resourceManagementMock,
            removeNodes(AllOf(UnorderedElementsAre(node, node2), IsSorted()), &model))
        .WillByDefault(
            Return(ModelResourceSet{{node, node2}, {property, property3}, {{property2, "bar"}}}));

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(node));
    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(node2));
    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(_)).Times(0);
    EXPECT_CALL(viewMock, bindingPropertiesChanged(_, _)).Times(0);

    model.removeModelNodes({node, node2}, QmlDesigner::BypassModelResourceManagement::Yes);
}

TEST_F(Model_ResourceManagment, by_default_remove_model_nodes_in_factory_method_calls_removes_node)
{
    QmlDesigner::Model newModel{{projectStorageMock, pathCacheMock}, "QtQuick.Item"};
    NiceMock<AbstractViewMock> viewMock;
    newModel.attachView(&viewMock);
    rootNode = viewMock.rootModelNode();
    auto node = createNodeWithParent(rootNode, "yi");
    auto node2 = createNodeWithParent(rootNode, "er");

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(node));
    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(node2));

    newModel.removeModelNodes({node, node2});
}

TEST_F(Model_ResourceManagment, remove_properties)
{
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");

    EXPECT_CALL(resourceManagementMock,
                removeProperties(AllOf(UnorderedElementsAre(property, property2), IsSorted()), &model));

    model.removeProperties({property, property2});
}

TEST_F(Model_ResourceManagment, remove_properties_filters_invalid_properties)
{
    auto property = createProperty(rootNode, "yi");

    EXPECT_CALL(resourceManagementMock, removeProperties(UnorderedElementsAre(property), &model));

    model.removeProperties({{}, property});
}

TEST_F(Model_ResourceManagment, remove_properties_for_only_invalid_properties_does_nothing)
{
    EXPECT_CALL(resourceManagementMock, removeProperties(_, _)).Times(0);

    model.removeProperties({{}});
}

TEST_F(Model_ResourceManagment, remove_properties_reverse)
{
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");

    EXPECT_CALL(resourceManagementMock,
                removeProperties(AllOf(UnorderedElementsAre(property, property2), IsSorted()), &model));

    model.removeProperties({property2, property});
}

TEST_F(Model_ResourceManagment, remove_properties_calls_notifier)
{
    auto node = createNodeWithParent(rootNode, "yi");
    auto node2 = createNodeWithParent(rootNode, "er");
    auto property = createProperty(rootNode, "foo");
    auto property2 = createBindingProperty(rootNode, "bar");
    auto property3 = createProperty(rootNode, "oh");

    ON_CALL(resourceManagementMock,
            removeProperties(AllOf(UnorderedElementsAre(property, property3), IsSorted()), &model))
        .WillByDefault(
            Return(ModelResourceSet{{node, node2}, {property, property3}, {{property2, "bar"}}}));

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(node));
    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(node2));
    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(UnorderedElementsAre(property, property3)));
    EXPECT_CALL(viewMock, bindingPropertiesChanged(ElementsAre(property2), _));

    model.removeProperties({property, property3});
}

TEST_F(Model_ResourceManagment, remove_properties_bypasses_model_resource_management)
{
    auto node = createNodeWithParent(rootNode, "yi");
    auto node2 = createNodeWithParent(rootNode, "er");
    auto property = createProperty(rootNode, "foo");
    auto property2 = createBindingProperty(rootNode, "bar");
    auto property3 = createProperty(rootNode, "oh");
    ON_CALL(resourceManagementMock,
            removeProperties(AllOf(UnorderedElementsAre(property, property3), IsSorted()), &model))
        .WillByDefault(
            Return(ModelResourceSet{{node, node2}, {property, property3}, {{property2, "bar"}}}));

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(_)).Times(0);
    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(UnorderedElementsAre(property, property3)));
    EXPECT_CALL(viewMock, bindingPropertiesChanged(_, _)).Times(0);

    model.removeProperties({property, property3}, QmlDesigner::BypassModelResourceManagement::Yes);
}

TEST_F(Model_ResourceManagment, by_default_remove_properties_in_factory_method_calls_removes_properties)
{
    model.detachView(&viewMock);
    QmlDesigner::Model newModel{{projectStorageMock, pathCacheMock}, "QtQuick.Item"};
    newModel.attachView(&viewMock);
    rootNode = viewMock.rootModelNode();
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");

    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(UnorderedElementsAre(property, property2)));

    newModel.removeProperties({property, property2});
}

class Model_Imports : public Model
{};

TEST_F(Model_Imports, change_imports_is_synchronizing_imports_with_project_storage)
{
    QmlDesigner::SourceId directoryPathId = QmlDesigner::SourceId::create(2);
    ON_CALL(pathCacheMock, sourceId(Eq("/path/foo/."))).WillByDefault(Return(directoryPathId));
    auto qtQuickModuleId = projectStorageMock.moduleId("QtQuick", ModuleKind::QmlLibrary);
    auto qtQmlModelsModuleId = projectStorageMock.moduleId("QtQml.Models", ModuleKind::QmlLibrary);
    auto localPathModuleId = projectStorageMock.moduleId("/path", ModuleKind::PathLibrary);
    auto qtQuickImport = QmlDesigner::Import::createLibraryImport("QtQuick", "2.1");
    auto qtQmlModelsImport = QmlDesigner::Import::createLibraryImport("QtQml.Models");
    auto directoryImport = QmlDesigner::Import::createFileImport("foo");

    EXPECT_CALL(projectStorageMock,
                synchronizeDocumentImports(
                    UnorderedElementsAre(IsImport(qtQuickModuleId, filePathId, 2, 1),
                                         IsImport(qtQmlModelsModuleId, filePathId, -1, -1),
                                         IsImport(localPathModuleId, filePathId, -1, -1)),
                    filePathId));

    model.changeImports({qtQuickImport, qtQmlModelsImport}, {});
}

TEST_F(Model_Imports,
       change_imports_is_not_synchronizing_imports_with_project_storage_if_no_new_imports_are_added)
{
    QmlDesigner::SourceId directoryPathId = QmlDesigner::SourceId::create(2);
    ON_CALL(pathCacheMock, sourceId(Eq("/path/foo/."))).WillByDefault(Return(directoryPathId));
    auto qtQuickImport = QmlDesigner::Import::createLibraryImport("QtQuick", "2.1");
    auto qtQmlModelsImport = QmlDesigner::Import::createLibraryImport("QtQml.Models");
    auto directoryImport = QmlDesigner::Import::createFileImport("foo");
    model.changeImports({qtQuickImport, qtQmlModelsImport}, {});

    EXPECT_CALL(projectStorageMock, synchronizeDocumentImports(_, _)).Times(0);

    model.changeImports({qtQuickImport, qtQmlModelsImport}, {});
}

TEST_F(Model_Imports, change_imports_is_adding_import_in_project_storage)
{
    QmlDesigner::SourceId directoryPathId = QmlDesigner::SourceId::create(2);
    ON_CALL(pathCacheMock, sourceId(Eq("/path/foo/."))).WillByDefault(Return(directoryPathId));
    auto qtQuickModuleId = projectStorageMock.moduleId("QtQuick", ModuleKind::QmlLibrary);
    auto qtQmlModelsModuleId = projectStorageMock.moduleId("QtQml.Models", ModuleKind::QmlLibrary);
    auto localPathModuleId = projectStorageMock.moduleId("/path", ModuleKind::PathLibrary);
    auto qtQuickImport = QmlDesigner::Import::createLibraryImport("QtQuick", "2.1");
    auto qtQmlModelsImport = QmlDesigner::Import::createLibraryImport("QtQml.Models");
    auto directoryImport = QmlDesigner::Import::createFileImport("foo");
    model.changeImports({qtQmlModelsImport}, {});

    EXPECT_CALL(projectStorageMock,
                synchronizeDocumentImports(
                    UnorderedElementsAre(IsImport(qtQuickModuleId, filePathId, 2, 1),
                                         IsImport(qtQmlModelsModuleId, filePathId, -1, -1),
                                         IsImport(localPathModuleId, filePathId, -1, -1)),
                    filePathId));

    model.changeImports({qtQuickImport}, {});
}

TEST_F(Model_Imports, change_imports_is_removing_import_in_project_storage)
{
    QmlDesigner::SourceId directoryPathId = QmlDesigner::SourceId::create(2);
    ON_CALL(pathCacheMock, sourceId(Eq("/path/foo/."))).WillByDefault(Return(directoryPathId));
    auto qtQmlModelsModuleId = projectStorageMock.moduleId("QtQml.Models", ModuleKind::QmlLibrary);
    auto localPathModuleId = projectStorageMock.moduleId("/path", ModuleKind::PathLibrary);
    auto qtQuickImport = QmlDesigner::Import::createLibraryImport("QtQuick", "2.1");
    auto qtQmlModelsImport = QmlDesigner::Import::createLibraryImport("QtQml.Models");
    auto directoryImport = QmlDesigner::Import::createFileImport("foo");
    model.changeImports({qtQuickImport, qtQmlModelsImport}, {});

    EXPECT_CALL(projectStorageMock,
                synchronizeDocumentImports(
                    UnorderedElementsAre(IsImport(qtQmlModelsModuleId, filePathId, -1, -1),
                                         IsImport(localPathModuleId, filePathId, -1, -1)),
                    filePathId));

    model.changeImports({}, {qtQuickImport});
}

TEST_F(Model_Imports,
       change_imports_is_not_removing_import_in_project_storage_if_import_is_not_in_model_imports)
{
    QmlDesigner::SourceId directoryPathId = QmlDesigner::SourceId::create(2);
    ON_CALL(pathCacheMock, sourceId(Eq("/path/foo/."))).WillByDefault(Return(directoryPathId));
    auto qtQuickImport = QmlDesigner::Import::createLibraryImport("QtQuick", "2.1");
    auto qtQmlModelsImport = QmlDesigner::Import::createLibraryImport("QtQml.Models");
    auto directoryImport = QmlDesigner::Import::createFileImport("foo");
    model.changeImports({qtQuickImport}, {});

    EXPECT_CALL(projectStorageMock, synchronizeDocumentImports(_, _)).Times(0);

    model.changeImports({}, {qtQmlModelsImport});
}

TEST_F(Model_Imports, change_imports_is_changing_import_version_with_project_storage)
{
    QmlDesigner::SourceId directoryPathId = QmlDesigner::SourceId::create(2);
    ON_CALL(pathCacheMock, sourceId(Eq("/path/foo/."))).WillByDefault(Return(directoryPathId));
    auto qtQuickModuleId = projectStorageMock.moduleId("QtQuick", ModuleKind::QmlLibrary);
    auto qtQmlModelsModuleId = projectStorageMock.moduleId("QtQml.Models", ModuleKind::QmlLibrary);
    auto localPathModuleId = projectStorageMock.moduleId("/path", ModuleKind::PathLibrary);
    auto qtQuickImport = QmlDesigner::Import::createLibraryImport("QtQuick", "2.1");
    auto qtQmlModelsImport = QmlDesigner::Import::createLibraryImport("QtQml.Models");
    auto directoryImport = QmlDesigner::Import::createFileImport("foo");
    model.changeImports({qtQuickImport, qtQmlModelsImport}, {});
    qtQuickImport = QmlDesigner::Import::createLibraryImport("QtQuick", "3.1");

    EXPECT_CALL(projectStorageMock,
                synchronizeDocumentImports(
                    UnorderedElementsAre(IsImport(qtQuickModuleId, filePathId, 3, 1),
                                         IsImport(qtQmlModelsModuleId, filePathId, -1, -1),
                                         IsImport(localPathModuleId, filePathId, -1, -1)),
                    filePathId));

    model.changeImports({qtQuickImport}, {});
}

class Model_Node : public Model
{};

TEST_F(Model_Node, create_model_node_has_meta_info)
{
    auto node = model.createModelNode("Item");

    ASSERT_THAT(node.metaInfo(), model.qtQuickItemMetaInfo());
}

TEST_F(Model_Node, create_qualified_model_node_has_meta_info)
{
    auto qtQmlModelsImport = QmlDesigner::Import::createLibraryImport("QtQml.Models", "", "Foo");
    auto qtQmlModelsModulesId = projectStorageMock.moduleId("QtQml.Models", ModuleKind::QmlLibrary);
    auto importId = projectStorageMock.createImportId(qtQmlModelsModulesId, filePathId);
    auto listModelTypeId = projectStorageMock.typeId(qtQmlModelsModulesId,
                                                     "ListModel",
                                                     QmlDesigner::Storage::Version{});
    projectStorageMock.createImportedTypeNameId(importId, "ListModel", listModelTypeId);
    model.changeImports({qtQmlModelsImport}, {});

    auto node = model.createModelNode("Foo.ListModel");

    ASSERT_THAT(node.metaInfo(), model.qtQmlModelsListModelMetaInfo());
}

TEST_F(Model_Node, change_root_node_type_changes_meta_info)
{
    projectStorageMock.createImportedTypeNameId(filePathId,
                                                "QtObject",
                                                model.qmlQtObjectMetaInfo().id());

    model.changeRootNodeType("QtObject");

    ASSERT_THAT(rootNode.metaInfo(), model.qmlQtObjectMetaInfo());
}

class Model_MetaInfo : public Model
{};

TEST_F(Model_MetaInfo, get_meta_info_for_type_name)
{
    auto meta_info = model.metaInfo("QtObject");

    ASSERT_THAT(meta_info, model.qmlQtObjectMetaInfo());
}

TEST_F(Model_MetaInfo, meta_info_of_not_existing_type_is_invalid)
{
    auto meta_info = model.metaInfo("Foo");

    ASSERT_THAT(meta_info, IsFalse());
}

TEST_F(Model_MetaInfo, module_is_valid)
{
    auto module = model.module("QML", ModuleKind::QmlLibrary);

    ASSERT_THAT(module, IsTrue());
}

TEST_F(Model_MetaInfo, module_returns_always_the_same)
{
    auto oldModule = model.module("QML", ModuleKind::QmlLibrary);

    auto module = model.module("QML", ModuleKind::QmlLibrary);

    ASSERT_THAT(module, oldModule);
}

TEST_F(Model_MetaInfo, get_meta_info_by_module)
{
    auto module = model.module("QML", ModuleKind::QmlLibrary);

    auto metaInfo = model.metaInfo(module, "QtObject");

    ASSERT_THAT(metaInfo, model.qmlQtObjectMetaInfo());
}

TEST_F(Model_MetaInfo, get_invalid_meta_info_by_module_for_wrong_name)
{
    auto module = model.module("QML", ModuleKind::QmlLibrary);

    auto metaInfo = model.metaInfo(module, "Object");

    ASSERT_THAT(metaInfo, IsFalse());
}

TEST_F(Model_MetaInfo, get_invalid_meta_info_by_module_for_wrong_module)
{
    auto module = model.module("Qml", ModuleKind::QmlLibrary);

    auto metaInfo = model.metaInfo(module, "Object");

    ASSERT_THAT(metaInfo, IsFalse());
}

TEST_F(Model_MetaInfo, add_project_storage_observer_to_project_storage)
{
    EXPECT_CALL(projectStorageMock, addObserver(_));

    QmlDesigner::Model model{{projectStorageMock, pathCacheMock}, "Item", -1, -1, nullptr, {}};
}

TEST_F(Model_MetaInfo, remove_project_storage_observer_from_project_storage)
{
    EXPECT_CALL(projectStorageMock, removeObserver(_)).Times(2); // the fixture model is calling it too

    QmlDesigner::Model model{{projectStorageMock, pathCacheMock}, "Item", -1, -1, nullptr, {}};
}

TEST_F(Model_MetaInfo, refresh_callback_is_calling_abstract_view)
{
    const QmlDesigner::TypeIds typeIds = {QmlDesigner::TypeId::create(3),
                                          QmlDesigner::TypeId::create(1)};
    ProjectStorageObserverMock observerMock;
    QmlDesigner::ProjectStorageObserver *observer = nullptr;
    ON_CALL(projectStorageMock, addObserver(_)).WillByDefault([&](auto *o) { observer = o; });

    QmlDesigner::Model model{{projectStorageMock, pathCacheMock}, "Item", -1, -1, nullptr, {}};
    model.attachView(&viewMock);

    EXPECT_CALL(viewMock, refreshMetaInfos(typeIds));

    observer->removedTypeIds(typeIds);
}

TEST_F(Model_MetaInfo, meta_infos_for_mdoule)
{
    projectStorageMock.createModule("Foo", ModuleKind::QmlLibrary);
    auto module = model.module("Foo", ModuleKind::QmlLibrary);
    auto typeId = projectStorageMock.createObject(module.id(), "Bar");
    ON_CALL(projectStorageMock, typeIds(module.id()))
        .WillByDefault(Return(QVarLengthArray<QmlDesigner::TypeId, 256>{typeId}));

    auto types = model.metaInfosForModule(module);

    ASSERT_THAT(types, ElementsAre(Eq(QmlDesigner::NodeMetaInfo{typeId, &projectStorageMock})));
}

TEST_F(Model_MetaInfo, create_node_resolved_meta_type)
{
    auto node = model.createModelNode("Item");

    ASSERT_THAT(node.metaInfo(), model.qtQuickItemMetaInfo());
}

TEST_F(Model_MetaInfo, create_node_has_unresolved_meta_type_for_invalid_type_name)
{
    auto node = model.createModelNode("Foo");

    ASSERT_THAT(node.metaInfo(), IsFalse());
}

TEST_F(Model_MetaInfo, refresh_type_id_if_project_storage_removed_type_id)
{
    auto node = model.createModelNode("Item");
    projectStorageMock.removeType(qtQuickModuleId, "Item");
    auto observer = projectStorageMock.observers.front();
    auto itemTypeId2 = projectStorageMock.createObject(qtQuickModuleId, "Item");
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId, itemTypeId2);

    observer->removedTypeIds({itemTypeId});

    ASSERT_THAT(node.metaInfo().id(), itemTypeId2);
}

TEST_F(Model_MetaInfo, set_null_type_id_if_project_storage_removed_type_id_cannot_be_refreshed)
{
    auto node = model.createModelNode("Item");
    projectStorageMock.removeType(qtQuickModuleId, "Item");
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId, QmlDesigner::TypeId{});

    auto observer = projectStorageMock.observers.front();

    observer->removedTypeIds({itemTypeId});

    ASSERT_THAT(node.metaInfo().id(), IsFalse());
}

TEST_F(Model_MetaInfo, null_type_id_are_refreshed_if_exported_types_are_updated)
{
    auto node = model.createModelNode("Item");
    projectStorageMock.removeType(qtQuickModuleId, "Item");
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId, QmlDesigner::TypeId{});
    auto observer = projectStorageMock.observers.front();
    observer->removedTypeIds({itemTypeId});
    auto itemTypeId2 = projectStorageMock.createObject(qtQuickModuleId, "Item");
    projectStorageMock.refreshImportedTypeNameId(itemTypeNameId, itemTypeId2);

    observer->exportedTypesChanged();

    ASSERT_THAT(node.metaInfo().id(), itemTypeId2);
}

class Model_TypeAnnotation : public Model
{};

TEST_F(Model_TypeAnnotation, item_library_entries)
{
    using namespace Qt::StringLiterals;
    QmlDesigner::Storage::Info::ItemLibraryEntries storageEntries{{itemTypeId,
                                                                   "Item",
                                                                   "Item",
                                                                   "/path/to/icon",
                                                                   "basic category",
                                                                   "QtQuick",
                                                                   "It's a item",
                                                                   "/path/to/template"}};
    storageEntries.front().properties.emplace_back("x", "double", Sqlite::ValueView::create(1));
    storageEntries.front().extraFilePaths.emplace_back("/extra/file/path");
    projectStorageMock.setItemLibraryEntries(pathCacheMock.sourceId, storageEntries);

    auto entries = model.itemLibraryEntries();

    ASSERT_THAT(entries,
                ElementsAre(
                    IsItemLibraryEntry(itemTypeId,
                                       "Item",
                                       u"Item",
                                       u"/path/to/icon",
                                       u"basic category",
                                       u"QtQuick",
                                       u"It's a item",
                                       u"/path/to/template",
                                       ElementsAre(IsItemLibraryProperty("x", "double"_L1, QVariant{1})),
                                       ElementsAre(u"/extra/file/path"))));
}

} // namespace
