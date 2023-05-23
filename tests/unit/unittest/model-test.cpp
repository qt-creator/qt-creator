// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "googletest.h"

#include "mocklistmodeleditorview.h"
#include "modelresourcemanagementmock.h"
#include "projectstoragemock.h"

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

class Model : public ::testing::Test
{
protected:
    Model()
    {
        model.attachView(&viewMock);
        rootNode = viewMock.rootModelNode();
        ON_CALL(resourceManagementMock, removeNode(_)).WillByDefault([](const auto &node) {
            return ModelResourceSet{{node}, {}, {}};
        });
        ON_CALL(resourceManagementMock, removeProperty(_)).WillByDefault([](const auto &property) {
            return ModelResourceSet{{}, {property}, {}};
        });
    }

    ~Model() { model.detachView(&viewMock); }

    auto createNodeWithParent(const ModelNode &parentNode)
    {
        auto node = viewMock.createModelNode("QtQuick.Item");
        parentNode.defaultNodeAbstractProperty().reparentHere(node);

        return node;
    }

    auto createProperty(const ModelNode &parentNode, QmlDesigner::PropertyName name)
    {
        auto property = parentNode.variantProperty(name);
        property.setValue(4);
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

    EXPECT_CALL(resourceManagementMock, removeNode(node));

    node.destroy();
}

TEST_F(Model, ModelNodeRemoveProperyIsCallingModelResourceManagementRemoveProperty)
{
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperty(Eq(property)));

    rootNode.removeProperty("foo");
}

TEST_F(Model, NodeAbstractPropertyReparentHereIsCallingModelResourceManagementRemoveProperty)
{
    auto node = createNodeWithParent(rootNode);
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperty(Eq(property)));

    rootNode.nodeListProperty("foo").reparentHere(node);
}

TEST_F(Model, NodePropertySetModelNodeIsCallingModelResourceManagementRemoveProperty)
{
    auto node = createNodeWithParent(rootNode);
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperty(Eq(property)));

    rootNode.nodeProperty("foo").setModelNode(node);
}

TEST_F(Model, VariantPropertySetValueIsCallingModelResourceManagementRemoveProperty)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperty(Eq(property)));

    rootNode.variantProperty("foo").setValue(7);
}

TEST_F(Model,
       VariantPropertySetDynamicTypeNameAndEnumerationIsCallingModelResourceManagementRemoveProperty)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperty(Eq(property)));

    rootNode.variantProperty("foo").setDynamicTypeNameAndEnumeration("int", "Ha");
}

TEST_F(Model, VariantPropertySetDynamicTypeNameAndValueIsCallingModelResourceManagementRemoveProperty)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperty(Eq(property)));

    rootNode.variantProperty("foo").setDynamicTypeNameAndValue("int", 7);
}

TEST_F(Model, BindingPropertySetExpressionIsCallingModelResourceManagementRemoveProperty)
{
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperty(Eq(property)));

    rootNode.bindingProperty("foo").setExpression("blah");
}

TEST_F(Model,
       BindingPropertySetDynamicTypeNameAndExpressionIsCallingModelResourceManagementRemoveProperty)
{
    auto property = rootNode.variantProperty("foo");
    property.setValue(4);

    EXPECT_CALL(resourceManagementMock, removeProperty(Eq(property)));

    rootNode.bindingProperty("foo").setDynamicTypeNameAndExpression("int", "blah");
}

TEST_F(Model, SignalHandlerPropertySetSourceIsCallingModelResourceManagementRemoveProperty)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperty(Eq(property)));

    rootNode.signalHandlerProperty("foo").setSource("blah");
}

TEST_F(Model, SignalDeclarationPropertySetSignatureIsCallingModelResourceManagementRemoveProperty)
{
    auto property = rootNode.bindingProperty("foo");
    property.setExpression("blah");

    EXPECT_CALL(resourceManagementMock, removeProperty(Eq(property)));

    rootNode.signalDeclarationProperty("foo").setSignature("blah");
}

TEST_F(Model, ModelNodeDestroyIsCallingAbstractViewNodeAboutToBeRemoved)
{
    auto node = createNodeWithParent(rootNode);
    auto node2 = createNodeWithParent(rootNode);
    ON_CALL(resourceManagementMock, removeNode(node))
        .WillByDefault(Return(ModelResourceSet{{node, node2}, {}, {}}));

    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(Eq(node)));
    EXPECT_CALL(viewMock, nodeAboutToBeRemoved(Eq(node2)));

    node.destroy();
}

TEST_F(Model, ModelNodeDestroyIsCallingAbstractViewNodeRemoved)
{
    auto node = createNodeWithParent(rootNode);
    auto node2 = createNodeWithParent(rootNode);
    ON_CALL(resourceManagementMock, removeNode(node))
        .WillByDefault(Return(ModelResourceSet{{node, node2}, {}, {}}));

    EXPECT_CALL(viewMock, nodeRemoved(Eq(node), _, _));
    EXPECT_CALL(viewMock, nodeRemoved(Eq(node2), _, _));

    node.destroy();
}

TEST_F(Model, ModelNodeDestroyIsCallingAbstractViewNodeRemovedWithValidNodes)
{
    auto node = createNodeWithParent(rootNode);
    auto node2 = createNodeWithParent(rootNode);
    ON_CALL(resourceManagementMock, removeNode(node))
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
    ON_CALL(resourceManagementMock, removeNode(node))
        .WillByDefault(Return(ModelResourceSet{{node}, {property, property2}, {}}));

    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(ElementsAre(Eq(property))));
    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(ElementsAre(Eq(property2))));

    node.destroy();
}

TEST_F(Model, ModelNodeDestroyIsCallingAbstractViewPropertiesRemoved)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createProperty(rootNode, "foo");
    auto property2 = createProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeNode(node))
        .WillByDefault(Return(ModelResourceSet{{node}, {property, property2}, {}}));

    EXPECT_CALL(viewMock, propertiesRemoved(ElementsAre(Eq(property))));
    EXPECT_CALL(viewMock, propertiesRemoved(ElementsAre(Eq(property2))));

    node.destroy();
}

TEST_F(Model, ModelNodeDestroyIsCallingAbstractViewPropertiesRemovedOnlyWithValidProperties)
{
    auto node = createNodeWithParent(rootNode);
    auto property = createProperty(rootNode, "foo");
    auto property2 = createProperty(rootNode, "bar");
    ON_CALL(resourceManagementMock, removeNode(node))
        .WillByDefault(Return(ModelResourceSet{{node}, {property, property2, {}}, {}}));

    EXPECT_CALL(viewMock, propertiesRemoved(ElementsAre(Eq(property))));
    EXPECT_CALL(viewMock, propertiesRemoved(ElementsAre(Eq(property2))));

    node.destroy();
}

TEST_F(Model, ModelNodeDestroyIsCallingAbstractViewBindingPropertiesAboutToBeChanged)
{
    auto node = createNodeWithParent(rootNode);
    auto property = rootNode.bindingProperty("foo");
    auto property2 = rootNode.bindingProperty("bar");
    ON_CALL(resourceManagementMock, removeNode(node))
        .WillByDefault(Return(ModelResourceSet{{node}, {}, {{property, "yi"}, {property2, "er"}}}));

    EXPECT_CALL(viewMock, bindingPropertiesAboutToBeChanged(ElementsAre(Eq(property))));
    EXPECT_CALL(viewMock, bindingPropertiesAboutToBeChanged(ElementsAre(Eq(property2))));

    node.destroy();
}

TEST_F(Model, ModelNodeDestroyIsCallingAbstractViewBindingPropertiesChanged)
{
    auto node = createNodeWithParent(rootNode);
    auto property = rootNode.bindingProperty("foo");
    auto property2 = rootNode.bindingProperty("bar");
    ON_CALL(resourceManagementMock, removeNode(node))
        .WillByDefault(Return(ModelResourceSet{{node}, {}, {{property, "yi"}, {property2, "er"}}}));

    EXPECT_CALL(viewMock, bindingPropertiesChanged(ElementsAre(Eq(property)), _));
    EXPECT_CALL(viewMock, bindingPropertiesChanged(ElementsAre(Eq(property2)), _));

    node.destroy();
}

TEST_F(Model, ModelNodeDestroyIsCallingAbstractViewBindingPropertiesChangedOnlyWithValidProperties)
{
    auto node = createNodeWithParent(rootNode);
    auto property = rootNode.bindingProperty("foo");
    auto property2 = rootNode.bindingProperty("bar");
    ON_CALL(resourceManagementMock, removeNode(node))
        .WillByDefault(Return(
            ModelResourceSet{{node}, {}, {{property, "yi"}, {property2, "er"}, {{}, "san"}}}));

    EXPECT_CALL(viewMock, bindingPropertiesChanged(ElementsAre(Eq(property)), _));
    EXPECT_CALL(viewMock, bindingPropertiesChanged(ElementsAre(Eq(property2)), _));

    node.destroy();
}

TEST_F(Model, ModelNodeRemovePropertyIsCallingAbstractViewNodeAboutToBeRemoved)
{
    auto property = createProperty(rootNode, "foo");
    auto node = createNodeWithParent(rootNode);
    auto node2 = createNodeWithParent(rootNode);
    ON_CALL(resourceManagementMock, removeProperty(property))
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
    ON_CALL(resourceManagementMock, removeProperty(property))
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
    ON_CALL(resourceManagementMock, removeProperty(property))
        .WillByDefault(Return(ModelResourceSet{{node, node2, ModelNode{}}, {property}, {}}));

    EXPECT_CALL(viewMock, nodeRemoved(Eq(node), _, _));
    EXPECT_CALL(viewMock, nodeRemoved(Eq(node2), _, _));

    rootNode.removeProperty("foo");
}

TEST_F(Model, ModelNodeRemovePropertyIsCallingAbstractViewPropertiesAboutToBeRemoved)
{
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");
    ON_CALL(resourceManagementMock, removeProperty(property))
        .WillByDefault(Return(ModelResourceSet{{}, {property, property2}, {}}));

    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(ElementsAre(Eq(property))));
    EXPECT_CALL(viewMock, propertiesAboutToBeRemoved(ElementsAre(Eq(property2))));

    rootNode.removeProperty("yi");
}

TEST_F(Model, ModelNodeRemovePropertyIsCallingAbstractViewPropertiesRemoved)
{
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");
    ON_CALL(resourceManagementMock, removeProperty(property))
        .WillByDefault(Return(ModelResourceSet{{}, {property, property2}, {}}));

    EXPECT_CALL(viewMock, propertiesRemoved(ElementsAre(Eq(property))));
    EXPECT_CALL(viewMock, propertiesRemoved(ElementsAre(Eq(property2))));

    rootNode.removeProperty("yi");
}

TEST_F(Model, ModelNodeRemovePropertyIsCallingAbstractViewPropertiesRemovedOnlyWithValidProperties)
{
    auto property = createProperty(rootNode, "yi");
    auto property2 = createProperty(rootNode, "er");
    ON_CALL(resourceManagementMock, removeProperty(property))
        .WillByDefault(Return(ModelResourceSet{{}, {property, property2, {}}, {}}));

    EXPECT_CALL(viewMock, propertiesRemoved(ElementsAre(Eq(property))));
    EXPECT_CALL(viewMock, propertiesRemoved(ElementsAre(Eq(property2))));

    rootNode.removeProperty("yi");
}

TEST_F(Model, ModelNodeRemovePropertyIsCallingAbstractViewBindingPropertiesAboutToBeChanged)
{
    auto property = createProperty(rootNode, "yi");
    auto property1 = rootNode.bindingProperty("foo");
    auto property2 = rootNode.bindingProperty("bar");
    ON_CALL(resourceManagementMock, removeProperty(property))
        .WillByDefault(
            Return(ModelResourceSet{{}, {property}, {{property1, "yi"}, {property2, "er"}}}));

    EXPECT_CALL(viewMock, bindingPropertiesAboutToBeChanged(ElementsAre(Eq(property1))));
    EXPECT_CALL(viewMock, bindingPropertiesAboutToBeChanged(ElementsAre(Eq(property2))));

    rootNode.removeProperty("yi");
}

TEST_F(Model, ModelNodeRemovePropertyIsCallingAbstractViewBindingPropertiesChanged)
{
    auto property = createProperty(rootNode, "yi");
    auto property1 = rootNode.bindingProperty("foo");
    auto property2 = rootNode.bindingProperty("bar");
    ON_CALL(resourceManagementMock, removeProperty(property))
        .WillByDefault(
            Return(ModelResourceSet{{}, {property}, {{property1, "yi"}, {property2, "er"}}}));

    EXPECT_CALL(viewMock, bindingPropertiesChanged(ElementsAre(Eq(property1)), _));
    EXPECT_CALL(viewMock, bindingPropertiesChanged(ElementsAre(Eq(property2)), _));

    rootNode.removeProperty("yi");
}

TEST_F(Model,
       ModelNodeRemovePropertyIsCallingAbstractViewBindingPropertiesChangedOnlyWithValidProperties)
{
    auto property = createProperty(rootNode, "yi");
    auto property1 = rootNode.bindingProperty("foo");
    auto property2 = rootNode.bindingProperty("bar");
    ON_CALL(resourceManagementMock, removeProperty(property))
        .WillByDefault(
            Return(ModelResourceSet{{}, {property}, {{property1, "yi"}, {property2, "er"}, {}}}));

    EXPECT_CALL(viewMock, bindingPropertiesChanged(ElementsAre(Eq(property1)), _));
    EXPECT_CALL(viewMock, bindingPropertiesChanged(ElementsAre(Eq(property2)), _));

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

} // namespace
