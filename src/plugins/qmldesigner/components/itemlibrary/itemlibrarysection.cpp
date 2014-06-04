/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "itemlibrarysection.h"

#include "itemlibraryitem.h"

namespace QmlDesigner {

ItemLibrarySection::ItemLibrarySection(int sectionLibId, const QString &sectionName, QObject *parent)
    : QObject(parent),
      m_sectionEntries(parent),
      m_name(sectionName),
      m_sectionLibraryId(sectionLibId),
      m_sectionExpanded(true),
      m_isVisible(false)
{
//    if (collapsedStateHash.contains(sectionName))
//        m_sectionExpanded=  collapsedStateHash.value(sectionName);
}


QString ItemLibrarySection::sectionName() const
{
    return m_name;
}

int ItemLibrarySection::sectionLibraryId() const
{
    return m_sectionLibraryId;
}

bool ItemLibrarySection::sectionExpanded() const
{
    return m_sectionExpanded;
}

QVariant ItemLibrarySection::sortingRole() const
{

    if (sectionName() == QStringLiteral("QML Components")) //Qml Components always come first
        return QVariant(QStringLiteral("AA.this_comes_first"));

    return sectionName();
}

void ItemLibrarySection::addSectionEntry(ItemLibraryItem *sectionEntry)
{
    m_sectionEntries.addItem(sectionEntry);
}

QObject *ItemLibrarySection::sectionEntries()
{
    return &m_sectionEntries;
}

bool ItemLibrarySection::updateSectionVisibility(const QString &searchText, bool *changed)
{
    bool haveVisibleItems = false;

    *changed = false;

    foreach(ItemLibraryItem *itemLibraryItem, m_sectionEntries.items()) {
        bool itemVisible = itemLibraryItem->itemName().toLower().contains(searchText);

        bool itemChanged = false;
        itemChanged = itemLibraryItem->setVisible(itemVisible);

        *changed |= itemChanged;

        if (itemVisible)
            haveVisibleItems = true;
    }

    m_sectionEntries.resetModel();

    emit sectionEntriesChanged();

    return haveVisibleItems;
}


void ItemLibrarySection::updateItemIconSize(const QSize &itemIconSize)
{
    foreach (ItemLibraryItem* itemLibraryItemModel, m_sectionEntries.items())
        itemLibraryItemModel->setItemIconSize(itemIconSize);
}

bool ItemLibrarySection::setVisible(bool isVisible)
{
    if (isVisible != m_isVisible) {
        m_isVisible = isVisible;
        return true;
    }

    return false;
}

bool ItemLibrarySection::isVisible() const
{
    return m_isVisible;
}

} // namespace QmlDesigner
