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

#include <QDir>

namespace ProjectExplorer {
namespace Internal {

class LocalApplicationRunControl : public RunControl
{
    Q_OBJECT

public:
    LocalApplicationRunControl(RunConfiguration *runConfiguration, Core::Id mode);

    void start() override;
    StopResult stop() override;
    bool isRunning() const override;

    void setRunnable(const StandardRunnable &runnable) { m_runnable = runnable; }

private:
    void processStarted();
    void processExited(int exitCode, QProcess::ExitStatus status);

    ApplicationLauncher m_applicationLauncher;
    StandardRunnable m_runnable;
    bool m_running = false;
};

LocalApplicationRunControl::LocalApplicationRunControl(RunConfiguration *rc, Core::Id mode)
    : RunControl(rc, mode)
{
    setIcon(Icons::RUN_SMALL);
    connect(&m_applicationLauncher, &ApplicationLauncher::appendMessage,
            this, static_cast<void(RunControl::*)(const QString &, Utils::OutputFormat)>(&RunControl::appendMessage));
    connect(&m_applicationLauncher, &ApplicationLauncher::processStarted,
            this, &LocalApplicationRunControl::processStarted);
    connect(&m_applicationLauncher, &ApplicationLauncher::processExited,
            this, &LocalApplicationRunControl::processExited);
    connect(&m_applicationLauncher, &ApplicationLauncher::bringToForegroundRequested,
            this, &RunControl::bringApplicationToForeground);
}

void LocalApplicationRunControl::start()
{
    emit started();
    if (m_runnable.executable.isEmpty()) {
        appendMessage(tr("No executable specified.") + QLatin1Char('\n'), Utils::ErrorMessageFormat);
        emit finished();
    }  else if (!QFileInfo::exists(m_runnable.executable)) {
        appendMessage(tr("Executable %1 does not exist.")
                        .arg(QDir::toNativeSeparators(m_runnable.executable)) + QLatin1Char('\n'),
                      Utils::ErrorMessageFormat);
        emit finished();
    } else {
        m_running = true;
        QString msg = tr("Starting %1...").arg(QDir::toNativeSeparators(m_runnable.executable)) + QLatin1Char('\n');
        appendMessage(msg, Utils::NormalMessageFormat);
        m_applicationLauncher.setEnvironment(m_runnable.environment);
        m_applicationLauncher.start(m_runnable.runMode, m_runnable.executable, m_runnable.commandLineArguments);
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
                .arg(QDir::toNativeSeparators(m_runnable.executable));
    } else {
        msg = tr("%1 exited with code %2")
                .arg(QDir::toNativeSeparators(m_runnable.executable)).arg(exitCode);
    }
    appendMessage(msg + QLatin1Char('\n'), Utils::NormalMessageFormat);
    emit finished();
}

// LocalApplicationRunControlFactory

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
    auto runControl = new LocalApplicationRunControl(runConfiguration, mode);
    runControl->setRunnable(runConfiguration->runnable().as<StandardRunnable>());
    return runControl;
}

} // namespace Internal
} // namespace ProjectExplorer

#include "localapplicationruncontrol.moc"
