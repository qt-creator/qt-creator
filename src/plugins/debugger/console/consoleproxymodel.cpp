// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "consoleproxymodel.h"

namespace Debugger::Internal {

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

} // Debugger::Internal
