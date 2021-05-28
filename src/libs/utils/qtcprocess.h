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

#include "utils_global.h"

#include "environment.h"
#include "commandline.h"

#include <QProcess>
#include <QTextCodec>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QDebug)

namespace Utils {

class CommandLine;
class Environment;
class QtcProcess;

namespace Internal { class QtcProcessPrivate; }

class DeviceProcessHooks
{
public:
    std::function<void(QtcProcess &)> startProcessHook;
    std::function<Environment(const FilePath &)> systemEnvironmentForBinary;
};

class QTCREATOR_UTILS_EXPORT QtcProcess : public QObject
{
    Q_OBJECT

public:
    QtcProcess(QObject *parent = nullptr);
    ~QtcProcess();

    enum Result {
        // Finished successfully. Unless an ExitCodeInterpreter is set
        // this corresponds to a return code 0.
        FinishedWithSuccess,
        Finished = FinishedWithSuccess, // FIXME: Kept to ease downstream transition
        // Finished unsuccessfully. Unless an ExitCodeInterpreter is set
        // this corresponds to a return code different from 0.
        FinishedWithError,
        FinishedError = FinishedWithError, // FIXME: Kept to ease downstream transition
        // Process terminated abnormally (kill)
        TerminatedAbnormally,
        // Executable could not be started
        StartFailed,
        // Hang, no output after time out
        Hang
    };

    void setEnvironment(const Environment &env);
    const Environment &environment() const;

    void setCommand(const CommandLine &cmdLine);
    const CommandLine &commandLine() const;

    FilePath workingDirectory() const;
    void setWorkingDirectory(const FilePath &dir);
    void setWorkingDirectory(const QString &dir); // FIXME: Kept to ease downstream transition

    void setUseCtrlCStub(bool enabled);
    void setLowPriority();
    void setDisableUnixTerminal();

    void start();
    void start(const QString &cmd, const QStringList &args = {});
    void terminate();
    void interrupt();

    /* Timeout for hanging processes (triggers after no more output
     * occurs on stderr/stdout). */
    void setTimeoutS(int timeoutS);

    void setCodec(QTextCodec *c);
    void setTimeOutMessageBoxEnabled(bool);
    void setExitCodeInterpreter(const std::function<QtcProcess::Result(int)> &interpreter);

    // FIXME: This is currently only used in run(), not in start()
    void setWriteData(const QByteArray &writeData);

    void setStdOutCallback(const std::function<void(const QString &)> &callback);
    void setStdErrCallback(const std::function<void(const QString &)> &callback);

    static void setRemoteProcessHooks(const DeviceProcessHooks &hooks);

    void setOpenMode(QIODevice::OpenMode mode);

    bool stopProcess();
    bool readDataFromProcess(int timeoutS, QByteArray *stdOut, QByteArray *stdErr,
                             bool showTimeOutMessageBox);

    static QString normalizeNewlines(const QString &text);

    Result result() const;
    void setResult(Result result);

    QByteArray allRawOutput() const;
    QString allOutput() const;

    QString stdOut() const;
    QString stdErr() const;

    QByteArray rawStdOut() const;
    QByteArray rawStdErr() const;

    int exitCode() const;

    QString exitMessage();

    // Helpers to find binaries. Do not use it for other path variables
    // and file types.
    static QString locateBinary(const QString &binary);
    static QString locateBinary(const QString &path, const QString &binary);

    static Environment systemEnvironmentForBinary(const FilePath &filePath);

    // FIXME: Cut down the following bits inherited from QProcess and QIODevice.

    void setProcessChannelMode(QProcess::ProcessChannelMode mode);

    QProcess::ProcessError error() const;
    QProcess::ProcessState state() const;

    QString errorString() const;
    void setErrorString(const QString &str);

    qint64 processId() const;

    bool waitForStarted(int msecs = 30000);
    bool waitForReadyRead(int msecs = 30000);
    bool waitForFinished(int msecs = 30000);

    QByteArray readAllStandardOutput();
    QByteArray readAllStandardError();

    QProcess::ExitStatus exitStatus() const;

    void kill();

    qint64 write(const QByteArray &input);
    void closeWriteChannel();
    void close();

signals:
    void started();
    void finished(int exitCode, QProcess::ExitStatus exitStatus);
    void errorOccurred(QProcess::ProcessError error);
    void stateChanged(QProcess::ProcessState state);

    void readyReadStandardOutput();
    void readyReadStandardError();
    void readyRead();


private:
    friend class SynchronousProcess;
    friend QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug str, const QtcProcess &r);

    Internal::QtcProcessPrivate *d = nullptr;

    void setProcessEnvironment(const QProcessEnvironment &environment) = delete;
    QProcessEnvironment processEnvironment() const = delete;
};

using ExitCodeInterpreter = std::function<QtcProcess::Result(int /*exitCode*/)>;

QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug str, const QtcProcess &);

class QTCREATOR_UTILS_EXPORT SynchronousProcess : public QtcProcess
{
    Q_OBJECT
public:
    SynchronousProcess();
    ~SynchronousProcess() override;

    // Force the use of 'runBlocking' for now.
    void start() = delete;

    // This starts a nested event loop when running the command.
    void setProcessUserEventWhileRunning(); // Avoid.

    // Starts the command and waits for finish. User input processing depends
    // on whether setProcessUserEventWhileRunning was called.
    void runBlocking();
};

} // namespace Utils
