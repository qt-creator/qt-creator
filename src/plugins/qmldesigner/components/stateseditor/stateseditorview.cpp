/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

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

enum {
    debug = false
};

namespace QmlDesigner {

/**
  We always have 'one' current state, where we get updates from (see sceneChanged()). In case
  the current state is the base state, we render the base state + all other states.
  */
StatesEditorView::StatesEditorView(QObject *parent) :
        QmlModelView(parent),
        m_statesEditorModel(new StatesEditorModel(this)),
        m_lastIndex(-1)
{
    Q_ASSERT(m_statesEditorModel);
    // base state
}

StatesEditorWidget *StatesEditorView::widget()
{
    if (m_statesEditorWidget.isNull())
        m_statesEditorWidget = new StatesEditorWidget(this, m_statesEditorModel.data());

    return m_statesEditorWidget.data();
}

void StatesEditorView::removeState(int nodeId)
{
    try {
        if (nodeId > 0 && hasModelNodeForInternalId(nodeId)) {
            ModelNode stateNode(modelNodeForInternalId(nodeId));
            Q_ASSERT(stateNode.metaInfo().isSubclassOf("QtQuick/State", 4, 7));
            NodeListProperty parentProperty = stateNode.parentProperty().toNodeListProperty();

            if (parentProperty.count() <= 1) {
                setCurrentState(baseState());
            } else if (parentProperty.isValid()){
                int index = parentProperty.indexOf(stateNode);
                if (index == 0) {
                    setCurrentState(parentProperty.at(1));
                } else {
                    setCurrentState(parentProperty.at(index - 1));
                }
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
    if (currentState().isBaseState()) {
        addState();
    } else {
        duplicateCurrentState();
    }
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
        newStateName = tr("State%1", "Default name for newly created states").arg(index++);
        if (!modelStateNames.contains(newStateName))
            break;
    }

    try {
        if (rootStateGroup().allStates().count() < 1)
            model()->addImport(Import::createLibraryImport("QtQuick", "1.0"));
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
        if (currentState().isBaseState()) {
            m_statesEditorWidget->setCurrentStateInternalId(currentState().modelNode().internalId());
        } else {
            m_statesEditorWidget->setCurrentStateInternalId(0);
        }
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

    resetModel();
}

void StatesEditorView::modelAboutToBeDetached(Model *model)
{
    QmlModelView::modelAboutToBeDetached(model);
    resetModel();
}

void StatesEditorView::propertiesAboutToBeRemoved(const QList<AbstractProperty> &propertyList)
{
    foreach (const AbstractProperty &property, propertyList) {
        if (property.name() == "states" && property.parentModelNode().isRootNode())
            m_statesEditorModel->reset();
    }
}


void StatesEditorView::variantPropertiesChanged(const QList<VariantProperty> &propertyList, PropertyChangeFlags /*propertyChange*/)
{
    foreach (const VariantProperty &property, propertyList) {
        if (property.name() == "name" && property.parentModelNode().hasParentProperty()) {
            NodeAbstractProperty parentProperty = property.parentModelNode().parentProperty();
            if (parentProperty.name() == "states" && parentProperty.parentModelNode().isRootNode()) {
                m_statesEditorModel->updateState(parentProperty.indexOf(property.parentModelNode()));
            }
        }
    }
}


void StatesEditorView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    if (removedNode.hasParentProperty()) {
        const NodeAbstractProperty propertyParent = removedNode.parentProperty();
        if (propertyParent.parentModelNode().isRootNode() && propertyParent.name() == "states") {
            m_lastIndex = propertyParent.indexOf(removedNode);
        }
    }
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

void StatesEditorView::stateChanged(const QmlModelState &newQmlModelState, const QmlModelState &oldQmlModelState)
{
    if (newQmlModelState.isBaseState()) {
        m_statesEditorWidget->setCurrentStateInternalId(0);
    } else {
        m_statesEditorWidget->setCurrentStateInternalId(newQmlModelState.modelNode().internalId());
    }
    QmlModelView::stateChanged(newQmlModelState, oldQmlModelState);
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


void StatesEditorView::customNotification(const AbstractView * view, const QString & identifier, const QList<ModelNode> & nodeList, const QList<QVariant> &imageList)
{
    if (debug)
        qDebug() << __FUNCTION__;

    if (identifier == "__state preview updated__")   {
        if (nodeList.size() != imageList.size())
            return;

//        if (++m_updateCounter == INT_MAX)
//            m_updateCounter = 0;

        for (int i = 0; i < nodeList.size(); i++) {
            QmlModelState modelState(nodeList.at(i));            
        }

     //   emit dataChanged(createIndex(i, 0), createIndex(i, 0));

    } else {
        QmlModelView::customNotification(view, identifier, nodeList, imageList);
    }
}

void StatesEditorView::scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList)
{
    if (debug)
        qDebug() << __FUNCTION__;

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
