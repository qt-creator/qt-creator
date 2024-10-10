// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "processenums.h"

#include <QProcess>

namespace Utils {

class ProcessStartHandler
{
public:
    ProcessStartHandler(QProcess *process) : m_process(process) {}

    void setProcessMode(ProcessMode mode) { m_processMode = mode; }
    void setWriteData(const QByteArray &writeData) { m_writeData = writeData; }
    QIODevice::OpenMode openMode() const;
    void handleProcessStart();
    void handleProcessStarted();
    void setNativeArguments(const QString &arguments);
    void setWindowsSpecificStartupFlags(
        bool belowNormalPriority, bool createConsoleWindow, bool forceDefaultErrorMode);

private:
    ProcessMode m_processMode = ProcessMode::Reader;
    QByteArray m_writeData;
    QProcess *m_process;
};

class ProcessHelper : public QProcess
{
    Q_OBJECT

public:
    explicit ProcessHelper(QObject *parent);

    ProcessStartHandler *processStartHandler() { return &m_processStartHandler; }

    using QProcess::setErrorString;

    void setLowPriority();
    void setUnixTerminalDisabled();
    void setUseCtrlCStub(bool enabled); // release only
    void setAllowCoreDumps(bool enabled);

    static void terminateProcess(QProcess *process);
    static void interruptPid(qint64 pid);

private:
    void enableChildProcessModifier();
    void terminateProcess();

    bool m_lowPriority = false;
    bool m_unixTerminalDisabled = false;
    bool m_useCtrlCStub = false;
    ProcessStartHandler m_processStartHandler;
};

} // namespace Utils
