/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "applicationlauncher.h"
#include "consoleprocess.h"
#include "winguiprocess.h"

#include <utils/winutils.h>

#include <QtCore/QDebug>

namespace ProjectExplorer {

struct ApplicationLauncherPrivate {
    ApplicationLauncherPrivate() : m_currentMode(ApplicationLauncher::Gui) {}

    Utils::ConsoleProcess m_consoleProcess;
    ApplicationLauncher::Mode m_currentMode;
    Internal::WinGuiProcess m_winGuiProcess;
};

ApplicationLauncher::ApplicationLauncher(QObject *parent)
    : QObject(parent), d(new ApplicationLauncherPrivate)
{
    connect(&d->m_consoleProcess, SIGNAL(processMessage(QString,bool)),
            this, SLOT(appendProcessMessage(QString,bool)));
    connect(&d->m_consoleProcess, SIGNAL(processStopped()),
            this, SLOT(processStopped()));

    connect(&d->m_winGuiProcess, SIGNAL(processMessage(QString, bool)),
        this, SLOT(appendProcessMessage(QString,bool)));
    connect(&d->m_winGuiProcess, SIGNAL(receivedDebugOutput(QString, bool)),
        this, SLOT(readWinDebugOutput(QString, bool)));
    connect(&d->m_winGuiProcess, SIGNAL(processFinished(int)),
            this, SLOT(processFinished(int)));
}

ApplicationLauncher::~ApplicationLauncher()
{
}

void ApplicationLauncher::setWorkingDirectory(const QString &dir)
{
    // Work around QTBUG-17529 (QtDeclarative fails with 'File name case mismatch' ...)
    QString fixedPath = dir;
    const QString longPath = Utils::getLongPathName(dir);
    if (!longPath.isEmpty())
        fixedPath = longPath;

    d->m_winGuiProcess.setWorkingDirectory(fixedPath);
    d->m_consoleProcess.setWorkingDirectory(fixedPath);
}

void ApplicationLauncher::setEnvironment(const Utils::Environment &env)
{
    d->m_winGuiProcess.setEnvironment(env);
    d->m_consoleProcess.setEnvironment(env);
}

void ApplicationLauncher::start(Mode mode, const QString &program, const QString &args)
{
    d->m_currentMode = mode;
    if (mode == Gui) {
        d->m_winGuiProcess.start(program, args);
    } else {
        d->m_consoleProcess.start(program, args);
    }
}

void ApplicationLauncher::stop()
{
    if (!isRunning())
        return;
    if (d->m_currentMode == Gui) {
        d->m_winGuiProcess.stop();
    } else {
        d->m_consoleProcess.stop();
        processStopped();
    }
}

bool ApplicationLauncher::isRunning() const
{
    if (d->m_currentMode == Gui)
        return d->m_winGuiProcess.isRunning();
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
        result = d->m_winGuiProcess.applicationPID();
    }
    return result;
}

void ApplicationLauncher::appendProcessMessage(const QString &output, bool onStdErr)
{
    emit appendMessage(output, onStdErr ? ErrorMessageFormat : NormalMessageFormat);
}

void ApplicationLauncher::readWinDebugOutput(const QString &output,
                                             bool onStdErr)
{
    emit appendMessage(output, onStdErr ? StdErrFormat : StdOutFormat);
}

void ApplicationLauncher::processStopped()
{
    emit processExited(0);
}

void ApplicationLauncher::processFinished(int exitCode)
{
    emit processExited(exitCode);
}

void ApplicationLauncher::bringToForeground()
{
}

} // namespace ProjectExplorer
