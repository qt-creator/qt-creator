// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentlibrarytexture.h"

#include "imageutils.h"
#include <utils/algorithm.h>

#include <QDir>
#include <QFileInfo>

namespace QmlDesigner {

ContentLibraryTexture::ContentLibraryTexture(QObject *parent, const QFileInfo &iconFileInfo,
                                             const QString &dirPath, const QString &suffix,
                                             const QSize &dimensions, const qint64 sizeInBytes,
                                             const QString &key, const QString &textureUrl,
                                             const QString &iconUrl, bool hasUpdate, bool isNew)
    : QObject(parent)
    , m_iconPath(iconFileInfo.filePath())
    , m_dirPath(dirPath)
    , m_textureUrl(textureUrl)
    , m_iconUrl(iconUrl)
    , m_baseName{iconFileInfo.baseName()}
    , m_suffix(suffix)
    , m_textureKey(key)
    , m_icon(QUrl::fromLocalFile(iconFileInfo.absoluteFilePath()))
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

QString ContentLibraryTexture::resolveSuffix()
{
    const QFileInfoList files = QDir(m_dirPath).entryInfoList(QDir::Files);
    const QFileInfoList textureFiles = Utils::filtered(files, [this](const QFileInfo &fi) {
        return fi.baseName() == m_baseName;
    });

    if (textureFiles.isEmpty())
        return {};

    if (textureFiles.size() > 1) {
        qWarning() << "Found multiple textures with the same name in the same directories: "
                   << Utils::transform(textureFiles, &QFileInfo::fileName);
    }

    return '.' + textureFiles.at(0).completeSuffix();
}

QString ContentLibraryTexture::resolveToolTipText()
{
    if (m_suffix.isEmpty())
        return m_baseName; // empty suffix means we have just the icon and no other data

    QString texFileName = fileName();
    QString imageInfo;

    if (!m_isDownloaded && m_sizeInBytes > 0 && !m_dimensions.isNull()) {
        imageInfo = ImageUtils::imageInfoString(m_dimensions, m_sizeInBytes);
    } else {
        QString fullDownloadPath = m_dirPath + '/' + texFileName;
        imageInfo = ImageUtils::imageInfoString(fullDownloadPath);
    }

    return QString("%1\n%2").arg(texFileName, imageInfo);
}

bool ContentLibraryTexture::isDownloaded() const
{
    return m_isDownloaded;
}

QString ContentLibraryTexture::texturePath() const
{
    return m_dirPath + '/' + fileName();
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
    if (m_suffix.isEmpty())
        m_suffix = resolveSuffix();

    m_isDownloaded = QFileInfo::exists(texturePath());
    m_toolTip = resolveToolTipText();
}

bool ContentLibraryTexture::visible() const
{
    return m_visible;
}

QString ContentLibraryTexture::parentDirPath() const
{
    return m_dirPath;
}

QString ContentLibraryTexture::textureKey() const
{
    return m_textureKey;
}

QString ContentLibraryTexture::fileName() const
{
    return m_baseName + m_suffix;
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
