/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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

#include "mesonprocess.h"
#include "outputparsers/mesonoutputparser.h"

#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskhub.h>
#include <utils/stringutils.h>

#include <QLoggingCategory>

namespace MesonProjectManager {
namespace Internal {
static Q_LOGGING_CATEGORY(mesonProcessLog, "qtc.meson.buildsystem", QtDebugMsg);

MesonProcess::MesonProcess()
{
    connect(&m_cancelTimer, &QTimer::timeout, this, &MesonProcess::checkForCancelled);
    m_cancelTimer.setInterval(500);
}

bool MesonProcess::run(const Command &command,
                       const Utils::Environment env,
                       const QString &projectName,
                       bool captureStdo)
{
    if (!sanityCheck(command))
        return false;
    m_currentCommand = command;
    m_stdo.clear();
    m_processWasCanceled = false;
    m_future = decltype(m_future){};
    ProjectExplorer::TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);
    setupProcess(command, env, captureStdo);
    m_future.setProgressRange(0, 1);
    Core::ProgressManager::addTimedTask(m_future,
                                        tr("Configuring \"%1\".").arg(projectName),
                                        "Meson.Configure",
                                        10);
    emit started();
    m_elapsed.start();
    m_process->start();
    m_cancelTimer.start(500);
    qCDebug(mesonProcessLog()) << "Starting:" << command.toUserOutput();
    return true;
}

QProcess::ProcessState MesonProcess::state() const
{
    return m_process->state();
}

void MesonProcess::reportCanceled()
{
    m_future.reportCanceled();
}

void MesonProcess::reportFinished()
{
    m_future.reportFinished();
}

void MesonProcess::setProgressValue(int p)
{
    m_future.setProgressValue(p);
}

void MesonProcess::handleProcessFinished(int code, QProcess::ExitStatus status)
{
    m_cancelTimer.stop();
    m_stdo = m_process->readAllStandardOutput();
    m_stderr = m_process->readAllStandardError();
    if (status == QProcess::NormalExit) {
        m_future.setProgressValue(1);
        m_future.reportFinished();
    } else {
        m_future.reportCanceled();
        m_future.reportFinished();
    }
    const QString elapsedTime = Utils::formatElapsedTime(m_elapsed.elapsed());
    Core::MessageManager::write(elapsedTime);
    emit finished(code, status);
}

void MesonProcess::handleProcessError(QProcess::ProcessError error)
{
    QString message;
    QString commandStr = m_currentCommand.toUserOutput();
    switch (error) {
    case QProcess::ProcessError::FailedToStart:
        message = tr("The process failed to start.")
                  + tr("Either the "
                       "invoked program \"%1\" is missing, or you may have insufficient "
                       "permissions to invoke the program.")
                        .arg(m_currentCommand.executable().toUserOutput());
        break;
    case QProcess::ProcessError::Crashed:
        message = tr("The process was ended forcefully.");
        break;
    case QProcess::ProcessError::Timedout:
        message = tr("Process timed out.");
        break;
    case QProcess::ProcessError::WriteError:
        message = tr("An error occurred when attempting to write "
                     "to the process. For example, the process may not be running, "
                     "or it may have closed its input channel.");
        break;
    case QProcess::ProcessError::ReadError:
        message = tr("An error occurred when attempting to read from "
                     "the process. For example, the process may not be running.");
        break;
    case QProcess::ProcessError::UnknownError:
        message = tr("An unknown error in the process occurred.");
        break;
    }
    ProjectExplorer::TaskHub::addTask(
        ProjectExplorer::BuildSystemTask{ProjectExplorer::Task::TaskType::Error,
                                         QString("%1\n%2").arg(message).arg(commandStr)});
    handleProcessFinished(-1, QProcess::CrashExit);
}

void MesonProcess::checkForCancelled()
{
    if (m_future.isCanceled()) {
        m_cancelTimer.stop();
        m_processWasCanceled = true;
        m_process->close();
    }
}

void MesonProcess::setupProcess(const Command &command,
                                const Utils::Environment env,
                                bool captureStdo)
{
    if (m_process)
        disconnect(m_process.get());
    m_process = std::make_unique<Utils::QtcProcess>();
    connect(m_process.get(),
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            &MesonProcess::handleProcessFinished);
    connect(m_process.get(), &QProcess::errorOccurred, this, &MesonProcess::handleProcessError);
    if (!captureStdo) {
        connect(m_process.get(),
                &QProcess::readyReadStandardOutput,
                this,
                &MesonProcess::processStandardOutput);

        connect(m_process.get(),
                &QProcess::readyReadStandardError,
                this,
                &MesonProcess::processStandardError);
    }

    m_process->setWorkingDirectory(command.workDir().toString());
    m_process->setEnvironment(env);
    Core::MessageManager::write(
        tr("Running %1 in %2.").arg(command.toUserOutput()).arg(command.workDir().toUserOutput()));
    m_process->setCommand(command.cmdLine());
}

bool MesonProcess::sanityCheck(const Command &command) const
{
    const auto &exe = command.cmdLine().executable();
    if (!exe.exists()) {
        //Should only reach this point if Meson exe is removed while a Meson project is opened
        ProjectExplorer::TaskHub::addTask(
            ProjectExplorer::BuildSystemTask{ProjectExplorer::Task::TaskType::Error,
                                             tr("Executable does not exist: %1")
                                                 .arg(exe.toUserOutput())});
        return false;
    }
    if (!exe.toFileInfo().isExecutable()) {
        ProjectExplorer::TaskHub::addTask(
            ProjectExplorer::BuildSystemTask{ProjectExplorer::Task::TaskType::Error,
                                             tr("Command is not executable: %1")
                                                 .arg(exe.toUserOutput())});
        return false;
    }
    return true;
}

void MesonProcess::processStandardOutput()
{
    QTC_ASSERT(m_process, return );
    auto data = m_process->readAllStandardOutput();
    Core::MessageManager::write(QString::fromLocal8Bit(data));
    emit readyReadStandardOutput(data);
}

void MesonProcess::processStandardError()
{
    QTC_ASSERT(m_process, return );

    Core::MessageManager::write(QString::fromLocal8Bit(m_process->readAllStandardError()));
}
} // namespace Internal
} // namespace MesonProjectManager
