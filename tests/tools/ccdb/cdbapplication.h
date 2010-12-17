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

#ifndef CDBAPPLICATION_H
#define CDBAPPLICATION_H

#include "corebreakpoint.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QSharedPointer>
#include <QtCore/QScopedPointer>
#include <QtCore/QStringList>

#include <cstdio>

namespace CdbCore {
    class CoreEngine;
    class StackTraceContext;
}

class CdbPromptThread;

class CdbApplication : public QCoreApplication
{
    Q_OBJECT
    Q_DISABLE_COPY(CdbApplication)
public:
    enum InitResult { InitFailed, InitUsageShown, InitOk };

    CdbApplication(int argc, char *argv[]);
    ~CdbApplication();

    InitResult init();

private slots:
    void promptThreadTerminated();
    void asyncCommand(int command, const QString &arg);
    void syncCommand(int command, const QString &arg);
    void executionCommand(int command, const QString &arg);
    void debugEvent();
    void processAttached(void *handle);

private:
    bool parseOptions(FILE **inputFile);
    void printFrame(const QString &arg);
    quint64 addQueuedBreakPoint(const QString &arg, QString *errorMessage);

    QString m_engineDll;
    QSharedPointer<CdbCore::CoreEngine> m_engine;
    QSharedPointer<CdbCore::StackTraceContext> m_stackTrace;
    CdbPromptThread *m_promptThread;
    QStringList m_queuedCommands;
    QStringList m_queuedBreakPoints;

    void *m_processHandle;
};

#endif // CDBAPPLICATION_H
