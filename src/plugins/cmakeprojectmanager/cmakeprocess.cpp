// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeprocess.h"

#include "builddirparameters.h"
#include "cmakeparser.h"
#include "cmakeprojectconstants.h"
#include "cmakeprojectmanagertr.h"
#include "cmakespecificsettings.h"
#include "cmaketoolmanager.h"

#include <coreplugin/progressmanager/processprogress.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/kitchooser.h>

#include <extensionsystem/invoker.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/process.h>
#include <utils/processinfo.h>
#include <utils/processinterface.h>
#include <utils/stringutils.h>
#include <utils/stylehelper.h>
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
    QTC_ASSERT(!m_process, return);

    CMakeTool *cmake = parameters.cmakeTool();
    QTC_ASSERT(parameters.isValid() && cmake, return);

    const FilePath cmakeExecutable = cmake->cmakeExecutable();

    if (!cmakeExecutable.ensureReachable(parameters.sourceDirectory)) {
        const QString msg = ::CMakeProjectManager::Tr::tr(
                "The source directory %1 is not reachable by the CMake executable %2.")
            .arg(parameters.sourceDirectory.displayName()).arg(cmakeExecutable.displayName());
        BuildSystem::appendBuildSystemOutput(addCMakePrefix({QString(), msg}).join('\n'));
        emit finished(failedToStartExitCode);
        return;
    }

    if (!cmakeExecutable.ensureReachable(parameters.buildDirectory)) {
        const QString msg = ::CMakeProjectManager::Tr::tr(
                "The build directory %1 is not reachable by the CMake executable %2.")
            .arg(parameters.buildDirectory.displayName()).arg(cmakeExecutable.displayName());
        BuildSystem::appendBuildSystemOutput(addCMakePrefix({QString(), msg}).join('\n'));
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

    if (buildDirectory.needsDevice()) {
        if (!cmake->cmakeExecutable().isSameDevice(buildDirectory)) {
            const QString msg = ::CMakeProjectManager::Tr::tr(
                  "CMake executable \"%1\" and build directory \"%2\" must be on the same device.")
                    .arg(cmake->cmakeExecutable().toUserOutput(), buildDirectory.toUserOutput());
            BuildSystem::appendBuildSystemOutput(addCMakePrefix({QString(), msg}).join('\n'));
            emit finished(failedToStartExitCode);
            return;
        }
    }

    // Copy the "package-manager" CMake code from the ${IDE:ResourcePath} to the build directory
    if (settings().packageManagerAutoSetup()) {
        const FilePath localPackageManagerDir = buildDirectory.pathAppended(Constants::PACKAGE_MANAGER_DIR);
        const FilePath idePackageManagerDir = FilePath::fromString(
            parameters.expander->expand(QStringLiteral("%{IDE:ResourcePath}/package-manager")));

        if (!localPackageManagerDir.exists() && idePackageManagerDir.exists())
            idePackageManagerDir.copyRecursively(localPackageManagerDir);
    }

    const auto parser = new CMakeParser;
    parser->setSourceDirectory(parameters.sourceDirectory);
    m_parser.addLineParser(parser);
    m_parser.addLineParsers(parameters.outputParsers());

    // Always use the sourceDir: If we are triggered because the build directory is getting deleted
    // then we are racing against CMakeCache.txt also getting deleted.

    m_process.reset(new Process);

    m_process->setWorkingDirectory(buildDirectory);
    m_process->setEnvironment(parameters.environment);

    m_process->setStdOutLineCallback([this](const QString &s) {
        BuildSystem::appendBuildSystemOutput(addCMakePrefix(stripTrailingNewline(s)));
        emit stdOutReady(s);
    });

    m_process->setStdErrLineCallback([this](const QString &s) {
        m_parser.appendMessage(s, StdErrFormat);
        BuildSystem::appendBuildSystemOutput(addCMakePrefix(stripTrailingNewline(s)));
    });

    connect(m_process.get(), &Process::done, this, [this] {
        if (m_process->result() != ProcessResult::FinishedWithSuccess) {
            const QString message = m_process->exitMessage();
            BuildSystem::appendBuildSystemOutput(addCMakePrefix({{}, message}).join('\n'));
            TaskHub::addTask(BuildSystemTask(Task::Error, message));
        }

        emit finished(m_process->exitCode());

        const QString elapsedTime = Utils::formatElapsedTime(m_elapsed.elapsed());
        BuildSystem::appendBuildSystemOutput(addCMakePrefix({{}, elapsedTime}).join('\n'));
    });

    CommandLine commandLine(cmakeExecutable);
    commandLine.addArgs({"-S",
                         CMakeToolManager::mappedFilePath(sourceDirectory).path(),
                         "-B",
                         CMakeToolManager::mappedFilePath(buildDirectory).path()});
    commandLine.addArgs(arguments);

    TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);

    BuildSystem::startNewBuildSystemOutput(
        addCMakePrefix(::CMakeProjectManager::Tr::tr("Running %1 in %2.")
                           .arg(commandLine.toUserOutput(), buildDirectory.toUserOutput())));

    ProcessProgress *progress = new ProcessProgress(m_process.get());
    progress->setDisplayName(::CMakeProjectManager::Tr::tr("Configuring \"%1\"")
                             .arg(parameters.projectName));
    m_process->setCommand(commandLine);
    m_elapsed.start();
    m_process->start();
}

void CMakeProcess::stop()
{
    if (m_process)
        m_process->stop();
}

QString addCMakePrefix(const QString &str)
{
    auto qColorToAnsiCode = [] (const QColor &color) {
        return QString::fromLatin1("\033[38;2;%1;%2;%3m")
            .arg(color.red()).arg(color.green()).arg(color.blue());
    };
    static const QColor bgColor = creatorTheme()->color(Theme::BackgroundColorNormal);
    static const QColor fgColor = creatorTheme()->color(Theme::TextColorNormal);
    static const QColor grey = StyleHelper::mergedColors(fgColor, bgColor, 80);
    static const QString prefixString = qColorToAnsiCode(grey) + Constants::OUTPUT_PREFIX
                                        + qColorToAnsiCode(fgColor);
    return QString("%1%2").arg(prefixString, str);
}

QStringList addCMakePrefix(const QStringList &list)
{
    return Utils::transform(list, [](const QString &str) { return addCMakePrefix(str); });
}

} // CMakeProjectManager::Internal
