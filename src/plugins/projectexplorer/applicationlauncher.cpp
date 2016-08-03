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
#ifdef WITH_JOURNALD
#include "journaldwatcher.h"
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

namespace ProjectExplorer {

#ifdef Q_OS_WIN
using namespace Internal; // for WinDebugInterface
#endif

struct ApplicationLauncherPrivate {
    ApplicationLauncherPrivate();

    Utils::QtcProcess m_guiProcess;
    Utils::ConsoleProcess m_consoleProcess;
    ApplicationLauncher::Mode m_currentMode;

    QTextCodec *m_outputCodec;
    QTextCodec::ConverterState m_outputCodecState;
    QTextCodec::ConverterState m_errorCodecState;
    // Keep track whether we need to emit a finished signal
    bool m_processRunning = false;

    qint64 m_listeningPid = 0;
};

ApplicationLauncherPrivate::ApplicationLauncherPrivate() :
    m_currentMode(ApplicationLauncher::Gui),
    m_outputCodec(QTextCodec::codecForLocale())
{
}

ApplicationLauncher::ApplicationLauncher(QObject *parent) : QObject(parent),
    d(new ApplicationLauncherPrivate)
{
    if (ProjectExplorerPlugin::projectExplorerSettings().mergeStdErrAndStdOut){
        d->m_guiProcess.setReadChannelMode(QProcess::MergedChannels);
    } else {
        d->m_guiProcess.setReadChannelMode(QProcess::SeparateChannels);
        connect(&d->m_guiProcess, &QProcess::readyReadStandardError,
                this, &ApplicationLauncher::readStandardError);
    }
    connect(&d->m_guiProcess, &QProcess::readyReadStandardOutput,
            this, &ApplicationLauncher::readStandardOutput);
    connect(&d->m_guiProcess, &QProcess::errorOccurred,
            this, &ApplicationLauncher::guiProcessError);
    connect(&d->m_guiProcess, static_cast<void (QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished),
            this, &ApplicationLauncher::processDone);
    connect(&d->m_guiProcess, &QProcess::started, this, &ApplicationLauncher::bringToForeground);
    connect(&d->m_guiProcess, &QProcess::errorOccurred, this, &ApplicationLauncher::error);

#ifdef Q_OS_UNIX
    d->m_consoleProcess.setSettings(Core::ICore::settings());
#endif
    connect(&d->m_consoleProcess, &Utils::ConsoleProcess::processStarted,
            this, &ApplicationLauncher::handleProcessStarted);
    connect(&d->m_consoleProcess, &Utils::ConsoleProcess::processError,
            this, &ApplicationLauncher::consoleProcessError);
    connect(&d->m_consoleProcess, &Utils::ConsoleProcess::processStopped,
            this, &ApplicationLauncher::processDone);
    connect(&d->m_consoleProcess,
            static_cast<void (Utils::ConsoleProcess::*)(QProcess::ProcessError)>(&Utils::ConsoleProcess::error),
            this, &ApplicationLauncher::error);

#ifdef Q_OS_WIN
    connect(WinDebugInterface::instance(), &WinDebugInterface::cannotRetrieveDebugOutput,
            this, &ApplicationLauncher::cannotRetrieveDebugOutput);
    connect(WinDebugInterface::instance(), &WinDebugInterface::debugOutput,
            this, &ApplicationLauncher::checkDebugOutput, Qt::BlockingQueuedConnection);
#endif
#ifdef WITH_JOURNALD
    connect(JournaldWatcher::instance(), &JournaldWatcher::journaldOutput,
            this, &ApplicationLauncher::checkDebugOutput);
#endif
}

ApplicationLauncher::~ApplicationLauncher()
{
    delete d;
}

void ApplicationLauncher::setProcessChannelMode(QProcess::ProcessChannelMode mode)
{
    d->m_guiProcess.setProcessChannelMode(mode);
}

void ApplicationLauncher::start(const StandardRunnable &runnable)
{
    // Work around QTBUG-17529 (QtDeclarative fails with 'File name case mismatch' ...)
    const QString fixedPath = Utils::FileUtils::normalizePathName(runnable.workingDirectory);
    d->m_guiProcess.setWorkingDirectory(fixedPath);
    d->m_consoleProcess.setWorkingDirectory(fixedPath);
    d->m_guiProcess.setEnvironment(runnable.environment);
    d->m_consoleProcess.setEnvironment(runnable.environment);

    d->m_processRunning = true;
#ifdef Q_OS_WIN
    if (!WinDebugInterface::instance()->isRunning())
        WinDebugInterface::instance()->start(); // Try to start listener again...
#endif

    d->m_currentMode = runnable.runMode;
    if (d->m_currentMode == Gui) {
        d->m_guiProcess.setCommand(runnable.executable, runnable.commandLineArguments);
        d->m_guiProcess.start();
    } else {
        d->m_consoleProcess.start(runnable.executable, runnable.commandLineArguments);
    }
}

void ApplicationLauncher::stop()
{
    if (!isRunning())
        return;
    if (d->m_currentMode == Gui) {
        d->m_guiProcess.terminate();
        if (!d->m_guiProcess.waitForFinished(1000) && d->m_guiProcess.state() == QProcess::Running) { // This is blocking, so be fast.
            d->m_guiProcess.kill();
            d->m_guiProcess.waitForFinished();
        }
    } else {
        d->m_consoleProcess.stop();
        processDone(0, QProcess::CrashExit);
    }
}

bool ApplicationLauncher::isRunning() const
{
    if (d->m_currentMode == Gui)
        return d->m_guiProcess.state() != QProcess::NotRunning;
    else
        return d->m_consoleProcess.isRunning();
}

qint64 ApplicationLauncher::applicationPID() const
{
    if (!isRunning())
        return 0;

    if (d->m_currentMode == Console)
        return d->m_consoleProcess.applicationPID();
    else
        return d->m_guiProcess.processId();
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

void ApplicationLauncher::guiProcessError()
{
    QString error;
    QProcess::ExitStatus status = QProcess::NormalExit;
    switch (d->m_guiProcess.error()) {
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
    emit appendMessage(error + QLatin1Char('\n'), Utils::ErrorMessageFormat);
    if (d->m_processRunning && !isRunning()) {
        d->m_processRunning = false;
        emit processExited(-1, status);
    }
}

void ApplicationLauncher::consoleProcessError(const QString &error)
{
    emit appendMessage(error + QLatin1Char('\n'), Utils::ErrorMessageFormat);
    if (d->m_processRunning && d->m_consoleProcess.applicationPID() == 0) {
        d->m_processRunning = false;
        emit processExited(-1, QProcess::NormalExit);
    }
}

void ApplicationLauncher::readStandardOutput()
{
    QByteArray data = d->m_guiProcess.readAllStandardOutput();
    QString msg = d->m_outputCodec->toUnicode(
            data.constData(), data.length(), &d->m_outputCodecState);
    emit appendMessage(msg, Utils::StdOutFormatSameLine);
}

void ApplicationLauncher::readStandardError()
{
    QByteArray data = d->m_guiProcess.readAllStandardError();
    QString msg = d->m_outputCodec->toUnicode(
            data.constData(), data.length(), &d->m_errorCodecState);
    emit appendMessage(msg, Utils::StdErrFormatSameLine);
}

#ifdef Q_OS_WIN
void ApplicationLauncher::cannotRetrieveDebugOutput()
{
    disconnect(WinDebugInterface::instance(), 0, this, 0);
    emit appendMessage(msgWinCannotRetrieveDebuggingOutput(), Utils::ErrorMessageFormat);
}
#endif

void ApplicationLauncher::checkDebugOutput(qint64 pid, const QString &message)
{
    if (d->m_listeningPid == pid)
        emit appendMessage(message, Utils::DebugFormat);
}

void ApplicationLauncher::processDone(int exitCode, QProcess::ExitStatus status)
{
    QTimer::singleShot(100, this, [this, exitCode, status]() {
        d->m_listeningPid = 0;
        emit processExited(exitCode, status);
    });
}

void ApplicationLauncher::bringToForeground()
{
    emit bringToForegroundRequested(applicationPID());
    handleProcessStarted();
}

QString ApplicationLauncher::msgWinCannotRetrieveDebuggingOutput()
{
    return tr("Cannot retrieve debugging output.") + QLatin1Char('\n');
}

void ApplicationLauncher::handleProcessStarted()
{
    d->m_listeningPid = applicationPID();
    emit processStarted();
}

} // namespace ProjectExplorer
