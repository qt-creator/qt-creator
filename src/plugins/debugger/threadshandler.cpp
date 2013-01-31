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

#include "threadshandler.h"

#include "debuggerconstants.h"
#include "debuggercore.h"
#include "debuggerprotocol.h"
#include "watchutils.h"

#include <utils/qtcassert.h>

#include <QDebug>
#include <QTextStream>
#include <QSortFilterProxyModel>

namespace Debugger {
namespace Internal {

void mergeThreadData(ThreadData &data, const ThreadData &other)
{
    if (!other.core.isEmpty())
        data.core = other.core;
    if (!other.fileName.isEmpty())
        data.fileName = other.fileName;
    if (!other.targetId.isEmpty())
        data.targetId = other.targetId;
    if (!other.name.isEmpty())
        data.name = other.name;
    if (other.frameLevel != -1)
        data.frameLevel = other.frameLevel;
    if (!other.function.isEmpty())
        data.function = other.function;
    if (!other.address)
        data.address = other.address;
    if (!other.module.isEmpty())
        data.module = other.module;
    if (!other.details.isEmpty())
        data.details = other.details;
    if (!other.state.isEmpty())
        data.state = other.state;
    if (other.lineNumber != -1)
        data.lineNumber = other.lineNumber;
}

////////////////////////////////////////////////////////////////////////
//
// ThreadsHandler
//
///////////////////////////////////////////////////////////////////////

static QString threadToolTip(const ThreadData &thread)
{
    const char start[] = "<tr><td>";
    const char sep[] = "</td><td>";
    const char end[] = "</td>";
    QString rc;
    QTextStream str(&rc);
    str << "<html><head/><body><table>"
        << start << ThreadsHandler::tr("Thread&nbsp;id:")
        << sep << thread.id.raw() << end;
    if (!thread.targetId.isEmpty())
        str << start << ThreadsHandler::tr("Target&nbsp;id:")
            << sep << thread.targetId << end;
    if (!thread.groupId.isEmpty())
        str << start << ThreadsHandler::tr("Group&nbsp;id:")
            << sep << thread.groupId << end;
    if (!thread.name.isEmpty())
        str << start << ThreadsHandler::tr("Name:")
            << sep << thread.name << end;
    if (!thread.state.isEmpty())
        str << start << ThreadsHandler::tr("State:")
            << sep << thread.state << end;
    if (!thread.core.isEmpty())
        str << start << ThreadsHandler::tr("Core:")
            << sep << thread.core << end;
    if (thread.address) {
        str << start << ThreadsHandler::tr("Stopped&nbsp;at:") << sep;
        if (!thread.function.isEmpty())
            str << thread.function << "<br>";
        if (!thread.fileName.isEmpty())
            str << thread.fileName << ':' << thread.lineNumber << "<br>";
        str << formatToolTipAddress(thread.address);
    }
    str << "</table></body></html>";
    return rc;
}

////////////////////////////////////////////////////////////////////////
//
// ThreadsHandler
//
///////////////////////////////////////////////////////////////////////

/*!
    \struct Debugger::Internal::ThreadData
    \brief A structure containing information about a single thread
*/

/*!
    \class Debugger::Internal::ThreadsHandler
    \brief A model to represent the running threads in a QTreeView or ComboBox
*/

ThreadsHandler::ThreadsHandler()
  : m_currentIndex(-1),
    m_positionIcon(QLatin1String(":/debugger/images/location_16.png")),
    m_emptyIcon(QLatin1String(":/debugger/images/debugger_empty_14.png"))
{
    m_resetLocationScheduled = false;
//    m_proxyModel = new QSortFilterProxyModel(this);
//    m_proxyModel->setSourceModel(this);
}

int ThreadsHandler::rowCount(const QModelIndex &parent) const
{
    // Since the stack is not a tree, row count is 0 for any valid parent.
    return parent.isValid() ? 0 : m_threads.size();
}

int ThreadsHandler::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(ThreadData::ColumnCount);
}

QVariant ThreadsHandler::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    const int row = index.row();
    if (row >= m_threads.size())
        return QVariant();
    const ThreadData &thread = m_threads.at(row);

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case ThreadData::IdColumn:
            return thread.id.raw();
        case ThreadData::FunctionColumn:
            return thread.function;
        case ThreadData::FileColumn:
            return thread.fileName.isEmpty() ? thread.module : thread.fileName;
        case ThreadData::LineColumn:
            return thread.lineNumber >= 0
                ? QString::number(thread.lineNumber) : QString();
        case ThreadData::AddressColumn:
            return thread.address > 0
                ? QLatin1String("0x") + QString::number(thread.address, 16)
                : QString();
        case ThreadData::CoreColumn:
            return thread.core;
        case ThreadData::StateColumn:
            return thread.state;
        case ThreadData::TargetIdColumn:
            if (thread.targetId.startsWith(QLatin1String("Thread ")))
                return thread.targetId.mid(7);
            return thread.targetId;
        case ThreadData::NameColumn:
            return thread.name;
        case ThreadData::DetailsColumn:
            return thread.details;
        case ThreadData::ComboNameColumn:
            return QString::fromLatin1("#%1 %2").arg(thread.id.raw()).arg(thread.name);
        }
    case Qt::ToolTipRole:
        return threadToolTip(thread);
    case Qt::DecorationRole:
        // Return icon that indicates whether this is the active stack frame.
        if (index.column() == 0)
            return (index.row() == m_currentIndex) ? m_positionIcon : m_emptyIcon;
        break;
    case ThreadData::IdRole:
        return thread.id.raw();
    default:
        break;
    }
    return QVariant();
}

QVariant ThreadsHandler::headerData
    (int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();
    switch (section) {
    case ThreadData::IdColumn:
        return QString(QLatin1String("  ") + tr("ID") + QLatin1String("  "));
    case ThreadData::FunctionColumn:
        return tr("Function");
    case ThreadData::FileColumn:
        return tr("File");
    case ThreadData::LineColumn:
        return tr("Line");
    case ThreadData::AddressColumn:
        return tr("Address");
    case ThreadData::CoreColumn:
        return tr("Core");
    case ThreadData::StateColumn:
        return tr("State");
    case ThreadData::TargetIdColumn:
        return tr("Target ID");
    case ThreadData::DetailsColumn:
        return tr("Details");
    case ThreadData::NameColumn:
        return tr("Name");
    }
    return QVariant();
}

Qt::ItemFlags ThreadsHandler::flags(const QModelIndex &index) const
{
    const int row = index.row();
    const bool stopped = row >= 0 && row < m_threads.size()
            && m_threads.at(row).stopped;
    return stopped ? QAbstractTableModel::flags(index) : Qt::ItemFlags(0);
}

ThreadId ThreadsHandler::currentThread() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_threads.size())
        return ThreadId();
    return m_threads[m_currentIndex].id;
}

ThreadId ThreadsHandler::threadAt(int index) const
{
    QTC_ASSERT(index >= 0 && index < m_threads.size(), return ThreadId());
    return m_threads[index].id;
}

void ThreadsHandler::setCurrentThread(ThreadId id)
{
    const int index = indexOf(id);
    if (index == m_currentIndex)
        return;

    if (index == -1) {
        qWarning("ThreadsHandler::setCurrentThreadId: No such thread %d.", int(id.raw()));
        return;
    }

    // Emit changed for previous frame.
    if (m_currentIndex != -1)
        dataChanged(m_currentIndex);

    m_currentIndex = index;

    // Emit changed for new frame.
    dataChanged(m_currentIndex);

    updateThreadBox();
}

int ThreadsHandler::indexOf(ThreadId threadId) const
{
    for (int i = m_threads.size(); --i >= 0; )
        if (m_threads.at(i).id == threadId)
            return i;
    return -1;
}

void ThreadsHandler::updateThread(const ThreadData &thread)
{
    const int i = indexOf(thread.id);
    if (i == -1) {
        beginInsertRows(QModelIndex(), m_threads.size(), m_threads.size());
        m_threads.append(thread);
        endInsertRows();
    } else {
        mergeThreadData(m_threads[i], thread);
        dataChanged(i);
    }
}

void ThreadsHandler::removeThread(ThreadId threadId)
{
    const int i = indexOf(threadId);
    if (i == -1)
        return;
    beginRemoveRows(QModelIndex(), i, i);
    m_threads.remove(i);
    endRemoveRows();
}

void ThreadsHandler::setThreads(const Threads &threads)
{
    beginResetModel();
    m_threads = threads;
    if (m_currentIndex >= m_threads.size())
        m_currentIndex = -1;
    m_resetLocationScheduled = false;
    endResetModel();
    updateThreadBox();
}

void ThreadsHandler::updateThreadBox()
{
    QStringList list;
    foreach (const ThreadData &thread, m_threads)
        list.append(QString::fromLatin1("#%1 %2").arg(thread.id.raw()).arg(thread.name));
    debuggerCore()->setThreads(list, m_currentIndex);
}

void ThreadsHandler::dataChanged(int index)
{
    Q_UNUSED(index);
    layoutChanged();
}

Threads ThreadsHandler::threads() const
{
    return m_threads;
}

ThreadData ThreadsHandler::thread(ThreadId id) const
{
    const int i = indexOf(id);
    return i == -1 ? ThreadData() : m_threads.at(i);
}

void ThreadsHandler::removeAll()
{
    beginResetModel();
    m_threads.clear();
    m_currentIndex = -1;
    endResetModel();
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
    for (int i = m_threads.size(); --i >= 0; )
        m_threads[i].notifyRunning();
    layoutChanged();
}

void ThreadsHandler::notifyRunning(ThreadId id)
{
    int i = indexOf(id);
    if (i >= 0) {
        m_threads[i].notifyRunning();
        dataChanged(i);
    }
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
    for (int i = m_threads.size(); --i >= 0; )
        m_threads[i].stopped = true;
    layoutChanged();
}

void ThreadsHandler::notifyStopped(ThreadId id)
{
    int i = indexOf(id);
    if (i >= 0) {
        m_threads[i].stopped = true;
        dataChanged(i);
    }
}

void ThreadsHandler::updateThreads(const GdbMi &data)
{
    // ^done,threads=[{id="1",target-id="Thread 0xb7fdc710 (LWP 4264)",
    // frame={level="0",addr="0x080530bf",func="testQString",args=[],
    // file="/.../app.cpp",fullname="/../app.cpp",line="1175"},
    // state="stopped",core="0"}],current-thread-id="1"

    // Emit changed for previous frame.
    if (m_currentIndex != -1) {
        dataChanged(m_currentIndex);
        m_currentIndex = -1;
    }

    ThreadId currentId;
    const GdbMi current = data.findChild("current-thread-id");
    if (current.isValid())
        currentId = ThreadId(current.data().toLongLong());

    const QList<GdbMi> items = data.findChild("threads").children();
    const int n = items.size();
    for (int index = 0; index != n; ++index) {
        bool ok = false;
        const GdbMi item = items.at(index);
        const GdbMi frame = item.findChild("frame");
        ThreadData thread;
        thread.id = ThreadId(item.findChild("id").data().toInt());
        thread.targetId = QString::fromLatin1(item.findChild("target-id").data());
        thread.details = QString::fromLatin1(item.findChild("details").data());
        thread.core = QString::fromLatin1(item.findChild("core").data());
        thread.state = QString::fromLatin1(item.findChild("state").data());
        thread.address = frame.findChild("addr").data().toULongLong(&ok, 0);
        thread.function = QString::fromLatin1(frame.findChild("func").data());
        thread.fileName = QString::fromLatin1(frame.findChild("fullname").data());
        thread.lineNumber = frame.findChild("line").data().toInt();
        thread.module = QString::fromLocal8Bit(frame.findChild("from").data());
        thread.stopped = true;
        // Non-GDB (Cdb2) output name here.
        thread.name = QString::fromLatin1(frame.findChild("name").data());
        if (thread.state == QLatin1String("running"))
            thread.stopped = false;
        if (thread.id == currentId)
            m_currentIndex = index;
        updateThread(thread);
    }

    if (m_currentIndex != -1)
        dataChanged(m_currentIndex);

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
    //return m_proxyModel;
}

} // namespace Internal
} // namespace Debugger
