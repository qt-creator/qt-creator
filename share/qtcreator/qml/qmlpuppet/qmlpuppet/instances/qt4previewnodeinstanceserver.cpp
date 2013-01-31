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

#include "qt4previewnodeinstanceserver.h"

#include "nodeinstanceclientinterface.h"
#include "statepreviewimagechangedcommand.h"
#include "createscenecommand.h"
#include "removesharedmemorycommand.h"

#include <QPainter>
#include <QDeclarativeView>

namespace QmlDesigner {

Qt4PreviewNodeInstanceServer::Qt4PreviewNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient) :
    Qt4NodeInstanceServer(nodeInstanceClient)
{
    setRenderTimerInterval(200);
    setSlowRenderTimerInterval(2000);
}

void Qt4PreviewNodeInstanceServer::createScene(const CreateSceneCommand &command)
{
    initializeView(command.imports());
    setupScene(command);

    startRenderTimer();
}
void Qt4PreviewNodeInstanceServer::startRenderTimer()
{
    if (timerId() != 0)
        killTimer(timerId());

    int timerId = startTimer(renderTimerInterval());

    setTimerId(timerId);
}

void Qt4PreviewNodeInstanceServer::collectItemChangesAndSendChangeCommands()
{
    static bool inFunction = false;

    if (!inFunction && nodeInstanceClient()->bytesToWrite() < 10000) {
        inFunction = true;
        QVector<ImageContainer> imageContainerVector;
        imageContainerVector.append(ImageContainer(0, renderPreviewImage(), -1));

        foreach (ServerNodeInstance instance,  rootNodeInstance().stateInstances()) {
            instance.activateState();
            imageContainerVector.append(ImageContainer(instance.instanceId(), renderPreviewImage(), instance.instanceId()));
            instance.deactivateState();
        }

        nodeInstanceClient()->statePreviewImagesChanged(StatePreviewImageChangedCommand(imageContainerVector));

        slowDownRenderTimer();
        inFunction = false;
    }
}

void Qt4PreviewNodeInstanceServer::changeState(const ChangeStateCommand &/*command*/)
{

}

void Qt4PreviewNodeInstanceServer::removeSharedMemory(const RemoveSharedMemoryCommand &command)
{
    if (command.typeName() == "Image")
        ImageContainer::removeSharedMemorys(command.keyNumbers());
}

QImage Qt4PreviewNodeInstanceServer::renderPreviewImage()
{
    QSize size = rootNodeInstance().boundingRect().size().toSize();
    size.scale(100, 100, Qt::KeepAspectRatio);

    QImage image(size, QImage::Format_ARGB32);
    image.fill(0xFFFFFFFF);

    if (declarativeView()) {
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
        painter.setRenderHint(QPainter::NonCosmeticDefaultPen, true);

        declarativeView()->scene()->render(&painter, image.rect(), rootNodeInstance().boundingRect().toRect(), Qt::IgnoreAspectRatio);
    }

    return image;
}
} // namespace QmlDesigner
