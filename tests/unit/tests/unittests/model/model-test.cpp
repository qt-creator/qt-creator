// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include <matchers/import-matcher.h>
#include <mocks/abstractviewmock.h>
#include <mocks/modelresourcemanagementmock.h>
#include <mocks/projectstoragemock.h>
#include <mocks/sourcepathcachemock.h>

#include <designercore/include/bindingproperty.h>
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

MATCHER(IsSorted, std::string(negation ? "isn't sorted" : "is sorted"))
{
    using std::begin;
    using std::end;
    return std::is_sorted(begin(arg), end(arg));
}

class Model : public ::testing::Test
{
protected:
    Model()
    {
        model.setFileUrl(QUrl::fromLocalFile(pathCacheMock.path.toQString()));
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
        auto node = viewMock.createModelNode("QtQuick.Item");
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
    NiceMock<AbstractViewMock> viewMock;
    NiceMock<SourcePathCacheMockWithPaths> pathCacheMock{"/path/foo.qml"};
    NiceMock<ProjectStorageMockWithQtQtuick> projectStorageMock{pathCacheMock.sourceId};
    NiceMock<ModelResourceManagementMock> resourceManagementMock;
    QmlDesigner::Model model{{projectStorageMock, pathCacheMock},
                             "Item",
                             -1,
                             -1,
                             nullptr,
                             std::make_unique<ModelResourceManagementMockWrapper>(
                                 resourceManagementMock)};
    QmlDesigner::SourceId filePathId = pathCacheMock.sourceId;
    QmlDesigner::TypeId itemTypeId = projectStorageMock.typeId(projectStorageMock.moduleId(
                                                                   "QtQuick"),
                                                               "Item",
                                                               QmlDesigner::Storage::Version{});
    QmlDesigner::ImportedTypeNameId itemTypeNameId = projectStorageMock.createImportedTypeNameId(
        filePathId, "Item", itemTypeId);
    ModelNode rootNode;
};

TEST_F(Model, model_node_destroy_is_calling_model_resource_management_remove_node)
{
    auto node = createNodeWithParent(rootNode);

    EXPECT_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model));

    node.destroy();
}

TEST_F(Model, model_node_remove_propery_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.removeProperty("foo");
}

TEST_F(Model, node_abstract_property_reparent_here_is_calling_model_resource_management_remove_property)
{
    auto node = createNodeWithParent(rootNode);
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.nodeListProperty("foo").reparentHere(node);
}

TEST_F(Model, node_property_set_model_node_is_calling_model_resource_management_remove_property)
{
    auto node = createNodeWithParent(rootNode);
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.nodeProperty("foo").setModelNode(node);
}

TEST_F(Model, variant_property_set_value_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.variantProperty("foo").setValue(7);
}

TEST_F(Model,
       variant_property_set_dynamic_type_name_and_enumeration_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.variantProperty("foo").setDynamicTypeNameAndEnumeration("int", "Ha");
}

TEST_F(Model, variant_property_set_dynamic_type_name_and_value_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.variantProperty("foo").setDynamicTypeNameAndValue("int", 7);
}

TEST_F(Model, binding_property_set_expression_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.bindingProperty("foo").setExpression("blah");
}

TEST_F(Model,
       binding_property_set_dynamic_type_name_and_expression_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.bindingProperty("foo").setDynamicTypeNameAndExpression("int", "blah");
}

TEST_F(Model, signal_handler_property_set_source_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.signalHandlerProperty("foo").setSource("blah");
}

TEST_F(Model, signal_declaration_property_set_signature_is_calling_model_resource_management_remove_property)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.signalDeclarationProperty("foo").setSignature("blah");
}

TEST_F(Model, model_node_destroy_is_calling_abstract_view_node_about_to_be_removed)
{
    auto node = createNodeWithParent(rootNode);
    auto node2 = createNodeWithParent(rootNode);
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node, node2}, {}, {}}));

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(Eq(node)));
    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(Eq(node2)));

    node.destroy();
}

TEST_F(Model, model_node_destroy_is_calling_abstract_view_node_removed)
{
    auto node = createNodeWithParent(rootNode);
    auto node2 = createNodeWithParent(rootNode);
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node, node2}, {}, {}}));

    EXPECT_CALL(viewMock, nodeRemoved(Eq(node), _, _));
    EXPECT_CALL(viewMock, nodeRemoved(Eq(node2), _, _));

    node.destroy();
}

TEST_F(Model, model_node_destroy_is_calling_abstract_view_node_removed_with_valid_nodes)
{
    auto node = createNodeWithParent(rootNode);
    auto node2 = createNodeWithParent(rootNode);
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node, node2, ModelNode{}}, {}, {}}));

    EXPECT_CALL(viewMock, nodeRemoved(Eq(node), _, _));
    EXPECT_CALL(viewMock, nodeRemoved(Eq(node2), _, _));

    node.destroy();
}

TEST_F(Model, model_node_destroy_is_calling_abstract_view_properties_about_to_be_removed)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createProperty(rootNode, "foo");
    auto property2 = createProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node}, {property, property2}, {}}));

    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(UnorderedElementsAre(property, property2)));

    node.destroy();
}

TEST_F(Model, model_node_destroy_is_calling_abstract_view_properties_removed)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createProperty(rootNode, "foo");
    auto property2 = createProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node}, {property, property2}, {}}));

    EXPECT_CALL(viewMock, propertiesRemoved(UnorderedElementsAre(property, property2)));

    node.destroy();
}

TEST_F(Model, model_node_destroy_is_calling_abstract_view_properties_removed_only_with_valid_properties)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createProperty(rootNode, "foo");
    auto property2 = createProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node}, {property, property2, {}}, {}}));

    EXPECT_CALL(viewMock, propertiesRemoved(UnorderedElementsAre(property, property2)));

    node.destroy();
}

TEST_F(Model, model_node_destroy_is_calling_abstract_view_binding_properties_about_to_be_changed)
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

TEST_F(Model, model_node_destroy_is_calling_abstract_view_binding_properties_changed)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createBindingProperty(rootNode, "foo");
    auto property2 = createBindingProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node}, {}, {{property, "yi"}, {property2, "er"}}}));

    EXPECT_CALL(viewMock, bindingPropertiesChanged(UnorderedElementsAre(property, property2), _));

    node.destroy();
}

TEST_F(Model, model_node_destroy_is_changing_binding_property_expression)
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

TEST_F(Model, model_node_destroy_is_only_changing_existing_binding_property)
{
    auto node = createNodeWithParent(rootNode);
    auto property = rootNode.bindingProperty("foo");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{}, {}, {{property, "yi"}}}));

    node.destroy();

    ASSERT_FALSE(rootNode.hasBindingProperty("foo"));
}

TEST_F(Model, model_node_destroy_is_calling_abstract_view_binding_properties_changed_only_with_existing_properties)
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

TEST_F(Model, model_node_remove_property_is_calling_abstract_view_node_about_to_be_removed)
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

TEST_F(Model, model_node_remove_property_is_calling_abstract_view_node_removed)
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

TEST_F(Model, model_node_remove_property_is_calling_abstract_view_node_removed_with_valid_nodes)
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

TEST_F(Model, model_node_remove_property_is_calling_abstract_view_properties_about_to_be_removed)
{
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(Return(ModelResourceSet{{}, {property, property2}, {}}));

    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(UnorderedElementsAre(property, property2)));

    rootNode.removeProperty("yi");
}

TEST_F(Model, model_node_remove_property_is_calling_abstract_view_properties_removed)
{
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(Return(ModelResourceSet{{}, {property, property2}, {}}));

    EXPECT_CALL(viewMock, propertiesRemoved(UnorderedElementsAre(property, property2)));

    rootNode.removeProperty("yi");
}

TEST_F(Model, model_node_remove_property_is_calling_abstract_view_properties_removed_only_with_valid_properties)
{
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(Return(ModelResourceSet{{}, {property, property2, {}}, {}}));

    EXPECT_CALL(viewMock, propertiesRemoved(UnorderedElementsAre(property, property2)));

    rootNode.removeProperty("yi");
}

TEST_F(Model, model_node_remove_property_is_calling_abstract_view_binding_properties_about_to_be_changed)
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

TEST_F(Model, model_node_remove_property_is_calling_abstract_view_binding_properties_changed)
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

TEST_F(Model,
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

TEST_F(Model, by_default_remove_model_node_removes_node)
{
    model.detachView(&viewMock);
    QmlDesigner::Model newModel{{projectStorageMock, pathCacheMock}, "QtQuick.Item"};
    newModel.attachView(&viewMock);
    auto node = createNodeWithParent(viewMock.rootModelNode());

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(Eq(node)));

    node.destroy();
}

TEST_F(Model, by_default_remove_properties_removes_property)
{
    model.detachView(&viewMock);
    QmlDesigner::Model newModel{{projectStorageMock, pathCacheMock}, "QtQuick.Item"};
    newModel.attachView(&viewMock);
    rootNode = viewMock.rootModelNode();
    auto property = createProperty(rootNode, "yi");

    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(ElementsAre(Eq(property))));

    rootNode.removeProperty("yi");
}

TEST_F(Model, by_default_remove_model_node_in_factory_method_calls_removes_node)
{
    model.detachView(&viewMock);
    auto newModel = QmlDesigner::Model::create({projectStorageMock, pathCacheMock}, "QtQuick.Item");
    newModel->attachView(&viewMock);
    auto node = createNodeWithParent(viewMock.rootModelNode());

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(Eq(node)));

    node.destroy();
}

TEST_F(Model, by_default_remove_properties_in_factory_method_calls_remove_property)
{
    model.detachView(&viewMock);
    auto newModel = QmlDesigner::Model::create({projectStorageMock, pathCacheMock}, "QtQuick.Item");
    newModel->attachView(&viewMock);
    rootNode = viewMock.rootModelNode();
    auto property = createProperty(rootNode, "yi");

    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(ElementsAre(Eq(property))));

    rootNode.removeProperty("yi");
}

TEST_F(Model, remove_model_nodes)
{
    auto node = createNodeWithParent(rootNode, "yi");
    auto node2 = createNodeWithParent(rootNode, "er");

    EXPECT_CALL(resourceManagementMock,
                removeNodes(AllOf(UnorderedElementsAre(node, node2), IsSorted()), &model));

    model.removeModelNodes({node, node2});
}

TEST_F(Model, remove_model_nodes_filters_invalid_model_nodes)
{
    auto node = createNodeWithParent(rootNode, "yi");

    EXPECT_CALL(resourceManagementMock, removeNodes(UnorderedElementsAre(node), &model));

    model.removeModelNodes({{}, node});
}

TEST_F(Model, remove_model_nodes_for_only_invalid_model_nodes_does_nothing)
{
    EXPECT_CALL(resourceManagementMock, removeNodes(_, _)).Times(0);

    model.removeModelNodes({{}});
}

TEST_F(Model, remove_model_nodes_reverse)
{
    auto node = createNodeWithParent(rootNode, "yi");
    auto node2 = createNodeWithParent(rootNode, "er");

    EXPECT_CALL(resourceManagementMock,
                removeNodes(AllOf(UnorderedElementsAre(node, node2), IsSorted()), &model));

    model.removeModelNodes({node2, node});
}

TEST_F(Model, remove_model_nodes_calls_notifier)
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

TEST_F(Model, remove_model_nodes_bypasses_model_resource_management)
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

TEST_F(Model, by_default_remove_model_nodes_in_factory_method_calls_removes_node)
{
    model.detachView(&viewMock);
    QmlDesigner::Model newModel{{projectStorageMock, pathCacheMock}, "QtQuick.Item"};
    newModel.attachView(&viewMock);
    rootNode = viewMock.rootModelNode();
    auto node = createNodeWithParent(rootNode, "yi");
    auto node2 = createNodeWithParent(rootNode, "er");

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(node));
    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(node2));

    newModel.removeModelNodes({node, node2});
}

TEST_F(Model, remove_properties)
{
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");

    EXPECT_CALL(resourceManagementMock,
                removeProperties(AllOf(UnorderedElementsAre(property, property2), IsSorted()), &model));

    model.removeProperties({property, property2});
}

TEST_F(Model, remove_properties_filters_invalid_properties)
{
    auto property = createProperty(rootNode, "yi");

    EXPECT_CALL(resourceManagementMock, removeProperties(UnorderedElementsAre(property), &model));

    model.removeProperties({{}, property});
}

TEST_F(Model, remove_properties_for_only_invalid_properties_does_nothing)
{
    EXPECT_CALL(resourceManagementMock, removeProperties(_, _)).Times(0);

    model.removeProperties({{}});
}

TEST_F(Model, remove_properties_reverse)
{
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");

    EXPECT_CALL(resourceManagementMock,
                removeProperties(AllOf(UnorderedElementsAre(property, property2), IsSorted()), &model));

    model.removeProperties({property2, property});
}

TEST_F(Model, remove_properties_calls_notifier)
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

TEST_F(Model, remove_properties_bypasses_model_resource_management)
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

TEST_F(Model, by_default_remove_properties_in_factory_method_calls_removes_properties)
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

TEST_F(Model, change_imports_is_synchronizing_imports_with_project_storage)
{
    QmlDesigner::SourceId directoryPathId = QmlDesigner::SourceId::create(2);
    ON_CALL(pathCacheMock, sourceId(Eq("/path/foo/."))).WillByDefault(Return(directoryPathId));
    auto qtQuickModuleId = projectStorageMock.moduleId("QtQuick");
    auto qtQmlModelsModuleId = projectStorageMock.moduleId("QtQml.Models");
    auto qtQuickImport = QmlDesigner::Import::createLibraryImport("QtQuick", "2.1");
    auto qtQmlModelsImport = QmlDesigner::Import::createLibraryImport("QtQml.Models");
    auto directoryImport = QmlDesigner::Import::createFileImport("foo");

    EXPECT_CALL(projectStorageMock,
                synchronizeDocumentImports(
                    UnorderedElementsAre(IsImport(qtQuickModuleId, filePathId, 2, 1),
                                         IsImport(qtQmlModelsModuleId, filePathId, -1, -1)),
                    filePathId));

    model.changeImports({qtQuickImport, qtQmlModelsImport}, {});
}

TEST_F(Model,
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

TEST_F(Model, change_imports_is_adding_import_in_project_storage)
{
    QmlDesigner::SourceId directoryPathId = QmlDesigner::SourceId::create(2);
    ON_CALL(pathCacheMock, sourceId(Eq("/path/foo/."))).WillByDefault(Return(directoryPathId));
    auto qtQuickModuleId = projectStorageMock.moduleId("QtQuick");
    auto qtQmlModelsModuleId = projectStorageMock.moduleId("QtQml.Models");
    auto qtQuickImport = QmlDesigner::Import::createLibraryImport("QtQuick", "2.1");
    auto qtQmlModelsImport = QmlDesigner::Import::createLibraryImport("QtQml.Models");
    auto directoryImport = QmlDesigner::Import::createFileImport("foo");
    model.changeImports({qtQmlModelsImport}, {});

    EXPECT_CALL(projectStorageMock,
                synchronizeDocumentImports(
                    UnorderedElementsAre(IsImport(qtQuickModuleId, filePathId, 2, 1),
                                         IsImport(qtQmlModelsModuleId, filePathId, -1, -1)),
                    filePathId));

    model.changeImports({qtQuickImport}, {});
}

TEST_F(Model, change_imports_is_removing_import_in_project_storage)
{
    QmlDesigner::SourceId directoryPathId = QmlDesigner::SourceId::create(2);
    ON_CALL(pathCacheMock, sourceId(Eq("/path/foo/."))).WillByDefault(Return(directoryPathId));
    auto qtQmlModelsModuleId = projectStorageMock.moduleId("QtQml.Models");
    auto qtQuickImport = QmlDesigner::Import::createLibraryImport("QtQuick", "2.1");
    auto qtQmlModelsImport = QmlDesigner::Import::createLibraryImport("QtQml.Models");
    auto directoryImport = QmlDesigner::Import::createFileImport("foo");
    model.changeImports({qtQuickImport, qtQmlModelsImport}, {});

    EXPECT_CALL(projectStorageMock,
                synchronizeDocumentImports(UnorderedElementsAre(
                                               IsImport(qtQmlModelsModuleId, filePathId, -1, -1)),
                                           filePathId));

    model.changeImports({}, {qtQuickImport});
}

TEST_F(Model,
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

TEST_F(Model, change_imports_is_changing_import_version_with_project_storage)
{
    QmlDesigner::SourceId directoryPathId = QmlDesigner::SourceId::create(2);
    ON_CALL(pathCacheMock, sourceId(Eq("/path/foo/."))).WillByDefault(Return(directoryPathId));
    auto qtQuickModuleId = projectStorageMock.moduleId("QtQuick");
    auto qtQmlModelsModuleId = projectStorageMock.moduleId("QtQml.Models");
    auto qtQuickImport = QmlDesigner::Import::createLibraryImport("QtQuick", "2.1");
    auto qtQmlModelsImport = QmlDesigner::Import::createLibraryImport("QtQml.Models");
    auto directoryImport = QmlDesigner::Import::createFileImport("foo");
    model.changeImports({qtQuickImport, qtQmlModelsImport}, {});
    qtQuickImport = QmlDesigner::Import::createLibraryImport("QtQuick", "3.1");

    EXPECT_CALL(projectStorageMock,
                synchronizeDocumentImports(
                    UnorderedElementsAre(IsImport(qtQuickModuleId, filePathId, 3, 1),
                                         IsImport(qtQmlModelsModuleId, filePathId, -1, -1)),
                    filePathId));

    model.changeImports({qtQuickImport}, {});
}

TEST_F(Model, create_model_node_has_meta_info)
{
    auto node = model.createModelNode("Item");

    ASSERT_THAT(node.metaInfo(), model.qtQuickItemMetaInfo());
}

TEST_F(Model, create_qualified_model_node_has_meta_info)
{
    auto qtQmlModelsImport = QmlDesigner::Import::createLibraryImport("QtQml.Models", "", "Foo");
    auto qtQmlModelsModulesId = projectStorageMock.moduleId("QtQml.Models");
    auto importId = projectStorageMock.createImportId(qtQmlModelsModulesId, filePathId);
    auto listModelTypeId = projectStorageMock.typeId(qtQmlModelsModulesId,
                                                     "ListModel",
                                                     QmlDesigner::Storage::Version{});
    projectStorageMock.createImportedTypeNameId(importId, "ListModel", listModelTypeId);
    model.changeImports({qtQmlModelsImport}, {});

    auto node = model.createModelNode("Foo.ListModel");

    ASSERT_THAT(node.metaInfo(), model.qtQmlModelsListModelMetaInfo());
}

TEST_F(Model, change_root_node_type_changes_meta_info)
{
    projectStorageMock.createImportedTypeNameId(filePathId,
                                                "QtObject",
                                                model.qmlQtObjectMetaInfo().id());

    model.changeRootNodeType("QtObject");

    ASSERT_THAT(rootNode.metaInfo(), model.qmlQtObjectMetaInfo());
}

TEST_F(Model, meta_info)
{
    auto meta_info = model.metaInfo("QtObject");

    ASSERT_THAT(meta_info, model.qmlQtObjectMetaInfo());
}

TEST_F(Model, meta_info_of_not_existing_type_is_invalid)
{
    auto meta_info = model.metaInfo("Foo");

    ASSERT_THAT(meta_info, IsFalse());
}

TEST_F(Model, module_is_valid)
{
    auto module = model.module("QML");

    ASSERT_THAT(module, IsTrue());
}

TEST_F(Model, module_returns_always_the_same)
{
    auto oldModule = model.module("QML");

    auto module = model.module("QML");

    ASSERT_THAT(module, oldModule);
}

TEST_F(Model, get_meta_info_by_module)
{
    auto module = model.module("QML");

    auto metaInfo = model.metaInfo(module, "QtObject");

    ASSERT_THAT(metaInfo, model.qmlQtObjectMetaInfo());
}

TEST_F(Model, get_invalid_meta_info_by_module_for_wrong_name)
{
    auto module = model.module("QML");

    auto metaInfo = model.metaInfo(module, "Object");

    ASSERT_THAT(metaInfo, IsFalse());
}

TEST_F(Model, get_invalid_meta_info_by_module_for_wrong_module)
{
    auto module = model.module("Qml");

    auto metaInfo = model.metaInfo(module, "Object");

    ASSERT_THAT(metaInfo, IsFalse());
}

TEST_F(Model, add_refresh_callback_to_project_storage)
{
    EXPECT_CALL(projectStorageMock, addRefreshCallback(_));

    QmlDesigner::Model model{{projectStorageMock, pathCacheMock}, "Item", -1, -1, nullptr, {}};
}

TEST_F(Model, remove_refresh_callback_from_project_storage)
{
    EXPECT_CALL(projectStorageMock, removeRefreshCallback(_)).Times(2); // there is a model in the fixture

    QmlDesigner::Model model{{projectStorageMock, pathCacheMock}, "Item", -1, -1, nullptr, {}};
}

TEST_F(Model, refresh_callback_is_calling_abstract_view)
{
    const QmlDesigner::TypeIds typeIds = {QmlDesigner::TypeId::create(3),
                                          QmlDesigner::TypeId::create(1)};
    std::function<void(const QmlDesigner::TypeIds &)> *callback = nullptr;
    ON_CALL(projectStorageMock, addRefreshCallback(_)).WillByDefault([&](auto *c) { callback = c; });
    QmlDesigner::Model model{{projectStorageMock, pathCacheMock}, "Item", -1, -1, nullptr, {}};
    model.attachView(&viewMock);

    EXPECT_CALL(viewMock, refreshMetaInfos(typeIds));

    (*callback)(typeIds);
}

} // namespace
