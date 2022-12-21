// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "explicitimagecacheimageprovider.h"

#include <asynchronousexplicitimagecache.h>

#include <QMetaObject>
#include <QQuickImageResponse>

namespace {

class ImageResponse : public QQuickImageResponse
{
public:
    ImageResponse(const QImage &defaultImage)
        : m_image(defaultImage)
    {}

    QQuickTextureFactory *textureFactory() const override
    {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

    void setImage(const QImage &image)
    {
        m_image = image;

        emit finished();
    }

    void abort() { emit finished(); }

private:
    QImage m_image;
};

} // namespace

namespace QmlDesigner {

QQuickImageResponse *ExplicitImageCacheImageProvider::requestImageResponse(const QString &id,
                                                                           const QSize &)
{
    auto response = std::make_unique<::ImageResponse>(m_defaultImage);

    m_cache.requestImage(
        id,
        [response = QPointer<::ImageResponse>(response.get())](const QImage &image) {
            QMetaObject::invokeMethod(
                response,
                [response, image] {
                    if (response)
                        response->setImage(image);
                },
                Qt::QueuedConnection);
        },
        [response = QPointer<::ImageResponse>(response.get())](ImageCache::AbortReason abortReason) {
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
