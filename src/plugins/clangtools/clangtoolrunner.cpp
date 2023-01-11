// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtoolrunner.h"

#include "clangtoolsutils.h"

#include <cppeditor/clangdiagnosticconfigsmodel.h>
#include <cppeditor/compileroptionsbuilder.h>

#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/temporaryfile.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(LOG, "qtc.clangtools.runner", QtWarningMsg)

using namespace CppEditor;
using namespace Utils;

namespace ClangTools {
namespace Internal {

static bool isClMode(const QStringList &options)
{
    return options.contains("--driver-mode=cl");
}

static QStringList checksArguments(ClangToolType tool,
                                   const ClangDiagnosticConfig &diagnosticConfig)
{
    if (tool == ClangToolType::Tidy) {
        const ClangDiagnosticConfig::TidyMode tidyMode = diagnosticConfig.clangTidyMode();
        // The argument "-config={}" stops stating/evaluating the .clang-tidy file.
        if (tidyMode == ClangDiagnosticConfig::TidyMode::UseDefaultChecks)
            return {"-config={}", "-checks=-clang-diagnostic-*"};
        if (tidyMode == ClangDiagnosticConfig::TidyMode::UseCustomChecks)
            return {"-config=" + diagnosticConfig.clangTidyChecksAsJson()};
        return {"--warnings-as-errors=-*", "-checks=-clang-diagnostic-*"};
    }
    const QString clazyChecks = diagnosticConfig.checks(ClangToolType::Clazy);
    if (!clazyChecks.isEmpty())
        return {"-checks=" + diagnosticConfig.checks(ClangToolType::Clazy)};
    return {};
}

static QStringList clangArguments(const ClangDiagnosticConfig &diagnosticConfig,
                                  const QStringList &baseOptions)
{
    QStringList arguments;
    arguments << ClangDiagnosticConfigsModel::globalDiagnosticOptions()
              << (isClMode(baseOptions) ? clangArgsForCl(diagnosticConfig.clangOptions())
                                        : diagnosticConfig.clangOptions())
              << baseOptions;

    if (LOG().isDebugEnabled())
        arguments << QLatin1String("-v");

    return arguments;
}

static QString generalProcessError(const QString &name)
{
    return ClangToolRunner::tr("An error occurred with the %1 process.").arg(name);
}

static QString finishedDueToCrash(const QString &name)
{
    return ClangToolRunner::tr("%1 crashed.").arg(name);
}

static QString finishedWithBadExitCode(const QString &name, int exitCode)
{
    return ClangToolRunner::tr("%1 finished with exit code: %2.").arg(name).arg(exitCode);
}

ClangToolRunner::ClangToolRunner(const AnalyzeInputData &input, QObject *parent)
    : QObject(parent)
    , m_input(input)
{
    m_name = input.tool == ClangToolType::Tidy ? tr("Clang-Tidy") : tr("Clazy");
    m_executable = toolExecutable(input.tool);
    m_argsCreator = [this, input](const QStringList &baseOptions) {
        return QStringList() << checksArguments(input.tool, input.config)
                             << mainToolArguments()
                             << "--"
                             << clangArguments(input.config, baseOptions);
    };
    QTC_CHECK(!m_input.outputDirPath.isEmpty());

    m_process.setEnvironment(input.environment);
    m_process.setUseCtrlCStub(true);
    m_process.setWorkingDirectory(m_input.outputDirPath); // Current clang-cl puts log file into working dir.
    connect(&m_process, &QtcProcess::done, this, &ClangToolRunner::onProcessDone);
}

QStringList ClangToolRunner::mainToolArguments() const
{
    QStringList result;
    result << "-export-fixes=" + m_outputFilePath;
    if (!m_input.overlayFilePath.isEmpty() && supportsVFSOverlay())
        result << "--vfsoverlay=" + m_input.overlayFilePath;
    result << QDir::toNativeSeparators(m_input.unit.file);
    return result;
}

bool ClangToolRunner::supportsVFSOverlay() const
{
    static QMap<FilePath, bool> vfsCapabilities;
    auto it = vfsCapabilities.find(m_executable);
    if (it == vfsCapabilities.end()) {
        QtcProcess p;
        p.setCommand({m_executable, {"--help"}});
        p.runBlocking();
        it = vfsCapabilities.insert(m_executable, p.allOutput().contains("vfsoverlay"));
    }
    return it.value();
}

static QString createOutputFilePath(const FilePath &dirPath, const QString &fileToAnalyze)
{
    const QString fileName = QFileInfo(fileToAnalyze).fileName();
    const FilePath fileTemplate = dirPath.pathAppended("report-" + fileName + "-XXXXXX");

    TemporaryFile temporaryFile("clangtools");
    temporaryFile.setAutoRemove(false);
    temporaryFile.setFileTemplate(fileTemplate.path());
    if (temporaryFile.open()) {
        temporaryFile.close();
        return temporaryFile.fileName();
    }
    return {};
}

bool ClangToolRunner::run()
{
    QTC_ASSERT(!m_executable.isEmpty(), return false);
    QTC_CHECK(!m_input.unit.arguments.contains(QLatin1String("-o")));
    QTC_CHECK(!m_input.unit.arguments.contains(m_input.unit.file));
    QTC_ASSERT(FilePath::fromString(m_input.unit.file).exists(), return false);

    m_outputFilePath = createOutputFilePath(m_input.outputDirPath, m_input.unit.file);
    QTC_ASSERT(!m_outputFilePath.isEmpty(), return false);
    const CommandLine commandLine = {m_executable, m_argsCreator(m_input.unit.arguments)};

    qCDebug(LOG).noquote() << "Starting" << commandLine.toUserOutput();
    m_process.setCommand(commandLine);
    m_process.start();
    return true;
}

void ClangToolRunner::onProcessDone()
{
    if (m_process.result() == ProcessResult::StartFailed) {
        emit finishedWithFailure(generalProcessError(m_name), commandlineAndOutput());
    } else if (m_process.result() == ProcessResult::FinishedWithSuccess) {
        qCDebug(LOG).noquote() << "Output:\n" << m_process.cleanedStdOut();
        emit finishedWithSuccess(m_input.unit.file);
    } else if (m_process.result() == ProcessResult::FinishedWithError) {
        emit finishedWithFailure(finishedWithBadExitCode(m_name, m_process.exitCode()),
                                 commandlineAndOutput());
    } else { // == QProcess::CrashExit
        emit finishedWithFailure(finishedDueToCrash(m_name), commandlineAndOutput());
    }
}

QString ClangToolRunner::commandlineAndOutput() const
{
    return tr("Command line: %1\n"
              "Process Error: %2\n"
              "Output:\n%3")
        .arg(m_process.commandLine().toUserOutput())
        .arg(m_process.error())
        .arg(m_process.cleanedStdOut());
}

} // namespace Internal
} // namespace ClangTools
