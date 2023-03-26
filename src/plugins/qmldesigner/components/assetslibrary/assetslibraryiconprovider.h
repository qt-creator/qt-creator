// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <synchronousimagecache.h>

#include <QQuickImageProvider>

#include "asset.h"

namespace QmlDesigner {

struct Thumbnail
{
    QPixmap pixmap;
    QSize originalSize;
    Asset::Type assetType;
    qint64 fileSize;
};

class AssetsLibraryIconProvider : public QQuickImageProvider
{
public:
    AssetsLibraryIconProvider(SynchronousImageCache &fontImageCache);

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;
    void clearCache();
    void invalidateThumbnail(const QString &id);
    QSize imageSize(const QString &id);
    qint64 fileSize(const QString &id);

private:
    QPixmap generateFontIcons(const QString &filePath, const QSize &requestedSize) const;
    QPair<QPixmap, qint64> fetchPixmap(const QString &id, const QSize &requestedSize) const;
    Thumbnail createThumbnail(const QString &id, const QSize &requestedSize);

    SynchronousImageCache &m_fontImageCache;

    // Generated icon sizes should contain all ItemLibraryResourceView needed icon sizes, and their
    // x2 versions for HDPI sceens
    std::vector<QSize> iconSizes = {{128, 128}, // Drag
                                    {96, 96},   // list @2x
                                    {48, 48}};  // list
    QHash<QString, Thumbnail> m_thumbnails;
};

} // namespace QmlDesigner
