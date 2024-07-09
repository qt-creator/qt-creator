// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtoolrunner.h"

#include "clangtoolslogfilereader.h"
#include "clangtoolstr.h"
#include "clangtoolsutils.h"

#include <coreplugin/icore.h>

#include <cppeditor/clangdiagnosticconfigsmodel.h>
#include <cppeditor/compileroptionsbuilder.h>
#include <cppeditor/cppcodemodelsettings.h>
#include <cppeditor/cppprojectfile.h>
#include <cppeditor/cpptoolsreuse.h>

#include <utils/async.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
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
    arguments.append(
        optionsBuilder.build(fileInfo.kind,
                             CppCodeModelSettings(fileInfo.settings).usePrecompiledHeaders()));
    arguments.append(extraClangToolsAppendOptions());
}

static bool isClMode(const QStringList &options)
{
    return options.contains("--driver-mode=cl");
}

static QStringList checksArguments(const AnalyzeUnit &unit, const AnalyzeInputData &input)
{
    if (input.tool == ClangToolType::Tidy) {
        if (input.runSettings.hasConfigFileForSourceFile(unit.file))
            return {"--warnings-as-errors=-*", "-checks=-clang-diagnostic-*"};
        switch (input.config.clangTidyMode()) {
        case ClangDiagnosticConfig::TidyMode::UseDefaultChecks:
            // The argument "-config={}" stops stating/evaluating the .clang-tidy file.
            return {"-config={}", "-checks=-clang-diagnostic-*"};
        case ClangDiagnosticConfig::TidyMode::UseCustomChecks:
            return {"-config=" + input.config.clangTidyChecksAsJson()};
        }
    }
    const QString clazyChecks = input.config.checks(ClangToolType::Clazy);
    if (!clazyChecks.isEmpty())
        return {"-checks=" + input.config.checks(ClangToolType::Clazy)};
    return {};
}

static QStringList clangArguments(const AnalyzeUnit &unit, const AnalyzeInputData &input)
{
    QStringList arguments;
    const ClangDiagnosticConfig &diagnosticConfig = input.config;
    const QStringList &baseOptions = unit.arguments;
    arguments << ClangDiagnosticConfigsModel::globalDiagnosticOptions()
              << (isClMode(baseOptions) ? clangArgsForCl(diagnosticConfig.clangOptions())
                                        : diagnosticConfig.clangOptions())
              << baseOptions;
    if (ProjectFile::isHeader(unit.file))
        arguments << "-Wno-pragma-once-outside-header";

    if (LOG().isDebugEnabled())
        arguments << QLatin1String("-v");

    return arguments;
}

static FilePath createOutputFilePath(const FilePath &dirPath, const FilePath &fileToAnalyze)
{
    const QString fileName = fileToAnalyze.fileName();
    const FilePath fileTemplate = dirPath.pathAppended("report-" + fileName + "-XXXXXX");

    TemporaryFile temporaryFile("clangtools");
    temporaryFile.setAutoRemove(false);
    temporaryFile.setFileTemplate(fileTemplate.path());
    if (temporaryFile.open()) {
        temporaryFile.close();
        return FilePath::fromString(temporaryFile.fileName());
    }
    return {};
}

GroupItem clangToolTask(const AnalyzeUnits &units,
                        const AnalyzeInputData &input,
                        const AnalyzeSetupHandler &setupHandler,
                        const AnalyzeOutputHandler &outputHandler)
{
    struct ClangToolStorage {
        QString name;
        FilePath executable;
        FilePath outputFilePath;
    };
    const Storage<ClangToolStorage> storage;
    const LoopList iterator(units);

    const auto mainToolArguments = [input, iterator](const ClangToolStorage &data) {
        QStringList result;
        result << "-export-fixes=" + data.outputFilePath.nativePath();
        if (!input.overlayFilePath.isEmpty() && isVFSOverlaySupported(data.executable))
            result << "--vfsoverlay=" + input.overlayFilePath;
        result << iterator->file.nativePath();
        return result;
    };

    const auto onSetup = [storage, input, setupHandler, iterator] {
        const AnalyzeUnit &unit = *iterator;
        if (setupHandler && !setupHandler(unit))
            return SetupResult::StopWithError;

        ClangToolStorage *data = storage.activeStorage();
        data->name = clangToolName(input.tool);
        data->executable = toolExecutable(input.tool);
        if (!data->executable.isExecutableFile()) {
            qWarning() << "Can't start:" << data->executable << "as" << data->name;
            return SetupResult::StopWithError;
        }

        QTC_CHECK(!unit.arguments.contains(QLatin1String("-o")));
        QTC_CHECK(!unit.arguments.contains(unit.file.nativePath()));
        QTC_ASSERT(unit.file.exists(), return SetupResult::StopWithError);
        data->outputFilePath = createOutputFilePath(input.outputDirPath, unit.file);
        QTC_ASSERT(!data->outputFilePath.isEmpty(), return SetupResult::StopWithError);

        return SetupResult::Continue;
    };
    const auto onProcessSetup = [storage, input, mainToolArguments, iterator](Process &process) {
        const AnalyzeUnit &unit = *iterator;
        process.setEnvironment(input.environment);
        process.setUseCtrlCStub(true);
        process.setLowPriority();
        process.setWorkingDirectory(input.outputDirPath); // Current clang-cl puts log file into working dir.

        const ClangToolStorage &data = *storage;
        const CommandLine commandLine{data.executable, {checksArguments(unit, input),
                                                        mainToolArguments(data), "--",
                                                        clangArguments(unit, input)}};
        qCDebug(LOG).noquote() << "Starting" << commandLine.toUserOutput();
        process.setCommand(commandLine);
    };
    const auto onProcessDone = [storage, input, outputHandler, iterator](
                                   const Process &process, DoneWith result) {
        qCDebug(LOG).noquote() << "Output:\n" << process.cleanedStdOut();

        if (!outputHandler)
            return;
        const AnalyzeUnit &unit = *iterator;
        if (result == DoneWith::Success) {
            const QString stdErr = process.cleanedStdErr();
            if (stdErr.isEmpty())
                return;
            outputHandler({true, unit.file, {}, {}, input.tool,
                           Tr::tr("%1 produced stderr output:").arg(storage->name), stdErr});
            return;
        }
        const QString details = Tr::tr("Command line: %1\nProcess Error: %2\nOutput:\n%3")
                                    .arg(process.commandLine().toUserOutput())
                                    .arg(process.error())
                                    .arg(process.cleanedStdOut());
        const ClangToolStorage &data = *storage;
        QString message;
        if (process.result() == ProcessResult::StartFailed)
            message = Tr::tr("An error occurred with the %1 process.").arg(data.name);
        else if (process.result() == ProcessResult::FinishedWithError)
            message = Tr::tr("%1 finished with exit code: %2.").arg(data.name).arg(process.exitCode());
        else
            message = Tr::tr("%1 crashed.").arg(data.name);
        outputHandler(
            {false, unit.file, data.outputFilePath, {}, input.tool, message, details});
    };

    const auto onReadSetup = [storage, input](Async<expected_str<Diagnostics>> &data) {
        data.setConcurrentCallData(&parseDiagnostics,
                                   storage->outputFilePath,
                                   input.diagnosticsFilter);
    };
    const auto onReadDone = [storage, input, outputHandler, iterator](
                                const Async<expected_str<Diagnostics>> &data, DoneWith result) {
        if (!outputHandler)
            return;
        const expected_str<Diagnostics> diagnosticsResult = data.result();
        const bool ok = result == DoneWith::Success && diagnosticsResult.has_value();
        Diagnostics diagnostics;
        QString error;
        if (ok)
            diagnostics = *diagnosticsResult;
        else
            error = diagnosticsResult.error();
        outputHandler({ok,
                       iterator->file,
                       storage->outputFilePath,
                       diagnostics,
                       input.tool,
                       error});
    };

    return For {
        iterator,
        parallelLimit(qMax(1, input.runSettings.parallelJobs())),
        finishAllAndSuccess,
        Group {
            storage,
            onGroupSetup(onSetup),
            sequential,
            ProcessTask(onProcessSetup, onProcessDone),
            AsyncTask<expected_str<Diagnostics>>(onReadSetup, onReadDone)
        }
    };
}

} // namespace Internal
} // namespace ClangTools
