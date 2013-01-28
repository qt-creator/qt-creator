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
#include "gdb/gdbmi.h"
#include "watchutils.h"

#include "debuggerconstants.h"
#include "debuggercore.h"

#include <QDebug>
#include <QTextStream>
#include <QSortFilterProxyModel>

namespace Debugger {
namespace Internal {

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
        << sep << thread.id << end;
    if (!thread.targetId.isEmpty())
        str << start << ThreadsHandler::tr("Target&nbsp;id:")
            << sep << thread.targetId << end;
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
  : m_currentIndex(0),
    m_positionIcon(QLatin1String(":/debugger/images/location_16.png")),
    m_emptyIcon(QLatin1String(":/debugger/images/debugger_empty_14.png"))
{
    m_resetLocationScheduled = false;
    m_contentsValid = false;
    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(this);
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
            return thread.id;
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
        case ThreadData::ComboNameColumn:
            return QString::fromLatin1("#%1 %2").arg(thread.id).arg(thread.name);
        }
    case Qt::ToolTipRole:
        return threadToolTip(thread);
    case Qt::DecorationRole:
        // Return icon that indicates whether this is the active stack frame.
        if (index.column() == 0)
            return (index.row() == m_currentIndex) ? m_positionIcon : m_emptyIcon;
        break;
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
    case ThreadData::NameColumn:
        return tr("Name");
    }
    return QVariant();
}

Qt::ItemFlags ThreadsHandler::flags(const QModelIndex &index) const
{
    return m_contentsValid ? QAbstractTableModel::flags(index) : Qt::ItemFlags(0);
}

int ThreadsHandler::currentThreadId() const
{
    if (m_currentIndex < 0 || m_currentIndex >= m_threads.size())
        return -1;
    return m_threads[m_currentIndex].id;
}

void ThreadsHandler::setCurrentThread(int index)
{
    if (index == m_currentIndex)
        return;

    // Emit changed for previous frame.
    QModelIndex i = ThreadsHandler::index(m_currentIndex, 0);
    emit dataChanged(i, i);

    m_currentIndex = index;

    // Emit changed for new frame.
    i = ThreadsHandler::index(m_currentIndex, 0);
    emit dataChanged(i, i);

    updateThreadBox();
}

void ThreadsHandler::setCurrentThreadId(int id)
{
    const int index = indexOf(id);
    if (index != -1)
        setCurrentThread(index);
    else
        qWarning("ThreadsHandler::setCurrentThreadId: No such thread %d.", id);
}

int ThreadsHandler::indexOf(quint64 threadId) const
{
    const int count = m_threads.size();
    for (int i = 0; i < count; ++i)
        if (m_threads.at(i).id == threadId)
            return i;
    return -1;
}

void ThreadsHandler::setThreads(const Threads &threads)
{
    m_threads = threads;
    if (m_currentIndex >= m_threads.size())
        m_currentIndex = -1;
    m_resetLocationScheduled = false;
    m_contentsValid = true;
    reset();
    updateThreadBox();
}

void ThreadsHandler::updateThreadBox()
{
    QStringList list;
    foreach (const ThreadData &thread, m_threads)
        list.append(QString::fromLatin1("#%1 %2").arg(thread.id).arg(thread.name));
    debuggerCore()->setThreads(list, m_currentIndex);
}

Threads ThreadsHandler::threads() const
{
    return m_threads;
}

void ThreadsHandler::removeAll()
{
    m_threads.clear();
    m_currentIndex = 0;
    reset();
}

void ThreadsHandler::notifyRunning()
{
    // Threads stopped (that is, address != 0 showing)?
    if (m_threads.empty())
        return;
    if (m_threads.front().address == 0)
        return;
    const Threads::iterator end = m_threads.end();
    for (Threads::iterator it = m_threads.begin(); it != end; ++it)
        it->notifyRunning();
    emit dataChanged(index(0, 1),
        index(m_threads.size() - 1, ThreadData::ColumnCount - 1));
}

Threads ThreadsHandler::parseGdbmiThreads(const GdbMi &data, int *currentThread)
{
    // ^done,threads=[{id="1",target-id="Thread 0xb7fdc710 (LWP 4264)",
    // frame={level="0",addr="0x080530bf",func="testQString",args=[],
    // file="/.../app.cpp",fullname="/../app.cpp",line="1175"},
    // state="stopped",core="0"}],current-thread-id="1"
    const QList<GdbMi> items = data.findChild("threads").children();
    const int n = items.size();
    Threads threads;
    threads.reserve(n);
    for (int index = 0; index != n; ++index) {
        bool ok = false;
        const GdbMi item = items.at(index);
        const GdbMi frame = item.findChild("frame");
        ThreadData thread;
        thread.id = item.findChild("id").data().toInt();
        thread.targetId = QString::fromLatin1(item.findChild("target-id").data());
        thread.core = QString::fromLatin1(item.findChild("core").data());
        thread.state = QString::fromLatin1(item.findChild("state").data());
        thread.address = frame.findChild("addr").data().toULongLong(&ok, 0);
        thread.function = QString::fromLatin1(frame.findChild("func").data());
        thread.fileName = QString::fromLatin1(frame.findChild("fullname").data());
        thread.lineNumber = frame.findChild("line").data().toInt();
        thread.module = QString::fromLocal8Bit(frame.findChild("from").data());
        // Non-GDB (Cdb2) output name here.
        thread.name = QString::fromLatin1(frame.findChild("name").data());
        threads.append(thread);
    }
    if (currentThread)
        *currentThread = data.findChild("current-thread-id").data().toInt();
    return threads;
}

void ThreadsHandler::scheduleResetLocation()
{
    m_resetLocationScheduled = true;
    m_contentsValid = false;
}

void ThreadsHandler::resetLocation()
{
    if (m_resetLocationScheduled) {
        m_resetLocationScheduled = false;
        reset();
    }
}

QAbstractItemModel *ThreadsHandler::model()
{
    return m_proxyModel;
}

} // namespace Internal
} // namespace Debugger
