// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "usertexturecategory.h"

#include "contentlibrarytexture.h"

#include <asset.h>
#include <imageutils.h>

#include <QPixmap>

namespace QmlDesigner {

UserTextureCategory::UserTextureCategory(const QString &title, const Utils::FilePath &bundlePath)
    : UserCategory(title, bundlePath)
{
}

void UserTextureCategory::loadBundle(bool force)
{
    if (m_bundleLoaded && !force)
        return;

    // clean up
    qDeleteAll(m_items);
    m_items.clear();

    m_bundlePath.ensureWritableDir();
    m_bundlePath.pathAppended("icons").ensureWritableDir();

    addItems(m_bundlePath.dirEntries({Asset::supportedImageSuffixes(), QDir::Files}));

    m_bundleLoaded = true;
}

void UserTextureCategory::filter(const QString &searchText)
{
    bool noMatch = true;
    for (QObject *item : std::as_const(m_items)) {
        ContentLibraryTexture *castedItem = qobject_cast<ContentLibraryTexture *>(item);
        bool itemVisible = castedItem->filter(searchText);
        if (itemVisible)
            noMatch = false;
    }
    setNoMatch(noMatch);
}

void UserTextureCategory::addItems(const Utils::FilePaths &paths)
{
    for (const Utils::FilePath &filePath : paths) {
        QString suffix = '.' + filePath.suffix();
        auto iconFileInfo = filePath.parentDir().pathAppended("icons/" + filePath.baseName() + ".png")
                                .toFileInfo();
        QPair<QSize, qint64> info = ImageUtils::imageInfo(filePath.path());
        QString dirPath = filePath.parentDir().toFSPathString();
        QSize imgDims = info.first;
        qint64 imgFileSize = info.second;

        if (!iconFileInfo.exists()) { // generate an icon if one doesn't exist
            Asset asset{filePath.toFSPathString()};
            QPixmap icon = asset.pixmap({120, 120});
            bool iconSaved = icon.save(iconFileInfo.filePath());
            if (!iconSaved)
                qWarning() << __FUNCTION__ << "icon save failed";
        }

        auto tex = new ContentLibraryTexture(this, iconFileInfo, dirPath, suffix, imgDims, imgFileSize);
        m_items.append(tex);
    }

    setIsEmpty(m_items.isEmpty());
    emit itemsChanged();
}

void UserTextureCategory::clearItems()
{
    qDeleteAll(m_items);
    m_items.clear();

    setIsEmpty(true);
    emit itemsChanged();
}

} // namespace QmlDesigner
