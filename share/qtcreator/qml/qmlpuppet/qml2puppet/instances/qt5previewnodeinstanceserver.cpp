/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "qt5previewnodeinstanceserver.h"

#include "nodeinstanceclientinterface.h"
#include "statepreviewimagechangedcommand.h"
#include "createscenecommand.h"

#include <QSGItem>
#include <designersupport.h>

namespace QmlDesigner {

Qt5PreviewNodeInstanceServer::Qt5PreviewNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient) :
    Qt5NodeInstanceServer(nodeInstanceClient)
{
}

void Qt5PreviewNodeInstanceServer::createScene(const CreateSceneCommand &command)
{
    initializeView(command.imports());
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

    if (!inFunction && nodeInstanceClient()->bytesToWrite() < 10000) {
        inFunction = true;
        QVector<ImageContainer> imageContainerVector;
        imageContainerVector.append(ImageContainer(0, renderPreviewImage()));

        foreach (ServerNodeInstance instance,  rootNodeInstance().stateInstances()) {
            instance.activateState();
            imageContainerVector.append(ImageContainer(instance.instanceId(), renderPreviewImage()));
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

static void updateDirtyNodeRecursive(QSGItem *parentItem)
{
    foreach (QSGItem *childItem, parentItem->childItems())
        updateDirtyNodeRecursive(childItem);

    DesignerSupport::updateDirtyNode(parentItem);
}

QImage Qt5PreviewNodeInstanceServer::renderPreviewImage()
{
    updateDirtyNodeRecursive(rootNodeInstance().internalSGItem());

    QRectF boundingRect = rootNodeInstance().boundingRect();

    QSize previewImageSize = boundingRect.size().toSize();
    previewImageSize.scale(QSize(100, 100), Qt::KeepAspectRatio);

    QImage previewImage;

    if (boundingRect.isValid() && rootNodeInstance().internalSGItem())
        previewImage = designerSupport()->renderImageForItem(rootNodeInstance().internalSGItem(), boundingRect, previewImageSize);

    previewImage = previewImage.convertToFormat(QImage::Format_ARGB32_Premultiplied);

    return previewImage;
}

void QmlDesigner::Qt5PreviewNodeInstanceServer::removeSharedMemory(const QmlDesigner::RemoveSharedMemoryCommand &command)
{
    if (command.typeName() == "Image")
        ImageContainer::removeSharedMemory(command.keyNumber());
}

} // namespace QmlDesigner
