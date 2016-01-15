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

#include "qt5previewnodeinstanceserver.h"

#include "nodeinstanceclientinterface.h"
#include "statepreviewimagechangedcommand.h"
#include "createscenecommand.h"
#include "removesharedmemorycommand.h"
#include <QQuickView>
#include <QQuickItem>
#include <designersupportdelegate.h>

namespace QmlDesigner {

Qt5PreviewNodeInstanceServer::Qt5PreviewNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient) :
    Qt5NodeInstanceServer(nodeInstanceClient)
{
    setSlowRenderTimerInterval(100000000);
    setRenderTimerInterval(100);
}

void Qt5PreviewNodeInstanceServer::createScene(const CreateSceneCommand &command)
{
    initializeView();
    setupScene(command);
    startRenderTimer();
}
void Qt5PreviewNodeInstanceServer::startRenderTimer()
{
    if (timerId() != 0)
        killTimer(timerId());

    int timerId = startTimer(renderTimerInterval());

    setTimerId(timerId);
}

void Qt5PreviewNodeInstanceServer::collectItemChangesAndSendChangeCommands()
{
    static bool inFunction = false;

    if (!rootNodeInstance().holdsGraphical())
        return;

    if (!inFunction && nodeInstanceClient()->bytesToWrite() < 10000) {
        inFunction = true;

        DesignerSupport::polishItems(quickView());

        QVector<ImageContainer> imageContainerVector;
        imageContainerVector.append(ImageContainer(0, renderPreviewImage(), -1));

        foreach (ServerNodeInstance instance,  rootNodeInstance().stateInstances()) {
            instance.activateState();
            QImage previewImage = renderPreviewImage();
            if (!previewImage.isNull())
                imageContainerVector.append(ImageContainer(instance.instanceId(), renderPreviewImage(), instance.instanceId()));
            instance.deactivateState();
        }

        nodeInstanceClient()->statePreviewImagesChanged(StatePreviewImageChangedCommand(imageContainerVector));

        slowDownRenderTimer();
        inFunction = false;
    }
}

void Qt5PreviewNodeInstanceServer::changeState(const ChangeStateCommand &/*command*/)
{

}

QImage Qt5PreviewNodeInstanceServer::renderPreviewImage()
{
    rootNodeInstance().updateDirtyNodeRecursive();

    QRectF boundingRect = rootNodeInstance().boundingRect();

    QSize previewImageSize = boundingRect.size().toSize();
    previewImageSize.scale(QSize(100, 100), Qt::KeepAspectRatio);

    QImage previewImage = rootNodeInstance().renderPreviewImage(previewImageSize);

    return previewImage;
}

void QmlDesigner::Qt5PreviewNodeInstanceServer::removeSharedMemory(const QmlDesigner::RemoveSharedMemoryCommand &command)
{
    if (command.typeName() == "Image")
        ImageContainer::removeSharedMemorys(command.keyNumbers());
}

} // namespace QmlDesigner
