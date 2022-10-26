// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "contentlibrarytexture.h"

namespace QmlDesigner {

ContentLibraryTexture::ContentLibraryTexture(QObject *parent, const QString &path, const QUrl &icon)
    : QObject(parent)
    , m_path(path)
    , m_icon(icon) {}

bool ContentLibraryTexture::filter(const QString &searchText)
{
    if (m_visible != m_path.contains(searchText, Qt::CaseInsensitive)) {
        m_visible = !m_visible;
        emit textureVisibleChanged();
    }

    return m_visible;
}

QUrl ContentLibraryTexture::icon() const
{
    return m_icon;
}

QString ContentLibraryTexture::path() const
{
    return m_path;
}

} // namespace QmlDesigner
