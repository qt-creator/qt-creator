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

#ifndef DEBUGGER_STACKHANDLER_H
#define DEBUGGER_STACKHANDLER_H

#include <QtCore/QAbstractTableModel>
#include <QtCore/QObject>

#include <QtGui/QIcon>

namespace Debugger {
namespace Internal {

////////////////////////////////////////////////////////////////////////
//
// StackModel
//
////////////////////////////////////////////////////////////////////////

struct StackFrame
{
    int level;
    QString function;
    QString file;  // we try to put an absolute file name in there
    QString from;
    QString to;
    int line;
    QString address;
};

/*! A model to represent the stack in a QTreeView. */
class StackHandler : public QAbstractTableModel
{
public:
    StackHandler(QObject *parent = 0);

    void setFrames(const QList<StackFrame> &frames);
    QList<StackFrame> frames() const;
    void setCurrentIndex(int index);
    int currentIndex() const { return m_currentIndex; }
    StackFrame currentFrame() const;
    int stackSize() const { return m_stackFrames.size(); }

    // Called from StackHandler after a new stack list has been received
    void removeAll();
    QAbstractItemModel *stackModel() { return this; }
    bool isDebuggingDumpers() const;

private:
    // QAbstractTableModel
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    QList<StackFrame> m_stackFrames;
    int m_currentIndex;
    QIcon m_positionIcon;
    QIcon m_emptyIcon;
};


////////////////////////////////////////////////////////////////////////
//
// ThreadsHandler
//
////////////////////////////////////////////////////////////////////////

struct ThreadData
{
    int id;
};

/*! A model to represent the running threads in a QTreeView or ComboBox */
class ThreadsHandler : public QAbstractTableModel
{
public:
    ThreadsHandler(QObject *parent = 0);

    void setCurrentThread(int index);
    void selectThread(int index);
    void setThreads(const QList<ThreadData> &threads);
    void removeAll();
    QList<ThreadData> threads() const;
    QAbstractItemModel *threadsModel() { return this; }

private:
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

private:
    friend class StackHandler;
    QList<ThreadData> m_threads;
    int m_currentIndex;
    QIcon m_positionIcon;
    QIcon m_emptyIcon;
};


} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_STACKHANDLER_H
