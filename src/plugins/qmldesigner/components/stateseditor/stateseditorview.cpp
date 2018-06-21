/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "stateseditorview.h"
#include "stateseditorwidget.h"
#include "stateseditormodel.h"
#include <rewritingexception.h>

#include <QDebug>
#include <QRegExp>
#include <math.h>

#include <nodemetainfo.h>

#include <bindingproperty.h>
#include <variantproperty.h>
#include <nodelistproperty.h>

#include <qmlitemnode.h>
#include <qmlstate.h>


namespace QmlDesigner {

/**
  We always have 'one' current state, where we get updates from (see sceneChanged()). In case
  the current state is the base state, we render the base state + all other states.
  */
StatesEditorView::StatesEditorView(QObject *parent) :
        AbstractView(parent),
        m_statesEditorModel(new StatesEditorModel(this)),
        m_lastIndex(-1)
{
    Q_ASSERT(m_statesEditorModel);
    // base state
}

StatesEditorView::~StatesEditorView()
{
    delete m_statesEditorWidget.data();
}

WidgetInfo StatesEditorView::widgetInfo()
{
    if (!m_statesEditorWidget)
        m_statesEditorWidget = new StatesEditorWidget(this, m_statesEditorModel.data());

    return createWidgetInfo(m_statesEditorWidget.data(), 0, QLatin1String("StatesEditor"), WidgetInfo::BottomPane, 0, tr("States"));
}

void StatesEditorView::rootNodeTypeChanged(const QString &/*type*/, int /*majorVersion*/, int /*minorVersion*/)
{
    checkForWindow();
}

void StatesEditorView::toggleStatesViewExpanded()
{
    if (m_statesEditorWidget)
        m_statesEditorWidget->toggleStatesViewExpanded();
}

void StatesEditorView::removeState(int nodeId)
{
    try {
        if (nodeId > 0 && hasModelNodeForInternalId(nodeId)) {
            ModelNode stateNode(modelNodeForInternalId(nodeId));
            Q_ASSERT(stateNode.metaInfo().isSubclassOf("QtQuick.State"));
            NodeListProperty parentProperty = stateNode.parentProperty().toNodeListProperty();

            if (parentProperty.count() <= 1) {
                setCurrentState(baseState());
            } else if (parentProperty.isValid()){
                int index = parentProperty.indexOf(stateNode);
                if (index == 0)
                    setCurrentState(parentProperty.at(1));
                else
                    setCurrentState(parentProperty.at(index - 1));
            }


            stateNode.destroy();
        }
    }  catch (const RewritingException &e) {
        e.showException();
    }
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
    if (currentState().isBaseState())
        addState();
    else
        duplicateCurrentState();
}

void StatesEditorView::addState()
{
    // can happen when root node is e.g. a ListModel
    if (!QmlItemNode::isValidQmlItemNode(rootModelNode()))
        return;

    QStringList modelStateNames = rootStateGroup().names();

    QString newStateName;
    int index = 1;
    while (true) {
        newStateName = QString(QStringLiteral("State%1")).arg(index++);
        if (!modelStateNames.contains(newStateName))
            break;
    }

    try {
        if ((rootStateGroup().allStates().count() < 1) && //QtQuick import might be missing
                (!model()->hasImport(Import::createLibraryImport("QtQuick", "1.0"), true, true))) {
            model()->changeImports({Import::createLibraryImport("QtQuick", "1.0")}, {});
        }
        ModelNode newState = rootStateGroup().addState(newStateName);
        setCurrentState(newState);
    } catch (const RewritingException &e) {
        e.showException();
    }
}

void StatesEditorView::resetModel()
{
    if (m_statesEditorModel)
        m_statesEditorModel->reset();

    if (m_statesEditorWidget) {
        if (currentState().isBaseState())
            m_statesEditorWidget->setCurrentStateInternalId(currentState().modelNode().internalId());
        else
            m_statesEditorWidget->setCurrentStateInternalId(0);
    }
}

void StatesEditorView::duplicateCurrentState()
{
    QmlModelState state = currentState();

    Q_ASSERT(!state.isBaseState());

    QString newName = state.name();

    // Strip out numbers at the end of the string
    QRegExp regEx(QLatin1String("[0-9]+$"));
    int numberIndex = newName.indexOf(regEx);
    if ((numberIndex != -1) && (numberIndex+regEx.matchedLength()==newName.length()))
        newName = newName.left(numberIndex);

    int i = 1;
    QStringList stateNames = rootStateGroup().names();
    while (stateNames.contains(newName + QString::number(i)))
        i++;

    QmlModelState newState = state.duplicate(newName + QString::number(i));
    setCurrentState(newState);
}

void StatesEditorView::checkForWindow()
{
    if (m_statesEditorWidget)
        m_statesEditorWidget->showAddNewStatesButton(!rootModelNode().metaInfo().isSubclassOf("QtQuick.Window.Window")
                                                     && !rootModelNode().metaInfo().isSubclassOf("QtQuick.Window.Popup"));
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

QmlModelStateGroup StatesEditorView::rootStateGroup() const
{
    return QmlModelStateGroup(rootModelNode());
}

bool StatesEditorView::validStateName(const QString &name) const
{
    if (name == tr("base state"))
        return false;
    QList<QmlModelState> modelStates = rootStateGroup().allStates();
    foreach (const QmlModelState &state, modelStates) {
        if (state.name() == name)
            return false;
    }
    return true;
}

QString StatesEditorView::currentStateName() const
{
    return currentState().isValid() ? currentState().name() : QString();
}

void StatesEditorView::renameState(int internalNodeId, const QString &newName)
{
    if (hasModelNodeForInternalId(internalNodeId)) {
        QmlModelState state(modelNodeForInternalId(internalNodeId));
        try {
            if (state.isValid() && state.name() != newName) {
                // Jump to base state for the change
                QmlModelState oldState = currentState();
                setCurrentState(baseState());
                state.setName(newName);
                setCurrentState(oldState);
            }
        }  catch (const RewritingException &e) {
            e.showException();
        }
    }
}

void StatesEditorView::setWhenCondition(int internalNodeId, const QString &condition)
{
    if (m_block)
        return;

    m_block = true;

    if (hasModelNodeForInternalId(internalNodeId)) {
        QmlModelState state(modelNodeForInternalId(internalNodeId));
        try {
            if (state.isValid())
                state.modelNode().bindingProperty("when").setExpression(condition);

        } catch (const RewritingException &e) {
            e.showException();
        }
    }

    m_block = false;
}

void StatesEditorView::resetWhenCondition(int internalNodeId)
{
    if (m_block)
        return;

    m_block = true;

    if (hasModelNodeForInternalId(internalNodeId)) {
        QmlModelState state(modelNodeForInternalId(internalNodeId));
        try {
            if (state.isValid() && state.modelNode().hasProperty("when"))
                state.modelNode().removeProperty("when");

        } catch (const RewritingException &e) {
            e.showException();
        }
    }

    m_block = false;
}

void StatesEditorView::modelAttached(Model *model)
{
    if (model == AbstractView::model())
        return;

    Q_ASSERT(model);
    AbstractView::modelAttached(model);

    if (m_statesEditorWidget)
        m_statesEditorWidget->setNodeInstanceView(nodeInstanceView());

    checkForWindow();

    resetModel();
}

void StatesEditorView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
    resetModel();
}

void StatesEditorView::propertiesRemoved(const QList<AbstractProperty>& propertyList)
{
    foreach (const AbstractProperty &property, propertyList) {
        if (property.name() == "states" && property.parentModelNode().isRootNode())
            resetModel();
        if (property.name() == "when" && QmlModelState::isValidQmlModelState(property.parentModelNode()))
            resetModel();
    }
}

void StatesEditorView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    if (removedNode.hasParentProperty()) {
        const NodeAbstractProperty propertyParent = removedNode.parentProperty();
        if (propertyParent.parentModelNode().isRootNode() && propertyParent.name() == "states")
            m_lastIndex = propertyParent.indexOf(removedNode);
    }
    if (currentState().isValid() && removedNode == currentState())
        setCurrentState(baseState());
}

void StatesEditorView::nodeRemoved(const ModelNode & /*removedNode*/, const NodeAbstractProperty &parentProperty, PropertyChangeFlags /*propertyChange*/)
{
    if (parentProperty.isValid() && parentProperty.parentModelNode().isRootNode() && parentProperty.name() == "states") {
        m_statesEditorModel->removeState(m_lastIndex);
        m_lastIndex = -1;
    }
}

void StatesEditorView::nodeAboutToBeReparented(const ModelNode &node, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (oldPropertyParent.isValid() && oldPropertyParent.parentModelNode().isRootNode() && oldPropertyParent.name() == "states")
        m_lastIndex = oldPropertyParent.indexOf(node);
}


void StatesEditorView::nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (oldPropertyParent.isValid() && oldPropertyParent.parentModelNode().isRootNode() && oldPropertyParent.name() == "states")
        m_statesEditorModel->removeState(m_lastIndex);

    m_lastIndex = -1;

    if (newPropertyParent.isValid() && newPropertyParent.parentModelNode().isRootNode() && newPropertyParent.name() == "states") {
        int index = newPropertyParent.indexOf(node);
        m_statesEditorModel->insertState(index);
    }
}

void StatesEditorView::nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode & /*movedNode*/, int /*oldIndex*/)
{
    if (listProperty.isValid() && listProperty.parentModelNode().isRootNode() && listProperty.name() == "states")
        resetModel();
}

void StatesEditorView::bindingPropertiesChanged(const QList<BindingProperty> &propertyList, AbstractView::PropertyChangeFlags propertyChange)
{
    Q_UNUSED(propertyChange)

    foreach (const BindingProperty &property, propertyList) {
        if (property.name() == "when" && QmlModelState::isValidQmlModelState(property.parentModelNode()))
            resetModel();
    }
}

void StatesEditorView::variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                                AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    m_block = true;

    for (const VariantProperty &property : propertyList) {
        if (property.name() == "name" && QmlModelState::isValidQmlModelState(property.parentModelNode()))
            resetModel();
    }

    m_block = false;
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
    foreach (const ModelNode &node, nodeList) {
        if (node.isRootNode()) {
            minimumIndex = qMin(minimumIndex, 0);
            maximumIndex = qMax(maximumIndex, 0);
        } else {
            int index = rootStateGroup().allStates().indexOf(QmlModelState(node)) + 1;
            if (index > 0) {
                minimumIndex = qMin(minimumIndex, index);
                maximumIndex = qMax(maximumIndex, index);
            }
        }
    }

    if (maximumIndex >= 0)
        m_statesEditorModel->updateState(minimumIndex, maximumIndex);
}

} // namespace QmlDesigner
