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

#include <utils/consoleprocess.h>
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

    \sa Utils::ConsoleProcess
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
    void localGuiProcessError();
    void localConsoleProcessError();
    void readLocalStandardOutput();
    void readLocalStandardError();
    void cannotRetrieveLocalDebugOutput();
    void checkLocalDebugOutput(qint64 pid, const QString &message);
    void localProcessDone(int, QProcess::ExitStatus);
    qint64 applicationPID() const;
    bool isRunning() const;

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
    QtcProcess m_guiProcess;
    ConsoleProcess m_consoleProcess;
    bool m_useTerminal = false;
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

ApplicationLauncherPrivate::ApplicationLauncherPrivate(ApplicationLauncher *parent)
    : q(parent), m_outputCodec(QTextCodec::codecForLocale())
{
    if (ProjectExplorerPlugin::appOutputSettings().mergeChannels) {
        m_guiProcess.setProcessChannelMode(QProcess::MergedChannels);
    } else {
        m_guiProcess.setProcessChannelMode(QProcess::SeparateChannels);
        connect(&m_guiProcess, &QtcProcess::readyReadStandardError,
                this, &ApplicationLauncherPrivate::readLocalStandardError);
    }
    connect(&m_guiProcess, &QtcProcess::readyReadStandardOutput,
            this, &ApplicationLauncherPrivate::readLocalStandardOutput);
    connect(&m_guiProcess, &QtcProcess::errorOccurred,
            this, &ApplicationLauncherPrivate::localGuiProcessError);
    connect(&m_guiProcess, &QtcProcess::finished, this, [this] {
        localProcessDone(m_guiProcess.exitCode(), m_guiProcess.exitStatus());
    });
    connect(&m_guiProcess, &QtcProcess::started,
            this, &ApplicationLauncherPrivate::handleProcessStarted);
    connect(&m_guiProcess, &QtcProcess::errorOccurred,
            q, &ApplicationLauncher::error);

    m_consoleProcess.setSettings(Core::ICore::settings());

    connect(&m_consoleProcess, &ConsoleProcess::started,
            this, &ApplicationLauncherPrivate::handleProcessStarted);
    connect(&m_consoleProcess, &ConsoleProcess::errorOccurred,
            this, &ApplicationLauncherPrivate::localConsoleProcessError);
    connect(&m_consoleProcess, &ConsoleProcess::finished, this, [this] {
        localProcessDone(m_consoleProcess.exitCode(), m_consoleProcess.exitStatus());
    });
    connect(&m_consoleProcess, &ConsoleProcess::errorOccurred,
            q, &ApplicationLauncher::error);

#ifdef Q_OS_WIN
    connect(WinDebugInterface::instance(), &WinDebugInterface::cannotRetrieveDebugOutput,
            this, &ApplicationLauncherPrivate::cannotRetrieveLocalDebugOutput);
    connect(WinDebugInterface::instance(), &WinDebugInterface::debugOutput,
            this, &ApplicationLauncherPrivate::checkLocalDebugOutput);
#endif
}

ApplicationLauncher::ApplicationLauncher(QObject *parent) : QObject(parent),
    d(std::make_unique<ApplicationLauncherPrivate>(this))
{ }

ApplicationLauncher::~ApplicationLauncher() = default;

void ApplicationLauncher::setProcessChannelMode(QProcess::ProcessChannelMode mode)
{
    d->m_guiProcess.setProcessChannelMode(mode);
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
        if (!isRunning())
            return;
        if (m_useTerminal) {
            m_consoleProcess.stop();
            localProcessDone(0, QProcess::CrashExit);
        } else {
            m_guiProcess.terminate();
            if (!m_guiProcess.waitForFinished(1000)) { // This is blocking, so be fast.
                m_guiProcess.kill();
                m_guiProcess.waitForFinished();
            }
        }
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
    return d->isRunning();
}

bool ApplicationLauncher::isLocal() const
{
    return d->m_isLocal;
}

bool ApplicationLauncherPrivate::isRunning() const
{
    if (m_useTerminal)
        return m_consoleProcess.isRunning();
    return m_guiProcess.state() != QProcess::NotRunning;
}

ProcessHandle ApplicationLauncher::applicationPID() const
{
    return ProcessHandle(d->applicationPID());
}

qint64 ApplicationLauncherPrivate::applicationPID() const
{
    if (!isRunning())
        return 0;

    if (m_useTerminal)
        return m_consoleProcess.processId();

    return m_guiProcess.processId();
}

QString ApplicationLauncher::errorString() const
{
    if (d->m_isLocal)
        return d->m_useTerminal ? d->m_consoleProcess.errorString() : d->m_guiProcess.errorString();
    return d->m_remoteErrorString;
}

QProcess::ProcessError ApplicationLauncher::processError() const
{
    if (d->m_isLocal)
        return d->m_useTerminal ? d->m_consoleProcess.error() : d->m_guiProcess.error();
    return d->m_remoteError;
}

void ApplicationLauncherPrivate::localGuiProcessError()
{
    QString error;
    QProcess::ExitStatus status = QProcess::NormalExit;
    switch (m_guiProcess.error()) {
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
    if (m_processRunning && !isRunning()) {
        m_processRunning = false;
        emit q->processExited(-1, status);
    }
}

void ApplicationLauncherPrivate::localConsoleProcessError()
{
    emit q->appendMessage(m_consoleProcess.errorString(), ErrorMessageFormat);
    if (m_processRunning && m_consoleProcess.processId() == 0) {
        m_processRunning = false;
        emit q->processExited(-1, QProcess::NormalExit);
    }
}

void ApplicationLauncherPrivate::readLocalStandardOutput()
{
    QByteArray data = m_guiProcess.readAllStandardOutput();
    QString msg = m_outputCodec->toUnicode(
            data.constData(), data.length(), &m_outputCodecState);
    emit q->appendMessage(msg, StdOutFormat, false);
}

void ApplicationLauncherPrivate::readLocalStandardError()
{
    QByteArray data = m_guiProcess.readAllStandardError();
    QString msg = m_outputCodec->toUnicode(
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
        // Work around QTBUG-17529 (QtDeclarative fails with 'File name case mismatch' ...)
        const FilePath fixedPath = runnable.workingDirectory.normalizedPathName();
        m_guiProcess.setWorkingDirectory(fixedPath);
        m_consoleProcess.setWorkingDirectory(fixedPath);

        Environment env = runnable.environment;
        if (m_runAsRoot)
            RunControl::provideAskPassEntry(env);

        m_guiProcess.setEnvironment(env);
        m_consoleProcess.setEnvironment(env);

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

        if (m_runAsRoot) {
            CommandLine wrapped("sudo", {"-A"});
            wrapped.addCommandLineAsArgs(cmdLine);
            cmdLine = wrapped;
        }

        if (m_useTerminal) {
            m_consoleProcess.setCommand(cmdLine);
            m_consoleProcess.start();
        } else {
            m_guiProcess.setCommand(cmdLine);
            m_guiProcess.start();
        }
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
        connect(m_deviceProcess, &DeviceProcess::error,
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
