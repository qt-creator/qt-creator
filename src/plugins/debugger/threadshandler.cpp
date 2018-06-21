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

#include "threadshandler.h"

#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggericons.h"
#include "debuggerprotocol.h"
#include "watchutils.h"

#include <utils/algorithm.h>
#include <utils/basetreeview.h>
#include <utils/qtcassert.h>
#include <utils/savedaction.h>

#include <QCoreApplication>
#include <QDebug>
#include <QIcon>
#include <QMenu>

using namespace Utils;

namespace Debugger {
namespace Internal {

////////////////////////////////////////////////////////////////////////
//
// ThreadItem
//
///////////////////////////////////////////////////////////////////////

class ThreadItem : public TreeItem
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::ThreadsHandler)

public:
    ThreadItem(const ThreadsHandler *handler, const ThreadData &data = ThreadData())
        : threadData(data), handler(handler)
    {}

    QVariant data(int column, int role) const
    {
        switch (role) {
        case Qt::DisplayRole:
            return threadPart(column);
        case Qt::ToolTipRole:
            return threadToolTip();
        case Qt::DecorationRole:
            // Return icon that indicates whether this is the active stack frame.
            if (column == 0)
                return threadData.id == handler->currentThread() ? Icons::LOCATION.icon()
                                                                 : Icons::EMPTY.icon();
            break;
        case ThreadData::IdRole:
            return threadData.id.raw();
        default:
            break;
        }
        return QVariant();
    }

    Qt::ItemFlags flags(int column) const
    {
        return threadData.stopped ? TreeItem::flags(column) : Qt::ItemFlags(0);
    }

    QString threadToolTip() const
    {
        const char start[] = "<tr><td>";
        const char sep[] = "</td><td>";
        const char end[] = "</td>";
        QString rc;
        QTextStream str(&rc);
        str << "<html><head/><body><table>"
            << start << ThreadsHandler::tr("Thread&nbsp;id:")
            << sep << threadData.id.raw() << end;
        if (!threadData.targetId.isEmpty())
            str << start << ThreadsHandler::tr("Target&nbsp;id:")
                << sep << threadData.targetId << end;
        if (!threadData.groupId.isEmpty())
            str << start << ThreadsHandler::tr("Group&nbsp;id:")
                << sep << threadData.groupId << end;
        if (!threadData.name.isEmpty())
            str << start << ThreadsHandler::tr("Name:")
                << sep << threadData.name << end;
        if (!threadData.state.isEmpty())
            str << start << ThreadsHandler::tr("State:")
                << sep << threadData.state << end;
        if (!threadData.core.isEmpty())
            str << start << ThreadsHandler::tr("Core:")
                << sep << threadData.core << end;
        if (threadData.address) {
            str << start << ThreadsHandler::tr("Stopped&nbsp;at:") << sep;
            if (!threadData.function.isEmpty())
                str << threadData.function << "<br>";
            if (!threadData.fileName.isEmpty())
                str << threadData.fileName << ':' << threadData.lineNumber << "<br>";
            str << formatToolTipAddress(threadData.address);
        }
        str << "</table></body></html>";
        return rc;
    }

    QVariant threadPart(int column) const
    {
        switch (column) {
        case ThreadData::IdColumn:
            return threadData.id.raw();
        case ThreadData::FunctionColumn:
            return threadData.function;
        case ThreadData::FileColumn:
            return threadData.fileName.isEmpty() ? threadData.module : threadData.fileName;
        case ThreadData::LineColumn:
            return threadData.lineNumber >= 0
                    ? QString::number(threadData.lineNumber) : QString();
        case ThreadData::AddressColumn:
            return threadData.address > 0
                    ? QLatin1String("0x") + QString::number(threadData.address, 16)
                    : QString();
        case ThreadData::CoreColumn:
            return threadData.core;
        case ThreadData::StateColumn:
            return threadData.state;
        case ThreadData::TargetIdColumn:
            if (threadData.targetId.startsWith(QLatin1String("Thread ")))
                return threadData.targetId.mid(7);
            return threadData.targetId;
        case ThreadData::NameColumn:
            return threadData.name;
        case ThreadData::DetailsColumn:
            return threadData.details;
        case ThreadData::ComboNameColumn:
            return QString::fromLatin1("#%1 %2").arg(threadData.id.raw()).arg(threadData.name);
        }
        return QVariant();
    }

    void notifyRunning() // Clear state information.
    {
        threadData.address = 0;
        threadData.function.clear();
        threadData.fileName.clear();
        threadData.frameLevel = -1;
        threadData.state.clear();
        threadData.lineNumber = -1;
        threadData.stopped = false;
        update();
    }

    void notifyStopped()
    {
        threadData.stopped = true;
        update();
    }

    void mergeThreadData(const ThreadData &other)
    {
        if (!other.core.isEmpty())
            threadData.core = other.core;
        if (!other.fileName.isEmpty())
            threadData.fileName = other.fileName;
        if (!other.targetId.isEmpty())
            threadData.targetId = other.targetId;
        if (!other.name.isEmpty())
            threadData.name = other.name;
        if (other.frameLevel != -1)
            threadData.frameLevel = other.frameLevel;
        if (!other.function.isEmpty())
            threadData.function = other.function;
        if (other.address)
            threadData.address = other.address;
        if (!other.module.isEmpty())
            threadData.module = other.module;
        if (!other.details.isEmpty())
            threadData.details = other.details;
        if (!other.state.isEmpty())
            threadData.state = other.state;
        if (other.lineNumber != -1)
            threadData.lineNumber = other.lineNumber;
        update();
    }


public:
    ThreadData threadData;
    const ThreadsHandler * const handler;
};

////////////////////////////////////////////////////////////////////////
//
// ThreadsHandler
//
///////////////////////////////////////////////////////////////////////

/*!
    \class Debugger::Internal::ThreadData
    \internal
    \brief  The ThreadData class contains information
            about a single thread.
*/

/*!
    \class Debugger::Internal::ThreadsHandler
    \internal
    \brief  The ThreadsHandler class provides a model to
            represent the running threads in a QTreeView or ComboBox.
*/

ThreadsHandler::ThreadsHandler(DebuggerEngine *engine)
    : m_engine(engine)
{
    m_resetLocationScheduled = false;
    setObjectName(QLatin1String("ThreadsModel"));
    setHeader({
        QLatin1String("  ") + tr("ID") + QLatin1String("  "),
        tr("Address"), tr("Function"), tr("File"), tr("Line"), tr("State"),
        tr("Name"), tr("Target ID"), tr("Details"), tr("Core"),
    });
}

bool ThreadsHandler::setData(const QModelIndex &idx, const QVariant &data, int role)
{
    if (role == BaseTreeView::ItemActivatedRole) {
        ThreadId id = ThreadId(idx.data(ThreadData::IdRole).toLongLong());
        m_engine->selectThread(id);
        return true;
    }

    if (role == BaseTreeView::ItemViewEventRole) {
        ItemViewEvent ev = data.value<ItemViewEvent>();

        if (ev.as<QContextMenuEvent>()) {
            auto menu = new QMenu;
            menu->addAction(action(SettingsDialog));
            menu->popup(ev.globalPos());
            return true;
        }
    }

    return false;
}

static ThreadItem *itemForThreadId(const ThreadsHandler *handler, ThreadId threadId)
{
    const auto matcher = [threadId](ThreadItem *item) { return item->threadData.id == threadId; };
    return handler->findItemAtLevel<1>(matcher);
}

static int indexForThreadId(const ThreadsHandler *handler, ThreadId threadId)
{
    ThreadItem *item = itemForThreadId(handler, threadId);
    return item ? handler->rootItem()->indexOf(item) : -1;
}

int ThreadsHandler::currentThreadIndex() const
{
    return indexForThreadId(this, m_currentId);
}

void ThreadsHandler::sort(int column, Qt::SortOrder order)
{
    rootItem()->sortChildren([order, column](const ThreadItem *item1, const ThreadItem *item2) -> bool {
        const QVariant v1 = item1->threadPart(column);
        const QVariant v2 = item2->threadPart(column);
        if (v1 == v2)
            return false;
        if (column == 0)
            return (v1.toInt() < v2.toInt()) ^ (order == Qt::DescendingOrder);
        // FIXME: Use correct toXXX();
        return (v1.toString() < v2.toString()) ^ (order == Qt::DescendingOrder);
    });
}

ThreadId ThreadsHandler::currentThread() const
{
    return m_currentId;
}

ThreadId ThreadsHandler::threadAt(int index) const
{
    QTC_ASSERT(index >= 0 && index < rootItem()->childCount(), return ThreadId());
    return rootItem()->childAt(index)->threadData.id;
}

void ThreadsHandler::setCurrentThread(ThreadId id)
{
    if (id == m_currentId)
        return;

    ThreadItem *newItem = itemForThreadId(this, id);
    if (!newItem) {
        qWarning("ThreadsHandler::setCurrentThreadId: No such thread %d.", int(id.raw()));
        return;
    }

    ThreadItem *oldItem = itemForThreadId(this, m_currentId);
    m_currentId = id;
    if (oldItem)
        oldItem->update();

    newItem->update();

    updateThreadBox();
}

QString ThreadsHandler::pidForGroupId(const QString &groupId) const
{
    return m_pidForGroupId[groupId];
}

void ThreadsHandler::notifyGroupCreated(const QString &groupId, const QString &pid)
{
    m_pidForGroupId[groupId] = pid;
}

void ThreadsHandler::updateThread(const ThreadData &threadData)
{
    if (ThreadItem *item = itemForThreadId(this, threadData.id))
        item->mergeThreadData(threadData);
    else
        rootItem()->appendChild(new ThreadItem(this, threadData));
}

void ThreadsHandler::removeThread(ThreadId threadId)
{
    if (ThreadItem *item = itemForThreadId(this, threadId))
        destroyItem(item);
}

void ThreadsHandler::setThreads(const Threads &threads)
{
    auto root = new ThreadItem(this);
    for (int i = 0, n = threads.size(); i < n; ++i)
        root->appendChild(new ThreadItem(this, threads.at(i)));
    rootItem()->removeChildren();
    setRootItem(root);
    m_resetLocationScheduled = false;
    updateThreadBox();
}

void ThreadsHandler::updateThreadBox()
{
    QStringList list;
    forItemsAtLevel<1>([&list](ThreadItem *item) {
        list.append(QString::fromLatin1("#%1 %2").arg(item->threadData.id.raw()).arg(item->threadData.name));
    });
    Internal::setThreadBoxContents(list, indexForThreadId(this, m_currentId));
}

ThreadData ThreadsHandler::thread(ThreadId id) const
{
    if (ThreadItem *item = itemForThreadId(this, id))
        return item->threadData;
    return ThreadData();
}

void ThreadsHandler::removeAll()
{
    rootItem()->removeChildren();
}

bool ThreadsHandler::notifyGroupExited(const QString &groupId)
{
    QList<ThreadItem *> list;
    forItemsAtLevel<1>([&list, groupId](ThreadItem *item) {
        if (item->threadData.groupId == groupId)
            list.append(item);
    });
    foreach (ThreadItem *item, list)
        destroyItem(item);

    m_pidForGroupId.remove(groupId);
    return m_pidForGroupId.isEmpty();
}

void ThreadsHandler::notifyRunning(const QString &data)
{
    if (data.isEmpty() || data == "all") {
        notifyAllRunning();
    } else {
        bool ok;
        qlonglong id = data.toLongLong(&ok);
        if (ok)
            notifyRunning(ThreadId(id));
        else // FIXME
            notifyAllRunning();
    }
}

void ThreadsHandler::notifyAllRunning()
{
    forItemsAtLevel<1>([](ThreadItem *item) { item->notifyRunning(); });
}

void ThreadsHandler::notifyRunning(ThreadId threadId)
{
    if (ThreadItem *item = itemForThreadId(this, threadId))
        item->notifyRunning();
}

void ThreadsHandler::notifyStopped(const QString &data)
{
    if (data.isEmpty() || data == "all") {
        notifyAllStopped();
    } else {
        bool ok;
        qlonglong id = data.toLongLong(&ok);
        if (ok)
            notifyRunning(ThreadId(id));
        else // FIXME
            notifyAllStopped();
    }
}

void ThreadsHandler::notifyAllStopped()
{
    forItemsAtLevel<1>([](ThreadItem *item) { item->notifyStopped(); });
}

void ThreadsHandler::notifyStopped(ThreadId threadId)
{
    if (ThreadItem *item = itemForThreadId(this, threadId))
        item->notifyStopped();
}

void ThreadsHandler::updateThreads(const GdbMi &data)
{
    // ^done,threads=[{id="1",target-id="Thread 0xb7fdc710 (LWP 4264)",
    // frame={level="0",addr="0x080530bf",func="testQString",args=[],
    // file="/.../app.cpp",fullname="/../app.cpp",line="1175"},
    // state="stopped",core="0"}],current-thread-id="1"

    const QVector<GdbMi> items = data["threads"].children();
    const int n = int(items.size());
    for (int index = 0; index != n; ++index) {
        const GdbMi item = items[index];
        const GdbMi frame = item["frame"];
        ThreadData thread;
        thread.id = ThreadId(item["id"].toInt());
        thread.targetId = item["target-id"].data();
        thread.details = item["details"].data();
        thread.core = item["core"].data();
        thread.state = item["state"].data();
        thread.address = frame["addr"].toAddress();
        thread.function = frame["func"].data();
        thread.fileName = frame["fullname"].data();
        thread.lineNumber = frame["line"].toInt();
        thread.module = frame["from"].data();
        thread.name = item["name"].data();
        thread.stopped = thread.state != "running";
        updateThread(thread);
    }

    const GdbMi current = data["current-thread-id"];
    m_currentId = current.isValid() ? ThreadId(current.data().toLongLong()) : ThreadId();

    updateThreadBox();
}

void ThreadsHandler::scheduleResetLocation()
{
    m_resetLocationScheduled = true;
}

void ThreadsHandler::resetLocation()
{
    if (m_resetLocationScheduled) {
        m_resetLocationScheduled = false;
        layoutChanged();
    }
}

QAbstractItemModel *ThreadsHandler::model()
{
    return this;
}

} // namespace Internal
} // namespace Debugger
