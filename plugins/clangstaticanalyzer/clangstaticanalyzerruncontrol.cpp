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

#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

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
    , m_filesAnalyzed(0)
    , m_filesNotAnalyzed(0)
{
}

static QList<ClangStaticAnalyzerRunControl::SourceFileConfiguration> calculateFilesToProcess(
            Project *project)
{
    typedef ClangStaticAnalyzerRunControl::SourceFileConfiguration SourceFileConfiguration;
    QTC_ASSERT(project, return QList<SourceFileConfiguration>());
    ProjectInfo projectInfo = CppModelManager::instance()->projectInfo(project);
    QTC_ASSERT(projectInfo, return QList<SourceFileConfiguration>());

    QList<SourceFileConfiguration> files;
    const QList<ProjectPart::Ptr> projectParts = projectInfo.projectParts();
    foreach (const ProjectPart::Ptr &projectPart, projectParts) {
        foreach (const ProjectFile &file, projectPart->files) {
            if (file.path == CppModelManager::configurationFileName())
                continue;
            QTC_CHECK(file.kind != ProjectFile::Unclassified);
            if (ProjectFile::isSource(file.kind))
                files << SourceFileConfiguration(file, projectPart);
        }
    }

    return files;
}

bool ClangStaticAnalyzerRunControl::startEngine()
{
    emit starting(this);

    RunConfiguration *runConfig = runConfiguration();
    QTC_ASSERT(runConfig, emit finished(); return false);
    Target *target = runConfig->target();
    QTC_ASSERT(target, emit finished(); return false);
    Project *project = target->project();
    QTC_ASSERT(project, emit finished(); return false);

    const QString projectFile = project->projectFilePath().toString();
    appendMessage(tr("Running Clang Static Analyzer on %1").arg(projectFile) + QLatin1Char('\n'),
                        Utils::NormalMessageFormat);

    // Check clang executable
    bool isValidClangExecutable;
    const QString executable = clangExecutableFromSettings(&isValidClangExecutable);
    if (!isValidClangExecutable) {
        emit appendMessage(tr("Clang Static Analyzer: Invalid executable \"%1\", stop.")
                            .arg(executable) + QLatin1Char('\n'),
                           Utils::ErrorMessageFormat);
        emit finished();
        return false;
    }
    m_clangExecutable = executable;

    // Create log dir
    QTemporaryDir temporaryDir(QDir::tempPath() + QLatin1String("/qtc-clangstaticanalyzer-XXXXXX"));
    temporaryDir.setAutoRemove(false);
    if (!temporaryDir.isValid()) {
        emit appendMessage(tr("Clang Static Analyzer: Failed to create temporary dir, stop.")
                           + QLatin1Char('\n'), Utils::ErrorMessageFormat);
        emit finished();
        return false;
    }
    m_clangLogFileDir = temporaryDir.path();

    // Collect files
    const QList<SourceFileConfiguration> filesToProcess = calculateFilesToProcess(project);
    qCDebug(LOG) << "Files to process:";
    foreach (const SourceFileConfiguration &fileConfig, filesToProcess) {
        qCDebug(LOG) << fileConfig.file.path + QLatin1String(" [")
                          + fileConfig.projectPart->projectFile + QLatin1Char(']');
    }
    m_filesToProcess = filesToProcess;
    m_initialFilesToProcessSize = m_filesToProcess.count();
    m_filesAnalyzed = 0;
    m_filesNotAnalyzed = 0;

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
    m_runners.clear();
    const int parallelRuns = ClangStaticAnalyzerSettings::instance()->simultaneousProcesses();
    QTC_ASSERT(parallelRuns >= 1, emit finished(); return false);
    while (m_runners.size() < parallelRuns && !m_filesToProcess.isEmpty())
        analyzeNextFile();
    return true;
}

void ClangStaticAnalyzerRunControl::stopEngine()
{
    QSetIterator<ClangStaticAnalyzerRunner *> i(m_runners);
    while (i.hasNext()) {
        ClangStaticAnalyzerRunner *runner = i.next();
        QObject::disconnect(runner, 0, this, 0);
        delete runner;
    }
    m_runners.clear();
    m_filesToProcess.clear();
    appendMessage(tr("Clang Static Analyzer stopped by user.") + QLatin1Char('\n'),
                  Utils::NormalMessageFormat);
    m_progress.reportFinished();
    emit finished();
}

void ClangStaticAnalyzerRunControl::analyzeNextFile()
{
    if (m_progress.isFinished())
        return; // The previous call already reported that we are finished.

    if (m_filesToProcess.isEmpty()) {
        if (m_runners.size() == 0) {
            appendMessage(tr("Clang Static Analyzer finished: "
                             "Processed %1 files successfully, %2 failed.")
                                .arg(m_filesAnalyzed)
                                .arg(m_filesNotAnalyzed)
                             + QLatin1Char('\n'),
                          Utils::NormalMessageFormat);
            m_progress.reportFinished();
            emit finished();
        }
        return;
    }

    const SourceFileConfiguration config = m_filesToProcess.takeFirst();
    const QString filePath = config.file.path;
    const QStringList options = config.createClangOptions();

    ClangStaticAnalyzerRunner *runner = createRunner();
    m_runners.insert(runner);
    qCDebug(LOG) << "analyzeNextFile:" << filePath;
    QTC_ASSERT(runner->run(filePath, options), return);
    appendMessage(tr("Analyzing \"%1\".").arg(filePath) + QLatin1Char('\n'),
                  Utils::StdOutFormat);
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

    QString errorMessage;
    const QList<Diagnostic> diagnostics = LogFileReader::read(logFilePath, &errorMessage);
    if (!errorMessage.isEmpty()) {
        qCDebug(LOG) << "onRunnerFinishedWithSuccess: Error reading log file:" << errorMessage;
        const QString filePath = qobject_cast<ClangStaticAnalyzerRunner *>(sender())->filePath();
        appendMessage(tr("Failed to analyze \"%1\": %2").arg(filePath, errorMessage)
                        + QLatin1Char('\n')
                      , Utils::StdErrFormat);
    } else {
        ++m_filesAnalyzed;
        if (!diagnostics.isEmpty())
            emit newDiagnosticsAvailable(diagnostics);
    }

    handleFinished();
}

void ClangStaticAnalyzerRunControl::onRunnerFinishedWithFailure(const QString &errorMessage,
                                                                const QString &errorDetails)
{
    qCDebug(LOG) << "onRunnerFinishedWithFailure:" << errorMessage << errorDetails;

    ++m_filesNotAnalyzed;
    const QString filePath = qobject_cast<ClangStaticAnalyzerRunner *>(sender())->filePath();
    appendMessage(tr("Failed to analyze \"%1\": %2").arg(filePath, errorMessage)
                  + QLatin1Char('\n')
                  , Utils::StdErrFormat);
    appendMessage(errorDetails, Utils::StdErrFormat);

    handleFinished();
}

void ClangStaticAnalyzerRunControl::handleFinished()
{
    m_runners.remove(qobject_cast<ClangStaticAnalyzerRunner *>(sender()));
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

QStringList ClangStaticAnalyzerRunControl::SourceFileConfiguration::createClangOptions() const
{
    QStringList result;

    const bool objcExt = projectPart->languageExtensions & ProjectPart::ObjectiveCExtensions;
    result += CppTools::CompilerOptionsBuilder::createLanguageOption(file.kind, objcExt);
    result += CppTools::CompilerOptionsBuilder::createOptionsForLanguage(
                                                    projectPart->languageVersion,
                                                    projectPart->languageExtensions);
    result += CppTools::CompilerOptionsBuilder::createDefineOptions(projectPart->toolchainDefines);
    result += CppTools::CompilerOptionsBuilder::createDefineOptions(projectPart->projectDefines);
    result += CppTools::CompilerOptionsBuilder::createHeaderPathOptions(projectPart->headerPaths);
    result += QLatin1String("-fPIC"); // TODO: Remove?

    return result;
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
