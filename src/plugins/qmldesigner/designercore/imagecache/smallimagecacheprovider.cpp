// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "smallimagecacheprovider.h"

#include <asynchronousimagecache.h>

#include <QMetaObject>

namespace QmlDesigner {

QQuickTextureFactory *ImageResponse::textureFactory() const
{
    return QQuickTextureFactory::textureFactoryForImage(m_image);
}

void ImageResponse::setImage(const QImage &image)
{
    m_image = image;

    emit finished();
}

void ImageResponse::abort()
{
    emit finished();
}

QQuickImageResponse *SmallImageCacheProvider::requestImageResponse(const QString &id, const QSize &)
{
    auto response = std::make_unique<QmlDesigner::ImageResponse>(m_defaultImage);

    m_cache.requestSmallImage(
        id,
        [response = QPointer<QmlDesigner::ImageResponse>(response.get())](const QImage &image) {
            QMetaObject::invokeMethod(
                response,
                [response, image] {
                    if (response)
                        response->setImage(image);
                },
                Qt::QueuedConnection);
        },
        [response = QPointer<QmlDesigner::ImageResponse>(response.get())](
            ImageCache::AbortReason abortReason) {
            QMetaObject::invokeMethod(
                response,
                [response, abortReason] {
                    switch (abortReason) {
                    case ImageCache::AbortReason::Failed:
                        if (response)
                            response->abort();
                        break;
                    case ImageCache::AbortReason::Abort:
                        response->cancel();
                        break;
                    }
                },
                Qt::QueuedConnection);
        });

    return response.release();
}

} // namespace QmlDesigner
