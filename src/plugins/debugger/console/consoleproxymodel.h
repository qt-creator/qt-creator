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

#pragma once

#include "consoleitem.h"

#include <QSortFilterProxyModel>
#include <QItemSelectionModel>

namespace Debugger {
namespace Internal {

class ConsoleProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit ConsoleProxyModel(QObject *parent);

    void setShowLogs(bool show);
    void setShowWarnings(bool show);
    void setShowErrors(bool show);
    void selectEditableRow(const QModelIndex &index,
                               QItemSelectionModel::SelectionFlags command);
    void onRowsInserted(const QModelIndex &index, int start, int end);

signals:
    void scrollToBottom();
    void setCurrentIndex(const QModelIndex &index,
                         QItemSelectionModel::SelectionFlags command);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;

private:
    QFlags<ConsoleItem::ItemType> m_filter;
};

} // Internal
} // Debugger
