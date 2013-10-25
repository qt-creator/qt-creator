/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "debuggeritemmodel.h"

#include "debuggeritem.h"
#include "debuggeritemmanager.h"

#include <utils/qtcassert.h>

namespace Debugger {
namespace Internal {

static QList<QStandardItem *> describeItem(const DebuggerItem &item)
{
    QList<QStandardItem *> row;
    row.append(new QStandardItem(item.displayName()));
    row.append(new QStandardItem(item.command().toUserOutput()));
    row.append(new QStandardItem(item.engineTypeName()));
    row.at(0)->setData(item.id());
    row.at(0)->setEditable(false);
    row.at(1)->setEditable(false);
    row.at(2)->setEditable(false);
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

void DebuggerItemModel::markCurrentDirty()
{
    QStandardItem *sitem = currentStandardItem();
    QTC_ASSERT(sitem, return);
    QFont font = sitem->font();
    font.setBold(true);
    sitem->setFont(font);
}

void DebuggerItemModel::addDebugger(const DebuggerItem &item)
{
    QTC_ASSERT(item.id().isValid(), return);
    QList<QStandardItem *> row = describeItem(item);
    (item.isAutoDetected() ? m_autoRoot : m_manualRoot)->appendRow(row);
    emit debuggerAdded(item.id(), item.displayName());
}

void DebuggerItemModel::removeDebugger(const QVariant &id)
{
    QStandardItem *sitem = findStandardItemById(id);
    QTC_ASSERT(sitem, return);
    QStandardItem *parent = sitem->parent();
    QTC_ASSERT(parent, return);
    // This will trigger a change of m_currentDebugger via changing the
    // view selection.
    parent->removeRow(sitem->row());
    emit debuggerRemoved(id);
}

void DebuggerItemModel::updateDebugger(const QVariant &id)
{
    QList<DebuggerItem> debuggers = DebuggerItemManager::debuggers();
    for (int i = 0, n = debuggers.size(); i != n; ++i) {
        DebuggerItem &item = debuggers[i];
        if (item.id() == id) {
            QStandardItem *sitem = findStandardItemById(id);
            QTC_ASSERT(sitem, return);
            QStandardItem *parent = sitem->parent();
            QTC_ASSERT(parent, return);
            int row = sitem->row();
            QFont font = sitem->font();
            font.setBold(false);
            parent->child(row, 0)->setData(item.displayName(), Qt::DisplayRole);
            parent->child(row, 0)->setFont(font);
            parent->child(row, 1)->setData(item.command().toUserOutput(), Qt::DisplayRole);
            parent->child(row, 1)->setFont(font);
            parent->child(row, 2)->setData(item.engineTypeName(), Qt::DisplayRole);
            parent->child(row, 2)->setFont(font);
            emit debuggerUpdated(id, item.displayName());
            return;
        }
    }
}

void DebuggerItemModel::setCurrentIndex(const QModelIndex &index)
{
    QStandardItem *sit = itemFromIndex(index);
    m_currentDebugger = sit ? sit->data() : QVariant();
}

} // namespace Internal
} // namespace Debugger
