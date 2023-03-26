// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QQuickAsyncImageProvider>

namespace QmlDesigner {

class AsynchronousImageCache;

class SmallImageCacheProvider : public QQuickAsyncImageProvider
{
public:
    SmallImageCacheProvider(AsynchronousImageCache &imageCache, const QImage &defaultImage = {})
        : m_cache{imageCache}
        , m_defaultImage(defaultImage)
    {}

    QQuickImageResponse *requestImageResponse(const QString &id,
                                              const QSize &requestedSize) override;
    QImage defaultImage() const { return m_defaultImage; }

private:
    AsynchronousImageCache &m_cache;
    QImage m_defaultImage;
};

} // namespace QmlDesigner
