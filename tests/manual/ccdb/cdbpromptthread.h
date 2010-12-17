/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef PROMPTTHREAD_H
#define PROMPTTHREAD_H

#include <QtCore/QThread>
#include <QtCore/QWaitCondition>
#include <QtCore/QMutex>

#include <cstdio>

enum CommandTypeFlags {
    // Interrupt or something.
    AsyncCommand      = 0x0010000,
    // Synchronous execution before next prompt,
    // eg eval expression. Connect with blocking slot.
    SyncCommand       = 0x0020000,
    // Starts debuggee. Requires starting the debug event
    // watch timer afterwards.
    ExecutionCommand  = 0x0040000
};

enum Command {
    UnknownCommand        = 0,
    Async_Interrupt       = AsyncCommand|1,
    Sync_EvalExpression   = SyncCommand|1,
    Sync_Queue            = SyncCommand|2,
    Sync_QueueBreakPoint  = SyncCommand|3,
    Sync_ListBreakPoints  = SyncCommand|4,
    Sync_PrintFrame       = SyncCommand|5,
    Sync_OutputVersion    = SyncCommand|6,
    Sync_Python           = SyncCommand|7,
    Execution_Go          = ExecutionCommand|1,
    Execution_StartBinary = ExecutionCommand|2,
    WaitCommand           = 0xFFFF
};

class CdbPromptThread : public QThread
{
    Q_OBJECT
public:
    explicit CdbPromptThread(FILE *file, QObject *parent = 0);

    virtual void run();

public slots:
    void notifyDebugEvent();

signals:
    void asyncCommand(int command, const QString &arg);
    void syncCommand(int command, const QString &arg);
    void executionCommand(int command, const QString &arg);

private:
    bool handleCommand(QString);
    QWaitCondition m_debugEventWaitCondition;
    QMutex m_debugEventMutex;
    bool m_waitingForDebugEvent;
    FILE *m_inputFile;
};

#endif // PROMPTTHREAD_H
