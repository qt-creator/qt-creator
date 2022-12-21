// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <debugger/debugger_global.h>

#include <QTreeView>
#include <QStyledItemDelegate>

namespace Debugger {

class DiagnosticLocation;

class DEBUGGER_EXPORT DetailedErrorView : public QTreeView
{
public:
    DetailedErrorView(QWidget *parent = nullptr);
    ~DetailedErrorView() override;

    virtual void goNext();
    virtual void goBack();

    void selectIndex(const QModelIndex &index);

    enum ItemRole {
        LocationRole = Qt::UserRole,
        FullTextRole
    };

    enum Column {
        DiagnosticColumn,
        LocationColumn,
    };

    static QVariant locationData(int role, const DiagnosticLocation &location);

private:
    void contextMenuEvent(QContextMenuEvent *e) override;
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) override;

    int currentRow() const;
    void setCurrentRow(int row);
    int rowCount() const;

    QList<QAction *> commonActions() const;
    virtual QList<QAction *> customActions() const;

    QAction * const m_copyAction;
};

} // namespace Debugger
