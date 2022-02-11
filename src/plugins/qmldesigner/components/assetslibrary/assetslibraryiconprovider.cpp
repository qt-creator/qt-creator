/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "assetslibraryiconprovider.h"
#include "assetslibrarymodel.h"

#include <hdrimage.h>
#include <theme.h>
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
    const QString suffix = "*." + id.split('.').last().toLower();
    if (id == "browse") {
        pixmap = Utils::StyleHelper::dpiSpecificImageFile(":/AssetsLibrary/images/browse.png");
    } else if (AssetsLibraryModel::supportedFontSuffixes().contains(suffix)) {
        pixmap = generateFontIcons(id, requestedSize);
    } else if (AssetsLibraryModel::supportedImageSuffixes().contains(suffix)) {
        pixmap = Utils::StyleHelper::dpiSpecificImageFile(id);
    } else if (AssetsLibraryModel::supportedTexture3DSuffixes().contains(suffix)) {
        pixmap = HdrImage{id}.toPixmap();
    } else {
        QString type;
        if (AssetsLibraryModel::supportedShaderSuffixes().contains(suffix))
            type = "shader";
        else if (AssetsLibraryModel::supportedAudioSuffixes().contains(suffix))
            type = "sound";
        else if (AssetsLibraryModel::supportedVideoSuffixes().contains(suffix))
            type = "video";

        QString pathTemplate = QString(":/AssetsLibrary/images/asset_%1%2.png").arg(type);
        QString path = pathTemplate.arg('_' + QString::number(requestedSize.width()));

        pixmap = Utils::StyleHelper::dpiSpecificImageFile(QFileInfo::exists(path) ? path
                                                                                  : pathTemplate.arg(""));
    }

    if (size) {
        size->setWidth(pixmap.width());
        size->setHeight(pixmap.height());
    }

    if (pixmap.isNull())
        pixmap = Utils::StyleHelper::dpiSpecificImageFile(":/AssetsLibrary/images/assets_default.png");

    if (requestedSize.isValid())
        return pixmap.scaled(requestedSize);

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

} // namespace QmlDesigner

