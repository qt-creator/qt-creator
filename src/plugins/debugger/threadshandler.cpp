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

#include "threadshandler.h"

#include "debuggercore.h"
#include "debuggerprotocol.h"
#include "watchutils.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDebug>
#include <QIcon>

using namespace Utils;

namespace Debugger {
namespace Internal {

////////////////////////////////////////////////////////////////////////
//
// ThreadItem
//
///////////////////////////////////////////////////////////////////////

static const QIcon &positionIcon()
{
    static QIcon icon(QLatin1String(":/debugger/images/location_16.png"));
    return icon;
}

static const QIcon &emptyIcon()
{
    static QIcon icon(QLatin1String(":/debugger/images/debugger_empty_14.png"));
    return icon;
}

class ThreadItem : public TreeItem, public ThreadData
{
    Q_DECLARE_TR_FUNCTIONS(Debugger::Internal::ThreadsHandler)

public:
    ThreadItem(const ThreadsHandler *handler, const ThreadData &data = ThreadData())
        : ThreadData(data), handler(handler)
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
                return id == handler->currentThread() ? positionIcon() : emptyIcon();
            break;
        case ThreadData::IdRole:
            return id.raw();
        default:
            break;
        }
        return QVariant();
    }

    Qt::ItemFlags flags(int column) const
    {
        return stopped ? TreeItem::flags(column) : Qt::ItemFlags(0);
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
            << sep << id.raw() << end;
        if (!targetId.isEmpty())
            str << start << ThreadsHandler::tr("Target&nbsp;id:")
                << sep << targetId << end;
        if (!groupId.isEmpty())
            str << start << ThreadsHandler::tr("Group&nbsp;id:")
                << sep << groupId << end;
        if (!name.isEmpty())
            str << start << ThreadsHandler::tr("Name:")
                << sep << name << end;
        if (!state.isEmpty())
            str << start << ThreadsHandler::tr("State:")
                << sep << state << end;
        if (!core.isEmpty())
            str << start << ThreadsHandler::tr("Core:")
                << sep << core << end;
        if (address) {
            str << start << ThreadsHandler::tr("Stopped&nbsp;at:") << sep;
            if (!function.isEmpty())
                str << function << "<br>";
            if (!fileName.isEmpty())
                str << fileName << ':' << lineNumber << "<br>";
            str << formatToolTipAddress(address);
        }
        str << "</table></body></html>";
        return rc;
    }

    QVariant threadPart(int column) const
    {
        switch (column) {
        case ThreadData::IdColumn:
            return id.raw();
        case ThreadData::FunctionColumn:
            return function;
        case ThreadData::FileColumn:
            return fileName.isEmpty() ? module : fileName;
        case ThreadData::LineColumn:
            return lineNumber >= 0
                    ? QString::number(lineNumber) : QString();
        case ThreadData::AddressColumn:
            return address > 0
                    ? QLatin1String("0x") + QString::number(address, 16)
                    : QString();
        case ThreadData::CoreColumn:
            return core;
        case ThreadData::StateColumn:
            return state;
        case ThreadData::TargetIdColumn:
            if (targetId.startsWith(QLatin1String("Thread ")))
                return targetId.mid(7);
            return targetId;
        case ThreadData::NameColumn:
            return name;
        case ThreadData::DetailsColumn:
            return details;
        case ThreadData::ComboNameColumn:
            return QString::fromLatin1("#%1 %2").arg(id.raw()).arg(name);
        }
        return QVariant();
    }

    void notifyRunning() // Clear state information.
    {
        address = 0;
        function.clear();
        fileName.clear();
        frameLevel = -1;
        state.clear();
        lineNumber = -1;
        stopped = false;
        update();
    }

    void notifyStopped()
    {
        stopped = true;
        update();
    }

    void mergeThreadData(const ThreadData &other)
    {
        if (!other.core.isEmpty())
            core = other.core;
        if (!other.fileName.isEmpty())
            fileName = other.fileName;
        if (!other.targetId.isEmpty())
            targetId = other.targetId;
        if (!other.name.isEmpty())
            name = other.name;
        if (other.frameLevel != -1)
            frameLevel = other.frameLevel;
        if (!other.function.isEmpty())
            function = other.function;
        if (other.address)
            address = other.address;
        if (!other.module.isEmpty())
            module = other.module;
        if (!other.details.isEmpty())
            details = other.details;
        if (!other.state.isEmpty())
            state = other.state;
        if (other.lineNumber != -1)
            lineNumber = other.lineNumber;
        update();
    }


public:
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

ThreadsHandler::ThreadsHandler()
{
    m_resetLocationScheduled = false;
    setObjectName(QLatin1String("ThreadsModel"));
    setRootItem(new ThreadItem(this));
    setHeader({
        QLatin1String("  ") + tr("ID") + QLatin1String("  "),
        tr("Address"), tr("Function"), tr("File"), tr("Line"), tr("State"),
        tr("Name"), tr("Target ID"), tr("Details"), tr("Core"),
    });
}

static ThreadItem *itemForThreadId(const ThreadsHandler *handler, ThreadId threadId)
{
    const auto matcher = [threadId](ThreadItem *item) { return item->id == threadId; };
    return handler->findItemAtLevel<ThreadItem *>(1, matcher);
}

static int indexForThreadId(const ThreadsHandler *handler, ThreadId threadId)
{
    ThreadItem *item = itemForThreadId(handler, threadId);
    return item ? handler->rootItem()->children().indexOf(item) : -1;
}

int ThreadsHandler::currentThreadIndex() const
{
    return indexForThreadId(this, m_currentId);
}

void ThreadsHandler::sort(int column, Qt::SortOrder order)
{
    rootItem()->sortChildren([order, column](const TreeItem *item1, const TreeItem *item2) -> bool {
        const QVariant v1 = static_cast<const ThreadItem *>(item1)->threadPart(column);
        const QVariant v2 = static_cast<const ThreadItem *>(item2)->threadPart(column);
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
    return static_cast<ThreadItem *>(rootItem()->childAt(index))->id;
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

QByteArray ThreadsHandler::pidForGroupId(const QByteArray &groupId) const
{
    return m_pidForGroupId[groupId];
}

void ThreadsHandler::notifyGroupCreated(const QByteArray &groupId, const QByteArray &pid)
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
        delete takeItem(item);
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
    auto items = itemsAtLevel<ThreadItem *>(1);
    foreach (ThreadItem *item, items)
        list.append(QString::fromLatin1("#%1 %2").arg(item->id.raw()).arg(item->name));
    Internal::setThreadBoxContents(list, indexForThreadId(this, m_currentId));
}

ThreadData ThreadsHandler::thread(ThreadId id) const
{
    if (ThreadItem *item = itemForThreadId(this, id))
        return *item;
    return ThreadData();
}

void ThreadsHandler::removeAll()
{
    rootItem()->removeChildren();
}

bool ThreadsHandler::notifyGroupExited(const QByteArray &groupId)
{
    QList<ThreadItem *> list;
    auto items = itemsAtLevel<ThreadItem *>(1);
    foreach (ThreadItem *item, items)
        if (item->groupId == groupId)
            list.append(item);
    foreach (ThreadItem *item, list)
        delete takeItem(item);

    m_pidForGroupId.remove(groupId);
    return m_pidForGroupId.isEmpty();
}

void ThreadsHandler::notifyRunning(const QByteArray &data)
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
    auto items = itemsAtLevel<ThreadItem *>(1);
    foreach (ThreadItem *item, items)
        item->notifyRunning();
}

void ThreadsHandler::notifyRunning(ThreadId threadId)
{
    if (ThreadItem *item = itemForThreadId(this, threadId))
        item->notifyRunning();
}

void ThreadsHandler::notifyStopped(const QByteArray &data)
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
    auto items = itemsAtLevel<ThreadItem *>(1);
    foreach (ThreadItem *item, items)
        item->notifyStopped();
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

    const std::vector<GdbMi> items = data["threads"].children();
    const int n = int(items.size());
    for (int index = 0; index != n; ++index) {
        const GdbMi item = items[index];
        const GdbMi frame = item["frame"];
        ThreadData thread;
        thread.id = ThreadId(item["id"].toInt());
        thread.targetId = item["target-id"].toLatin1();
        thread.details = item["details"].toLatin1();
        thread.core = item["core"].toLatin1();
        thread.state = item["state"].toLatin1();
        thread.address = frame["addr"].toAddress();
        thread.function = frame["func"].toLatin1();
        thread.fileName = frame["fullname"].toLatin1();
        thread.lineNumber = frame["line"].toInt();
        thread.module = QString::fromLocal8Bit(frame["from"].data());
        thread.name = item["name"].toLatin1();
        thread.stopped = thread.state != QLatin1String("running");
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
