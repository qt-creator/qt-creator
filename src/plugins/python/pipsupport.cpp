/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "pipsupport.h"

#include "pythonproject.h"
#include "pythonrunconfiguration.h"
#include "pythonsettings.h"

#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/mimeutils.h>
#include <utils/qtcprocess.h>

using namespace Utils;

namespace Python {
namespace Internal {

static constexpr char pipInstallTaskId[] = "Python::pipInstallTask";

PipInstallTask::PipInstallTask(const Utils::FilePath &python)
    : m_python(python)
{
    m_watcher.setFuture(m_future.future());
}

void PipInstallTask::setPackage(const PipPackage &package)
{
    m_package = package;
}

void PipInstallTask::run()
{
    if (m_package.packageName.isEmpty()) {
        emit finished(false);
        return;
    }
    const QString taskTitle = tr("Install %1").arg(m_package.displayName);
    Core::ProgressManager::addTask(m_future.future(), taskTitle, pipInstallTaskId);
    connect(&m_process, &QtcProcess::finished, this, &PipInstallTask::installFinished);
    connect(&m_process, &QtcProcess::readyReadStandardError, this, &PipInstallTask::handleError);
    connect(&m_process, &QtcProcess::readyReadStandardOutput, this, &PipInstallTask::handleOutput);

    connect(&m_killTimer, &QTimer::timeout, this, &PipInstallTask::cancel);
    connect(&m_watcher, &QFutureWatcher<void>::canceled, this, &PipInstallTask::cancel);

    QString package = m_package.packageName;
    if (!m_package.version.isEmpty())
        package += "==" + m_package.version;
    QStringList arguments = {"-m", "pip", "install", package};

    // add --user to global pythons, but skip it for venv pythons
    if (!QDir(m_python.parentDir().toString()).exists("activate"))
        arguments << "--user";

    m_process.setCommand({m_python, arguments});
    m_process.start();

    Core::MessageManager::writeDisrupting(
        tr("Running \"%1\" to install %2.")
            .arg(m_process.commandLine().toUserOutput(), m_package.displayName));

    m_killTimer.setSingleShot(true);
    m_killTimer.start(5 /*minutes*/ * 60 * 1000);
}

void PipInstallTask::cancel()
{
    m_process.stopProcess();
    Core::MessageManager::writeFlashing(
        tr("The %1 installation was canceled by %2.")
            .arg(m_package.displayName, m_killTimer.isActive() ? tr("user") : tr("time out")));
}

void PipInstallTask::installFinished()
{
    m_future.reportFinished();
    const bool success = m_process.result() == ProcessResult::FinishedWithSuccess;
    if (!success) {
        Core::MessageManager::writeFlashing(
            tr("Installing the %1 failed with exit code %2")
                .arg(m_package.displayName, m_process.exitCode()));
    }
    emit finished(success);
}

void PipInstallTask::handleOutput()
{
    const QString &stdOut = QString::fromLocal8Bit(m_process.readAllStandardOutput().trimmed());
    if (!stdOut.isEmpty())
        Core::MessageManager::writeSilently(stdOut);
}

void PipInstallTask::handleError()
{
    const QString &stdErr = QString::fromLocal8Bit(m_process.readAllStandardError().trimmed());
    if (!stdErr.isEmpty())
        Core::MessageManager::writeSilently(stdErr);
}

} // namespace Internal
} // namespace Python
