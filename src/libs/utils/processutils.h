/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "processenums.h"
#include "processreaper.h"

#include <QIODevice>
#include <QProcess>

namespace Utils {

class ProcessStartHandler {
public:
    ProcessStartHandler(QProcess *process) : m_process(process) {}

    void setProcessMode(ProcessMode mode) { m_processMode = mode; }
    void setWriteData(const QByteArray &writeData) { m_writeData = writeData; }
    QIODevice::OpenMode openMode() const;
    void handleProcessStart();
    void handleProcessStarted();
    void setBelowNormalPriority();
    void setNativeArguments(const QString &arguments);

private:
    ProcessMode m_processMode = ProcessMode::Reader;
    QByteArray m_writeData;
    QProcess *m_process;
};

class ProcessHelper : public QProcess
{
    Q_OBJECT

public:
    ProcessHelper(QObject *parent) : QProcess(parent), m_processStartHandler(this)
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) && defined(Q_OS_UNIX)
        setChildProcessModifier([this] { setupChildProcess_impl(); });
#endif
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    void setupChildProcess() override { setupChildProcess_impl(); }
#endif

    ProcessStartHandler *processStartHandler() { return &m_processStartHandler; }

    using QProcess::setErrorString;

    void setLowPriority() { m_lowPriority = true; }
    void setUnixTerminalDisabled() { m_unixTerminalDisabled = true; }
    void setUseCtrlCStub(bool enabled); // release only

    static void terminateProcess(QProcess *process);
    static void interruptProcess(QProcess *process);
    static void interruptPid(qint64 pid);

private:
    void terminateProcess();
    void setupChildProcess_impl();

    bool m_lowPriority = false;
    bool m_unixTerminalDisabled = false;
    bool m_useCtrlCStub = false; // release only
    ProcessStartHandler m_processStartHandler;
};

} // namespace Utils
