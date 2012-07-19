/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "remotelinuxruncontrol.h"

#include "remotelinuxapplicationrunner.h"
#include "remotelinuxrunconfiguration.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcassert.h>

#include <QString>
#include <QIcon>

using namespace ProjectExplorer;

namespace RemoteLinux {

using ProjectExplorer::RunConfiguration;

AbstractRemoteLinuxRunControl::AbstractRemoteLinuxRunControl(RunConfiguration *rc)
    : RunControl(rc, ProjectExplorer::NormalRunMode)
    , m_running(false)
{
}

AbstractRemoteLinuxRunControl::~AbstractRemoteLinuxRunControl()
{
}

void AbstractRemoteLinuxRunControl::start()
{
    m_running = true;
    emit started();
    disconnect(runner(), 0, this, 0);
    connect(runner(), SIGNAL(error(QString)), SLOT(handleSshError(QString)));
    connect(runner(), SIGNAL(readyForExecution()), SLOT(startExecution()));
    connect(runner(), SIGNAL(remoteErrorOutput(QByteArray)),
        SLOT(handleRemoteErrorOutput(QByteArray)));
    connect(runner(), SIGNAL(remoteOutput(QByteArray)),
        SLOT(handleRemoteOutput(QByteArray)));
    connect(runner(), SIGNAL(remoteProcessStarted()),
        SLOT(handleRemoteProcessStarted()));
    connect(runner(), SIGNAL(remoteProcessFinished(qint64)),
        SLOT(handleRemoteProcessFinished(qint64)));
    connect(runner(), SIGNAL(reportProgress(QString)),
        SLOT(handleProgressReport(QString)));
    runner()->start();
}

RunControl::StopResult AbstractRemoteLinuxRunControl::stop()
{
    runner()->stop();
    return AsynchronousStop;
}

void AbstractRemoteLinuxRunControl::handleSshError(const QString &error)
{
    handleError(error);
    setFinished();
}

void AbstractRemoteLinuxRunControl::startExecution()
{
    appendMessage(tr("Starting remote process...\n"), Utils::NormalMessageFormat);
    runner()->startExecution(QString::fromLatin1("%1 %2 %3")
        .arg(runner()->commandPrefix())
        .arg(runner()->remoteExecutable())
        .arg(runner()->arguments()).toUtf8());
}

void AbstractRemoteLinuxRunControl::handleRemoteProcessFinished(qint64 exitCode)
{
    if (exitCode != AbstractRemoteLinuxApplicationRunner::InvalidExitCode) {
        appendMessage(tr("Finished running remote process. Exit code was %1.\n")
            .arg(exitCode), Utils::NormalMessageFormat);
    }
    setFinished();
}

void AbstractRemoteLinuxRunControl::handleRemoteOutput(const QByteArray &output)
{
    appendMessage(QString::fromUtf8(output), Utils::StdOutFormatSameLine);
}

void AbstractRemoteLinuxRunControl::handleRemoteErrorOutput(const QByteArray &output)
{
    appendMessage(QString::fromUtf8(output), Utils::StdErrFormatSameLine);
}

void AbstractRemoteLinuxRunControl::handleProgressReport(const QString &progressString)
{
    appendMessage(progressString + QLatin1Char('\n'), Utils::NormalMessageFormat);
}

bool AbstractRemoteLinuxRunControl::isRunning() const
{
    return m_running;
}

QIcon AbstractRemoteLinuxRunControl::icon() const
{
    return QIcon(ProjectExplorer::Constants::ICON_RUN_SMALL);
}

void AbstractRemoteLinuxRunControl::handleError(const QString &errString)
{
    stop();
    appendMessage(errString, Utils::ErrorMessageFormat);
}

void AbstractRemoteLinuxRunControl::setFinished()
{
    disconnect(runner(), 0, this, 0);
    m_running = false;
    emit finished();
}


RemoteLinuxRunControl::RemoteLinuxRunControl(ProjectExplorer::RunConfiguration *runConfig)
    : AbstractRemoteLinuxRunControl(runConfig),
      m_runner(new GenericRemoteLinuxApplicationRunner(qobject_cast<RemoteLinuxRunConfiguration *>(runConfig), this))
{
}

RemoteLinuxRunControl::~RemoteLinuxRunControl()
{
}

AbstractRemoteLinuxApplicationRunner *RemoteLinuxRunControl::runner() const
{
    return m_runner;
}

} // namespace RemoteLinux
