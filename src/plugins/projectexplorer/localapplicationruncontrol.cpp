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
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>

#include <utils/utilsicons.h>
#include <utils/qtcassert.h>

#include <QDir>

using namespace Utils;

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

private:
    void processStarted();
    void processExited(int exitCode, QProcess::ExitStatus status);

    ApplicationLauncher m_applicationLauncher;
    bool m_running = false;
};

LocalApplicationRunControl::LocalApplicationRunControl(RunConfiguration *rc, Core::Id mode)
    : RunControl(rc, mode)
{
    setRunnable(rc->runnable());
    setIcon(Utils::Icons::RUN_SMALL_TOOLBAR);
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
    QTC_ASSERT(runnable().is<StandardRunnable>(), return);
    auto r = runnable().as<StandardRunnable>();
    emit started();
    if (r.executable.isEmpty()) {
        appendMessage(tr("No executable specified.") + QLatin1Char('\n'), Utils::ErrorMessageFormat);
        emit finished();
    }  else if (!QFileInfo::exists(r.executable)) {
        appendMessage(tr("Executable %1 does not exist.")
                        .arg(QDir::toNativeSeparators(r.executable)) + QLatin1Char('\n'),
                      Utils::ErrorMessageFormat);
        emit finished();
    } else {
        m_running = true;
        QString msg = tr("Starting %1...").arg(QDir::toNativeSeparators(r.executable)) + QLatin1Char('\n');
        appendMessage(msg, Utils::NormalMessageFormat);
        m_applicationLauncher.start(r);
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
    QString exe = runnable().as<StandardRunnable>().executable;
    if (status == QProcess::CrashExit)
        msg = tr("%1 crashed.").arg(QDir::toNativeSeparators(exe));
    else
        msg = tr("%1 exited with code %2").arg(QDir::toNativeSeparators(exe)).arg(exitCode);
    appendMessage(msg + QLatin1Char('\n'), Utils::NormalMessageFormat);
    emit finished();
}

// LocalApplicationRunControlFactory

static bool isLocal(RunConfiguration *runConfiguration)
{
    Target *target = runConfiguration ? runConfiguration->target() : nullptr;
    Kit *kit = target ? target->kit() : nullptr;
    return DeviceTypeKitInformation::deviceTypeId(kit) == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
}

bool LocalApplicationRunControlFactory::canRun(RunConfiguration *runConfiguration, Core::Id mode) const
{
    if (mode != Constants::NORMAL_RUN_MODE)
        return false;
    const Runnable runnable = runConfiguration->runnable();
    if (!runnable.is<StandardRunnable>())
        return false;
    const IDevice::ConstPtr device = runnable.as<StandardRunnable>().device;
    if (device && device->type() == ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
        return true;
    return isLocal(runConfiguration);
}

RunControl *LocalApplicationRunControlFactory::create(RunConfiguration *runConfiguration, Core::Id mode, QString *errorMessage)
{
    Q_UNUSED(errorMessage)
    return new LocalApplicationRunControl(runConfiguration, mode);
}

} // namespace Internal
} // namespace ProjectExplorer

#include "localapplicationruncontrol.moc"
