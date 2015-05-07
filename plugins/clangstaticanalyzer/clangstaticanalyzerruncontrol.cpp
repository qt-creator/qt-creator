/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise LicenseChecker Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#include "clangstaticanalyzerruncontrol.h"

#include "clangstaticanalyzerlogfilereader.h"
#include "clangstaticanalyzerrunner.h"
#include "clangstaticanalyzersettings.h"
#include "clangstaticanalyzerutils.h"

#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerutils.h>

#include <clangcodemodel/clangutils.h>

#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppprojects.h>
#include <cpptools/cppprojectfile.h>

#include <projectexplorer/abi.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <utils/algorithm.h>

#include <QLoggingCategory>
#include <QTemporaryDir>

using namespace CppTools;
using namespace ProjectExplorer;

static Q_LOGGING_CATEGORY(LOG, "qtc.clangstaticanalyzer.runcontrol")

namespace ClangStaticAnalyzer {
namespace Internal {

ClangStaticAnalyzerRunControl::ClangStaticAnalyzerRunControl(
            const Analyzer::AnalyzerStartParameters &startParams,
            ProjectExplorer::RunConfiguration *runConfiguration,
            const ProjectInfo &projectInfo)
    : AnalyzerRunControl(startParams, runConfiguration)
    , m_projectInfo(projectInfo)
    , m_toolchainType(ProjectExplorer::ToolChainKitInformation
                      ::toolChain(runConfiguration->target()->kit())->type())
    , m_wordWidth(runConfiguration->abi().wordWidth())
    , m_initialFilesToProcessSize(0)
    , m_filesAnalyzed(0)
    , m_filesNotAnalyzed(0)
{
}

static void prependWordWidthArgumentIfNotIncluded(QStringList *arguments, unsigned char wordWidth)
{
    QTC_ASSERT(arguments, return);

    const QString m64Argument = QLatin1String("-m64");
    const QString m32Argument = QLatin1String("-m32");

    const QString argument = wordWidth == 64 ? m64Argument : m32Argument;
    if (!arguments->contains(argument))
        arguments->prepend(argument);

    QTC_CHECK(!arguments->contains(m32Argument) || !arguments->contains(m64Argument));
}

// Removes (1) filePath (2) -o <somePath>.
// Adds -m64/-m32 argument if not already included.
static QStringList tweakedArguments(const QString &filePath,
                                    const QStringList &arguments,
                                    unsigned char wordWidth)
{
    QStringList newArguments;

    bool skip = false;
    foreach (const QString &argument, arguments) {
        if (skip) {
            skip = false;
            continue;
        } else if (argument == QLatin1String("-o")) {
            skip = true;
            continue;
        } else if (QDir::fromNativeSeparators(argument) == filePath) {
            continue; // TODO: Let it in?
        }

        newArguments << argument;
    }
    QTC_CHECK(skip == false);

    prependWordWidthArgumentIfNotIncluded(&newArguments, wordWidth);

    return newArguments;
}

static QStringList argumentsFromProjectPart(const CppTools::ProjectPart::Ptr &projectPart,
                                            CppTools::ProjectFile::Kind fileKind,
                                            const QString &toolchainType,
                                            unsigned char wordWidth)
{
    QStringList result;

    const bool objcExt = projectPart->languageExtensions & ProjectPart::ObjectiveCExtensions;
    result += CppTools::CompilerOptionsBuilder::createLanguageOption(fileKind, objcExt,
                                                                     toolchainType);
    result += CppTools::CompilerOptionsBuilder::createOptionsForLanguage(
                                                    projectPart->languageVersion,
                                                    projectPart->languageExtensions, false,
                                                    toolchainType);
    result += CppTools::CompilerOptionsBuilder::createDefineOptions(projectPart->toolchainDefines,
                                                                    false, toolchainType);
    result += CppTools::CompilerOptionsBuilder::createDefineOptions(projectPart->projectDefines,
                                                                    false, toolchainType);
    result += CppTools::CompilerOptionsBuilder::createHeaderPathOptions(
                projectPart->headerPaths,
                CompilerOptionsBuilder::IsBlackListed(),
                toolchainType);

    if (toolchainType == QLatin1String("msvc"))
        result += QLatin1String("/EHsc"); // clang-cl does not understand exceptions
    else
        result += QLatin1String("-fPIC"); // TODO: Remove?

    prependWordWidthArgumentIfNotIncluded(&result, wordWidth);

    return result;
}

static AnalyzeUnits unitsToAnalyzeFromCompilerCallData(
            const ProjectInfo::CompilerCallData &compilerCallData,
            unsigned char wordWidth)
{
    qCDebug(LOG) << "Taking arguments for analyzing from CompilerCallData.";

    AnalyzeUnits unitsToAnalyze;

    QHashIterator<QString, QList<QStringList> > it(compilerCallData);
    while (it.hasNext()) {
        it.next();
        const QString file = it.key();
        const QList<QStringList> compilerCalls = it.value();
        foreach (const QStringList &options, compilerCalls) {
            const QStringList arguments = tweakedArguments(file, options, wordWidth);
            unitsToAnalyze << AnalyzeUnit(file, arguments);
        }
    }

    return unitsToAnalyze;
}

static AnalyzeUnits unitsToAnalyzeFromProjectParts(const QList<ProjectPart::Ptr> projectParts,
                                                   const QString &toolchainType,
                                                   unsigned char wordWidth)
{
    qCDebug(LOG) << "Taking arguments for analyzing from ProjectParts.";

    AnalyzeUnits unitsToAnalyze;

    foreach (const ProjectPart::Ptr &projectPart, projectParts) {
        if (!projectPart->selectedForBuilding)
            continue;

        foreach (const ProjectFile &file, projectPart->files) {
            if (file.path == CppModelManager::configurationFileName())
                continue;
            QTC_CHECK(file.kind != ProjectFile::Unclassified);
            if (ProjectFile::isSource(file.kind)) {
                const QStringList arguments = argumentsFromProjectPart(projectPart,
                                                                       file.kind,
                                                                       toolchainType,
                                                                       wordWidth);
                unitsToAnalyze << AnalyzeUnit(file.path, arguments);
            }
        }
    }

    return unitsToAnalyze;
}

AnalyzeUnits ClangStaticAnalyzerRunControl::sortedUnitsToAnalyze()
{
    QTC_ASSERT(m_projectInfo.isValid(), return AnalyzeUnits());

    AnalyzeUnits units;
    const ProjectInfo::CompilerCallData compilerCallData = m_projectInfo.compilerCallData();
    if (compilerCallData.isEmpty()) {
        units = unitsToAnalyzeFromProjectParts(m_projectInfo.projectParts(),
                                               m_toolchainType,
                                               m_wordWidth);
    } else {
        units = unitsToAnalyzeFromCompilerCallData(compilerCallData, m_wordWidth);
    }

    Utils::sort(units, [](const AnalyzeUnit &a1, const AnalyzeUnit &a2) -> bool {
        return a1.file < a2.file;
    });
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

bool ClangStaticAnalyzerRunControl::startEngine()
{
    m_success = false;
    emit starting(this);

    QTC_ASSERT(m_projectInfo.isValid(), emit finished(); return false);
    const QString projectFile = m_projectInfo.project()->projectFilePath().toString();
    appendMessage(tr("Running Clang Static Analyzer on %1").arg(projectFile) + QLatin1Char('\n'),
                        Utils::NormalMessageFormat);

    // Check clang executable
    bool isValidClangExecutable;
    const QString executable
            = clangExecutableFromSettings(m_toolchainType, &isValidClangExecutable);
    if (!isValidClangExecutable) {
        const QString errorMessage = tr("Clang Static Analyzer: Invalid executable \"%1\", stop.")
                .arg(executable);
        appendMessage(errorMessage + QLatin1Char('\n'), Utils::ErrorMessageFormat);
        AnalyzerUtils::logToIssuesPane(Task::Error, errorMessage);
        emit finished();
        return false;
    }
    m_clangExecutable = executable;

    // Create log dir
    QTemporaryDir temporaryDir(QDir::tempPath() + QLatin1String("/qtc-clangstaticanalyzer-XXXXXX"));
    temporaryDir.setAutoRemove(false);
    if (!temporaryDir.isValid()) {
        const QString errorMessage
                = tr("Clang Static Analyzer: Failed to create temporary dir, stop.");
        appendMessage(errorMessage + QLatin1Char('\n'), Utils::ErrorMessageFormat);
        AnalyzerUtils::logToIssuesPane(Task::Error, errorMessage);
        emit finished();
        return false;
    }
    m_clangLogFileDir = temporaryDir.path();

    // Collect files
    const AnalyzeUnits unitsToProcess = sortedUnitsToAnalyze();
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
            this, &ClangStaticAnalyzerRunControl::onProgressCanceled);
    m_progress.setProgressRange(0, m_initialFilesToProcessSize);
    m_progress.reportStarted();

    // Start process(es)
    qCDebug(LOG) << "Environment:" << startParameters().environment;
    m_runners.clear();
    const int parallelRuns = ClangStaticAnalyzerSettings::instance()->simultaneousProcesses();
    QTC_ASSERT(parallelRuns >= 1, emit finished(); return false);
    m_success = true;
    while (m_runners.size() < parallelRuns && !m_unitsToProcess.isEmpty())
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
    m_unitsToProcess.clear();
    appendMessage(tr("Clang Static Analyzer stopped by user.") + QLatin1Char('\n'),
                  Utils::NormalMessageFormat);
    m_progress.reportFinished();
    emit finished();
}

void ClangStaticAnalyzerRunControl::analyzeNextFile()
{
    if (m_progress.isFinished())
        return; // The previous call already reported that we are finished.

    if (m_unitsToProcess.isEmpty()) {
        if (m_runners.size() == 0) {
            appendMessage(tr("Clang Static Analyzer finished: "
                             "Processed %1 files successfully, %2 failed.")
                                .arg(m_filesAnalyzed)
                                .arg(m_filesNotAnalyzed)
                             + QLatin1Char('\n'),
                          Utils::NormalMessageFormat);
            if (m_filesAnalyzed == 0 && m_filesNotAnalyzed != 0) {
                AnalyzerUtils::logToIssuesPane(Task::Error,
                        tr("Clang Static Analyzer: Failed to analyze any files."));
            }
            m_progress.reportFinished();
            emit finished();
        }
        return;
    }

    const AnalyzeUnit unit = m_unitsToProcess.takeFirst();
    qCDebug(LOG) << "analyzeNextFile:" << unit.file;

    ClangStaticAnalyzerRunner *runner = createRunner();
    m_runners.insert(runner);
    QTC_ASSERT(runner->run(unit.file, unit.arguments), return);

    appendMessage(tr("Analyzing \"%1\".").arg(unit.file) + QLatin1Char('\n'),
                  Utils::StdOutFormat);
}

ClangStaticAnalyzerRunner *ClangStaticAnalyzerRunControl::createRunner()
{
    QTC_ASSERT(!m_clangExecutable.isEmpty(), return 0);
    QTC_ASSERT(!m_clangLogFileDir.isEmpty(), return 0);

    ClangStaticAnalyzerRunner *runner
            = new ClangStaticAnalyzerRunner(m_clangExecutable,
                                            m_clangLogFileDir,
                                            startParameters().environment,
                                            this);
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
    m_success = false;
    const QString filePath = qobject_cast<ClangStaticAnalyzerRunner *>(sender())->filePath();
    appendMessage(tr("Failed to analyze \"%1\": %2").arg(filePath, errorMessage)
                  + QLatin1Char('\n')
                  , Utils::StdErrFormat);
    appendMessage(errorDetails, Utils::StdErrFormat);
    AnalyzerUtils::logToIssuesPane(Task::Warning, errorMessage);
    AnalyzerUtils::logToIssuesPane(Task::Warning, errorDetails);
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
    m_progress.setProgressValue(m_initialFilesToProcessSize - m_unitsToProcess.size());
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
