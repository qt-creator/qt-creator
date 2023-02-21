// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "contentlibrarytexture.h"

#include "imageutils.h"
#include <utils/algorithm.h>

#include <QDir>
#include <QFileInfo>

namespace QmlDesigner {

ContentLibraryTexture::ContentLibraryTexture(QObject *parent, const QFileInfo &iconFileInfo,
    const QString &downloadPath, const QUrl &icon, const QString &webUrl)
    : QObject(parent)
    , m_iconPath(iconFileInfo.filePath())
    , m_downloadPath(downloadPath)
    , m_webUrl(webUrl)
    , m_baseName{iconFileInfo.baseName()}
    , m_icon(icon)
{
    m_fileExt = resolveFileExt();
    m_toolTip = resolveToolTipText();
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

    if (textureFiles.count() > 1) {
        qWarning() << "Found multiple textures with the same name in the same directories: "
                   << Utils::transform(textureFiles, [](const QFileInfo &fi) {
                          return fi.fileName();
                      });
    }

    return QString{"."} + textureFiles.at(0).completeSuffix();
}

QString ContentLibraryTexture::resolveToolTipText()
{
    QString fileName;
    QString imageInfo;

    if (m_fileExt.isEmpty()) {
        imageInfo = ImageUtils::imageInfo(m_iconPath, false);
        fileName = m_baseName + m_defaultExt;
    } else {
        fileName = m_baseName + m_fileExt;
        QString fullDownloadPath = m_downloadPath + "/" + fileName;
        imageInfo = ImageUtils::imageInfo(fullDownloadPath, true);
    }

    return QLatin1String("%1\n%2").arg(fileName, imageInfo);
}

bool ContentLibraryTexture::isDownloaded() const
{
    if (m_fileExt.isEmpty())
        return false;

    QString fullPath = realTexturePath();
    return QFileInfo(fullPath).isFile();
}

QString ContentLibraryTexture::realTexturePath() const
{
    return m_downloadPath + "/" + m_baseName + m_fileExt;
}

void ContentLibraryTexture::setDownloaded()
{
    m_fileExt = resolveFileExt();
    QString toolTip = resolveToolTipText();

    if (toolTip != m_toolTip) {
        m_toolTip = toolTip;
        emit textureToolTipChanged();
    }
}

QString ContentLibraryTexture::parentDirPath() const
{
    return m_downloadPath;
}

} // namespace QmlDesigner
