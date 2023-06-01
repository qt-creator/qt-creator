// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "googletest.h"

#include "mocklistmodeleditorview.h"
#include "modelresourcemanagementmock.h"
#include "projectstoragemock.h"

#include <include/bindingproperty.h>
#include <include/model.h>
#include <include/modelnode.h>
#include <include/nodeabstractproperty.h>
#include <include/nodelistproperty.h>
#include <include/nodeproperty.h>
#include <include/signalhandlerproperty.h>
#include <include/variantproperty.h>
#include <model/modelresourcemanagement.h>

namespace {
using QmlDesigner::AbstractProperty;
using QmlDesigner::ModelNode;
using QmlDesigner::ModelNodes;
using QmlDesigner::ModelResourceSet;

template<typename Matcher>
auto SetExpressionProperty(const Matcher &matcher)
{
    return Field("property", &QmlDesigner::ModelResourceSet::SetExpression::property, matcher);
}

template<typename Matcher>
auto SetExpressionExpression(const Matcher &matcher)
{
    return Field("expression", &QmlDesigner::ModelResourceSet::SetExpression::expression, matcher);
}

template<typename PropertyMatcher, typename ExpressionMatcher>
auto SetExpression(const PropertyMatcher &propertyMatcher, const ExpressionMatcher &expressionMatcher)
{
    return AllOf(SetExpressionProperty(propertyMatcher), SetExpressionExpression(expressionMatcher));
}

class ModelResourceManagement : public testing::Test
{
protected:
    ModelResourceManagement()
    {
        model.attachView(&viewMock);
        rootNode = model.rootModelNode();
    }
    auto createNodeWithParent(const QmlDesigner::TypeName &typeName,
                              QmlDesigner::NodeAbstractProperty parentProperty,
                              const QString &id = {})
    {
        auto node = model.createModelNode(typeName);
        parentProperty.reparentHere(node);
        if (id.size())
            node.setIdWithoutRefactoring(id);

        return node;
    }

protected:
    NiceMock<MockListModelEditorView> viewMock;
    NiceMock<ProjectStorageMockWithQtQtuick> projectStorageMock;
    QmlDesigner::ModelResourceManagement management;
    QmlDesigner::Model model{projectStorageMock, "QtQuick.Item"};
    ModelNode rootNode;
};

TEST_F(ModelResourceManagement, RemoveProperty)
{
    auto layerEnabledProperty = rootNode.variantProperty("layer.enabled");
    layerEnabledProperty.setValue(true);

    auto resources = management.removeProperties({layerEnabledProperty}, &model);

    ASSERT_THAT(resources.removeProperties, Contains(layerEnabledProperty));
}

TEST_F(ModelResourceManagement, RemoveMultipleProperties)
{
    auto layerEnabledProperty = rootNode.variantProperty("layer.enabled");
    layerEnabledProperty.setValue(true);
    auto layerHiddenProperty = rootNode.variantProperty("layer.hidden");
    layerHiddenProperty.setValue(true);

    auto resources = management.removeProperties({layerEnabledProperty, layerHiddenProperty}, &model);

    ASSERT_THAT(resources.removeProperties, IsSupersetOf({layerEnabledProperty, layerHiddenProperty}));
}

TEST_F(ModelResourceManagement, RemoveNode)
{
    auto node = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty());

    auto resources = management.removeNodes({node}, &model);

    ASSERT_THAT(resources.removeModelNodes, Contains(node));
}

TEST_F(ModelResourceManagement, RemoveMultipleNodes)
{
    auto node = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty());
    auto node2 = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty());

    auto resources = management.removeNodes({node, node2}, &model);

    ASSERT_THAT(resources.removeModelNodes, IsSupersetOf({node, node2}));
}

TEST_F(ModelResourceManagement, DontRemoveChildNodes)
{
    auto node = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty());
    auto childNode = createNodeWithParent("QtQuick.Item", node.defaultNodeListProperty());

    auto resources = management.removeNodes({node}, &model);

    ASSERT_THAT(resources.removeModelNodes, Not(Contains(childNode)));
}

TEST_F(ModelResourceManagement, RemovePropertyLayerEnabled)
{
    auto node = createNodeWithParent("QtQuick.Item", rootNode.nodeProperty("layer.effect"));
    auto layerEnabledProperty = rootNode.variantProperty("layer.enabled");
    layerEnabledProperty.setValue(true);

    auto resources = management.removeNodes({node}, &model);

    ASSERT_THAT(resources.removeProperties, Contains(layerEnabledProperty));
}

TEST_F(ModelResourceManagement, RemovePropertyLayerEnabledIfLayerEffectPropertyIsRemoved)
{
    auto layerEffectProperty = rootNode.nodeProperty("layer.effect");
    auto node = createNodeWithParent("QtQuick.Item", layerEffectProperty);
    auto layerEnabledProperty = rootNode.variantProperty("layer.enabled");
    layerEnabledProperty.setValue(true);

    auto resources = management.removeProperties({layerEffectProperty}, &model);

    ASSERT_THAT(resources.removeProperties, Contains(layerEnabledProperty));
}

TEST_F(ModelResourceManagement, DontRemovePropertyLayerEnabledIfNotExists)
{
    auto node = createNodeWithParent("QtQuick.Item", rootNode.nodeProperty("layer.effect"));
    auto layerEnabledProperty = rootNode.variantProperty("layer.enabled");

    auto resources = management.removeNodes({node}, &model);

    ASSERT_THAT(resources.removeProperties, Not(Contains(layerEnabledProperty)));
}

TEST_F(ModelResourceManagement, RemoveAliasExportProperty)
{
    auto fooNode = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty(), "foo");
    auto aliasExportProperty = rootNode.bindingProperty("foo");
    aliasExportProperty.setDynamicTypeNameAndExpression("alias", "foo");

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeProperties, Contains(aliasExportProperty));
}

TEST_F(ModelResourceManagement, RemoveAliasForChildExportProperty)
{
    auto node = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty());
    auto fooNode = createNodeWithParent("QtQuick.Item", node.defaultNodeListProperty(), "foo");
    auto aliasExportProperty = rootNode.bindingProperty("foo");
    aliasExportProperty.setDynamicTypeNameAndExpression("alias", "foo");

    auto resources = management.removeNodes({node}, &model);

    ASSERT_THAT(resources.removeProperties, Contains(aliasExportProperty));
}

TEST_F(ModelResourceManagement, RemoveAliasForGrandChildExportProperty)
{
    auto node = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty());
    auto childNode = createNodeWithParent("QtQuick.Item", node.defaultNodeListProperty());
    auto fooNode = createNodeWithParent("QtQuick.Item", childNode.defaultNodeListProperty(), "foo");
    auto aliasExportProperty = rootNode.bindingProperty("foo");
    aliasExportProperty.setDynamicTypeNameAndExpression("alias", "foo");

    auto resources = management.removeNodes({node}, &model);

    ASSERT_THAT(resources.removeProperties, Contains(aliasExportProperty));
}

TEST_F(ModelResourceManagement, DontRemoveNonAliasExportProperty)
{
    auto fooNode = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty(), "foo");
    auto aliasExportProperty = rootNode.bindingProperty("foo");
    aliasExportProperty.setDynamicTypeNameAndExpression("int", "foo");

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeProperties, Not(Contains(aliasExportProperty)));
}

struct TargetData
{
    QmlDesigner::TypeName targetType;
    QmlDesigner::TypeName type;
    QmlDesigner::PropertyName propertyName;

    // printer function for TargetData - don't remove
    friend std::ostream &operator<<(std::ostream &os, const TargetData &data)
    {
        return os << "(" << data.targetType << ", " << data.type << ")";
    }
};

class ForTarget : public ModelResourceManagement, public testing::WithParamInterface<TargetData>
{
protected:
    TargetData parameters = GetParam();
    ModelNode fooNode = createNodeWithParent(parameters.targetType,
                                             rootNode.defaultNodeListProperty(),
                                             "foo");
    ModelNode barNode = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty(), "bar");
    ModelNode source = createNodeWithParent(parameters.type,
                                            rootNode.defaultNodeListProperty(),
                                            "source1");
    ModelNode source2 = createNodeWithParent(parameters.type,
                                             rootNode.defaultNodeListProperty(),
                                             "source2");
    QmlDesigner::BindingProperty sourceTargetProperty = source.bindingProperty(parameters.propertyName);
    QmlDesigner::BindingProperty source2TargetProperty = source2.bindingProperty(
        parameters.propertyName);
};

INSTANTIATE_TEST_SUITE_P(
    ModelResourceManagement,
    ForTarget,
    testing::Values(TargetData{"QtQuick.Item", "QtQuick.PropertyChanges", "target"},
                    TargetData{"QtQuick.Item", "QtQuick.Timeline.KeyframeGroup", "target"},
                    TargetData{"FlowView.FlowTransition", "FlowView.FlowActionArea", "target"},
                    TargetData{"QtQuick.Item", "QtQuick.PropertyAnimation", "target"},
                    TargetData{"FlowView.FlowItem", "FlowView.FlowTransition", "to"},
                    TargetData{"FlowView.FlowItem", "FlowView.FlowTransition", "from"}));

TEST_P(ForTarget, Remove)
{
    sourceTargetProperty.setExpression("foo");
    source2TargetProperty.setExpression("foo");

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, IsSupersetOf({source, source2}));
}

TEST_P(ForTarget, DontRemoveForDifferentTarget)
{
    sourceTargetProperty.setExpression("bar");

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, Not(Contains(source)));
}

TEST_P(ForTarget, DontRemoveKeyIfTargetIsNotSet)
{
    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, Not(Contains(source)));
}

TEST_P(ForTarget, DontRemoveIfTargetCannotBeResolved)
{
    sourceTargetProperty.setExpression("not_exists");

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, Not(Contains(source)));
}

class ForTargets : public ModelResourceManagement, public testing::WithParamInterface<TargetData>
{
protected:
    TargetData parameters = GetParam();
    ModelNode fooNode = createNodeWithParent(parameters.targetType,
                                             rootNode.defaultNodeListProperty(),
                                             "foo");
    ModelNode barNode = createNodeWithParent(parameters.targetType,
                                             rootNode.defaultNodeListProperty(),
                                             "bar");
    ModelNode yiNode = createNodeWithParent(parameters.targetType,
                                            rootNode.defaultNodeListProperty(),
                                            "yi");
    ModelNode erNode = createNodeWithParent(parameters.targetType,
                                            rootNode.defaultNodeListProperty(),
                                            "er");
    ModelNode sanNode = createNodeWithParent(parameters.targetType,
                                             rootNode.defaultNodeListProperty(),
                                             "san");
    ModelNode source = createNodeWithParent(parameters.type,
                                            rootNode.defaultNodeListProperty(),
                                            "source1");
    ModelNode source2 = createNodeWithParent(parameters.type,
                                             rootNode.defaultNodeListProperty(),
                                             "source2");
    QmlDesigner::BindingProperty sourceTargetsProperty = source.bindingProperty(
        parameters.propertyName);
    QmlDesigner::BindingProperty source2TargetsProperty = source2.bindingProperty(
        parameters.propertyName);
};

INSTANTIATE_TEST_SUITE_P(
    ModelResourceManagement,
    ForTargets,
    testing::Values(TargetData{"FlowView.FlowTransition", "FlowView.FlowDecision", "targets"},
                    TargetData{"FlowView.FlowTransition", "FlowView.FlowWildcard", "targets"},
                    TargetData{"QtQuick.Item", "QtQuick.PropertyAnimation", "targets"}));

TEST_P(ForTargets, Remove)
{
    sourceTargetsProperty.setExpression("[foo]");
    source2TargetsProperty.setExpression("[foo]");

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, IsSupersetOf({source, source2}));
}

TEST_P(ForTargets, HandleInvalidBinding)
{
    sourceTargetsProperty.setExpression("[foo, broken]");
    source2TargetsProperty.setExpression("[foo, fail]");

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, IsSupersetOf({source, source2}));
}

TEST_P(ForTargets, RemoveIndirectly)
{
    auto parenNode = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty(), "hoo");
    parenNode.defaultNodeListProperty().reparentHere(fooNode);
    parenNode.defaultNodeListProperty().reparentHere(barNode);
    sourceTargetsProperty.setExpression("[foo, bar]");
    source2TargetsProperty.setExpression("[bar, foo]");

    auto resources = management.removeNodes({parenNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, IsSupersetOf({source, source2}));
}

TEST_P(ForTargets, DontRemoveTargetIfThereAreStillAnOtherTargets)
{
    sourceTargetsProperty.setExpression("[foo, bar]");
    source2TargetsProperty.setExpression("[foo, bar]");

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, AllOf(Not(Contains(source)), Not(Contains(source2))));
}

TEST_P(ForTargets, ChangeExpressionIfThereAreStillAnOtherTargets)
{
    sourceTargetsProperty.setExpression("[foo, bar]");
    source2TargetsProperty.setExpression("[foo, bar]");

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.setExpressions,
                UnorderedElementsAre(SetExpression(sourceTargetsProperty, "[bar]"),
                                     SetExpression(source2TargetsProperty, "[bar]")));
}

TEST_P(ForTargets, DontChangeOrderInExpression)
{
    sourceTargetsProperty.setExpression("[yi, foo, er, san]");

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.setExpressions,
                UnorderedElementsAre(SetExpression(sourceTargetsProperty, "[yi, er, san]")));
}

struct StateData
{
    QmlDesigner::TypeName type;
    QmlDesigner::PropertyName propertyName;

    // printer function for TargetData - don't remove
    friend std::ostream &operator<<(std::ostream &os, const StateData &data)
    {
        return os << "(" << data.type << ", " << data.propertyName << ")";
    }
};

class ForState : public ModelResourceManagement, public testing::WithParamInterface<StateData>
{
protected:
    ModelNode createStateWithParent(QmlDesigner::NodeAbstractProperty parentProperty,
                                    const QString &name)
    {
        ModelNode stateNode = createNodeWithParent("QtQuick.State", parentProperty, name);
        stateNode.variantProperty("name").setValue(name);

        return stateNode;
    }

protected:
    StateData parameters = GetParam();
    ModelNode fooNode = createStateWithParent(rootNode.defaultNodeListProperty(), "foo");
    ModelNode barNode = createStateWithParent(rootNode.defaultNodeListProperty(), "bar");
    ModelNode yiNode = createStateWithParent(rootNode.defaultNodeListProperty(), "yi");
    ModelNode erNode = createStateWithParent(rootNode.defaultNodeListProperty(), "er");
    ModelNode sanNode = createStateWithParent(rootNode.defaultNodeListProperty(), "san");
    ModelNode source = createNodeWithParent(parameters.type,
                                            rootNode.defaultNodeListProperty(),
                                            "source1");
    ModelNode source2 = createNodeWithParent(parameters.type,
                                             rootNode.defaultNodeListProperty(),
                                             "source2");
    QmlDesigner::VariantProperty sourceStateProperty = source.variantProperty(parameters.propertyName);
    QmlDesigner::VariantProperty source2StateProperty = source2.variantProperty(
        parameters.propertyName);
};

INSTANTIATE_TEST_SUITE_P(ModelResourceManagement,
                         ForState,
                         testing::Values(StateData{"QtQuick.Transition", "from"},
                                         StateData{"QtQuick.Transition", "to"}));

TEST_P(ForState, Remove)
{
    sourceStateProperty.setValue(QString{"foo"});
    source2StateProperty.setValue(QString{"foo"});

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, IsSupersetOf({source, source2}));
}

TEST_P(ForState, DontRemoveForStarState)
{
    fooNode.variantProperty("name").setValue("*");
    sourceStateProperty.setValue(QString{"*"});

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, Not(Contains(source)));
}

TEST_P(ForState, DontRemoveForEmptyState)
{
    fooNode.variantProperty("name").setValue("");
    sourceStateProperty.setValue(QString{""});

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, Not(Contains(source)));
}

TEST_P(ForState, DontRemoveForDifferentState)
{
    sourceStateProperty.setValue(QString{"foo"});
    source2StateProperty.setValue(QString{"bar"});

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, IsSupersetOf({source}));
}

} // namespace
