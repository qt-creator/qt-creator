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

#include "threaddata.h"

#include <utils/treemodel.h>

////////////////////////////////////////////////////////////////////////
//
// ThreadsHandler
//
////////////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

class DebuggerEngine;
class GdbMi;
class ThreadItem;

class ThreadsHandler : public Utils::TreeModel<Utils::TypedTreeItem<ThreadItem>, ThreadItem>
{
    Q_OBJECT

public:
    explicit ThreadsHandler(DebuggerEngine *engine);

    int currentThreadIndex() const;
    ThreadId currentThread() const;
    ThreadId threadAt(int index) const;
    void setCurrentThread(ThreadId id);
    QString pidForGroupId(const QString &groupId) const;

    void updateThread(const ThreadData &threadData);
    void updateThreads(const GdbMi &data);

    void removeThread(ThreadId threadId);
    void setThreads(const Threads &threads);
    void removeAll();
    ThreadData thread(ThreadId id) const;
    QAbstractItemModel *model();

    void notifyGroupCreated(const QString &groupId, const QString &pid);
    bool notifyGroupExited(const QString &groupId); // Returns true when empty.

    // Clear out all frame information
    void notifyRunning(const QString &data);
    void notifyRunning(ThreadId threadId);
    void notifyAllRunning();

    void notifyStopped(const QString &data);
    void notifyStopped(ThreadId threadId);
    void notifyAllStopped();

    void resetLocation();
    void scheduleResetLocation();

private:
    void updateThreadBox();

    void sort(int column, Qt::SortOrder order);
    bool setData(const QModelIndex &idx, const QVariant &data, int role);

    DebuggerEngine *m_engine;
    ThreadId m_currentId;
    bool m_resetLocationScheduled;
    QHash<QString, QString> m_pidForGroupId;
};

} // namespace Internal
} // namespace Debugger
