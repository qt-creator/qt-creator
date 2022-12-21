// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "imagecache/smallimagecacheprovider.h"

#include <QQuickAsyncImageProvider>

namespace QmlDesigner {

class PropertyEditorImageProvider : public QQuickAsyncImageProvider
{
public:
    PropertyEditorImageProvider(AsynchronousImageCache &imageCache, const QImage &defaultImage = {})
        : m_smallImageCacheProvider(imageCache, defaultImage)
    {}

    QQuickImageResponse *requestImageResponse(const QString &id,
                                              const QSize &requestedSize) override;

private:
    SmallImageCacheProvider m_smallImageCacheProvider;
};

} // namespace QmlDesigner
