/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "stateseditorview.h"
#include "stateseditormodel.h"
#include <customnotifications.h>
#include <rewritingexception.h>

#include <QPainter>
#include <QTimerEvent>
#include <QMessageBox>
#include <QDebug>
#include <math.h>

#include <variantproperty.h>
#include <nodelistproperty.h>

enum {
    debug = false
};

namespace QmlDesigner {
namespace Internal {

/**
  We always have 'one' current state, where we get updates from (see sceneChanged()). In case
  the current state is the base state, we render the base state + all other states.
  */
StatesEditorView::StatesEditorView(StatesEditorModel *editorModel, QObject *parent) :
        QmlModelView(parent),
        m_editorModel(editorModel)
{
    Q_ASSERT(m_editorModel);
    // base state
}

void StatesEditorView::setCurrentState(int index)
{
    if (debug)
        qDebug() << __FUNCTION__ << index;

    // happens to be the case for an invalid document / no base state
    if (m_modelStates.isEmpty())
        return;

    Q_ASSERT(index < m_modelStates.count());
    if (index == -1)
        return;

    if (m_modelStates.indexOf(currentState()) == index)
        return;

    QmlModelState state(m_modelStates.at(index));
    Q_ASSERT(state.isValid());
    QmlModelView::setCurrentState(state);
}

void StatesEditorView::createState(const QString &name)
{
    if (debug)
        qDebug() << __FUNCTION__ << name;

    try {
        model()->addImport(Import::createLibraryImport("QtQuick", "1.0"));
        stateRootNode().states().addState(name);
    }  catch (RewritingException &e) {
        QMessageBox::warning(0, "Error", e.description());
    }
}

void StatesEditorView::removeState(int index)
{
    if (debug)
        qDebug() << __FUNCTION__ << index;

    Q_ASSERT(index > 0 && index < m_modelStates.size());
    QmlModelState state = m_modelStates.at(index);
    Q_ASSERT(state.isValid());

    setCurrentState(0);

    try {
        m_modelStates.removeAll(state);
        state.destroy();

        m_editorModel->removeState(index);

        int newIndex = (index < m_modelStates.count()) ? index : m_modelStates.count() - 1;
        setCurrentState(newIndex);
    }  catch (RewritingException &e) {
        QMessageBox::warning(0, "Error", e.description());
    }
}

void StatesEditorView::renameState(int index, const QString &newName)
{
    if (debug)
        qDebug() << __FUNCTION__ << index << newName;

    Q_ASSERT(index > 0 && index < m_modelStates.size());
    QmlModelState state = m_modelStates.at(index);
    Q_ASSERT(state.isValid());

    try {
        if (state.name() != newName) {
            // Jump to base state for the change
            QmlModelState oldState = currentState();
            state.setName(newName);
            setCurrentState(m_modelStates.indexOf(oldState));
        }
    }  catch (RewritingException &e) {
        QMessageBox::warning(0, "Error", e.description());
    }
}

void StatesEditorView::duplicateCurrentState(int index)
{
    if (debug)
        qDebug() << __FUNCTION__ << index;

    Q_ASSERT(index > 0 && index < m_modelStates.size());

    QmlModelState state = m_modelStates.at(index);
    Q_ASSERT(state.isValid());
    QString newName = state.name();

    // Strip out numbers at the end of the string
    QRegExp regEx(QString("[0-9]+$"));
    int numberIndex = newName.indexOf(regEx);
    if ((numberIndex != -1) && (numberIndex+regEx.matchedLength()==newName.length()))
        newName = newName.left(numberIndex);

    int i = 1;
    QStringList stateNames = state.stateGroup().names();
    while (stateNames.contains(newName + QString::number(i)))
        i++;
    state.duplicate(newName + QString::number(i));
}

void StatesEditorView::modelAttached(Model *model)
{
    if (debug)
        qDebug() << __FUNCTION__;

    if (model == QmlModelView::model())
        return;

    Q_ASSERT(model);
    QmlModelView::modelAttached(model);
    clearModelStates();

    // Add base state
    if (!baseState().isValid())
        return;

    m_modelStates.insert(0, baseState());
    m_editorModel->insertState(0, baseState().name());

    // Add custom states
    m_stateRootNode = QmlItemNode(rootModelNode());
    if (!m_stateRootNode.isValid())
        return;    

    for (int i = 0; i < m_stateRootNode.states().allStates().size(); ++i) {
        QmlModelState state = QmlItemNode(rootModelNode()).states().allStates().at(i);
        insertModelState(i, state);
    }
}

void StatesEditorView::modelAboutToBeDetached(Model *model)
{
    if (debug)
        qDebug() << __FUNCTION__;

    clearModelStates();

    QmlModelView::modelAboutToBeDetached(model);
}

void StatesEditorView::propertiesAboutToBeRemoved(const QList<AbstractProperty> &propertyList)
{
    if (debug)
        qDebug() << __FUNCTION__;

    foreach (const AbstractProperty &property, propertyList) {
        // remove all states except base state
        if ((property.name()=="states") && (property.parentModelNode().isRootNode())) {
            foreach (const QmlModelState &state, m_modelStates) {
                if (!state.isBaseState())
                    removeModelState(state);
            }
        }
    }

    QmlModelView::propertiesAboutToBeRemoved(propertyList);
}


void StatesEditorView::variantPropertiesChanged(const QList<VariantProperty> &propertyList, PropertyChangeFlags propertyChange)
{
    if (debug)
        qDebug() << __FUNCTION__;

    QmlModelView::variantPropertiesChanged(propertyList, propertyChange);
    foreach (const VariantProperty &property, propertyList) {
        ModelNode node (property.parentModelNode());
        if (QmlModelState(node).isValid() && (property.name() == QLatin1String("name"))) {
            int index = m_modelStates.indexOf(node);
            if (index != -1)
                m_editorModel->renameState(index, property.value().toString());
        }
    }
}

void StatesEditorView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    if (debug)
        qDebug() << __FUNCTION__;

    removeModelState(removedNode);


    QmlModelView::nodeAboutToBeRemoved(removedNode);
}


void StatesEditorView::nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange)
{
    if (debug)
        qDebug() << __FUNCTION__;

    QmlModelView::nodeReparented(node, newPropertyParent, oldPropertyParent, propertyChange);

    // this would be sliding
    Q_ASSERT(newPropertyParent != oldPropertyParent);

    if (QmlModelState(node).isValid()) {
        if (oldPropertyParent.parentModelNode() == m_stateRootNode) {
            if (oldPropertyParent.isNodeListProperty()
                && oldPropertyParent.name() == "states") {
                removeModelState(node);
            } else {
                qWarning() << "States Editor: Reparented model state was not in states property list";
            }
        }

        if (newPropertyParent.parentModelNode() == m_stateRootNode) {
            if (newPropertyParent.isNodeListProperty()
                && newPropertyParent.name() == "states") {
                NodeListProperty statesProperty = newPropertyParent.toNodeListProperty();
                int index = statesProperty.toModelNodeList().indexOf(node);
                Q_ASSERT(index >= 0);
                insertModelState(index, node);
            } else {
                qWarning() << "States Editor: Reparented model state is not in the states property list";
            }
        }
    }
}

void StatesEditorView::nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex)
{
    if (debug)
        qDebug() << __FUNCTION__;

    QmlModelView::nodeOrderChanged(listProperty, movedNode, oldIndex);
    if (listProperty.parentModelNode() == m_stateRootNode
        && listProperty.name() == "states") {

        int newIndex = listProperty.toModelNodeList().indexOf(movedNode);
        Q_ASSERT(newIndex >= 0);

        QmlModelState state = QmlModelState(movedNode);
        if (state.isValid()) {
            Q_ASSERT(oldIndex == modelStateIndex(state));
            removeModelState(state);
            insertModelState(newIndex, state);
            Q_ASSERT(newIndex == modelStateIndex(state));
        }
    }
}

void StatesEditorView::nodeInstancePropertyChanged(const ModelNode &node, const QString &propertyName)
{
    // sets currentState() used in sceneChanged
    QmlModelView::nodeInstancePropertyChanged(node, propertyName);
}

void StatesEditorView::stateChanged(const QmlModelState &newQmlModelState, const QmlModelState &oldQmlModelState)
{
    if (debug)
        qDebug() << __FUNCTION__;

    QmlModelView::stateChanged(newQmlModelState, oldQmlModelState);

    if (!m_settingSilentState) {
        if (newQmlModelState.isBaseState())
            m_editorModel->emitChangedToState(0);
        else
            m_editorModel->emitChangedToState(m_modelStates.indexOf(newQmlModelState));
    }
}

void StatesEditorView::transformChanged(const QmlObjectNode &qmlObjectNode, const QString &propertyName)
{
    if (debug)
        qDebug() << __FUNCTION__;

    QmlModelView::transformChanged(qmlObjectNode, propertyName);
}

void StatesEditorView::parentChanged(const QmlObjectNode &qmlObjectNode)
{
    if (debug)
        qDebug() << __FUNCTION__;

    QmlModelView::parentChanged(qmlObjectNode);
}

void StatesEditorView::otherPropertyChanged(const QmlObjectNode &qmlObjectNode, const QString &propertyName)
{
    if (debug)
        qDebug() << __FUNCTION__;

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


// index without base state
void StatesEditorView::insertModelState(int i, const QmlModelState &state)
{
    if (debug)
        qDebug() << __FUNCTION__ << i << state.name();

    Q_ASSERT(state.isValid());
    Q_ASSERT(!state.isBaseState());
    // For m_modelStates / m_editorModel, i=0 is base state
    m_modelStates.insert(i+1, state);
    m_editorModel->insertState(i+1, state.name());
}

void StatesEditorView::removeModelState(const QmlModelState &state)
{
    if (debug)
        qDebug() << __FUNCTION__ << state.name();

    Q_ASSERT(state.isValid());
    Q_ASSERT(!state.isBaseState());
    int index = m_modelStates.indexOf(state);
    if (index != -1) {
        m_modelStates.removeOne(state);
        m_editorModel->removeState(index);
    }
}

void StatesEditorView::clearModelStates()
{
    if (debug)
        qDebug() << __FUNCTION__;


    // Remove all states
    const int modelStateCount = m_modelStates.size();
    for (int i=modelStateCount-1; i>=0; --i) {
        m_modelStates.removeAt(i);
        m_editorModel->removeState(i);
    }
}

// index without base state
int StatesEditorView::modelStateIndex(const QmlModelState &state)
{
    return m_modelStates.indexOf(state) - 1;
}


} // namespace Internal
} // namespace QmlDesigner
