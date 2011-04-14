/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "applicationlauncher.h"
#include "consoleprocess.h"

#include <coreplugin/icore.h>

#include <utils/qtcprocess.h>

#include <QtCore/QTimer>
#include <QtCore/QTextCodec>

namespace ProjectExplorer {

struct ApplicationLauncherPrivate {
    ApplicationLauncherPrivate();

    Utils::QtcProcess m_guiProcess;
    Utils::ConsoleProcess m_consoleProcess;
    ApplicationLauncher::Mode m_currentMode;

    QTextCodec *m_outputCodec;
    QTextCodec::ConverterState m_outputCodecState;
    QTextCodec::ConverterState m_errorCodecState;
};

ApplicationLauncherPrivate::ApplicationLauncherPrivate() :
    m_currentMode(ApplicationLauncher::Gui),
    m_outputCodec(QTextCodec::codecForLocale())
{
}

ApplicationLauncher::ApplicationLauncher(QObject *parent)
    : QObject(parent), d(new ApplicationLauncherPrivate)
{
    d->m_guiProcess.setReadChannelMode(QProcess::SeparateChannels);
    connect(&d->m_guiProcess, SIGNAL(error(QProcess::ProcessError)),
        this, SLOT(guiProcessError()));
    connect(&d->m_guiProcess, SIGNAL(readyReadStandardOutput()),
        this, SLOT(readStandardOutput()));
    connect(&d->m_guiProcess, SIGNAL(readyReadStandardError()),
        this, SLOT(readStandardError()));
    connect(&d->m_guiProcess, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(processDone(int, QProcess::ExitStatus)));
    connect(&d->m_guiProcess, SIGNAL(started()),
            this, SLOT(bringToForeground()));

    d->m_consoleProcess.setSettings(Core::ICore::instance()->settings());
    connect(&d->m_consoleProcess, SIGNAL(processMessage(QString,bool)),
            this, SLOT(appendProcessMessage(QString,bool)));
    connect(&d->m_consoleProcess, SIGNAL(processStopped()),
            this, SLOT(processStopped()));
}

ApplicationLauncher::~ApplicationLauncher()
{
}

void ApplicationLauncher::appendProcessMessage(const QString &output, bool onStdErr)
{
    emit appendMessage(output, onStdErr ? ErrorMessageFormat : NormalMessageFormat);
}

void ApplicationLauncher::setWorkingDirectory(const QString &dir)
{
    d->m_guiProcess.setWorkingDirectory(dir);
    d->m_consoleProcess.setWorkingDirectory(dir);
}

void ApplicationLauncher::setEnvironment(const Utils::Environment &env)
{
    d->m_guiProcess.setEnvironment(env);
    d->m_consoleProcess.setEnvironment(env);
}

void ApplicationLauncher::start(Mode mode, const QString &program, const QString &args)
{
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
        result = (qint64)d->m_guiProcess.pid();
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
    emit appendMessage(error + QLatin1Char('\n'), ErrorMessageFormat);
    emit processExited(d->m_guiProcess.exitCode());
}

void ApplicationLauncher::readStandardOutput()
{
    QByteArray data = d->m_guiProcess.readAllStandardOutput();
    QString msg = d->m_outputCodec->toUnicode(
            data.constData(), data.length(), &d->m_outputCodecState);
    emit appendMessage(msg, StdOutFormatSameLine);
}

void ApplicationLauncher::readStandardError()
{
    QByteArray data = d->m_guiProcess.readAllStandardError();
    QString msg = d->m_outputCodec->toUnicode(
            data.constData(), data.length(), &d->m_outputCodecState);
    emit appendMessage(msg, StdErrFormatSameLine);
}

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
}

} // namespace ProjectExplorer
