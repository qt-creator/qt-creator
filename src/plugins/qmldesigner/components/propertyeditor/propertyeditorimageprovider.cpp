// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

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
