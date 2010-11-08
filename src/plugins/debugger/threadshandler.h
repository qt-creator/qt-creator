/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef THREADSHANDLER_H
#define THREADSHANDLER_H

#include <QtCore/QAbstractTableModel>
#include <QtCore/QList>

#include <QtGui/QIcon>

#include "threaddata.h"

namespace Debugger {
namespace Internal {

////////////////////////////////////////////////////////////////////////
//
// ThreadsHandler
//
////////////////////////////////////////////////////////////////////////

/*! A model to represent the running threads in a QTreeView or ComboBox */
class ThreadsHandler : public QAbstractTableModel
{
    Q_OBJECT

public:
    ThreadsHandler();

    int currentThread() const { return m_currentIndex; }
    void setCurrentThread(int index);
    int currentThreadId() const;
    void setCurrentThreadId(int id);
    int indexOf(quint64 threadId) const;

    void setThreads(const Threads &threads);
    void removeAll();
    Threads threads() const;
    QAbstractItemModel *model() { return this; }

    // Clear out all frame information
    void notifyRunning();

private:
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const;

    Threads m_threads;
    int m_currentIndex;
    const QIcon m_positionIcon;
    const QIcon m_emptyIcon;
};

} // namespace Internal
} // namespace Debugger

#endif // THREADSHANDLER_H
