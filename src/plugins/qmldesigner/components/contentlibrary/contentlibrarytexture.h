// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QUrl>

namespace QmlDesigner {

class ContentLibraryTexture : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString texturePath MEMBER m_path CONSTANT)
    Q_PROPERTY(QString textureToolTip MEMBER m_toolTip CONSTANT)
    Q_PROPERTY(QUrl textureIcon MEMBER m_icon CONSTANT)
    Q_PROPERTY(bool textureVisible MEMBER m_visible NOTIFY textureVisibleChanged)

public:
    ContentLibraryTexture(QObject *parent, const QString &path, const QUrl &icon);

    bool filter(const QString &searchText);

    QUrl icon() const;
    QString path() const;

signals:
    void textureVisibleChanged();

private:
    QString m_path;
    QString m_toolTip;
    QUrl m_icon;

    bool m_visible = true;
};

} // namespace QmlDesigner
