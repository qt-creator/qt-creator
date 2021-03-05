/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
    if (m_itemLibraryEntry.customComponentSource().isEmpty()) {
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
