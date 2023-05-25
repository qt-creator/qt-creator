// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../utils/googletest.h"

#include "../mocks/mocklistmodeleditorview.h"
#include "../mocks/modelresourcemanagementmock.h"
#include "../mocks/projectstoragemock.h"

#include <designercore/include/bindingproperty.h>
#include <designercore/include/model.h>
#include <designercore/include/modelnode.h>
#include <designercore/include/nodeabstractproperty.h>
#include <designercore/include/nodelistproperty.h>
#include <designercore/include/nodeproperty.h>
#include <designercore/include/signalhandlerproperty.h>
#include <designercore/include/variantproperty.h>

namespace {
using QmlDesigner::AbstractProperty;
using QmlDesigner::ModelNode;
using QmlDesigner::ModelNodes;
using QmlDesigner::ModelResourceSet;

template<typename Matcher>
auto HasPropertyName(const Matcher &matcher)
{
    return Property(&AbstractProperty::name, matcher);
}

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
        model.attachView(&viewMock);
        rootNode = viewMock.rootModelNode();
        ON_CALL(resourceManagementMock, removeNodes(_, _)).WillByDefault([](auto nodes, auto) {
            return ModelResourceSet{std::move(nodes), {}, {}};
        });
        ON_CALL(resourceManagementMock, removeProperties(_, _)).WillByDefault([](auto properties, auto) {
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
    NiceMock<MockListModelEditorView> viewMock;
    NiceMock<ProjectStorageMockWithQtQtuick> projectStorageMock;
    NiceMock<ModelResourceManagementMock> resourceManagementMock;
    QmlDesigner::Model model{projectStorageMock,
                             "QtQuick.Item",
                             -1,
                             -1,
                             nullptr,
                             std::make_unique<ModelResourceManagementMockWrapper>(
                                 resourceManagementMock)};
    ModelNode rootNode;
};

TEST_F(Model, ModelNodeDestroyIsCallingModelResourceManagementRemoveNode)
{
    auto node = createNodeWithParent(rootNode);

    EXPECT_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model));

    node.destroy();
}

TEST_F(Model, ModelNodeRemoveProperyIsCallingModelResourceManagementRemoveProperty)
{
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.removeProperty("foo");
}

TEST_F(Model, NodeAbstractPropertyReparentHereIsCallingModelResourceManagementRemoveProperty)
{
    auto node = createNodeWithParent(rootNode);
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.nodeListProperty("foo").reparentHere(node);
}

TEST_F(Model, NodePropertySetModelNodeIsCallingModelResourceManagementRemoveProperty)
{
    auto node = createNodeWithParent(rootNode);
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.nodeProperty("foo").setModelNode(node);
}

TEST_F(Model, VariantPropertySetValueIsCallingModelResourceManagementRemoveProperty)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.variantProperty("foo").setValue(7);
}

TEST_F(Model,
       VariantPropertySetDynamicTypeNameAndEnumerationIsCallingModelResourceManagementRemoveProperty)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.variantProperty("foo").setDynamicTypeNameAndEnumeration("int", "Ha");
}

TEST_F(Model, VariantPropertySetDynamicTypeNameAndValueIsCallingModelResourceManagementRemoveProperty)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.variantProperty("foo").setDynamicTypeNameAndValue("int", 7);
}

TEST_F(Model, BindingPropertySetExpressionIsCallingModelResourceManagementRemoveProperty)
{
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.bindingProperty("foo").setExpression("blah");
}

TEST_F(Model,
       BindingPropertySetDynamicTypeNameAndExpressionIsCallingModelResourceManagementRemoveProperty)
{
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.bindingProperty("foo").setDynamicTypeNameAndExpression("int", "blah");
}

TEST_F(Model, SignalHandlerPropertySetSourceIsCallingModelResourceManagementRemoveProperty)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.signalHandlerProperty("foo").setSource("blah");
}

TEST_F(Model, SignalDeclarationPropertySetSignatureIsCallingModelResourceManagementRemoveProperty)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model));

    rootNode.signalDeclarationProperty("foo").setSignature("blah");
}

TEST_F(Model, ModelNodeDestroyIsCallingAbstractViewNodeAboutToBeRemoved)
{
    auto node = createNodeWithParent(rootNode);
    auto node2 = createNodeWithParent(rootNode);
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node, node2}, {}, {}}));

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(Eq(node)));
    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(Eq(node2)));

    node.destroy();
}

TEST_F(Model, ModelNodeDestroyIsCallingAbstractViewNodeRemoved)
{
    auto node = createNodeWithParent(rootNode);
    auto node2 = createNodeWithParent(rootNode);
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node, node2}, {}, {}}));

    EXPECT_CALL(viewMock, nodeRemoved(Eq(node), _, _));
    EXPECT_CALL(viewMock, nodeRemoved(Eq(node2), _, _));

    node.destroy();
}

TEST_F(Model, ModelNodeDestroyIsCallingAbstractViewNodeRemovedWithValidNodes)
{
    auto node = createNodeWithParent(rootNode);
    auto node2 = createNodeWithParent(rootNode);
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node, node2, ModelNode{}}, {}, {}}));

    EXPECT_CALL(viewMock, nodeRemoved(Eq(node), _, _));
    EXPECT_CALL(viewMock, nodeRemoved(Eq(node2), _, _));

    node.destroy();
}

TEST_F(Model, ModelNodeDestroyIsCallingAbstractViewPropertiesAboutToBeRemoved)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createProperty(rootNode, "foo");
    auto property2 = createProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node}, {property, property2}, {}}));

    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(UnorderedElementsAre(property, property2)));

    node.destroy();
}

TEST_F(Model, ModelNodeDestroyIsCallingAbstractViewPropertiesRemoved)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createProperty(rootNode, "foo");
    auto property2 = createProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node}, {property, property2}, {}}));

    EXPECT_CALL(viewMock, propertiesRemoved(UnorderedElementsAre(property, property2)));

    node.destroy();
}

TEST_F(Model, ModelNodeDestroyIsCallingAbstractViewPropertiesRemovedOnlyWithValidProperties)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createProperty(rootNode, "foo");
    auto property2 = createProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node}, {property, property2, {}}, {}}));

    EXPECT_CALL(viewMock, propertiesRemoved(UnorderedElementsAre(property, property2)));

    node.destroy();
}

TEST_F(Model, ModelNodeDestroyIsCallingAbstractViewBindingPropertiesAboutToBeChanged)
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

TEST_F(Model, ModelNodeDestroyIsCallingAbstractViewBindingPropertiesChanged)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createBindingProperty(rootNode, "foo");
    auto property2 = createBindingProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{node}, {}, {{property, "yi"}, {property2, "er"}}}));

    EXPECT_CALL(viewMock, bindingPropertiesChanged(UnorderedElementsAre(property, property2), _));

    node.destroy();
}

TEST_F(Model, ModelNodeDestroyIsChangingBindingPropertyExpression)
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

TEST_F(Model, ModelNodeDestroyIsOnlyChangingExistingBindingProperty)
{
    auto node = createNodeWithParent(rootNode);
    auto property = rootNode.bindingProperty("foo");
    ON_CALL(resourceManagementMock, removeNodes(ElementsAre(node), &model))
        .WillByDefault(Return(ModelResourceSet{{}, {}, {{property, "yi"}}}));

    node.destroy();

    ASSERT_FALSE(rootNode.hasBindingProperty("foo"));
}

TEST_F(Model, ModelNodeDestroyIsCallingAbstractViewBindingPropertiesChangedOnlyWithExistingProperties)
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

TEST_F(Model, ModelNodeRemovePropertyIsCallingAbstractViewNodeAboutToBeRemoved)
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

TEST_F(Model, ModelNodeRemovePropertyIsCallingAbstractViewNodeRemoved)
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

TEST_F(Model, ModelNodeRemovePropertyIsCallingAbstractViewNodeRemovedWithValidNodes)
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

TEST_F(Model, ModelNodeRemovePropertyIsCallingAbstractViewPropertiesAboutToBeRemoved)
{
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(Return(ModelResourceSet{{}, {property, property2}, {}}));

    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(UnorderedElementsAre(property, property2)));

    rootNode.removeProperty("yi");
}

TEST_F(Model, ModelNodeRemovePropertyIsCallingAbstractViewPropertiesRemoved)
{
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(Return(ModelResourceSet{{}, {property, property2}, {}}));

    EXPECT_CALL(viewMock, propertiesRemoved(UnorderedElementsAre(property, property2)));

    rootNode.removeProperty("yi");
}

TEST_F(Model, ModelNodeRemovePropertyIsCallingAbstractViewPropertiesRemovedOnlyWithValidProperties)
{
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");
    ON_CALL(resourceManagementMock, removeProperties(ElementsAre(property), &model))
        .WillByDefault(Return(ModelResourceSet{{}, {property, property2, {}}, {}}));

    EXPECT_CALL(viewMock, propertiesRemoved(UnorderedElementsAre(property, property2)));

    rootNode.removeProperty("yi");
}

TEST_F(Model, ModelNodeRemovePropertyIsCallingAbstractViewBindingPropertiesAboutToBeChanged)
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

TEST_F(Model, ModelNodeRemovePropertyIsCallingAbstractViewBindingPropertiesChanged)
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
       ModelNodeRemovePropertyIsCallingAbstractViewBindingPropertiesChangedOnlyWithValidProperties)
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

TEST_F(Model, ByDefaultRemoveModelNodeRemovesNode)
{
    model.detachView(&viewMock);
    QmlDesigner::Model newModel{projectStorageMock, "QtQuick.Item"};
    newModel.attachView(&viewMock);
    auto node = createNodeWithParent(viewMock.rootModelNode());

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(Eq(node)));

    node.destroy();
}

TEST_F(Model, ByDefaultRemovePropertiesRemovesProperty)
{
    model.detachView(&viewMock);
    QmlDesigner::Model newModel{projectStorageMock, "QtQuick.Item"};
    newModel.attachView(&viewMock);
    rootNode = viewMock.rootModelNode();
    auto property = createProperty(rootNode, "yi");

    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(ElementsAre(Eq(property))));

    rootNode.removeProperty("yi");
}

TEST_F(Model, ByDefaultRemoveModelNodeInFactoryMethodCallsRemovesNode)
{
    model.detachView(&viewMock);
    auto newModel = QmlDesigner::Model::create(projectStorageMock, "QtQuick.Item");
    newModel->attachView(&viewMock);
    auto node = createNodeWithParent(viewMock.rootModelNode());

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(Eq(node)));

    node.destroy();
}

TEST_F(Model, ByDefaultRemovePropertiesInFactoryMethodCallsRemoveProperty)
{
    model.detachView(&viewMock);
    auto newModel = QmlDesigner::Model::create(projectStorageMock, "QtQuick.Item");
    newModel->attachView(&viewMock);
    rootNode = viewMock.rootModelNode();
    auto property = createProperty(rootNode, "yi");

    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(ElementsAre(Eq(property))));

    rootNode.removeProperty("yi");
}

TEST_F(Model, RemoveModelNodes)
{
    auto node = createNodeWithParent(rootNode, "yi");
    auto node2 = createNodeWithParent(rootNode, "er");

    EXPECT_CALL(resourceManagementMock,
                removeNodes(AllOf(UnorderedElementsAre(node, node2), IsSorted()), &model));

    model.removeModelNodes({node, node2});
}

TEST_F(Model, RemoveModelNodesFiltersInvalidModelNodes)
{
    auto node = createNodeWithParent(rootNode, "yi");

    EXPECT_CALL(resourceManagementMock, removeNodes(UnorderedElementsAre(node), &model));

    model.removeModelNodes({{}, node});
}

TEST_F(Model, RemoveModelNodesForOnlyInvalidModelNodesDoesNothing)
{
    EXPECT_CALL(resourceManagementMock, removeNodes(_, _)).Times(0);

    model.removeModelNodes({{}});
}

TEST_F(Model, RemoveModelNodesReverse)
{
    auto node = createNodeWithParent(rootNode, "yi");
    auto node2 = createNodeWithParent(rootNode, "er");

    EXPECT_CALL(resourceManagementMock,
                removeNodes(AllOf(UnorderedElementsAre(node, node2), IsSorted()), &model));

    model.removeModelNodes({node2, node});
}

TEST_F(Model, RemoveModelNodesCallsNotifier)
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

TEST_F(Model, RemoveModelNodesBypassesModelResourceManagement)
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

TEST_F(Model, ByDefaultRemoveModelNodesInFactoryMethodCallsRemovesNode)
{
    model.detachView(&viewMock);
    QmlDesigner::Model newModel{projectStorageMock, "QtQuick.Item"};
    newModel.attachView(&viewMock);
    rootNode = viewMock.rootModelNode();
    auto node = createNodeWithParent(rootNode, "yi");
    auto node2 = createNodeWithParent(rootNode, "er");

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(node));
    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(node2));

    newModel.removeModelNodes({node, node2});
}

TEST_F(Model, RemoveProperties)
{
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");

    EXPECT_CALL(resourceManagementMock,
                removeProperties(AllOf(UnorderedElementsAre(property, property2), IsSorted()), &model));

    model.removeProperties({property, property2});
}

TEST_F(Model, RemovePropertiesFiltersInvalidProperties)
{
    auto property = createProperty(rootNode, "yi");

    EXPECT_CALL(resourceManagementMock, removeProperties(UnorderedElementsAre(property), &model));

    model.removeProperties({{}, property});
}

TEST_F(Model, RemovePropertiesForOnlyInvalidPropertiesDoesNothing)
{
    EXPECT_CALL(resourceManagementMock, removeProperties(_, _)).Times(0);

    model.removeProperties({{}});
}

TEST_F(Model, RemovePropertiesReverse)
{
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");

    EXPECT_CALL(resourceManagementMock,
                removeProperties(AllOf(UnorderedElementsAre(property, property2), IsSorted()), &model));

    model.removeProperties({property2, property});
}

TEST_F(Model, RemovePropertiesCallsNotifier)
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

TEST_F(Model, RemovePropertiesBypassesModelResourceManagement)
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

TEST_F(Model, ByDefaultRemovePropertiesInFactoryMethodCallsRemovesProperties)
{
    model.detachView(&viewMock);
    QmlDesigner::Model newModel{projectStorageMock, "QtQuick.Item"};
    newModel.attachView(&viewMock);
    rootNode = viewMock.rootModelNode();
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");

    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(UnorderedElementsAre(property, property2)));

    newModel.removeProperties({property, property2});
}

} // namespace
