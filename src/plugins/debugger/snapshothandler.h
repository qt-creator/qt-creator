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

#ifndef DEBUGGER_SNAPSHOTHANDLER_H
#define DEBUGGER_SNAPSHOTHANDLER_H

#include <QAbstractTableModel>
#include <QPointer>

namespace Debugger {

class DebuggerEngine;

namespace Internal {

////////////////////////////////////////////////////////////////////////
//
// SnapshotModel
//
////////////////////////////////////////////////////////////////////////


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
    void appendSnapshot(DebuggerEngine *engine);
    void removeSnapshot(DebuggerEngine *engine);
    void setCurrentIndex(int index);
    int size() const { return m_snapshots.size(); }
    DebuggerEngine *at(int index) const;

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
    Q_SLOT void resetModel() { beginResetModel(); endResetModel(); }

    int m_currentIndex;
    QList< QPointer<DebuggerEngine> > m_snapshots;
    const QVariant m_positionIcon;
    const QVariant m_emptyIcon;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_SNAPSHOTHANDLER_H
