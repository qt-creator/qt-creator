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

#include "qt5capturepreviewnodeinstanceserver.h"
#include "servernodeinstance.h"

#include <captureddatacommand.h>
#include <createscenecommand.h>
#include <nodeinstanceclientinterface.h>

#include <QImage>
#include <QQuickView>

namespace QmlDesigner {

namespace {

QImage renderPreviewImage(ServerNodeInstance rootNodeInstance)
{
    rootNodeInstance.updateDirtyNodeRecursive();

    QSize previewImageSize = rootNodeInstance.boundingRect().size().toSize();

    QImage previewImage = rootNodeInstance.renderPreviewImage(previewImageSize);

    return previewImage;
}

CapturedDataCommand::StateData collectStateData(ServerNodeInstance rootNodeInstance,
                                                const QVector<ServerNodeInstance> &nodeInstances,
                                                qint32 stateInstanceId)
{
    CapturedDataCommand::StateData stateData;
    stateData.image = ImageContainer(stateInstanceId,
                                     QmlDesigner::renderPreviewImage(rootNodeInstance),
                                     stateInstanceId);
    stateData.nodeId = stateInstanceId;

    for (const ServerNodeInstance &instance : nodeInstances) {
        CapturedDataCommand::NodeData nodeData;

        nodeData.nodeId = instance.instanceId();
        nodeData.contentRect = instance.boundingRect();
        nodeData.sceneTransform = instance.sceneTransform();
        auto textProperty = instance.property("text");
        if (!textProperty.isNull() && instance.holdsGraphical())
            nodeData.properties.emplace_back(QString{"text"}, textProperty);
        auto colorProperty = instance.property("color");
        if (!colorProperty.isNull())
            nodeData.properties.emplace_back(QString{"color"}, colorProperty);

        auto visibleProperty = instance.property("visible");
        if (!colorProperty.isNull())
            nodeData.properties.emplace_back(QString{"visible"}, visibleProperty);

        stateData.nodeData.push_back(std::move(nodeData));
    }

    return stateData;
}
} // namespace

void Qt5CapturePreviewNodeInstanceServer::collectItemChangesAndSendChangeCommands()
{
    static bool inFunction = false;

    if (!rootNodeInstance().holdsGraphical())
        return;

    if (!inFunction) {
        inFunction = true;

        DesignerSupport::polishItems(quickWindow());

        QVector<CapturedDataCommand::StateData> stateDatas;
        stateDatas.push_back(collectStateData(rootNodeInstance(), nodeInstances(), 0));

        for (ServerNodeInstance stateInstance : rootNodeInstance().stateInstances()) {
            stateInstance.activateState();
            stateDatas.push_back(
                collectStateData(rootNodeInstance(), nodeInstances(), stateInstance.instanceId()));
            stateInstance.deactivateState();
        }

        nodeInstanceClient()->capturedData(CapturedDataCommand{std::move(stateDatas)});

        slowDownRenderTimer();
        inFunction = false;
    }
}

} // namespace QmlDesigner
