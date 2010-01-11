/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QPainter>
#include <QTimerEvent>
#include <QDebug>

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


    connect(nodeInstanceView(), SIGNAL(instanceRemoved(NodeInstance)), this, SLOT(sceneChanged()));
    connect(nodeInstanceView(), SIGNAL(transformPropertyChanged(NodeInstance)), this, SLOT(sceneChanged()));
    connect(nodeInstanceView(), SIGNAL(parentPropertyChanged(NodeInstance)), this, SLOT(sceneChanged()));
    connect(nodeInstanceView(), SIGNAL(otherPropertyChanged(NodeInstance)), this, SLOT(sceneChanged()));
    connect(nodeInstanceView(), SIGNAL(updateItem(NodeInstance)), this, SLOT(sceneChanged()));
}

void StatesEditorView::setCurrentStateSilent(int index)
{
    // TODO
    QmlModelState state(m_modelStates.at(index));
    if (!state.isValid())
        return;
    if (state == currentState())
        return;
    QmlModelView::stateChanged(state, currentState());
}

void StatesEditorView::setCurrentState(int index)
{
    // TODO
    if (m_modelStates.indexOf(currentState()) == index)
        return;

    if (index >= m_modelStates.count())
        return;

    QmlModelState state(m_modelStates.at(index));
    Q_ASSERT(state.isValid());
    QmlModelView::setCurrentState(state);
}


void StatesEditorView::setBackCurrentState(int index, const QmlModelState &oldState)
{
    // TODO
    QmlModelState state(m_modelStates.at(index));
    if (!state.isValid())
        return;
    if (state == oldState)
        return;
    QmlModelView::stateChanged(oldState, state);
}

void StatesEditorView::createState(const QString &name)
{
    stateRootNode().states().addState(name);
}

void StatesEditorView::removeState(int index)
{
    Q_ASSERT(index > 0 && index < m_modelStates.size());
    QmlModelState state = m_modelStates.at(index);
    Q_ASSERT(state.isValid());
    m_modelStates.removeAll(state);
    m_editorModel->removeState(index);
    QmlModelView::setCurrentState(baseState());
    state.destroy();
}

void StatesEditorView::renameState(int index,const QString &newName)
{
    Q_ASSERT(index > 0 && index < m_modelStates.size());
    QmlModelState state = m_modelStates.at(index);
    Q_ASSERT(state.isValid());
    if (state.name() != newName) {
        // Jump to base state for the change
        QmlModelState oldState = currentState();
        setCurrentStateSilent(0);
        state.setName(newName);
        setBackCurrentState(0, oldState);
    }
}

void StatesEditorView::duplicateCurrentState(int index)
{
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
    if (model == QmlModelView::model())
        return;

    Q_ASSERT(model);
    QmlModelView::modelAttached(model);
    Q_ASSERT(m_editorModel->rowCount(QModelIndex()) == 0);

    // find top level states
    m_stateRootNode = QmlItemNode(rootModelNode());
    if (!m_stateRootNode.isValid())
        return;

    clearModelStates();

    // Add base state
    m_modelStates.insert(0, baseState());
    m_editorModel->insertState(0, baseState().name());

    // Add custom states
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

void StatesEditorView::propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList)
{
    foreach (const AbstractProperty &property, propertyList) {
        // remove all states except base state
        if ((property.name()=="states") && (property.parentModelNode().isRootNode())) {
            foreach (const QmlModelState &state, m_modelStates) {
                if (!state.isBaseState())
                    removeModelState(state);
            }
        } else {
            ModelNode node (property.parentModelNode().parentProperty().parentModelNode());
            if (QmlModelState(node).isValid()) {
                startUpdateTimer(modelStateIndex(node) + 1, 0);
            }
        }
    }
}

void StatesEditorView::propertiesRemoved(const QList<AbstractProperty>& propertyList)
{
    QmlModelView::propertiesRemoved(propertyList);
}

void StatesEditorView::variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange)
{
    QmlModelView::variantPropertiesChanged(propertyList, propertyChange);
    foreach (const VariantProperty &property, propertyList) {
        ModelNode node (property.parentModelNode());
        if (QmlModelState(node).isValid() && (property.name() == QLatin1String("name"))) {
            int index = m_modelStates.indexOf(node);
            if (index != -1)
                m_editorModel->renameState(index, property.value().toString());
        }
    }
    foreach (const AbstractProperty &property, propertyList) {
        ModelNode node (property.parentModelNode().parentProperty().parentModelNode());
        if (QmlModelState(node).isValid()) {
            startUpdateTimer(modelStateIndex(node) + 1, 0);
        } else { //a change to the base state update all
            for (int i = 0; i < m_modelStates.count(); ++i)
                startUpdateTimer(i, 0);
        }
    }
}

void StatesEditorView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    if (removedNode.parentProperty().parentModelNode() == m_stateRootNode
          && QmlModelState(removedNode).isValid()) {
        removeModelState(removedNode);
    }
    QmlModelView::nodeAboutToBeRemoved(removedNode);
}


void StatesEditorView::nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange)
{
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

void StatesEditorView::nodeSlidedToIndex(const NodeListProperty &listProperty, int newIndex, int oldIndex)
{
    QmlModelView::nodeSlidedToIndex(listProperty, newIndex, oldIndex);
    if (listProperty.parentModelNode() == m_stateRootNode
        && listProperty.name() == "states") {
        int index = newIndex;
        if (oldIndex < newIndex)
            --index;
        QmlModelState state = listProperty.toModelNodeList().at(index);
        if (state.isValid()) {
            Q_ASSERT(oldIndex == modelStateIndex(state));
            removeModelState(state);
            insertModelState(newIndex, state);
            Q_ASSERT(newIndex == modelStateIndex(state));
        }
    }
}

void StatesEditorView::stateChanged(const QmlModelState &newQmlModelState, const QmlModelState &oldQmlModelState)
{
    QmlModelView::stateChanged(newQmlModelState, oldQmlModelState);

    if (newQmlModelState.isBaseState())
        m_editorModel->emitChangedToState(0);
    else
        m_editorModel->emitChangedToState(m_modelStates.indexOf(newQmlModelState));
}

void StatesEditorView::customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data)
{
    QmlModelView::customNotification(view, identifier, nodeList, data);
    if (identifier == StartRewriterAmend) {
        m_oldRewriterAmendState = currentState();
        QmlModelView::setCurrentState(baseState());
    } else if (identifier == EndRewriterAmend) {
        if (m_oldRewriterAmendState.isValid())
            QmlModelView::setCurrentState(m_oldRewriterAmendState);
    }
}

QPixmap StatesEditorView::renderState(int i)
{
    Q_ASSERT(i >= 0 && i < m_modelStates.size());
    nodeInstanceView()->setBlockChangeSignal(true);
    QmlModelState oldState = currentState();
    setCurrentStateSilent(i);

    Q_ASSERT(nodeInstanceView());

    const int checkerbordSize= 10;
    QPixmap tilePixmap(checkerbordSize * 2, checkerbordSize * 2);
    tilePixmap.fill(Qt::white);
    QPainter tilePainter(&tilePixmap);
    QColor color(220, 220, 220);
    tilePainter.fillRect(0, 0, checkerbordSize, checkerbordSize, color);
    tilePainter.fillRect(checkerbordSize, checkerbordSize, checkerbordSize, checkerbordSize, color);
    tilePainter.end();


    QSize pixmapSize(nodeInstanceView()->sceneRect().size().toSize());
    if (pixmapSize.width() > 150 || pixmapSize.height() > 150) // sensible maximum size
        pixmapSize.scale(QSize(150, 150), Qt::KeepAspectRatio);
    QPixmap pixmap(pixmapSize);

    QPainter painter(&pixmap);
    painter.drawTiledPixmap(pixmap.rect(), tilePixmap);
    nodeInstanceView()->render(&painter, pixmap.rect(), nodeInstanceView()->sceneRect());

    setBackCurrentState(i, oldState);
    nodeInstanceView()->setBlockChangeSignal(false);
    Q_ASSERT(oldState == currentState());

    return pixmap;
}

void StatesEditorView::sceneChanged()
{
    // If we are in base state we have to update the pixmaps of all states
    // otherwise only the pixmpap for the current state

    if (currentState().isValid()) { //during setup we might get sceneChanged signals with an invalid currentState()
        if (currentState().isBaseState()) {
            for (int i = 0; i < m_modelStates.count(); ++i)
                startUpdateTimer(i, i * 80);
        } else {
            startUpdateTimer(modelStateIndex(currentState()) + 1, 0);
        }
    }
}

void StatesEditorView::startUpdateTimer(int i, int offset) {
    if (i < m_updateTimerIdList.size() && m_updateTimerIdList.at(i) != 0)
        return;
    // TODO: Add an offset so not all states are rendered at once
    Q_ASSERT(i >= 0 && i < m_modelStates.count());
    if (i < m_updateTimerIdList.size() && i > 0)
        if (m_updateTimerIdList.at(i))
            killTimer(m_updateTimerIdList.at(i));
    int j = i;

    while (m_updateTimerIdList.size() <= i) {
        m_updateTimerIdList.insert(j, 0);
        j++;
    }
    m_updateTimerIdList[i] =  startTimer(100 + offset);
}

// index without base state
void StatesEditorView::insertModelState(int i, const QmlModelState &state)
{
    Q_ASSERT(state.isValid());
    Q_ASSERT(!state.isBaseState());
    // For m_modelStates / m_editorModel, i=0 is base state
    m_modelStates.insert(i+1, state);
    m_editorModel->insertState(i+1, state.name());
}

void StatesEditorView::removeModelState(const QmlModelState &state)
{
    Q_ASSERT(state.isValid());
    Q_ASSERT(!state.isBaseState());
    int index = m_modelStates.indexOf(state);
    if (index != -1) {
        m_modelStates.removeOne(state);

        if (m_updateTimerIdList.contains(index)) {
            killTimer(m_updateTimerIdList[index]);
            m_updateTimerIdList[index] = 0;
        }
        m_editorModel->removeState(index);
    }
}

void StatesEditorView::clearModelStates()
{
    // For m_modelStates / m_editorModel, i=0 is base state
    while (m_modelStates.size()) {
        m_modelStates.removeFirst();
        m_editorModel->removeState(0);
    }
}

// index without base state
int StatesEditorView::modelStateIndex(const QmlModelState &state)
{
    return m_modelStates.indexOf(state) - 1;
}

void StatesEditorView::timerEvent(QTimerEvent *event)
{
    int index = m_updateTimerIdList.indexOf(event->timerId());
    if (index > -1) {
        event->accept();
        Q_ASSERT(index >= 0);
        if (index < m_modelStates.count()) //there might be updates for a state already deleted 100ms are long
            m_editorModel->updateState(index);
        killTimer(m_updateTimerIdList[index]);
       m_updateTimerIdList[index] = 0;
    } else {
        QmlModelView::timerEvent(event);
    }
}

} // namespace Internal
} // namespace QmlDesigner
