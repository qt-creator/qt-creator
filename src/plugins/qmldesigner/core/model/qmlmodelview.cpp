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

#include "qmlmodelview.h"
#include "qmlobjectnode.h"
#include "qmlitemnode.h"
#include "itemlibraryinfo.h"
#include "modelutilities.h"
#include "mathutils.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>

namespace QmlDesigner {

QmlModelView::QmlModelView(QObject *parent)
    : ForwardView<NodeInstanceView>(parent)
{
    appendView(new NodeInstanceView(this));
    connect(nodeInstanceView(), SIGNAL(transformPropertyChanged(NodeInstance)), SLOT(notifyTransformChanged(NodeInstance)));
    connect(nodeInstanceView(), SIGNAL(parentPropertyChanged(NodeInstance)), SLOT(notifyParentChanged(NodeInstance)));
    connect(nodeInstanceView(), SIGNAL(otherPropertyChanged(NodeInstance)), SLOT(notifyOtherPropertyChanged(NodeInstance)));
    connect(nodeInstanceView(), SIGNAL(updateItem(NodeInstance)), SLOT(notifyUpdateItem(NodeInstance)));
}

void QmlModelView::setCurrentState(const QmlModelState &state)
{
    if (m_state == state)
        return;
    QmlModelState oldState = m_state;
    emitCustomNotification("__state changed__", QList<ModelNode>() << state.modelNode());
}

QmlModelState QmlModelView::currentState() const
{
    return m_state;
}

QmlModelState QmlModelView::baseState() const
{
    return QmlModelState::createBaseState(this);
}

QmlObjectNode QmlModelView::createQmlObjectNode(const QString &typeString,
                     int majorVersion,
                     int minorVersion,
                     const PropertyListType &propertyList)
{
    return QmlObjectNode(createModelNode(typeString, majorVersion, minorVersion, propertyList));
}

QmlItemNode QmlModelView::createQmlItemNode(const QString &typeString,
                                int majorVersion,
                                int minorVersion,
                                const PropertyListType &propertyList)
{
    return createQmlObjectNode(typeString, majorVersion, minorVersion, propertyList).toQmlItemNode();
}

QmlItemNode QmlModelView::createQmlItemNodeFromImage(const QString &imageName, const QPointF &position, QmlItemNode parentNode)
{
    if (!parentNode.isValid())
        parentNode = rootQmlItemNode();

    if (!parentNode.isValid())
        return QmlItemNode();

    QmlItemNode newNode;
    RewriterTransaction transaction = beginRewriterTransaction();
    {
        QList<QPair<QString, QVariant> > propertyPairList;
        propertyPairList.append(qMakePair(QString("x"), QVariant( round(position.x(), 4))));
        propertyPairList.append(qMakePair(QString("y"), QVariant( round(position.y(), 4))));

        QString relativeImageName = imageName;

        //use relative path
        if (QFileInfo(model()->fileUrl().toLocalFile()).exists()) {
            QDir fileDir(QFileInfo(model()->fileUrl().toLocalFile()).absolutePath());
            relativeImageName = fileDir.relativeFilePath(imageName);
        }

        propertyPairList.append(qMakePair(QString("source"), QVariant(relativeImageName)));
        newNode = createQmlItemNode("Qt/Image",4, 6, propertyPairList);
        parentNode.nodeAbstractProperty("data").reparentHere(newNode);

        Q_ASSERT(newNode.isValid());

        QString id;
        int i = 1;
        QString name("image");
        name.remove(QLatin1Char(' '));
        do {
            id = name + QString::number(i);
            i++;
        } while (hasId(id)); //If the name already exists count upwards

        newNode.setId(id);

        Q_ASSERT(newNode.isValid());
    }

    Q_ASSERT(newNode.isValid());

    return newNode;
}

QmlItemNode QmlModelView::createQmlItemNode(const ItemLibraryInfo &itemLibraryRepresentation, const QPointF &position, QmlItemNode parentNode)
{
    if (!parentNode.isValid())
        parentNode = rootQmlItemNode();

    Q_ASSERT(parentNode.isValid());


    QmlItemNode newNode;
    RewriterTransaction transaction = beginRewriterTransaction();
    {
        QList<QPair<QString, QVariant> > propertyPairList;
        propertyPairList.append(qMakePair(QString("x"), QVariant(round(position.x(), 4))));
        propertyPairList.append(qMakePair(QString("y"), QVariant(round(position.y(), 4))));

        foreach (const PropertyContainer &property, itemLibraryRepresentation.properties())
            propertyPairList.append(qMakePair(property.name(), property.value()));

        qDebug() << itemLibraryRepresentation.typeName();

        newNode = createQmlItemNode(itemLibraryRepresentation.typeName(), itemLibraryRepresentation.majorVersion(), itemLibraryRepresentation.minorVersion(), propertyPairList);
        parentNode.nodeAbstractProperty("data").reparentHere(newNode);

        Q_ASSERT(newNode.isValid());

        QString id;
        int i = 1;
        QString name(itemLibraryRepresentation.name().toLower());
        name.remove(QLatin1Char(' '));
        do {
            id = name + QString::number(i);
            i++;
        } while (hasId(id)); //If the name already exists count upwards

        newNode.setId(id);

        Q_ASSERT(newNode.isValid());
    }

    Q_ASSERT(newNode.isValid());

    return newNode;
}

QmlObjectNode QmlModelView::rootQmlObjectNode() const
{
    return QmlObjectNode(rootModelNode());
}

QmlItemNode QmlModelView::rootQmlItemNode() const
{
    return rootQmlObjectNode().toQmlItemNode();
}

void QmlModelView::setSelectedQmlObjectNodes(const QList<QmlObjectNode> &selectedNodeList)
{
    setSelectedModelNodes(QmlDesigner::toModelNodeList(selectedNodeList));
}

void QmlModelView::selectQmlObjectNode(const QmlObjectNode &node)
{
     selectModelNode(node.modelNode());
}

void QmlModelView::deselectQmlObjectNode(const QmlObjectNode &node)
{
    deselectModelNode(node.modelNode());
}

QList<QmlObjectNode> QmlModelView::selectedQmlObjectNodes() const
{
    return toQmlObjectNodeList(selectedModelNodes());
}

QList<QmlItemNode> QmlModelView::selectedQmlItemNodes() const
{
    return toQmlItemNodeList(selectedModelNodes());
}

void QmlModelView::setSelectedQmlItemNodes(const QList<QmlItemNode> &selectedNodeList)
{
    return setSelectedModelNodes(QmlDesigner::toModelNodeList(selectedNodeList));
}

QmlObjectNode QmlModelView::fxObjectNodeForId(const QString &id)
{
    return QmlObjectNode(modelNodeForId(id));
}


void QmlModelView::notifyTransformChanged(const NodeInstance &nodeInstance)
{
    transformChanged(QmlObjectNode(ModelNode(nodeInstance.modelNode(), this)));
}

void QmlModelView::notifyParentChanged(const NodeInstance &nodeInstance)
{
    parentChanged(QmlObjectNode(ModelNode(nodeInstance.modelNode(), this)));
}

void QmlModelView::notifyOtherPropertyChanged(const NodeInstance &nodeInstance)
{
    otherPropertyChanged(QmlObjectNode(ModelNode(nodeInstance.modelNode(), this)));
}

void QmlModelView::notifyUpdateItem(const NodeInstance &nodeInstance)
{
    updateItem(QmlObjectNode(ModelNode(nodeInstance.modelNode(), this)));
}

void QmlModelView::customNotification(const AbstractView * /* view */, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> & /* data */)
{
    if (identifier == "__state changed__") {
        QmlModelState state(nodeList.first());
        if (state.isValid())
            stateChanged(state, currentState());
        else
            stateChanged(baseState(), currentState());
    }
}

NodeInstance QmlModelView::instanceForModelNode(const ModelNode &modelNode)
{
    return nodeInstanceView()->instanceForNode(modelNode);
}

bool QmlModelView::hasInstanceForModelNode(const ModelNode &modelNode)
{
    return nodeInstanceView()->hasInstanceForNode(modelNode);
}

NodeInstanceView *QmlModelView::nodeInstanceView() const
{
    return firstView();
}

void QmlModelView::modelAttached(Model *model)
{
    m_state = QmlModelState();
    ForwardView<NodeInstanceView>::modelAttached(model);
    m_state = baseState();
    Q_ASSERT(m_state.isBaseState());
}

void QmlModelView::modelAboutToBeDetached(Model *model)
{
    ForwardView<NodeInstanceView>::modelAboutToBeDetached(model);
    m_state = QmlModelState();
}

void QmlModelView::transformChanged(const QmlObjectNode &/*qmlObjectNode*/)
{
}

void QmlModelView::parentChanged(const QmlObjectNode &/*qmlObjectNode*/)
{

}

void QmlModelView::otherPropertyChanged(const QmlObjectNode &/*qmlObjectNode*/)
{

}

void QmlModelView::updateItem(const QmlObjectNode &/*qmlObjectNode*/)
{

}

void  QmlModelView::stateChanged(const QmlModelState &newQmlModelState, const QmlModelState &oldQmlModelState)
{
    m_state = newQmlModelState;
    if (newQmlModelState.isValid()) {
        NodeInstance newStateInstance = instanceForModelNode(newQmlModelState.modelNode());
        Q_ASSERT(newStateInstance.isValid());
        if (!newQmlModelState.isBaseState())
            newStateInstance.setPropertyVariant("__activateState", true);
    }

    if (oldQmlModelState.isValid()) {
        NodeInstance oldStateInstance = instanceForModelNode(oldQmlModelState.modelNode());
        Q_ASSERT(oldStateInstance.isValid());
        if (!oldQmlModelState.isBaseState())
            oldStateInstance.setPropertyVariant("__activateState", false);
    }
}

} //QmlDesigner
