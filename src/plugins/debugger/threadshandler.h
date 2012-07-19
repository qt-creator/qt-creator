/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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

    int currentThread() const { return m_currentIndex; }
    void setCurrentThread(int index);
    int currentThreadId() const;
    void setCurrentThreadId(int id);
    int indexOf(quint64 threadId) const;

    void setThreads(const Threads &threads);
    void removeAll();
    Threads threads() const;
    QAbstractItemModel *model();

    // Clear out all frame information
    void notifyRunning();

    static Threads parseGdbmiThreads(const GdbMi &data, int *currentThread = 0);

    void resetLocation();
    void scheduleResetLocation();

private:
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation,
        int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    void updateThreadBox();

    Threads m_threads;
    int m_currentIndex;
    const QIcon m_positionIcon;
    const QIcon m_emptyIcon;

    bool m_resetLocationScheduled;
    bool m_contentsValid;

    QSortFilterProxyModel *m_proxyModel;
};

} // namespace Internal
} // namespace Debugger

#endif // THREADSHANDLER_H
