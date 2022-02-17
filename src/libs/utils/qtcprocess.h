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
#include "qtcassert.h"

#include <QProcess>

#include <functional>

QT_FORWARD_DECLARE_CLASS(QDebug)
QT_FORWARD_DECLARE_CLASS(QTextCodec)

class tst_QtcProcess;

namespace Utils {

class ProcessInterface;
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

    void setProcessInterface(ProcessInterface *interface);

    void setProcessImpl(ProcessImpl processImpl);

    void setTerminalMode(TerminalMode mode);
    bool usesTerminal() const { return terminalMode() != TerminalOff; }
    TerminalMode terminalMode() const;

    void setProcessMode(ProcessMode processMode);
    ProcessMode processMode() const;

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

    virtual void start();
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

    void setExtraData(const QString &key, const QVariant &value);
    QVariant extraData(const QString &key) const;

    void setExtraData(const QVariantHash &extraData);
    QVariantHash extraData() const;

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

class QTCREATOR_UTILS_EXPORT ProcessSetupData
{
public:
    QtcProcess::ProcessImpl m_processImpl = QtcProcess::DefaultImpl;
    ProcessMode m_processMode = ProcessMode::Reader;
    QtcProcess::TerminalMode m_terminalMode = QtcProcess::TerminalOff;

    CommandLine m_commandLine;
    FilePath m_workingDirectory;
    Environment m_environment;
    QByteArray m_writeData;
    bool m_runAsRoot = false;
    bool m_haveEnv = false;
    bool m_useCtrlCStub = false;
    QVariantHash m_extraData;

    QString m_nativeArguments;
    QString m_standardInputFile;
    QString m_errorString;
    bool m_belowNormalPriority = false;
    bool m_lowPriority = false;
    bool m_unixTerminalDisabled = false;
    bool m_abortOnMetaChars = true;
    QProcess::ProcessChannelMode m_processChannelMode = QProcess::SeparateChannels;
};

class QTCREATOR_UTILS_EXPORT ProcessInterface : public QObject
{
    Q_OBJECT

public:
    ProcessInterface(QObject *parent) : QObject(parent) {}

    virtual QByteArray readAllStandardOutput() = 0;
    virtual QByteArray readAllStandardError() = 0;

    virtual void start() { defaultStart(); }
    virtual void terminate() = 0;
    virtual void kill() = 0;
    virtual void close() = 0;
    virtual qint64 write(const QByteArray &data) = 0;

    virtual QProcess::ProcessError error() const = 0;
    virtual QProcess::ProcessState state() const = 0;
    virtual qint64 processId() const = 0;
    virtual int exitCode() const = 0;
    virtual QProcess::ExitStatus exitStatus() const = 0;
    virtual QString errorString() const = 0;
    virtual void setErrorString(const QString &str) = 0;

    virtual bool waitForStarted(int msecs) = 0;
    virtual bool waitForReadyRead(int msecs) = 0;
    virtual bool waitForFinished(int msecs) = 0;

    virtual void kickoffProcess() { QTC_CHECK(false); }
    virtual void interruptProcess() { QTC_CHECK(false); }
    virtual qint64 applicationMainThreadID() const { QTC_CHECK(false); return -1; }

signals:
    void started();
    void finished();
    void errorOccurred(QProcess::ProcessError error);
    void readyReadStandardOutput();
    void readyReadStandardError();

protected:
    void defaultStart();

    ProcessSetupData m_setup;

private:
    virtual void doDefaultStart(const QString &program, const QStringList &arguments)
    { Q_UNUSED(program) Q_UNUSED(arguments) QTC_CHECK(false); }
    bool dissolveCommand(QString *program, QStringList *arguments);
    bool ensureProgramExists(const QString &program);
    friend class QtcProcess;
};


using ExitCodeInterpreter = std::function<QtcProcess::Result(int /*exitCode*/)>;

} // namespace Utils
