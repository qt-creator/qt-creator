// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtoolrunner.h"

#include "clangtoolsutils.h"

#include <coreplugin/icore.h>

#include <cppeditor/clangdiagnosticconfigsmodel.h>
#include <cppeditor/compileroptionsbuilder.h>
#include <cppeditor/cpptoolsreuse.h>

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

ClangToolRunner::ClangToolRunner(const AnalyzeInputData &input, QObject *parent)
    : QObject(parent)
    , m_input(input)
{
    m_name = input.tool == ClangToolType::Tidy ? tr("Clang-Tidy") : tr("Clazy");
    m_executable = toolExecutable(input.tool);
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
    if (!m_input.overlayFilePath.isEmpty() && isVFSOverlaySupported(m_executable))
        result << "--vfsoverlay=" + m_input.overlayFilePath;
    result << QDir::toNativeSeparators(m_input.unit.file);
    return result;
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
    QTC_ASSERT(m_executable.isExecutableFile(),
               qWarning() << "Can't start:" << m_executable << "as" << m_name; return false);
    QTC_CHECK(!m_input.unit.arguments.contains(QLatin1String("-o")));
    QTC_CHECK(!m_input.unit.arguments.contains(m_input.unit.file));
    QTC_ASSERT(FilePath::fromString(m_input.unit.file).exists(), return false);

    m_outputFilePath = createOutputFilePath(m_input.outputDirPath, m_input.unit.file);
    QTC_ASSERT(!m_outputFilePath.isEmpty(), return false);

    const QStringList args = checksArguments(m_input.tool, m_input.config)
                           + mainToolArguments()
                           + QStringList{"--"}
                           + clangArguments(m_input.config, m_input.unit.arguments);
    const CommandLine commandLine = {m_executable, args};

    qCDebug(LOG).noquote() << "Starting" << commandLine.toUserOutput();
    m_process.setCommand(commandLine);
    m_process.start();
    return true;
}

void ClangToolRunner::onProcessDone()
{
    if (m_process.result() == ProcessResult::FinishedWithSuccess) {
        qCDebug(LOG).noquote() << "Output:\n" << m_process.cleanedStdOut();
        emit done({true, m_input.unit.file, m_outputFilePath, m_name});
        return;
    }

    const QString details = tr("Command line: %1\n"
                               "Process Error: %2\n"
                               "Output:\n%3")
                                .arg(m_process.commandLine().toUserOutput())
                                .arg(m_process.error())
                                .arg(m_process.cleanedStdOut());
    QString message;
    if (m_process.result() == ProcessResult::StartFailed)
        message = tr("An error occurred with the %1 process.").arg(m_name);
    else if (m_process.result() == ProcessResult::FinishedWithError)
        message = tr("%1 finished with exit code: %2.").arg(m_name).arg(m_process.exitCode());
    else
        message = tr("%1 crashed.").arg(m_name);

    emit done({false, m_input.unit.file, m_outputFilePath, m_name, message, details});
}

} // namespace Internal
} // namespace ClangTools
