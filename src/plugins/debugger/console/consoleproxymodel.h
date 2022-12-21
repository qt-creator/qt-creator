// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "consoleitem.h"

#include <QSortFilterProxyModel>
#include <QItemSelectionModel>

namespace Debugger::Internal {

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
    bool filterAcceptsRow(int source_row,
                          const QModelIndex &source_parent) const override;

private:
    QFlags<ConsoleItem::ItemType> m_filter;
};

} // Debugger::Internal
