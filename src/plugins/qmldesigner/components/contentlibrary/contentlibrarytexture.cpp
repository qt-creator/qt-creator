// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentlibrarytexture.h"

#include "imageutils.h"
#include <utils/algorithm.h>

#include <QDir>
#include <QFileInfo>

namespace QmlDesigner {

ContentLibraryTexture::ContentLibraryTexture(QObject *parent, const QFileInfo &iconFileInfo,
                                             const QString &downloadPath, const QUrl &icon,
                                             const QString &key, const QString &webTextureUrl,
                                             const QString &webIconUrl, const QString &fileExt,
                                             const QSize &dimensions, const qint64 sizeInBytes,
                                             bool hasUpdate, bool isNew)
    : QObject(parent)
    , m_iconPath(iconFileInfo.filePath())
    , m_downloadPath(downloadPath)
    , m_webTextureUrl(webTextureUrl)
    , m_webIconUrl(webIconUrl)
    , m_baseName{iconFileInfo.baseName()}
    , m_fileExt(fileExt)
    , m_textureKey(key)
    , m_icon(icon)
    , m_dimensions(dimensions)
    , m_sizeInBytes(sizeInBytes)
    , m_hasUpdate(hasUpdate)
    , m_isNew(isNew)
{
    doSetDownloaded();
}

bool ContentLibraryTexture::filter(const QString &searchText)
{
    if (m_visible != m_iconPath.contains(searchText, Qt::CaseInsensitive)) {
        m_visible = !m_visible;
        emit textureVisibleChanged();
    }

    return m_visible;
}

QUrl ContentLibraryTexture::icon() const
{
    return m_icon;
}

QString ContentLibraryTexture::iconPath() const
{
    return m_iconPath;
}

QString ContentLibraryTexture::resolveFileExt()
{
    const QFileInfoList files = QDir(m_downloadPath).entryInfoList(QDir::Files);
    const QFileInfoList textureFiles = Utils::filtered(files, [this](const QFileInfo &fi) {
        return fi.baseName() == m_baseName;
    });

    if (textureFiles.isEmpty())
        return {};

    if (textureFiles.size() > 1) {
        qWarning() << "Found multiple textures with the same name in the same directories: "
                   << Utils::transform(textureFiles, [](const QFileInfo &fi) {
                          return fi.fileName();
                      });
    }

    return '.' + textureFiles.at(0).completeSuffix();
}

QString ContentLibraryTexture::resolveToolTipText()
{
    if (m_fileExt.isEmpty()) {
        // No supplied or resolved extension means we have just the icon and no other data
        return m_baseName;
    }

    QString fileName = m_baseName + m_fileExt;
    QString imageInfo;

    if (!m_isDownloaded && m_sizeInBytes > 0 && !m_dimensions.isNull()) {
        imageInfo = ImageUtils::imageInfo(m_dimensions, m_sizeInBytes);
    } else {
        QString fullDownloadPath = m_downloadPath + '/' + fileName;
        imageInfo = ImageUtils::imageInfo(fullDownloadPath);
    }

    return QStringLiteral("%1\n%2").arg(fileName, imageInfo);
}

bool ContentLibraryTexture::isDownloaded() const
{
    return m_isDownloaded;
}

QString ContentLibraryTexture::downloadedTexturePath() const
{
    return m_downloadPath + '/' + m_baseName + m_fileExt;
}

void ContentLibraryTexture::setDownloaded()
{
    QString toolTip = m_toolTip;

    doSetDownloaded();

    if (toolTip != m_toolTip)
        emit textureToolTipChanged();
}

void ContentLibraryTexture::doSetDownloaded()
{
    if (m_fileExt.isEmpty())
        m_fileExt = resolveFileExt();

    m_isDownloaded = QFileInfo::exists(downloadedTexturePath());
    m_toolTip = resolveToolTipText();
}

QString ContentLibraryTexture::parentDirPath() const
{
    return m_downloadPath;
}

QString ContentLibraryTexture::textureKey() const
{
    return m_textureKey;
}

void ContentLibraryTexture::setHasUpdate(bool value)
{
    if (m_hasUpdate != value) {
        m_hasUpdate = value;
        emit hasUpdateChanged();
    }
}

bool ContentLibraryTexture::hasUpdate() const
{
    return m_hasUpdate;
}

} // namespace QmlDesigner
