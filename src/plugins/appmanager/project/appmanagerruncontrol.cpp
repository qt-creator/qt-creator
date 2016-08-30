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

#include "appmanagerruncontrol.h"
#include "appmanagerrunconfiguration.h"

#include "../appmanagerconstants.h"

#include <projectexplorer/runnables.h>
#include <projectexplorer/target.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/devicesupport/deviceapplicationrunner.h>

#include <utils/qtcprocess.h>
#include <utils/outputformat.h>

#include <QDir>

using namespace ProjectExplorer;
using namespace Utils;

namespace AppManager {

AppManagerRunControl::AppManagerRunControl(AppManagerRunConfiguration *rc, Core::Id mode)
    : RunControl(rc, mode)
    , m_running(false)
    , m_runnable(rc->runnable().as<StandardRunnable>())
{}

void AppManagerRunControl::start()
{
    auto runConfig = runConfiguration();
    auto target = runConfig->target();
    auto kit = target->kit();
    auto dev = DeviceKitInformation::device(kit);

    if (m_launcher) {
        m_launcher->disconnect(this);
        m_launcher->deleteLater();
        m_launcher = nullptr;
    }

    if (dev && dev->canCreateProcess()) {
        auto process = new ProjectExplorer::DeviceApplicationRunner(this);
        m_launcher = process;

        connect(process, &ProjectExplorer::DeviceApplicationRunner::remoteProcessStarted,
                this, &RunControl::started);
        connect(process, &ProjectExplorer::DeviceApplicationRunner::finished, this, [this, process] (bool success) {
            if (success)
                processExited(0, QProcess::NormalExit);
            else
                processExited(1, QProcess::CrashExit);
        });
        connect(process, &ProjectExplorer::DeviceApplicationRunner::remoteStdout, this, [this, process] (const QByteArray &output) {
            appendMessage(QString::fromUtf8(output), Utils::StdOutFormat);
        });
        connect(process, &ProjectExplorer::DeviceApplicationRunner::remoteStderr, this, [this, process] (const QByteArray &output) {
            appendMessage(QString::fromUtf8(output), Utils::StdErrFormat);
        });
        connect(process, &ProjectExplorer::DeviceApplicationRunner::reportProgress, this, [this, process] (const QString &output) {
            appendMessage(output, Utils::NormalMessageFormat);
        });
        connect(process, &ProjectExplorer::DeviceApplicationRunner::reportError, this, [this, process] (const QString &output) {
            appendMessage(output, Utils::ErrorMessageFormat);
        });

        process->start(dev, m_runnable);
    } else {
        auto launcher = new ProjectExplorer::ApplicationLauncher(this);
        m_launcher = launcher;

        connect(launcher, &ApplicationLauncher::appendMessage,
                this, &AppManagerRunControl::slotAppendMessage);
        connect(launcher, &ApplicationLauncher::processStarted,
                this, &AppManagerRunControl::processStarted);
        connect(launcher, &ApplicationLauncher::processExited,
                this, &AppManagerRunControl::processExited);
        connect(launcher, &ApplicationLauncher::bringToForegroundRequested,
                this, &RunControl::bringApplicationToForeground);

        launcher->start(m_runnable);
    }

    m_running = true;
    emit started();
}

ProjectExplorer::RunControl::StopResult AppManagerRunControl::stop()
{
    if (!m_running)
        return StoppedSynchronously;

    if (auto device_process = qobject_cast<ProjectExplorer::DeviceApplicationRunner *>(m_launcher))
        device_process->stop();

    if (auto application_launcher = qobject_cast<ProjectExplorer::ApplicationLauncher *>(m_launcher))
        application_launcher->stop();

    return AsynchronousStop;
}

bool AppManagerRunControl::isRunning() const
{
    return m_running;
}

void AppManagerRunControl::processStarted()
{
    // Console processes only know their pid after being started
    if (auto application_launcher = qobject_cast<ProjectExplorer::ApplicationLauncher *>(m_launcher))
        setApplicationProcessHandle(ProcessHandle(quint64(application_launcher->applicationPID())));

    m_running = true;
    emit started();
}

void AppManagerRunControl::processExited(int exitCode, QProcess::ExitStatus status)
{
    setApplicationProcessHandle(ProcessHandle());
    QString msg;
    if (status == QProcess::CrashExit) {
        msg = tr("%1 crashed")
                .arg(QDir::toNativeSeparators(m_runnable.executable));
    } else {
        msg = tr("%1 exited with code %2")
                .arg(QDir::toNativeSeparators(m_runnable.executable)).arg(exitCode);
    }
    appendMessage(msg + QLatin1Char('\n'), NormalMessageFormat);
    m_running = false;
    emit finished();
}

void AppManagerRunControl::slotAppendMessage(const QString &err, OutputFormat format)
{
    appendMessage(err, format);
}

} // namespace AppManager
