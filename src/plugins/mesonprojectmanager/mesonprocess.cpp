// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "mesonprocess.h"

#include "mesonoutputparser.h"

#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskhub.h>

#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

#include <QLoggingCategory>

using namespace Utils;

namespace MesonProjectManager {
namespace Internal {

static Q_LOGGING_CATEGORY(mesonProcessLog, "qtc.meson.buildsystem", QtWarningMsg);

MesonProcess::MesonProcess()
{
    connect(&m_cancelTimer, &QTimer::timeout, this, &MesonProcess::checkForCancelled);
    m_cancelTimer.setInterval(500);
}

bool MesonProcess::run(const Command &command,
                       const Environment &env,
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

void MesonProcess::handleProcessDone()
{
    if (m_process->result() != ProcessResult::FinishedWithSuccess) {
        ProjectExplorer::TaskHub::addTask(ProjectExplorer::BuildSystemTask{
                ProjectExplorer::Task::TaskType::Error, m_process->exitMessage()});
    }
    m_cancelTimer.stop();
    m_stdo = m_process->readAllStandardOutput();
    m_stderr = m_process->readAllStandardError();
    if (m_process->exitStatus() == QProcess::NormalExit) {
        m_future.setProgressValue(1);
        m_future.reportFinished();
    } else {
        m_future.reportCanceled();
        m_future.reportFinished();
    }
    const QString elapsedTime = formatElapsedTime(m_elapsed.elapsed());
    Core::MessageManager::writeSilently(elapsedTime);
    emit finished(m_process->exitCode(), m_process->exitStatus());
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
                                const Environment env,
                                bool captureStdo)
{
    m_process.reset(new QtcProcess);
    connect(m_process.get(), &QtcProcess::done, this, &MesonProcess::handleProcessDone);
    if (!captureStdo) {
        connect(m_process.get(), &QtcProcess::readyReadStandardOutput,
                this, &MesonProcess::processStandardOutput);
        connect(m_process.get(), &QtcProcess::readyReadStandardError,
                this, &MesonProcess::processStandardError);
    }

    m_process->setWorkingDirectory(command.workDir());
    m_process->setEnvironment(env);
    Core::MessageManager::writeFlashing(
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
    const auto data = m_process->readAllStandardOutput();
    Core::MessageManager::writeSilently(QString::fromLocal8Bit(data));
    emit readyReadStandardOutput(data);
}

void MesonProcess::processStandardError()
{
    Core::MessageManager::writeSilently(QString::fromLocal8Bit(m_process->readAllStandardError()));
}

} // namespace Internal
} // namespace MesonProjectManager
