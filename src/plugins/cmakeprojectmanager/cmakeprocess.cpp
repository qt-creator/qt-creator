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

#include "cmakeprocess.h"

#include "cmakeparser.h"

#include <coreplugin/progressmanager/progressmanager.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskhub.h>

#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

using namespace Utils;

namespace CMakeProjectManager {
namespace Internal {

using namespace ProjectExplorer;

static QString stripTrailingNewline(QString str)
{
    if (str.endsWith('\n'))
        str.chop(1);
    return str;
}

CMakeProcess::CMakeProcess()
{
    connect(&m_cancelTimer, &QTimer::timeout, this, &CMakeProcess::checkForCancelled);
    m_cancelTimer.setInterval(500);
}

CMakeProcess::~CMakeProcess()
{
    m_process.reset();
    m_parser.flush();

    if (m_future) {
        reportCanceled();
        reportFinished();
    }
}

void CMakeProcess::run(const BuildDirParameters &parameters, const QStringList &arguments)
{
    QTC_ASSERT(!m_process && !m_future, return);

    CMakeTool *cmake = parameters.cmakeTool();
    QTC_ASSERT(parameters.isValid() && cmake, return);

    const FilePath cmakeExecutable = cmake->cmakeExecutable();

    const FilePath sourceDirectory = parameters.sourceDirectory.onDevice(cmakeExecutable);
    const FilePath buildDirectory = parameters.buildDirectory.onDevice(cmakeExecutable);

    if (!buildDirectory.exists()) {
        QString msg = tr("The build directory \"%1\" does not exist")
                .arg(buildDirectory.toUserOutput());
        BuildSystem::appendBuildSystemOutput(msg + '\n');
        emit finished();
        return;
    }

    if (buildDirectory.needsDevice()) {
        if (cmake->cmakeExecutable().host() != buildDirectory.host()) {
            QString msg = tr("CMake executable \"%1\" and build directory "
                    "\"%2\" must be on the same device.")
                .arg(cmake->cmakeExecutable().toUserOutput(), buildDirectory.toUserOutput());
            BuildSystem::appendBuildSystemOutput(msg + '\n');
            emit finished();
            return;
        }
    }

    const auto parser = new CMakeParser;
    parser->setSourceDirectory(parameters.sourceDirectory.path());
    m_parser.addLineParser(parser);

    // Always use the sourceDir: If we are triggered because the build directory is getting deleted
    // then we are racing against CMakeCache.txt also getting deleted.

    auto process = std::make_unique<QtcProcess>();
    m_processWasCanceled = false;

    m_cancelTimer.start();

    process->setWorkingDirectory(buildDirectory);
    process->setEnvironment(parameters.environment);

    process->setStdOutLineCallback([](const QString &s) {
        BuildSystem::appendBuildSystemOutput(stripTrailingNewline(s));
    });

    process->setStdErrLineCallback([this](const QString &s) {
        m_parser.appendMessage(s, StdErrFormat);
        BuildSystem::appendBuildSystemOutput(stripTrailingNewline(s));
    });

    connect(process.get(), &QtcProcess::finished,
            this, &CMakeProcess::handleProcessFinished);

    CommandLine commandLine(cmakeExecutable);
    commandLine.addArgs({"-S", sourceDirectory.path(), "-B", buildDirectory.path()});
    commandLine.addArgs(arguments);

    TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

    BuildSystem::startNewBuildSystemOutput(
        tr("Running %1 in %2.").arg(commandLine.toUserOutput()).arg(buildDirectory.toUserOutput()));

    auto future = std::make_unique<QFutureInterface<void>>();
    future->setProgressRange(0, 1);
    Core::ProgressManager::addTimedTask(*future.get(),
                                        tr("Configuring \"%1\"").arg(parameters.projectName),
                                        "CMake.Configure",
                                        10);

    process->setUseCtrlCStub(true);
    process->setCommand(commandLine);
    emit started();
    m_elapsed.start();
    process->start();

    m_process = std::move(process);
    m_future = std::move(future);
}

void CMakeProcess::terminate()
{
    if (m_process) {
        m_processWasCanceled = true;
        m_process->terminate();
    }
}

QProcess::ProcessState CMakeProcess::state() const
{
    if (m_process)
        return m_process->state();
    return QProcess::NotRunning;
}

void CMakeProcess::reportCanceled()
{
    QTC_ASSERT(m_future, return);
    m_future->reportCanceled();
}

void CMakeProcess::reportFinished()
{
    QTC_ASSERT(m_future, return);
    m_future->reportFinished();
    m_future.reset();
}

void CMakeProcess::setProgressValue(int p)
{
    QTC_ASSERT(m_future, return);
    m_future->setProgressValue(p);
}

void CMakeProcess::handleProcessFinished()
{
    QTC_ASSERT(m_process && m_future, return);

    m_cancelTimer.stop();

    const int code = m_process->exitCode();

    QString msg;
    if (m_process->exitStatus() != QProcess::NormalExit) {
        if (m_processWasCanceled) {
            msg = tr("CMake process was canceled by the user.");
        } else {
            msg = tr("CMake process crashed.");
        }
    } else if (code != 0) {
        msg = tr("CMake process exited with exit code %1.").arg(code);
    }
    m_lastExitCode = code;

    if (!msg.isEmpty()) {
        BuildSystem::appendBuildSystemOutput(msg + '\n');
        TaskHub::addTask(BuildSystemTask(Task::Error, msg));
        m_future->reportCanceled();
    } else {
        m_future->setProgressValue(1);
    }

    m_future->reportFinished();

    emit finished();

    const QString elapsedTime = Utils::formatElapsedTime(m_elapsed.elapsed());
    BuildSystem::appendBuildSystemOutput(elapsedTime + '\n');
}

void CMakeProcess::checkForCancelled()
{
    if (!m_process || !m_future)
        return;

    if (m_future->isCanceled()) {
        m_cancelTimer.stop();
        m_processWasCanceled = true;
        m_process->close();
    }
}

} // namespace Internal
} // namespace CMakeProjectManager
