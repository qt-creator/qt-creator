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

#ifndef ABSTRACTNODEINSTANCEVIEW_H
#define ABSTRACTNODEINSTANCEVIEW_H

#include "corelib_global.h"
#include "abstractview.h"
#include <QtGui/QWidget>
#include <QtCore/QHash>
#include <QtScript/QScriptEngine>
#include <QWeakPointer>
#include <QtCore/QHash>

#include <modelnode.h>
#include <nodeinstance.h>

QT_BEGIN_NAMESPACE
class QmlEngine;
class QGraphicsScene;
class QGraphicsView;
QT_END_NAMESPACE

namespace QmlDesigner {

namespace Internal {
    class ChildrenChangeEventFilter;
}

class CORESHARED_EXPORT NodeInstanceView : public AbstractView
{
    Q_OBJECT

    friend class NodeInstance;
    friend class Internal::ObjectNodeInstance;
public:
    typedef QWeakPointer<NodeInstanceView> Pointer;

    NodeInstanceView(QObject *parent = 0);
    ~NodeInstanceView();

    void modelAttached(Model *model);
    void modelAboutToBeDetached(Model *model);
    void nodeCreated(const ModelNode &createdNode);
    void nodeAboutToBeRemoved(const ModelNode &removedNode);
    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange);
    void propertiesAdded(const ModelNode &node, const QList<AbstractProperty>& propertyList);
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList);
    void propertiesRemoved(const QList<AbstractProperty>& propertyList);
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange);
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange);
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion);

    void fileUrlChanged(const QUrl &oldUrl, const QUrl &newUrl);
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId);

    void modelStateAboutToBeRemoved(const ModelState &modelState);
    void modelStateAdded(const ModelState &modelState);

    void nodeStatesAboutToBeRemoved(const QList<ModelNode> &nodeStateList);
    void nodeStatesAdded(const QList<ModelNode> &nodeStateList);

    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex);

    NodeInstance rootNodeInstance() const;
    NodeInstance viewNodeInstance() const;

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList, const QList<ModelNode> &lastSelectedNodeList);

    QList<NodeInstance> instances() const;
    NodeInstance instanceForNode(const ModelNode &node);
    bool hasInstanceForNode(const ModelNode &node);

    NodeInstance instanceForObject(QObject *object);
    bool hasInstanceForObject(QObject *object);

    void anchorsChanged(const ModelNode &nodeState);

    void render(QPainter *painter, const QRectF &target=QRectF(), const QRectF &source=QRect(), Qt::AspectRatioMode aspectRatioMode=Qt::KeepAspectRatio);

    QRectF boundingRect() const;
    QRectF sceneRect() const;
    void setBlockChangeSignal(bool block);

    void notifyPropertyChange(const ModelNode &modelNode, const QString &propertyName);

    void setQmlModelView(QmlModelView *qmlModelView);
    QmlModelView *qmlModelView() const ;

signals:
    void instanceRemoved(const NodeInstance &nodeInstance);

private slots:
    void emitParentChanged(QObject *child);

private: // functions
    NodeInstance loadNode(const ModelNode &rootNode, QObject *objectToBeWrapped = 0);
    void loadModel(Model *model);
    void loadNodes(const QList<ModelNode> &nodeList);

    void removeAllInstanceNodeRelationships();

    void removeRecursiveChildRelationship(const ModelNode &removedNode);

    void insertInstanceNodeRelationship(const ModelNode &node, const NodeInstance &instance);
    void removeInstanceNodeRelationship(const ModelNode &node);

    QmlEngine *engine() const;
    Internal::ChildrenChangeEventFilter *childrenChangeEventFilter();
    void removeInstanceAndSubInstances(const ModelNode &node);

private: //variables
    NodeInstance m_rootNodeInstance;
    QScopedPointer<QGraphicsView> m_graphicsView;

    QHash<ModelNode, NodeInstance> m_nodeInstanceHash;
    QHash<QObject*, NodeInstance> m_objectInstanceHash; // This is purely internal. Might contain dangling pointers!
    QWeakPointer<QmlEngine> m_engine;
    QWeakPointer<Internal::ChildrenChangeEventFilter> m_childrenChangeEventFilter;

    QWeakPointer<QmlModelView> m_qmlModelView;

    bool m_blockChangeSignal;
};

}

#endif // ABSTRACTNODEINSTANCEVIEW_H
