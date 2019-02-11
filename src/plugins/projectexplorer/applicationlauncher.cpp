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
    void localConsoleProcessError(const QString &error);
    void readLocalStandardOutput();
    void readLocalStandardError();
    void cannotRetrieveLocalDebugOutput();
    void checkLocalDebugOutput(qint64 pid, const QString &message);
    void localProcessDone(int, QProcess::ExitStatus);
    void bringToForeground();
    qint64 applicationPID() const;
    bool isRunning() const;
    bool isRemoteRunning() const;

    // Remote
    void doReportError(const QString &message);
    void handleRemoteStderr();
    void handleRemoteStdout();
    void handleApplicationFinished();
    void setFinished();
    void handleApplicationError(QProcess::ProcessError error);

public:
    ApplicationLauncher *q;

    bool m_isLocal = true;

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
    State m_state = Inactive;
    bool m_stopRequested = false;
    bool m_success = false;
};

} // Internal

ApplicationLauncherPrivate::ApplicationLauncherPrivate(ApplicationLauncher *parent)
    : q(parent), m_outputCodec(QTextCodec::codecForLocale())
{
    if (ProjectExplorerPlugin::projectExplorerSettings().mergeStdErrAndStdOut){
        m_guiProcess.setProcessChannelMode(QProcess::MergedChannels);
    } else {
        m_guiProcess.setProcessChannelMode(QProcess::SeparateChannels);
        connect(&m_guiProcess, &QProcess::readyReadStandardError,
                this, &ApplicationLauncherPrivate::readLocalStandardError);
    }
    connect(&m_guiProcess, &QProcess::readyReadStandardOutput,
            this, &ApplicationLauncherPrivate::readLocalStandardOutput);
    connect(&m_guiProcess, &QProcess::errorOccurred,
            this, &ApplicationLauncherPrivate::localGuiProcessError);
    connect(&m_guiProcess, static_cast<void (QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished),
            this, &ApplicationLauncherPrivate::localProcessDone);
    connect(&m_guiProcess, &QProcess::started,
            this, &ApplicationLauncherPrivate::handleProcessStarted);
    connect(&m_guiProcess, &QProcess::errorOccurred,
            q, &ApplicationLauncher::error);

    m_consoleProcess.setSettings(Core::ICore::settings());

    connect(&m_consoleProcess, &ConsoleProcess::processStarted,
            this, &ApplicationLauncherPrivate::handleProcessStarted);
    connect(&m_consoleProcess, &ConsoleProcess::processError,
            this, &ApplicationLauncherPrivate::localConsoleProcessError);
    connect(&m_consoleProcess, &ConsoleProcess::processStopped,
            this, &ApplicationLauncherPrivate::localProcessDone);
    connect(&m_consoleProcess,
            static_cast<void (ConsoleProcess::*)(QProcess::ProcessError)>(&ConsoleProcess::error),
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
            if (!m_guiProcess.waitForFinished(1000) && m_guiProcess.state() == QProcess::Running) { // This is blocking, so be fast.
                m_guiProcess.kill();
                m_guiProcess.waitForFinished();
            }
        }
    } else {
        if (m_stopRequested)
            return;
        m_stopRequested = true;
        m_success = false;
        emit q->reportProgress(ApplicationLauncher::tr("User requested stop. Shutting down..."));
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

bool ApplicationLauncher::isRemoteRunning() const
{
    return d->isRemoteRunning();
}

bool ApplicationLauncherPrivate::isRunning() const
{
    if (m_useTerminal)
        return m_consoleProcess.isRunning();
    return m_guiProcess.state() != QProcess::NotRunning;
}

bool ApplicationLauncherPrivate::isRemoteRunning() const
{
    return m_isLocal ? false : m_deviceProcess->state() == QProcess::Running;
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
        return m_consoleProcess.applicationPID();

    return m_guiProcess.processId();
}

QString ApplicationLauncher::errorString() const
{
    if (d->m_useTerminal)
        return d->m_consoleProcess.errorString();
    else
        return d->m_guiProcess.errorString();
}

QProcess::ProcessError ApplicationLauncher::processError() const
{
    if (d->m_useTerminal)
        return d->m_consoleProcess.error();
    else
        return d->m_guiProcess.error();
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
        error = ApplicationLauncher::tr("The program has unexpectedly finished.");
        status = QProcess::CrashExit;
        break;
    default:
        error = ApplicationLauncher::tr("Some error has occurred while running the program.");
    }
    emit q->appendMessage(error, ErrorMessageFormat);
    if (m_processRunning && !isRunning()) {
        m_processRunning = false;
        emit q->processExited(-1, status);
    }
}

void ApplicationLauncherPrivate::localConsoleProcessError(const QString &error)
{
    emit q->appendMessage(error, ErrorMessageFormat);
    if (m_processRunning && m_consoleProcess.applicationPID() == 0) {
        m_processRunning = false;
        emit q->processExited(-1, QProcess::NormalExit);
    }
}

void ApplicationLauncherPrivate::readLocalStandardOutput()
{
    QByteArray data = m_guiProcess.readAllStandardOutput();
    QString msg = m_outputCodec->toUnicode(
            data.constData(), data.length(), &m_outputCodecState);
    emit q->appendMessage(msg, StdOutFormatSameLine, false);
}

void ApplicationLauncherPrivate::readLocalStandardError()
{
    QByteArray data = m_guiProcess.readAllStandardError();
    QString msg = m_outputCodec->toUnicode(
            data.constData(), data.length(), &m_errorCodecState);
    emit q->appendMessage(msg, StdErrFormatSameLine, false);
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
        const QString fixedPath = FileUtils::normalizePathName(runnable.workingDirectory);
        m_guiProcess.setWorkingDirectory(fixedPath);
        m_consoleProcess.setWorkingDirectory(fixedPath);
        m_guiProcess.setEnvironment(runnable.environment);
        m_consoleProcess.setEnvironment(runnable.environment);

        m_processRunning = true;
    #ifdef Q_OS_WIN
        if (!WinDebugInterface::instance()->isRunning())
            WinDebugInterface::instance()->start(); // Try to start listener again...
    #endif

        if (!m_useTerminal) {
            m_guiProcess.setCommand(runnable.executable, runnable.commandLineArguments);
            m_guiProcess.closeWriteChannel();
            m_guiProcess.start();
        } else {
            m_consoleProcess.start(runnable.executable, runnable.commandLineArguments);
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

        if (runnable.executable.isEmpty()) {
            doReportError(ApplicationLauncher::tr("Cannot run: No command given."));
            setFinished();
            return;
        }

        m_stopRequested = false;
        m_success = true;

        m_deviceProcess = device->createProcess(this);
        m_deviceProcess->setRunInTerminal(m_useTerminal);
        connect(m_deviceProcess, &DeviceProcess::started,
                q, &ApplicationLauncher::remoteProcessStarted);
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

    if (m_deviceProcess) {
        m_deviceProcess->disconnect(this);
        m_deviceProcess->deleteLater();
        m_deviceProcess = nullptr;
    }

    m_state = Inactive;
    emit q->finished(m_success);
}

void ApplicationLauncherPrivate::handleApplicationFinished()
{
    QTC_ASSERT(m_state == Run, return);

    if (m_deviceProcess->exitStatus() == QProcess::CrashExit) {
        doReportError(m_deviceProcess->errorString());
    } else {
        const int exitCode = m_deviceProcess->exitCode();
        if (exitCode != 0) {
            doReportError(ApplicationLauncher::tr("Application finished with exit code %1.").arg(exitCode));
        } else {
            emit q->reportProgress(ApplicationLauncher::tr("Application finished with exit code 0."));
        }
    }
    setFinished();
}

void ApplicationLauncherPrivate::handleRemoteStdout()
{
    QTC_ASSERT(m_state == Run, return);
    const QByteArray output = m_deviceProcess->readAllStandardOutput();
    emit q->remoteStdout(QString::fromUtf8(output));
}

void ApplicationLauncherPrivate::handleRemoteStderr()
{
    QTC_ASSERT(m_state == Run, return);
    const QByteArray output = m_deviceProcess->readAllStandardError();
    emit q->remoteStderr(QString::fromUtf8(output));
}

void ApplicationLauncherPrivate::doReportError(const QString &message)
{
    m_success = false;
    emit q->reportError(message);
}

} // namespace ProjectExplorer
