// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "nodegrapheditorimageprovider.h"

namespace QmlDesigner::Internal {

NodeGraphEditorImageProvider::NodeGraphEditorImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage NodeGraphEditorImageProvider::requestImage(const QString &/*id*/, QSize */*size*/, const QSize &requestedSize)
{
    const QSize newSize = requestedSize.isEmpty()
                            ? QSize(100, 100)
                            : requestedSize;
    QImage image(newSize, QImage::Format_ARGB32);
    image.fill(0xFFFFFFFF);
    return image;
}

} // namespace QmlDesigner::Internal
