// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemlibraryitem.h"

namespace QmlDesigner {

ItemLibraryItem::ItemLibraryItem(const ItemLibraryEntry &itemLibraryEntry, bool isUsable, QObject *parent)
    : QObject(parent)
    , m_itemLibraryEntry(itemLibraryEntry)
    , m_isUsable(isUsable)
{
}

ItemLibraryItem::~ItemLibraryItem() = default;

QString ItemLibraryItem::itemName() const
{
    return m_itemLibraryEntry.name();
}

QString ItemLibraryItem::typeName() const
{
    return QString::fromUtf8(m_itemLibraryEntry.typeName());
}

QString ItemLibraryItem::itemLibraryIconPath() const
{
    if (m_itemLibraryEntry.customComponentSource().isEmpty()
            || !m_itemLibraryEntry.libraryEntryIconPath().isEmpty()) {
        return QStringLiteral("image://qmldesigner_itemlibrary/")
               + m_itemLibraryEntry.libraryEntryIconPath();
    } else {
        return QStringLiteral("image://itemlibrary_preview/")
               + m_itemLibraryEntry.customComponentSource();
    }
}

QString ItemLibraryItem::componentPath() const
{
    return m_itemLibraryEntry.customComponentSource();
}

QString ItemLibraryItem::requiredImport() const
{
    return m_itemLibraryEntry.requiredImport();
}

QString ItemLibraryItem::componentSource() const
{
    return m_itemLibraryEntry.customComponentSource();
}

QString ItemLibraryItem::toolTip() const
{
    return m_itemLibraryEntry.toolTip();
}

bool ItemLibraryItem::setVisible(bool isVisible)
{
    if (isVisible != m_isVisible) {
        m_isVisible = isVisible;
        emit visibilityChanged();
        return true;
    }

    return false;
}

bool ItemLibraryItem::isVisible() const
{
    return m_isVisible;
}

bool ItemLibraryItem::isUsable() const
{
    return m_isUsable;
}

QVariant ItemLibraryItem::itemLibraryEntry() const
{
    return QVariant::fromValue(m_itemLibraryEntry);
}
} // namespace QmlDesigner
