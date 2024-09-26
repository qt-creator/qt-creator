// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "stateseditorimageprovider.h"
#include "nodeinstanceview.h"

#include <QDebug>

namespace QmlDesigner {
namespace Internal {

StatesEditorImageProvider::StatesEditorImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage StatesEditorImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QImage image;

    if (auto view = static_cast<const NodeInstanceView *>(m_nodeInstanceView.data())) {
        QString imageId = id.split(QLatin1Char('-')).constFirst();
        if (imageId == QLatin1String("baseState")) {
            image = view->statePreviewImage(m_nodeInstanceView->rootModelNode());
        } else {
            bool canBeConverted;
            int instanceId = imageId.toInt(&canBeConverted);
            if (canBeConverted && m_nodeInstanceView->hasModelNodeForInternalId(instanceId)) {
                image = view->statePreviewImage(m_nodeInstanceView->modelNodeForInternalId(instanceId));
            }
        }
    }

    if (image.isNull()) {
        //creating white QImage
        QSize newSize = requestedSize;
        if (newSize.isEmpty())
            newSize = QSize(100, 100);

        QImage image(newSize, QImage::Format_ARGB32);
        image.fill(0xFFFFFFFF);
        return image;
    }

    *size = image.size();

    return image;
}

void StatesEditorImageProvider::setNodeInstanceView(const AbstractView *nodeInstanceView)
{
    m_nodeInstanceView = static_cast<const NodeInstanceView *>(nodeInstanceView);
}

} // namespace Internal
} // namespace QmlDesigner
