// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtoolruncontrol.h"

#include "clangtool.h"
#include "clangtoolrunner.h"
#include "clangtoolssettings.h"
#include "clangtoolsutils.h"
#include "executableinfo.h"

#include <debugger/analyzer/analyzerconstants.h>

#include <clangcodemodel/clangutils.h>

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <cppeditor/clangdiagnosticconfigsmodel.h>
#include <cppeditor/compileroptionsbuilder.h>
#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/cppprojectfile.h>
#include <cppeditor/cpptoolsreuse.h>
#include <cppeditor/projectinfo.h>

#include <projectexplorer/abi.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>

#include <QAction>
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

AnalyzeUnit::AnalyzeUnit(const FileInfo &fileInfo,
                         const FilePath &clangIncludeDir,
                         const QString &clangVersion)
{
    const FilePath actualClangIncludeDir = Core::ICore::clangIncludeDirectory(
                clangVersion, clangIncludeDir);
    CompilerOptionsBuilder optionsBuilder(*fileInfo.projectPart,
                                          UseSystemHeader::No,
                                          UseTweakedHeaderPaths::Tools,
                                          UseLanguageDefines::No,
                                          UseBuildSystemWarnings::No,
                                          actualClangIncludeDir);
    file = fileInfo.file.toString();
    arguments = extraClangToolsPrependOptions();
    arguments.append(optionsBuilder.build(fileInfo.kind, CppEditor::getPchUsage()));
    arguments.append(extraClangToolsAppendOptions());
}

AnalyzeUnits ClangToolRunWorker::unitsToAnalyze(const FilePath &clangIncludeDir,
                                                const QString &clangVersion)
{
    QTC_ASSERT(m_projectInfo, return AnalyzeUnits());

    AnalyzeUnits units;
    for (const FileInfo &fileInfo : m_fileInfos)
        units << AnalyzeUnit(fileInfo, clangIncludeDir, clangVersion);
    return units;
}

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

QList<RunnerCreator> ClangToolRunWorker::runnerCreators(const AnalyzeUnit &unit)
{
    if (m_tool == ClangTidyTool::instance())
        return {[=] { return createRunner(ClangToolType::Tidy, unit); }};
    return {[=] { return createRunner(ClangToolType::Clazy, unit); }};
}

void ClangToolRunWorker::start()
{
    ProjectExplorerPlugin::saveModifiedFiles();

    if (m_projectBuilder && !m_projectBuilder->success()) {
        emit buildFailed();
        reportFailure(tr("Failed to build the project."));
        return;
    }

    const QString &toolName = m_tool->name();
    Project *project = runControl()->project();
    m_projectInfo = CppEditor::CppModelManager::instance()->projectInfo(project);
    if (!m_projectInfo) {
        reportFailure(tr("No code model data available for project."));
        return;
    }
    m_projectFiles = Utils::toSet(project->files(Project::AllFiles));

    // Project changed in the mean time?
    if (m_projectInfo->configurationOrFilesChanged(*m_projectInfoBeforeBuild)) {
        // If it's more than a release/debug build configuration change, e.g.
        // a version control checkout, files might be not valid C++ anymore
        // or even gone, so better stop here.
        reportFailure(tr("The project configuration changed since the start of "
                         "the %1. Please re-run with current configuration.")
                          .arg(toolName));
        emit startFailed();
        return;
    }

    // Create log dir
    if (!m_temporaryDir.isValid()) {
        reportFailure(
            tr("Failed to create temporary directory: %1.").arg(m_temporaryDir.errorString()));
        emit startFailed();
        return;
    }

    const Utils::FilePath projectFile = m_projectInfo->projectFilePath();
    appendMessage(tr("Running %1 on %2 with configuration \"%3\".")
                     .arg(toolName, projectFile.toUserOutput(), m_diagnosticConfig.displayName()),
                  Utils::NormalMessageFormat);

    // Collect files
    const auto clangIncludeDirAndVersion =
            getClangIncludeDirAndVersion(runControl()->commandLine().executable());
    const AnalyzeUnits unitsToProcess = unitsToAnalyze(clangIncludeDirAndVersion.first,
                                                       clangIncludeDirAndVersion.second);
    qCDebug(LOG) << Q_FUNC_INFO << runControl()->commandLine().executable()
                 << clangIncludeDirAndVersion.first << clangIncludeDirAndVersion.second;
    qCDebug(LOG) << "Files to process:" << unitsToProcess;

    m_runnerCreators.clear();
    for (const AnalyzeUnit &unit : unitsToProcess) {
        for (const RunnerCreator &creator : runnerCreators(unit))
            m_runnerCreators << creator;
    }
    m_initialQueueSize = m_runnerCreators.count();
    m_filesAnalyzed.clear();
    m_filesNotAnalyzed.clear();

    // Set up progress information
    using namespace Core;
    m_progress = QFutureInterface<void>();
    FutureProgress *futureProgress
        = ProgressManager::addTask(m_progress.future(), tr("Analyzing"),
                                   toolName.toStdString().c_str());
    connect(futureProgress, &FutureProgress::canceled,
            this, &ClangToolRunWorker::onProgressCanceled);
    m_progress.setProgressRange(0, m_initialQueueSize);
    m_progress.reportStarted();

    // Start process(es)
    qCDebug(LOG) << "Environment:" << m_environment;
    m_runners.clear();
    const int parallelRuns = m_runSettings.parallelJobs();
    QTC_ASSERT(parallelRuns >= 1, reportFailure(); return);

    if (m_runnerCreators.isEmpty()) {
        finalize();
        return;
    }

    reportStarted();
    m_elapsed.start();

    while (m_runners.size() < parallelRuns && !m_runnerCreators.isEmpty())
        analyzeNextFile();
}

void ClangToolRunWorker::stop()
{
    for (ClangToolRunner *runner : std::as_const(m_runners)) {
        QObject::disconnect(runner, nullptr, this, nullptr);
        delete runner;
    }
    m_projectFiles.clear();
    m_runners.clear();
    m_runnerCreators.clear();
    m_progress.reportFinished();

    reportStopped();

    // Print elapsed time since start
    const QString elapsedTime = Utils::formatElapsedTime(m_elapsed.elapsed());
    appendMessage(elapsedTime, NormalMessageFormat);
}

void ClangToolRunWorker::analyzeNextFile()
{
    if (m_progress.isFinished())
        return; // The previous call already reported that we are finished.

    if (m_runnerCreators.isEmpty()) {
        if (m_runners.isEmpty())
            finalize();
        return;
    }

    const RunnerCreator runnerCreator = m_runnerCreators.takeFirst();

    ClangToolRunner *runner = runnerCreator();
    m_runners.insert(runner);

    if (runner->run()) {
        const QString filePath = FilePath::fromString(runner->fileToAnalyze()).toUserOutput();
        appendMessage(tr("Analyzing \"%1\" [%2].").arg(filePath, runner->name()),
                      Utils::StdOutFormat);
    } else {
        reportFailure(tr("Failed to start runner \"%1\".").arg(runner->name()));
        stop();
    }
}

void ClangToolRunWorker::onRunnerFinishedWithSuccess(ClangToolRunner *runner,
                                                     const QString &filePath)
{
    const QString outputFilePath = runner->outputFilePath();
    qCDebug(LOG) << "onRunnerFinishedWithSuccess:" << outputFilePath;

    emit runnerFinished();

    QString errorMessage;
    const Diagnostics diagnostics = m_tool->read(outputFilePath, m_projectFiles, &errorMessage);

    if (!errorMessage.isEmpty()) {
        m_filesAnalyzed.remove(filePath);
        m_filesNotAnalyzed.insert(filePath);
        qCDebug(LOG) << "onRunnerFinishedWithSuccess: Error reading log file:" << errorMessage;
        const QString filePath = runner->fileToAnalyze();
        appendMessage(tr("Failed to analyze \"%1\": %2").arg(filePath, errorMessage),
                      Utils::StdErrFormat);
    } else {
        if (!m_filesNotAnalyzed.contains(filePath))
            m_filesAnalyzed.insert(filePath);
        if (!diagnostics.isEmpty()) {
            // do not generate marks when we always analyze open files since marks from that
            // analysis should be more up to date
            const bool generateMarks = !m_runSettings.analyzeOpenFiles();
            m_tool->onNewDiagnosticsAvailable(diagnostics, generateMarks);
        }
    }

    handleFinished(runner);
}

void ClangToolRunWorker::onRunnerFinishedWithFailure(ClangToolRunner *runner,
                                                     const QString &errorMessage,
                                                     const QString &errorDetails)
{
    qCDebug(LOG).noquote() << "onRunnerFinishedWithFailure:" << errorMessage
                           << '\n' << errorDetails;

    emit runnerFinished();

    const QString fileToAnalyze = runner->fileToAnalyze();

    m_filesAnalyzed.remove(fileToAnalyze);
    m_filesNotAnalyzed.insert(fileToAnalyze);

    const QString message = tr("Failed to analyze \"%1\": %2").arg(fileToAnalyze, errorMessage);
    appendMessage(message, Utils::StdErrFormat);
    appendMessage(errorDetails, Utils::StdErrFormat);
    handleFinished(runner);
}

void ClangToolRunWorker::handleFinished(ClangToolRunner *runner)
{
    m_runners.remove(runner);
    updateProgressValue();
    runner->deleteLater();
    analyzeNextFile();
}

void ClangToolRunWorker::onProgressCanceled()
{
    m_progress.reportCanceled();
    runControl()->initiateStop();
}

void ClangToolRunWorker::updateProgressValue()
{
    m_progress.setProgressValue(m_initialQueueSize - m_runnerCreators.size());
}

void ClangToolRunWorker::finalize()
{
    const QString toolName = m_tool->name();
    if (m_filesNotAnalyzed.size() != 0) {
        appendMessage(tr("Error: Failed to analyze %n files.", nullptr, m_filesNotAnalyzed.size()),
                      ErrorMessageFormat);
        Target *target = runControl()->target();
        if (target && target->activeBuildConfiguration() && !target->activeBuildConfiguration()->buildDirectory().exists()
            && !m_runSettings.buildBeforeAnalysis()) {
            appendMessage(
                tr("Note: You might need to build the project to generate or update source "
                   "files. To build automatically, enable \"Build the project before analysis\"."),
                NormalMessageFormat);
        }
    }

    appendMessage(tr("%1 finished: "
                     "Processed %2 files successfully, %3 failed.")
                      .arg(toolName)
                      .arg(m_filesAnalyzed.size())
                      .arg(m_filesNotAnalyzed.size()),
                  Utils::NormalMessageFormat);

    m_progress.reportFinished();
    runControl()->initiateStop();
}

ClangToolRunner *ClangToolRunWorker::createRunner(ClangToolType tool, const AnalyzeUnit &unit)
{
    using namespace std::placeholders;
    auto runner = new ClangToolRunner(
        {tool, m_diagnosticConfig, m_temporaryDir.path(), m_environment, unit}, this);
    connect(runner, &ClangToolRunner::finishedWithSuccess, this,
            std::bind(&ClangToolRunWorker::onRunnerFinishedWithSuccess, this, runner, _1));
    connect(runner, &ClangToolRunner::finishedWithFailure, this,
            std::bind(&ClangToolRunWorker::onRunnerFinishedWithFailure, this, runner, _1, _2));
    return runner;
}

} // namespace Internal
} // namespace ClangTools
