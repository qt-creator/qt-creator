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

ItemLibraryItem::ItemLibraryItem(QObject *parent)
    : QObject(parent),
      m_isVisible(true)
{
}

ItemLibraryItem::~ItemLibraryItem()
{
}

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
    //Prepend image provider prefix
    return QStringLiteral("image://qmldesigner_itemlibrary/") + m_itemLibraryEntry.libraryEntryIconPath();
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

void ItemLibraryItem::setItemLibraryEntry(const ItemLibraryEntry &itemLibraryEntry)
{
    m_itemLibraryEntry = itemLibraryEntry;
}

QVariant ItemLibraryItem::itemLibraryEntry() const
{
    return QVariant::fromValue(m_itemLibraryEntry);
}
} // namespace QmlDesigner
