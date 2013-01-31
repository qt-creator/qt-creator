/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "applicationlauncher.h"
#ifdef Q_OS_WIN
#include "windebuginterface.h"
#endif

#include <coreplugin/icore.h>

#include <utils/consoleprocess.h>
#include <utils/qtcprocess.h>
#ifdef Q_OS_WIN
#include <utils/winutils.h>
#endif

#include <QTimer>
#include <QTextCodec>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "projectexplorerconstants.h"
#include "projectexplorer.h"
#include "projectexplorersettings.h"

/*!
    \class ProjectExplorer::ApplicationLauncher

    \brief Application launcher of the ProjectExplorer plugin.

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
    bool m_processRunning;
};

ApplicationLauncherPrivate::ApplicationLauncherPrivate() :
    m_currentMode(ApplicationLauncher::Gui),
    m_outputCodec(QTextCodec::codecForLocale())
{
}

ApplicationLauncher::ApplicationLauncher(QObject *parent)
    : QObject(parent), d(new ApplicationLauncherPrivate)
{
    if (ProjectExplorerPlugin::instance()->projectExplorerSettings().mergeStdErrAndStdOut){
        d->m_guiProcess.setReadChannelMode(QProcess::MergedChannels);
    } else {
        d->m_guiProcess.setReadChannelMode(QProcess::SeparateChannels);
        connect(&d->m_guiProcess, SIGNAL(readyReadStandardError()),
            this, SLOT(readStandardError()));
    }
    connect(&d->m_guiProcess, SIGNAL(readyReadStandardOutput()),
        this, SLOT(readStandardOutput()));
    connect(&d->m_guiProcess, SIGNAL(error(QProcess::ProcessError)),
        this, SLOT(guiProcessError()));
    connect(&d->m_guiProcess, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(processDone(int,QProcess::ExitStatus)));
    connect(&d->m_guiProcess, SIGNAL(started()),
            this, SLOT(bringToForeground()));

#ifdef Q_OS_UNIX
    d->m_consoleProcess.setSettings(Core::ICore::settings());
#endif
    connect(&d->m_consoleProcess, SIGNAL(processStarted()),
            this, SIGNAL(processStarted()));
    connect(&d->m_consoleProcess, SIGNAL(processError(QString)),
            this, SLOT(consoleProcessError(QString)));
    connect(&d->m_consoleProcess, SIGNAL(processStopped()),
            this, SLOT(processStopped()));

#ifdef Q_OS_WIN
    connect(WinDebugInterface::instance(), SIGNAL(cannotRetrieveDebugOutput()),
            this, SLOT(cannotRetrieveDebugOutput()));
    connect(WinDebugInterface::instance(), SIGNAL(debugOutput(qint64,QString)),
            this, SLOT(checkDebugOutput(qint64,QString)));
#endif
}

ApplicationLauncher::~ApplicationLauncher()
{
    delete d;
}

void ApplicationLauncher::setWorkingDirectory(const QString &dir)
{
#ifdef Q_OS_WIN
    // Work around QTBUG-17529 (QtDeclarative fails with 'File name case mismatch' ...)
    const QString fixedPath = Utils::normalizePathName(dir);
#else
#   define fixedPath dir
#endif

    d->m_guiProcess.setWorkingDirectory(fixedPath);
    d->m_consoleProcess.setWorkingDirectory(fixedPath);

#ifndef Q_OS_WIN
#   undef fixedPath
#endif
}

void ApplicationLauncher::setEnvironment(const Utils::Environment &env)
{
    d->m_guiProcess.setEnvironment(env);
    d->m_consoleProcess.setEnvironment(env);
}

void ApplicationLauncher::start(Mode mode, const QString &program, const QString &args)
{
    d->m_processRunning = true;
#ifdef Q_OS_WIN
    if (!WinDebugInterface::instance()->isRunning())
        WinDebugInterface::instance()->start(); // Try to start listener again...
#endif

    d->m_currentMode = mode;
    if (mode == Gui) {
        d->m_guiProcess.setCommand(program, args);
        d->m_guiProcess.start();
    } else {
        d->m_consoleProcess.start(program, args);
    }
}

void ApplicationLauncher::stop()
{
    if (!isRunning())
        return;
    if (d->m_currentMode == Gui) {
        d->m_guiProcess.terminate();
        if (!d->m_guiProcess.waitForFinished(1000)) { // This is blocking, so be fast.
            d->m_guiProcess.kill();
            d->m_guiProcess.waitForFinished();
        }
    } else {
        d->m_consoleProcess.stop();
        processStopped();
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
    qint64 result = 0;
    if (!isRunning())
        return result;

    if (d->m_currentMode == Console) {
        result = d->m_consoleProcess.applicationPID();
    } else {
#ifdef Q_OS_WIN
        result = (qint64)d->m_guiProcess.pid()->dwProcessId;
#else
        result = (qint64)d->m_guiProcess.pid();
#endif
    }
    return result;
}

void ApplicationLauncher::guiProcessError()
{
    QString error;
    switch (d->m_guiProcess.error()) {
    case QProcess::FailedToStart:
        error = tr("Failed to start program. Path or permissions wrong?");
        break;
    case QProcess::Crashed:
        error = tr("The program has unexpectedly finished.");
        break;
    default:
        error = tr("Some error has occurred while running the program.");
    }
    emit appendMessage(error + QLatin1Char('\n'), Utils::ErrorMessageFormat);
    if (d->m_processRunning && !isRunning()) {
        d->m_processRunning = false;
        emit processExited(-1);
    }
}

void ApplicationLauncher::consoleProcessError(const QString &error)
{
    emit appendMessage(error + QLatin1Char('\n'), Utils::ErrorMessageFormat);
    if (d->m_processRunning && d->m_consoleProcess.applicationPID() == 0) {
        d->m_processRunning = false;
        emit processExited(-1);
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

void ApplicationLauncher::checkDebugOutput(qint64 pid, const QString &message)
{
    if (applicationPID() == pid)
        emit appendMessage(message, Utils::DebugFormat);
}
#endif

void ApplicationLauncher::processStopped()
{
    emit processExited(0);
}

void ApplicationLauncher::processDone(int exitCode, QProcess::ExitStatus)
{
    emit processExited(exitCode);
}

void ApplicationLauncher::bringToForeground()
{
    emit bringToForegroundRequested(applicationPID());
    emit processStarted();
}

QString ApplicationLauncher::msgWinCannotRetrieveDebuggingOutput()
{
    return tr("Cannot retrieve debugging output.\n");
}

} // namespace ProjectExplorer
