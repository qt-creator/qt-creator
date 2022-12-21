// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

namespace Debugger::Internal {

class DebuggerEngine;
class GdbMi;
class ThreadsHandler;

class ThreadItem : public QObject, public Utils::TreeItem
{
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

} // Debugger::Internal
