// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemlibraryiconimageprovider.h"

#include <imagecacheimageresponse.h>

#include <projectexplorer/target.h>
#include <utils/stylehelper.h>

#include <QMetaObject>
#include <QQuickImageResponse>

namespace QmlDesigner {

QQuickImageResponse *ItemLibraryIconImageProvider::requestImageResponse(const QString &id,
                                                                        const QSize &)
{
    auto response = std::make_unique<ImageCacheImageResponse>(QImage{
        Utils::StyleHelper::dpiSpecificImageFile(":/ItemLibrary/images/item-default-icon.png")});

    m_cache.requestSmallImage(
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
        },
        "libIcon",
        ImageCache::LibraryIconAuxiliaryData{true});

    return response.release();
}

} // namespace QmlDesigner
