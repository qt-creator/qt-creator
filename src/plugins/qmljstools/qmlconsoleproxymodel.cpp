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

#include "qmlconsoleproxymodel.h"
#include "qmlconsoleitemmodel.h"

using namespace QmlJS;

namespace QmlJSTools {
namespace Internal {

QmlConsoleProxyModel::QmlConsoleProxyModel(QObject *parent) :
    QSortFilterProxyModel(parent),
    m_filter(ConsoleItem::DefaultTypes)
{
}

void QmlConsoleProxyModel::setShowLogs(bool show)
{
    m_filter = show ? m_filter | ConsoleItem::DebugType : m_filter & ~ConsoleItem::DebugType;
    setFilterRegExp(QString());
}

void QmlConsoleProxyModel::setShowWarnings(bool show)
{
    m_filter = show ? m_filter | ConsoleItem::WarningType
                    : m_filter & ~ConsoleItem::WarningType;
    setFilterRegExp(QString());
}

void QmlConsoleProxyModel::setShowErrors(bool show)
{
    m_filter = show ? m_filter | ConsoleItem::ErrorType : m_filter & ~ConsoleItem::ErrorType;
    setFilterRegExp(QString());
}

void QmlConsoleProxyModel::selectEditableRow(const QModelIndex &index,
                           QItemSelectionModel::SelectionFlags command)
{
    emit setCurrentIndex(mapFromSource(index), command);
}

bool QmlConsoleProxyModel::filterAcceptsRow(int sourceRow,
         const QModelIndex &sourceParent) const
 {
     QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
     return m_filter.testFlag((ConsoleItem::ItemType)sourceModel()->data(
                                  index, QmlConsoleItemModel::TypeRole).toInt());
 }

void QmlConsoleProxyModel::onRowsInserted(const QModelIndex &index, int start, int end)
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
} // QmlJSTools
