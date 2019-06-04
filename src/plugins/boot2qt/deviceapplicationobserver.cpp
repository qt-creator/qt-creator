/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "deviceapplicationobserver.h"

#include "qdbutils.h"

#include <projectexplorer/runcontrol.h>

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace ProjectExplorer;

namespace Qdb {
namespace Internal {

DeviceApplicationObserver::DeviceApplicationObserver(QObject *parent)
    : QObject(parent), m_appRunner(new ApplicationLauncher(this))
{
    connect(m_appRunner, &ApplicationLauncher::remoteStdout, this,
            &DeviceApplicationObserver::handleStdout);
    connect(m_appRunner, &ApplicationLauncher::remoteStderr, this,
            &DeviceApplicationObserver::handleStderr);
    connect(m_appRunner, &ApplicationLauncher::reportError, this,
            &DeviceApplicationObserver::handleError);
    connect(m_appRunner, &ApplicationLauncher::finished, this,
            &DeviceApplicationObserver::handleFinished);
}

void DeviceApplicationObserver::start(const IDevice::ConstPtr &device,
        const QList<Command> &commands)
{
    QTC_ASSERT(device, return);
    m_device = device;
    m_commandsToRun = commands;
    runNext();
}

void DeviceApplicationObserver::runNext()
{
    if (m_commandsToRun.isEmpty()) {
        showMessage(tr("Commands on device '%1' finished successfully.")
                    .arg(m_device->displayName()));
        deleteLater();
        return;
    }
    const Command c = m_commandsToRun.takeFirst();
    m_stdout.clear();
    m_stderr.clear();

    Runnable r;
    r.executable = c.binary;
    r.commandLineArguments = Utils::QtcProcess::joinArgs(c.arguments);
    m_appRunner->start(r, m_device);
    showMessage(tr("Starting command '%1 %2' on device '%3'.")
                .arg(r.executable, r.commandLineArguments, m_device->displayName()));
}

void DeviceApplicationObserver::handleStdout(const QString &data)
{
    m_stdout += data;
}

void DeviceApplicationObserver::handleStderr(const QString &data)
{
    m_stderr += data;
}

void DeviceApplicationObserver::handleError(const QString &message)
{
    m_error = message;
}

void DeviceApplicationObserver::handleFinished(bool success)
{
    if (success && (m_stdout.contains("fail") || m_stdout.contains("error")
                    || m_stdout.contains("not found"))) {
        success = false; // adb does not forward exit codes and all stderr goes to stdout.
    }
    if (!success) {
        QString errorString;
        if (!m_error.isEmpty()) {
            errorString = tr("Command failed on device '%1': %2").arg(m_device->displayName(),
                                                                    m_error);
        } else {
            errorString = tr("Command failed on device '%1'.").arg(m_device->displayName());
        }
        showMessage(errorString, true);
        if (!m_stdout.isEmpty())
            showMessage(tr("stdout was: '%1'").arg(m_stdout));
        if (!m_stderr.isEmpty())
            showMessage(tr("stderr was: '%1'").arg(m_stderr));
        deleteLater();
        return;
    }
    runNext();
}

} // namespace Internal
} // namespace Qdb
