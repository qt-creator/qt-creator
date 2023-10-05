// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "assetslibraryiconprovider.h"
#include "asset.h"
#include "modelnodeoperations.h"

#include <theme.h>
#include <utils/hdrimage.h>
#include <utils/ktximage.h>
#include <utils/stylehelper.h>

namespace QmlDesigner {

AssetsLibraryIconProvider::AssetsLibraryIconProvider(SynchronousImageCache &fontImageCache)
    : QQuickImageProvider(QQuickImageProvider::Pixmap)
    , m_fontImageCache(fontImageCache)
{
}

QPixmap AssetsLibraryIconProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    QPixmap pixmap;

    if (m_thumbnails.contains(id)) {
        pixmap = m_thumbnails[id].pixmap;
    } else {
        Thumbnail thumbnail = createThumbnail(id, requestedSize);
        pixmap = thumbnail.pixmap;

        if (thumbnail.assetType != Asset::MissingImage)
            m_thumbnails[id] = thumbnail;
    }

    if (size) {
        size->setWidth(pixmap.width());
        size->setHeight(pixmap.height());
    }

    return pixmap;
}

Thumbnail AssetsLibraryIconProvider::createThumbnail(const QString &id, const QSize &requestedSize)
{
    auto [pixmap, fileSize] = fetchPixmap(id, requestedSize);
    QSize originalSize = pixmap.size();
    Asset asset(id);
    Asset::Type assetType = asset.type();

    if (pixmap.isNull()) {
        pixmap = Utils::StyleHelper::dpiSpecificImageFile(":/AssetsLibrary/images/assets_default.png");

        if (assetType == Asset::Image)
            assetType = Asset::MissingImage;
        else if (asset.isKtxFile())
            originalSize = KtxImage(id).dimensions();
    }

    if (requestedSize.isValid()) {
        double ratio = requestedSize.width() / 48.;
        if (ratio * pixmap.size().width() > requestedSize.width()
            || ratio * pixmap.size().height() > requestedSize.height()) {
            pixmap = pixmap.scaled(requestedSize, Qt::KeepAspectRatio);
        } else if (!qFuzzyCompare(ratio, 1.)) {
            pixmap = pixmap.scaled(pixmap.size() * ratio, Qt::KeepAspectRatio);
        }
    }

    return Thumbnail{pixmap, originalSize, assetType, fileSize};
}

QPixmap AssetsLibraryIconProvider::generateFontIcons(const QString &filePath, const QSize &requestedSize) const
{
    QSize reqSize = requestedSize.isValid() ? requestedSize : QSize{48, 48};
    return m_fontImageCache.icon(filePath, {},
           ImageCache::FontCollectorSizesAuxiliaryData{Utils::span{iconSizes},
                                                       Theme::getColor(Theme::DStextColor).name(),
                                                       "Abc"}).pixmap(reqSize);
}

QPair<QPixmap, qint64> AssetsLibraryIconProvider::fetchPixmap(const QString &id, const QSize &requestedSize) const
{
    Asset asset(id);

    if (id == "browse") {
        QString filePath = Utils::StyleHelper::dpiSpecificImageFile(":/AssetsLibrary/images/browse.png");
        return {QPixmap{filePath}, 0};
    } else if (asset.isFont()) {
        qint64 size = QFileInfo(id).size();
        QPixmap pixmap = generateFontIcons(id, requestedSize);
        return {pixmap, size};
    } else if (asset.isImage()) {
        QString filePath = Utils::StyleHelper::dpiSpecificImageFile(id);
        qint64 size = QFileInfo(filePath).size();
        return {QPixmap{filePath}, size};
    } else if (asset.isHdrFile()) {
        qint64 size = QFileInfo(id).size();
        QPixmap pixmap = HdrImage{id}.toPixmap();
        return {pixmap, size};
    } else if (asset.isKtxFile()) {
        qint64 size = QFileInfo(id).size();
        QString filePath = Utils::StyleHelper::dpiSpecificImageFile(":/AssetsLibrary/images/asset_ktx.png");
        return {QPixmap{filePath}, size};
    } else {
        QString type;
        if (asset.isShader())
            type = "shader";
        else if (asset.isAudio())
            type = "sound";
        else if (asset.isVideo())
            type = "video";
        else if (asset.isEffect())
            type = QmlDesigner::ModelNodeOperations::getEffectIcon(id);

        QString pathTemplate = QString(":/AssetsLibrary/images/asset_%1%2.png").arg(type);
        QString path = pathTemplate.arg('_' + QString::number(requestedSize.width()));

        QString filePath = Utils::StyleHelper::dpiSpecificImageFile(QFileInfo::exists(path)
                                                                        ? path
                                                                        : pathTemplate.arg(""));
        qint64 size = QFileInfo(filePath).size();
        return {QPixmap{filePath}, size};
    }
}

void AssetsLibraryIconProvider::clearCache()
{
    m_thumbnails.clear();
}

void AssetsLibraryIconProvider::invalidateThumbnail(const QString &id)
{
    m_thumbnails.remove(id);
}

QSize AssetsLibraryIconProvider::imageSize(const QString &id)
{
    static QSize invalidSize = {};
    return m_thumbnails.contains(id) ? m_thumbnails[id].originalSize : invalidSize;
}

qint64 AssetsLibraryIconProvider::fileSize(const QString &id)
{
    return m_thumbnails.contains(id) ? m_thumbnails[id].fileSize : 0;
}

} // namespace QmlDesigner

