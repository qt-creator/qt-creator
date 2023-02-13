// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "assetimageprovider.h"

#include <asset.h>
#include <imagecacheimageresponse.h>

#include <projectexplorer/target.h>
#include <utils/hdrimage.h>
#include <utils/stylehelper.h>

#include <QMetaObject>
#include <QQuickImageResponse>

namespace QmlDesigner {

QQuickImageResponse *AssetImageProvider::requestImageResponse(const QString &id,
                                                              const QSize &requestedSize)
{
    if (id.endsWith(".mesh"))
        return m_imageCacheProvider.requestImageResponse(id, {});

    if (id.endsWith(".builtin"))
        return m_imageCacheProvider.requestImageResponse("#" + id.split('.').first(), {});

    return m_imageCacheProvider.requestImageResponse(id, requestedSize);
}

} // namespace QmlDesigner
