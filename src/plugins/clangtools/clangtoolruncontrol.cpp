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

#include "clangtoolruncontrol.h"

#include "clangtool.h"
#include "clangtoolslogfilereader.h"
#include "clangtoolsprojectsettings.h"
#include "clangtoolssettings.h"
#include "clangtoolsutils.h"
#include "clangtoolrunner.h"

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
#include <utils/qtcprocess.h>

#include <QAction>
#include <QLoggingCategory>

using namespace CppTools;
using namespace ProjectExplorer;
using namespace Utils;

static Q_LOGGING_CATEGORY(LOG, "qtc.clangtools.runcontrol")

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

static QStringList extraClangToolsPrependOptions() {
    constexpr char csaPrependOptions[] = "QTC_CLANG_CSA_CMD_PREPEND";
    constexpr char toolsPrependOptions[] = "QTC_CLANG_TOOLS_CMD_PREPEND";
    static const QStringList options = extraOptions(csaPrependOptions)
            + extraOptions(toolsPrependOptions);
    if (!options.isEmpty())
        qWarning() << "ClangTools options are prepended with " << options.toVector();
    return options;
}

static QStringList extraClangToolsAppendOptions() {
    constexpr char csaAppendOptions[] = "QTC_CLANG_CSA_CMD_APPEND";
    constexpr char toolsAppendOptions[] = "QTC_CLANG_TOOLS_CMD_APPEND";
    static const QStringList options = extraOptions(csaAppendOptions)
            + extraOptions(toolsAppendOptions);
    if (!options.isEmpty())
        qWarning() << "ClangTools options are appended with " << options.toVector();
    return options;
}

namespace ClangTools {
namespace Internal {

class ProjectBuilder : public RunWorker
{
public:
    ProjectBuilder(RunControl *runControl, Project *project, ClangToolRunControl *parent)
        : RunWorker(runControl), m_project(project), m_parent(parent)
    {
        setDisplayName("ProjectBuilder");
    }

    void setEnabled(bool enabled) { m_enabled = enabled; }

    bool success() const { return m_success; }

private:
    void start() final
    {
        if (!m_enabled) {
            ProjectExplorerPlugin::saveModifiedFiles();
            onBuildFinished(true);
            return;
        }

        Target *target = m_project->activeTarget();
        QTC_ASSERT(target, reportFailure(); return);

        BuildConfiguration::BuildType buildType = BuildConfiguration::Unknown;
        if (const BuildConfiguration *buildConfig = target->activeBuildConfiguration())
            buildType = buildConfig->buildType();

        if (buildType == BuildConfiguration::Release) {
            const QString wrongMode = ClangToolRunControl::tr("Release");
            const QString toolName = m_parent->tool()->name();
            const QString title = ClangToolRunControl::tr("Run %1 in %2 Mode?").arg(toolName)
                    .arg(wrongMode);
            const QString message = ClangToolRunControl::tr(
                        "<html><head/><body>"
                        "<p>You are trying to run the tool \"%1\" on an application in %2 mode. The tool is "
                        "designed to be used in Debug mode since enabled assertions can reduce the number of "
                        "false positives.</p>"
                        "<p>Do you want to continue and run the tool in %2 mode?</p>"
                        "</body></html>")
                    .arg(toolName).arg(wrongMode);
            if (Utils::CheckableMessageBox::doNotAskAgainQuestion(Core::ICore::mainWindow(),
                                                                  title, message, Core::ICore::settings(),
                                                                  "ClangToolsCorrectModeWarning") != QDialogButtonBox::Yes)
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
     ClangToolRunControl *m_parent;
     bool m_enabled = true;
     bool m_success = false;
};

static AnalyzeUnits toAnalyzeUnits(const FileInfos &fileInfos,
                                   const QString &clangVersion,
                                   const QString &clangResourceDirectory)
{
    AnalyzeUnits unitsToAnalyze;
    const CompilerOptionsBuilder::PchUsage pchUsage = CppTools::getPchUsage();
    for (const FileInfo &fileInfo : fileInfos) {
        CompilerOptionsBuilder optionsBuilder(*fileInfo.projectPart,
                                              clangVersion,
                                              clangResourceDirectory);
        QStringList arguments = extraClangToolsPrependOptions();
        arguments.append(optionsBuilder.build(fileInfo.kind, pchUsage));
        arguments.append(extraClangToolsAppendOptions());
        unitsToAnalyze << AnalyzeUnit(fileInfo.file.toString(), arguments);
    }

    return unitsToAnalyze;
}

AnalyzeUnits ClangToolRunControl::unitsToAnalyze(const QString &clangVersion)
{
    QTC_ASSERT(m_projectInfo.isValid(), return AnalyzeUnits());

    const QString clangResourceDirectory = clangIncludeDirectory(m_clangExecutable, clangVersion);
    return toAnalyzeUnits(m_fileInfos, clangVersion, clangResourceDirectory);
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

ClangToolRunControl::ClangToolRunControl(RunControl *runControl,
                                         Target *target,
                                         const FileInfos &fileInfos)
    : RunWorker(runControl)
    , m_projectBuilder(new ProjectBuilder(runControl, target->project(), this))
    , m_clangExecutable(CppTools::clangExecutable(CLANG_BINDIR))
    , m_temporaryDir("clangtools-XXXXXX")
    , m_target(target)
    , m_fileInfos(fileInfos)
{
    addStartDependency(m_projectBuilder);

    ClangToolsProjectSettings *projectSettings = ClangToolsProjectSettingsManager::getSettings(
        target->project());
    if (projectSettings->useGlobalSettings())
        m_projectBuilder->setEnabled(ClangToolsSettings::instance()->savedBuildBeforeAnalysis());
    else
        m_projectBuilder->setEnabled(projectSettings->buildBeforeAnalysis());
}

void ClangToolRunControl::init()
{
    setSupportsReRunning(false);
    m_projectInfoBeforeBuild = CppTools::CppModelManager::instance()->projectInfo(
                m_target->project());

    BuildConfiguration *buildConfiguration = m_target->activeBuildConfiguration();
    QTC_ASSERT(buildConfiguration, return);
    m_environment = buildConfiguration->environment();

    ToolChain *toolChain = ToolChainKitInformation::toolChain(m_target->kit(),
                                                              ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    QTC_ASSERT(toolChain, return);
    m_targetTriple = toolChain->originalTargetTriple();
    m_toolChainType = toolChain->typeId();
}

void ClangToolRunControl::start()
{
    if (ClangToolsSettings::instance()->savedBuildBeforeAnalysis()) {
        QTC_ASSERT(m_projectBuilder, return;);
        if (!m_projectBuilder->success()) {
            reportFailure();
            return;
        }
    }

    const QString &toolName = tool()->name();
    if (m_clangExecutable.isEmpty()) {
        const QString errorMessage = tr("%1: Can't find clang executable, stop.").arg(toolName);
        appendMessage(errorMessage, Utils::ErrorMessageFormat);
        TaskHub::addTask(Task::Error, errorMessage, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
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
                         "the %1. Please re-run with current configuration.").arg(toolName));
        return;
    }

    const Utils::FileName projectFile = m_projectInfo.project()->projectFilePath();
    appendMessage(tr("Running %1 on %2").arg(toolName).arg(projectFile.toUserOutput()),
                  Utils::NormalMessageFormat);

    // Create log dir
    if (!m_temporaryDir.isValid()) {
        const QString errorMessage
                = toolName + tr(": Failed to create temporary dir, stop.");
        appendMessage(errorMessage, Utils::ErrorMessageFormat);
        TaskHub::addTask(Task::Error, errorMessage, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
        reportFailure(errorMessage);
        return;
    }

    // Collect files
    const AnalyzeUnits unitsToProcess = unitsToAnalyze(CLANG_VERSION);
    qCDebug(LOG) << "Files to process:" << unitsToProcess;
    m_unitsToProcess = unitsToProcess;
    m_initialFilesToProcessSize = m_unitsToProcess.count();
    m_filesAnalyzed = 0;
    m_filesNotAnalyzed = 0;

    // Set up progress information
    using namespace Core;
    m_progress = QFutureInterface<void>();
    FutureProgress *futureProgress
        = ProgressManager::addTask(m_progress.future(), tr("Analyzing"),
                                   toolName.toStdString().c_str());
    futureProgress->setKeepOnFinish(FutureProgress::HideOnFinish);
    connect(futureProgress, &FutureProgress::canceled,
            this, &ClangToolRunControl::onProgressCanceled);
    m_progress.setProgressRange(0, m_initialFilesToProcessSize);
    m_progress.reportStarted();

    // Start process(es)
    qCDebug(LOG) << "Environment:" << m_environment;
    m_runners.clear();
    const int parallelRuns = ClangToolsSettings::instance()->savedSimultaneousProcesses();
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

void ClangToolRunControl::stop()
{
    QSetIterator<ClangToolRunner *> i(m_runners);
    while (i.hasNext()) {
        ClangToolRunner *runner = i.next();
        QObject::disconnect(runner, 0, this, 0);
        delete runner;
    }
    m_runners.clear();
    m_unitsToProcess.clear();
    m_progress.reportFinished();

    reportStopped();
}

void ClangToolRunControl::analyzeNextFile()
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

    ClangToolRunner *runner = createRunner();
    m_runners.insert(runner);
    QTC_ASSERT(runner->run(unit.file, unit.arguments), return);

    appendMessage(tr("Analyzing \"%1\".").arg(
                      Utils::FileName::fromString(unit.file).toUserOutput()),
                  Utils::StdOutFormat);
}

void ClangToolRunControl::onRunnerFinishedWithSuccess(const QString &filePath)
{
    const QString logFilePath = qobject_cast<ClangToolRunner *>(sender())->logFilePath();
    qCDebug(LOG) << "onRunnerFinishedWithSuccess:" << logFilePath;

    QString errorMessage;
    const QList<Diagnostic> diagnostics = tool()->read(filePath, logFilePath, &errorMessage);
    QFile::remove(logFilePath); // Clean-up.

    if (!errorMessage.isEmpty()) {
        qCDebug(LOG) << "onRunnerFinishedWithSuccess: Error reading log file:" << errorMessage;
        const QString filePath = qobject_cast<ClangToolRunner *>(sender())->filePath();
        appendMessage(tr("Failed to analyze \"%1\": %2").arg(filePath, errorMessage),
                      Utils::StdErrFormat);
    } else {
        ++m_filesAnalyzed;
        if (!diagnostics.isEmpty())
            tool()->onNewDiagnosticsAvailable(diagnostics);
    }

    handleFinished();
}

void ClangToolRunControl::onRunnerFinishedWithFailure(const QString &errorMessage,
                                                            const QString &errorDetails)
{
    qCDebug(LOG).noquote() << "onRunnerFinishedWithFailure:"
                           << errorMessage << '\n' << errorDetails;

    // Even in the error case the log file was created, so clean it up here, too.
    QFile::remove(qobject_cast<ClangToolRunner *>(sender())->logFilePath());

    ++m_filesNotAnalyzed;
    m_success = false;
    const QString filePath = qobject_cast<ClangToolRunner *>(sender())->filePath();
    appendMessage(tr("Failed to analyze \"%1\": %2").arg(filePath, errorMessage),
                  Utils::StdErrFormat);
    appendMessage(errorDetails, Utils::StdErrFormat);
    TaskHub::addTask(Task::Warning, errorMessage, Debugger::Constants::ANALYZERTASK_ID);
    TaskHub::addTask(Task::Warning, errorDetails, Debugger::Constants::ANALYZERTASK_ID);
    handleFinished();
}

void ClangToolRunControl::handleFinished()
{
    m_runners.remove(qobject_cast<ClangToolRunner *>(sender()));
    updateProgressValue();
    sender()->deleteLater();
    analyzeNextFile();
}

void ClangToolRunControl::onProgressCanceled()
{
    m_progress.reportCanceled();
    runControl()->initiateStop();
}

void ClangToolRunControl::updateProgressValue()
{
    m_progress.setProgressValue(m_initialFilesToProcessSize - m_unitsToProcess.size());
}

void ClangToolRunControl::finalize()
{
    const QString toolName = tool()->name();
    appendMessage(toolName + tr(" finished: "
                     "Processed %1 files successfully, %2 failed.")
                        .arg(m_filesAnalyzed).arg(m_filesNotAnalyzed),
                  Utils::NormalMessageFormat);

    if (m_filesNotAnalyzed != 0) {
        QString msg = toolName + tr(": Not all files could be analyzed.");
        TaskHub::addTask(Task::Error, msg, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
    }

    m_progress.reportFinished();
    runControl()->initiateStop();
}

} // namespace Internal
} // namespace ClangTools
