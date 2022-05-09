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

#include "commandline.h"
#include "processenums.h"

#include <QProcess>

QT_BEGIN_NAMESPACE
class QDebug;
class QTextCodec;
QT_END_NAMESPACE

class tst_QtcProcess;

namespace Utils {

namespace Internal { class QtcProcessPrivate; }

class Environment;
class DeviceProcessHooks;
class ProcessInterface;
class ProcessResultData;

class QTCREATOR_UTILS_EXPORT QtcProcess final : public QObject
{
    Q_OBJECT

public:
    QtcProcess(QObject *parent = nullptr);
    ~QtcProcess();

    // ProcessInterface related

    void start();

    void terminate();
    void kill();
    void interrupt();
    void kickoffProcess();
    void close();

    QByteArray readAllStandardOutput();
    QByteArray readAllStandardError();

    qint64 write(const QString &input);
    qint64 writeRaw(const QByteArray &input);

    qint64 processId() const;
    qint64 applicationMainThreadId() const;

    QProcess::ProcessState state() const;
    virtual ProcessResultData resultData() const;

    int exitCode() const;
    QProcess::ExitStatus exitStatus() const;

    QProcess::ProcessError error() const;
    QString errorString() const;

    bool waitForStarted(int msecs = 30000);
    bool waitForReadyRead(int msecs = 30000);
    bool waitForFinished(int msecs = 30000);

    // ProcessSetupData related

    void setProcessImpl(ProcessImpl processImpl);

    void setTerminalMode(TerminalMode mode);
    TerminalMode terminalMode() const;
    bool usesTerminal() const { return terminalMode() != TerminalMode::Off; }

    void setProcessMode(ProcessMode processMode);
    ProcessMode processMode() const;

    void setEnvironment(const Environment &env);  // Main process
    const Environment &environment() const;

    void setControlEnvironment(const Environment &env); // Possible helper process (ssh on host etc)
    const Environment &controlEnvironment() const;

    void setCommand(const CommandLine &cmdLine);
    const CommandLine &commandLine() const;

    void setWorkingDirectory(const FilePath &dir);
    FilePath workingDirectory() const;

    void setWriteData(const QByteArray &writeData);

    void setUseCtrlCStub(bool enabled); // release only
    void setLowPriority();
    void setDisableUnixTerminal();
    void setRunAsRoot(bool on);
    bool isRunAsRoot() const;
    void setAbortOnMetaChars(bool abort);

    QProcess::ProcessChannelMode processChannelMode() const;
    void setProcessChannelMode(QProcess::ProcessChannelMode mode);
    void setStandardInputFile(const QString &inputFile);

    void setExtraData(const QString &key, const QVariant &value);
    QVariant extraData(const QString &key) const;

    void setExtraData(const QVariantHash &extraData);
    QVariantHash extraData() const;

    static void setRemoteProcessHooks(const DeviceProcessHooks &hooks);

    // TODO: Some usages of this method assume that Starting phase is also a running state
    // i.e. if isRunning() returns false, they assume NotRunning state, what may be an error.
    bool isRunning() const; // Short for state() == QProcess::Running.

    // Other enhancements.
    // These (or some of them) may be potentially moved outside of the class.
    // For some we may aggregate in another public utils class (or subclass of QtcProcess)?

    // TODO: How below 3 methods relate to QtcProcess? Action: move them somewhere else.
    // Helpers to find binaries. Do not use it for other path variables
    // and file types.
    static QString locateBinary(const QString &binary);
    static QString locateBinary(const QString &path, const QString &binary);
    static QString normalizeNewlines(const QString &text);

    // TODO: Unused currently? Should it serve as a compartment for contrary of remoteEnvironment?
    static Environment systemEnvironmentForBinary(const FilePath &filePath);

    static bool startDetached(const CommandLine &cmd, const FilePath &workingDirectory = {},
                              qint64 *pid = nullptr);

    // Starts the command and waits for finish.
    // User input processing is enabled when EventLoopMode::On was passed.
    void runBlocking(EventLoopMode eventLoopMode = EventLoopMode::Off);

    /* Timeout for hanging processes (triggers after no more output
     * occurs on stderr/stdout). */
    void setTimeoutS(int timeoutS);

    // TODO: We should specify the purpose of the codec, e.g. setCodecForStandardChannel()
    void setCodec(QTextCodec *c);
    void setTimeOutMessageBoxEnabled(bool);
    void setExitCodeInterpreter(const ExitCodeInterpreter &interpreter);

    void setStdOutCallback(const std::function<void(const QString &)> &callback);
    void setStdOutLineCallback(const std::function<void(const QString &)> &callback);
    void setStdErrCallback(const std::function<void(const QString &)> &callback);
    void setStdErrLineCallback(const std::function<void(const QString &)> &callback);

    void stopProcess();
    bool readDataFromProcess(int timeoutS, QByteArray *stdOut, QByteArray *stdErr,
                             bool showTimeOutMessageBox);

    ProcessResult result() const;
    void setResult(const ProcessResult &result);

    QByteArray allRawOutput() const;
    QString allOutput() const;

    QString stdOut() const;
    QString stdErr() const;

    QByteArray rawStdOut() const;

    QString exitMessage() const;

    QString toStandaloneCommandLine() const;

signals:
    void started();
    void finished();
    void done(); // The same as finished() with the addition it's being also emitted after
                 // FailedToStart error occurred.
    void errorOccurred(QProcess::ProcessError error);
    void readyReadStandardOutput();
    void readyReadStandardError();

private:
    friend QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug str, const QtcProcess &r);

    friend class Internal::QtcProcessPrivate;
    Internal::QtcProcessPrivate *d = nullptr;
};

class DeviceProcessHooks
{
public:
    std::function<ProcessInterface *(const FilePath &)> processImplHook;
    std::function<Environment(const FilePath &)> systemEnvironmentForBinary;
};

} // namespace Utils
