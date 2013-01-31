/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "stateseditorview.h"
#include "stateseditorwidget.h"
#include "stateseditormodel.h"
#include <customnotifications.h>
#include <rewritingexception.h>

#include <QPainter>
#include <QTimerEvent>
#include <QMessageBox>
#include <QDebug>
#include <math.h>

#include <nodemetainfo.h>

#include <variantproperty.h>
#include <nodelistproperty.h>

namespace QmlDesigner {

/**
  We always have 'one' current state, where we get updates from (see sceneChanged()). In case
  the current state is the base state, we render the base state + all other states.
  */
StatesEditorView::StatesEditorView(QObject *parent) :
        QmlModelView(parent),
        m_statesEditorModel(new StatesEditorModel(this)),
        m_statesEditorWidget(new StatesEditorWidget(this, m_statesEditorModel.data())),
        m_lastIndex(-1)
{
    Q_ASSERT(m_statesEditorModel);
    // base state
}

QWidget *StatesEditorView::widget()
{
    return m_statesEditorWidget.data();
}

void StatesEditorView::removeState(int nodeId)
{
    try {
        if (nodeId > 0 && hasModelNodeForInternalId(nodeId)) {
            ModelNode stateNode(modelNodeForInternalId(nodeId));
            Q_ASSERT(stateNode.metaInfo().isSubclassOf("QtQuick.State", -1, -1));
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
    }  catch (RewritingException &e) {
        QMessageBox::warning(0, "Error", e.description());
    }
}

void StatesEditorView::synchonizeCurrentStateFromWidget()
{
    if (!model())
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
    if (!rootQmlItemNode().isValid())
        return;

    QStringList modelStateNames = rootStateGroup().names();

    QString newStateName;
    int index = 1;
    while (true) {
        newStateName = QString("State%1").arg(index++);
        if (!modelStateNames.contains(newStateName))
            break;
    }

    try {
        if ((rootStateGroup().allStates().count() < 1) && //QtQuick import might be missing
            (!model()->hasImport(Import::createLibraryImport("QtQuick", "1.0"), true)
             && !model()->hasImport(Import::createLibraryImport("QtQuick", "1.1"), true)
             && !model()->hasImport(Import::createLibraryImport("QtQuick", "2.0"), true)))
            model()->changeImports(QList<Import>() << Import::createLibraryImport("QtQuick", "1.0"), QList<Import>());
        ModelNode newState = rootStateGroup().addState(newStateName);
        setCurrentState(newState);
    }  catch (RewritingException &e) {
        QMessageBox::warning(0, "Error", e.description());
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
    QRegExp regEx(QString("[0-9]+$"));
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

void StatesEditorView::renameState(int nodeId, const QString &newName)
{
    if (hasModelNodeForInternalId(nodeId)) {
        QmlModelState state(modelNodeForInternalId(nodeId));
        try {
            if (state.isValid() && state.name() != newName) {
                // Jump to base state for the change
                QmlModelState oldState = currentState();
                setCurrentState(baseState());
                state.setName(newName);
                setCurrentState(oldState);
            }
        }  catch (RewritingException &e) {
            QMessageBox::warning(0, "Error", e.description());
        }
    }
}

void StatesEditorView::modelAttached(Model *model)
{
    if (model == QmlModelView::model())
        return;

    Q_ASSERT(model);
    QmlModelView::modelAttached(model);

    if (m_statesEditorWidget)
        m_statesEditorWidget->setNodeInstanceView(nodeInstanceView());

    resetModel();
}

void StatesEditorView::modelAboutToBeDetached(Model *model)
{
    QmlModelView::modelAboutToBeDetached(model);
    resetModel();
}

void StatesEditorView::propertiesRemoved(const QList<AbstractProperty>& propertyList)
{
    foreach (const AbstractProperty &property, propertyList) {
        if (property.name() == "states" && property.parentModelNode().isRootNode())
            resetModel();
    }
}


void StatesEditorView::variantPropertiesChanged(const QList<VariantProperty> &/*propertyList*/, PropertyChangeFlags /*propertyChange*/)
{
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

void StatesEditorView::nodeInstancePropertyChanged(const ModelNode &node, const QString &propertyName)
{
    // sets currentState() used in sceneChanged
    QmlModelView::nodeInstancePropertyChanged(node, propertyName);
}

void StatesEditorView::actualStateChanged(const ModelNode &node)
{
    QmlModelState newQmlModelState(node);

    if (newQmlModelState.isBaseState())
        m_statesEditorWidget->setCurrentStateInternalId(0);
    else
        m_statesEditorWidget->setCurrentStateInternalId(newQmlModelState.modelNode().internalId());
    QmlModelView::actualStateChanged(node);
}

void StatesEditorView::transformChanged(const QmlObjectNode &qmlObjectNode, const QString &propertyName)
{
    QmlModelView::transformChanged(qmlObjectNode, propertyName);
}

void StatesEditorView::parentChanged(const QmlObjectNode &qmlObjectNode)
{
    QmlModelView::parentChanged(qmlObjectNode);
}

void StatesEditorView::otherPropertyChanged(const QmlObjectNode &qmlObjectNode, const QString &propertyName)
{
    QmlModelView::otherPropertyChanged(qmlObjectNode, propertyName);
}

void StatesEditorView::instancesPreviewImageChanged(const QVector<ModelNode> &nodeList)
{
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

void StatesEditorView::scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList)
{

    QmlModelView::scriptFunctionsChanged(node, scriptFunctionList);
}

void StatesEditorView::nodeIdChanged(const ModelNode &/*node*/, const QString &/*newId*/, const QString &/*oldId*/)
{

}

void StatesEditorView::bindingPropertiesChanged(const QList<BindingProperty> &/*propertyList*/, PropertyChangeFlags /*propertyChange*/)
{

}

void StatesEditorView::selectedNodesChanged(const QList<ModelNode> &/*selectedNodeList*/, const QList<ModelNode> &/*lastSelectedNodeList*/)
{

}

} // namespace QmlDesigner
