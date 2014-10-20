/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Enterprise LicenseChecker Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "clangstaticanalyzerruncontrol.h"

#include "clangstaticanalyzerlogfilereader.h"
#include "clangstaticanalyzerrunner.h"
#include "clangstaticanalyzersettings.h"
#include "clangstaticanalyzerutils.h"

#include <analyzerbase/analyzermanager.h>

#include <clangcodemodel/clangutils.h>

#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppprojects.h>
#include <cpptools/cppprojectfile.h>

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>

#include <QLoggingCategory>
#include <QTemporaryDir>

using namespace CppTools;
using namespace ProjectExplorer;

static Q_LOGGING_CATEGORY(LOG, "qtc.clangstaticanalyzer.runcontrol")

namespace ClangStaticAnalyzer {
namespace Internal {

ClangStaticAnalyzerRunControl::ClangStaticAnalyzerRunControl(
        const Analyzer::AnalyzerStartParameters &startParams,
        ProjectExplorer::RunConfiguration *runConfiguration)
    : AnalyzerRunControl(startParams, runConfiguration)
    , m_initialFilesToProcessSize(0)
    , m_runningProcesses(0)
{
}

static QList<ClangStaticAnalyzerRunControl::FileConfiguration> calculateFilesToProcess(
            Project *project)
{
    typedef ClangStaticAnalyzerRunControl::FileConfiguration ProjectFileConfiguration;
    QTC_ASSERT(project, return QList<ProjectFileConfiguration>());
    ProjectInfo projectInfo = CppModelManager::instance()->projectInfo(project);
    QTC_ASSERT(projectInfo, return QList<ProjectFileConfiguration>());

    QList<ProjectFileConfiguration> files;
    const QList<ProjectPart::Ptr> projectParts = projectInfo.projectParts();
    foreach (const ProjectPart::Ptr &projectPart, projectParts) {
        foreach (const ProjectFile &file, projectPart->files) {
            if (file.path == CppModelManager::configurationFileName())
                continue;
            QTC_CHECK(file.kind != ProjectFile::Unclassified);
            if (ProjectFile::isSource(file.kind))
                files << ProjectFileConfiguration(file.path, projectPart);
        }
    }

    return files;
}

bool ClangStaticAnalyzerRunControl::startEngine()
{
    emit starting(this);

    Project *currentProject = ProjectExplorerPlugin::currentProject();
    QTC_ASSERT(currentProject, emit finished(); return false);

    // Check clang executable
    bool isValidClangExecutable;
    const QString executable = clangExecutableFromSettings(&isValidClangExecutable);
    if (!isValidClangExecutable) {
        emit appendMessage(tr("Clang Static Analyzer: Invalid executable \"%1\", stop.\n")
                            .arg(executable),
                           Utils::ErrorMessageFormat);
        emit finished();
        return false;
    }
    m_clangExecutable = executable;

    // Create log dir
    QTemporaryDir temporaryDir(QDir::tempPath() + QLatin1String("/qtc-clangstaticanalyzer-XXXXXX"));
    temporaryDir.setAutoRemove(false);
    if (!temporaryDir.isValid()) {
        emit appendMessage(tr("Clang Static Analyzer: Failed to create temporary dir, stop.\n"),
                           Utils::ErrorMessageFormat);
        emit finished();
        return false;
    }
    m_clangLogFileDir = temporaryDir.path();

    // Collect files
    const QList<FileConfiguration> filesToProcess = calculateFilesToProcess(currentProject);
    qCDebug(LOG()) << "Files to process:";
    foreach (const FileConfiguration &fileConfig, filesToProcess) {
        qCDebug(LOG()) << fileConfig.filePath + QLatin1String(" [")
                          + fileConfig.projectPart->projectFile + QLatin1Char(']');
    }
    m_filesToProcess = filesToProcess;
    m_initialFilesToProcessSize = m_filesToProcess.count();

    // Set up progress information
    using namespace Core;
    FutureProgress *futureProgress
        = ProgressManager::addTask(m_progress.future(), tr("Analyzing"), "ClangStaticAnalyzer");
    futureProgress->setKeepOnFinish(FutureProgress::HideOnFinish);
    connect(futureProgress, &FutureProgress::canceled,
            this, &ClangStaticAnalyzerRunControl::onProgressCanceled);
    m_progress.setProgressRange(0, m_initialFilesToProcessSize);
    m_progress.reportStarted();

    // Start process(es)
    const int parallelRuns = ClangStaticAnalyzerSettings::instance()->simultaneousProcesses();
    QTC_ASSERT(parallelRuns >= 1, emit finished(); return false);
    m_runningProcesses = 0;
    while (m_runningProcesses < parallelRuns && !m_filesToProcess.isEmpty())
        analyzeNextFile();
    return true;
}

void ClangStaticAnalyzerRunControl::stopEngine()
{
    m_filesToProcess.clear();
}

QStringList createDefinesAndIncludesOptions(const ProjectPart::Ptr projectPart)
{
    QStringList result;
    result += CppTools::CompilerOptionsBuilder::createDefineOptions(projectPart->projectDefines);
    result += CppTools::CompilerOptionsBuilder::createHeaderPathOptions(projectPart->headerPaths);
    result += QLatin1String("-fPIC"); // TODO: Remove?
    return result;
}

void ClangStaticAnalyzerRunControl::analyzeNextFile()
{
    if (m_progress.isFinished())
        return; // The previous call already reported that we are finished.

    if (m_filesToProcess.isEmpty()) {
        QTC_ASSERT(m_runningProcesses >= 0, return);
        if (m_runningProcesses == 0) {
            m_progress.reportFinished();
            emit finished();
        }
        return;
    }

    const FileConfiguration config = m_filesToProcess.takeFirst();
    const QString filePath = config.filePath;
    const QStringList definesAndIncludesOptions
            = createDefinesAndIncludesOptions(config.projectPart);

    ClangStaticAnalyzerRunner *runner = createRunner();
    qCDebug(LOG) << "analyzeNextFile:" << filePath;
    QTC_ASSERT(runner->run(filePath, definesAndIncludesOptions), return);
    ++m_runningProcesses;
}

ClangStaticAnalyzerRunner *ClangStaticAnalyzerRunControl::createRunner()
{
    QTC_ASSERT(!m_clangExecutable.isEmpty(), return 0);
    QTC_ASSERT(!m_clangLogFileDir.isEmpty(), return 0);

    ClangStaticAnalyzerRunner *runner
            = new ClangStaticAnalyzerRunner(m_clangExecutable, m_clangLogFileDir, this);
    connect(runner, &ClangStaticAnalyzerRunner::finishedWithSuccess,
            this, &ClangStaticAnalyzerRunControl::onRunnerFinishedWithSuccess);
    connect(runner, &ClangStaticAnalyzerRunner::finishedWithFailure,
            this, &ClangStaticAnalyzerRunControl::onRunnerFinishedWithFailure);
    return runner;
}

void ClangStaticAnalyzerRunControl::onRunnerFinishedWithSuccess(const QString &logFilePath)
{
    qCDebug(LOG) << "onRunnerFinishedWithSuccess:" << logFilePath;
    handleFinished();

    QString errorMessage;
    const QList<Diagnostic> diagnostics = LogFileReader::read(logFilePath, &errorMessage);
    if (!errorMessage.isEmpty())
        qCDebug(LOG) << "onRunnerFinishedWithSuccess: Error reading log file:" << errorMessage;
    if (!diagnostics.isEmpty())
        emit newDiagnosticsAvailable(diagnostics);
}

void ClangStaticAnalyzerRunControl::onRunnerFinishedWithFailure(const QString &errorMessage,
                                                                const QString &errorDetails)
{
    qCDebug(LOG) << "onRunnerFinishedWithFailure:" << errorMessage << errorDetails;
    handleFinished();
}

void ClangStaticAnalyzerRunControl::handleFinished()
{
    --m_runningProcesses;
    updateProgressValue();
    sender()->deleteLater();
    analyzeNextFile();
}

void ClangStaticAnalyzerRunControl::onProgressCanceled()
{
    Analyzer::AnalyzerManager::stopTool();
    m_progress.reportCanceled();
    m_progress.reportFinished();
}

void ClangStaticAnalyzerRunControl::updateProgressValue()
{
    m_progress.setProgressValue(m_initialFilesToProcessSize - m_filesToProcess.size());
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
