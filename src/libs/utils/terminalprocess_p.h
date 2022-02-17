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

#include <QProcess>

namespace Utils {

class CommandLine;
class Environment;
class FilePath;

namespace Internal {

class TerminalProcess : public QObject
{
    Q_OBJECT
public:
    explicit TerminalProcess(QObject *parent, ProcessImpl processImpl, TerminalMode terminalMode);
    ~TerminalProcess() override;

    void setCommand(const CommandLine &command);
    const CommandLine &commandLine() const;

    void setWorkingDirectory(const FilePath &dir);
    FilePath workingDirectory() const;

    void setEnvironment(const Environment &env);
    const Environment &environment() const;

    QProcess::ProcessError error() const;
    QString errorString() const;

    void start();
    void stopProcess();

    // OK, however, impl looks a bit different (!= NotRunning vs == Running).
    // Most probably changing it into (== Running) should be OK.
    bool isRunning() const;

    QProcess::ProcessState state() const;
    qint64 processId() const;
    int exitCode() const;
    QProcess::ExitStatus exitStatus() const;

    void setAbortOnMetaChars(bool abort); // used only in sshDeviceProcess
    void kickoffProcess(); // only debugger terminal, only non-windows
    void interruptProcess(); // only debugger terminal, only non-windows
    qint64 applicationMainThreadID() const; // only debugger terminal, only windows (-1 otherwise)

signals:
    void started();
    void finished();
    void errorOccurred(QProcess::ProcessError error);

private:
    void stubConnectionAvailable();
    void readStubOutput();
    void stubExited();
    void cleanupAfterStartFailure(const QString &errorMessage);
    void finish(int exitCode, QProcess::ExitStatus exitStatus);
    void killProcess();
    void killStub();
    void emitError(QProcess::ProcessError err, const QString &errorString);
    QString stubServerListen();
    void stubServerShutdown();
    void cleanupStub();
    void cleanupInferior();

    class TerminalProcessPrivate *d;
};

} // Internal
} // Utils
