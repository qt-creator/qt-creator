// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "stateseditorview.h"
#include "propertychangesmodel.h"
#include "stateseditormodel.h"
#include "stateseditorwidget.h"

#include <QDebug>
#include <QRegularExpression>
#include <QMessageBox>
#include <cmath>
#include <memory>

#include <nodemetainfo.h>

#include <bindingproperty.h>
#include <customnotifications.h>
#include <nodelistproperty.h>
#include <rewritingexception.h>
#include <variantproperty.h>

#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qmlitemnode.h>
#include <qmlstate.h>
#include <annotationeditor/annotationeditor.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QMessageBox>
#include <QRegularExpression>
#include <QScopeGuard>

#include <cmath>
#include <memory>

namespace QmlDesigner {

/**
  We always have 'one' current state, where we get updates from (see sceneChanged()). In case
  the current state is the base state, we render the base state + all other states.
  */

StatesEditorView::StatesEditorView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView(externalDependencies)
    , m_statesEditorModel(new StatesEditorModel(this))
    , m_lastIndex(-1)
    , m_editor(nullptr)
{
    Q_ASSERT(m_statesEditorModel);
    // base state
}

StatesEditorView::~StatesEditorView()
{
    if (m_editor)
        delete m_editor;
    delete m_statesEditorWidget.data();
}

WidgetInfo StatesEditorView::widgetInfo()
{
    if (!m_statesEditorWidget)
        m_statesEditorWidget = new StatesEditorWidget(this, m_statesEditorModel.data());

    return createWidgetInfo(m_statesEditorWidget.data(),
                            "StatesEditor",
                            WidgetInfo::BottomPane,
                            0,
                            tr("States"));
}

void StatesEditorView::rootNodeTypeChanged(const QString &/*type*/, int /*majorVersion*/, int /*minorVersion*/)
{
    checkForStatesAvailability();
}

ModelNode StatesEditorView::activeStatesGroupNode() const
{
    return m_activeStatesGroupNode;
}

void StatesEditorView::setActiveStatesGroupNode(const ModelNode &modelNode)
{
    if (m_activeStatesGroupNode == modelNode)
        return;

    m_activeStatesGroupNode = modelNode;
    resetModel();

    checkForStatesAvailability();

    emit m_statesEditorModel->activeStateGroupChanged();
    emit m_statesEditorModel->activeStateGroupIndexChanged();
}

int StatesEditorView::activeStatesGroupIndex() const
{
    if (!model())
        return -1;

    return Utils::indexOf(allModelNodesOfType(model()->qtQuickStateGroupMetaInfo()),
                          [this](const ModelNode &node) { return node == m_activeStatesGroupNode; })
           + 1;
}

void StatesEditorView::setActiveStatesGroupIndex(int index)
{
    if (!model())
        return;

    if (index > 0) {
        const ModelNode statesGroup = allModelNodesOfType(model()->qtQuickStateGroupMetaInfo())
                                          .at(index - 1);
        if (statesGroup.isValid())
            setActiveStatesGroupNode(statesGroup);
    } else {
        setActiveStatesGroupNode(rootModelNode());
    }
}

void StatesEditorView::registerPropertyChangesModel(PropertyChangesModel *model)
{
    m_propertyChangedModels.insert(model);
}

void StatesEditorView::deregisterPropertyChangesModel(PropertyChangesModel *model)
{
    m_propertyChangedModels.remove(model);
}

void StatesEditorView::synchonizeCurrentStateFromWidget()
{
    if (!model())
        return;

    if (m_block)
        return;

    int internalId = m_statesEditorWidget->currentStateInternalId();

    if (internalId > 0 && hasModelNodeForInternalId(internalId)) {
        ModelNode node = modelNodeForInternalId(internalId);
        QmlModelState modelState(node);
        if (modelState.isValid() && modelState != currentState())
            setCurrentState(modelState);
    } else {
        setCurrentState(baseState());
    }
}

void StatesEditorView::createNewState()
{
    // can happen when root node is e.g. a ListModel
    if (!QmlVisualNode::isValidQmlVisualNode(activeStatesGroupNode())
        && m_activeStatesGroupNode.type() != "QtQuick.StateGroup")
        return;

    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_STATE_ADDED);

    QStringList modelStateNames = activeStateGroup().names();

    QString newStateName;
    int index = 1;
    while (true) {
        newStateName = QString(QStringLiteral("State%1")).arg(index++);
        if (!modelStateNames.contains(newStateName))
            break;
    }

    executeInTransaction("createNewState", [this, newStateName]() {
        activeStatesGroupNode().validId();

        ModelNode newState = activeStateGroup().addState(newStateName);
        setCurrentState(newState);
    });
}

void StatesEditorView::cloneState(int nodeId)
{
    if (!(nodeId > 0 && hasModelNodeForInternalId(nodeId)))
        return;

    ModelNode stateNode(modelNodeForInternalId(nodeId));
    QTC_ASSERT(stateNode.simplifiedTypeName() == "State", return );

    QmlModelState modelState(stateNode);
    if (!modelState.isValid() || modelState.isBaseState())
        return;

    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_STATE_CLONED);

    QString newName = modelState.name();

    // Strip out numbers at the end of the string
    QRegularExpression regEx(QLatin1String("[0-9]+$"));
    const QRegularExpressionMatch match = regEx.match(newName);
    if (match.hasMatch() && (match.capturedStart() + match.capturedLength() == newName.length()))
        newName = newName.left(match.capturedStart());

    int i = 1;
    QStringList stateNames = activeStateGroup().names();
    while (stateNames.contains(newName + QString::number(i)))
        i++;
    const QString newStateName = newName + QString::number(i);

    QmlModelState newState;

    executeInTransaction("cloneState", [newStateName, modelState, &newState]() {
        newState = modelState.duplicate(newStateName);
    });

    ModelNode newNode = newState.modelNode();
    int from = newNode.parentProperty().indexOf(newNode);
    int to = stateNode.parentProperty().indexOf(stateNode) + 1;

    // When duplicating an extended state the new state needs to be added after the extend group.
    if (!modelState.hasExtend()) {
        auto modelNodeList = activeStatesGroupNode().nodeListProperty("states").toModelNodeList();
        for (; to != modelNodeList.count(); ++to) {
            QmlModelState currentState(modelNodeList.at(to));
            if (!currentState.isValid() || currentState.isBaseState() || !currentState.hasExtend())
                break;
        }
    }

    executeInTransaction("moveState", [this, &newState, from, to]() {
        activeStatesGroupNode().nodeListProperty("states").slide(from, to);
        setCurrentState(newState);
    });
}

void StatesEditorView::extendState(int nodeId)
{
    if (!(nodeId > 0 && hasModelNodeForInternalId(nodeId)))
        return;

    ModelNode stateNode(modelNodeForInternalId(nodeId));
    QTC_ASSERT(stateNode.simplifiedTypeName() == "State", return );

    QmlModelState modelState(stateNode);
    if (!modelState.isValid() || modelState.isBaseState())
        return;

    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_STATE_EXTENDED);

    QString newName = modelState.name();

    // Strip out numbers at the end of the string
    QRegularExpression regEx(QLatin1String("[0-9]+$"));
    const QRegularExpressionMatch match = regEx.match(newName);
    if (match.hasMatch() && (match.capturedStart() + match.capturedLength() == newName.length()))
        newName = newName.left(match.capturedStart());

    int i = 1;
    QStringList stateNames = activeStateGroup().names();
    while (stateNames.contains(newName + QString::number(i)))
        i++;
    const QString newStateName = newName + QString::number(i);

    QmlModelState newState;

    executeInTransaction("extendState", [this, newStateName, modelState, &newState]() {
        newState = activeStateGroup().addState(newStateName);
        newState.setExtend(modelState.name());
    });

    ModelNode newNode = newState.modelNode();
    int from = newNode.parentProperty().indexOf(newNode);
    int to = stateNode.parentProperty().indexOf(stateNode) + 1;

    executeInTransaction("moveState", [this, &newState, from, to]() {
        activeStatesGroupNode().nodeListProperty("states").slide(from, to);
        setCurrentState(newState);
    });
}

void StatesEditorView::removeState(int nodeId)
{
    try {
        if (nodeId > 0 && hasModelNodeForInternalId(nodeId)) {
            ModelNode stateNode(modelNodeForInternalId(nodeId));
            QTC_ASSERT(stateNode.simplifiedTypeName() == "State", return );

            QmlModelState modelState(stateNode);
            if (modelState.isValid()) {
                QStringList lockedTargets;
                const auto propertyChanges = modelState.propertyChanges();

                // confirm removing not empty states
                if (!propertyChanges.isEmpty()) {
                    QMessageBox msgBox;
                    msgBox.setTextFormat(Qt::RichText);
                    msgBox.setIcon(QMessageBox::Question);
                    msgBox.setWindowTitle(tr("Remove State"));
                    msgBox.setText(
                        tr("This state is not empty. Are you sure you want to remove it?"));
                    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
                    msgBox.setDefaultButton(QMessageBox::Yes);

                    if (msgBox.exec() == QMessageBox::Cancel)
                        return;
                }

                // confirm removing states with locked targets
                for (const QmlPropertyChanges &change : propertyChanges) {
                    const ModelNode target = change.target();
                    QTC_ASSERT(target.isValid(), continue);
                    if (target.locked())
                        lockedTargets.push_back(target.id());
                }

                if (!lockedTargets.empty()) {
                    Utils::sort(lockedTargets);
                    QString detailedText = QString("<b>" + tr("Locked components:") + "</b><br>");

                    for (const auto &id : std::as_const(lockedTargets))
                        detailedText.append("- " + id + "<br>");

                    detailedText.chop(QString("<br>").size());

                    QMessageBox msgBox;
                    msgBox.setTextFormat(Qt::RichText);
                    msgBox.setIcon(QMessageBox::Question);
                    msgBox.setWindowTitle(tr("Remove State"));
                    msgBox.setText(QString(tr("Removing this state will modify locked components.")
                                           + "<br><br>%1")
                                       .arg(detailedText));
                    msgBox.setInformativeText(tr("Continue by removing the state?"));
                    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
                    msgBox.setDefaultButton(QMessageBox::Ok);

                    if (msgBox.exec() == QMessageBox::Cancel)
                        return;
                }
            }

            NodeListProperty parentProperty = stateNode.parentProperty().toNodeListProperty();

            if (modelState.isDefault())
                m_statesEditorModel->resetDefaultState();

            if (parentProperty.count() <= 1) {
                setCurrentState(baseState());
            } else if (parentProperty.isValid()) {
                int index = parentProperty.indexOf(stateNode);
                if (index == 0)
                    setCurrentState(parentProperty.at(1));
                else
                    setCurrentState(parentProperty.at(index - 1));
            }

            stateNode.destroy();
        }
    } catch (const RewritingException &e) {
        e.showException();
    }
}

void StatesEditorView::resetModel()
{
    if (m_bulkChange) {
        m_modelDirty = true;
        return;
    }

    if (m_statesEditorModel)
        m_statesEditorModel->reset();

    if (m_statesEditorWidget) {
        if (currentState().isBaseState())
            m_statesEditorWidget->setCurrentStateInternalId(0);
        else
            m_statesEditorWidget->setCurrentStateInternalId(currentState().modelNode().internalId());
    }

    m_modelDirty = false;
}

void StatesEditorView::resetPropertyChangesModels()
{
    if (m_bulkChange) {
        m_propertyChangesDirty = true;
        return;
    }

    std::for_each(m_propertyChangedModels.begin(),
                  m_propertyChangedModels.end(),
                  [](PropertyChangesModel *model) { model->reset(); });

    m_propertyChangesDirty = false;
}

void StatesEditorView::resetExtend()
{
    if (m_bulkChange) {
        m_extendDirty = true;
        return;
    }

    m_statesEditorModel->evaluateExtend();

    m_extendDirty = false;
}

void StatesEditorView::resetStateGroups()
{
    if (m_bulkChange) {
        m_stateGroupsDirty = true;
        return;
    }

    emit m_statesEditorModel->stateGroupsChanged();

    m_stateGroupsDirty = false;
}

void StatesEditorView::checkForStatesAvailability()
{
    if (m_statesEditorWidget) {
        const bool isVisual = activeStatesGroupNode().metaInfo().isBasedOn(
            model()->qtQuickItemMetaInfo(), model()->qtQuick3DNodeMetaInfo());
        const bool isRoot = activeStatesGroupNode().isRootNode();
        m_statesEditorModel->setCanAddNewStates(isVisual || !isRoot);
    }
}

void StatesEditorView::beginBulkChange()
{
    m_bulkChange = true;
}

void StatesEditorView::endBulkChange()
{
    if (!m_bulkChange)
        return;

    m_bulkChange = false;

    if (m_modelDirty)
        resetModel();

    if (m_propertyChangesDirty)
        resetPropertyChangesModels();

    if (m_extendDirty)
        resetExtend();

    if (m_stateGroupsDirty)
        resetStateGroups();
}

void StatesEditorView::setCurrentState(const QmlModelState &state)
{
    if (!model() && !state.isValid())
        return;

    if (currentStateNode() != state.modelNode())
        setCurrentStateNode(state.modelNode());
}

QmlModelState StatesEditorView::baseState() const
{
    return QmlModelState::createBaseState(this);
}

QmlModelStateGroup StatesEditorView::activeStateGroup() const
{
    return QmlModelStateGroup(activeStatesGroupNode());
}

bool StatesEditorView::validStateName(const QString &name) const
{
    if (name == tr("base state"))
        return false;
    const QList<QmlModelState> modelStates = activeStateGroup().allStates();
    for (const QmlModelState &state : modelStates) {
        if (state.name() == name)
            return false;
    }
    return true;
}

bool StatesEditorView::hasExtend() const
{
    if (!model())
        return false;

    const QList<QmlModelState> modelStates = activeStateGroup().allStates();
    for (const QmlModelState &state : modelStates) {
        if (state.hasExtend())
            return true;
    }
    return false;
}

QStringList StatesEditorView::extendedStates() const
{
    if (!model())
        return {};

    QStringList states;

    const QList<QmlModelState> modelStates = activeStateGroup().allStates();
    for (const QmlModelState &state : modelStates) {
        if (state.hasExtend())
            states.append(state.extend());
    }
    states.removeDuplicates();
    return states;
}

QString StatesEditorView::currentStateName() const
{
    return currentState().isValid() ? currentState().name() : QString();
}

void StatesEditorView::renameState(int internalNodeId, const QString &newName)
{
    if (hasModelNodeForInternalId(internalNodeId)) {
        QmlModelState renamedState(modelNodeForInternalId(internalNodeId));
        try {
            if (renamedState.isValid() && renamedState.name() != newName) {
                executeInTransaction("renameState", [this, &renamedState, &newName]() {
                    // Jump to base state for the change
                    QmlModelState oldState = currentState();
                    setCurrentState(baseState());
                    const bool updateDefault = renamedState.isDefault();

                    // If state is extended rename all references
                    QList<QmlModelState> states;
                    const QList<QmlModelState> modelStates = activeStateGroup().allStates();
                    for (const QmlModelState &state : modelStates) {
                        if (state.hasExtend() && state.extend() == renamedState.name())
                            states.append(state);
                    }

                    renamedState.setName(newName.trimmed());

                    for (QmlModelState &state : states)
                        state.setExtend(newName.trimmed());

                    if (updateDefault)
                        renamedState.setAsDefault();

                    setCurrentState(oldState);
                });
            }
        } catch (const RewritingException &e) {
            e.showException();
        }
    }
}

void StatesEditorView::setWhenCondition(int internalNodeId, const QString &condition)
{
    if (m_block)
        return;

    m_block = true;
    const QScopeGuard cleanup([&] { m_block = false; });

    if (hasModelNodeForInternalId(internalNodeId)) {
        QmlModelState state(modelNodeForInternalId(internalNodeId));
        try {
            if (state.isValid())
                state.modelNode().bindingProperty("when").setExpression(condition);

        } catch (const Exception &e) {
            e.showException();
        }
    }
}

void StatesEditorView::resetWhenCondition(int internalNodeId)
{
    if (m_block)
        return;

    m_block = true;
    const QScopeGuard cleanup([&] { m_block = false; });

    if (hasModelNodeForInternalId(internalNodeId)) {
        QmlModelState state(modelNodeForInternalId(internalNodeId));
        try {
            if (state.isValid() && state.modelNode().hasProperty("when"))
                state.modelNode().removeProperty("when");

        } catch (const RewritingException &e) {
            e.showException();
        }
    }
}

void StatesEditorView::setStateAsDefault(int internalNodeId)
{
    if (m_block)
        return;

    m_block = true;
    const QScopeGuard cleanup([&] { m_block = false; });

    if (hasModelNodeForInternalId(internalNodeId)) {
        QmlModelState state(modelNodeForInternalId(internalNodeId));
        try {
            if (state.isValid())
                state.setAsDefault();

        } catch (const RewritingException &e) {
            e.showException();
        }
    }
}

void StatesEditorView::resetDefaultState()
{
    if (m_block)
        return;

    m_block = true;
    const QScopeGuard cleanup([&] { m_block = false; });

    try {
        if (activeStatesGroupNode().hasProperty("state"))
            activeStatesGroupNode().removeProperty("state");

    } catch (const RewritingException &e) {
        e.showException();
    }
}

bool StatesEditorView::hasDefaultState() const
{
    return activeStatesGroupNode().hasProperty("state");
}

void StatesEditorView::setAnnotation(int internalNodeId)
{
    if (m_block)
        return;

    m_block = true;
    const QScopeGuard cleanup([&] { m_block = false; });

    if (hasModelNodeForInternalId(internalNodeId)) {
        QmlModelState state(modelNodeForInternalId(internalNodeId));
        try {
            if (state.isValid()) {
                ModelNode modelNode = state.modelNode();

                if (modelNode.isValid()) {
                    if (!m_editor)
                        m_editor = new AnnotationEditor(this);

                    m_editor->setModelNode(modelNode);
                    m_editor->showWidget();
                }
            }

        } catch (const RewritingException &e) {
            e.showException();
        }
    }
}

void StatesEditorView::removeAnnotation(int internalNodeId)
{
    if (m_block)
        return;

    m_block = true;
    const QScopeGuard cleanup([&] { m_block = false; });

    if (hasModelNodeForInternalId(internalNodeId)) {
        QmlModelState state(modelNodeForInternalId(internalNodeId));
        try {
            if (state.isValid())
                state.removeAnnotation();

        } catch (const RewritingException &e) {
            e.showException();
        }
    }
}

bool StatesEditorView::hasAnnotation(int internalNodeId) const
{
    if (!model())
        return false;

    if (hasModelNodeForInternalId(internalNodeId)) {
        QmlModelState state(modelNodeForInternalId(internalNodeId));
        if (state.isValid())
            return state.hasAnnotation();
    }

    return false;
}

void StatesEditorView::modelAttached(Model *model)
{
    if (model == AbstractView::model())
        return;

    QTC_ASSERT(model, return );
    AbstractView::modelAttached(model);

    m_activeStatesGroupNode = rootModelNode();

    if (m_statesEditorWidget)
        m_statesEditorWidget->setNodeInstanceView(nodeInstanceView());

    checkForStatesAvailability();

    resetModel();
    resetStateGroups();

    emit m_statesEditorModel->activeStateGroupChanged();
    emit m_statesEditorModel->activeStateGroupIndexChanged();
}

void StatesEditorView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
    resetModel();
}

void StatesEditorView::propertiesRemoved(const QList<AbstractProperty> &propertyList)
{
    for (const AbstractProperty &property : propertyList) {
        if (property.name() == "states" && property.parentModelNode() == activeStateGroup().modelNode())
            resetModel();
        if ((property.name() == "when" || property.name() == "name")
            && QmlModelState::isValidQmlModelState(property.parentModelNode()))
            resetModel();
        if (property.name() == "extend")
            resetExtend();
        if (property.parentModelNode().simplifiedTypeName() == "PropertyChanges"
            || (property.name() == "changes"
                && property.parentModelNode().simplifiedTypeName() == "State"))
            resetPropertyChangesModels();
    }
}

void StatesEditorView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    if (removedNode.hasParentProperty()) {
        const NodeAbstractProperty propertyParent = removedNode.parentProperty();
        if (propertyParent.parentModelNode() == activeStateGroup().modelNode()
            && propertyParent.name() == "states")
            m_lastIndex = propertyParent.indexOf(removedNode);
    }
    if (currentState().isValid() && removedNode == currentState())
        setCurrentState(baseState());

    if (removedNode.simplifiedTypeName() == "PropertyChanges")
        m_propertyChangesRemoved = true;

    if (removedNode.simplifiedTypeName() == "StateGroup") {
        if (removedNode == activeStatesGroupNode())
            setActiveStatesGroupNode(rootModelNode());

        m_stateGroupRemoved = true;
    }
}

void StatesEditorView::nodeRemoved(const ModelNode & /*removedNode*/,
                                   const NodeAbstractProperty &parentProperty,
                                   PropertyChangeFlags /*propertyChange*/)
{
    if (parentProperty.isValid()
        && parentProperty.parentModelNode() == activeStateGroup().modelNode()
        && parentProperty.name() == "states") {
        m_statesEditorModel->removeState(m_lastIndex);
        m_lastIndex = -1;
        resetModel();
    }

    if (m_propertyChangesRemoved) {
        m_propertyChangesRemoved = false;
        resetPropertyChangesModels();
    }

    if (m_stateGroupRemoved) {
        m_stateGroupRemoved = false;
        resetStateGroups();
    }
}

void StatesEditorView::nodeAboutToBeReparented(const ModelNode &node,
                                               const NodeAbstractProperty & /*newPropertyParent*/,
                                               const NodeAbstractProperty &oldPropertyParent,
                                               AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (oldPropertyParent.isValid()
        && oldPropertyParent.parentModelNode() == activeStateGroup().modelNode()
        && oldPropertyParent.name() == "states")
        m_lastIndex = oldPropertyParent.indexOf(node);

    if (node.simplifiedTypeName() == "StateGroup")
        resetStateGroups();
}

void StatesEditorView::nodeReparented(const ModelNode &node,
                                      const NodeAbstractProperty &newPropertyParent,
                                      const NodeAbstractProperty &oldPropertyParent,
                                      AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (oldPropertyParent.isValid()
        && oldPropertyParent.parentModelNode() == activeStateGroup().modelNode()
        && oldPropertyParent.name() == "states") {
        m_statesEditorModel->removeState(m_lastIndex);
        resetModel();
        m_lastIndex = -1;
    }

    if (newPropertyParent.isValid()
        && newPropertyParent.parentModelNode() == activeStateGroup().modelNode()
        && newPropertyParent.name() == "states") {
        int index = newPropertyParent.indexOf(node);
        m_statesEditorModel->insertState(index);
    }

    if (node.simplifiedTypeName() == "PropertyChanges")
        resetPropertyChangesModels();
}

void StatesEditorView::nodeOrderChanged(const NodeListProperty &listProperty)
{
    if (m_block)
        return;

    if (listProperty.isValid() && listProperty.parentModelNode() == activeStateGroup().modelNode()
        && listProperty.name() == "states")
        resetModel();
}

void StatesEditorView::bindingPropertiesChanged(const QList<BindingProperty> &propertyList, AbstractView::PropertyChangeFlags propertyChange)
{
    Q_UNUSED(propertyChange)

    for (const BindingProperty &property : propertyList) {
        if (property.name() == "when"
            && QmlModelState::isValidQmlModelState(property.parentModelNode()))
            resetModel();
        if (property.parentModelNode().simplifiedTypeName() == "PropertyChanges")
            resetPropertyChangesModels();
    }
}

void StatesEditorView::variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                                AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (m_block)
        return;

    m_block = true;
    const QScopeGuard cleanup([&] { m_block = false; });

    for (const VariantProperty &property : propertyList) {
        if (property.name() == "name"
            && QmlModelState::isValidQmlModelState(property.parentModelNode()))
            resetModel();
        else if (property.name() == "state"
                 && property.parentModelNode() == activeStateGroup().modelNode())
            resetModel();
        else if (property.name() == "extend")
            resetExtend();

        if (property.parentModelNode().simplifiedTypeName() == "PropertyChanges")
            resetPropertyChangesModels();
    }
}

void StatesEditorView::customNotification(const AbstractView * /*view*/,
                                          const QString &identifier,
                                          const QList<ModelNode> & /*nodeList*/,
                                          const QList<QVariant> & /*data*/)
{
    if (identifier == StartRewriterAmend)
        beginBulkChange();

    if (identifier == EndRewriterAmend)
        endBulkChange();
}

void StatesEditorView::rewriterBeginTransaction()
{
    beginBulkChange();
}

void StatesEditorView::rewriterEndTransaction()
{
    endBulkChange();
}

void StatesEditorView::currentStateChanged(const ModelNode &node)
{
    QmlModelState newQmlModelState(node);

    if (newQmlModelState.isBaseState())
        m_statesEditorWidget->setCurrentStateInternalId(0);
    else
        m_statesEditorWidget->setCurrentStateInternalId(newQmlModelState.modelNode().internalId());
}

void StatesEditorView::instancesPreviewImageChanged(const QVector<ModelNode> &nodeList)
{
    if (!model())
        return;

    int minimumIndex = 10000;
    int maximumIndex = -1;
    for (const ModelNode &node : nodeList) {
        if (node.isRootNode()) {
            minimumIndex = qMin(minimumIndex, 0);
            maximumIndex = qMax(maximumIndex, 0);
        } else {
            int index = activeStateGroup().allStates().indexOf(QmlModelState(node)) + 1;
            if (index > 0) {
                minimumIndex = qMin(minimumIndex, index);
                maximumIndex = qMax(maximumIndex, index);
            }
        }
    }

    if (maximumIndex >= 0)
        m_statesEditorModel->updateState(minimumIndex, maximumIndex);
}

void StatesEditorView::moveStates(int from, int to)
{
    if (m_block)
        return;

    m_block = true;
    auto guard = qScopeGuard([&]() { m_block = false; });

    if (!activeStatesGroupNode().hasNodeListProperty("states"))
        return;

    executeInTransaction("moveState", [this, from, to]() {
        activeStatesGroupNode().nodeListProperty("states").slide(from - 1, to - 1);
    });
}

} // namespace QmlDesigner
