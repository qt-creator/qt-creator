// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtoolrunner.h"

#include "clangtoolstr.h"
#include "clangtoolsutils.h"

#include <coreplugin/icore.h>

#include <cppeditor/clangdiagnosticconfigsmodel.h>
#include <cppeditor/cpptoolsreuse.h>

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
using namespace Tasking;

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
    file = fileInfo.file;
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

static QString createOutputFilePath(const FilePath &dirPath, const FilePath &fileToAnalyze)
{
    const QString fileName = fileToAnalyze.fileName();
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

TaskItem clangToolTask(const AnalyzeInputData &input,
                       const AnalyzeSetupHandler &setupHandler,
                       const AnalyzeOutputHandler &outputHandler)
{
    struct ClangToolStorage {
        QString name;
        Utils::FilePath executable;
        QString outputFilePath;
    };
    const TreeStorage<ClangToolStorage> storage;

    const auto mainToolArguments = [=](const ClangToolStorage *data)
    {
        QStringList result;
        result << "-export-fixes=" + data->outputFilePath;
        if (!input.overlayFilePath.isEmpty() && isVFSOverlaySupported(data->executable))
            result << "--vfsoverlay=" + input.overlayFilePath;
        result << input.unit.file.nativePath();
        return result;
    };

    const auto onGroupSetup = [=] {
        if (setupHandler && !setupHandler())
            return TaskAction::StopWithError;

        ClangToolStorage *data = storage.activeStorage();
        data->name = clangToolName(input.tool);
        data->executable = toolExecutable(input.tool);
        if (!data->executable.isExecutableFile()) {
            qWarning() << "Can't start:" << data->executable << "as" << data->name;
            return TaskAction::StopWithError;
        }

        QTC_CHECK(!input.unit.arguments.contains(QLatin1String("-o")));
        QTC_CHECK(!input.unit.arguments.contains(input.unit.file.nativePath()));
        QTC_ASSERT(input.unit.file.exists(), return TaskAction::StopWithError);
        data->outputFilePath = createOutputFilePath(input.outputDirPath, input.unit.file);
        QTC_ASSERT(!data->outputFilePath.isEmpty(), return TaskAction::StopWithError);

        return TaskAction::Continue;
    };
    const auto onProcessSetup = [=](QtcProcess &process) {
        process.setEnvironment(input.environment);
        process.setUseCtrlCStub(true);
        process.setLowPriority();
        process.setWorkingDirectory(input.outputDirPath); // Current clang-cl puts log file into working dir.

        const ClangToolStorage *data = storage.activeStorage();

        const QStringList args = checksArguments(input.tool, input.config)
                                 + mainToolArguments(data)
                                 + QStringList{"--"}
                                 + clangArguments(input.config, input.unit.arguments);
        const CommandLine commandLine = {data->executable, args};

        qCDebug(LOG).noquote() << "Starting" << commandLine.toUserOutput();
        process.setCommand(commandLine);
    };
    const auto onProcessDone = [=](const QtcProcess &process) {
        qCDebug(LOG).noquote() << "Output:\n" << process.cleanedStdOut();
        if (!outputHandler)
            return;
        const ClangToolStorage *data = storage.activeStorage();
        outputHandler({true, input.unit.file, data->outputFilePath, input.tool});
    };
    const auto onProcessError = [=](const QtcProcess &process) {
        if (!outputHandler)
            return;
        const QString details = Tr::tr("Command line: %1\nProcess Error: %2\nOutput:\n%3")
                                    .arg(process.commandLine().toUserOutput())
                                    .arg(process.error())
                                    .arg(process.cleanedStdOut());
        const ClangToolStorage *data = storage.activeStorage();
        QString message;
        if (process.result() == ProcessResult::StartFailed)
            message = Tr::tr("An error occurred with the %1 process.").arg(data->name);
        else if (process.result() == ProcessResult::FinishedWithError)
            message = Tr::tr("%1 finished with exit code: %2.").arg(data->name).arg(process.exitCode());
        else
            message = Tr::tr("%1 crashed.").arg(data->name);
        outputHandler({false, input.unit.file, data->outputFilePath, input.tool, message, details});
    };

    const Group group {
        Storage(storage),
        OnGroupSetup(onGroupSetup),
        Group {
            optional,
            Process(onProcessSetup, onProcessDone, onProcessError)
        }
    };
    return group;
}

} // namespace Internal
} // namespace ClangTools
