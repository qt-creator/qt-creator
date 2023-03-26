// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "processenums.h"
#include "processinterface.h"
#include "qtcassert.h"

#include <QProcess>

namespace Utils {

class CommandLine;
class Environment;
class FilePath;

namespace Internal {

class TerminalImpl final : public ProcessInterface
{
public:
    TerminalImpl();
    ~TerminalImpl() final;

private:
    void start() final;
    qint64 write(const QByteArray &) final { QTC_CHECK(false); return -1; }
    void sendControlSignal(ControlSignal controlSignal) final;

    // OK, however, impl looks a bit different (!= NotRunning vs == Running).
    // Most probably changing it into (== Running) should be OK.
    bool isRunning() const;

    void stopProcess();
    void stubConnectionAvailable();
    void readStubOutput();
    void stubExited();
    void cleanupAfterStartFailure(const QString &errorMessage);
    void killProcess();
    void killStub();
    void emitError(QProcess::ProcessError error, const QString &errorString);
    void emitFinished(int exitCode, QProcess::ExitStatus exitStatus);
    QString stubServerListen();
    void stubServerShutdown();
    void cleanupStub();
    void cleanupInferior();
    void sendCommand(char c);

    class TerminalProcessPrivate *d;
};

} // Internal
} // Utils
