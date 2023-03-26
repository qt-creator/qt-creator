// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QFileInfo>
#include <QObject>
#include <QSize>
#include <QUrl>

namespace QmlDesigner {

class ContentLibraryTexture : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString textureIconPath MEMBER m_iconPath CONSTANT)
    Q_PROPERTY(QString textureParentPath READ parentDirPath CONSTANT)
    Q_PROPERTY(QString textureToolTip MEMBER m_toolTip NOTIFY textureToolTipChanged)
    Q_PROPERTY(QUrl textureIcon MEMBER m_icon CONSTANT)
    Q_PROPERTY(bool textureVisible MEMBER m_visible NOTIFY textureVisibleChanged)
    Q_PROPERTY(QString textureWebUrl MEMBER m_webUrl CONSTANT)

public:
    ContentLibraryTexture(QObject *parent, const QFileInfo &iconFileInfo,
                          const QString &downloadPath, const QUrl &icon, const QString &webUrl,
                          const QString &fileExt, const QSize &dimensions, const qint64 sizeInBytes);

    Q_INVOKABLE bool isDownloaded() const;
    Q_INVOKABLE void setDownloaded();

    bool filter(const QString &searchText);

    QUrl icon() const;
    QString iconPath() const;
    QString downloadedTexturePath() const;
    QString parentDirPath() const;

signals:
    void textureVisibleChanged();
    void textureToolTipChanged();

private:
    QString resolveFileExt();
    QString resolveToolTipText();
    void doSetDownloaded();

    QString m_iconPath;
    QString m_downloadPath;
    QString m_webUrl;
    QString m_toolTip;
    QString m_baseName;
    QString m_fileExt;
    QUrl m_icon;
    QSize m_dimensions;
    qint64 m_sizeInBytes = -1;
    bool m_isDownloaded = false;

    bool m_visible = true;
};

} // namespace QmlDesigner
