// Copyright (C) 2016 Brian McGillion and Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcscommand.h"

#include "vcsbaseplugin.h"
#include "vcsoutputwindow.h"

#include <coreplugin/icore.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

#include <utils/environment.h>
#include <utils/globalfilechangeblocker.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/threadutils.h>

using namespace Core;
using namespace QtTaskTree;
using namespace Utils;

using namespace std::chrono;

namespace VcsBase {

CommandResult::CommandResult(const Process &process)
    : m_result(process.result())
    , m_exitCode(process.exitCode())
    , m_exitMessage(process.exitMessage())
    , m_cleanedStdOut(process.cleanedStdOut())
    , m_cleanedStdErr(process.cleanedStdErr())
    , m_rawStdOut(process.rawStdOut())
    , m_workingDirectory(process.workingDirectory())
{}

CommandResult::CommandResult(const Process &process, ProcessResult result)
    : CommandResult(process)
{
    m_result = result;
}

ExecutableItem errorTask(const FilePath &workingDir, const QString &errorMessage)
{
    return QSyncTask([workingDir, errorMessage] {
        VcsOutputWindow::appendError(workingDir, errorMessage);
        return false;
    });
}

enum class RunMode { Asynchronous, Blocking };

static ProcessTask vcsProcessTaskHelper(
    const VcsProcessData &data,
    const std::optional<Storage<CommandResult>> &resultStorage = {},
    const seconds timeout = seconds(10),
    RunMode runMode = RunMode::Asynchronous)
{
    const auto onDone = [data, resultStorage](const Process &process, DoneWith doneWith) {
        if (data.flags & RunFlags::ExpectRepoChanges)
            GlobalFileChangeBlocker::instance()->forceBlocked(false);
        ProcessResult result;
        if (doneWith == DoneWith::Cancel) {
            result = ProcessResult::Canceled;
        } else if (data.interpreter && process.error() != QProcess::FailedToStart
            && process.exitStatus() == QProcess::NormalExit) {
            result = data.interpreter(process.exitCode());
        } else {
            result = process.result();
        }

        const FilePath workingDirectory = process.workingDirectory();
        const QString message = Process::exitMessage(process.commandLine(), result,
                                                     process.exitCode(), process.processDuration());
        if (result == ProcessResult::FinishedWithSuccess) {
            if (data.flags & RunFlags::ShowSuccessMessage)
                VcsOutputWindow::appendMessage(workingDirectory, message);
        } else if (!(data.flags & RunFlags::SuppressFailMessage)) {
            VcsOutputWindow::appendError(workingDirectory, message);
        }
        if (data.flags & RunFlags::ExpectRepoChanges)
            VcsManager::emitRepositoryChanged(workingDirectory);
        if (resultStorage)
            **resultStorage = CommandResult(process, result);
        return result == ProcessResult::FinishedWithSuccess;
    };

    const auto onSetup = [data, onDone, timeout, runMode](Process &process) {
        Environment environment = data.runData.environment;
        VcsBase::setProcessEnvironment(&environment);
        if (data.flags & RunFlags::ForceCLocale) {
            environment.set("LANG", "C");
            environment.set("LANGUAGE", "C");
        }
        process.setEnvironment(environment);
        process.setWorkingDirectory(data.runData.workingDirectory);
        process.setCommand(data.runData.command);
        process.setDisableUnixTerminal();
        process.setUseCtrlCStub(true);

        if (data.flags & RunFlags::ExpectRepoChanges)
            GlobalFileChangeBlocker::instance()->forceBlocked(true);

        if (!(data.flags & RunFlags::SuppressCommandLogging))
            VcsOutputWindow::appendCommand(data.runData.workingDirectory, data.runData.command);

        if (data.flags & RunFlags::MergeOutputChannels)
            process.setProcessChannelMode(QProcess::MergedChannels);

        if (data.encoding.isValid())
            process.setEncoding(data.encoding);

        const bool installStdError = !(data.flags & RunFlags::MergeOutputChannels)
            && (data.stdErrHandler || data.progressParser || !(data.flags & RunFlags::SuppressStdErr));

        if (installStdError) {
            process.setTextChannelMode(Channel::Error, TextChannelMode::MultiLine);
            QObject::connect(&process, &Process::textOnStandardError, &process,
                             [flags = data.flags, workingDir = process.workingDirectory(),
                              handler = data.stdErrHandler](const QString &text) {
                if (!(flags & RunFlags::SuppressStdErr))
                    VcsOutputWindow::appendError(workingDir, text);
                if (handler)
                    handler(text);
            });
        }
        if (data.progressParser || data.stdOutHandler || data.flags & RunFlags::ShowStdOut) {
            process.setTextChannelMode(Channel::Output, TextChannelMode::MultiLine);
            QObject::connect(&process, &Process::textOnStandardOutput, &process,
                             [flags = data.flags, workingDir = process.workingDirectory(),
                              handler = data.stdOutHandler](const QString &text) {
                if (flags & RunFlags::ShowStdOut)
                    VcsOutputWindow::appendText(workingDir, text);
                if (handler)
                    handler(text);
            });
        }

        if (!(data.flags & RunFlags::SuppressCommandLogging)) {
            ProcessProgress *progress = new ProcessProgress(&process);
            if (data.progressParser)
                progress->setProgressParser(data.progressParser);
        }

        if (runMode == RunMode::Blocking) {
            process.runBlocking(timeout);
            DoneWith doneWith = DoneWith::Error;
            if (process.result() == ProcessResult::FinishedWithSuccess)
                doneWith = DoneWith::Success;
            else if (process.result() == ProcessResult::Canceled)
                doneWith = DoneWith::Cancel;
            const bool success = onDone(process, doneWith);
            return success ? SetupResult::StopWithSuccess : SetupResult::StopWithError;
        }
        return SetupResult::Continue;
    };
    return ProcessTask(onSetup, onDone);
}

ProcessTask vcsProcessTask(const VcsProcessData &data,
                           const std::optional<Storage<CommandResult>> &resultStorage)
{
    return vcsProcessTaskHelper(data, resultStorage);
}

CommandResult vcsRunBlocking(const VcsProcessData &data, const seconds timeout)
{
    CommandResult result;
    const Storage<CommandResult> resultStorage;

    const auto onDone = [resultStorage, &result] { result = *resultStorage; };

    const Group recipe {
        resultStorage,
        vcsProcessTaskHelper(data, resultStorage, timeout, RunMode::Blocking),
        onGroupDone(onDone)
    };

    QSingleTaskTreeRunner taskTreeRunner;
    taskTreeRunner.start(recipe);
    QTC_CHECK(!taskTreeRunner.isRunning());
    return result;
}

} // namespace VcsBase
