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
                .arg(m_package.displayName).arg(m_process.exitCode()));
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

PipPackageInfo PipPackage::info(const Utils::FilePath &python) const
{
    PipPackageInfo result;

    QtcProcess pip;
    pip.setCommand(CommandLine(python, {"-m", "pip", "show", "-f", packageName}));
    pip.runBlocking();
    QString fieldName;
    QStringList data;
    const QString pipOutput = pip.allOutput();
    for (const QString &line : pipOutput.split('\n')) {
        if (line.isEmpty())
            continue;
        if (line.front().isSpace()) {
            data.append(line.trimmed());
        } else {
            result.parseField(fieldName, data);
            if (auto colonPos = line.indexOf(':'); colonPos >= 0) {
                fieldName = line.left(colonPos);
                data = QStringList(line.mid(colonPos + 1).trimmed());
            } else {
                fieldName.clear();
                data.clear();
            }
        }
    }
    result.parseField(fieldName, data);
    return result;
}

void PipPackageInfo::parseField(const QString &field, const QStringList &data)
{
    if (field.isEmpty())
        return;
    if (field == "Name") {
        name = data.value(0);
    } else if (field == "Version") {
        version = data.value(0);
    } else if (field == "Summary") {
        summary = data.value(0);
    } else if (field == "Home-page") {
        homePage = QUrl(data.value(0));
    } else if (field == "Author") {
        author = data.value(0);
    } else if (field == "Author-email") {
        authorEmail = data.value(0);
    } else if (field == "License") {
        license = data.value(0);
    } else if (field == "Location") {
        location = FilePath::fromUserInput(data.value(0)).normalizedPathName();
    } else if (field == "Requires") {
        requiresPackage = data.value(0).split(',', Qt::SkipEmptyParts);
    } else if (field == "Required-by") {
        requiredByPackage = data.value(0).split(',', Qt::SkipEmptyParts);
    } else if (field == "Files") {
        for (const QString &fileName : data) {
            if (!fileName.isEmpty())
                files.append(FilePath::fromUserInput(fileName.trimmed()));
        }
    }
}

} // namespace Internal
} // namespace Python
