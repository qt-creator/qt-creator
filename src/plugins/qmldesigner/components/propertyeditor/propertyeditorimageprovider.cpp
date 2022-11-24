// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "propertyeditorimageprovider.h"
#include "assetslibrarymodel.h"

#include <projectexplorer/target.h>
#include <utils/hdrimage.h>
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

    auto response = std::make_unique<QmlDesigner::ImageResponse>(m_smallImageCacheProvider.defaultImage());

    QMetaObject::invokeMethod(
        response.get(),
        [response = QPointer<QmlDesigner::ImageResponse>(response.get()), suffix, id, requestedSize] {
            if (AssetsLibraryModel::supportedImageSuffixes().contains(suffix)) {
                QImage image = QImage(Utils::StyleHelper::dpiSpecificImageFile(id));
                if (!image.isNull()) {
                    response->setImage(image.scaled(requestedSize, Qt::KeepAspectRatio));
                    return;
                }
            } else if (AssetsLibraryModel::supportedTexture3DSuffixes().contains(suffix)) {
                HdrImage hdr{id};
                if (!hdr.image().isNull()) {
                    response->setImage(hdr.image().scaled(requestedSize, Qt::KeepAspectRatio));
                    return;
                }
            }
            response->setImage(response->image().scaled(requestedSize, Qt::KeepAspectRatio));
        },
        Qt::QueuedConnection);

    return response.release();
}

} // namespace QmlDesigner
