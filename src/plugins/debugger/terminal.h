/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QCoreApplication>
#include <QSocketNotifier>

#include <projectexplorer/runconfiguration.h>

#include <utils/consoleprocess.h>

namespace Debugger {

class DebuggerRunTool;

namespace Internal {

class Terminal : public QObject
{
    Q_OBJECT

public:
    Terminal(QObject *parent = nullptr);

    void setup();
    bool isUsable() const;

    QByteArray slaveDevice() const { return m_slaveName; }

    int write(const QByteArray &msg);
    bool sendInterrupt();

signals:
    void stdOutReady(const QString &);
    void stdErrReady(const QString &);
    void error(const QString &);

private:
    void onSlaveReaderActivated(int fd);

    bool m_isUsable = false;
    int m_masterFd = -1;
    QSocketNotifier *m_masterReader = nullptr;
    QByteArray m_slaveName;
};


class TerminalRunner : public ProjectExplorer::RunWorker
{
public:
    explicit TerminalRunner(DebuggerRunTool *runControl);

    qint64 applicationPid() const { return m_applicationPid; }
    qint64 applicationMainThreadId() const { return m_applicationMainThreadId; }

private:
    void start() final;
    void stop() final;

    void stubStarted();
    void stubError(const QString &msg);

    Utils::ConsoleProcess m_stubProc;
    ProjectExplorer::Runnable m_stubRunnable;
    qint64 m_applicationPid = 0;
    qint64 m_applicationMainThreadId = 0;
};

} // namespace Internal
} // namespace Debugger
