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

#ifndef DEBUGGER_STACKHANDLER_H
#define DEBUGGER_STACKHANDLER_H

#include "stackframe.h"

#include <QtCore/QAbstractItemModel>
#include <QtCore/QObject>

#include <QtGui/QIcon>


namespace Debugger {
namespace Internal {

struct StackCookie
{
    StackCookie() : isFull(true), gotoLocation(false) {}
    StackCookie(bool full, bool jump) : isFull(full), gotoLocation(jump) {}
    bool isFull;
    bool gotoLocation;
};

////////////////////////////////////////////////////////////////////////
//
// StackModel
//
////////////////////////////////////////////////////////////////////////

/*! A model to represent the stack in a QTreeView. */
class StackHandler : public QAbstractTableModel
{
    Q_OBJECT

public:
    StackHandler(QObject *parent = 0);

    void setFrames(const QList<StackFrame> &frames, bool canExpand = false);
    QList<StackFrame> frames() const;
    void setCurrentIndex(int index);
    int currentIndex() const { return m_currentIndex; }
    StackFrame currentFrame() const;
    int stackSize() const { return m_stackFrames.size(); }
    QString topAddress() const { return m_stackFrames.at(0).address; }

    // Called from StackHandler after a new stack list has been received
    void removeAll();
    QAbstractItemModel *stackModel() { return this; }
    bool isDebuggingDebuggingHelpers() const;

private:
    // QAbstractTableModel
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    Q_SLOT void resetModel() { reset(); }

    QList<StackFrame> m_stackFrames;
    int m_currentIndex;
    const QVariant m_positionIcon;
    const QVariant m_emptyIcon;
    bool m_canExpand;
};


////////////////////////////////////////////////////////////////////////
//
// ThreadsHandler
//
////////////////////////////////////////////////////////////////////////

struct ThreadData
{
    ThreadData(int threadId = 0);
    void notifyRunning(); // Clear state information

    int id;
    // State information when stopped
    quint64 address;
    QString function;
    QString file;
    int line;
};

/*! A model to represent the running threads in a QTreeView or ComboBox */
class ThreadsHandler : public QAbstractTableModel
{
    Q_OBJECT

public:
    ThreadsHandler(QObject *parent = 0);

    void setCurrentThread(int index);
    void selectThread(int index);
    void setThreads(const QList<ThreadData> &threads);
    void removeAll();
    QList<ThreadData> threads() const;
    QAbstractItemModel *threadsModel() { return this; }

    // Clear out all frame information
    void notifyRunning();

private:
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

private:
    QList<ThreadData> m_threads;
    int m_currentIndex;
    const QIcon m_positionIcon;
    const QIcon m_emptyIcon;
};


} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::StackCookie)


#endif // DEBUGGER_STACKHANDLER_H
