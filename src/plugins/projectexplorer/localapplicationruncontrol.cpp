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

#include "localapplicationruncontrol.h"
#include "runnables.h"
#include "environmentaspect.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>

#include <utils/qtcassert.h>
#include <utils/environment.h>

#include <QDir>

namespace ProjectExplorer {
namespace Internal {

static bool isLocal(RunConfiguration *runConfiguration)
{
    Target *target = runConfiguration ? runConfiguration->target() : 0;
    Kit *kit = target ? target->kit() : 0;
    return DeviceTypeKitInformation::deviceTypeId(kit) == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
}

bool LocalApplicationRunControlFactory::canRun(RunConfiguration *runConfiguration, Core::Id mode) const
{
    return mode == Constants::NORMAL_RUN_MODE && isLocal(runConfiguration);
}

RunControl *LocalApplicationRunControlFactory::create(RunConfiguration *runConfiguration, Core::Id mode, QString *errorMessage)
{
    Q_UNUSED(errorMessage)
    QTC_ASSERT(runConfiguration->runnable().is<StandardRunnable>(), return 0);
    auto runnable = runConfiguration->runnable().as<StandardRunnable>();
    auto runControl = new LocalApplicationRunControl(runConfiguration, mode);
    runControl->setCommand(runnable.executable, runnable.commandLineArguments);
    runControl->setApplicationLauncherMode(runnable.runMode);
    runControl->setWorkingDirectory(runnable.workingDirectory);

    return runControl;
}

} // namespace Internal

// ApplicationRunControl

LocalApplicationRunControl::LocalApplicationRunControl(RunConfiguration *rc, Core::Id mode)
    : RunControl(rc, mode), m_runMode(ApplicationLauncher::Console), m_running(false)
{
    setIcon(Icons::RUN_SMALL);
    EnvironmentAspect *environment = rc->extraAspect<EnvironmentAspect>();
    Utils::Environment env;
    if (environment)
        env = environment->environment();
    m_applicationLauncher.setEnvironment(env);

    connect(&m_applicationLauncher, SIGNAL(appendMessage(QString,Utils::OutputFormat)),
            this, SLOT(slotAppendMessage(QString,Utils::OutputFormat)));
    connect(&m_applicationLauncher, SIGNAL(processStarted()),
            this, SLOT(processStarted()));
    connect(&m_applicationLauncher, SIGNAL(processExited(int,QProcess::ExitStatus)),
            this, SLOT(processExited(int,QProcess::ExitStatus)));
    connect(&m_applicationLauncher, SIGNAL(bringToForegroundRequested(qint64)),
            this, SLOT(bringApplicationToForeground(qint64)));
}

LocalApplicationRunControl::~LocalApplicationRunControl()
{
}

void LocalApplicationRunControl::start()
{
    emit started();
    if (m_executable.isEmpty()) {
        appendMessage(tr("No executable specified.") + QLatin1Char('\n'), Utils::ErrorMessageFormat);
        emit finished();
    }  else if (!QFileInfo::exists(m_executable)) {
        appendMessage(tr("Executable %1 does not exist.").arg(QDir::toNativeSeparators(m_executable)) + QLatin1Char('\n'),
                      Utils::ErrorMessageFormat);
        emit finished();
    } else {
        m_running = true;
        QString msg = tr("Starting %1...").arg(QDir::toNativeSeparators(m_executable)) + QLatin1Char('\n');
        appendMessage(msg, Utils::NormalMessageFormat);
        m_applicationLauncher.start(m_runMode, m_executable, m_commandLineArguments);
        setApplicationProcessHandle(ProcessHandle(m_applicationLauncher.applicationPID()));
    }
}

LocalApplicationRunControl::StopResult LocalApplicationRunControl::stop()
{
    m_applicationLauncher.stop();
    return StoppedSynchronously;
}

bool LocalApplicationRunControl::isRunning() const
{
    return m_running;
}

void LocalApplicationRunControl::setCommand(const QString &executable, const QString &commandLineArguments)
{
    m_executable = executable;
    m_commandLineArguments = commandLineArguments;
}

void LocalApplicationRunControl::setApplicationLauncherMode(const ApplicationLauncher::Mode mode)
{
    m_runMode = mode;
}

void LocalApplicationRunControl::setWorkingDirectory(const QString &workingDirectory)
{
    m_applicationLauncher.setWorkingDirectory(workingDirectory);
}

void LocalApplicationRunControl::slotAppendMessage(const QString &err,
                                                   Utils::OutputFormat format)
{
    appendMessage(err, format);
}

void LocalApplicationRunControl::processStarted()
{
    // Console processes only know their pid after being started
    setApplicationProcessHandle(ProcessHandle(m_applicationLauncher.applicationPID()));
}

void LocalApplicationRunControl::processExited(int exitCode, QProcess::ExitStatus status)
{
    m_running = false;
    setApplicationProcessHandle(ProcessHandle());
    QString msg;
    if (status == QProcess::CrashExit) {
        msg = tr("%1 crashed")
                .arg(QDir::toNativeSeparators(m_executable));
    } else {
        msg = tr("%1 exited with code %2")
                .arg(QDir::toNativeSeparators(m_executable)).arg(exitCode);
    }
    appendMessage(msg + QLatin1Char('\n'), Utils::NormalMessageFormat);
    emit finished();
}

} // namespace ProjectExplorer
