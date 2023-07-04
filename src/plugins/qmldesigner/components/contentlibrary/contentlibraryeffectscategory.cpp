// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contentlibraryeffectscategory.h"

#include "contentlibraryeffect.h"

namespace QmlDesigner {

ContentLibraryEffectsCategory::ContentLibraryEffectsCategory(QObject *parent, const QString &name)
    : QObject(parent), m_name(name) {}

void ContentLibraryEffectsCategory::addBundleItem(ContentLibraryEffect *bundleItem)
{
    m_categoryItems.append(bundleItem);
}

bool ContentLibraryEffectsCategory::updateImportedState(const QStringList &importedItems)
{
    bool changed = false;

    for (ContentLibraryEffect *item : std::as_const(m_categoryItems))
        changed |= item->setImported(importedItems.contains(item->qml().chopped(4)));

    return changed;
}

bool ContentLibraryEffectsCategory::filter(const QString &searchText)
{
    bool visible = false;
    for (ContentLibraryEffect *item : std::as_const(m_categoryItems))
        visible |= item->filter(searchText);

    if (visible != m_visible) {
        m_visible = visible;
        emit categoryVisibleChanged();
        return true;
    }

    return false;
}

QString ContentLibraryEffectsCategory::name() const
{
    return m_name;
}

bool ContentLibraryEffectsCategory::visible() const
{
    return m_visible;
}

bool ContentLibraryEffectsCategory::expanded() const
{
    return m_expanded;
}

QList<ContentLibraryEffect *> ContentLibraryEffectsCategory::categoryItems() const
{
    return m_categoryItems;
}

} // namespace QmlDesigner
