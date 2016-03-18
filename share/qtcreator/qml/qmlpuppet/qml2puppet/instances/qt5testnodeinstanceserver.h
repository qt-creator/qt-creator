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

namespace QmlDesigner {

class Qt5TestNodeInstanceServer : public Qt5NodeInstanceServer
{
public:
    Qt5TestNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient);


    void createInstances(const CreateInstancesCommand &command) override;
    void changeFileUrl(const ChangeFileUrlCommand &command) override;
    void changePropertyValues(const ChangeValuesCommand &command) override;
    void changePropertyBindings(const ChangeBindingsCommand &command) override;
    void changeAuxiliaryValues(const ChangeAuxiliaryCommand &command) override;
    void changeIds(const ChangeIdsCommand &command) override;
    void createScene(const CreateSceneCommand &command) override;
    void clearScene(const ClearSceneCommand &command) override;
    void removeInstances(const RemoveInstancesCommand &command) override;
    void removeProperties(const RemovePropertiesCommand &command) override;
    void reparentInstances(const ReparentInstancesCommand &command) override;
    void changeState(const ChangeStateCommand &command) override;
    void completeComponent(const CompleteComponentCommand &command) override;
    void changeNodeSource(const ChangeNodeSourceCommand &command) override;
    void removeSharedMemory(const RemoveSharedMemoryCommand &command) override;

    using Qt5NodeInstanceServer::createInstances;

protected:
    void collectItemChangesAndSendChangeCommands() override;
    void sendChildrenChangedCommand(const QList<ServerNodeInstance> &childList);
    bool isDirtyRecursiveForNonInstanceItems(QQuickItem *item) const;
};

} // namespace QmlDesigner
