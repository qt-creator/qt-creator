/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "threadshandler.h"

namespace Debugger {
namespace Internal {

////////////////////////////////////////////////////////////////////////
//
// ThreadsHandler
//
///////////////////////////////////////////////////////////////////////

ThreadData::ThreadData(int threadId)
{
    notifyRunning();
    id = threadId;
}

void ThreadData::notifyRunning()
{
    address = 0;
    function.clear();
    fileName.clear();
    frameLevel = -1;
    state.clear();
    lineNumber = -1;
}

////////////////////////////////////////////////////////////////////////
//
// ThreadsHandler
//
///////////////////////////////////////////////////////////////////////

ThreadsHandler::ThreadsHandler(QObject *parent)  :
    QAbstractTableModel(parent),
    m_currentIndex(0),
    m_positionIcon(QLatin1String(":/debugger/images/location_16.png")),
    m_emptyIcon(QLatin1String(":/debugger/images/debugger_empty_14.png"))
{
}

int ThreadsHandler::rowCount(const QModelIndex &parent) const
{
    // Since the stack is not a tree, row count is 0 for any valid parent
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
    if (row  >= m_threads.size())
        return QVariant();
    const ThreadData &thread = m_threads.at(row);

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case ThreadData::IdColumn:
            return thread.id;
        case ThreadData::FunctionColumn:
            return thread.function;
        case ThreadData::FileColumn:
            return thread.fileName;
        case ThreadData::LineColumn:
            return thread.lineNumber >= 0 ? QString::number(thread.lineNumber) : QString();
        case ThreadData::AddressColumn:
            return thread.address > 0 ? QLatin1String("0x") + QString::number(thread.address, 16) : QString();
        case ThreadData::CoreColumn:
            return thread.core;
        case ThreadData::StateColumn:
            return thread.state;
        }
    } else if (role == Qt::ToolTipRole) {
        if (thread.address == 0)
            return tr("Thread: %1").arg(thread.id);
        // Stopped
        if (thread.fileName.isEmpty())
            return tr("Thread: %1 at %2 (0x%3)").arg(thread.id).arg(thread.function).arg(thread.address, 0, 16);
        return tr("Thread: %1 at %2, %3:%4 (0x%5)").
                arg(thread.id).arg(thread.function, thread.fileName).arg(thread.lineNumber).arg(thread.address, 0, 16);
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
    case ThreadData::IdColumn:
        return tr("Thread ID");
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
    }
    return QVariant();
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
    emit dataChanged(index(0, 1),
        index(m_threads.size() - 1, ThreadData::ColumnCount - 1));
}

} // namespace Internal
} // namespace Debugger
