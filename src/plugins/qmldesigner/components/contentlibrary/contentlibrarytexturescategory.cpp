// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentlibrarytexturescategory.h"

#include "contentlibrarytexture.h"

#include <utils/algorithm.h>

#include <QFileInfo>

namespace QmlDesigner {

ContentLibraryTexturesCategory::ContentLibraryTexturesCategory(QObject *parent, const QString &name)
    : QObject(parent), m_name(name) {}

void ContentLibraryTexturesCategory::addTexture(const QFileInfo &tex, const QString &downloadPath,
                                                const QString &key, const QString &webTextureUrl,
                                                const QString &webIconUrl, const QString &fileExt,
                                                const QSize &dimensions, const qint64 sizeInBytes,
                                                bool hasUpdate, bool isNew)
{
    QUrl icon = QUrl::fromLocalFile(tex.absoluteFilePath());

    m_categoryTextures.append(new ContentLibraryTexture(
        this, tex, downloadPath, icon, key, webTextureUrl, webIconUrl,
        fileExt, dimensions, sizeInBytes, hasUpdate, isNew));
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

void ContentLibraryTexturesCategory::markTextureHasNoUpdate(const QString &textureKey)
{
    auto *texture = Utils::findOrDefault(m_categoryTextures, [&textureKey](ContentLibraryTexture *t) {
        return t->textureKey() == textureKey;
    });

    if (texture)
        texture->setHasUpdate(false);
}

} // namespace QmlDesigner
