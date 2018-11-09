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

#include <QComboBox>
#include <QPointer>

////////////////////////////////////////////////////////////////////////
//
// ThreadsHandler
//
////////////////////////////////////////////////////////////////////////

namespace Debugger {
namespace Internal {

class DebuggerEngine;
class GdbMi;
class ThreadsHandler;

class ThreadItem : public QObject, public Utils::TreeItem
{
    Q_OBJECT

public:
    ThreadItem(const ThreadData &data = ThreadData());

    QVariant data(int column, int role) const override;
    Qt::ItemFlags flags(int column) const override;

    QString threadToolTip() const;
    QVariant threadPart(int column) const;

    void notifyRunning();
    void notifyStopped();

    void mergeThreadData(const ThreadData &other);
    QString id() const { return threadData.id; }

public:
    ThreadData threadData;
};

using Thread = QPointer<ThreadItem>;
using ThreadsHandlerModel = Utils::TreeModel<Utils::TypedTreeItem<ThreadItem>, ThreadItem>;

class ThreadsHandler : public ThreadsHandlerModel
{
    Q_OBJECT

public:
    explicit ThreadsHandler(DebuggerEngine *engine);
    ~ThreadsHandler();

    Thread currentThread() const;
    Thread threadForId(const QString &id) const;
    void setCurrentThread(const Thread &thread);

    void updateThread(const ThreadData &threadData);
    void setThreads(const GdbMi &data);

    void removeThread(const QString &id);
    void removeAll();
    QAbstractItemModel *model();

    void notifyGroupCreated(const QString &groupId, const QString &pid);
    bool notifyGroupExited(const QString &groupId); // Returns true when empty.

    void notifyRunning(const QString &id);
    void notifyStopped(const QString &id);

    QPointer<QComboBox> threadSwitcher();

private:
    void sort(int column, Qt::SortOrder order) override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &idx, const QVariant &data, int role) override;

    DebuggerEngine *m_engine;
    Thread m_currentThread;
    QHash<QString, QString> m_pidForGroupId;
    QPointer<QComboBox> m_comboBox;
};

} // namespace Internal
} // namespace Debugger
