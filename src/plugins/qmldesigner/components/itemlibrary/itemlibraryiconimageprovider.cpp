// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemlibraryiconimageprovider.h"

#include <projectexplorer/target.h>
#include <utils/stylehelper.h>

#include <QMetaObject>
#include <QQuickImageResponse>

namespace QmlDesigner {

class ImageRespose : public QQuickImageResponse
{
public:
    QQuickTextureFactory *textureFactory() const override
    {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

    void setImage(const QImage &image)
    {
        m_image = image;

        emit finished();
    }

    void abort()
    {
        m_image = QImage{
            Utils::StyleHelper::dpiSpecificImageFile(":/ItemLibrary/images/item-default-icon.png")};

        emit finished();
    }

private:
    QImage m_image;
};


QQuickImageResponse *ItemLibraryIconImageProvider::requestImageResponse(const QString &id,
                                                                        const QSize &)
{
    auto response = std::make_unique<ImageRespose>();

    m_cache.requestSmallImage(
        id,
        [response = QPointer<ImageRespose>(response.get())](const QImage &image) {
            QMetaObject::invokeMethod(
                response,
                [response, image] {
                    if (response)
                        response->setImage(image);
                },
                Qt::QueuedConnection);
        },
        [response = QPointer<ImageRespose>(response.get())](ImageCache::AbortReason abortReason) {
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
        },
        "libIcon",
        ImageCache::LibraryIconAuxiliaryData{true});

    return response.release();
}

} // namespace QmlDesigner
