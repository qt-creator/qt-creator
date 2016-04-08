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

#include <debugger/analyzer/analyzermanager.h>
#include <debugger/analyzer/analyzerutils.h>

#include <clangcodemodel/clangutils.h>

#include <coreplugin/progressmanager/futureprogress.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <cpptools/compileroptionsbuilder.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppprojectfile.h>
#include <cpptools/projectinfo.h>

#include <projectexplorer/abi.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <utils/algorithm.h>

#include <QLoggingCategory>
#include <QTemporaryDir>

using namespace CppTools;
using namespace ProjectExplorer;

static Q_LOGGING_CATEGORY(LOG, "qtc.clangstaticanalyzer.runcontrol")

namespace ClangStaticAnalyzer {
namespace Internal {

ClangStaticAnalyzerRunControl::ClangStaticAnalyzerRunControl(
            RunConfiguration *runConfiguration,
            Core::Id runMode,
            const ProjectInfo &projectInfo)
    : AnalyzerRunControl(runConfiguration, runMode)
    , m_projectInfo(projectInfo)
    , m_initialFilesToProcessSize(0)
    , m_filesAnalyzed(0)
    , m_filesNotAnalyzed(0)
{
    Target *target = runConfiguration->target();
    BuildConfiguration *buildConfiguration = target->activeBuildConfiguration();
    QTC_ASSERT(buildConfiguration, return);
    m_environment = buildConfiguration->environment();

    ToolChain *toolChain = ToolChainKitInformation::toolChain(target->kit());
    QTC_ASSERT(toolChain, return);
    Abi abi = runConfiguration->abi();
    m_extraToolChainInfo.wordWidth = abi.wordWidth();
    m_extraToolChainInfo.isMsvc2015 = abi.osFlavor() == Abi::WindowsMsvc2015Flavor;
    m_extraToolChainInfo.targetTriple = toolChain->originalTargetTriple();
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

static void appendMsCompatibility2015OptionForMsvc2015(QStringList *arguments, bool isMsvc2015)
{
    QTC_ASSERT(arguments, return);

    if (isMsvc2015)
        arguments->append(QLatin1String("-fms-compatibility-version=19"));
}

static QStringList languageFeatureMacros()
{
    // Collected with:
    //  $ CALL "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86
    //  $ D:\usr\llvm-3.8.0\bin\clang++.exe -fms-compatibility-version=19 -std=c++1y -dM -E D:\empty.cpp | grep __cpp_
    static QStringList macros {
        QLatin1String("__cpp_aggregate_nsdmi"),
        QLatin1String("__cpp_alias_templates"),
        QLatin1String("__cpp_attributes"),
        QLatin1String("__cpp_binary_literals"),
        QLatin1String("__cpp_constexpr"),
        QLatin1String("__cpp_decltype"),
        QLatin1String("__cpp_decltype_auto"),
        QLatin1String("__cpp_delegating_constructors"),
        QLatin1String("__cpp_digit_separators"),
        QLatin1String("__cpp_generic_lambdas"),
        QLatin1String("__cpp_inheriting_constructors"),
        QLatin1String("__cpp_init_captures"),
        QLatin1String("__cpp_initializer_lists"),
        QLatin1String("__cpp_lambdas"),
        QLatin1String("__cpp_nsdmi"),
        QLatin1String("__cpp_range_based_for"),
        QLatin1String("__cpp_raw_strings"),
        QLatin1String("__cpp_ref_qualifiers"),
        QLatin1String("__cpp_return_type_deduction"),
        QLatin1String("__cpp_rtti"),
        QLatin1String("__cpp_rvalue_references"),
        QLatin1String("__cpp_static_assert"),
        QLatin1String("__cpp_unicode_characters"),
        QLatin1String("__cpp_unicode_literals"),
        QLatin1String("__cpp_user_defined_literals"),
        QLatin1String("__cpp_variable_templates"),
        QLatin1String("__cpp_variadic_templates"),
    };

    return macros;
}

static void undefineCppLanguageFeatureMacrosForMsvc2015(QStringList *arguments, bool isMsvc2015)
{
    QTC_ASSERT(arguments, return);

    if (isMsvc2015) {
        foreach (const QString &macroName, languageFeatureMacros())
            arguments->append(QLatin1String("/U") + macroName);
    }
}

static QStringList tweakedArguments(const QString &filePath,
                                    const QStringList &arguments,
                                    const ExtraToolChainInfo &extraParams)
{
    QStringList newArguments = inputAndOutputArgumentsRemoved(filePath, arguments);
    prependWordWidthArgumentIfNotIncluded(&newArguments, extraParams.wordWidth);
    prependTargetTripleIfNotIncludedAndNotEmpty(&newArguments, extraParams.targetTriple);
    appendMsCompatibility2015OptionForMsvc2015(&newArguments, extraParams.isMsvc2015);
    undefineCppLanguageFeatureMacrosForMsvc2015(&newArguments, extraParams.isMsvc2015);

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
                             const ExtraToolChainInfo &extraParams)
    {
        ClangStaticAnalyzerOptionsBuilder optionsBuilder(projectPart);

        optionsBuilder.addTargetTriple();
        optionsBuilder.addLanguageOption(fileKind);
        optionsBuilder.addOptionsForLanguage(false);

        // In gcc headers, lots of built-ins are referenced that clang does not understand.
        // Therefore, prevent the inclusion of the header that references them. Of course, this
        // will break if code actually requires stuff from there, but that should be the less common
        // case.
        const Core::Id type = projectPart.toolchainType;
        if (type == ProjectExplorer::Constants::MINGW_TOOLCHAIN_TYPEID
                || type == ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID)
            optionsBuilder.addDefine("#define _X86INTRIN_H_INCLUDED\n");

        optionsBuilder.addToolchainAndProjectDefines();
        optionsBuilder.addHeaderPathOptions();

        if (type == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID)
            optionsBuilder.add(QLatin1String("/EHsc")); // clang-cl does not understand exceptions
        else
            optionsBuilder.add(QLatin1String("-fPIC")); // TODO: Remove?

        QStringList options = optionsBuilder.options();
        prependWordWidthArgumentIfNotIncluded(&options, extraParams.wordWidth);
        appendMsCompatibility2015OptionForMsvc2015(&options, extraParams.isMsvc2015);
        undefineCppLanguageFeatureMacrosForMsvc2015(&options, extraParams.isMsvc2015);

        return options;
    }

private:
    ClangStaticAnalyzerOptionsBuilder(const CppTools::ProjectPart &projectPart)
        : CompilerOptionsBuilder(projectPart)
        , m_isMsvcToolchain(m_projectPart.toolchainType == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID)
    {
    }

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
            return QLatin1String("/I");
        return CompilerOptionsBuilder::includeOption();
    }

    QString defineOption() const override
    {
        if (m_isMsvcToolchain)
            return QLatin1String("/D");
        return CompilerOptionsBuilder::defineOption();
    }

private:
    bool m_isMsvcToolchain;
};

static AnalyzeUnits unitsToAnalyzeFromCompilerCallData(
            const ProjectInfo::CompilerCallData &compilerCallData,
            const ExtraToolChainInfo &extraParams)
{
    qCDebug(LOG) << "Taking arguments for analyzing from CompilerCallData.";

    AnalyzeUnits unitsToAnalyze;

    QHashIterator<QString, QList<QStringList> > it(compilerCallData);
    while (it.hasNext()) {
        it.next();
        const QString file = it.key();
        const QList<QStringList> compilerCalls = it.value();
        foreach (const QStringList &options, compilerCalls) {
            const QStringList arguments = tweakedArguments(file, options, extraParams);
            unitsToAnalyze << AnalyzeUnit(file, arguments);
        }
    }

    return unitsToAnalyze;
}

static AnalyzeUnits unitsToAnalyzeFromProjectParts(const QList<ProjectPart::Ptr> projectParts,
                                                   const ExtraToolChainInfo &extraParams)
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
                const QStringList arguments
                    = ClangStaticAnalyzerOptionsBuilder::build(*projectPart.data(),
                                                               file.kind,
                                                               extraParams);
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
    if (compilerCallData.isEmpty())
        units = unitsToAnalyzeFromProjectParts(m_projectInfo.projectParts(), m_extraToolChainInfo);
    else
        units = unitsToAnalyzeFromCompilerCallData(compilerCallData, m_extraToolChainInfo);

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

static Core::Id toolchainType(ProjectExplorer::RunConfiguration *runConfiguration)
{
    QTC_ASSERT(runConfiguration, return Core::Id());
    return ToolChainKitInformation::toolChain(runConfiguration->target()->kit())->typeId();
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
    const QString executable = clangExecutableFromSettings(toolchainType(runConfiguration()),
                                                           &isValidClangExecutable);
    if (!isValidClangExecutable) {
        const QString errorMessage = tr("Clang Static Analyzer: Invalid executable \"%1\", stop.")
                .arg(executable);
        appendMessage(errorMessage + QLatin1Char('\n'), Utils::ErrorMessageFormat);
        AnalyzerUtils::logToIssuesPane(Task::Error, errorMessage);
        emit finished();
        return;
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
    m_progress.reportCanceled();
    m_progress.reportFinished();
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
        AnalyzerUtils::logToIssuesPane(Task::Error,
                tr("Clang Static Analyzer: Not all files could be analyzed."));
    }

    m_progress.reportFinished();
    m_running = false;
    emit finished();
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
