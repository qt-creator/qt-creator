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

// ThreadItem

ThreadItem::ThreadItem(const ThreadData &data)
    : threadData(data)
{}

QVariant ThreadItem::data(int column, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
        if (column == 0)
            return QString("#%1 %2").arg(threadData.id).arg(threadData.name);
        return threadPart(column);
    case Qt::ToolTipRole:
        return threadToolTip();
    default:
        break;
    }
    return QVariant();
}

Qt::ItemFlags ThreadItem::flags(int column) const
{
    return threadData.stopped ? TreeItem::flags(column) : Qt::ItemFlags({});
}

QString ThreadItem::threadToolTip() const
{
    const char start[] = "<tr><td>";
    const char sep[] = "</td><td>";
    const char end[] = "</td>";
    QString rc;
    QTextStream str(&rc);
    str << "<html><head/><body><table>"
        << start << ThreadsHandler::tr("Thread&nbsp;id:")
        << sep << threadData.id << end;
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

QVariant ThreadItem::threadPart(int column) const
{
    switch (column) {
    case ThreadData::IdColumn:
        return threadData.id;
    case ThreadData::FunctionColumn:
        return threadData.function;
    case ThreadData::FileColumn:
        return threadData.fileName.isEmpty() ? threadData.module : threadData.fileName;
    case ThreadData::LineColumn:
        return threadData.lineNumber >= 0
                ? QString::number(threadData.lineNumber) : QString();
    case ThreadData::AddressColumn:
        return threadData.address > 0
                ? "0x" + QString::number(threadData.address, 16)
                : QString();
    case ThreadData::CoreColumn:
        return threadData.core;
    case ThreadData::StateColumn:
        return threadData.state;
    case ThreadData::TargetIdColumn:
        if (threadData.targetId.startsWith("Thread "))
            return threadData.targetId.mid(7);
        return threadData.targetId;
    case ThreadData::NameColumn:
        return threadData.name;
    case ThreadData::DetailsColumn:
        return threadData.details;
    case ThreadData::ComboNameColumn:
        return QString::fromLatin1("#%1 %2").arg(threadData.id).arg(threadData.name);
    }
    return QVariant();
}

void ThreadItem::notifyRunning() // Clear state information.
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

void ThreadItem::notifyStopped()
{
    threadData.stopped = true;
    update();
}

void ThreadItem::mergeThreadData(const ThreadData &other)
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


// ThreadsHandler

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
    setObjectName("ThreadsModel");
    setHeader({
                  "  " + tr("ID") + "  ",
                  tr("Address"), tr("Function"), tr("File"), tr("Line"), tr("State"),
                  tr("Name"), tr("Target ID"), tr("Details"), tr("Core"),
              });
}

ThreadsHandler::~ThreadsHandler()
{
    delete m_comboBox;
}

QVariant ThreadsHandler::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DecorationRole && index.column() == 0) {
        // Return icon that indicates whether this is the active thread.
        TreeItem *item = itemForIndex(index);
        if (item && item == m_currentThread)
            return Icons::LOCATION.icon();
        return Icons::EMPTY.icon();
    }

    return ThreadsHandlerModel::data(index, role);
}

bool ThreadsHandler::setData(const QModelIndex &idx, const QVariant &data, int role)
{
    if (role == BaseTreeView::ItemActivatedRole) {
        const Thread thread = itemForIndexAtLevel<1>(idx);
        if (thread != m_currentThread) {
            m_currentThread = thread;
            threadSwitcher()->setCurrentIndex(idx.row());
            m_engine->selectThread(thread);
        }
        return true;
    }

    if (role == BaseTreeView::ItemViewEventRole) {
        ItemViewEvent ev = data.value<ItemViewEvent>();

        if (ev.as<QContextMenuEvent>()) {
            auto menu = new QMenu;
            Internal::addHideColumnActions(menu, ev.view());
            menu->addAction(action(SettingsDialog));
            menu->popup(ev.globalPos());
            return true;
        }
    }

    return false;
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

Thread ThreadsHandler::currentThread() const
{
    return m_currentThread;
}

void ThreadsHandler::setCurrentThread(const Thread &thread)
{
    QTC_ASSERT(thread, return);
    if (thread == m_currentThread)
        return;

    if (!threadForId(thread->id())) {
        qWarning("ThreadsHandler::setCurrentThreadId: No such thread %s.", qPrintable(thread->id()));
        return;
    }

    m_currentThread = thread;
    thread->update();
}

void ThreadsHandler::notifyGroupCreated(const QString &groupId, const QString &pid)
{
    m_pidForGroupId[groupId] = pid;
}

void ThreadsHandler::updateThread(const ThreadData &threadData)
{
    if (Thread thread = threadForId(threadData.id))
        thread->mergeThreadData(threadData);
    else
        rootItem()->appendChild(new ThreadItem(threadData));
}

void ThreadsHandler::removeThread(const QString &id)
{
    if (Thread thread = threadForId(id))
        destroyItem(thread);
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

Thread ThreadsHandler::threadForId(const QString &id) const
{
    return findItemAtLevel<1>([id](const Thread &item) {
        return item->threadData.id == id;
    });
}

void ThreadsHandler::notifyRunning(const QString &id)
{
    if (id.isEmpty() || id == "all")
        forItemsAtLevel<1>([](const Thread &thread) { thread->notifyRunning(); });
    else if (Thread thread = threadForId(id))
        thread->notifyRunning();
}

void ThreadsHandler::notifyStopped(const QString &id)
{
    if (id.isEmpty() || id == "all")
        forItemsAtLevel<1>([](const Thread &thread) { thread->notifyStopped(); });
    else if (Thread thread = threadForId(id))
        thread->notifyStopped();
}

QPointer<QComboBox> ThreadsHandler::threadSwitcher()
{
    if (!m_comboBox) {
        m_comboBox = new QComboBox;
        m_comboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        m_comboBox->setModel(this);
        connect(m_comboBox, QOverload<int>::of(&QComboBox::activated), this, [this](int row) {
            setData(index(row, 0), {}, BaseTreeView::ItemActivatedRole);
        });
    }
    return m_comboBox;
}

void ThreadsHandler::setThreads(const GdbMi &data)
{
    rootItem()->removeChildren();

    // ^done,threads=[{id="1",target-id="Thread 0xb7fdc710 (LWP 4264)",
    // frame={level="0",addr="0x080530bf",func="testQString",args=[],
    // file="/.../app.cpp",fullname="/../app.cpp",line="1175"},
    // state="stopped",core="0"}],current-thread-id="1"

    const GdbMi &threads = data["threads"];
    for (const GdbMi &item : threads) {
        const GdbMi &frame = item["frame"];
        ThreadData thread;
        thread.id = item["id"].data();
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

    const QString &currentId = data["current-thread-id"].data();
    m_currentThread = threadForId(currentId);

    if (!m_currentThread && threads.childCount() > 0)
        m_currentThread = rootItem()->childAt(0);

    if (m_currentThread) {
        const QModelIndex currentThreadIndex = m_currentThread->index();
        threadSwitcher()->setCurrentIndex(currentThreadIndex.row());
    }
}

QAbstractItemModel *ThreadsHandler::model()
{
    return this;
}

} // namespace Internal
} // namespace Debugger
