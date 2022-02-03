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
#include "processutils.h"

#include <QProcess>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QDebug)
QT_FORWARD_DECLARE_CLASS(QTextCodec)

class tst_QtcProcess;

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
    enum ProcessImpl {
        QProcessImpl,
        ProcessLauncherImpl,
        DefaultImpl,
    };

    enum TerminalMode {
        TerminalOff,
        TerminalRun,
        TerminalDebug,
        TerminalSuspend,
        TerminalOn = TerminalRun // default mode for ON
    };

    struct Setup {
        Setup() {}
        Setup(ProcessImpl processImpl) : processImpl(processImpl) {}
        Setup(ProcessMode processMode) : processMode(processMode) {}
        Setup(TerminalMode terminalMode) : terminalMode(terminalMode) {}

        ProcessImpl processImpl = DefaultImpl;
        ProcessMode processMode = ProcessMode::Reader;
        TerminalMode terminalMode = TerminalOff;
    };

    QtcProcess(const Setup &setup = {}, QObject *parent = nullptr);
    QtcProcess(QObject *parent);
    ~QtcProcess();

    ProcessMode processMode() const;
    TerminalMode terminalMode() const;

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
    void unsetEnvironment();
    const Environment &environment() const;
    bool hasEnvironment() const;

    void setCommand(const CommandLine &cmdLine);
    const CommandLine &commandLine() const;

    FilePath workingDirectory() const;
    void setWorkingDirectory(const FilePath &dir);

    void setUseCtrlCStub(bool enabled);
    void setLowPriority();
    void setDisableUnixTerminal();
    void setRunAsRoot(bool on);
    bool isRunAsRoot() const;

    void setAbortOnMetaChars(bool abort);

    void start();
    virtual void terminate();
    virtual void interrupt();

    static bool startDetached(const CommandLine &cmd, const FilePath &workingDirectory = {},
                              qint64 *pid = nullptr);

    enum EventLoopMode {
        NoEventLoop,
        WithEventLoop // Avoid
    };

    // Starts the command and waits for finish.
    // User input processing is enabled when WithEventLoop was passed.
    void runBlocking(EventLoopMode eventLoopMode = NoEventLoop);

    /* Timeout for hanging processes (triggers after no more output
     * occurs on stderr/stdout). */
    void setTimeoutS(int timeoutS);

    void setCodec(QTextCodec *c);
    void setTimeOutMessageBoxEnabled(bool);
    void setExitCodeInterpreter(const std::function<QtcProcess::Result(int)> &interpreter);

    void setWriteData(const QByteArray &writeData);

    void setStdOutCallback(const std::function<void(const QString &)> &callback);
    void setStdOutLineCallback(const std::function<void(const QString &)> &callback);
    void setStdErrCallback(const std::function<void(const QString &)> &callback);
    void setStdErrLineCallback(const std::function<void(const QString &)> &callback);

    static void setRemoteProcessHooks(const DeviceProcessHooks &hooks);

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

    virtual int exitCode() const;

    QString exitMessage();

    // Helpers to find binaries. Do not use it for other path variables
    // and file types.
    static QString locateBinary(const QString &binary);
    static QString locateBinary(const QString &path, const QString &binary);

    static Environment systemEnvironmentForBinary(const FilePath &filePath);

    void kickoffProcess();
    void interruptProcess();
    qint64 applicationMainThreadID() const;

    // FIXME: Cut down the following bits inherited from QProcess and QIODevice.

    void setProcessChannelMode(QProcess::ProcessChannelMode mode);

    QProcess::ProcessError error() const;
    virtual QProcess::ProcessState state() const;
    bool isRunning() const; // Short for state() == QProcess::Running.

    virtual QString errorString() const;
    void setErrorString(const QString &str);

    qint64 processId() const;

    bool waitForStarted(int msecs = 30000);
    bool waitForReadyRead(int msecs = 30000);
    bool waitForFinished(int msecs = 30000);

    virtual QByteArray readAllStandardOutput();
    virtual QByteArray readAllStandardError();

    virtual QProcess::ExitStatus exitStatus() const;

    virtual void kill();

    virtual qint64 write(const QByteArray &input);
    void close();

    void setStandardInputFile(const QString &inputFile);

    QString toStandaloneCommandLine() const;

signals:
    void started();
    void finished();
    void errorOccurred(QProcess::ProcessError error);
    void readyReadStandardOutput();
    void readyReadStandardError();

private:
    friend QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug str, const QtcProcess &r);

    Internal::QtcProcessPrivate *d = nullptr;

    friend tst_QtcProcess;
    void beginFeed();
    void feedStdOut(const QByteArray &data);
    void endFeed();
};

using ExitCodeInterpreter = std::function<QtcProcess::Result(int /*exitCode*/)>;

} // namespace Utils
