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
#endif

#include <coreplugin/icore.h>

#include <utils/consoleprocess.h>
#include <utils/fileutils.h>
#include <utils/qtcprocess.h>

#include <QTextCodec>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "projectexplorer.h"
#include "projectexplorersettings.h"
#include "runnables.h"

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

class ApplicationLauncherPrivate : public QObject
{
public:
    explicit ApplicationLauncherPrivate(ApplicationLauncher *parent);

    void handleLocalProcessStarted();
    void localGuiProcessError();
    void localConsoleProcessError(const QString &error);
    void readLocalStandardOutput();
    void readLocalStandardError();
    void cannotRetrieveLocalDebugOutput();
    void checkLocalDebugOutput(qint64 pid, const QString &message);
    void localProcessDone(int, QProcess::ExitStatus);
    void bringToForeground();
    void localStop();
    void localStart(const StandardRunnable &runnable);
    qint64 applicationPID() const;
    bool isRunning() const;

public:
    ApplicationLauncher *q;

    QtcProcess m_guiProcess;
    ConsoleProcess m_consoleProcess;
    ApplicationLauncher::Mode m_currentMode = ApplicationLauncher::Gui;
    // Keep track whether we need to emit a finished signal
    bool m_processRunning = false;

    QTextCodec *m_outputCodec;
    QTextCodec::ConverterState m_outputCodecState;
    QTextCodec::ConverterState m_errorCodecState;

    qint64 m_listeningPid = 0;
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
            this, &ApplicationLauncherPrivate::bringToForeground);
    connect(&m_guiProcess, &QProcess::errorOccurred,
            q, &ApplicationLauncher::error);

    m_consoleProcess.setSettings(Core::ICore::settings());

    connect(&m_consoleProcess, &ConsoleProcess::processStarted,
            this, &ApplicationLauncherPrivate::handleLocalProcessStarted);
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

void ApplicationLauncher::start(const Runnable &runnable)
{
    d->localStart(runnable.as<StandardRunnable>());
}

void ApplicationLauncherPrivate::localStart(const StandardRunnable &runnable)
{
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

    m_currentMode = runnable.runMode;
    if (m_currentMode == ApplicationLauncher::Gui) {
        m_guiProcess.setCommand(runnable.executable, runnable.commandLineArguments);
        m_guiProcess.start();
    } else {
        m_consoleProcess.start(runnable.executable, runnable.commandLineArguments);
    }
}

void ApplicationLauncher::stop()
{
    d->localStop();
}

void ApplicationLauncherPrivate::localStop()
{
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

qint64 ApplicationLauncher::applicationPID() const
{
    return d->applicationPID();
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
        error = tr("Failed to start program. Path or permissions wrong?");
        break;
    case QProcess::Crashed:
        error = tr("The program has unexpectedly finished.");
        status = QProcess::CrashExit;
        break;
    default:
        error = tr("Some error has occurred while running the program.");
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

void ApplicationLauncherPrivate::bringToForeground()
{
    handleLocalProcessStarted();
    emit q->bringToForegroundRequested();
}

QString ApplicationLauncher::msgWinCannotRetrieveDebuggingOutput()
{
    return tr("Cannot retrieve debugging output.") + QLatin1Char('\n');
}

void ApplicationLauncherPrivate::handleLocalProcessStarted()
{
    m_listeningPid = applicationPID();
    emit q->processStarted();
}

} // namespace ProjectExplorer
