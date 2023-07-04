// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentlibraryiconprovider.h"

#include <coreplugin/icore.h>

namespace QmlDesigner {

namespace Internal {

ContentLibraryIconProvider::ContentLibraryIconProvider()
    : QQuickImageProvider(Pixmap)
{

}

QPixmap ContentLibraryIconProvider::requestPixmap(const QString &id,
                                                  QSize *size,
                                                  [[maybe_unused]] const QSize &requestedSize)
{
    QString realPath = Core::ICore::resourcePath("qmldesigner/contentLibraryImages/" + id).toString();

    QPixmap pixmap{realPath};

    if (size) {
        size->setWidth(pixmap.width());
        size->setHeight(pixmap.height());
    }

    if (pixmap.isNull())
        return pixmap;

    if (requestedSize.isValid())
        return pixmap.scaled(requestedSize);

    return pixmap;
}

} // namespace Internal

} // namespace QmlDesigner
