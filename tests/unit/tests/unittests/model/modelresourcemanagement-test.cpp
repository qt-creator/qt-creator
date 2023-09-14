// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../../utils/googletest.h"

#include "../mocks/abstractviewmock.h"
#include "../mocks/modelresourcemanagementmock.h"
#include "../mocks/projectstoragemock.h"
#include "../mocks/sourcepathcachemock.h"

#include <include/bindingproperty.h>
#include <include/model.h>
#include <include/modelnode.h>
#include <include/nodeabstractproperty.h>
#include <include/nodelistproperty.h>
#include <include/nodemetainfo.h>
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
        auto itemId = rootNode.metaInfo().id();
        projectStorageMock.createProperty(itemId,
                                          "layer.effect",
                                          QmlDesigner::Storage::PropertyDeclarationTraits::IsList,
                                          itemId);
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
    NiceMock<AbstractViewMock> viewMock;
    NiceMock<SourcePathCacheMockWithPaths> pathCacheMock{"/path/foo.qml"};
    NiceMock<ProjectStorageMockWithQtQtuick> projectStorageMock{pathCacheMock.sourceId};
    QmlDesigner::ModelResourceManagement management;
    QmlDesigner::Model model{{projectStorageMock, pathCacheMock},
                             "Item",
                             {QmlDesigner::Import::createLibraryImport("QtQtuick")},
                             QUrl::fromLocalFile(pathCacheMock.path.toQString())};
    ModelNode rootNode;
};

TEST_F(ModelResourceManagement, remove_property)
{
    auto layerEnabledProperty = rootNode.variantProperty("layer.enabled");
    layerEnabledProperty.setValue(true);

    auto resources = management.removeProperties({layerEnabledProperty}, &model);

    ASSERT_THAT(resources.removeProperties, Contains(layerEnabledProperty));
}

TEST_F(ModelResourceManagement, remove_multiple_properties)
{
    auto layerEnabledProperty = rootNode.variantProperty("layer.enabled");
    layerEnabledProperty.setValue(true);
    auto layerHiddenProperty = rootNode.variantProperty("layer.hidden");
    layerHiddenProperty.setValue(true);

    auto resources = management.removeProperties({layerEnabledProperty, layerHiddenProperty}, &model);

    ASSERT_THAT(resources.removeProperties, IsSupersetOf({layerEnabledProperty, layerHiddenProperty}));
}

TEST_F(ModelResourceManagement, remove_node)
{
    auto node = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty());

    auto resources = management.removeNodes({node}, &model);

    ASSERT_THAT(resources.removeModelNodes, Contains(node));
}

TEST_F(ModelResourceManagement, remove_multiple_nodes)
{
    auto node = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty());
    auto node2 = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty());

    auto resources = management.removeNodes({node, node2}, &model);

    ASSERT_THAT(resources.removeModelNodes, IsSupersetOf({node, node2}));
}

TEST_F(ModelResourceManagement, remove_multiple_nodes_once)
{
    auto node = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty());
    auto node2 = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty());

    auto resources = management.removeNodes({node, node2, node, node2}, &model);

    ASSERT_THAT(resources.removeModelNodes, UnorderedElementsAre(node, node2));
}

TEST_F(ModelResourceManagement, dont_remove_child_nodes)
{
    auto node = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty());
    auto childNode = createNodeWithParent("QtQuick.Item", node.defaultNodeListProperty());

    auto resources = management.removeNodes({node}, &model);

    ASSERT_THAT(resources.removeModelNodes, Not(Contains(childNode)));
}

TEST_F(ModelResourceManagement, remove_property_layer_enabled)
{
    auto node = createNodeWithParent("QtQuick.Item", rootNode.nodeProperty("layer.effect"));
    auto layerEnabledProperty = rootNode.variantProperty("layer.enabled");
    layerEnabledProperty.setValue(true);

    auto resources = management.removeNodes({node}, &model);

    ASSERT_THAT(resources.removeProperties, Contains(layerEnabledProperty));
}

TEST_F(ModelResourceManagement, remove_property_layer_enabled_if_layer_effect_property_is_removed)
{
    auto layerEffectProperty = rootNode.nodeProperty("layer.effect");
    auto node = createNodeWithParent("QtQuick.Item", layerEffectProperty);
    auto layerEnabledProperty = rootNode.variantProperty("layer.enabled");
    layerEnabledProperty.setValue(true);

    auto resources = management.removeProperties({layerEffectProperty}, &model);

    ASSERT_THAT(resources.removeProperties, Contains(layerEnabledProperty));
}

TEST_F(ModelResourceManagement, dont_remove_property_layer_enabled_if_not_exists)
{
    auto node = createNodeWithParent("QtQuick.Item", rootNode.nodeProperty("layer.effect"));
    auto layerEnabledProperty = rootNode.variantProperty("layer.enabled");

    auto resources = management.removeNodes({node}, &model);

    ASSERT_THAT(resources.removeProperties, Not(Contains(layerEnabledProperty)));
}

TEST_F(ModelResourceManagement, remove_property_with_id)
{
    auto fooNode = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty(), "foo");
    auto property = rootNode.bindingProperty("foo");
    property.setDynamicTypeNameAndExpression("var", "foo");

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeProperties, Contains(property));
}

TEST_F(ModelResourceManagement, remove_property_with_id_in_complex_expression)
{
    auto fooNode = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty(), "foo");
    auto foobarProperty = rootNode.bindingProperty("foo");
    foobarProperty.setDynamicTypeNameAndExpression("var", "foo.x+bar.y");
    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeProperties, Contains(foobarProperty));
}

TEST_F(ModelResourceManagement, dont_remove_property_without_id)
{
    auto fooNode = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty(), "foo");
    auto barNode = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty(), "bar");
    auto fooProperty = rootNode.bindingProperty("foo");
    fooProperty.setDynamicTypeNameAndExpression("var", "foo.x");

    auto resources = management.removeNodes({barNode}, &model);

    ASSERT_THAT(resources.removeProperties, Not(Contains(fooProperty)));
}

TEST_F(ModelResourceManagement, remove_property_with_id_once)
{
    auto fooNode = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty(), "foo");
    auto barNode = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty(), "bar");
    auto foobarProperty = rootNode.bindingProperty("foo");
    foobarProperty.setDynamicTypeNameAndExpression("var", "foo.x+bar.y");

    auto resources = management.removeNodes({fooNode, barNode}, &model);

    ASSERT_THAT(resources.removeProperties, ElementsAre(foobarProperty));
}

TEST_F(ModelResourceManagement, remove_alias_property_with_id)
{
    auto fooNode = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty(), "foo");
    auto property = rootNode.bindingProperty("foo");
    property.setDynamicTypeNameAndExpression("alias", "foo");

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeProperties, Contains(property));
}

TEST_F(ModelResourceManagement, dont_remove_property_with_different_id_which_contains_id_string)
{
    auto fooNode = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty(), "foo");
    auto property = rootNode.bindingProperty("foo");
    property.setDynamicTypeNameAndExpression("var", "foobar+barfoo");

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeProperties, Not(Contains(property)));
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

class ForTarget : public ModelResourceManagement,
                  public testing::WithParamInterface<TargetData>
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

TEST_P(ForTarget, remove)
{
    sourceTargetProperty.setExpression("foo");
    source2TargetProperty.setExpression("foo");

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, IsSupersetOf({source, source2}));
}

TEST_P(ForTarget, dont_remove_for_different_target)
{
    sourceTargetProperty.setExpression("bar");

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, Not(Contains(source)));
}

TEST_P(ForTarget, dont_remove_key_if_target_is_not_set)
{
    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, Not(Contains(source)));
}

TEST_P(ForTarget, dont_remove_if_target_cannot_be_resolved)
{
    sourceTargetProperty.setExpression("not_exists");

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, Not(Contains(source)));
}

class ForTargets : public ModelResourceManagement,
                   public testing::WithParamInterface<TargetData>
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

TEST_P(ForTargets, remove)
{
    sourceTargetsProperty.setExpression("[foo]");
    source2TargetsProperty.setExpression("[foo]");

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, IsSupersetOf({source, source2}));
}

TEST_P(ForTargets, handle_invalid_binding)
{
    sourceTargetsProperty.setExpression("[foo, broken]");
    source2TargetsProperty.setExpression("[foo, fail]");

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, IsSupersetOf({source, source2}));
}

TEST_P(ForTargets, remove_indirectly)
{
    auto parenNode = createNodeWithParent("QtQuick.Item", rootNode.defaultNodeListProperty(), "hoo");
    parenNode.defaultNodeListProperty().reparentHere(fooNode);
    parenNode.defaultNodeListProperty().reparentHere(barNode);
    sourceTargetsProperty.setExpression("[foo, bar]");
    source2TargetsProperty.setExpression("[bar, foo]");

    auto resources = management.removeNodes({parenNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, IsSupersetOf({source, source2}));
}

TEST_P(ForTargets, dont_remove_target_if_there_are_still_an_other_targets)
{
    sourceTargetsProperty.setExpression("[foo, bar]");
    source2TargetsProperty.setExpression("[foo, bar]");

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, AllOf(Not(Contains(source)), Not(Contains(source2))));
}

TEST_P(ForTargets, change_expression_if_there_are_still_an_other_targets)
{
    sourceTargetsProperty.setExpression("[foo, bar]");
    source2TargetsProperty.setExpression("[foo, bar]");

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.setExpressions,
                UnorderedElementsAre(SetExpression(sourceTargetsProperty, "[bar]"),
                                     SetExpression(source2TargetsProperty, "[bar]")));
}

TEST_P(ForTargets, dont_change_order_in_expression)
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

class ForState : public ModelResourceManagement,
                 public testing::WithParamInterface<StateData>
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

TEST_P(ForState, remove)
{
    sourceStateProperty.setValue(QString{"foo"});
    source2StateProperty.setValue(QString{"foo"});

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, IsSupersetOf({source, source2}));
}

TEST_P(ForState, dont_remove_for_star_state)
{
    fooNode.variantProperty("name").setValue("*");
    sourceStateProperty.setValue(QString{"*"});

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, Not(Contains(source)));
}

TEST_P(ForState, dont_remove_for_empty_state)
{
    fooNode.variantProperty("name").setValue("");
    sourceStateProperty.setValue(QString{""});

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, Not(Contains(source)));
}

TEST_P(ForState, dont_remove_for_different_state)
{
    sourceStateProperty.setValue(QString{"foo"});
    source2StateProperty.setValue(QString{"bar"});

    auto resources = management.removeNodes({fooNode}, &model);

    ASSERT_THAT(resources.removeModelNodes, IsSupersetOf({source}));
}

} // namespace
