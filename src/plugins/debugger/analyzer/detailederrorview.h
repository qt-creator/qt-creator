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

#include <debugger/debugger_global.h>

#include <QTreeView>
#include <QStyledItemDelegate>

namespace Debugger {

class DiagnosticLocation;

class DEBUGGER_EXPORT DetailedErrorView : public QTreeView
{
    Q_OBJECT

public:
    DetailedErrorView(QWidget *parent = nullptr);
    ~DetailedErrorView() override;

    void goNext();
    void goBack();

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
