// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qt5previewnodeinstanceserver.h"

#include "changepreviewimagesizecommand.h"
#include "createscenecommand.h"
#include "nodeinstanceclientinterface.h"
#include "removesharedmemorycommand.h"
#include "statepreviewimagechangedcommand.h"

#include <QQuickView>
#include <QQuickItem>
#include <private/qquickdesignersupport_p.h>

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
    registerFonts(command.resourceUrl);
    setTranslationLanguage(command.language);
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

        QQuickDesignerSupport::polishItems(quickWindow());

        QVector<ImageContainer> imageContainerVector;

        // Base state needs to be rendered twice to properly render shared resources,
        // if there is more than one View3D and at least one of them is dirty.
        bool dirtyView3d = false;
        const QList<ServerNodeInstance> view3dInstances = allView3DInstances();
        for (const auto &instance : view3dInstances) {
            if (QQuickDesignerSupport::isDirty(instance.rootQuickItem(),
                                               QQuickDesignerSupport::ContentUpdateMask)) {
                dirtyView3d = true;
                break;
            }
        }
        if (dirtyView3d)
            renderPreviewImage();
        imageContainerVector.append(ImageContainer(0, renderPreviewImage(), -1));

        QList<ServerNodeInstance> stateInstances = rootNodeInstance().stateInstances();

        const QList<ServerNodeInstance> groupInstances = allGroupStateInstances();

        for (const ServerNodeInstance &instance : groupInstances)
            stateInstances.append(instance.stateInstances());

        for (ServerNodeInstance instance : std::as_const(stateInstances)) {
            instance.activateState();
            QImage previewImage = renderPreviewImage();
            if (!previewImage.isNull())
                imageContainerVector.append(ImageContainer(instance.instanceId(),
                                                           renderPreviewImage(),
                                                           instance.instanceId()));
            instance.deactivateState();
        }

        nodeInstanceClient()->statePreviewImagesChanged(
                    StatePreviewImageChangedCommand(imageContainerVector));

        slowDownRenderTimer();
        handleExtraRender();
        inFunction = false;
    }
}

void Qt5PreviewNodeInstanceServer::changeState(const ChangeStateCommand &/*command*/)
{

}

QImage Qt5PreviewNodeInstanceServer::renderPreviewImage()
{
    // Ensure the state preview image is always clipped properly to root item dimensions
    if (auto rootItem = qobject_cast<QQuickItem *>(rootNodeInstance().internalObject()))
        rootItem->setClip(true);

    rootNodeInstance().updateDirtyNodeRecursive();

    QRectF boundingRect = rootNodeInstance().boundingRect();

    QSize previewImageSize = boundingRect.size().toSize();

    if (m_previewSize.isValid() && !m_previewSize.isNull())
        previewImageSize.scale(m_previewSize, Qt::KeepAspectRatio);

    QImage previewImage = rootNodeInstance().renderPreviewImage(previewImageSize);

    return previewImage;
}

void QmlDesigner::Qt5PreviewNodeInstanceServer::removeSharedMemory(const QmlDesigner::RemoveSharedMemoryCommand &command)
{
    if (command.typeName() == "Image")
        ImageContainer::removeSharedMemorys(command.keyNumbers());
}

void Qt5PreviewNodeInstanceServer::changePreviewImageSize(
    const ChangePreviewImageSizeCommand &command)
{
    m_previewSize = command.size;

    collectItemChangesAndSendChangeCommands();
}

bool Qt5PreviewNodeInstanceServer::isPreviewServer() const
{
    return true;
}

} // namespace QmlDesigner
