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

#include "itemlibrarysectionmodel.h"

namespace QmlDesigner {

ItemLibrarySectionModel::ItemLibrarySectionModel(int sectionLibId, const QString &sectionName, QObject *parent)
    : QObject(parent),
      m_name(sectionName),
      m_sectionLibId(sectionLibId),
      m_sectionExpanded(true),
      m_sectionEntries(parent)
{
//    if (collapsedStateHash.contains(sectionName))
//        m_sectionExpanded=  collapsedStateHash.value(sectionName);
}


QString ItemLibrarySectionModel::sectionName() const
{
    return m_name;
}

int ItemLibrarySectionModel::sectionLibId() const
{
    return m_sectionLibId;
}

bool ItemLibrarySectionModel::sectionExpanded() const
{
    return m_sectionExpanded;
}

QVariant ItemLibrarySectionModel::sortingRole() const
{

    if (sectionName() == QStringLiteral("QML Components")) //Qml Components always come first
        return QVariant(QStringLiteral("AA.this_comes_first"));

    return sectionName();
}

void ItemLibrarySectionModel::addSectionEntry(ItemLibraryItemModel *sectionEntry)
{
    m_sectionEntries.addElement(sectionEntry, sectionEntry->itemLibId());
}


void ItemLibrarySectionModel::removeSectionEntry(int itemLibId)
{
    m_sectionEntries.removeElement(itemLibId);
}

QObject *ItemLibrarySectionModel::sectionEntries()
{
    return &m_sectionEntries;
}

int ItemLibrarySectionModel::visibleItemIndex(int itemLibId)
{
    return m_sectionEntries.visibleElementPosition(itemLibId);
}


bool ItemLibrarySectionModel::isItemVisible(int itemLibId)
{
    return m_sectionEntries.elementVisible(itemLibId);
}


bool ItemLibrarySectionModel::updateSectionVisibility(const QString &searchText, bool *changed)
{
    bool haveVisibleItems = false;

    *changed = false;

    QMap<int, QObject *>::const_iterator itemIt = m_sectionEntries.elements().constBegin();
    while (itemIt != m_sectionEntries.elements().constEnd()) {

//        bool itemVisible = m_sectionEntries.elementByType<ItemLibraryItemModel*>(
//                    itemIt.key())->itemName().toLower().contains(searchText);

        bool itemChanged = false;
//        itemChanged = m_sectionEntries.setElementVisible(itemIt.key(), itemVisible);

        *changed |= itemChanged;

//        if (itemVisible)
//            haveVisibleItems = true;

        ++itemIt;
    }

    m_sectionEntries.resetModel();

    emit sectionEntriesChanged();

    return haveVisibleItems;
}


void ItemLibrarySectionModel::updateItemIconSize(const QSize &itemIconSize)
{
//    foreach (ItemLibraryItemModel* itemLibraryItemModel, m_sectionEntries.elementsByType<ItemLibraryItemModel*>()) {
//        itemLibraryItemModel->setItemIconSize(itemIconSize);
//    }
}

} // namespace QmlDesigner
