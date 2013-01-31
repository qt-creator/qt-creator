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

#ifndef NODEINSTANCEVIEW_H
#define NODEINSTANCEVIEW_H

#include "qmldesignercorelib_global.h"
#include "abstractview.h"

#include <modelnode.h>
#include <nodeinstance.h>
#include <nodeinstanceclientinterface.h>
#include <nodeinstanceserverinterface.h>

#include <QHash>
#include <QSet>
#include <QImage>
#include <QWeakPointer>
#include <QRectF>
#include <QTime>

QT_BEGIN_NAMESPACE
class QDeclarativeEngine;
class QGraphicsView;
class QFileSystemWatcher;
class QPainter;
QT_END_NAMESPACE

namespace QmlDesigner {

class NodeInstanceServerInterface;
class CreateSceneCommand;
class CreateInstancesCommand;
class ClearSceneCommand;
class ReparentInstancesCommand;
class ChangeFileUrlCommand;
class ChangeValuesCommand;
class ChangeBindingsCommand;
class ChangeIdsCommand;
class RemoveInstancesCommand;
class RemovePropertiesCommand;
class CompleteComponentCommand;
class InformationContainer;
class TokenCommand;

class QMLDESIGNERCORE_EXPORT NodeInstanceView : public AbstractView, public NodeInstanceClientInterface
{
    Q_OBJECT

    friend class NodeInstance;

public:
    typedef QWeakPointer<NodeInstanceView> Pointer;

    explicit NodeInstanceView(QObject *parent = 0, NodeInstanceServerInterface::RunModus runModus = NodeInstanceServerInterface::NormalModus);
    ~NodeInstanceView();

    void modelAttached(Model *model);
    void modelAboutToBeDetached(Model *model);
    void nodeCreated(const ModelNode &createdNode);
    void nodeAboutToBeRemoved(const ModelNode &removedNode);
    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange);
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList);
    void propertiesRemoved(const QList<AbstractProperty>& propertyList);
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange);
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange);
    void nodeAboutToBeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion);
    void fileUrlChanged(const QUrl &oldUrl, const QUrl &newUrl);
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId);
    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex);
    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList, const QList<ModelNode> &lastSelectedNodeList);
    void scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList);
    void instancePropertyChange(const QList<QPair<ModelNode, QString> > &propertyList);
    void instancesCompleted(const QVector<ModelNode> &completedNodeList);
    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports);
    void instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &informationChangeHash);
    void instancesRenderImageChanged(const QVector<ModelNode> &nodeList);
    void instancesPreviewImageChanged(const QVector<ModelNode> &nodeList);
    void instancesChildrenChanged(const QVector<ModelNode> &nodeList);
    void instancesToken(const QString &tokenName, int tokenNumber, const QVector<ModelNode> &nodeVector);
    void auxiliaryDataChanged(const ModelNode &node, const QString &name, const QVariant &data);
    void customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data);
    void nodeSourceChanged(const ModelNode &modelNode, const QString &newNodeSource);


    void rewriterBeginTransaction();
    void rewriterEndTransaction();

    void actualStateChanged(const ModelNode &node);

    QList<NodeInstance> instances() const;
    NodeInstance instanceForNode(const ModelNode &node) const ;
    bool hasInstanceForNode(const ModelNode &node) const;

    NodeInstance instanceForId(qint32 id);
    bool hasInstanceForId(qint32 id);

    QRectF sceneRect() const;

    NodeInstance activeStateInstance() const;


    void valuesChanged(const ValuesChangedCommand &command);
    void pixmapChanged(const PixmapChangedCommand &command);
    void informationChanged(const InformationChangedCommand &command);
    void childrenChanged(const ChildrenChangedCommand &command);
    void statePreviewImagesChanged(const StatePreviewImageChangedCommand &command);
    void componentCompleted(const ComponentCompletedCommand &command);
    void token(const TokenCommand &command);
    void debugOutput(const DebugOutputCommand &command);

    QImage statePreviewImage(const ModelNode &stateNode) const;

    void setPathToQt(const QString &pathToQt);

    void sendToken(const QString &token, int number, const QVector<ModelNode> &nodeVector);

    QWidget *widget();

signals:
    void qmlPuppetCrashed();

private: // functions
    void activateState(const NodeInstance &instance);
    void activateBaseState();

    NodeInstance rootNodeInstance() const;

    NodeInstance loadNode(const ModelNode &node);

    void removeAllInstanceNodeRelationships();

    void removeRecursiveChildRelationship(const ModelNode &removedNode);

    void insertInstanceRelationships(const NodeInstance &instance);
    void removeInstanceNodeRelationship(const ModelNode &node);

    void removeInstanceAndSubInstances(const ModelNode &node);

    void setStateInstance(const NodeInstance &stateInstance);
    void clearStateInstance();

    NodeInstanceServerInterface *nodeInstanceServer() const;
    QMultiHash<ModelNode, InformationName> informationChanged(const QVector<InformationContainer> &containerVector);


    CreateSceneCommand createCreateSceneCommand();
    ClearSceneCommand createClearSceneCommand() const;
    CreateInstancesCommand createCreateInstancesCommand(const QList<NodeInstance> &instanceList) const;
    CompleteComponentCommand createComponentCompleteCommand(const QList<NodeInstance> &instanceList) const;
    ComponentCompletedCommand createComponentCompletedCommand(const QList<NodeInstance> &instanceList) const;
    ReparentInstancesCommand createReparentInstancesCommand(const QList<NodeInstance> &instanceList) const;
    ReparentInstancesCommand createReparentInstancesCommand(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent) const;
    ChangeFileUrlCommand createChangeFileUrlCommand(const QUrl &fileUrl) const;
    ChangeValuesCommand createChangeValueCommand(const QList<VariantProperty> &propertyList) const;
    ChangeBindingsCommand createChangeBindingCommand(const QList<BindingProperty> &propertyList) const;
    ChangeIdsCommand createChangeIdsCommand(const QList<NodeInstance> &instanceList) const;
    RemoveInstancesCommand createRemoveInstancesCommand(const QList<ModelNode> &nodeList) const;
    RemoveInstancesCommand createRemoveInstancesCommand(const ModelNode &node) const;
    RemovePropertiesCommand createRemovePropertiesCommand(const QList<AbstractProperty> &propertyList) const;
    RemoveSharedMemoryCommand createRemoveSharedMemoryCommand(const QString &sharedMemoryTypeName, quint32 keyNumber);
    RemoveSharedMemoryCommand createRemoveSharedMemoryCommand(const QString &sharedMemoryTypeName, const QList<ModelNode> &nodeList);

    void resetHorizontalAnchors(const ModelNode &node);
    void resetVerticalAnchors(const ModelNode &node);

    void restartProcess();

private slots:
    void handleChrash();

private: //variables
    NodeInstance m_rootNodeInstance;
    NodeInstance m_activeStateInstance;

    QHash<ModelNode, NodeInstance> m_nodeInstanceHash;
    QHash<ModelNode, QImage> m_statePreviewImage;

    uint m_blockUpdates;
    QWeakPointer<NodeInstanceServerInterface> m_nodeInstanceServer;
    QImage m_baseStatePreviewImage;
    QTime m_lastCrashTime;
    NodeInstanceServerInterface::RunModus m_runModus;
    QString m_pathToQt;
};

} // namespace ProxyNodeInstanceView

#endif // NODEINSTANCEVIEW_H
