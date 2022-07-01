/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "propertyeditorimageprovider.h"
#include "assetslibrarymodel.h"

#include <hdrimage.h>
#include <projectexplorer/target.h>
#include <utils/stylehelper.h>

#include <QMetaObject>
#include <QQuickImageResponse>

namespace QmlDesigner {

QQuickImageResponse *PropertyEditorImageProvider::requestImageResponse(const QString &id,
                                                                       const QSize &requestedSize)
{
    const QString suffix = "*." + id.split('.').last().toLower();

    if (suffix == "*.mesh")
        return m_smallImageCacheProvider.requestImageResponse(id, requestedSize);

    if (suffix == "*.builtin")
        return m_smallImageCacheProvider.requestImageResponse("#" + id.split('.').first(),
                                                              requestedSize);

    QImage image;
    auto response = std::make_unique<QmlDesigner::ImageResponse>(image);

    QMetaObject::invokeMethod(
        response.get(),
        [response = QPointer<QmlDesigner::ImageResponse>(response.get()), image, suffix, id] {
            if (AssetsLibraryModel::supportedImageSuffixes().contains(suffix))
                response->setImage(QImage(Utils::StyleHelper::dpiSpecificImageFile(id)));
            else if (AssetsLibraryModel::supportedTexture3DSuffixes().contains(suffix))
                response->setImage(HdrImage{id}.image());
            else
                response->abort();
        },
        Qt::QueuedConnection);

    return response.release();
}

} // namespace QmlDesigner
