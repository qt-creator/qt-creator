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

#ifndef PROMPTTHREAD_H
#define PROMPTTHREAD_H

#include <QtCore/QThread>

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
    Execution_Go          = ExecutionCommand|1,
    Execution_StartBinary = ExecutionCommand|2
};

class CdbPromptThread : public QThread
{
    Q_OBJECT
public:
    explicit CdbPromptThread(QObject *parent = 0);

    virtual void run();

signals:
    void asyncCommand(int command, const QString &arg);
    void syncCommand(int command, const QString &arg);
    void executionCommand(int command, const QString &arg);

private:
    bool handleCommand(QString);
};

#endif // PROMPTTHREAD_H
