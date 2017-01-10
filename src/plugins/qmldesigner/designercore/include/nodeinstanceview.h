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

#pragma once

#include "qmldesignercorelib_global.h"
#include "abstractview.h"

#include <modelnode.h>
#include <nodeinstance.h>
#include <nodeinstanceclientinterface.h>
#include <nodeinstanceserverinterface.h>

#include <QHash>
#include <QImage>
#include <QPointer>
#include <QRectF>
#include <QTime>

QT_BEGIN_NAMESPACE
class QDeclarativeEngine;
class QGraphicsView;
class QFileSystemWatcher;
class QPainter;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Kit;
class Project;
}

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

    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;
    void nodeCreated(const ModelNode &createdNode) override;
    void nodeAboutToBeRemoved(const ModelNode &removedNode) override;
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList) override;
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        AbstractView::PropertyChangeFlags propertyChange) override;
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion) override;
    void nodeTypeChanged(const ModelNode& node, const TypeName &type, int majorVersion, int minorVersion) override;
    void fileUrlChanged(const QUrl &oldUrl, const QUrl &newUrl) override;
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId) override;
    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex) override;
    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports) override;
    void auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &data) override;
    void customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data) override;
    void nodeSourceChanged(const ModelNode &modelNode, const QString &newNodeSource) override;


    void currentStateChanged(const ModelNode &node) override;

    QList<NodeInstance> instances() const;
    NodeInstance instanceForModelNode(const ModelNode &node) const ;
    bool hasInstanceForModelNode(const ModelNode &node) const;

    NodeInstance instanceForId(qint32 id);
    bool hasInstanceForId(qint32 id);

    QRectF sceneRect() const;

    NodeInstance activeStateInstance() const;

    void updateChildren(const NodeAbstractProperty &newPropertyParent);
    void updatePosition(const QList<VariantProperty>& propertyList);

    void valuesChanged(const ValuesChangedCommand &command) override;
    void pixmapChanged(const PixmapChangedCommand &command) override;
    void informationChanged(const InformationChangedCommand &command) override;
    void childrenChanged(const ChildrenChangedCommand &command) override;
    void statePreviewImagesChanged(const StatePreviewImageChangedCommand &command) override;
    void componentCompleted(const ComponentCompletedCommand &command) override;
    void token(const TokenCommand &command) override;
    void debugOutput(const DebugOutputCommand &command) override;

    QImage statePreviewImage(const ModelNode &stateNode) const;

    void setKit(ProjectExplorer::Kit *kit);
    void setProject(ProjectExplorer::Project *project);

    void sendToken(const QString &token, int number, const QVector<ModelNode> &nodeVector);

protected:
    void timerEvent(QTimerEvent *event) override;

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
    void delayedRestartProcess();

private slots:
    void handleCrash();

private: //variables
    NodeInstance m_rootNodeInstance;
    NodeInstance m_activeStateInstance;

    QHash<ModelNode, NodeInstance> m_nodeInstanceHash;
    QHash<ModelNode, QImage> m_statePreviewImage;

    uint m_blockUpdates;
    QPointer<NodeInstanceServerInterface> m_nodeInstanceServer;
    QImage m_baseStatePreviewImage;
    QTime m_lastCrashTime;
    NodeInstanceServerInterface::RunModus m_runModus;
    ProjectExplorer::Kit *m_currentKit = nullptr;
    ProjectExplorer::Project *m_currentProject = nullptr;
    int m_restartProcessTimerId;
};

} // namespace ProxyNodeInstanceView
