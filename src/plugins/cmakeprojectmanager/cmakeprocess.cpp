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

#include "builddirparameters.h"
#include "cmakeparser.h"

#include <coreplugin/progressmanager/progressmanager.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskhub.h>

#include <utils/processinterface.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

#include <QFutureWatcher>

using namespace Utils;

namespace CMakeProjectManager {
namespace Internal {

using namespace ProjectExplorer;
const int USER_STOP_EXIT_CODE = 15;

static QString stripTrailingNewline(QString str)
{
    if (str.endsWith('\n'))
        str.chop(1);
    return str;
}

CMakeProcess::CMakeProcess() = default;

CMakeProcess::~CMakeProcess()
{
    m_parser.flush();

    if (m_futureWatcher) {
        m_futureWatcher.reset();
        // None of the progress related functions will work after this!
        m_futureInterface.reportCanceled();
        m_futureInterface.reportFinished();
    }
}

void CMakeProcess::run(const BuildDirParameters &parameters, const QStringList &arguments)
{
    QTC_ASSERT(!m_process, return);

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

    m_process.reset(new QtcProcess);

    m_process->setWorkingDirectory(buildDirectory);
    m_process->setEnvironment(parameters.environment);

    m_process->setStdOutLineCallback([](const QString &s) {
        BuildSystem::appendBuildSystemOutput(stripTrailingNewline(s));
    });

    m_process->setStdErrLineCallback([this](const QString &s) {
        m_parser.appendMessage(s, StdErrFormat);
        BuildSystem::appendBuildSystemOutput(stripTrailingNewline(s));
    });

    connect(m_process.get(), &QtcProcess::done, this, [this] {
        handleProcessDone(m_process->resultData());
    });

    CommandLine commandLine(cmakeExecutable);
    commandLine.addArgs({"-S", sourceDirectory.path(), "-B", buildDirectory.path()});
    commandLine.addArgs(arguments);

    TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

    BuildSystem::startNewBuildSystemOutput(
        tr("Running %1 in %2.").arg(commandLine.toUserOutput(), buildDirectory.toUserOutput()));

    m_futureInterface = QFutureInterface<void>();
    m_futureInterface.setProgressRange(0, 1);
    Core::ProgressManager::addTimedTask(m_futureInterface,
                                        tr("Configuring \"%1\"").arg(parameters.projectName),
                                        "CMake.Configure",
                                        10);
    m_futureWatcher.reset(new QFutureWatcher<void>);
    connect(m_futureWatcher.get(), &QFutureWatcher<void>::canceled, this, &CMakeProcess::stop);
    m_futureWatcher->setFuture(m_futureInterface.future());

    m_process->setCommand(commandLine);
    emit started();
    m_elapsed.start();
    m_process->start();
}

void CMakeProcess::stop()
{
    if (!m_process)
        return;
    m_process->close();
    handleProcessDone({USER_STOP_EXIT_CODE, QProcess::CrashExit, QProcess::Crashed, {}});
}

void CMakeProcess::handleProcessDone(const Utils::ProcessResultData &resultData)
{
    if (m_futureWatcher) {
        m_futureWatcher->disconnect();
        m_futureWatcher.release()->deleteLater();
    }
    const int code = resultData.m_exitCode;

    QString msg;
    if (resultData.m_error == QProcess::FailedToStart) {
        msg = tr("CMake process failed to start.");
    } else if (resultData.m_exitStatus != QProcess::NormalExit) {
        if (m_futureInterface.isCanceled() || code == USER_STOP_EXIT_CODE)
            msg = tr("CMake process was canceled by the user.");
         else
            msg = tr("CMake process crashed.");
    } else if (code != 0) {
        msg = tr("CMake process exited with exit code %1.").arg(code);
    }
    m_lastExitCode = code;

    if (!msg.isEmpty()) {
        BuildSystem::appendBuildSystemOutput(msg + '\n');
        TaskHub::addTask(BuildSystemTask(Task::Error, msg));
        m_futureInterface.reportCanceled();
    } else {
        m_futureInterface.setProgressValue(1);
    }

    m_futureInterface.reportFinished();

    emit finished();

    const QString elapsedTime = Utils::formatElapsedTime(m_elapsed.elapsed());
    BuildSystem::appendBuildSystemOutput(elapsedTime + '\n');
}

} // namespace Internal
} // namespace CMakeProjectManager
