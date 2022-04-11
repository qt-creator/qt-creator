/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

    void start() final;
    qint64 write(const QByteArray &) final { QTC_CHECK(false); return -1; }
    void sendControlSignal(ControlSignal controlSignal) final;

    // intentionally no-op without an assert
    bool waitForStarted(int) final { return false; }
    bool waitForReadyRead(int) final { QTC_CHECK(false); return false; }
    // intentionally no-op without an assert
    bool waitForFinished(int) final { return false; }

    QProcess::ProcessState state() const final;

private:
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
