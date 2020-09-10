/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <nodeinstanceserver.h>

#include <memory>
#include <vector>

namespace QmlDesigner {

class NodeInstanceServerDispatcher : public NodeInstanceServerInterface
{
public:
    NodeInstanceServerDispatcher(const QStringList &serverNames,
                                 NodeInstanceClientInterface *nodeInstanceClient);

    void createInstances(const CreateInstancesCommand &command);
    void changeFileUrl(const ChangeFileUrlCommand &command);
    void createScene(const CreateSceneCommand &command);
    void clearScene(const ClearSceneCommand &command);
    void update3DViewState(const Update3dViewStateCommand &command);
    void removeInstances(const RemoveInstancesCommand &command);
    void removeProperties(const RemovePropertiesCommand &command);
    void changePropertyBindings(const ChangeBindingsCommand &command);
    void changePropertyValues(const ChangeValuesCommand &command);
    void changeAuxiliaryValues(const ChangeAuxiliaryCommand &command);
    void reparentInstances(const ReparentInstancesCommand &command);
    void changeIds(const ChangeIdsCommand &command);
    void changeState(const ChangeStateCommand &command);
    void completeComponent(const CompleteComponentCommand &command);
    void changeNodeSource(const ChangeNodeSourceCommand &command);
    void token(const TokenCommand &command);
    void removeSharedMemory(const RemoveSharedMemoryCommand &command);
    void changeSelection(const ChangeSelectionCommand &command);
    void inputEvent(const InputEventCommand &command);
    void view3DAction(const View3DActionCommand &command);
    void requestModelNodePreviewImage(const RequestModelNodePreviewImageCommand &command);
    void changeLanguage(const ChangeLanguageCommand &command);
    void changePreviewImageSize(const ChangePreviewImageSizeCommand &command);

private:
    void addServer(const QString &serverName, NodeInstanceClientInterface *nodeInstanceClient);

protected:
    std::vector<std::unique_ptr<NodeInstanceServer>> m_servers;
};

} // namespace QmlDesigner
