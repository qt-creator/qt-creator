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

#include "clangstaticanalyzerruncontrol.h"

#include "clangstaticanalyzerlogfilereader.h"
#include "clangstaticanalyzerrunner.h"
#include "clangstaticanalyzersettings.h"
#include "clangstaticanalyzertool.h"
#include "clangstaticanalyzerutils.h"

#include <debugger/analyzer/analyzerconstants.h>

#include <clangcodemodel/clangutils.h>

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <cpptools/compileroptionsbuilder.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppprojectfile.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/projectinfo.h>

#include <projectexplorer/abi.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/hostosinfo.h>
#include <utils/temporarydirectory.h>
#include <utils/qtcprocess.h>

#include <QAction>
#include <QLoggingCategory>

using namespace CppTools;
using namespace ProjectExplorer;
using namespace Utils;

static Q_LOGGING_CATEGORY(LOG, "qtc.clangstaticanalyzer.runcontrol")

static QStringList splitArgs(QString &argsString)
{
    QStringList result;
    Utils::QtcProcess::ArgIterator it(&argsString);
    while (it.next())
        result.append(it.value());
    return result;
}

template<size_t Size>
static QStringList extraOptions(const char(&environment)[Size])
{
    if (!qEnvironmentVariableIsSet(environment))
        return QStringList();
    QString arguments = QString::fromLocal8Bit(qgetenv(environment));
    return splitArgs(arguments);
}

static QStringList extraClangStaticAnalyzerPrependOptions() {
    constexpr char csaPrependOptions[] = "QTC_CLANG_CSA_CMD_PREPEND";
    static const QStringList options = extraOptions(csaPrependOptions);
    if (!options.isEmpty())
        qWarning() << "ClangStaticAnalyzer options are prepended with " << options.toVector();
    return options;
}

static QStringList extraClangStaticAnalyzerAppendOptions() {
    constexpr char csaAppendOptions[] = "QTC_CLANG_CSA_CMD_APPEND";
    static const QStringList options = extraOptions(csaAppendOptions);
    if (!options.isEmpty())
        qWarning() << "ClangStaticAnalyzer options are appended with " << options.toVector();
    return options;
}

namespace ClangTools {
namespace Internal {

class ProjectBuilder : public RunWorker
{
public:
    ProjectBuilder(RunControl *runControl, Project *project)
        : RunWorker(runControl), m_project(project)
    {
        setDisplayName("ProjectBuilder");
    }

    bool success() const { return m_success; }

private:
    void start() final
    {
        Target *target = m_project->activeTarget();
        QTC_ASSERT(target, reportFailure(); return);

        BuildConfiguration::BuildType buildType = BuildConfiguration::Unknown;
        if (const BuildConfiguration *buildConfig = target->activeBuildConfiguration())
            buildType = buildConfig->buildType();

        if (buildType == BuildConfiguration::Release) {
            const QString wrongMode = ClangStaticAnalyzerTool::tr("Release");
            const QString toolName = ClangStaticAnalyzerTool::tr("Clang Static Analyzer");
            const QString title = ClangStaticAnalyzerTool::tr("Run %1 in %2 Mode?").arg(toolName)
                    .arg(wrongMode);
            const QString message = ClangStaticAnalyzerTool::tr(
                        "<html><head/><body>"
                        "<p>You are trying to run the tool \"%1\" on an application in %2 mode. The tool is "
                        "designed to be used in Debug mode since enabled assertions can reduce the number of "
                        "false positives.</p>"
                        "<p>Do you want to continue and run the tool in %2 mode?</p>"
                        "</body></html>")
                    .arg(toolName).arg(wrongMode);
            if (CheckableMessageBox::doNotAskAgainQuestion(Core::ICore::mainWindow(),
                          title, message, Core::ICore::settings(),
                          "ClangStaticAnalyzerCorrectModeWarning") != QDialogButtonBox::Yes)
            {
                reportFailure();
                return;
            }
        }

        connect(BuildManager::instance(), &BuildManager::buildQueueFinished,
                this, &ProjectBuilder::onBuildFinished, Qt::QueuedConnection);

        ProjectExplorerPlugin::buildProject(m_project);
     }

     void onBuildFinished(bool success)
     {
         disconnect(BuildManager::instance(), &BuildManager::buildQueueFinished,
                    this, &ProjectBuilder::onBuildFinished);
         m_success = success;
         reportDone();
     }

private:
     QPointer<Project> m_project;
     bool m_success = false;
};

static AnalyzeUnits unitsToAnalyzeFromProjectParts(const QVector<ProjectPart::Ptr> projectParts,
                                                   const QString &clangVersion,
                                                   const QString &clangResourceDirectory)
{
    qCDebug(LOG) << "Taking arguments for analyzing from ProjectParts.";

    AnalyzeUnits unitsToAnalyze;

    foreach (const ProjectPart::Ptr &projectPart, projectParts) {
        if (!projectPart->selectedForBuilding || !projectPart.data())
            continue;

        foreach (const ProjectFile &file, projectPart->files) {
            if (file.path == CppModelManager::configurationFileName())
                continue;
            QTC_CHECK(file.kind != ProjectFile::Unclassified);
            QTC_CHECK(file.kind != ProjectFile::Unsupported);
            if (ProjectFile::isSource(file.kind)) {
                const CompilerOptionsBuilder::PchUsage pchUsage = CppTools::getPchUsage();
                CompilerOptionsBuilder optionsBuilder(*projectPart, clangVersion,
                                                      clangResourceDirectory);
                QStringList arguments = extraClangStaticAnalyzerPrependOptions();
                arguments.append(optionsBuilder.build(file.kind, pchUsage));
                arguments.append(extraClangStaticAnalyzerAppendOptions());
                unitsToAnalyze << AnalyzeUnit(file.path, arguments);
            }
        }
    }

    return unitsToAnalyze;
}

static QString clangResourceDir(const QString &clangExecutable, const QString &clangVersion)
{
    QDir llvmDir = QFileInfo(clangExecutable).dir();
    llvmDir.cdUp();
    return llvmDir.absolutePath() + clangIncludePath(clangVersion);
}

AnalyzeUnits ClangStaticAnalyzerToolRunner::sortedUnitsToAnalyze(const QString &clangVersion)
{
    QTC_ASSERT(m_projectInfo.isValid(), return AnalyzeUnits());

    const QString clangResourceDirectory = clangResourceDir(m_clangExecutable, clangVersion);
    AnalyzeUnits units = unitsToAnalyzeFromProjectParts(m_projectInfo.projectParts(), clangVersion,
                                                        clangResourceDirectory);

    Utils::sort(units, &AnalyzeUnit::file);
    return units;
}

static QDebug operator<<(QDebug debug, const Utils::Environment &environment)
{
    foreach (const QString &entry, environment.toStringList())
        debug << "\n  " << entry;
    return debug;
}

static QDebug operator<<(QDebug debug, const AnalyzeUnits &analyzeUnits)
{
    foreach (const AnalyzeUnit &unit, analyzeUnits)
        debug << "\n  " << unit.file;
    return debug;
}

ClangStaticAnalyzerToolRunner::ClangStaticAnalyzerToolRunner(RunControl *runControl, Target *target)
    : RunWorker(runControl), m_target(target)
{
    setDisplayName("ClangStaticAnalyzerRunner");
    setSupportsReRunning(false);

    m_projectBuilder = new ProjectBuilder(runControl, target->project());
    addStartDependency(m_projectBuilder);

    m_projectInfoBeforeBuild = CppTools::CppModelManager::instance()->projectInfo(target->project());

    BuildConfiguration *buildConfiguration = target->activeBuildConfiguration();
    QTC_ASSERT(buildConfiguration, return);
    m_environment = buildConfiguration->environment();

    ToolChain *toolChain = ToolChainKitInformation::toolChain(target->kit(), ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    QTC_ASSERT(toolChain, return);
    m_targetTriple = toolChain->originalTargetTriple();
    m_toolChainType = toolChain->typeId();
}

void ClangStaticAnalyzerToolRunner::start()
{
    m_success = m_projectBuilder->success();
    if (!m_success) {
        reportFailure();
        return;
    }

    m_projectInfo = CppTools::CppModelManager::instance()->projectInfo(m_target->project());

    // Some projects provides CompilerCallData once a build is finished,
    if (m_projectInfo.configurationOrFilesChanged(m_projectInfoBeforeBuild)) {
        // If it's more than a release/debug build configuration change, e.g.
        // a version control checkout, files might be not valid C++ anymore
        // or even gone, so better stop here.
        reportFailure(tr("The project configuration changed since the start of "
                         "the Clang Static Analyzer. Please re-run with current configuration."));
        return;
    }

    const Utils::FileName projectFile = m_projectInfo.project()->projectFilePath();
    appendMessage(tr("Running Clang Static Analyzer on %1").arg(projectFile.toUserOutput()),
                  Utils::NormalMessageFormat);

    // Check clang executable
    bool isValidClangExecutable;
    const QString executable = clangExecutableFromSettings(&isValidClangExecutable);
    if (!isValidClangExecutable) {
        const QString errorMessage = tr("Clang Static Analyzer: Invalid executable \"%1\", stop.")
                .arg(executable);
        appendMessage(errorMessage, Utils::ErrorMessageFormat);
        TaskHub::addTask(Task::Error, errorMessage, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
        reportFailure();
        return;
    }

    // Check clang version
    const ClangExecutableVersion version = clangExecutableVersion(executable);
    if (!version.isValid()) {
        const QString warningMessage
            = tr("Clang Static Analyzer: Running with possibly unsupported version, "
                 "could not determine version from executable \"%1\".")
                    .arg(executable);
        appendMessage(warningMessage, Utils::StdErrFormat);
        TaskHub::addTask(Task::Warning, warningMessage, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
    } else if (!version.isSupportedVersion()) {
        const QString warningMessage
            = tr("Clang Static Analyzer: Running with unsupported version %1, "
                 "supported version is %2.")
                    .arg(version.toString())
                    .arg(ClangExecutableVersion::supportedVersionAsString());
        appendMessage(warningMessage, Utils::StdErrFormat);
        TaskHub::addTask(Task::Warning, warningMessage, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
    }

    m_clangExecutable = executable;

    // Create log dir
    Utils::TemporaryDirectory temporaryDir("qtc-clangstaticanalyzer-XXXXXX");
    temporaryDir.setAutoRemove(false);
    if (!temporaryDir.isValid()) {
        const QString errorMessage
                = tr("Clang Static Analyzer: Failed to create temporary dir, stop.");
        appendMessage(errorMessage, Utils::ErrorMessageFormat);
        TaskHub::addTask(Task::Error, errorMessage, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
        reportFailure(errorMessage);
        return;
    }
    m_clangLogFileDir = temporaryDir.path();

    // Collect files
    const AnalyzeUnits unitsToProcess = sortedUnitsToAnalyze(version.toString());
    qCDebug(LOG) << "Files to process:" << unitsToProcess;
    m_unitsToProcess = unitsToProcess;
    m_initialFilesToProcessSize = m_unitsToProcess.count();
    m_filesAnalyzed = 0;
    m_filesNotAnalyzed = 0;

    // Set up progress information
    using namespace Core;
    m_progress = QFutureInterface<void>();
    FutureProgress *futureProgress
        = ProgressManager::addTask(m_progress.future(), tr("Analyzing"), "ClangStaticAnalyzer");
    futureProgress->setKeepOnFinish(FutureProgress::HideOnFinish);
    connect(futureProgress, &FutureProgress::canceled,
            this, &ClangStaticAnalyzerToolRunner::onProgressCanceled);
    m_progress.setProgressRange(0, m_initialFilesToProcessSize);
    m_progress.reportStarted();

    // Start process(es)
    qCDebug(LOG) << "Environment:" << m_environment;
    m_runners.clear();
    const int parallelRuns = ClangStaticAnalyzerSettings::instance()->simultaneousProcesses();
    QTC_ASSERT(parallelRuns >= 1, reportFailure(); return);
    m_success = true;

    if (m_unitsToProcess.isEmpty()) {
        finalize();
        return;
    }

    reportStarted();

    while (m_runners.size() < parallelRuns && !m_unitsToProcess.isEmpty())
        analyzeNextFile();
}

void ClangStaticAnalyzerToolRunner::stop()
{
    QSetIterator<ClangStaticAnalyzerRunner *> i(m_runners);
    while (i.hasNext()) {
        ClangStaticAnalyzerRunner *runner = i.next();
        QObject::disconnect(runner, 0, this, 0);
        delete runner;
    }
    m_runners.clear();
    m_unitsToProcess.clear();
    m_progress.reportFinished();
    //ClangStaticAnalyzerTool::instance()->onEngineFinished(m_success);
    reportStopped();
}

void ClangStaticAnalyzerToolRunner::analyzeNextFile()
{
    if (m_progress.isFinished())
        return; // The previous call already reported that we are finished.

    if (m_unitsToProcess.isEmpty()) {
        if (m_runners.isEmpty())
            finalize();
        return;
    }

    const AnalyzeUnit unit = m_unitsToProcess.takeFirst();
    qCDebug(LOG) << "analyzeNextFile:" << unit.file;

    ClangStaticAnalyzerRunner *runner = createRunner();
    m_runners.insert(runner);
    QTC_ASSERT(runner->run(unit.file, unit.arguments), return);

    appendMessage(tr("Analyzing \"%1\".").arg(
                      Utils::FileName::fromString(unit.file).toUserOutput()),
                  Utils::StdOutFormat);
}

ClangStaticAnalyzerRunner *ClangStaticAnalyzerToolRunner::createRunner()
{
    QTC_ASSERT(!m_clangExecutable.isEmpty(), return 0);
    QTC_ASSERT(!m_clangLogFileDir.isEmpty(), return 0);

    auto runner = new ClangStaticAnalyzerRunner(m_clangExecutable,
                                                m_clangLogFileDir,
                                                m_environment,
                                                this);
    connect(runner, &ClangStaticAnalyzerRunner::finishedWithSuccess,
            this, &ClangStaticAnalyzerToolRunner::onRunnerFinishedWithSuccess);
    connect(runner, &ClangStaticAnalyzerRunner::finishedWithFailure,
            this, &ClangStaticAnalyzerToolRunner::onRunnerFinishedWithFailure);
    return runner;
}

void ClangStaticAnalyzerToolRunner::onRunnerFinishedWithSuccess(const QString &logFilePath)
{
    qCDebug(LOG) << "onRunnerFinishedWithSuccess:" << logFilePath;

    QString errorMessage;
    const QList<Diagnostic> diagnostics = LogFileReader::read(logFilePath, &errorMessage);
    if (!errorMessage.isEmpty()) {
        qCDebug(LOG) << "onRunnerFinishedWithSuccess: Error reading log file:" << errorMessage;
        const QString filePath = qobject_cast<ClangStaticAnalyzerRunner *>(sender())->filePath();
        appendMessage(tr("Failed to analyze \"%1\": %2").arg(filePath, errorMessage),
                      Utils::StdErrFormat);
    } else {
        ++m_filesAnalyzed;
        if (!diagnostics.isEmpty())
            ClangStaticAnalyzerTool::instance()->onNewDiagnosticsAvailable(diagnostics);
    }

    handleFinished();
}

void ClangStaticAnalyzerToolRunner::onRunnerFinishedWithFailure(const QString &errorMessage,
                                                            const QString &errorDetails)
{
    qCDebug(LOG).noquote() << "onRunnerFinishedWithFailure:"
                           << errorMessage << '\n' << errorDetails;

    ++m_filesNotAnalyzed;
    m_success = false;
    const QString filePath = qobject_cast<ClangStaticAnalyzerRunner *>(sender())->filePath();
    appendMessage(tr("Failed to analyze \"%1\": %2").arg(filePath, errorMessage),
                  Utils::StdErrFormat);
    appendMessage(errorDetails, Utils::StdErrFormat);
    TaskHub::addTask(Task::Warning, errorMessage, Debugger::Constants::ANALYZERTASK_ID);
    TaskHub::addTask(Task::Warning, errorDetails, Debugger::Constants::ANALYZERTASK_ID);
    handleFinished();
}

void ClangStaticAnalyzerToolRunner::handleFinished()
{
    m_runners.remove(qobject_cast<ClangStaticAnalyzerRunner *>(sender()));
    updateProgressValue();
    sender()->deleteLater();
    analyzeNextFile();
}

void ClangStaticAnalyzerToolRunner::onProgressCanceled()
{
    m_progress.reportCanceled();
    runControl()->initiateStop();
}

void ClangStaticAnalyzerToolRunner::updateProgressValue()
{
    m_progress.setProgressValue(m_initialFilesToProcessSize - m_unitsToProcess.size());
}

void ClangStaticAnalyzerToolRunner::finalize()
{
    appendMessage(tr("Clang Static Analyzer finished: "
                     "Processed %1 files successfully, %2 failed.")
                        .arg(m_filesAnalyzed).arg(m_filesNotAnalyzed),
                  Utils::NormalMessageFormat);

    if (m_filesNotAnalyzed != 0) {
        QString msg = tr("Clang Static Analyzer: Not all files could be analyzed.");
        TaskHub::addTask(Task::Error, msg, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
    }

    m_progress.reportFinished();
    runControl()->initiateStop();
}

} // namespace Internal
} // namespace ClangTools
