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

#include "qt5nodeinstanceserver.h"
#include "tokencommand.h"
#include "valueschangedcommand.h"
#include "changeselectioncommand.h"

#include <QTimer>
#include <QVariant>

QT_BEGIN_NAMESPACE
class QDragMoveEvent;
QT_END_NAMESPACE

namespace QmlDesigner {

class Qt5InformationNodeInstanceServer : public Qt5NodeInstanceServer
{
    Q_OBJECT

public:
    explicit Qt5InformationNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient);

    void reparentInstances(const ReparentInstancesCommand &command) override;
    void clearScene(const ClearSceneCommand &command) override;
    void update3DViewState(const Update3dViewStateCommand &command) override;
    void enable3DView(const Enable3DViewCommand &command) override;
    void createScene(const CreateSceneCommand &command) override;
    void completeComponent(const CompleteComponentCommand &command) override;
    void token(const TokenCommand &command) override;
    void removeSharedMemory(const RemoveSharedMemoryCommand &command) override;
    void changeSelection(const ChangeSelectionCommand &command) override;
    void changePropertyValues(const ChangeValuesCommand &command) override;

private slots:
    void handleSelectionChanged(const QVariant &objs);
    void handleObjectPropertyCommit(const QVariant &object, const QVariant &propName);
    void handleObjectPropertyChange(const QVariant &object, const QVariant &propName);
    void handleToolStateChanged(const QString &tool, const QVariant &toolState);
    void handleView3DSizeChange();

protected:
    void collectItemChangesAndSendChangeCommands() override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void sendChildrenChangedCommand(const QList<ServerNodeInstance> &childList);
    void sendTokenBack();
    bool isDirtyRecursiveForNonInstanceItems(QQuickItem *item) const;
    bool isDirtyRecursiveForParentInstances(QQuickItem *item) const;
    void selectInstances(const QList<ServerNodeInstance> &instanceList);
    void modifyProperties(const QVector<InstancePropertyValueTriple> &properties);
    QList<ServerNodeInstance> createInstances(const QVector<InstanceContainer> &container) override;

private:
    void handleObjectPropertyChangeTimeout();
    void handleSelectionChangeTimeout();
    QObject *createEditView3D(QQmlEngine *engine);
    void setup3DEditView(const QList<ServerNodeInstance> &instanceList,
                         const QVariantMap &toolStates);
    void createCameraAndLightGizmos(const QList<ServerNodeInstance> &instanceList) const;
    void addViewPorts(const QList<ServerNodeInstance> &instanceList);
    ServerNodeInstance findView3DForInstance(const ServerNodeInstance &instance) const;
    QObject *find3DSceneRoot(const ServerNodeInstance &instance) const;
    QVector<InstancePropertyValueTriple> vectorToPropertyValue(const ServerNodeInstance &instance,
                                                               const PropertyName &propertyName,
                                                               const QVariant &variant);
    void modifyVariantValue(const QVariant &node,
                            const PropertyName &propertyName,
                            ValuesModifiedCommand::TransactionOption option);
    bool dropAcceptable(QDragMoveEvent *event) const;
    void updateView3DRect(QObject *view3D);
    void updateActiveSceneToEditView3D();

    QObject *m_editView3D = nullptr;
    QVector<ServerNodeInstance> m_view3Ds;
    ServerNodeInstance m_active3DView;
    QObject *m_active3DScene = nullptr;
    QSet<ServerNodeInstance> m_parentChangedSet;
    QList<ServerNodeInstance> m_completedComponentList;
    QList<TokenCommand> m_tokenList;
    QTimer m_propertyChangeTimer;
    QTimer m_selectionChangeTimer;
    QVariant m_changedNode;
    PropertyName m_changedProperty;
    ChangeSelectionCommand m_pendingSelectionChangeCommand;
};

} // namespace QmlDesigner
