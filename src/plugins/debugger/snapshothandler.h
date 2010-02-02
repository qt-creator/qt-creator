/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef DEBUGGER_SNAPSHOTHANDLER_H
#define DEBUGGER_SNAPSHOTHANDLER_H

#include "stackframe.h"

#include <QtCore/QAbstractItemModel>
#include <QtCore/QDateTime>
#include <QtCore/QObject>

#include <QtGui/QIcon>


namespace Debugger {
namespace Internal {

////////////////////////////////////////////////////////////////////////
//
// SnapshotData
//
////////////////////////////////////////////////////////////////////////

/*! An entry in the snapshot model. */

class SnapshotData
{
public:
    SnapshotData();

    void clear();
    QString toString() const;
    QString toToolTip() const;

    QDateTime date() const { return m_date; }
    void setDate(const QDateTime &date) { m_date = date; }

    void setLocation(const QString &location) { m_location = location; }
    QString location() const { return m_location; }

    void setFrames(const QList<StackFrame> &frames) { m_frames = frames; }
    QList<StackFrame> frames() const { return m_frames; }

    QString function() const; // Topmost entry.

private:
    QString m_location;         // Name of the core file.
    QDateTime m_date;           // Date of the snapshot
    QList<StackFrame> m_frames; // Stack frames.
};


////////////////////////////////////////////////////////////////////////
//
// SnapshotModel
//
////////////////////////////////////////////////////////////////////////

/*! A model to represent the snapshot in a QTreeView. */
class SnapshotHandler : public QAbstractTableModel
{
    Q_OBJECT

public:
    SnapshotHandler(QObject *parent = 0);
    ~SnapshotHandler();

    void setFrames(const QList<SnapshotData> &snapshots, bool canExpand = false);
    QList<SnapshotData> snapshots() const;

    // Called from SnapshotHandler after a new snapshot has been added
    void removeAll();
    void removeSnapshot(int index);
    QAbstractItemModel *model() { return this; }
    int currentIndex() const { return m_currentIndex; }
    void appendSnapshot(const SnapshotData &snapshot);
    SnapshotData setCurrentIndex(int index);

private:
    // QAbstractTableModel
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    Q_SLOT void resetModel() { reset(); }

    int m_currentIndex;
    QList<SnapshotData> m_snapshots;
    const QVariant m_positionIcon;
    const QVariant m_emptyIcon;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_SNAPSHOTHANDLER_H
