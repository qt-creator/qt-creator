// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "commandline.h"
#include "expected.h"
#include "processinterface.h"

namespace Utils {

class TerminalInterfacePrivate;

const char TERMINAL_SHELL_NAME[] = "Terminal.ShellName";

class StubCreator : public QObject
{
public:
    virtual expected_str<qint64> startStubProcess(const ProcessSetupData &setup) = 0;
};

class QTCREATOR_UTILS_EXPORT TerminalInterface : public ProcessInterface
{
    friend class TerminalInterfacePrivate;
    friend class StubCreator;

public:
    TerminalInterface(bool waitOnExit = true);
    ~TerminalInterface() override;

    int inferiorProcessId() const;
    int inferiorThreadId() const;

    void setStubCreator(StubCreator *creator);

    void emitError(QProcess::ProcessError error, const QString &errorString);
    void emitFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onStubExited();

protected:
    void onNewStubConnection();
    void onStubReadyRead();

    void sendCommand(char c);

    void killInferiorProcess();
    void killStubProcess();

    expected_str<void> startStubServer();
    void shutdownStubServer();
    void cleanupAfterStartFailure(const QString &errorMessage);

    bool isRunning() const;

private:
    void start() override;
    qint64 write(const QByteArray &data) override;
    void sendControlSignal(ControlSignal controlSignal) override;

    TerminalInterfacePrivate *d{nullptr};
};

} // namespace Utils
