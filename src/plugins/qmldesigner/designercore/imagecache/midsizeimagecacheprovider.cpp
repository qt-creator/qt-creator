// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "midsizeimagecacheprovider.h"
#include "imagecacheimageresponse.h"

#include <asynchronousimagecache.h>

#include <QMetaObject>

namespace QmlDesigner {

QQuickImageResponse *MidSizeImageCacheProvider::requestImageResponse(const QString &id, const QSize &)
{
    auto response = std::make_unique<ImageCacheImageResponse>(m_defaultImage);

    m_cache.requestMidSizeImage(
        Utils::PathString{id},
        [response = QPointer<ImageCacheImageResponse>(response.get())](const QImage &image) {
            QMetaObject::invokeMethod(
                response,
                [response, image] {
                    if (response)
                        response->setImage(image);
                },
                Qt::QueuedConnection);
        },
        [response = QPointer<ImageCacheImageResponse>(response.get())](
            ImageCache::AbortReason abortReason) {
            QMetaObject::invokeMethod(
                response,
                [response, abortReason] {
                    switch (abortReason) {
                    case ImageCache::AbortReason::Failed:
                    case ImageCache::AbortReason::NoEntry:
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
