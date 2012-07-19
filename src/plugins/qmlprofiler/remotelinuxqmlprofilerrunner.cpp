/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "remotelinuxqmlprofilerrunner.h"
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <remotelinux/remotelinuxapplicationrunner.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

using namespace ExtensionSystem;
using namespace ProjectExplorer;
using namespace QmlProfiler::Internal;
using namespace RemoteLinux;

RemoteLinuxQmlProfilerRunner::RemoteLinuxQmlProfilerRunner(
    RemoteLinuxRunConfiguration *runConfiguration, QObject *parent)
    : AbstractQmlProfilerRunner(parent)
    , m_port(0)
    , m_runControl(0)
{
    // find run control factory
    IRunControlFactory *runControlFactory = 0;
    QList<IRunControlFactory*> runControlFactories
            = PluginManager::getObjects<IRunControlFactory>();

    foreach (IRunControlFactory *factory, runControlFactories) {
        if (factory->canRun(runConfiguration, NormalRunMode)) {
            runControlFactory = factory;
            break;
        }
    }

    QTC_ASSERT(runControlFactory, return);

    // create run control
    RunControl *runControl = runControlFactory->create(runConfiguration, NormalRunMode);

    m_runControl = qobject_cast<AbstractRemoteLinuxRunControl*>(runControl);
    QTC_ASSERT(m_runControl, return);

    connect(runner(), SIGNAL(readyForExecution()), this, SLOT(getPorts()));
    connect(runner(), SIGNAL(error(QString)), this, SLOT(handleError(QString)));
    connect(runner(), SIGNAL(remoteErrorOutput(QByteArray)), this, SLOT(handleStdErr(QByteArray)));
    connect(runner(), SIGNAL(remoteOutput(QByteArray)), this, SLOT(handleStdOut(QByteArray)));

    connect(runner(), SIGNAL(remoteProcessStarted()), this, SLOT(handleRemoteProcessStarted()));
    connect(runner(), SIGNAL(remoteProcessFinished(qint64)),
            this, SLOT(handleRemoteProcessFinished(qint64)));
    connect(runner(), SIGNAL(reportProgress(QString)), this, SLOT(handleProgressReport(QString)));
}

RemoteLinuxQmlProfilerRunner::~RemoteLinuxQmlProfilerRunner()
{
    delete m_runControl;
}

void RemoteLinuxQmlProfilerRunner::start()
{
    QTC_ASSERT(runner(), return);
    runner()->start();
}

void RemoteLinuxQmlProfilerRunner::stop()
{
    QTC_ASSERT(runner(), return);
    runner()->stop();
}

quint16 RemoteLinuxQmlProfilerRunner::debugPort() const
{
    return m_port;
}

void RemoteLinuxQmlProfilerRunner::getPorts()
{
    QTC_ASSERT(runner(), return);
    m_port = runner()->freePorts()->getNext();
    if (m_port == 0) {
        emit appendMessage(tr("Not enough free ports on device for analyzing.\n"),
                           Utils::ErrorMessageFormat);
        runner()->stop();
    } else {
        emit appendMessage(tr("Starting remote process ...\n"), Utils::NormalMessageFormat);

        QString arguments = runner()->arguments();
        if (!arguments.isEmpty())
            arguments.append(QLatin1Char(' '));
        arguments.append(QString::fromLatin1("-qmljsdebugger=port:%1,block").arg(m_port));

        runner()->startExecution(QString::fromLatin1("%1 %2 %3")
                                 .arg(runner()->commandPrefix())
                                 .arg(runner()->remoteExecutable())
                                 .arg(arguments).toUtf8());
    }
}

void RemoteLinuxQmlProfilerRunner::handleError(const QString &msg)
{
    emit appendMessage(msg + QLatin1Char('\n'), Utils::ErrorMessageFormat);
}

void RemoteLinuxQmlProfilerRunner::handleStdErr(const QByteArray &msg)
{
    emit appendMessage(QString::fromUtf8(msg), Utils::StdErrFormat);
}

void RemoteLinuxQmlProfilerRunner::handleStdOut(const QByteArray &msg)
{
    emit appendMessage(QString::fromUtf8(msg), Utils::StdOutFormat);
}

void RemoteLinuxQmlProfilerRunner::handleRemoteProcessStarted()
{
    emit started();
}

void RemoteLinuxQmlProfilerRunner::handleRemoteProcessFinished(qint64 exitCode)
{
    if (exitCode != AbstractRemoteLinuxApplicationRunner::InvalidExitCode) {
        appendMessage(tr("Finished running remote process. Exit code was %1.\n")
                      .arg(exitCode), Utils::NormalMessageFormat);
    }

    emit stopped();
}

void RemoteLinuxQmlProfilerRunner::handleProgressReport(const QString &progressString)
{
    appendMessage(progressString + QLatin1Char('\n'), Utils::NormalMessageFormat);
}

AbstractRemoteLinuxApplicationRunner *RemoteLinuxQmlProfilerRunner::runner() const
{
    if (!m_runControl)
        return 0;
    return m_runControl->runner();
}

