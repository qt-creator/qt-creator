// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "imagecache/midsizeimagecacheprovider.h"

#include <QQuickAsyncImageProvider>

namespace QmlDesigner {

class AssetImageProvider : public QQuickAsyncImageProvider
{
public:
    AssetImageProvider(AsynchronousImageCache &imageCache, const QImage &defaultImage = {})
        : m_imageCacheProvider(imageCache, defaultImage)
    {}

    QQuickImageResponse *requestImageResponse(const QString &id,
                                              const QSize &requestedSize) override;

private:
    MidSizeImageCacheProvider m_imageCacheProvider;
};

} // namespace QmlDesigner
