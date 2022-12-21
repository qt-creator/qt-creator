// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentlibrarytexturescategory.h"

#include "contentlibrarytexture.h"

#include <QFileInfo>

namespace QmlDesigner {

ContentLibraryTexturesCategory::ContentLibraryTexturesCategory(QObject *parent, const QString &name)
    : QObject(parent), m_name(name) {}

void ContentLibraryTexturesCategory::addTexture(const QFileInfo &tex)
{
    QUrl icon = QUrl::fromLocalFile(tex.path() + "/icon/" + tex.baseName() + ".png");
    m_categoryTextures.append(new ContentLibraryTexture(this, tex.filePath(), icon));
}

bool ContentLibraryTexturesCategory::filter(const QString &searchText)
{
    bool visible = false;
    for (ContentLibraryTexture *tex : std::as_const(m_categoryTextures))
        visible |= tex->filter(searchText);

    if (visible != m_visible) {
        m_visible = visible;
        emit categoryVisibleChanged();
        return true;
    }

    return false;
}

QString ContentLibraryTexturesCategory::name() const
{
    return m_name;
}

bool ContentLibraryTexturesCategory::visible() const
{
    return m_visible;
}

bool ContentLibraryTexturesCategory::expanded() const
{
    return m_expanded;
}

QList<ContentLibraryTexture *> ContentLibraryTexturesCategory::categoryTextures() const
{
    return m_categoryTextures;
}

} // namespace QmlDesigner
