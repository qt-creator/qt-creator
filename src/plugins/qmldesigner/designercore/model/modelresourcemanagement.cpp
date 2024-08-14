// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modelresourcemanagement.h"

#include <abstractview.h>
#include <bindingproperty.h>
#include <nodelistproperty.h>
#include <nodemetainfo.h>
#include <variantproperty.h>

#include <utils/algorithm.h>
#include <utils/set_algorithm.h>

#include <QHash>
#include <QRegularExpression>

#include <concepts>
#include <functional>

QT_WARNING_DISABLE_GCC("-Wmissing-field-initializers")

namespace QmlDesigner {

ModelResourceManagementInterface::~ModelResourceManagementInterface() = default;

namespace {

enum class CheckRecursive { No, Yes };

struct Base;

template<typename T, typename... Actions>
concept NodeAction = std::derived_from<T, Base>
                     && requires(T t,
                                 ModelNodes nodes,
                                 AbstractProperties properties,
                                 ModelResourceSet &resources,
                                 CheckRecursive checkRecursive,
                                 std::tuple<Actions...> &actions) {
                            t.removeNodes(nodes, checkRecursive, actions);
                            t.checkModelNodes(nodes, actions);
                            t.removeProperties(properties, checkRecursive, actions);
                            t.finally();
                            t.handleNodes(nodes, actions);
                            t.handleProperties(properties, actions);
                            t.setModelResourceSet(resources);
                        };

template<typename T>
concept NodeActions = requires(T nodeActions) {
    std::apply([](NodeAction auto &...) {}, nodeActions);
};

void forEachAction(NodeActions auto &nodeActions, auto &&actionCall)
{
    std::apply([&](NodeAction auto &...action) { (actionCall(action), ...); }, nodeActions);
}

struct Base
{
    void removeNodes(ModelNodes newModelNodes,
                     CheckRecursive checkRecursive,
                     NodeActions auto &nodeActions)
    {
        if (newModelNodes.empty())
            return;

        auto oldModelNodes = removeNodes(newModelNodes);

        if (checkRecursive == CheckRecursive::Yes)
            checkNewModelNodes(newModelNodes, oldModelNodes, nodeActions);
    }

    void checkModelNodes(ModelNodes newModelNodes, NodeActions auto &nodeActions)
    {
        if (newModelNodes.empty())
            return;

        std::sort(newModelNodes.begin(), newModelNodes.end());

        checkNewModelNodes(newModelNodes, resourceSet->removeModelNodes, nodeActions);
    }

    void removeProperties(AbstractProperties newProperties,
                          CheckRecursive checkRecursive,
                          NodeActions auto &nodeActions)
    {
        if (newProperties.empty())
            return;

        auto oldProperties = removeProperties(newProperties);

        if (checkRecursive == CheckRecursive::Yes)
            checkNewProperties(newProperties, oldProperties, nodeActions);
    }

    auto &setExpressions() { return resourceSet->setExpressions; }

    void handleNodes(const ModelNodes &, [[maybe_unused]] NodeActions auto &actions) {}

    void handleProperties(const AbstractProperties &, [[maybe_unused]] NodeActions auto &actions) {}

    void finally() {}

    void setModelResourceSet(ModelResourceSet &resources) { resourceSet = &resources; }

private:
    ModelNodes removeNodes(ModelNodes &newModelNodes)
    {
        std::sort(newModelNodes.begin(), newModelNodes.end());

        newModelNodes.erase(std::unique(newModelNodes.begin(), newModelNodes.end()),
                            newModelNodes.end());

        auto oldModelNodes = std::move(resourceSet->removeModelNodes);
        resourceSet->removeModelNodes = {};
        resourceSet->removeModelNodes.reserve(oldModelNodes.size() + newModelNodes.size());

        std::set_union(newModelNodes.begin(),
                       newModelNodes.end(),
                       oldModelNodes.begin(),
                       oldModelNodes.end(),
                       std::back_inserter(resourceSet->removeModelNodes));

        return oldModelNodes;
    }

    AbstractProperties removeProperties(AbstractProperties &newProperties)
    {
        std::sort(newProperties.begin(), newProperties.end());

        newProperties.erase(std::unique(newProperties.begin(), newProperties.end()),
                            newProperties.end());

        auto oldProperties = std::move(resourceSet->removeProperties);
        resourceSet->removeProperties = {};
        resourceSet->removeProperties.reserve(oldProperties.size() + newProperties.size());

        std::set_union(newProperties.begin(),
                       newProperties.end(),
                       oldProperties.begin(),
                       oldProperties.end(),
                       std::back_inserter(resourceSet->removeProperties));

        return oldProperties;
    }

    void checkNewModelNodes(const ModelNodes &newModelNodes,
                            const ModelNodes &oldModelNodes,
                            NodeActions auto &nodeActions)
    {
        ModelNodes addedModelNodes;
        addedModelNodes.reserve(newModelNodes.size());

        std::set_difference(newModelNodes.begin(),
                            newModelNodes.end(),
                            oldModelNodes.begin(),
                            oldModelNodes.end(),
                            std::back_inserter(addedModelNodes));

        if (addedModelNodes.size())
            forEachAction(nodeActions, [&](NodeAction auto &action) {
                action.handleNodes(addedModelNodes, nodeActions);
            });
    }

    void checkNewProperties(const AbstractProperties &newProperties,
                            const AbstractProperties &oldProperties,
                            NodeActions auto &nodeActions)
    {
        AbstractProperties addedProperties;
        addedProperties.reserve(newProperties.size());

        std::set_difference(newProperties.begin(),
                            newProperties.end(),
                            oldProperties.begin(),
                            oldProperties.end(),
                            std::back_inserter(addedProperties));

        if (addedProperties.size())
            forEachAction(nodeActions, [&](NodeAction auto &action) {
                action.handleProperties(addedProperties, nodeActions);
            });
    }

private:
    ModelResourceSet *resourceSet = nullptr;
};

struct CheckChildNodes : public Base
{
    using Base::Base;

    void handleNodes(const ModelNodes &nodes, NodeActions auto &actions)
    {
        ModelNodes childNodes;
        for (const ModelNode &node : nodes)
            childNodes.append(node.directSubModelNodes());

        checkModelNodes(std::move(childNodes), actions);
    }
};

struct CheckNodesInNodeAbstractProperties : public Base
{
    using Base::Base;

    ModelNodes collectNodes(const AbstractProperties &properties)
    {
        ModelNodes modelNodes;

        for (const AbstractProperty &property : properties) {
            if (property.isNodeAbstractProperty())
                modelNodes.append(property.toNodeAbstractProperty().directSubNodes());
        }

        return modelNodes;
    }

    void handleProperties(const AbstractProperties &properties, auto &actions)
    {
        checkModelNodes(collectNodes(properties), actions);
    }
};

struct RemoveLayerEnabled : public Base
{
    using Base::Base;

    AbstractProperties collectProperties(const ModelNodes &nodes)
    {
        AbstractProperties properties;

        for (const ModelNode &node : nodes) {
            if (node.parentProperty().name() == "layer.effect") {
                auto layerEnabledProperty = node.parentProperty().parentModelNode().property(
                    "layer.enabled");
                if (layerEnabledProperty.exists())
                    properties.push_back(layerEnabledProperty);
            }
        }

        return properties;
    }

    void handleNodes(const ModelNodes &nodes, auto &actions)
    {
        removeProperties(collectProperties(nodes), CheckRecursive::No, actions);
    }
};

struct NodeDependency
{
    ModelNode target;
    ModelNode source;

    friend bool operator<(const NodeDependency &first, const NodeDependency &second)
    {
        return std::tie(first.target, first.source) < std::tie(second.target, second.source);
    }
};

using NodeDependencies = std::vector<NodeDependency>;

struct BindingDependency
{
    ModelNode target;
    BindingProperty property;

    friend bool operator<(const BindingDependency &first, const BindingDependency &second)
    {
        return std::tie(first.target, first.property) < std::tie(second.target, second.property);
    }
};

using BindingDependencies = std::vector<BindingDependency>;

struct NameNode
{
    QString name;
    ModelNode node;

    friend bool operator<(const NameNode &first, const NameNode &second)
    {
        return first.name < second.name;
    }
};

using NameNodes = std::vector<NameNode>;

struct NodesProperty
{
    ModelNode source;
    PropertyName name;
    ModelNodes targets;
    bool isChanged = false;

    friend bool operator<(const NodesProperty &first, const NodesProperty &second)
    {
        return first.source < second.source;
    }
};

using NodesProperties = std::vector<NodesProperty>;

#include <concepts>
struct RemoveDependentBindings : public Base
{
    AbstractProperties collectProperties(const ModelNodes &nodes)
    {
        AbstractProperties properties;
        Utils::set_greedy_intersection(
            dependencies,
            nodes,
            [&](const BindingDependency &dependency) { properties.push_back(dependency.property); },
            {},
            &BindingDependency::target);

        return properties;
    }

    void handleNodes(const ModelNodes &nodes, auto &actions)
    {
        removeProperties(collectProperties(nodes), CheckRecursive::No, actions);
    }

    BindingDependencies dependencies;
};

struct RemoveDependencies : public Base
{

    ModelNodes collectNodes(const ModelNodes &nodes) const
    {
        ModelNodes targetNodes;
        ::Utils::set_greedy_intersection(
            dependencies,
            nodes,
            [&](const NodeDependency &dependency) { targetNodes.push_back(dependency.source); },
            {},
            &NodeDependency::target);

        return targetNodes;
    }

    void handleNodes(const ModelNodes &nodes, auto &actions)
    {
        removeNodes(collectNodes(nodes), CheckRecursive::No, actions);
    }

    NodeDependencies dependencies;
};

struct RemoveTargetsSources : public Base
{
    static void removeDependency(NodesProperties &removedTargetNodesInProperties,
                                 const NodeDependency &dependency)
    {
        auto found = std::ranges::find(removedTargetNodesInProperties,
                                       dependency.source,
                                       &NodesProperty::source);

        if (found == removedTargetNodesInProperties.end())
            removedTargetNodesInProperties.push_back({dependency.source, "", {dependency.target}});
        else
            found->targets.push_back(dependency.target);
    }

    NodesProperties collectRemovedDependencies(const ModelNodes &nodes)
    {
        NodesProperties removedTargetNodesInProperties;

        Utils::set_greedy_intersection(
            dependencies,
            nodes,
            [&](const NodeDependency &dependency) {
                removeDependency(removedTargetNodesInProperties, dependency);
            },
            {},
            &NodeDependency::target);

        std::sort(removedTargetNodesInProperties.begin(), removedTargetNodesInProperties.end());

        return removedTargetNodesInProperties;
    }

    ModelNodes collectNodesToBeRemoved(const ModelNodes &nodes)
    {
        ModelNodes nodesToBeRemoved;

        auto removeTargets = [&](auto &first, auto &second) {
            auto newEnd = std::remove_if(first.targets.begin(),
                                         first.targets.end(),
                                         [&](const ModelNode &node) {
                                             return std::find(second.targets.begin(),
                                                              second.targets.end(),
                                                              node)
                                                    != second.targets.end();
                                         });
            if (newEnd != first.targets.end()) {
                first.isChanged = true;
                first.targets.erase(newEnd, first.targets.end());

                if (first.targets.empty())
                    nodesToBeRemoved.push_back(first.source);
            }
        };

        NodesProperties removedTargetNodesInProperties = collectRemovedDependencies(nodes);
        ::Utils::set_intersection_compare(nodesProperties.begin(),
                                          nodesProperties.end(),
                                          removedTargetNodesInProperties.begin(),
                                          removedTargetNodesInProperties.end(),
                                          removeTargets,
                                          std::less<NodesProperty>{});

        return nodesToBeRemoved;
    }

    void handleNodes(const ModelNodes &nodes, auto &actions)
    {
        removeNodes(collectNodesToBeRemoved(nodes), CheckRecursive::No, actions);
    }

    QString createExpression(const NodesProperty &nodesProperty)
    {
        QString expression = "[";
        const ModelNode &last = nodesProperty.targets.back();
        for (const ModelNode &node : nodesProperty.targets) {
            expression += node.id();
            if (node != last)
                expression += ", ";
        }
        expression += "]";

        return expression;
    }

    void finally()
    {

        for (const NodesProperty &nodesProperty : nodesProperties) {
            if (nodesProperty.isChanged && nodesProperty.targets.size()) {
                setExpressions().push_back({nodesProperty.source.bindingProperty(nodesProperty.name),
                                            createExpression(nodesProperty)});
            }
        }
    }

    NodeDependencies dependencies;
    NodesProperties nodesProperties;
};

struct DependenciesSet
{
    NodeDependencies nodeDependencies;
    NodeDependencies targetsDependencies;
    NodesProperties targetsNodesProperties;
    BindingDependencies bindingDependencies;
};

struct BindingFilter
{
    void filterBindingProperty(const BindingProperty &property)
    {
        const QString &expression = property.expression();
        auto iterator = wordsRegex.globalMatch(expression);

        while (iterator.hasNext()) {
            auto match = iterator.next();
            auto word = match.capturedView();
            if (auto modelNode = idModelNodeDict.value(word))
                dependencies.push_back({modelNode, property});
        }
    }

    void operator()(const NodeMetaInfo &, const ModelNode &node)
    {
        for (const BindingProperty &property : node.bindingProperties())
            filterBindingProperty(property);
    }

    void finally() { std::sort(dependencies.begin(), dependencies.end()); }

    BindingDependencies &dependencies;
    QHash<QStringView, ModelNode> idModelNodeDict;
    QRegularExpression wordsRegex{"[[:<:]](\\w+)[[:>:]]"};
};

struct TargetFilter
{
    static std::optional<ModelNode> resolveBinding(const ModelNode &node,
                                                   const PropertyName &propertyName)
    {
        auto property = node.bindingProperty(propertyName);
        if (property.exists()) {
            if (ModelNode targetNode = property.resolveToModelNode())
                return targetNode;
        }

        return {};
    }

    bool hasTargetProperty(const NodeMetaInfo &metaInfo) const
    {
        return metaInfo.isBasedOn(qtQuickPropertyChangesMetaInfo,
                                  qtQuickTimelineKeyframeGroupMetaInfo,
                                  qtQuickPropertyAnimationMetaInfo);
    }

    bool hasToOrFromProperty(const NodeMetaInfo &metaInfo)
    {
        return metaInfo.isBasedOn(flowViewFlowTransitionMetaInfo);
    }

    void operator()(const NodeMetaInfo &metaInfo, const ModelNode &node)
    {
        if (hasTargetProperty(metaInfo)) {
            if (auto targetNode = resolveBinding(node, "target"))
                dependencies.push_back({std::move(*targetNode), node});
        } else if (hasToOrFromProperty(metaInfo)) {
            if (auto toNode = resolveBinding(node, "to"))
                dependencies.push_back({std::move(*toNode), node});
            if (auto fromNode = resolveBinding(node, "from"))
                dependencies.push_back({std::move(*fromNode), node});
        }
    }

    void finally() { std::sort(dependencies.begin(), dependencies.end()); }

    NodeMetaInfo flowViewFlowTransitionMetaInfo;
    NodeMetaInfo qtQuickPropertyChangesMetaInfo;
    NodeMetaInfo qtQuickTimelineKeyframeGroupMetaInfo;
    NodeMetaInfo qtQuickPropertyAnimationMetaInfo;
    NodeDependencies &dependencies;
};

template<std::invocable<const NodeMetaInfo &> Predicate>
struct TargetsFilter
{
    TargetsFilter(Predicate predicate,
                  NodeDependencies &dependencies,
                  NodesProperties &targetsNodesProperties)
        : predicate{std::move(predicate)}
        , dependencies{dependencies}
        , targetsNodesProperties{targetsNodesProperties}
    {}

    static ModelNodes resolveTargets(const ModelNode &node)
    {
        auto targetProperty = node.bindingProperty("targets");
        if (targetProperty.exists())
            return targetProperty.resolveToModelNodeList();

        return {};
    }

    void operator()(const NodeMetaInfo &metaInfo, const ModelNode &node)
    {
        if (predicate(metaInfo)) {
            const auto targetNodes = resolveTargets(node);
            if (targetNodes.size()) {
                targetsNodesProperties.push_back({node, "targets", targetNodes});
                for (auto &&targetNode : targetNodes)
                    dependencies.push_back({targetNode, node});
            }
        }
    }

    void finally()
    {
        std::sort(dependencies.begin(), dependencies.end());
        std::sort(targetsNodesProperties.begin(), targetsNodesProperties.end());
    }

    Predicate predicate;
    NodeDependencies &dependencies;
    NodesProperties &targetsNodesProperties;
};

void addDependency(NameNodes &dependencies, const ModelNode &node, const PropertyName &propertyName)
{
    if (auto property = node.variantProperty(propertyName); property.exists()) {
        QString stateName = property.value().toString();
        if (stateName.size() && stateName != "*")
            dependencies.push_back({stateName, node});
    }
}

struct StateFilter
{
    void operator()(const NodeMetaInfo &metaInfo, const ModelNode &node)
    {
        if (metaInfo.isQtQuickState())
            addDependency(dependencies, node, "name");
    }

    void finally() { std::sort(dependencies.begin(), dependencies.end()); }

    NameNodes &dependencies;
};

struct TransitionFilter
{
    void operator()(const NodeMetaInfo &metaInfo, const ModelNode &node)
    {
        if (metaInfo.isQtQuickTransition()) {
            addDependency(transitionNodes, node, "to");
            addDependency(transitionNodes, node, "from");
        }
    }

    void finally()
    {
        std::sort(transitionNodes.begin(), transitionNodes.end());

        auto removeTransition = [&](const auto &first, const auto &second) {
            dependencies.push_back({second.node, first.node});
        };

        ::Utils::set_greedy_intersection_compare(transitionNodes.begin(),
                                                 transitionNodes.end(),
                                                 stateNodes.begin(),
                                                 stateNodes.end(),
                                                 removeTransition,
                                                 std::less<NameNode>{});
        std::sort(dependencies.begin(), dependencies.end());
    }

    NameNodes transitionNodes = {};
    NameNodes &stateNodes;
    NodeDependencies &dependencies;
};

template<typename T>
concept Filter = std::invocable<T, const NodeMetaInfo &, const ModelNode &>
                 && requires(T t) { t.finally(); };

template<Filter... FilterTypes>
void forEachFilter(std::tuple<FilterTypes...> &filters, auto &&call)
{
    std::apply([&](Filter auto &...filter) { (call(filter), ...); }, filters);
}

DependenciesSet createDependenciesSet(Model *model)
{
    const ModelNodes nodes = model->allModelNodesUnordered();

    DependenciesSet set;
    NameNodes stateNames;

    auto flowViewFlowDecisionMetaInfo = model->flowViewFlowDecisionMetaInfo();
    auto flowViewFlowWildcardMetaInfo = model->flowViewFlowWildcardMetaInfo();
    auto flowViewFlowTransitionMetaInfo = model->flowViewFlowTransitionMetaInfo();
    auto qtQuickPropertyChangesMetaInfo = model->qtQuickPropertyChangesMetaInfo();
    auto qtQuickTimelineKeyframeGroupMetaInfo = model->qtQuickTimelineKeyframeGroupMetaInfo();
    auto qtQuickPropertyAnimationMetaInfo = model->qtQuickPropertyAnimationMetaInfo();

    std::tuple filters = {
        TargetFilter{.flowViewFlowTransitionMetaInfo = flowViewFlowTransitionMetaInfo,
                     .qtQuickPropertyChangesMetaInfo = qtQuickPropertyChangesMetaInfo,
                     .qtQuickTimelineKeyframeGroupMetaInfo = qtQuickTimelineKeyframeGroupMetaInfo,
                     .qtQuickPropertyAnimationMetaInfo = qtQuickPropertyAnimationMetaInfo,
                     .dependencies = set.nodeDependencies},
        TargetsFilter{[&](const auto &metaInfo) {
                          return metaInfo.isBasedOn(flowViewFlowDecisionMetaInfo,
                                                    flowViewFlowWildcardMetaInfo,
                                                    qtQuickPropertyAnimationMetaInfo);
                      },
                      set.targetsDependencies,
                      set.targetsNodesProperties},
        StateFilter{.dependencies = stateNames},
        TransitionFilter{.stateNodes = stateNames, .dependencies = set.nodeDependencies},
        BindingFilter{.dependencies = set.bindingDependencies,
                      .idModelNodeDict = model->idModelNodeDict()}};

    for (const ModelNode &node : nodes) {
        auto metaInfo = node.metaInfo();
        forEachFilter(filters, [&](Filter auto &&filter) { filter(metaInfo, node); });
    }

    forEachFilter(filters, [](Filter auto &&filter) { filter.finally(); });

    return set;
}

template<NodeAction... Actions>
class Dispatcher
{
public:
    template<NodeAction... ActionTypes>
    Dispatcher(ActionTypes &&...actions)
        : actions{std::forward<ActionTypes>(actions)...}
    {
        forEachAction(this->actions, [&](NodeAction auto &action) {
            action.setModelResourceSet(modelResourceSet);
        });
    }

    Dispatcher(const Dispatcher &) = delete;
    Dispatcher &opertor(const Dispatcher &) = delete;
    Dispatcher(Dispatcher &&) = delete;
    Dispatcher &opertor(Dispatcher &&) = delete;

    void removeProperties(const AbstractProperties &properties, CheckRecursive checkRecursive)
    {
        Base base;
        base.setModelResourceSet(modelResourceSet);

        base.removeProperties(properties, checkRecursive, actions);
    }

    void removeNodes(const ModelNodes &nodes, CheckRecursive checkRecursive)
    {
        Base base;
        base.setModelResourceSet(modelResourceSet);

        base.removeNodes(nodes, checkRecursive, actions);
    }

    ModelResourceSet &takeModelResourceSet()
    {
        forEachAction(actions, [&](NodeAction auto &action) { action.finally(); });

        return modelResourceSet;
    }

private:
    std::tuple<Actions...> actions;
    ModelResourceSet modelResourceSet;
};

template<NodeAction... Actions>
Dispatcher(Actions &&...actions) -> Dispatcher<Actions...>;

auto createDispatcher(Model *model)
{
    DependenciesSet set = createDependenciesSet(model);

    return Dispatcher{CheckChildNodes{},
                      CheckNodesInNodeAbstractProperties{},
                      RemoveLayerEnabled{},
                      RemoveDependentBindings{.dependencies = std::move(set.bindingDependencies)},
                      RemoveDependencies{.dependencies = std::move(set.nodeDependencies)},
                      RemoveTargetsSources{.dependencies = std::move(set.targetsDependencies),
                                           .nodesProperties = std::move(set.targetsNodesProperties)}};
}

} // namespace

ModelResourceSet ModelResourceManagement::removeNodes(ModelNodes nodes, Model *model) const
{
    std::sort(nodes.begin(), nodes.end());

    Dispatcher dispatcher = createDispatcher(model);

    dispatcher.removeNodes(nodes, CheckRecursive::Yes);

    return dispatcher.takeModelResourceSet();
}

ModelResourceSet ModelResourceManagement::removeProperties(AbstractProperties properties,
                                                           Model *model) const
{
    std::sort(properties.begin(), properties.end());

    Dispatcher dispatcher = createDispatcher(model);

    dispatcher.removeProperties(properties, CheckRecursive::Yes);

    return dispatcher.takeModelResourceSet();
}

} // namespace QmlDesigner
