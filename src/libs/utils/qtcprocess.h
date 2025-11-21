// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "commandline.h"
#include "processenums.h"

#include <QtTaskTree/QTaskTree>

#include <QDeadlineTimer>
#include <QProcess>

QT_BEGIN_NAMESPACE
class QDebug;
QT_END_NAMESPACE

namespace Utils {

namespace Internal { class ProcessPrivate; }
namespace Pty { class Data; }

class Environment;
class DeviceProcessHooks;
class ProcessInterface;
class ProcessResultData;
class ProcessRunData;
class TextEncoding;

class QTCREATOR_UTILS_EXPORT Process final : public QObject
{
    Q_OBJECT

public:
    explicit Process(QObject *parent = nullptr);
    ~Process();

    // ProcessInterface related

    void start();

    void terminate();
    void kill();
    void interrupt();
    void kickoffProcess();
    void closeWriteChannel();
    void close();
    void stop();

    QString readAllStandardOutput();
    QString readAllStandardError();

    QByteArray readAllRawStandardOutput();
    QByteArray readAllRawStandardError();

    qint64 write(const QString &input);
    qint64 writeRaw(const QByteArray &input);

    qint64 processId() const;
    qint64 applicationMainThreadId() const;

    QProcess::ProcessState state() const;
    ProcessResultData resultData() const;

    int exitCode() const;
    QProcess::ExitStatus exitStatus() const;

    QProcess::ProcessError error() const;
    QString errorString() const;

    bool waitForStarted(QDeadlineTimer timeout = std::chrono::seconds(30));
    bool waitForReadyRead(QDeadlineTimer timeout = std::chrono::seconds(30));
    bool waitForFinished(QDeadlineTimer timeout = std::chrono::seconds(30));

    // ProcessSetupData related

    void setRunData(const ProcessRunData &data);
    ProcessRunData runData() const;

    void setCommand(const CommandLine &cmdLine);
    const CommandLine &commandLine() const;

    void setWorkingDirectory(const FilePath &dir);
    FilePath workingDirectory() const;

    void setEnvironment(const Environment &env);  // Main process
    const Environment &environment() const;

    void setControlEnvironment(const Environment &env); // Possible helper process (ssh on host etc)
    const Environment &controlEnvironment() const;

    void setPtyData(const std::optional<Pty::Data> &data);
    std::optional<Pty::Data> ptyData() const;

    void setTerminalMode(TerminalMode mode);
    TerminalMode terminalMode() const;
    bool usesTerminal() const { return terminalMode() != TerminalMode::Off; }

    void setProcessMode(ProcessMode processMode);
    ProcessMode processMode() const;

    void setWriteData(const QByteArray &writeData);

    void setUseCtrlCStub(bool enabled); // release only
    void setAllowCoreDumps(bool enabled);
    void setLowPriority();
    void setDisableUnixTerminal();
    void setRunAsUser(const QString &user);
    void setAbortOnMetaChars(bool abort);

    using ProcessInterfaceCreator = std::function<ProcessInterface *()>;
    void setProcessInterfaceCreator(const ProcessInterfaceCreator &creator);

    void setProcessChannelMode(QProcess::ProcessChannelMode mode);
    QProcess::ProcessChannelMode processChannelMode() const;

    void setStandardInputFile(const QString &inputFile);

    void setExtraData(const QString &key, const QVariant &value);
    QVariant extraData(const QString &key) const;

    void setExtraData(const QVariantHash &extraData);
    QVariantHash extraData() const;

    void setReaperTimeout(std::chrono::milliseconds timeout);
    std::chrono::milliseconds reaperTimeout() const;

    static void setRemoteProcessHooks(const DeviceProcessHooks &hooks);

    // TODO: Some usages of this method assume that Starting phase is also a running state
    // i.e. if isRunning() returns false, they assume NotRunning state, what may be an error.
    bool isRunning() const; // Short for state() == QProcess::Running.

    // Other enhancements.
    // These (or some of them) may be potentially moved outside of the class.
    // Some of them could be aggregated in another public utils class.

    static bool startDetached(const CommandLine &cmd, const FilePath &workingDirectory = {},
                              DetachedChannelMode channelMode = DetachedChannelMode::Forward,
                              qint64 *pid = nullptr);

    // Starts the command and waits for finish.
    void runBlocking(std::chrono::seconds timeout = std::chrono::seconds(10));

    void setEncoding(const TextEncoding &encoding); // for stdOut and stdErr
    void setUtf8Codec(); // for stdOut and stdErr
    void setUtf8StdOutCodec(); // for stdOut, stdErr uses executable.processStdErrCodec()

    void setStdOutCallback(const TextChannelCallback &callback);
    void setStdOutLineCallback(const TextChannelCallback &callback);
    void setStdErrCallback(const TextChannelCallback &callback);
    void setStdErrLineCallback(const TextChannelCallback &callback);

    void setTextChannelMode(Channel channel, TextChannelMode mode);
    TextChannelMode textChannelMode(Channel channel) const;

    bool readDataFromProcess(QByteArray *stdOut, QByteArray *stdErr, int timeoutS = 30);

    ProcessResult result() const;

    QByteArray allRawOutput() const;
    QString allOutput() const;

    QByteArray rawStdOut() const;
    QByteArray rawStdErr() const;

    QString stdOut() const; // possibly with CR
    QString stdErr() const; // possibly with CR

    QString cleanedStdOut() const; // with sequences of CR squashed and CR LF replaced by LF
    QString cleanedStdErr() const; // with sequences of CR squashed and CR LF replaced by LF

    const QStringList stdOutLines() const; // split, CR removed
    const QStringList stdErrLines() const; // split, CR removed

    enum class FailureMessageFormat { Plain, WithStdErr, WithStdOut, WithAllOutput };
    static QString exitMessage(const CommandLine &command, ProcessResult result, int exitCode,
                               std::chrono::milliseconds duration);
    QString exitMessage(FailureMessageFormat format = FailureMessageFormat::Plain) const;
    QString verboseExitMessage() const { return exitMessage(FailureMessageFormat::WithAllOutput); }
    std::chrono::milliseconds processDuration() const;

    QString toStandaloneCommandLine() const;

    void setCreateConsoleOnWindows(bool create);
    bool createConsoleOnWindows() const;

    void setForceDefaultErrorModeOnWindows(bool force);
    bool forceDefaultErrorModeOnWindows() const;

    // Use it with care!
    // That's useful only when process uses ExternalTerminalProcessImpl.
    // In this case, after the process is finished, you may take the process interface
    // to keep the external window open and delete the process afterwards, without
    // closing the external window.
    // You are responsible for deleting the interface at later point in time, otherwise you leak it.
    // Deleting the interface will close the external window immediately.
    // Call it only from slot connected to Process::done() signal.
    ProcessInterface *takeProcessInterface();

signals:
    void starting(); // On NotRunning -> Starting state transition
    void started();  // On Starting -> Running state transition
    void done();     // On Starting | Running -> NotRunning state transition
    void readyReadStandardOutput();
    void readyReadStandardError();
    void textOnStandardOutput(const QString &text);
    void textOnStandardError(const QString &text);
    void stoppingForcefully();

private:
    friend QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug str, const Process &r);

    friend class Internal::ProcessPrivate;
    Internal::ProcessPrivate *d = nullptr;
};

class DeviceProcessHooks
{
public:
    std::function<ProcessInterface *(const FilePath &)> processImplHook;
};

class ProcessTaskAdapter final
{
public:
    QTCREATOR_UTILS_EXPORT void operator()(Process *task, QTaskInterface *iface);
};

using ProcessTask = QCustomTask<Process, ProcessTaskAdapter>;

} // namespace Utils
