// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_exports.h>

#include <QQuickImageResponse>

namespace QmlDesigner {

class AsynchronousImageCache;

class QMLDESIGNERCORE_EXPORT ImageCacheImageResponse : public QQuickImageResponse
{
public:
    ImageCacheImageResponse(const QImage &defaultImage = {})
        : m_image(defaultImage)
    {}

    QQuickTextureFactory *textureFactory() const override;

    void setImage(const QImage &image);
    QImage image() const { return m_image; }

    void abort();

private:
    QImage m_image;
};

} // namespace QmlDesigner
