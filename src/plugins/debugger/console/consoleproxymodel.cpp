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

#include "consoleproxymodel.h"
#include "consoleitemmodel.h"

namespace Debugger {
namespace Internal {

ConsoleProxyModel::ConsoleProxyModel(QObject *parent) :
    QSortFilterProxyModel(parent),
    m_filter(ConsoleItem::AllTypes)
{
}

void ConsoleProxyModel::setShowLogs(bool show)
{
    m_filter = show ? (m_filter | ConsoleItem::DebugType)
                    : (m_filter & ~ConsoleItem::DebugType);
    invalidateFilter();
}

void ConsoleProxyModel::setShowWarnings(bool show)
{
    m_filter = show ? (m_filter | ConsoleItem::WarningType)
                    : (m_filter & ~ConsoleItem::WarningType);
    invalidateFilter();
}

void ConsoleProxyModel::setShowErrors(bool show)
{
    m_filter = show ? (m_filter | ConsoleItem::ErrorType)
                    : (m_filter & ~ConsoleItem::ErrorType);
    invalidateFilter();
}

void ConsoleProxyModel::selectEditableRow(const QModelIndex &index,
                           QItemSelectionModel::SelectionFlags command)
{
    emit setCurrentIndex(mapFromSource(index), command);
}

bool ConsoleProxyModel::filterAcceptsRow(int sourceRow,
         const QModelIndex &sourceParent) const
 {
     QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
     return m_filter.testFlag((ConsoleItem::ItemType)sourceModel()->data(
                                  index, ConsoleItem::TypeRole).toInt());
 }

void ConsoleProxyModel::onRowsInserted(const QModelIndex &index, int start, int end)
{
    int rowIndex = end;
    do {
        if (filterAcceptsRow(rowIndex, index)) {
            emit scrollToBottom();
            break;
        }
    } while (--rowIndex >= start);
}

} // Internal
} // Debugger
