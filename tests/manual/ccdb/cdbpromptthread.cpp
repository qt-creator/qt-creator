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

#include "cdbpromptthread.h"

#include <cstdio>
#include <QtCore/QDebug>

static const char help[] =
"Special commands:\n\n"
"H                        Display Help\n"
"q                        Quit\n"
"E expression             Evaluate C++expression\n"
"S binary args            Start binary\n"
"I                        Interrupt\n"
"G                        Go\n"
"Q cmd                    Queue command for execution in AttachProcess\n"
"Q                        Clear command queue\n"
"B file:line              Queue a breakpoint for adding in AttachProcess\n"
"B                        Clear breakpoint queue\n"
"F <n>                    Print stack frame <n>, 0 being top\n"
"\nThe remaining commands are passed to CDB.\n";

CdbPromptThread::CdbPromptThread(QObject *parent) :
        QThread(parent)
{
}

void CdbPromptThread::run()
{
    enum { bufSize =1024 };

    QString cmd;
    char buf[bufSize];
    std::putc('>', stdout);
    while (true) {
        if (std::fgets(buf, bufSize, stdin) == NULL)
            break;
        cmd += QString::fromLatin1(buf);
        if (cmd.endsWith(QLatin1Char('\n'))) {
            cmd.truncate(cmd.size() - 1);
            if (!cmd.isEmpty() && !handleCommand(cmd.trimmed()))
                break;
            cmd.clear();
        }
        std::putc('>', stdout);
    }
}

// Determine the command
static Command evaluateCommand(const QString &cmdToken)
{
    if (cmdToken.size() == 1) {
        switch(cmdToken.at(0).toAscii()) {
        case 'I':
            return Async_Interrupt;
        case 'Q':
            return Sync_Queue;
        case 'B':
            return Sync_QueueBreakPoint;
        case 'E':
            return Sync_EvalExpression;
        case 'G':
            return Execution_Go;
        case 'S':
            return Execution_StartBinary;
        case 'F':
            return Sync_PrintFrame;
        default:
            break;
        }
        return UnknownCommand;
    }
    return UnknownCommand;
}

// Chop off command and return argument list
static Command parseCommand(QString *s)
{
    if (s->isEmpty())
        return UnknownCommand;
    int firstBlank = s->indexOf(QLatin1Char(' '));
    // No further arguments
    if (firstBlank == -1) {
        const Command rc1 = evaluateCommand(*s);
        if (rc1 != UnknownCommand) // pass through debugger cmds
            s->clear();
        return rc1;
    }
    // Chop
    const Command rc = evaluateCommand(s->left(firstBlank));
    if (rc != UnknownCommand) { // pass through debugger cmds)
        int nextToken = firstBlank + 1;
        for ( ;  nextToken < s->size() && s->at(nextToken).isSpace(); nextToken++) ;
        s->remove(0, nextToken);
    }
    return rc;
}

bool CdbPromptThread::handleCommand(QString cmd)
{
    if (cmd == QLatin1String("q"))
        return false;
    if (cmd == QLatin1String("H")) {
        std::fputs(help, stdout);
        return true;
    }
    const Command c = parseCommand(&cmd);
    if (c & AsyncCommand) {
        emit asyncCommand(c, cmd);
        return true;
    }
    if (c & ExecutionCommand) {
        emit executionCommand(c, cmd);
        return true;
    }
    // Let Unknown default to sync exeute
    emit syncCommand(c, cmd);
    return true;
}
