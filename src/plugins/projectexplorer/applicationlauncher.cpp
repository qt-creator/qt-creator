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
#include <windows.h>
#endif

#include <coreplugin/icore.h>

#include <utils/consoleprocess.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include "devicesupport/deviceprocess.h"
#include "projectexplorer.h"
#include "projectexplorersettings.h"
#include "runnables.h"

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
using namespace ProjectExplorer::Internal;

namespace ProjectExplorer {
namespace Internal {

enum State { Inactive, Run };

class ApplicationLauncherPrivate : public QObject
{
public:
    explicit ApplicationLauncherPrivate(ApplicationLauncher *parent);
    ~ApplicationLauncherPrivate() { setFinished(); }

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
    ApplicationLauncher::Mode m_currentMode = ApplicationLauncher::Gui;
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
        m_guiProcess.setReadChannelMode(QProcess::MergedChannels);
    } else {
        m_guiProcess.setReadChannelMode(QProcess::SeparateChannels);
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
            this, &ApplicationLauncherPrivate::checkLocalDebugOutput, Qt::BlockingQueuedConnection);
#endif
}

ApplicationLauncher::ApplicationLauncher(QObject *parent) : QObject(parent),
    d(new ApplicationLauncherPrivate(this))
{
}

ApplicationLauncher::~ApplicationLauncher()
{
    delete d;
}

void ApplicationLauncher::setProcessChannelMode(QProcess::ProcessChannelMode mode)
{
    d->m_guiProcess.setProcessChannelMode(mode);
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
        if (m_currentMode == ApplicationLauncher::Gui) {
            m_guiProcess.terminate();
            if (!m_guiProcess.waitForFinished(1000) && m_guiProcess.state() == QProcess::Running) { // This is blocking, so be fast.
                m_guiProcess.kill();
                m_guiProcess.waitForFinished();
            }
        } else {
            m_consoleProcess.stop();
            localProcessDone(0, QProcess::CrashExit);
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

bool ApplicationLauncherPrivate::isRunning() const
{
    if (m_currentMode == ApplicationLauncher::Gui)
        return m_guiProcess.state() != QProcess::NotRunning;
    return m_consoleProcess.isRunning();
}

ProcessHandle ApplicationLauncher::applicationPID() const
{
    return ProcessHandle(d->applicationPID());
}

qint64 ApplicationLauncherPrivate::applicationPID() const
{
    if (!isRunning())
        return 0;

    if (m_currentMode == ApplicationLauncher::Console)
        return m_consoleProcess.applicationPID();

    return m_guiProcess.processId();
}

QString ApplicationLauncher::errorString() const
{
    if (d->m_currentMode == Gui)
        return d->m_guiProcess.errorString();
    else
        return d->m_consoleProcess.errorString();
}

QProcess::ProcessError ApplicationLauncher::processError() const
{
    if (d->m_currentMode == Gui)
        return d->m_guiProcess.error();
    else
        return d->m_consoleProcess.error();
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
    emit q->appendMessage(error + QLatin1Char('\n'), ErrorMessageFormat);
    if (m_processRunning && !isRunning()) {
        m_processRunning = false;
        emit q->processExited(-1, status);
    }
}

void ApplicationLauncherPrivate::localConsoleProcessError(const QString &error)
{
    emit q->appendMessage(error + QLatin1Char('\n'), ErrorMessageFormat);
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
    emit q->appendMessage(msg, StdOutFormatSameLine);
}

void ApplicationLauncherPrivate::readLocalStandardError()
{
    QByteArray data = m_guiProcess.readAllStandardError();
    QString msg = m_outputCodec->toUnicode(
            data.constData(), data.length(), &m_errorCodecState);
    emit q->appendMessage(msg, StdErrFormatSameLine);
}

void ApplicationLauncherPrivate::cannotRetrieveLocalDebugOutput()
{
#ifdef Q_OS_WIN
    disconnect(WinDebugInterface::instance(), 0, this, 0);
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
        QTC_ASSERT(runnable.is<StandardRunnable>(), return);
        StandardRunnable stdRunnable = runnable.as<StandardRunnable>();

        // Work around QTBUG-17529 (QtDeclarative fails with 'File name case mismatch' ...)
        const QString fixedPath = FileUtils::normalizePathName(stdRunnable.workingDirectory);
        m_guiProcess.setWorkingDirectory(fixedPath);
        m_consoleProcess.setWorkingDirectory(fixedPath);
        m_guiProcess.setEnvironment(stdRunnable.environment);
        m_consoleProcess.setEnvironment(stdRunnable.environment);

        m_processRunning = true;
    #ifdef Q_OS_WIN
        if (!WinDebugInterface::instance()->isRunning())
            WinDebugInterface::instance()->start(); // Try to start listener again...
    #endif

        m_currentMode = stdRunnable.runMode;
        if (m_currentMode == ApplicationLauncher::Gui) {
            m_guiProcess.setCommand(stdRunnable.executable, stdRunnable.commandLineArguments);
            m_guiProcess.start();
        } else {
            m_consoleProcess.start(stdRunnable.executable, stdRunnable.commandLineArguments);
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

        if (runnable.is<StandardRunnable>() && runnable.as<StandardRunnable>().executable.isEmpty()) {
            doReportError(ApplicationLauncher::tr("Cannot run: No command given."));
            setFinished();
            return;
        }

        m_stopRequested = false;
        m_success = true;

        m_deviceProcess = device->createProcess(this);
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
        m_deviceProcess = 0;
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
    emit q->remoteStdout(m_deviceProcess->readAllStandardOutput());
}

void ApplicationLauncherPrivate::handleRemoteStderr()
{
    QTC_ASSERT(m_state == Run, return);
    emit q->remoteStderr(m_deviceProcess->readAllStandardError());
}

void ApplicationLauncherPrivate::doReportError(const QString &message)
{
    m_success = false;
    emit q->reportError(message);
}

} // namespace ProjectExplorer
