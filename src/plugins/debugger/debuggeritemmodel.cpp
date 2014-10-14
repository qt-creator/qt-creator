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
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "debuggeritemmodel.h"

#include "debuggeritem.h"
#include "debuggeritemmanager.h"

#include <utils/qtcassert.h>

namespace Debugger {
namespace Internal {

const int AbiRole = Qt::UserRole + 2;

static QList<QStandardItem *> describeItem(const DebuggerItem &item)
{
    QList<QStandardItem *> row;
    row.append(new QStandardItem(item.displayName()));
    row.append(new QStandardItem(item.command().toUserOutput()));
    row.append(new QStandardItem(item.engineTypeName()));
    row.at(0)->setData(item.id());
    row.at(0)->setData(item.abiNames(), AbiRole);
    row.at(0)->setEditable(false);
    row.at(1)->setEditable(false);
    row.at(1)->setData(item.toMap());
    row.at(2)->setEditable(false);
    row.at(2)->setData(static_cast<int>(item.engineType()));
    row.at(0)->setSelectable(true);
    row.at(1)->setSelectable(true);
    row.at(2)->setSelectable(true);
    return row;
}

static QList<QStandardItem *> createRow(const QString &display)
{
    QList<QStandardItem *> row;
    row.append(new QStandardItem(display));
    row.append(new QStandardItem());
    row.append(new QStandardItem());
    row.at(0)->setEditable(false);
    row.at(1)->setEditable(false);
    row.at(2)->setEditable(false);
    row.at(0)->setSelectable(false);
    row.at(1)->setSelectable(false);
    row.at(2)->setSelectable(false);
    return row;
}

// --------------------------------------------------------------------------
// DebuggerItemModel
// --------------------------------------------------------------------------

DebuggerItemModel::DebuggerItemModel(QObject *parent)
    : QStandardItemModel(parent)
{
    setColumnCount(3);

    QList<QStandardItem *> row = createRow(tr("Auto-detected"));
    m_autoRoot = row.at(0);
    appendRow(row);

    row = createRow(tr("Manual"));
    m_manualRoot = row.at(0);
    appendRow(row);

    foreach (const DebuggerItem &item, DebuggerItemManager::debuggers())
        addDebuggerStandardItem(item, false);

    QObject *manager = DebuggerItemManager::instance();
    connect(manager, SIGNAL(debuggerAdded(QVariant)),
            this, SLOT(onDebuggerAdded(QVariant)));
    connect(manager, SIGNAL(debuggerUpdated(QVariant)),
            this, SLOT(onDebuggerUpdate(QVariant)));
    connect(manager, SIGNAL(debuggerRemoved(QVariant)),
            this, SLOT(onDebuggerRemoval(QVariant)));
}

QVariant DebuggerItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return tr("Name");
        case 1:
            return tr("Path");
        case 2:
            return tr("Type");
        }
    }
    return QVariant();
}

bool DebuggerItemModel::addDebuggerStandardItem(const DebuggerItem &item, bool changed)
{
    if (findStandardItemById(item.id()))
        return false;

    QList<QStandardItem *> row = describeItem(item);
    foreach (QStandardItem *cell, row) {
        QFont font = cell->font();
        font.setBold(changed);
        cell->setFont(font);
    }
    (item.isAutoDetected() ? m_autoRoot : m_manualRoot)->appendRow(row);
    return true;
}

bool DebuggerItemModel::removeDebuggerStandardItem(const QVariant &id)
{
    QStandardItem *sitem = findStandardItemById(id);
    QTC_ASSERT(sitem, return false);
    QStandardItem *parent = sitem->parent();
    QTC_ASSERT(parent, return false);
    // This will trigger a change of m_currentDebugger via changing the
    // view selection.
    parent->removeRow(sitem->row());
    return true;
}

bool DebuggerItemModel::updateDebuggerStandardItem(const DebuggerItem &item, bool changed)
{
    QStandardItem *sitem = findStandardItemById(item.id());
    QTC_ASSERT(sitem, return false);
    QStandardItem *parent = sitem->parent();
    QTC_ASSERT(parent, return false);

    // Do not mark items as changed if they actually are not:
    const DebuggerItem *orig = DebuggerItemManager::findById(item.id());
    if (orig && *orig == item)
        changed = false;

    int row = sitem->row();
    QFont font = sitem->font();
    font.setBold(changed);
    parent->child(row, 0)->setData(item.displayName(), Qt::DisplayRole);
    parent->child(row, 0)->setData(item.abiNames(), AbiRole);
    parent->child(row, 0)->setFont(font);
    parent->child(row, 1)->setData(item.command().toUserOutput(), Qt::DisplayRole);
    parent->child(row, 1)->setFont(font);
    parent->child(row, 2)->setData(item.engineTypeName(), Qt::DisplayRole);
    parent->child(row, 2)->setData(static_cast<int>(item.engineType()));
    parent->child(row, 2)->setFont(font);
    return true;
}

DebuggerItem DebuggerItemModel::debuggerItem(QStandardItem *sitem) const
{
    DebuggerItem item = DebuggerItem(QVariant());
    if (sitem && sitem->parent()) {
        item.setAutoDetected(sitem->parent() == m_autoRoot);

        QStandardItem *i = sitem->parent()->child(sitem->row(), 0);
        item.m_id = i->data();
        item.setDisplayName(i->data(Qt::DisplayRole).toString());

        QStringList abis = i->data(AbiRole).toStringList();
        QList<ProjectExplorer::Abi> abiList;
        foreach (const QString &abi, abis)
            abiList << ProjectExplorer::Abi(abi);
        item.setAbis(abiList);

        i = sitem->parent()->child(sitem->row(), 1);
        item.setCommand(Utils::FileName::fromUserInput(i->data(Qt::DisplayRole).toString()));

        i = sitem->parent()->child(sitem->row(), 2);
        item.setEngineType(static_cast<DebuggerEngineType>(i->data().toInt()));
    }
    return item;
}

QList<DebuggerItem> DebuggerItemModel::debuggerItems() const
{
    QList<DebuggerItem> result;
    for (int i = 0, n = m_autoRoot->rowCount(); i != n; ++i)
        result << debuggerItem(m_autoRoot->child(i));
    for (int i = 0, n = m_manualRoot->rowCount(); i != n; ++i)
        result << debuggerItem(m_manualRoot->child(i));
    return result;
}

QStandardItem *DebuggerItemModel::currentStandardItem() const
{
    return findStandardItemById(m_currentDebugger);
}

QStandardItem *DebuggerItemModel::findStandardItemById(const QVariant &id) const
{
    for (int i = 0, n = m_autoRoot->rowCount(); i != n; ++i) {
        QStandardItem *sitem = m_autoRoot->child(i);
        if (sitem->data() == id)
            return sitem;
    }
    for (int i = 0, n = m_manualRoot->rowCount(); i != n; ++i) {
        QStandardItem *sitem = m_manualRoot->child(i);
        if (sitem->data() == id)
            return sitem;
    }
    return 0;
}

QModelIndex DebuggerItemModel::currentIndex() const
{
    QStandardItem *current = currentStandardItem();
    return current ? current->index() : QModelIndex();
}

QModelIndex DebuggerItemModel::lastIndex() const
{
    int n = m_manualRoot->rowCount();
    QStandardItem *current = m_manualRoot->child(n - 1);
    return current ? current->index() : QModelIndex();
}

void DebuggerItemModel::onDebuggerAdded(const QVariant &id)
{
    const DebuggerItem *item = DebuggerItemManager::findById(id);
    QTC_ASSERT(item, return);
    if (!addDebuggerStandardItem(*item, false))
        updateDebuggerStandardItem(*item, false); // already had it added, so just update it.
}

void DebuggerItemModel::onDebuggerUpdate(const QVariant &id)
{
    const DebuggerItem *item = DebuggerItemManager::findById(id);
    QTC_ASSERT(item, return);
    updateDebuggerStandardItem(*item, false);
}

void DebuggerItemModel::onDebuggerRemoval(const QVariant &id)
{
    removeDebuggerStandardItem(id);
}

void DebuggerItemModel::addDebugger(const DebuggerItem &item)
{
    addDebuggerStandardItem(item, true);
}

void DebuggerItemModel::removeDebugger(const QVariant &id)
{
    if (!removeDebuggerStandardItem(id)) // Nothing there!
        return;

    if (DebuggerItemManager::findById(id))
        m_removedItems.append(id);
}

void DebuggerItemModel::updateDebugger(const DebuggerItem &item)
{
    updateDebuggerStandardItem(item, true);
}

void DebuggerItemModel::apply()
{
    foreach (const QVariant &id, m_removedItems)
        DebuggerItemManager::deregisterDebugger(id);

    foreach (const DebuggerItem &item, debuggerItems()) {
        const DebuggerItem *managed = DebuggerItemManager::findById(item.id());
        if (managed) {
            if (*managed == item)
                continue;
            else
                DebuggerItemManager::setItemData(item.id(), item.displayName(), item.command());
        } else {
            DebuggerItemManager::registerDebugger(item);
        }
    }
}

void DebuggerItemModel::setCurrentIndex(const QModelIndex &index)
{
    QStandardItem *sit = itemFromIndex(index);
    m_currentDebugger = sit ? sit->data() : QVariant();
}

DebuggerItem DebuggerItemModel::currentDebugger() const
{
    return debuggerItem(currentStandardItem());
}

} // namespace Internal
} // namespace Debugger
