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

#ifndef THREADSHANDLER_H
#define THREADSHANDLER_H

#include <QAbstractTableModel>
#include <QIcon>

#include "threaddata.h"

QT_BEGIN_NAMESPACE
class QSortFilterProxyModel;
QT_END_NAMESPACE

////////////////////////////////////////////////////////////////////////
//
// ThreadsHandler
//
////////////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

class GdbMi;

class ThreadsHandler : public QAbstractTableModel
{
    Q_OBJECT

public:
    ThreadsHandler();

    int currentThreadIndex() const { return m_currentIndex; }
    ThreadId currentThread() const;
    ThreadId threadAt(int index) const;
    void setCurrentThread(ThreadId id);

    void updateThread(const ThreadData &thread);
    void updateThreads(const GdbMi &data);

    void removeThread(ThreadId threadId);
    void setThreads(const Threads &threads);
    void removeAll();
    Threads threads() const;
    ThreadData thread(ThreadId id) const;
    QAbstractItemModel *model();

    // Clear out all frame information
    void notifyRunning(const QByteArray &data);
    void notifyRunning(ThreadId id);
    void notifyAllRunning();

    void notifyStopped(const QByteArray &data);
    void notifyStopped(ThreadId id);
    void notifyAllStopped();

    void resetLocation();
    void scheduleResetLocation();

private:
    int indexOf(ThreadId threadId) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    void updateThreadBox();
    void dataChanged(int index);

    Threads m_threads;
    int m_currentIndex;
    const QIcon m_positionIcon;
    const QIcon m_emptyIcon;

    bool m_resetLocationScheduled;

    //QSortFilterProxyModel *m_proxyModel;
};

} // namespace Internal
} // namespace Debugger

#endif // THREADSHANDLER_H
