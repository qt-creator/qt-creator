// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemlibraryimageprovider.h"

#include "itemlibrarytracing.h"

#include <utils/stylehelper.h>

namespace QmlDesigner {

using ItemLibraryTracing::category;

namespace Internal {

ItemLibraryImageProvider::ItemLibraryImageProvider() :
        QQuickImageProvider(QQuickImageProvider::Pixmap)
{
    NanotraceHR::Tracer tracer{"item library image provider constructor", category()};
}

QPixmap ItemLibraryImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    NanotraceHR::Tracer tracer{"item library image provider request pixmap", category()};

    QPixmap pixmap(Utils::StyleHelper::dpiSpecificImageFile(id));
    if (size) {
        size->setWidth(pixmap.width());
        size->setHeight(pixmap.height());
    }

    if (pixmap.isNull()) {
        pixmap = QPixmap(Utils::StyleHelper::dpiSpecificImageFile(
            QStringLiteral(":/ItemLibrary/images/item-default-icon.png")));
    }

    if (requestedSize.isValid())
        return pixmap.scaled(requestedSize);
    return pixmap;
}

}

}

