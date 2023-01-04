// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qt5captureimagenodeinstanceserver.h"
#include "servernodeinstance.h"

#include <captureddatacommand.h>
#include <createscenecommand.h>
#include <nodeinstanceclientinterface.h>

#include <QImage>
#include <QQuickItem>
#include <QQuickView>

#include <private/qquickdesignersupport_p.h>

namespace QmlDesigner {

namespace {

QImage renderImage(ServerNodeInstance rootNodeInstance, QSize minimumSize, QSize maximumSize)
{
    rootNodeInstance.updateDirtyNodeRecursive();

    QSize previewImageSize = rootNodeInstance.boundingRect().size().toSize();
    if (previewImageSize.isEmpty()) {
        previewImageSize = minimumSize;
    } else if (previewImageSize.width() < minimumSize.width()
               || previewImageSize.height() < minimumSize.height()) {
        previewImageSize.scale(minimumSize, Qt::KeepAspectRatio);
    }

    if (previewImageSize.width() > maximumSize.width()
        || previewImageSize.height() > maximumSize.height()) {
        previewImageSize.scale(maximumSize, Qt::KeepAspectRatio);
    }

    QImage previewImage = rootNodeInstance.renderPreviewImage(previewImageSize);

    return previewImage;
}
} // namespace

void Qt5CaptureImageNodeInstanceServer::collectItemChangesAndSendChangeCommands()
{
    static bool inFunction = false;

    if (!rootNodeInstance().holdsGraphical()) {
        nodeInstanceClient()->capturedData(CapturedDataCommand{});
        return;
    }

    if (!inFunction) {
        inFunction = true;

        auto rooNodeInstance = rootNodeInstance();
        if (QQuickItem *qitem = rooNodeInstance.rootQuickItem())
            qitem->setClip(true);

        QQuickDesignerSupport::polishItems(quickWindow());

        QImage image = renderImage(rooNodeInstance, m_minimumSize, m_maximumSize);

        nodeInstanceClient()->capturedData(CapturedDataCommand{std::move(image)});

        slowDownRenderTimer();
        inFunction = false;
    }
}

void QmlDesigner::Qt5CaptureImageNodeInstanceServer::createScene(const CreateSceneCommand &command)
{
    m_minimumSize = command.captureImageMinimumSize;
    m_maximumSize = command.captureImageMaximumSize;

    Qt5PreviewNodeInstanceServer::createScene(command);
}

} // namespace
