/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "stackhandler.h"

#include <utils/qtcassert.h>

#include <QtCore/QAbstractTableModel>
#include <QtCore/QDebug>
#include <QtCore/QFileInfo>

using namespace Debugger::Internal;


////////////////////////////////////////////////////////////////////////
//
// StackHandler
//
////////////////////////////////////////////////////////////////////////

StackHandler::StackHandler(QObject *parent)
  : QAbstractTableModel(parent), m_currentIndex(0)
{
    m_emptyIcon = QIcon(":/gdbdebugger/images/empty.svg");
    m_positionIcon = QIcon(":/gdbdebugger/images/location.svg");
}

int StackHandler::rowCount(const QModelIndex &parent) const
{
    // Since the stack is not a tree, row count is 0 for any valid parent
    return parent.isValid() ? 0 : m_stackFrames.size();
}

int StackHandler::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 4;
}

QVariant StackHandler::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_stackFrames.size())
        return QVariant();

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
    } else if (role == Qt::ToolTipRole) {
        return  "<table><tr><td>Address:</td><td>" + frame.address + "</td></tr>"
            + "<tr><td>Function: </td><td>" + frame.function + "</td></tr>"
            + "<tr><td>File: </td><td>" + frame.file + "</td></tr>"
            + "<tr><td>Line: </td><td>" + QString::number(frame.line) + "</td></tr>"
            + "<tr><td>From: </td><td>" + frame.from + "</td></tr></table>"
            + "<tr><td>To: </td><td>" + frame.to + "</td></tr></table>";
    } else if (role == Qt::DecorationRole && index.column() == 0) {
        // Return icon that indicates whether this is the active stack frame
        return (index.row() == m_currentIndex) ? m_positionIcon : m_emptyIcon;
    }

    return QVariant();
}

QVariant StackHandler::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        static const char * const headers[] = {
            QT_TR_NOOP("Level"),
            QT_TR_NOOP("Function"),
            QT_TR_NOOP("File"),
            QT_TR_NOOP("Line"),
            QT_TR_NOOP("Address")
        };
        if (section < 5)
            return tr(headers[section]);
    }
    return QVariant();
}

Qt::ItemFlags StackHandler::flags(const QModelIndex &index) const
{
    if (index.row() >= m_stackFrames.size())
        return 0;
    const StackFrame &frame = m_stackFrames.at(index.row());
    const bool isValid = !frame.file.isEmpty() && !frame.function.isEmpty();
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

void StackHandler::setFrames(const QList<StackFrame> &frames)
{
    m_stackFrames = frames;
    if (m_currentIndex >= m_stackFrames.size())
        m_currentIndex = m_stackFrames.size() - 1;
    reset();
}

QList<StackFrame> StackHandler::frames() const
{
    return m_stackFrames;
}

bool StackHandler::isDebuggingDumpers() const
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

ThreadsHandler::ThreadsHandler(QObject *parent)
  : QAbstractTableModel(parent), m_currentIndex(0)
{
    m_emptyIcon = QIcon(":/gdbdebugger/images/empty.svg");
    m_positionIcon = QIcon(":/gdbdebugger/images/location.svg");
}

int ThreadsHandler::rowCount(const QModelIndex &parent) const
{
    // Since the stack is not a tree, row count is 0 for any valid parent
    return parent.isValid() ? 0 : m_threads.size();
}

int ThreadsHandler::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 1;
}

QVariant ThreadsHandler::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_threads.size())
        return QVariant();

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case 0: // Thread ID
            return m_threads.at(index.row()).id;
        case 1: // Function name
            return "???";
        }
    } else if (role == Qt::ToolTipRole) {
        return "Thread: " + QString::number(m_threads.at(index.row()).id);
    } else if (role == Qt::DecorationRole && index.column() == 0) {
        // Return icon that indicates whether this is the active stack frame
        return (index.row() == m_currentIndex) ? m_positionIcon : m_emptyIcon;
    }

    return QVariant();
}

QVariant ThreadsHandler::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        static const char * const headers[] = {
            QT_TR_NOOP("Thread ID"),
        };
        if (section < 1)
            return tr(headers[section]);
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
