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

#include "nodeinstanceserverdispatcher.h"

#include "qt5captureimagenodeinstanceserver.h"
#include "qt5capturepreviewnodeinstanceserver.h"
#include "qt5informationnodeinstanceserver.h"
#include "qt5rendernodeinstanceserver.h"

namespace QmlDesigner {

NodeInstanceServerDispatcher::NodeInstanceServerDispatcher(const QStringList &serverNames,
                                                           NodeInstanceClientInterface *nodeInstanceClient)
{
    for (const QString &serverName : serverNames)
        addServer(serverName, nodeInstanceClient);
}

void NodeInstanceServerDispatcher::createInstances(const CreateInstancesCommand &command)
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->createInstances(command);
}

void NodeInstanceServerDispatcher::changeFileUrl(const ChangeFileUrlCommand &command)
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->changeFileUrl(command);
}

void NodeInstanceServerDispatcher::createScene(const CreateSceneCommand &command)
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->createScene(command);
}

void NodeInstanceServerDispatcher::clearScene(const ClearSceneCommand &command)
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->clearScene(command);
}

void NodeInstanceServerDispatcher::update3DViewState(const Update3dViewStateCommand &command)
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->update3DViewState(command);
}

void NodeInstanceServerDispatcher::removeInstances(const RemoveInstancesCommand &command)
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->removeInstances(command);
}

void NodeInstanceServerDispatcher::removeProperties(const RemovePropertiesCommand &command)
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->removeProperties(command);
}

void NodeInstanceServerDispatcher::changePropertyBindings(const ChangeBindingsCommand &command)
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->changePropertyBindings(command);
}

void NodeInstanceServerDispatcher::changePropertyValues(const ChangeValuesCommand &command)
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->changePropertyValues(command);
}

void NodeInstanceServerDispatcher::changeAuxiliaryValues(const ChangeAuxiliaryCommand &command)
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->changeAuxiliaryValues(command);
}

void NodeInstanceServerDispatcher::reparentInstances(const ReparentInstancesCommand &command)
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->reparentInstances(command);
}

void NodeInstanceServerDispatcher::changeIds(const ChangeIdsCommand &command)
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->changeIds(command);
}

void NodeInstanceServerDispatcher::changeState(const ChangeStateCommand &command)
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->changeState(command);
}

void NodeInstanceServerDispatcher::completeComponent(const CompleteComponentCommand &command)
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->completeComponent(command);
}

void NodeInstanceServerDispatcher::changeNodeSource(const ChangeNodeSourceCommand &command)
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->changeNodeSource(command);
}

void NodeInstanceServerDispatcher::token(const TokenCommand &command)
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->token(command);
}

void NodeInstanceServerDispatcher::removeSharedMemory(const RemoveSharedMemoryCommand &command)
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->removeSharedMemory(command);
}

void NodeInstanceServerDispatcher::changeSelection(const ChangeSelectionCommand &command)
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->changeSelection(command);
}

void NodeInstanceServerDispatcher::inputEvent(const InputEventCommand &command)
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->inputEvent(command);
}

void NodeInstanceServerDispatcher::view3DAction(const View3DActionCommand &command)
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->view3DAction(command);
}

void NodeInstanceServerDispatcher::requestModelNodePreviewImage(const RequestModelNodePreviewImageCommand &command)
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->requestModelNodePreviewImage(command);
}

void NodeInstanceServerDispatcher::changeLanguage(const ChangeLanguageCommand &command)
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->changeLanguage(command);
}

void NodeInstanceServerDispatcher::changePreviewImageSize(const ChangePreviewImageSizeCommand &command)
{
    for (std::unique_ptr<NodeInstanceServer> &server : m_servers)
        server->changePreviewImageSize(command);
}

namespace {

std::unique_ptr<NodeInstanceServer> createNodeInstanceServer(
    const QString &serverName, NodeInstanceClientInterface *nodeInstanceClient)
{
    if (serverName == "capturemode")
        return std::make_unique<Qt5CapturePreviewNodeInstanceServer>(nodeInstanceClient);
    else if (serverName == "captureiconmode")
        return std::make_unique<Qt5CaptureImageNodeInstanceServer>(nodeInstanceClient);
    else if (serverName == "rendermode")
        return std::make_unique<Qt5RenderNodeInstanceServer>(nodeInstanceClient);
    else if (serverName == "editormode")
        return std::make_unique<Qt5InformationNodeInstanceServer>(nodeInstanceClient);
    else if (serverName == "previewmode")
        return std::make_unique<Qt5PreviewNodeInstanceServer>(nodeInstanceClient);

    return {};
}

} // namespace

void NodeInstanceServerDispatcher::addServer(const QString &serverName,
                                             NodeInstanceClientInterface *nodeInstanceClient)
{
    auto server = createNodeInstanceServer(serverName, nodeInstanceClient);

    server->disableTimer();

    m_servers.push_back(std::move(server));
}

} // namespace QmlDesigner
