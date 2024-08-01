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
    Q_PROPERTY(QString textureUrl MEMBER m_textureUrl CONSTANT)
    Q_PROPERTY(QString textureIconUrl MEMBER m_iconUrl CONSTANT)
    Q_PROPERTY(bool textureHasUpdate WRITE setHasUpdate READ hasUpdate NOTIFY hasUpdateChanged)
    Q_PROPERTY(bool textureIsNew MEMBER m_isNew CONSTANT)
    Q_PROPERTY(QString textureKey MEMBER m_textureKey CONSTANT)
    Q_PROPERTY(QString bundleId MEMBER m_bundleId CONSTANT)

public:
    ContentLibraryTexture(QObject *parent, const QFileInfo &iconFileInfo, const QString &dirPath,
                          const QString &suffix, const QSize &dimensions, const qint64 sizeInBytes,
                          const QString &key = {}, const QString &textureUrl = {},
                          const QString &iconUrl = {}, bool hasUpdate = false, bool isNew = false);

    Q_INVOKABLE bool isDownloaded() const;
    Q_INVOKABLE void setDownloaded();

    bool filter(const QString &searchText);

    QUrl icon() const;
    QString iconPath() const;
    QString texturePath() const;
    QString parentDirPath() const;
    QString textureKey() const;
    QString fileName() const;

    void setHasUpdate(bool value);
    bool hasUpdate() const;

    bool visible() const;

signals:
    void textureVisibleChanged();
    void textureToolTipChanged();
    void hasUpdateChanged();

private:
    QString resolveSuffix();
    QString resolveToolTipText();
    void doSetDownloaded();

    QString m_iconPath;
    QString m_dirPath;
    QString m_textureUrl;
    QString m_iconUrl;
    QString m_toolTip;
    QString m_baseName;
    QString m_suffix;
    QString m_textureKey;
    QUrl m_icon;
    QSize m_dimensions;
    qint64 m_sizeInBytes = -1;
    bool m_isDownloaded = false;

    bool m_visible = true;
    bool m_hasUpdate = false;
    bool m_isNew = false;
    const QString m_bundleId = "UserTextures";
};

} // namespace QmlDesigner
