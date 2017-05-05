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

#include <QAbstractTableModel>
#include <QPointer>

namespace Debugger {

class DebuggerRunTool;

namespace Internal {

class SnapshotHandler : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit SnapshotHandler();
    ~SnapshotHandler();

    // Called from SnapshotHandler after a new snapshot has been added
    void removeAll();
    QAbstractItemModel *model() { return this; }
    int currentIndex() const { return m_currentIndex; }
    void appendSnapshot(DebuggerRunTool *runTool);
    void removeSnapshot(DebuggerRunTool *runTool);
    void setCurrentIndex(int index);
    int size() const { return m_snapshots.size(); }
    DebuggerRunTool *at(int index) const;

    void createSnapshot(int index);
    void activateSnapshot(int index);
    void removeSnapshot(int index);

private:
    // QAbstractTableModel
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    int m_currentIndex;
    QList< QPointer<DebuggerRunTool> > m_snapshots;
};

} // namespace Internal
} // namespace Debugger
