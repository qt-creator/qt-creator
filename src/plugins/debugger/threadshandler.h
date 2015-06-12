/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef THREADSHANDLER_H
#define THREADSHANDLER_H

#include "threaddata.h"

#include <utils/treemodel.h>

////////////////////////////////////////////////////////////////////////
//
// ThreadsHandler
//
////////////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

class GdbMi;

class ThreadsHandler : public Utils::TreeModel
{
    Q_OBJECT

public:
    ThreadsHandler();

    int currentThreadIndex() const;
    ThreadId currentThread() const;
    ThreadId threadAt(int index) const;
    void setCurrentThread(ThreadId id);
    QByteArray pidForGroupId(const QByteArray &groupId) const;

    void updateThread(const ThreadData &threadData);
    void updateThreads(const GdbMi &data);

    void removeThread(ThreadId threadId);
    void setThreads(const Threads &threads);
    void removeAll();
    ThreadData thread(ThreadId id) const;
    QAbstractItemModel *model();

    void notifyGroupCreated(const QByteArray &groupId, const QByteArray &pid);
    bool notifyGroupExited(const QByteArray &groupId); // Returns true when empty.

    // Clear out all frame information
    void notifyRunning(const QByteArray &data);
    void notifyRunning(ThreadId threadId);
    void notifyAllRunning();

    void notifyStopped(const QByteArray &data);
    void notifyStopped(ThreadId threadId);
    void notifyAllStopped();

    void resetLocation();
    void scheduleResetLocation();

private:
    void updateThreadBox();

    void sort(int column, Qt::SortOrder order);

    ThreadId m_currentId;
    bool m_resetLocationScheduled;
    QHash<QByteArray, QByteArray> m_pidForGroupId;
};

} // namespace Internal
} // namespace Debugger

#endif // THREADSHANDLER_H
