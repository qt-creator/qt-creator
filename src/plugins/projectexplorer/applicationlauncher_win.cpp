/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "applicationlauncher.h"
#include "consoleprocess.h"
#include "winguiprocess.h"

#include <QDebug>

using namespace ProjectExplorer::Internal;

ApplicationLauncher::ApplicationLauncher(QObject *parent)
    : QObject(parent)
{
    m_currentMode = Gui;

    m_consoleProcess = new ConsoleProcess(this);
    connect(m_consoleProcess, SIGNAL(processError(const QString&)),
            this, SIGNAL(applicationError(const QString&)));
    connect(m_consoleProcess, SIGNAL(processStopped()),
            this, SLOT(processStopped()));

    m_winGuiProcess = new WinGuiProcess(this);
    connect(m_winGuiProcess, SIGNAL(processError(const QString&)),
        this, SIGNAL(applicationError(const QString&)));
    connect(m_winGuiProcess, SIGNAL(receivedDebugOutput(const QString&)),
        this, SLOT(readWinDebugOutput(const QString&)));
    connect(m_winGuiProcess, SIGNAL(processFinished(int)),
            this, SLOT(processFinished(int)));

}

void ApplicationLauncher::setWorkingDirectory(const QString &dir)
{
    m_winGuiProcess->setWorkingDirectory(dir);
    m_consoleProcess->setWorkingDirectory(dir);
}

void ApplicationLauncher::setEnvironment(const QStringList &env)
{
    m_winGuiProcess->setEnvironment(env);
    m_consoleProcess->setEnvironment(env);
}

void ApplicationLauncher::start(Mode mode, const QString &program, const QStringList &args)
{
    qDebug()<<"ApplicationLauncher::start"<<program<<args;
    m_currentMode = mode;
    if (mode == Gui) {
        m_winGuiProcess->start(program, args);
    } else {
        m_consoleProcess->start(program, args);
    }
}

void ApplicationLauncher::stop()
{
    if (m_currentMode == Gui) {
        m_winGuiProcess->stop();
    } else {
        m_consoleProcess->stop();
    }
}

bool ApplicationLauncher::isRunning() const
{
    if (m_currentMode == Gui)
        return m_winGuiProcess->isRunning();
    else
        return m_consoleProcess->isRunning();
}

qint64 ApplicationLauncher::applicationPID() const
{
    qint64 result = 0;
    if (!isRunning())
        return result;

    if (m_currentMode == Console) {
        result = m_consoleProcess->applicationPID();
    } else {
        result = m_winGuiProcess->applicationPID();
    }
    return result;
}

void ApplicationLauncher::readWinDebugOutput(const QString &output)
{
    emit appendOutput(output);
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
