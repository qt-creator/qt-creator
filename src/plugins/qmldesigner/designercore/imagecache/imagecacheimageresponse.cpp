// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "imagecacheimageresponse.h"

namespace QmlDesigner {

QQuickTextureFactory *ImageCacheImageResponse::textureFactory() const
{
    return QQuickTextureFactory::textureFactoryForImage(m_image);
}

void ImageCacheImageResponse::setImage(const QImage &image)
{
    m_image = image;

    emit finished();
}

void ImageCacheImageResponse::abort()
{
    emit finished();
}

} // namespace QmlDesigner
