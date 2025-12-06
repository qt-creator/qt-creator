// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeprocess.h"

#include "builddirparameters.h"
#include "cmakeautogenparser.h"
#include "cmakeoutputparser.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"
#include "cmakespecificsettings.h"
#include "cmaketoolmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/outputwindow.h>
#include <coreplugin/progressmanager/processprogress.h>

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskhub.h>

#include <utils/algorithm.h>
#include <utils/macroexpander.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>

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

static const int failedToStartExitCode = 0xFF; // See ProcessPrivate::handleDone() impl

void CMakeProcess::run(const BuildDirParameters &parameters, const QStringList &arguments)
{
    QTC_ASSERT(parameters.isValid(), return);

    const FilePath cmakeExecutable = parameters.cmakeExecutable;

    const QString mountHint = ::CMakeProjectManager::Tr::tr(
        "You may need to add the project directory to the list of directories that are mounted by "
        "the build device.");

    if (!cmakeExecutable.ensureReachable(parameters.sourceDirectory)) {
        const QString msg = ::CMakeProjectManager::Tr::tr(
                "The source directory %1 is not reachable by the CMake executable %2.")
            .arg(parameters.sourceDirectory.displayName()).arg(cmakeExecutable.displayName());
        BuildSystem::appendBuildSystemOutput(addCMakePrefix({QString(), msg, mountHint}).join('\n'));
        emit finished(failedToStartExitCode);
        return;
    }

    if (!cmakeExecutable.ensureReachable(parameters.buildDirectory)) {
        const QString msg = ::CMakeProjectManager::Tr::tr(
                "The build directory %1 is not reachable by the CMake executable %2.")
            .arg(parameters.buildDirectory.displayName()).arg(cmakeExecutable.displayName());
        BuildSystem::appendBuildSystemOutput(addCMakePrefix({QString(), msg, mountHint}).join('\n'));
        emit finished(failedToStartExitCode);
        return;
    }

    const FilePath sourceDirectory = cmakeExecutable.withNewMappedPath(parameters.sourceDirectory);
    const FilePath buildDirectory = parameters.buildDirectory;

    if (!buildDirectory.exists()) {
        const QString msg = ::CMakeProjectManager::Tr::tr(
                "The build directory \"%1\" does not exist").arg(buildDirectory.toUserOutput());
        BuildSystem::appendBuildSystemOutput(addCMakePrefix({QString(), msg}).join('\n'));
        emit finished(failedToStartExitCode);
        return;
    }

    if (!buildDirectory.isLocal()) {
        if (!cmakeExecutable.isSameDevice(buildDirectory)) {
            const QString msg = ::CMakeProjectManager::Tr::tr(
                  "CMake executable \"%1\" and build directory \"%2\" must be on the same device.")
                    .arg(cmakeExecutable.toUserOutput(), buildDirectory.toUserOutput());
            BuildSystem::appendBuildSystemOutput(addCMakePrefix({QString(), msg}).join('\n'));
            emit finished(failedToStartExitCode);
            return;
        }
    }

    // Copy the "cmake-helper" CMake code from the ${IDE:ResourcePath} to the build directory
    const FilePath ideCMakeHelperDir = Core::ICore::resourcePath("cmake-helper");
    const FilePath localCMakeHelperDir = buildDirectory / Constants::PACKAGE_MANAGER_DIR;

    if (!ideCMakeHelperDir.isDir()) {
        BuildSystem::appendBuildSystemOutput(
            Tr::tr("Qt Creator installation is missing the "
                   "cmake-helper directory. It was expected here: \"%1\".")
                .arg(ideCMakeHelperDir.toUserOutput()));
    } else if (!localCMakeHelperDir.exists()) {
        const auto result = ideCMakeHelperDir.copyRecursively(localCMakeHelperDir);
        if (!result) {
            BuildSystem::appendBuildSystemOutput(
                addCMakePrefix(
                    {Tr::tr("Failed to copy cmake-helper folder:"), result.error()})
                    .join('\n'));
        }
    }

    const auto parser = new CMakeOutputParser;
    parser->setSourceDirectories({parameters.sourceDirectory, parameters.buildDirectory});
    m_parser.addLineParsers({new CMakeAutogenParser, parser});
    m_parser.addLineParsers(parameters.outputParsers());

    // Always use the sourceDir: If we are triggered because the build directory is getting deleted
    // then we are racing against CMakeCache.txt also getting deleted.

    m_process.setWorkingDirectory(buildDirectory);
    m_process.setEnvironment(parameters.environment);

    m_process.setStdOutLineCallback([this](const QString &s) {
        BuildSystem::appendBuildSystemOutput(addCMakePrefix(stripTrailingNewline(s)));
        emit stdOutReady(s);
    });

    m_process.setStdErrLineCallback([this](const QString &s) {
        m_parser.appendMessage(s, StdErrFormat);
        BuildSystem::appendBuildSystemOutput(addCMakePrefix(stripTrailingNewline(s)));
    });

    connect(&m_process, &Process::done, this, [this] {
        if (m_process.result() != ProcessResult::FinishedWithSuccess) {
            const QString message = m_process.exitMessage();
            BuildSystem::appendBuildSystemOutput(addCMakePrefix({{}, message}).join('\n'));
            TaskHub::addTask<CMakeTask>(Task::Error, message);
        }

        emit finished(m_process.exitCode());

        const QString elapsedTime = Utils::formatElapsedTime(m_elapsed.elapsed());
        BuildSystem::appendBuildSystemOutput(addCMakePrefix({{}, elapsedTime}).join('\n'));
    });

    CommandLine commandLine(cmakeExecutable,
        {"-S",
         CMakeToolManager::mappedFilePath(parameters.project, sourceDirectory).path(),
         "-B",
         CMakeToolManager::mappedFilePath(parameters.project, buildDirectory).path()});
    commandLine.addArgs(arguments);

    TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

    if (settings(parameters.project).cleanOldOutput())
    {
        ProjectExplorerPlugin::buildSystemOutput()->clearLinesPrefixedWith(Constants::OUTPUT_PREFIX, true);
        Core::MessageManager::clearLinesPrefixedWith(Constants::OUTPUT_PREFIX, false);
    }

    BuildSystem::startNewBuildSystemOutput(
        addCMakePrefix(::CMakeProjectManager::Tr::tr("Running %1 in %2.")
                           .arg(commandLine.toUserOutput(), buildDirectory.toUserOutput())));

    ProcessProgress *progress = new ProcessProgress(&m_process);
    progress->setDisplayName(::CMakeProjectManager::Tr::tr("Configuring \"%1\"")
                             .arg(parameters.projectName));
    m_process.setCommand(commandLine);
    m_elapsed.start();
    m_process.start();
}

void CMakeProcess::stop()
{
    m_process.stop();
}

QString addCMakePrefix(const QString &str)
{
    static const QString prefix
        = ansiColoredText(Constants::OUTPUT_PREFIX, creatorColor(Theme::Token_Text_Muted));
    return prefix + str;
}

QStringList addCMakePrefix(const QStringList &list)
{
    return Utils::transform(list, [](const QString &str) { return addCMakePrefix(str); });
}

} // CMakeProjectManager::Internal
