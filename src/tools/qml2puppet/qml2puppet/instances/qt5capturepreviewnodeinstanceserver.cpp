// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qt5capturepreviewnodeinstanceserver.h"
#include "servernodeinstance.h"

#include <captureddatacommand.h>
#include <createscenecommand.h>
#include <nodeinstanceclientinterface.h>

#include <QImage>
#include <QQuickView>

#include <private/qquickdesignersupport_p.h>

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

        QQuickDesignerSupport::polishItems(quickWindow());

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
