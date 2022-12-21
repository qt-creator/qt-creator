// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeprocess.h"

#include "builddirparameters.h"
#include "cmakeparser.h"
#include "cmakeprojectmanagertr.h"

#include <coreplugin/progressmanager/processprogress.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskhub.h>

#include <utils/processinterface.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace CMakeProjectManager::Internal {

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
}

static const int failedToStartExitCode = 0xFF; // See QtcProcessPrivate::handleDone() impl

void CMakeProcess::run(const BuildDirParameters &parameters, const QStringList &arguments)
{
    QTC_ASSERT(!m_process, return);

    CMakeTool *cmake = parameters.cmakeTool();
    QTC_ASSERT(parameters.isValid() && cmake, return);

    const FilePath cmakeExecutable = cmake->cmakeExecutable();

    if (!cmakeExecutable.ensureReachable(parameters.sourceDirectory)) {
        const QString msg = ::CMakeProjectManager::Tr::tr(
                "The source directory %1 is not reachable by the CMake executable %2.")
            .arg(parameters.sourceDirectory.displayName()).arg(cmakeExecutable.displayName());
        BuildSystem::appendBuildSystemOutput(msg + '\n');
        emit finished(failedToStartExitCode);
        return;
    }

    if (!cmakeExecutable.ensureReachable(parameters.buildDirectory)) {
        const QString msg = ::CMakeProjectManager::Tr::tr(
                "The build directory %1 is not reachable by the CMake executable %2.")
            .arg(parameters.buildDirectory.displayName()).arg(cmakeExecutable.displayName());
        BuildSystem::appendBuildSystemOutput(msg + '\n');
        emit finished(failedToStartExitCode);
        return;
    }

    const FilePath sourceDirectory = parameters.sourceDirectory.onDevice(cmakeExecutable);
    const FilePath buildDirectory = parameters.buildDirectory.onDevice(cmakeExecutable);

    if (!buildDirectory.exists()) {
        const QString msg = ::CMakeProjectManager::Tr::tr(
                "The build directory \"%1\" does not exist").arg(buildDirectory.toUserOutput());
        BuildSystem::appendBuildSystemOutput(msg + '\n');
        emit finished(failedToStartExitCode);
        return;
    }

    if (buildDirectory.needsDevice()) {
        if (cmake->cmakeExecutable().host() != buildDirectory.host()) {
            const QString msg = ::CMakeProjectManager::Tr::tr(
                  "CMake executable \"%1\" and build directory \"%2\" must be on the same device.")
                    .arg(cmake->cmakeExecutable().toUserOutput(), buildDirectory.toUserOutput());
            BuildSystem::appendBuildSystemOutput(msg + '\n');
            emit finished(failedToStartExitCode);
            return;
        }
    }

    const auto parser = new CMakeParser;
    parser->setSourceDirectory(parameters.sourceDirectory);
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

    BuildSystem::startNewBuildSystemOutput(::CMakeProjectManager::Tr::tr("Running %1 in %2.")
            .arg(commandLine.toUserOutput(), buildDirectory.toUserOutput()));

    ProcessProgress *progress = new ProcessProgress(m_process.get());
    progress->setDisplayName(::CMakeProjectManager::Tr::tr("Configuring \"%1\"")
                             .arg(parameters.projectName));
    m_process->setTimeoutS(10); // for process progress timeout estimation
    m_process->setCommand(commandLine);
    m_elapsed.start();
    m_process->start();
}

void CMakeProcess::stop()
{
    if (m_process)
        m_process->stop();
}

void CMakeProcess::handleProcessDone(const Utils::ProcessResultData &resultData)
{
    const int code = resultData.m_exitCode;
    QString msg;
    if (resultData.m_error == QProcess::FailedToStart) {
        msg = ::CMakeProjectManager::Tr::tr("CMake process failed to start.");
    } else if (resultData.m_exitStatus != QProcess::NormalExit) {
        if (resultData.m_canceledByUser)
            msg = ::CMakeProjectManager::Tr::tr("CMake process was canceled by the user.");
        else
            msg = ::CMakeProjectManager::Tr::tr("CMake process crashed.");
    } else if (code != 0) {
        msg = ::CMakeProjectManager::Tr::tr("CMake process exited with exit code %1.").arg(code);
    }

    if (!msg.isEmpty()) {
        BuildSystem::appendBuildSystemOutput(msg + '\n');
        TaskHub::addTask(BuildSystemTask(Task::Error, msg));
    }

    emit finished(code);

    const QString elapsedTime = Utils::formatElapsedTime(m_elapsed.elapsed());
    BuildSystem::appendBuildSystemOutput(elapsedTime + '\n');
}

} // CMakeProjectManager::Internal
