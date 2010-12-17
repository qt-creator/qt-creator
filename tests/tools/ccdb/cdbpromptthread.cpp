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

#include "cdbpromptthread.h"

#include <QtCore/QDebug>

static const char help[] =
"Special commands:\n\n"
"H                        Display Help\n"
"V                        Output version\n"
"E expression             Evaluate C++expression\n"
"S binary args            Start binary\n"
"I                        Interrupt\n"
"G                        Go\n"
"Q cmd                    Queue command for execution in AttachProcess\n"
"Q                        Clear command queue\n"
"B file:line              Queue a breakpoint for adding in AttachProcess\n"
"B                        Clear breakpoint queue\n"
"L                        List breakpoints\n"
"F <n>                    Print stack frame <n>, 0 being top\n"
"P <cmd>                  Run Python command\n"
"W                        Synchronous wait for debug event\n"
"\nThe remaining commands are passed to CDB.\n";

CdbPromptThread::CdbPromptThread(FILE *file, QObject *parent) :
        QThread(parent),
        m_waitingForDebugEvent(false),
        m_inputFile(file)
{
    if (!m_inputFile)
        m_inputFile = stdin;
}

void CdbPromptThread::run()
{
    enum { bufSize =1024 };

    QString cmd;
    char buf[bufSize];
    std::putc('>', stdout);
    // When reading from an input file, switch to stdin after reading it out
    while (true) {
        if (std::fgets(buf, bufSize, m_inputFile) == NULL) {
            if (m_inputFile == stdin) {
                break;
            } else {
                fclose(m_inputFile);
                m_inputFile = stdin;
                continue;
            }
        }
        cmd += QString::fromLatin1(buf);
        if (cmd.endsWith(QLatin1Char('\n'))) {
            cmd.truncate(cmd.size() - 1);
            if (!cmd.isEmpty() && !handleCommand(cmd.trimmed()))
                break;
            cmd.clear();
        }
        std::putc('>', stdout);
    }
    if (m_inputFile != stdin)
        fclose(m_inputFile);
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
        case 'L':
            return Sync_ListBreakPoints;
        case 'E':
            return Sync_EvalExpression;
        case 'G':
            return Execution_Go;
        case 'S':
            return Execution_StartBinary;
        case 'F':
            return Sync_PrintFrame;
        case 'P':
            return Sync_Python;
        case 'V':
            return Sync_OutputVersion;
        case 'W':
            return WaitCommand;
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

void CdbPromptThread::notifyDebugEvent()
{
    if (m_waitingForDebugEvent)
        m_debugEventWaitCondition.wakeAll();
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
    if (c == WaitCommand) {
        std::fputs("Waiting for debug event\n", stdout);
        m_debugEventMutex.lock();
        m_waitingForDebugEvent = true;
        m_debugEventWaitCondition.wait(&m_debugEventMutex);
        m_debugEventMutex.unlock();
        std::fputs("Debug event received\n", stdout);
        m_waitingForDebugEvent = false;
        return true;
    }
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
