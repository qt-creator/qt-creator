// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesonprocess.h"

#include "mesonprojectmanagertr.h"
#include "toolwrapper.h"

#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/processprogress.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskhub.h>

#include <utils/environment.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

#include <QLoggingCategory>

using namespace Core;
using namespace Utils;

namespace MesonProjectManager {
namespace Internal {

static Q_LOGGING_CATEGORY(mesonProcessLog, "qtc.meson.buildsystem", QtWarningMsg);

MesonProcess::MesonProcess() = default;
MesonProcess::~MesonProcess() = default;

bool MesonProcess::run(const Command &command,
                       const Environment &env,
                       const QString &projectName,
                       bool captureStdo)
{
    if (!sanityCheck(command))
        return false;
    m_stdo.clear();
    ProjectExplorer::TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);
    setupProcess(command, env, projectName, captureStdo);
    m_elapsed.start();
    m_process->start();
    qCDebug(mesonProcessLog()) << "Starting:" << command.toUserOutput();
    return true;
}

void MesonProcess::handleProcessDone()
{
    if (m_process->result() != ProcessResult::FinishedWithSuccess) {
        ProjectExplorer::TaskHub::addTask(ProjectExplorer::BuildSystemTask{
                ProjectExplorer::Task::TaskType::Error, m_process->exitMessage()});
    }
    m_stdo = m_process->readAllRawStandardOutput();
    m_stderr = m_process->readAllRawStandardError();
    const QString elapsedTime = formatElapsedTime(m_elapsed.elapsed());
    MessageManager::writeSilently(elapsedTime);
    emit finished(m_process->exitCode(), m_process->exitStatus());
}

void MesonProcess::setupProcess(const Command &command, const Environment &env,
                                const QString &projectName, bool captureStdo)
{
    if (m_process)
        m_process.release()->deleteLater();
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
    MessageManager::writeFlashing(Tr::tr("Running %1 in %2.")
                                  .arg(command.toUserOutput(), command.workDir().toUserOutput()));
    m_process->setCommand(command.cmdLine());
    m_process->setTimeoutS(10);
    ProcessProgress *progress = new ProcessProgress(m_process.get());
    progress->setDisplayName(Tr::tr("Configuring \"%1\".").arg(projectName));
}

bool MesonProcess::sanityCheck(const Command &command) const
{
    const auto &exe = command.cmdLine().executable();
    if (!exe.exists()) {
        //Should only reach this point if Meson exe is removed while a Meson project is opened
        ProjectExplorer::TaskHub::addTask(
            ProjectExplorer::BuildSystemTask{ProjectExplorer::Task::TaskType::Error,
                                             Tr::tr("Executable does not exist: %1")
                                                 .arg(exe.toUserOutput())});
        return false;
    }
    if (!exe.toFileInfo().isExecutable()) {
        ProjectExplorer::TaskHub::addTask(
            ProjectExplorer::BuildSystemTask{ProjectExplorer::Task::TaskType::Error,
                                             Tr::tr("Command is not executable: %1")
                                                    .arg(exe.toUserOutput())});
        return false;
    }
    return true;
}

void MesonProcess::processStandardOutput()
{
    const auto data = m_process->readAllRawStandardOutput();
    MessageManager::writeSilently(QString::fromLocal8Bit(data));
    emit readyReadStandardOutput(data);
}

void MesonProcess::processStandardError()
{
    MessageManager::writeSilently(QString::fromLocal8Bit(m_process->readAllRawStandardError()));
}

} // namespace Internal
} // namespace MesonProjectManager
