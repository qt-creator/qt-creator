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

#include "applicationlauncher.h"
#ifdef Q_OS_WIN
#include "windebuginterface.h"
#include <qt_windows.h>
#endif

#include <coreplugin/icore.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include "devicesupport/deviceprocess.h"
#include "projectexplorer.h"
#include "projectexplorersettings.h"
#include "runcontrol.h"

#include <QTextCodec>
#include <QTimer>

/*!
    \class ProjectExplorer::ApplicationLauncher

    \brief The ApplicationLauncher class is the application launcher of the
    ProjectExplorer plugin.

    Encapsulates processes running in a console or as GUI processes,
    captures debug output of GUI processes on Windows (outputDebugString()).

    \sa Utils::QtcProcess
*/

using namespace Utils;

namespace ProjectExplorer {

using namespace Internal;

namespace Internal {

class ApplicationLauncherPrivate : public QObject
{
public:
    enum State { Inactive, Run };
    explicit ApplicationLauncherPrivate(ApplicationLauncher *parent);
    ~ApplicationLauncherPrivate() override { setFinished(); }

    void start(const Runnable &runnable, const IDevice::ConstPtr &device, bool local);
    void stop();

    // Local
    void handleProcessStarted();
    void localProcessError(QProcess::ProcessError error);
    void readLocalStandardOutput();
    void readLocalStandardError();
    void cannotRetrieveLocalDebugOutput();
    void checkLocalDebugOutput(qint64 pid, const QString &message);
    void localProcessDone(int, QProcess::ExitStatus);
    qint64 applicationPID() const;
    bool isLocalRunning() const;

    // Remote
    void doReportError(const QString &message,
                       QProcess::ProcessError error = QProcess::FailedToStart);
    void handleRemoteStderr();
    void handleRemoteStdout();
    void handleApplicationFinished();
    void setFinished();
    void handleApplicationError(QProcess::ProcessError error);

public:
    ApplicationLauncher *q;

    bool m_isLocal = true;
    bool m_runAsRoot = false;

    // Local
    std::unique_ptr<QtcProcess> m_localProcess;
    bool m_useTerminal = false;
    QProcess::ProcessChannelMode m_processChannelMode;
    // Keep track whether we need to emit a finished signal
    bool m_processRunning = false;

    QTextCodec *m_outputCodec;
    QTextCodec::ConverterState m_outputCodecState;
    QTextCodec::ConverterState m_errorCodecState;

    qint64 m_listeningPid = 0;

    // Remote
    DeviceProcess *m_deviceProcess = nullptr;
    QString m_remoteErrorString;
    QProcess::ProcessError m_remoteError = QProcess::UnknownError;
    QProcess::ExitStatus m_remoteExitStatus = QProcess::CrashExit;
    State m_state = Inactive;
    bool m_stopRequested = false;
};

} // Internal

static QProcess::ProcessChannelMode defaultProcessChannelMode()
{
    return ProjectExplorerPlugin::appOutputSettings().mergeChannels
            ? QProcess::MergedChannels : QProcess::SeparateChannels;
}

ApplicationLauncherPrivate::ApplicationLauncherPrivate(ApplicationLauncher *parent)
    : q(parent)
    , m_processChannelMode(defaultProcessChannelMode())
    , m_outputCodec(QTextCodec::codecForLocale())
{
#ifdef Q_OS_WIN
    connect(WinDebugInterface::instance(), &WinDebugInterface::cannotRetrieveDebugOutput,
            this, &ApplicationLauncherPrivate::cannotRetrieveLocalDebugOutput);
    connect(WinDebugInterface::instance(), &WinDebugInterface::debugOutput,
            this, &ApplicationLauncherPrivate::checkLocalDebugOutput);
#endif
}

ApplicationLauncher::ApplicationLauncher(QObject *parent) : QObject(parent),
    d(std::make_unique<ApplicationLauncherPrivate>(this))
{
}

ApplicationLauncher::~ApplicationLauncher() = default;

void ApplicationLauncher::setProcessChannelMode(QProcess::ProcessChannelMode mode)
{
    d->m_processChannelMode = mode;
}

void ApplicationLauncher::setUseTerminal(bool on)
{
    d->m_useTerminal = on;
}

void ApplicationLauncher::setRunAsRoot(bool on)
{
    d->m_runAsRoot = on;
}

void ApplicationLauncher::stop()
{
    d->stop();
}

void ApplicationLauncherPrivate::stop()
{
    if (m_isLocal) {
        if (!isLocalRunning())
            return;
        QTC_ASSERT(m_localProcess, return);
        m_localProcess->stopProcess();
        localProcessDone(0, QProcess::CrashExit);
    } else {
        if (m_stopRequested)
            return;
        m_stopRequested = true;
        m_remoteExitStatus = QProcess::CrashExit;
        emit q->appendMessage(ApplicationLauncher::tr("User requested stop. Shutting down..."),
                              Utils::NormalMessageFormat);
        switch (m_state) {
            case Run:
                m_deviceProcess->terminate();
                break;
            case Inactive:
                break;
        }
    }
}

bool ApplicationLauncher::isRunning() const
{
    return d->isLocalRunning();
}

bool ApplicationLauncher::isLocal() const
{
    return d->m_isLocal;
}

bool ApplicationLauncherPrivate::isLocalRunning() const
{
    if (!m_localProcess)
        return false;
    return m_localProcess->state() != QProcess::NotRunning;
}

ProcessHandle ApplicationLauncher::applicationPID() const
{
    return ProcessHandle(d->applicationPID());
}

qint64 ApplicationLauncherPrivate::applicationPID() const
{
    if (!isLocalRunning())
        return 0;

    return m_localProcess->processId();
}

QString ApplicationLauncher::errorString() const
{
    if (d->m_isLocal)
        return d->m_localProcess ? d->m_localProcess->errorString() : QString();
    return d->m_remoteErrorString;
}

QProcess::ProcessError ApplicationLauncher::processError() const
{
    if (d->m_isLocal)
        return d->m_localProcess ? d->m_localProcess->error() : QProcess::UnknownError;
    return d->m_remoteError;
}

void ApplicationLauncherPrivate::localProcessError(QProcess::ProcessError error)
{
    // TODO: why below handlings are different?
    if (m_useTerminal) {
        emit q->appendMessage(m_localProcess->errorString(), ErrorMessageFormat);
        if (m_processRunning && m_localProcess->processId() == 0) {
            m_processRunning = false;
            emit q->processExited(-1, QProcess::NormalExit);
        }
    } else {
        QString error;
        QProcess::ExitStatus status = QProcess::NormalExit;
        switch (m_localProcess->error()) {
        case QProcess::FailedToStart:
            error = ApplicationLauncher::tr("Failed to start program. Path or permissions wrong?");
            break;
        case QProcess::Crashed:
            status = QProcess::CrashExit;
            break;
        default:
            error = ApplicationLauncher::tr("Some error has occurred while running the program.");
        }
        if (!error.isEmpty())
            emit q->appendMessage(error, ErrorMessageFormat);
        if (m_processRunning && !isLocalRunning()) {
            m_processRunning = false;
            emit q->processExited(-1, status);
        }
    }
    emit q->error(error);
}

void ApplicationLauncherPrivate::readLocalStandardOutput()
{
    const QByteArray data = m_localProcess->readAllStandardOutput();
    const QString msg = m_outputCodec->toUnicode(
                data.constData(), data.length(), &m_outputCodecState);
    emit q->appendMessage(msg, StdOutFormat, false);
}

void ApplicationLauncherPrivate::readLocalStandardError()
{
    const QByteArray data = m_localProcess->readAllStandardError();
    const QString msg = m_outputCodec->toUnicode(
                data.constData(), data.length(), &m_errorCodecState);
    emit q->appendMessage(msg, StdErrFormat, false);
}

void ApplicationLauncherPrivate::cannotRetrieveLocalDebugOutput()
{
#ifdef Q_OS_WIN
    disconnect(WinDebugInterface::instance(), nullptr, this, nullptr);
    emit q->appendMessage(ApplicationLauncher::msgWinCannotRetrieveDebuggingOutput(), ErrorMessageFormat);
#endif
}

void ApplicationLauncherPrivate::checkLocalDebugOutput(qint64 pid, const QString &message)
{
    if (m_listeningPid == pid)
        emit q->appendMessage(message, DebugFormat);
}

void ApplicationLauncherPrivate::localProcessDone(int exitCode, QProcess::ExitStatus status)
{
    QTimer::singleShot(100, this, [this, exitCode, status]() {
        m_listeningPid = 0;
        emit q->processExited(exitCode, status);
    });
}

QString ApplicationLauncher::msgWinCannotRetrieveDebuggingOutput()
{
    return tr("Cannot retrieve debugging output.") + QLatin1Char('\n');
}

void ApplicationLauncherPrivate::handleProcessStarted()
{
    m_listeningPid = applicationPID();
    emit q->processStarted();
}

void ApplicationLauncher::start(const Runnable &runnable)
{
    d->start(runnable, IDevice::ConstPtr(), true);
}

void ApplicationLauncher::start(const Runnable &runnable, const IDevice::ConstPtr &device)
{
    d->start(runnable, device, false);
}

void ApplicationLauncherPrivate::start(const Runnable &runnable, const IDevice::ConstPtr &device, bool local)
{
    m_isLocal = local;

    if (m_isLocal) {
        const QtcProcess::TerminalMode terminalMode = m_useTerminal
                ? QtcProcess::TerminalOn : QtcProcess::TerminalOff;
        m_localProcess.reset(new QtcProcess(terminalMode, this));
        m_localProcess->setProcessChannelMode(m_processChannelMode);

        if (m_processChannelMode == QProcess::SeparateChannels) {
            connect(m_localProcess.get(), &QtcProcess::readyReadStandardError,
                    this, &ApplicationLauncherPrivate::readLocalStandardError);
        }
        if (!m_useTerminal) {
            connect(m_localProcess.get(), &QtcProcess::readyReadStandardOutput,
                    this, &ApplicationLauncherPrivate::readLocalStandardOutput);
        }

        connect(m_localProcess.get(), &QtcProcess::started,
                this, &ApplicationLauncherPrivate::handleProcessStarted);
        connect(m_localProcess.get(), &QtcProcess::finished, this, [this] {
            localProcessDone(m_localProcess->exitCode(), m_localProcess->exitStatus());
        });
        connect(m_localProcess.get(), &QtcProcess::errorOccurred,
                this, &ApplicationLauncherPrivate::localProcessError);


        // Work around QTBUG-17529 (QtDeclarative fails with 'File name case mismatch' ...)
        const FilePath fixedPath = runnable.workingDirectory.normalizedPathName();
        m_localProcess->setWorkingDirectory(fixedPath);

        Environment env = runnable.environment;
        if (m_runAsRoot)
            RunControl::provideAskPassEntry(env);

        m_localProcess->setEnvironment(env);

        m_processRunning = true;
    #ifdef Q_OS_WIN
        if (!WinDebugInterface::instance()->isRunning())
            WinDebugInterface::instance()->start(); // Try to start listener again...
    #endif

        CommandLine cmdLine = runnable.command;

        if (HostOsInfo::isMacHost()) {
            CommandLine disclaim(Core::ICore::libexecPath("disclaim"));
            disclaim.addCommandLineAsArgs(cmdLine);
            cmdLine = disclaim;
        }

        m_localProcess->setRunAsRoot(m_runAsRoot);
        m_localProcess->setCommand(cmdLine);
        m_localProcess->start();
    } else {
        QTC_ASSERT(m_state == Inactive, return);

        m_state = Run;
        if (!device) {
            doReportError(ApplicationLauncher::tr("Cannot run: No device."));
            setFinished();
            return;
        }

        if (!device->canCreateProcess()) {
            doReportError(ApplicationLauncher::tr("Cannot run: Device is not able to create processes."));
            setFinished();
            return;
        }

        if (!device->isEmptyCommandAllowed() && runnable.command.isEmpty()) {
            doReportError(ApplicationLauncher::tr("Cannot run: No command given."));
            setFinished();
            return;
        }

        m_stopRequested = false;
        m_remoteExitStatus = QProcess::NormalExit;

        m_deviceProcess = device->createProcess(this);
        m_deviceProcess->setRunInTerminal(m_useTerminal);
        connect(m_deviceProcess, &DeviceProcess::started,
                q, &ApplicationLauncher::processStarted);
        connect(m_deviceProcess, &DeviceProcess::readyReadStandardOutput,
                this, &ApplicationLauncherPrivate::handleRemoteStdout);
        connect(m_deviceProcess, &DeviceProcess::readyReadStandardError,
                this, &ApplicationLauncherPrivate::handleRemoteStderr);
        connect(m_deviceProcess, &DeviceProcess::errorOccurred,
                this, &ApplicationLauncherPrivate::handleApplicationError);
        connect(m_deviceProcess, &DeviceProcess::finished,
                this, &ApplicationLauncherPrivate::handleApplicationFinished);
        m_deviceProcess->start(runnable);
    }
}

void ApplicationLauncherPrivate::handleApplicationError(QProcess::ProcessError error)
{
    if (error == QProcess::FailedToStart) {
        doReportError(ApplicationLauncher::tr("Application failed to start: %1")
                         .arg(m_deviceProcess->errorString()));
        setFinished();
    }
}

void ApplicationLauncherPrivate::setFinished()
{
    if (m_state == Inactive)
        return;

    int exitCode = 0;
    if (m_deviceProcess)
        exitCode = m_deviceProcess->exitCode();

    m_state = Inactive;
    emit q->processExited(exitCode, m_remoteExitStatus);
}

void ApplicationLauncherPrivate::handleApplicationFinished()
{
    QTC_ASSERT(m_state == Run, return);

    if (m_deviceProcess->exitStatus() == QProcess::CrashExit) {
        doReportError(m_deviceProcess->errorString(), QProcess::Crashed);
    } else {
        const int exitCode = m_deviceProcess->exitCode();
        if (exitCode != 0) {
            doReportError(ApplicationLauncher::tr("Application finished with exit code %1.")
                          .arg(exitCode), QProcess::UnknownError);
        } else {
            emit q->appendMessage(ApplicationLauncher::tr("Application finished with exit code 0."),
                                  Utils::NormalMessageFormat);
        }
    }
    setFinished();
}

void ApplicationLauncherPrivate::handleRemoteStdout()
{
    QTC_ASSERT(m_state == Run, return);
    const QByteArray output = m_deviceProcess->readAllStandardOutput();
    emit q->appendMessage(QString::fromUtf8(output), Utils::StdOutFormat, false);
}

void ApplicationLauncherPrivate::handleRemoteStderr()
{
    QTC_ASSERT(m_state == Run, return);
    const QByteArray output = m_deviceProcess->readAllStandardError();
    emit q->appendMessage(QString::fromUtf8(output), Utils::StdErrFormat, false);
}

void ApplicationLauncherPrivate::doReportError(const QString &message, QProcess::ProcessError error)
{
    m_remoteErrorString = message;
    m_remoteError = error;
    m_remoteExitStatus = QProcess::CrashExit;
    emit q->error(error);
}

} // namespace ProjectExplorer
