// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "newprojectdialogimageprovider.h"

#include <coreplugin/icore.h>
#include <utils/utilsicons.h>
#include <utils/stylehelper.h>

namespace StudioWelcome {

namespace Internal {

NewProjectDialogImageProvider::NewProjectDialogImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap)
{}

QPixmap NewProjectDialogImageProvider::invalidStyleIcon()
{
    QString iconPath = Core::ICore::resourcePath("qmldesigner/newprojectdialog/image/style-error.png").toString();
    QString file = Utils::StyleHelper::dpiSpecificImageFile(iconPath);
    return QPixmap{file};
}

QPixmap NewProjectDialogImageProvider::requestStatusPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(size)

    QPixmap pixmap;

    if (id == "status-warning") {
        static const QPixmap warning = Utils::Icons::WARNING.pixmap();
        pixmap = warning;
    } else if (id == "status-error") {
        static const QPixmap error = Utils::Icons::CRITICAL.pixmap();
        pixmap = error;
    }

    if (requestedSize.isValid())
        return pixmap.scaled(requestedSize);

    return pixmap;
}

QPixmap NewProjectDialogImageProvider::requestStylePixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    QString realPath = Core::ICore::resourcePath("qmldesigner/newprojectdialog/image/" + id).toString();

    QPixmap pixmap{realPath};

    if (size) {
        size->setWidth(pixmap.width());
        size->setHeight(pixmap.height());
    }

    if (pixmap.isNull())
        pixmap = invalidStyleIcon();

    if (requestedSize.isValid())
        return pixmap.scaled(requestedSize);

    return pixmap;
}

QPixmap NewProjectDialogImageProvider::requestDefaultPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    QString realPath = Core::ICore::resourcePath("qmldesigner/newprojectdialog/image/" + id).toString();

    QPixmap pixmap{realPath};

    if (size) {
        size->setWidth(pixmap.width());
        size->setHeight(pixmap.height());
    }

    if (pixmap.isNull())
        return {};

    if (requestedSize.isValid())
        return pixmap.scaled(requestedSize);

    return pixmap;
}

QPixmap NewProjectDialogImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    if (id.startsWith("style-"))
        return requestStylePixmap(id, size, requestedSize);

    if (id.startsWith("status-"))
        return requestStatusPixmap(id, size, requestedSize);

    return requestDefaultPixmap(id, size, requestedSize);
}

} // namespace Internal

} // namespace StudioWelcome

