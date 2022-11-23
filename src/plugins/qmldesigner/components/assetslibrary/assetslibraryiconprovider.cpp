// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "assetslibraryiconprovider.h"
#include "assetslibrarymodel.h"
#include "modelnodeoperations.h"

#include <theme.h>
#include <utils/hdrimage.h>
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
        pixmap = m_thumbnails[id];
    } else {
        pixmap = fetchPixmap(id, requestedSize);
        if (pixmap.isNull())
            pixmap = Utils::StyleHelper::dpiSpecificImageFile(":/AssetsLibrary/images/assets_default.png");

        if (requestedSize.isValid())
            pixmap = pixmap.scaled(requestedSize, Qt::KeepAspectRatio);

        m_thumbnails[id] = pixmap;
    }

    if (size) {
        size->setWidth(pixmap.width());
        size->setHeight(pixmap.height());
    }

    return pixmap;
}

QPixmap AssetsLibraryIconProvider::generateFontIcons(const QString &filePath, const QSize &requestedSize) const
{
    QSize reqSize = requestedSize.isValid() ? requestedSize : QSize{48, 48};
    return m_fontImageCache.icon(filePath, {},
           ImageCache::FontCollectorSizesAuxiliaryData{Utils::span{iconSizes},
                                                       Theme::getColor(Theme::DStextColor).name(),
                                                       "Abc"}).pixmap(reqSize);
}

QPixmap AssetsLibraryIconProvider::fetchPixmap(const QString &id, const QSize &requestedSize) const
{
    const QString suffix = "*." + id.split('.').last().toLower();
    if (id == "browse") {
        return Utils::StyleHelper::dpiSpecificImageFile(":/AssetsLibrary/images/browse.png");
    } else if (AssetsLibraryModel::supportedFontSuffixes().contains(suffix)) {
        return generateFontIcons(id, requestedSize);
    } else if (AssetsLibraryModel::supportedImageSuffixes().contains(suffix)) {
        return Utils::StyleHelper::dpiSpecificImageFile(id);
    } else if (AssetsLibraryModel::supportedTexture3DSuffixes().contains(suffix)) {
        return HdrImage{id}.toPixmap();
    } else {
        QString type;
        if (AssetsLibraryModel::supportedShaderSuffixes().contains(suffix))
            type = "shader";
        else if (AssetsLibraryModel::supportedAudioSuffixes().contains(suffix))
            type = "sound";
        else if (AssetsLibraryModel::supportedVideoSuffixes().contains(suffix))
            type = "video";
        else if (AssetsLibraryModel::supportedEffectMakerSuffixes().contains(suffix))
            type = QmlDesigner::ModelNodeOperations::getEffectIcon(id);

        QString pathTemplate = QString(":/AssetsLibrary/images/asset_%1%2.png").arg(type);
        QString path = pathTemplate.arg('_' + QString::number(requestedSize.width()));

        return Utils::StyleHelper::dpiSpecificImageFile(QFileInfo::exists(path)
                                                            ? path
                                                            : pathTemplate.arg(""));
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

} // namespace QmlDesigner

