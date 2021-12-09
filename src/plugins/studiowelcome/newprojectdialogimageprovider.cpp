/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

