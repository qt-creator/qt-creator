// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtoolruncontrol.h"

#include "clangtool.h"
#include "clangtoolrunner.h"
#include "clangtoolstr.h"
#include "executableinfo.h"

#include <coreplugin/progressmanager/taskprogress.h>

#include <cppeditor/cppmodelmanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <utils/algorithm.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/tasktree.h>

#include <QLoggingCategory>

using namespace CppEditor;
using namespace ProjectExplorer;
using namespace Utils;

static Q_LOGGING_CATEGORY(LOG, "qtc.clangtools.runcontrol", QtWarningMsg)

namespace ClangTools {
namespace Internal {

class ProjectBuilder : public RunWorker
{
public:
    ProjectBuilder(RunControl *runControl)
        : RunWorker(runControl)
    {
        setId("ProjectBuilder");
    }

    bool success() const { return m_success; }

private:
    void start() final
    {
        Target *target = runControl()->target();
        QTC_ASSERT(target, reportFailure(); return);

        connect(BuildManager::instance(), &BuildManager::buildQueueFinished,
                this, &ProjectBuilder::onBuildFinished, Qt::QueuedConnection);
        BuildManager::buildProjectWithDependencies(target->project());
     }

     void onBuildFinished(bool success)
     {
         disconnect(BuildManager::instance(), &BuildManager::buildQueueFinished,
                    this, &ProjectBuilder::onBuildFinished);
         m_success = success;
         reportDone();
     }

private:
     bool m_success = false;
};

static QDebug operator<<(QDebug debug, const Utils::Environment &environment)
{
    for (const QString &entry : environment.toStringList())
        debug << "\n  " << entry;
    return debug;
}

static QDebug operator<<(QDebug debug, const AnalyzeUnits &analyzeUnits)
{
    for (const AnalyzeUnit &unit : analyzeUnits)
        debug << "\n  " << unit.file;
    return debug;
}

ClangToolRunWorker::ClangToolRunWorker(ClangTool *tool, RunControl *runControl,
                                       const RunSettings &runSettings,
                                       const CppEditor::ClangDiagnosticConfig &diagnosticConfig,
                                       const FileInfos &fileInfos,
                                       bool buildBeforeAnalysis)
    : RunWorker(runControl)
    , m_tool(tool)
    , m_runSettings(runSettings)
    , m_diagnosticConfig(diagnosticConfig)
    , m_fileInfos(fileInfos)
    , m_temporaryDir("clangtools-XXXXXX")
{
    m_temporaryDir.setAutoRemove(qtcEnvironmentVariable("QTC_CLANG_DONT_DELETE_OUTPUT_FILES")
                                 != "1");
    setId("ClangTidyClazyRunner");
    setSupportsReRunning(false);

    if (buildBeforeAnalysis) {
        m_projectBuilder = new ProjectBuilder(runControl);
        addStartDependency(m_projectBuilder);
    }

    Target *target = runControl->target();
    m_projectInfoBeforeBuild = CppEditor::CppModelManager::instance()->projectInfo(target->project());

    BuildConfiguration *buildConfiguration = target->activeBuildConfiguration();
    QTC_ASSERT(buildConfiguration, return);
    m_environment = buildConfiguration->environment();

    ToolChain *toolChain = ToolChainKitAspect::cxxToolChain(target->kit());
    QTC_ASSERT(toolChain, return);
    m_targetTriple = toolChain->originalTargetTriple();
    m_toolChainType = toolChain->typeId();
}

ClangToolRunWorker::~ClangToolRunWorker() = default;

void ClangToolRunWorker::start()
{
    ProjectExplorerPlugin::saveModifiedFiles();

    if (m_projectBuilder && !m_projectBuilder->success()) {
        emit buildFailed();
        reportFailure(Tr::tr("Failed to build the project."));
        return;
    }

    const QString &toolName = m_tool->name();
    Project *project = runControl()->project();
    m_projectInfo = CppEditor::CppModelManager::instance()->projectInfo(project);
    if (!m_projectInfo) {
        reportFailure(Tr::tr("No code model data available for project."));
        return;
    }
    m_projectFiles = Utils::toSet(project->files(Project::AllFiles));

    // Project changed in the mean time?
    if (m_projectInfo->configurationOrFilesChanged(*m_projectInfoBeforeBuild)) {
        // If it's more than a release/debug build configuration change, e.g.
        // a version control checkout, files might be not valid C++ anymore
        // or even gone, so better stop here.
        reportFailure(Tr::tr("The project configuration changed since the start of "
                             "the %1. Please re-run with current configuration.")
                          .arg(toolName));
        emit startFailed();
        return;
    }

    // Create log dir
    if (!m_temporaryDir.isValid()) {
        reportFailure(
            Tr::tr("Failed to create temporary directory: %1.").arg(m_temporaryDir.errorString()));
        emit startFailed();
        return;
    }

    const Utils::FilePath projectFile = m_projectInfo->projectFilePath();
    appendMessage(Tr::tr("Running %1 on %2 with configuration \"%3\".")
                     .arg(toolName, projectFile.toUserOutput(), m_diagnosticConfig.displayName()),
                  Utils::NormalMessageFormat);

    // Collect files
    const auto [includeDir, clangVersion]
        = getClangIncludeDirAndVersion(runControl()->commandLine().executable());

    AnalyzeUnits unitsToProcess;
    for (const FileInfo &fileInfo : m_fileInfos)
        unitsToProcess.append({fileInfo, includeDir, clangVersion});

    qCDebug(LOG) << Q_FUNC_INFO << runControl()->commandLine().executable()
                 << includeDir << clangVersion;
    qCDebug(LOG) << "Files to process:" << unitsToProcess;
    qCDebug(LOG) << "Environment:" << m_environment;

    m_filesAnalyzed.clear();
    m_filesNotAnalyzed.clear();

    const ClangToolType tool = m_tool == ClangTidyTool::instance() ? ClangToolType::Tidy
                                                                   : ClangToolType::Clazy;
    using namespace Tasking;
    QList<TaskItem> tasks{ParallelLimit(qMax(1, m_runSettings.parallelJobs()))};
    for (const AnalyzeUnit &unit : std::as_const(unitsToProcess)) {
        const AnalyzeInputData input{tool, m_diagnosticConfig, m_temporaryDir.path(),
                                     m_environment, unit};
        const auto setupHandler = [this, unit, tool] {
            const QString filePath = unit.file.toUserOutput();
            appendMessage(Tr::tr("Analyzing \"%1\" [%2].").arg(filePath, clangToolName(tool)),
                          Utils::StdOutFormat);
            return true;
        };
        const auto outputHandler = [this](const AnalyzeOutputData &output) { onDone(output); };
        tasks.append(clangToolTask(input, setupHandler, outputHandler));
    }

    m_taskTree.reset(new TaskTree(tasks));
    connect(m_taskTree.get(), &TaskTree::done, this, &ClangToolRunWorker::finalize);
    connect(m_taskTree.get(), &TaskTree::errorOccurred, this, &ClangToolRunWorker::finalize);
    auto progress = new Core::TaskProgress(m_taskTree.get());
    progress->setDisplayName(Tr::tr("Analyzing"));
    reportStarted();
    m_elapsed.start();
    m_taskTree->start();
}

void ClangToolRunWorker::stop()
{
    m_taskTree.reset();
    m_projectFiles.clear();

    reportStopped();

    // Print elapsed time since start
    const QString elapsedTime = Utils::formatElapsedTime(m_elapsed.elapsed());
    appendMessage(elapsedTime, NormalMessageFormat);
}

void ClangToolRunWorker::onDone(const AnalyzeOutputData &output)
{
    emit runnerFinished();

    if (!output.success) {
        qCDebug(LOG).noquote() << "onRunnerFinishedWithFailure:" << output.errorMessage << '\n'
                               << output.errorDetails;
        m_filesAnalyzed.remove(output.fileToAnalyze);
        m_filesNotAnalyzed.insert(output.fileToAnalyze);

        const QString message = Tr::tr("Failed to analyze \"%1\": %2")
                                    .arg(output.fileToAnalyze.toUserOutput(), output.errorMessage);
        appendMessage(message, Utils::StdErrFormat);
        appendMessage(output.errorDetails, Utils::StdErrFormat);
        return;
    }

    qCDebug(LOG) << "onRunnerFinishedWithSuccess:" << output.outputFilePath;

    QString errorMessage;
    const Diagnostics diagnostics = m_tool->read(output.outputFilePath, m_projectFiles,
                                                 &errorMessage);

    if (!errorMessage.isEmpty()) {
        m_filesAnalyzed.remove(output.fileToAnalyze);
        m_filesNotAnalyzed.insert(output.fileToAnalyze);
        qCDebug(LOG) << "onRunnerFinishedWithSuccess: Error reading log file:" << errorMessage;
        appendMessage(Tr::tr("Failed to analyze \"%1\": %2")
                        .arg(output.fileToAnalyze.toUserOutput(), errorMessage),
                      Utils::StdErrFormat);
    } else {
        if (!m_filesNotAnalyzed.contains(output.fileToAnalyze))
            m_filesAnalyzed.insert(output.fileToAnalyze);
        if (!diagnostics.isEmpty()) {
            // do not generate marks when we always analyze open files since marks from that
            // analysis should be more up to date
            const bool generateMarks = !m_runSettings.analyzeOpenFiles();
            m_tool->onNewDiagnosticsAvailable(diagnostics, generateMarks);
        }
    }
}

void ClangToolRunWorker::finalize()
{
    if (m_taskTree)
        m_taskTree.release()->deleteLater();
    const QString toolName = m_tool->name();
    if (m_filesNotAnalyzed.size() != 0) {
        appendMessage(Tr::tr("Error: Failed to analyze %n files.", nullptr, m_filesNotAnalyzed.size()),
                      ErrorMessageFormat);
        Target *target = runControl()->target();
        if (target && target->activeBuildConfiguration() && !target->activeBuildConfiguration()->buildDirectory().exists()
            && !m_runSettings.buildBeforeAnalysis()) {
            appendMessage(
                Tr::tr("Note: You might need to build the project to generate or update source "
                   "files. To build automatically, enable \"Build the project before analysis\"."),
                NormalMessageFormat);
        }
    }

    appendMessage(Tr::tr("%1 finished: "
                         "Processed %2 files successfully, %3 failed.")
                      .arg(toolName)
                      .arg(m_filesAnalyzed.size())
                      .arg(m_filesNotAnalyzed.size()),
                  Utils::NormalMessageFormat);

    runControl()->initiateStop();
}

} // namespace Internal
} // namespace ClangTools
