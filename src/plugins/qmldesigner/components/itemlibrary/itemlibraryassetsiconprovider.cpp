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

#include "itemlibraryassetsiconprovider.h"
#include "itemlibraryassetsmodel.h"

#include <hdrimage.h>
#include <theme.h>
#include <utils/stylehelper.h>

namespace QmlDesigner {

ItemLibraryAssetsIconProvider::ItemLibraryAssetsIconProvider(SynchronousImageCache &fontImageCache)
    : QQuickImageProvider(QQuickImageProvider::Pixmap)
    , m_fontImageCache(fontImageCache)
{
}

QPixmap ItemLibraryAssetsIconProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    QPixmap pixmap;
    const QString suffix = "*." + id.split('.').last().toLower();
    if (id == "browse")
        pixmap = Utils::StyleHelper::dpiSpecificImageFile(":/ItemLibrary/images/browse.png");
    else if (ItemLibraryAssetsModel::supportedFontSuffixes().contains(suffix))
        pixmap = generateFontIcons(id);
    else if (ItemLibraryAssetsModel::supportedImageSuffixes().contains(suffix))
        pixmap = Utils::StyleHelper::dpiSpecificImageFile(id);
    else if (ItemLibraryAssetsModel::supportedTexture3DSuffixes().contains(suffix))
        pixmap = HdrImage{id}.toPixmap();
    else if (ItemLibraryAssetsModel::supportedShaderSuffixes().contains(suffix))
        pixmap = Utils::StyleHelper::dpiSpecificImageFile(":/ItemLibrary/images/asset_shader_48.png");
    else if (ItemLibraryAssetsModel::supportedAudioSuffixes().contains(suffix))
        pixmap = Utils::StyleHelper::dpiSpecificImageFile(":/ItemLibrary/images/asset_sound_48.png");

    if (size) {
        size->setWidth(pixmap.width());
        size->setHeight(pixmap.height());
    }

    if (pixmap.isNull()) {
        pixmap = QPixmap(Utils::StyleHelper::dpiSpecificImageFile(
            QStringLiteral(":/ItemLibrary/images/item-default-icon.png")));
    }

    if (requestedSize.isValid())
        return pixmap.scaled(requestedSize);

    return pixmap;
}

QPixmap ItemLibraryAssetsIconProvider::generateFontIcons(const QString &filePath) const
{
     return m_fontImageCache.icon(filePath, {},
           ImageCache::FontCollectorSizesAuxiliaryData{Utils::span{iconSizes},
                                                       Theme::getColor(Theme::DStextColor).name(),
                                                       "Abc"}).pixmap({48, 48});
}

} // namespace QmlDesigner

