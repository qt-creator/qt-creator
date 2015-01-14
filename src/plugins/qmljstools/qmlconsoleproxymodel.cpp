/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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
