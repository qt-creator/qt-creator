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

#include "abstractprocessstep.h"
#include "buildstep.h"
#include "project.h"

#include <QtCore/QProcess>
#include <QtCore/QEventLoop>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtGui/QTextDocument>

using namespace ProjectExplorer;

AbstractProcessStep::AbstractProcessStep(Project *pro)
  : BuildStep(pro)
{
}

void AbstractProcessStep::setCommand(const QString &buildConfiguration, const QString &cmd)
{
    setValue(buildConfiguration, "abstractProcess.command", cmd);
}

QString AbstractProcessStep::command(const QString &buildConfiguration) const
{
    return value(buildConfiguration, "abstractProcess.command").toString();
}

void AbstractProcessStep::setWorkingDirectory(const QString &buildConfiguration, const QString &workingDirectory)
{
    setValue(buildConfiguration, "abstractProcess.workingDirectory", workingDirectory);
}

QString AbstractProcessStep::workingDirectory(const QString &buildConfiguration) const
{
    return value(buildConfiguration, "abstractProcess.workingDirectory").toString();
}

void AbstractProcessStep::setArguments(const QString &buildConfiguration, const QStringList &arguments)
{
    setValue(buildConfiguration, "abstractProcess.arguments", arguments);
}

QStringList AbstractProcessStep::arguments(const QString &buildConfiguration) const
{
    return value(buildConfiguration, "abstractProcess.arguments").toStringList();
}

void AbstractProcessStep::setEnabled(const QString &buildConfiguration, bool b)
{
    setValue(buildConfiguration, "abstractProcess.enabled", b);
}

bool AbstractProcessStep::enabled(const QString &buildConfiguration) const
{
    return value(buildConfiguration, "abstractProcess.enabled").toBool();
}

void AbstractProcessStep::setEnvironment(const QString &buildConfiguration, Environment env)
{
    setValue(buildConfiguration, "abstractProcess.Environment", env.toStringList());
}

Environment AbstractProcessStep::environment(const QString &buildConfiguration) const
{
    return Environment(value(buildConfiguration, "abstractProcess.Environment").toStringList());
}

bool AbstractProcessStep::init(const QString &name)
{
    m_command = value(name, "abstractProcess.command").toString();
    m_arguments = value(name, "abstractProcess.arguments").toStringList();
    QVariant var = value(name, "abstractProcess.enabled");
    m_enabled =  var.isValid() && var.toBool();
    m_workingDirectory = value(name, "abstractProcess.workingDirectory").toString();
    m_environment = Environment(value(name, "abstractProcess.Environment").toStringList());
    return true;
}

void AbstractProcessStep::run(QFutureInterface<bool> & fi)
{
    m_futureInterface = &fi;
    if (!m_enabled) {
        fi.reportResult(true);
        return;
    }
    QDir wd(m_workingDirectory);
    if (!wd.exists())
        wd.mkpath(wd.absolutePath());

    m_process = new QProcess();
    m_process->setWorkingDirectory(m_workingDirectory);
    m_process->setEnvironment(m_environment.toStringList());

    connect(m_process, SIGNAL(readyReadStandardOutput()),
            this, SLOT(processReadyReadStdOutput()),
            Qt::DirectConnection);
    connect(m_process, SIGNAL(readyReadStandardError()),
            this, SLOT(processReadyReadStdError()),
            Qt::DirectConnection);

    connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(slotProcessFinished(int, QProcess::ExitStatus)),
            Qt::DirectConnection);

    m_process->start(m_command, m_arguments);
    if (!m_process->waitForStarted()) {
        processStartupFailed();
        delete m_process;
        m_process = 0;
        fi.reportResult(false);
        return;
    }
    processStarted();

    m_timer = new QTimer();
    connect(m_timer, SIGNAL(timeout()), this, SLOT(checkForCancel()), Qt::DirectConnection);
    m_timer->start(500);
    m_eventLoop = new QEventLoop;
    m_eventLoop->exec();
    m_timer->stop();
    delete m_timer;

    // The process has finished, leftover data is read in processFinished
    bool returnValue = processFinished(m_process->exitCode(), m_process->exitStatus());

    delete m_process;
    m_process = 0;
    delete m_eventLoop;
    m_eventLoop = 0;
    fi.reportResult(returnValue);
    return;
}

void AbstractProcessStep::processStarted()
{
    emit addToOutputWindow(tr("<font color=\"#0000ff\">Starting: %1 %2</font>\n").arg(m_command, Qt::escape(m_arguments.join(" "))));
}

bool AbstractProcessStep::processFinished(int exitCode, QProcess::ExitStatus status)
{
    const bool ok = (status == QProcess::NormalExit && exitCode == 0);
    if (ok) {
        emit addToOutputWindow(tr("<font color=\"#0000ff\">Exited with code %1.</font>").arg(m_process->exitCode()));
    } else {
        emit addToOutputWindow(tr("<font color=\"#ff0000\"><b>Exited with code %1.</b></font>").arg(m_process->exitCode()));
    }
    return ok;
}

void AbstractProcessStep::processStartupFailed()
{
   emit addToOutputWindow(tr("<font color=\"#ff0000\">Could not start process %1 </b></font>").arg(m_command));
}

void AbstractProcessStep::processReadyReadStdOutput()
{
    m_process->setReadChannel(QProcess::StandardOutput);
    while (m_process->canReadLine()) {
        QString line = QString::fromLocal8Bit(m_process->readLine()).trimmed();
        stdOut(line);
    }
}

void AbstractProcessStep::stdOut(const QString &line)
{
    emit addToOutputWindow(Qt::escape(line));
}

void AbstractProcessStep::processReadyReadStdError()
{
    m_process->setReadChannel(QProcess::StandardError);
    while (m_process->canReadLine()) {
        QString line = QString::fromLocal8Bit(m_process->readLine()).trimmed();
        stdError(line);
    }
}

void AbstractProcessStep::stdError(const QString &line)
{
    emit addToOutputWindow(QLatin1String("<font color=\"#ff0000\">") + Qt::escape(line) + QLatin1String("</font>"));
}

void AbstractProcessStep::checkForCancel()
{
    if (m_futureInterface->isCanceled() && m_timer->isActive()) {
        m_timer->stop();
        m_process->terminate();
        m_process->waitForFinished(5000);
        m_process->kill();
    }
}

void AbstractProcessStep::slotProcessFinished(int, QProcess::ExitStatus)
{
    QString line = QString::fromLocal8Bit(m_process->readAllStandardError()).trimmed();
    if (!line.isEmpty())
        stdOut(line);

    line = QString::fromLocal8Bit(m_process->readAllStandardOutput()).trimmed();
    if (!line.isEmpty())
        stdError(line);

    m_eventLoop->exit(0);
}
