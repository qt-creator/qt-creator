/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "stackhandler.h"

#include "debuggeractions.h"

#include <utils/qtcassert.h>

#include <QtCore/QAbstractTableModel>
#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>

namespace Debugger {
namespace Internal {

StackFrame::StackFrame()
  : level(0), line(0)
{}

void StackFrame::clear()
{
    line = level = 0;
    function.clear();
    file.clear();
    from.clear();
    to.clear();
    address.clear();
}

bool StackFrame::isUsable() const
{
    return !file.isEmpty() && QFileInfo(file).isReadable();
}

QString StackFrame::toString() const
{
    QString res;
    QTextStream str(&res);
    str << StackHandler::tr("Address:") << ' ' << address << ' '
        << StackHandler::tr("Function:") << ' ' << function << ' '
        << StackHandler::tr("File:") << ' ' << file << ' '
        << StackHandler::tr("Line:") << ' ' << line << ' '
        << StackHandler::tr("From:") << ' ' << from << ' '
        << StackHandler::tr("To:") << ' ' << to;
    return res;
}

QString StackFrame::toToolTip() const
{
    QString res;
    QTextStream str(&res);
    str << "<html><body><table>"
        << "<tr><td>" << StackHandler::tr("Address:") << "</td><td>" <<  address << "</td></tr>"
        << "<tr><td>" << StackHandler::tr("Function:") << "</td><td>" << function << "</td></tr>"
        << "<tr><td>" << StackHandler::tr("File:") << "</td><td>" << QDir::toNativeSeparators(file) << "</td></tr>"
        << "<tr><td>" << StackHandler::tr("Line:") << "</td><td>" << line << "</td></tr>"
        << "<tr><td>" << StackHandler::tr("From:") << "</td><td>" << from << "</td></tr>"
        << "<tr><td>" << StackHandler::tr("To:") << "</td><td>" << to << "</td></tr>"
        << "</table></body></html>";
    return res;
}

QDebug operator<<(QDebug d, const  StackFrame &f)
{
    QString res;
    QTextStream str(&res);
    str << "level=" << f.level << " address=" << f.address;
    if (!f.function.isEmpty())
        str << ' ' << f.function;
    if (!f.file.isEmpty())
        str << ' ' << f.file << ':' << f.line;
    if (!f.from.isEmpty())
        str << " from=" << f.from;
    if (!f.to.isEmpty())
        str << " to=" << f.to;
    d.nospace() << res;
    return d;
}

////////////////////////////////////////////////////////////////////////
//
// StackHandler
//
////////////////////////////////////////////////////////////////////////

StackHandler::StackHandler(QObject *parent)
  : QAbstractTableModel(parent),
    m_positionIcon(QIcon(":/debugger/images/location.svg")),
    m_emptyIcon(QIcon(":/debugger/images/empty.svg"))
{
    m_currentIndex = 0;
    m_canExpand = false;
    connect(theDebuggerAction(OperateByInstruction), SIGNAL(triggered()),
        this, SLOT(resetModel()));
}

int StackHandler::rowCount(const QModelIndex &parent) const
{
    // Since the stack is not a tree, row count is 0 for any valid parent
    return parent.isValid() ? 0 : (m_stackFrames.size() + m_canExpand);
}

int StackHandler::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 5;
}

QVariant StackHandler::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_stackFrames.size() + m_canExpand)
        return QVariant();

    if (index.row() == m_stackFrames.size()) {
        if (role == Qt::DisplayRole && index.column() == 0)
            return tr("...");
        if (role == Qt::DisplayRole && index.column() == 1)
            return tr("<More>");
        if (role == Qt::DecorationRole && index.column() == 0)
            return m_emptyIcon;
        return QVariant();
    }

    const StackFrame &frame = m_stackFrames.at(index.row());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0: // Stack frame level
            return QString::number(frame.level);
        case 1: // Function name
            return frame.function;
        case 2: // File name
            return frame.file.isEmpty() ? frame.from : QFileInfo(frame.file).fileName();
        case 3: // Line number
            return frame.line;
        case 4: // Address
            return frame.address;
        }
        return QVariant();
    }

    if (role == Qt::ToolTipRole) {
        //: Tooltip for variable
        return frame.toToolTip();
    }

    if (role == Qt::DecorationRole && index.column() == 0) {
        // Return icon that indicates whether this is the active stack frame
        return (index.row() == m_currentIndex) ? m_positionIcon : m_emptyIcon;
    }

    if (role == Qt::UserRole)
        return QVariant::fromValue(frame);

    return QVariant();
}

QVariant StackHandler::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0: return tr("Level");
            case 1: return tr("Function");
            case 2: return tr("File");
            case 3: return tr("Line");
            case 4: return tr("Address");
        };
    }
    return QVariant();
}

Qt::ItemFlags StackHandler::flags(const QModelIndex &index) const
{
    if (index.row() >= m_stackFrames.size() + m_canExpand)
        return 0;
    if (index.row() == m_stackFrames.size())
        return QAbstractTableModel::flags(index);
    const StackFrame &frame = m_stackFrames.at(index.row());
    const bool isValid = (frame.isUsable() && !frame.function.isEmpty())
        || theDebuggerBoolSetting(OperateByInstruction);
    return isValid ? QAbstractTableModel::flags(index) : Qt::ItemFlags(0);
}

StackFrame StackHandler::currentFrame() const
{
    QTC_ASSERT(m_currentIndex >= 0, return StackFrame());
    QTC_ASSERT(m_currentIndex < m_stackFrames.size(), return StackFrame());
    return m_stackFrames.at(m_currentIndex);
}

void StackHandler::setCurrentIndex(int level)
{
    if (level == m_currentIndex)
        return;

    // Emit changed for previous frame
    QModelIndex i = index(m_currentIndex, 0);
    emit dataChanged(i, i);

    m_currentIndex = level;

    // Emit changed for new frame
    i = index(m_currentIndex, 0);
    emit dataChanged(i, i);
}

void StackHandler::removeAll()
{
    m_stackFrames.clear();
    m_currentIndex = 0;
    reset();
}

void StackHandler::setFrames(const QList<StackFrame> &frames, bool canExpand)
{
    m_canExpand = canExpand;
    m_stackFrames = frames;
    if (m_currentIndex >= m_stackFrames.size())
        m_currentIndex = m_stackFrames.size() - 1;
    reset();
}

QList<StackFrame> StackHandler::frames() const
{
    return m_stackFrames;
}

bool StackHandler::isDebuggingDebuggingHelpers() const
{
    for (int i = m_stackFrames.size(); --i >= 0; )
        if (m_stackFrames.at(i).function.startsWith("qDumpObjectData"))
            return true;
    return false;
}


////////////////////////////////////////////////////////////////////////
//
// ThreadsHandler
//
////////////////////////////////////////////////////////////////////////

ThreadData::ThreadData(int threadId) :
    id(threadId),
    address(0),
    line(-1)
{
}

void ThreadData::notifyRunning()
{
    address = 0;
    function.clear();
    file.clear();
    line = -1;
}

enum { IdColumn, AddressColumn, FunctionColumn, FileColumn, LineColumn, ColumnCount };

ThreadsHandler::ThreadsHandler(QObject *parent)  :
    QAbstractTableModel(parent),
    m_currentIndex(0),
    m_positionIcon(QLatin1String(":/debugger/images/location.svg")),
    m_emptyIcon(QLatin1String(":/debugger/images/empty.svg"))
{
}

int ThreadsHandler::rowCount(const QModelIndex &parent) const
{
    // Since the stack is not a tree, row count is 0 for any valid parent
    return parent.isValid() ? 0 : m_threads.size();
}

int ThreadsHandler::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(ColumnCount);
}

QVariant ThreadsHandler::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    const int row = index.row();
    if (row  >= m_threads.size())
        return QVariant();
    const ThreadData &thread = m_threads.at(row);

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case IdColumn:
            return thread.id;
        case FunctionColumn:
            return thread.function;
        case FileColumn:
            return thread.file;
        case LineColumn:
            return thread.line >= 0 ? QString::number(thread.line) : QString();
        case AddressColumn:
            return thread.address > 0 ? QLatin1String("0x") + QString::number(thread.address, 16) : QString();
        }
    } else if (role == Qt::ToolTipRole) {
        if (thread.address == 0)
            return tr("Thread: %1").arg(thread.id);
        // Stopped
        if (thread.file.isEmpty())
            return tr("Thread: %1 at %2 (0x%3)").arg(thread.id).arg(thread.function).arg(thread.address, 0, 16);
        return tr("Thread: %1 at %2, %3:%4 (0x%5)").
                arg(thread.id).arg(thread.function, thread.file).arg(thread.line).arg(thread.address, 0, 16);
    } else if (role == Qt::DecorationRole && index.column() == 0) {
        // Return icon that indicates whether this is the active stack frame
        return (index.row() == m_currentIndex) ? m_positionIcon : m_emptyIcon;
    }

    return QVariant();
}

QVariant ThreadsHandler::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();
    switch (section) {
    case IdColumn:
        return tr("Thread ID");
    case FunctionColumn:
        return tr("Function");
    case FileColumn:
        return tr("File");
    case LineColumn:
        return tr("Line");
    case AddressColumn:
        return tr("Address");
    }
    return QVariant();
}

void ThreadsHandler::setCurrentThread(int index)
{
    if (index == m_currentIndex)
        return;

    // Emit changed for previous frame
    QModelIndex i = ThreadsHandler::index(m_currentIndex, 0);
    emit dataChanged(i, i);

    m_currentIndex = index;

    // Emit changed for new frame
    i = ThreadsHandler::index(m_currentIndex, 0);
    emit dataChanged(i, i);
}

void ThreadsHandler::setThreads(const QList<ThreadData> &threads)
{
    m_threads = threads;
    if (m_currentIndex >= m_threads.size())
        m_currentIndex = m_threads.size() - 1;
    reset();
}

QList<ThreadData> ThreadsHandler::threads() const
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
    const QList<ThreadData>::iterator end = m_threads.end();
    for (QList<ThreadData>::iterator it = m_threads.begin(); it != end; ++it)
        it->notifyRunning();
    emit dataChanged(index(0, 1), index(m_threads.size()- 1, ColumnCount - 1));
}
} // namespace Internal
} // namespace Debugger
