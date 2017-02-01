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
#include "clangstaticanalyzerutils.h"

#include <debugger/analyzer/analyzerconstants.h>

#include <clangcodemodel/clangutils.h>

#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <cpptools/compileroptionsbuilder.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppprojectfile.h>
#include <cpptools/cpptoolsreuse.h>
#include <cpptools/projectinfo.h>

#include <projectexplorer/abi.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/temporarydirectory.h>

#include <QLoggingCategory>

using namespace CppTools;
using namespace ProjectExplorer;

static Q_LOGGING_CATEGORY(LOG, "qtc.clangstaticanalyzer.runcontrol")

namespace ClangStaticAnalyzer {
namespace Internal {

ClangStaticAnalyzerRunControl::ClangStaticAnalyzerRunControl(
            RunConfiguration *runConfiguration,
            Core::Id runMode,
            const ProjectInfo &projectInfo)
    : RunControl(runConfiguration, runMode)
    , m_projectInfo(projectInfo)
    , m_initialFilesToProcessSize(0)
    , m_filesAnalyzed(0)
    , m_filesNotAnalyzed(0)
{
    setDisplayName(tr("Clang Static Analyzer"));
    setIcon(ProjectExplorer::Icons::ANALYZER_START_SMALL_TOOLBAR);

    Target *target = runConfiguration->target();
    BuildConfiguration *buildConfiguration = target->activeBuildConfiguration();
    QTC_ASSERT(buildConfiguration, return);
    m_environment = buildConfiguration->environment();

    ToolChain *toolChain = ToolChainKitInformation::toolChain(target->kit(), ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    QTC_ASSERT(toolChain, return);
    m_targetTriple = toolChain->originalTargetTriple();
}

static void prependWordWidthArgumentIfNotIncluded(QStringList *arguments,
                                                  ProjectPart::ToolChainWordWidth wordWidth)
{
    QTC_ASSERT(arguments, return);

    const QString m64Argument = QLatin1String("-m64");
    const QString m32Argument = QLatin1String("-m32");

    const QString argument = wordWidth == ProjectPart::WordWidth64Bit ? m64Argument : m32Argument;
    if (!arguments->contains(argument))
        arguments->prepend(argument);

    QTC_CHECK(!arguments->contains(m32Argument) || !arguments->contains(m64Argument));
}

static void prependTargetTripleIfNotIncludedAndNotEmpty(QStringList *arguments,
                                                        const QString &targetTriple)
{
    QTC_ASSERT(arguments, return);

    if (targetTriple.isEmpty())
        return;

    const QString targetOption = QLatin1String("-target");

    if (!arguments->contains(targetOption)) {
        arguments->prepend(targetTriple);
        arguments->prepend(targetOption);
    }
}

// Removes (1) inputFile (2) -o <somePath>.
QStringList inputAndOutputArgumentsRemoved(const QString &inputFile, const QStringList &arguments)
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
        } else if (QDir::fromNativeSeparators(argument) == inputFile) {
            continue; // TODO: Let it in?
        }

        newArguments << argument;
    }
    QTC_CHECK(skip == false);

    return newArguments;
}

static QString createLanguageOptionMsvc(ProjectFile::Kind fileKind)
{
    switch (fileKind) {
    case ProjectFile::CHeader:
    case ProjectFile::CSource:
        return QLatin1String("/TC");
        break;
    case ProjectFile::CXXHeader:
    case ProjectFile::CXXSource:
        return QLatin1String("/TP");
        break;
    default:
        break;
    }
    return QString();
}

class ClangStaticAnalyzerOptionsBuilder : public CompilerOptionsBuilder
{
public:
    static QStringList build(const CppTools::ProjectPart &projectPart,
                             CppTools::ProjectFile::Kind fileKind,
                             PchUsage pchUsage)
    {
        ClangStaticAnalyzerOptionsBuilder optionsBuilder(projectPart);

        optionsBuilder.addWordWidth();
        optionsBuilder.addTargetTriple();
        optionsBuilder.addLanguageOption(fileKind);
        optionsBuilder.addOptionsForLanguage(false);
        optionsBuilder.enableExceptions();

        optionsBuilder.addDefineFloat128ForMingw();
        optionsBuilder.addDefineToAvoidIncludingGccOrMinGwIntrinsics();
        const Core::Id type = projectPart.toolchainType;
        if (type != ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID)
            optionsBuilder.addDefines(projectPart.toolchainDefines);
        optionsBuilder.addDefines(projectPart.projectDefines);
        optionsBuilder.undefineClangVersionMacrosForMsvc();
        optionsBuilder.undefineCppLanguageFeatureMacrosForMsvc2015();
        optionsBuilder.addHeaderPathOptions();
        optionsBuilder.addPrecompiledHeaderOptions(pchUsage);
        optionsBuilder.addMsvcCompatibilityVersion();

        if (type != ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID)
            optionsBuilder.add(QLatin1String("-fPIC")); // TODO: Remove?

        return optionsBuilder.options();
    }

    ClangStaticAnalyzerOptionsBuilder(const CppTools::ProjectPart &projectPart)
        : CompilerOptionsBuilder(projectPart)
        , m_isMsvcToolchain(m_projectPart.toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID)
    {
    }

public:
    void undefineClangVersionMacrosForMsvc()
    {
        if (m_projectPart.toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID) {
            static QStringList macroNames {
                "__clang__",
                "__clang_major__",
                "__clang_minor__",
                "__clang_patchlevel__",
                "__clang_version__"
            };

            foreach (const QString &macroName, macroNames)
                add(QLatin1String("/U") + macroName);
        }
    }

private:
    void addTargetTriple() override
    {
        // For MSVC toolchains we use clang-cl.exe, so there is nothing to do here since
        //    1) clang-cl.exe does not understand the "-triple" option
        //    2) clang-cl.exe already hardcodes the right triple value (even if built with mingw)
        if (m_projectPart.toolchainType != ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID)
            CompilerOptionsBuilder::addTargetTriple();
    }

    void addLanguageOption(ProjectFile::Kind fileKind) override
    {
        if (m_isMsvcToolchain)
            add(createLanguageOptionMsvc(fileKind));
        else
            CompilerOptionsBuilder::addLanguageOption(fileKind);
    }

    void addOptionsForLanguage(bool checkForBorlandExtensions) override
    {
        if (m_isMsvcToolchain)
            return;
        CompilerOptionsBuilder::addOptionsForLanguage(checkForBorlandExtensions);
    }

    QString includeOption() const override
    {
        if (m_isMsvcToolchain)
            return QLatin1String("/FI");
        return CompilerOptionsBuilder::includeOption();
    }

    QString includeDirOption() const override
    {
        if (m_isMsvcToolchain)
            return QLatin1String("/I");
        return CompilerOptionsBuilder::includeDirOption();
    }

    QString defineOption() const override
    {
        if (m_isMsvcToolchain)
            return QLatin1String("/D");
        return CompilerOptionsBuilder::defineOption();
    }

    QString undefineOption() const override
    {
        if (m_isMsvcToolchain)
            return QLatin1String("/U");
        return CompilerOptionsBuilder::undefineOption();
    }

    void enableExceptions() override
    {
        if (m_isMsvcToolchain)
            add(QLatin1String("/EHsc"));
        else
            CompilerOptionsBuilder::enableExceptions();
    }

private:
    bool m_isMsvcToolchain;
};

static QStringList createMsCompatibilityVersionOption(const ProjectPart &projectPart)
{
    ClangStaticAnalyzerOptionsBuilder optionsBuilder(projectPart);
    optionsBuilder.addMsvcCompatibilityVersion();
    const QStringList option = optionsBuilder.options();

    return option;
}

static QStringList createOptionsToUndefineCppLanguageFeatureMacrosForMsvc2015(
            const ProjectPart &projectPart)
{
    ClangStaticAnalyzerOptionsBuilder optionsBuilder(projectPart);
    optionsBuilder.undefineCppLanguageFeatureMacrosForMsvc2015();

    return optionsBuilder.options();
}

static QStringList createOptionsToUndefineClangVersionMacrosForMsvc(const ProjectPart &projectPart)
{
    ClangStaticAnalyzerOptionsBuilder optionsBuilder(projectPart);
    optionsBuilder.undefineClangVersionMacrosForMsvc();

    return optionsBuilder.options();
}

static QStringList createHeaderPathsOptionsForClangOnMac(const ProjectPart &projectPart)
{
    QStringList options;

    if (Utils::HostOsInfo::isMacHost()
            && projectPart.toolchainType == ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID) {
        ClangStaticAnalyzerOptionsBuilder optionsBuilder(projectPart);
        optionsBuilder.addHeaderPathOptions();
        options = optionsBuilder.options();
    }

    return options;
}

static QStringList tweakedArguments(const ProjectPart &projectPart,
                                    const QString &filePath,
                                    const QStringList &arguments,
                                    const QString &targetTriple)
{
    QStringList newArguments = inputAndOutputArgumentsRemoved(filePath, arguments);
    prependWordWidthArgumentIfNotIncluded(&newArguments, projectPart.toolChainWordWidth);
    prependTargetTripleIfNotIncludedAndNotEmpty(&newArguments, targetTriple);
    newArguments.append(createHeaderPathsOptionsForClangOnMac(projectPart));
    newArguments.append(createMsCompatibilityVersionOption(projectPart));
    newArguments.append(createOptionsToUndefineClangVersionMacrosForMsvc(projectPart));
    newArguments.append(createOptionsToUndefineCppLanguageFeatureMacrosForMsvc2015(projectPart));

    return newArguments;
}

static AnalyzeUnits unitsToAnalyzeFromCompilerCallData(
            const QHash<QString, ProjectPart::Ptr> &projectFileToProjectPart,
            const ProjectInfo::CompilerCallData &compilerCallData,
            const QString &targetTriple)
{
    qCDebug(LOG) << "Taking arguments for analyzing from CompilerCallData.";

    AnalyzeUnits unitsToAnalyze;

    foreach (const ProjectInfo::CompilerCallGroup &compilerCallGroup, compilerCallData) {
        const ProjectPart::Ptr projectPart
                = projectFileToProjectPart.value(compilerCallGroup.groupId);
        QTC_ASSERT(projectPart, continue);

        QHashIterator<QString, QList<QStringList> > it(compilerCallGroup.callsPerSourceFile);
        while (it.hasNext()) {
            it.next();
            const QString file = it.key();
            const QList<QStringList> compilerCalls = it.value();
            foreach (const QStringList &options, compilerCalls) {
                const QStringList arguments = tweakedArguments(*projectPart,
                                                               file,
                                                               options,
                                                               targetTriple);
                unitsToAnalyze << AnalyzeUnit(file, arguments);
            }
        }
    }

    return unitsToAnalyze;
}

static AnalyzeUnits unitsToAnalyzeFromProjectParts(const QVector<ProjectPart::Ptr> projectParts)
{
    qCDebug(LOG) << "Taking arguments for analyzing from ProjectParts.";

    AnalyzeUnits unitsToAnalyze;

    foreach (const ProjectPart::Ptr &projectPart, projectParts) {
        if (!projectPart->selectedForBuilding)
            continue;

        foreach (const ProjectFile &file, projectPart->files) {
            QTC_CHECK(file.kind != ProjectFile::Unclassified);
            QTC_CHECK(file.kind != ProjectFile::Unsupported);
            if (ProjectFile::isSource(file.kind)) {
                const CompilerOptionsBuilder::PchUsage pchUsage = CppTools::getPchUsage();
                const QStringList arguments
                    = ClangStaticAnalyzerOptionsBuilder::build(*projectPart.data(), file.kind, pchUsage);
                unitsToAnalyze << AnalyzeUnit(file.path, arguments);
            }
        }
    }

    return unitsToAnalyze;
}

static QHash<QString, ProjectPart::Ptr> generateProjectFileToProjectPartMapping(
            const QVector<ProjectPart::Ptr> &projectParts)
{
    QHash<QString, ProjectPart::Ptr> mapping;

    foreach (const ProjectPart::Ptr &projectPart, projectParts) {
        QTC_ASSERT(projectPart, continue);
        mapping[projectPart->projectFile] = projectPart;
    }

    return mapping;
}

AnalyzeUnits ClangStaticAnalyzerRunControl::sortedUnitsToAnalyze()
{
    QTC_ASSERT(m_projectInfo.isValid(), return AnalyzeUnits());

    AnalyzeUnits units;
    const ProjectInfo::CompilerCallData compilerCallData = m_projectInfo.compilerCallData();
    if (compilerCallData.isEmpty()) {
        units = unitsToAnalyzeFromProjectParts(m_projectInfo.projectParts());
    } else {
        const QHash<QString, ProjectPart::Ptr> projectFileToProjectPart
                = generateProjectFileToProjectPartMapping(m_projectInfo.projectParts());
        units = unitsToAnalyzeFromCompilerCallData(projectFileToProjectPart,
                                                   compilerCallData,
                                                   m_targetTriple);
    }

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

static Core::Id toolchainType(ProjectExplorer::RunConfiguration *runConfiguration)
{
    QTC_ASSERT(runConfiguration, return Core::Id());
    return ToolChainKitInformation::toolChain(runConfiguration->target()->kit(), ProjectExplorer::Constants::CXX_LANGUAGE_ID)->typeId();
}

static QString executableForVersionCheck(Core::Id toolchainType, const QString &executable)
{
    if (toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID) {
        const QString suffix = QLatin1String("-cl.exe");
        if (executable.endsWith(suffix, Utils::HostOsInfo::fileNameCaseSensitivity())) {
            QString modified = executable;
            modified.chop(suffix.length());
            modified.append(QLatin1String(".exe"));
            return modified;
        }
    }

    return executable;
}

void ClangStaticAnalyzerRunControl::start()
{
    m_success = false;
    emit starting();

    QTC_ASSERT(m_projectInfo.isValid(), emit finished(); return);
    const Utils::FileName projectFile = m_projectInfo.project()->projectFilePath();
    appendMessage(tr("Running Clang Static Analyzer on %1").arg(projectFile.toUserOutput())
                  + QLatin1Char('\n'), Utils::NormalMessageFormat);

    // Check clang executable
    bool isValidClangExecutable;
    const Core::Id theToolchainType = toolchainType(runConfiguration());
    const QString executable = clangExecutableFromSettings(theToolchainType,
                                                           &isValidClangExecutable);
    if (!isValidClangExecutable) {
        const QString errorMessage = tr("Clang Static Analyzer: Invalid executable \"%1\", stop.")
                .arg(executable);
        appendMessage(errorMessage + QLatin1Char('\n'), Utils::ErrorMessageFormat);
        TaskHub::addTask(Task::Error, errorMessage, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
        emit finished();
        return;
    }

    // Check clang version
    const QString versionCheckExecutable = executableForVersionCheck(theToolchainType, executable);
    const ClangExecutableVersion version = clangExecutableVersion(versionCheckExecutable);
    if (!version.isValid()) {
        const QString warningMessage
            = tr("Clang Static Analyzer: Running with possibly unsupported version, "
                 "could not determine version from executable \"%1\".")
                    .arg(versionCheckExecutable);
        appendMessage(warningMessage + QLatin1Char('\n'), Utils::StdErrFormat);
        TaskHub::addTask(Task::Warning, warningMessage, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
    } else if (!version.isSupportedVersion()) {
        const QString warningMessage
            = tr("Clang Static Analyzer: Running with unsupported version %1, "
                 "supported version is %2.")
                    .arg(version.toString())
                    .arg(ClangExecutableVersion::supportedVersionAsString());
        appendMessage(warningMessage + QLatin1Char('\n'), Utils::StdErrFormat);
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
        appendMessage(errorMessage + QLatin1Char('\n'), Utils::ErrorMessageFormat);
        TaskHub::addTask(Task::Error, errorMessage, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
        emit finished();
        return;
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
    qCDebug(LOG) << "Environment:" << m_environment;
    m_runners.clear();
    const int parallelRuns = ClangStaticAnalyzerSettings::instance()->simultaneousProcesses();
    QTC_ASSERT(parallelRuns >= 1, emit finished(); return);
    m_success = true;
    m_running = true;

    if (m_unitsToProcess.isEmpty()) {
        finalize();
        return;
    }

    emit started();

    while (m_runners.size() < parallelRuns && !m_unitsToProcess.isEmpty())
        analyzeNextFile();
}

RunControl::StopResult ClangStaticAnalyzerRunControl::stop()
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
    m_running = false;
    emit finished();
    return RunControl::StoppedSynchronously;
}

bool ClangStaticAnalyzerRunControl::isRunning() const
{
    return m_running;
}

void ClangStaticAnalyzerRunControl::analyzeNextFile()
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
                      Utils::FileName::fromString(unit.file).toUserOutput()) + QLatin1Char('\n'),
                  Utils::StdOutFormat);
}

ClangStaticAnalyzerRunner *ClangStaticAnalyzerRunControl::createRunner()
{
    QTC_ASSERT(!m_clangExecutable.isEmpty(), return 0);
    QTC_ASSERT(!m_clangLogFileDir.isEmpty(), return 0);

    auto runner = new ClangStaticAnalyzerRunner(m_clangExecutable,
                                                m_clangLogFileDir,
                                                m_environment,
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
    qCDebug(LOG).noquote() << "onRunnerFinishedWithFailure:"
                           << errorMessage << '\n' << errorDetails;

    ++m_filesNotAnalyzed;
    m_success = false;
    const QString filePath = qobject_cast<ClangStaticAnalyzerRunner *>(sender())->filePath();
    appendMessage(tr("Failed to analyze \"%1\": %2").arg(filePath, errorMessage)
                  + QLatin1Char('\n')
                  , Utils::StdErrFormat);
    appendMessage(errorDetails, Utils::StdErrFormat);
    TaskHub::addTask(Task::Warning, errorMessage, Debugger::Constants::ANALYZERTASK_ID);
    TaskHub::addTask(Task::Warning, errorDetails, Debugger::Constants::ANALYZERTASK_ID);
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
    m_progress.reportCanceled();
    stop();
}

void ClangStaticAnalyzerRunControl::updateProgressValue()
{
    m_progress.setProgressValue(m_initialFilesToProcessSize - m_unitsToProcess.size());
}

void ClangStaticAnalyzerRunControl::finalize()
{
    appendMessage(tr("Clang Static Analyzer finished: "
                     "Processed %1 files successfully, %2 failed.")
                        .arg(m_filesAnalyzed)
                        .arg(m_filesNotAnalyzed)
                     + QLatin1Char('\n'),
                  Utils::NormalMessageFormat);

    if (m_filesNotAnalyzed != 0) {
        QString msg = tr("Clang Static Analyzer: Not all files could be analyzed.");
        TaskHub::addTask(Task::Error, msg, Debugger::Constants::ANALYZERTASK_ID);
        TaskHub::requestPopup();
    }

    m_progress.reportFinished();
    m_running = false;
    emit finished();
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
