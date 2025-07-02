// Copyright (C) 2016 Brian McGillion and Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcscommand.h"

#include "vcsbaseplugin.h"
#include "vcsoutputwindow.h"

#include <coreplugin/icore.h>

#include <utils/environment.h>
#include <utils/globalfilechangeblocker.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/threadutils.h>

using namespace Core;
using namespace Tasking;
using namespace Utils;

using namespace std::chrono;

namespace VcsBase {
namespace Internal {

class VcsCommandPrivate : public QObject
{
public:
    struct Job {
        CommandLine command;
        int timeoutS = 10;
        FilePath workingDirectory;
        ExitCodeInterpreter exitCodeInterpreter = {};
    };

    VcsCommandPrivate(VcsCommand *vcsCommand, const FilePath &defaultWorkingDirectory,
                      const Environment &environment)
        : q(vcsCommand)
        , m_defaultWorkingDirectory(defaultWorkingDirectory)
        , m_environment(environment)
    {
        VcsBase::setProcessEnvironment(&m_environment);
    }

    Environment environment()
    {
        if (!(m_flags & RunFlags::ForceCLocale))
            return m_environment;

        m_environment.set("LANG", "C");
        m_environment.set("LANGUAGE", "C");
        return m_environment;
    }

    void setup();
    void cleanup();
    void setupProcess(Process *process, const Job &job);
    void installStdCallbacks(Process *process);
    EventLoopMode eventLoopMode() const;
    ProcessResult handleDone(Process *process, const Job &job) const;
    void startAll();
    void startNextJob();
    void processDone();

    VcsCommand *q = nullptr;

    QString m_displayName;
    const FilePath m_defaultWorkingDirectory;
    Environment m_environment;
    TextEncoding m_encoding;
    ProgressParser m_progressParser = {};
    TextChannelCallback m_stdOutCallback = {};
    TextChannelCallback m_stdErrCallback = {};
    QList<Job> m_jobs;

    int m_currentJob = 0;
    std::unique_ptr<Process> m_process;
    QString m_stdOut;
    QString m_stdErr;
    ProcessResult m_result = ProcessResult::StartFailed;

    RunFlags m_flags = RunFlags::None;
};

void VcsCommandPrivate::setup()
{
    if (m_flags & RunFlags::ExpectRepoChanges)
        GlobalFileChangeBlocker::instance()->forceBlocked(true);
}

void VcsCommandPrivate::cleanup()
{
    if (m_flags & RunFlags::ExpectRepoChanges)
        GlobalFileChangeBlocker::instance()->forceBlocked(false);
}

void VcsCommandPrivate::setupProcess(Process *process, const Job &job)
{
    if (!job.workingDirectory.isEmpty())
        process->setWorkingDirectory(job.workingDirectory);
    if (!(m_flags & RunFlags::SuppressCommandLogging))
        VcsOutputWindow::appendCommand(job.workingDirectory, job.command);
    process->setCommand(job.command);
    process->setDisableUnixTerminal();
    process->setEnvironment(environment());
    if (m_flags & RunFlags::MergeOutputChannels)
        process->setProcessChannelMode(QProcess::MergedChannels);
    if (m_encoding.isValid())
        process->setEncoding(m_encoding);
    process->setUseCtrlCStub(true);

    installStdCallbacks(process);

    if (m_flags & RunFlags::SuppressCommandLogging)
        return;

    ProcessProgress *progress = new ProcessProgress(process);
    progress->setDisplayName(m_displayName);
    progress->setExpectedDuration(seconds(qMin(1, job.timeoutS / 5)));
    if (m_progressParser)
        progress->setProgressParser(m_progressParser);
}

void VcsCommandPrivate::installStdCallbacks(Process *process)
{
    if (!(m_flags & RunFlags::MergeOutputChannels) && (m_stdErrCallback || m_progressParser
                                                       || !(m_flags & RunFlags::SuppressStdErr))) {
        process->setTextChannelMode(Channel::Error, TextChannelMode::MultiLine);
        connect(process, &Process::textOnStandardError, this, [this](const QString &text) {
            if (!(m_flags & RunFlags::SuppressStdErr))
                VcsOutputWindow::appendError(m_defaultWorkingDirectory, text);
            if (m_stdErrCallback)
                m_stdErrCallback(text);
        });
    }
    if (m_progressParser || m_stdOutCallback || m_flags & RunFlags::ShowStdOut) {
        process->setTextChannelMode(Channel::Output, TextChannelMode::MultiLine);
        connect(process, &Process::textOnStandardOutput, this, [this](const QString &text) {
            if (m_flags & RunFlags::ShowStdOut)
                VcsOutputWindow::appendSilently(m_defaultWorkingDirectory, text);
            if (m_stdOutCallback)
                m_stdOutCallback(text);
        });
    }
}

EventLoopMode VcsCommandPrivate::eventLoopMode() const
{
    if ((m_flags & RunFlags::UseEventLoop) && isMainThread())
        return EventLoopMode::On;
    return EventLoopMode::Off;
}

ProcessResult VcsCommandPrivate::handleDone(Process *process, const Job &job) const
{
    ProcessResult result;
    if (job.exitCodeInterpreter && process->error() != QProcess::FailedToStart
        && process->exitStatus() == QProcess::NormalExit) {
        result = job.exitCodeInterpreter(process->exitCode());
    } else {
        result = process->result();
    }
    const Utils::FilePath workingDirectory = process->workingDirectory();
    const QString message = Process::exitMessage(process->commandLine(), result,
                                                 process->exitCode(), process->processDuration());
    // Success/Fail message in appropriate window?
    if (result == ProcessResult::FinishedWithSuccess) {
        if (m_flags & RunFlags::ShowSuccessMessage)
            VcsOutputWindow::appendMessage(workingDirectory, message);
    } else if (!(m_flags & RunFlags::SuppressFailMessage)) {
        VcsOutputWindow::appendError(workingDirectory, message);
    }
    if (m_flags & RunFlags::ExpectRepoChanges) {
        // TODO tell the document manager that the directory now received all expected changes
        // DocumentManager::unexpectDirectoryChange(workingDirectory);
        VcsManager::emitRepositoryChanged(workingDirectory);
    }
    return result;
}

void VcsCommandPrivate::startAll()
{
    // Check that the binary path is not empty
    QTC_ASSERT(!m_jobs.isEmpty(), return);
    QTC_ASSERT(!m_process, return);
    setup();
    m_currentJob = 0;
    startNextJob();
}

void VcsCommandPrivate::startNextJob()
{
    QTC_ASSERT(m_currentJob < m_jobs.count(), return);
    m_process.reset(new Process);
    connect(m_process.get(), &Process::done, this, &VcsCommandPrivate::processDone);
    setupProcess(m_process.get(), m_jobs.at(m_currentJob));
    m_process->start();
}

void VcsCommandPrivate::processDone()
{
    m_result = handleDone(m_process.get(), m_jobs.at(m_currentJob));
    m_stdOut += m_process->cleanedStdOut();
    m_stdErr += m_process->cleanedStdErr();
    ++m_currentJob;
    if (m_currentJob < m_jobs.count() && m_result == ProcessResult::FinishedWithSuccess) {
        m_process.release()->deleteLater();
        startNextJob();
        return;
    }
    emit q->done();
    cleanup();
    q->deleteLater();
}

} // namespace Internal

VcsCommand::VcsCommand(const FilePath &workingDirectory, const Environment &environment) :
    d(new Internal::VcsCommandPrivate(this, workingDirectory, environment))
{
    connect(ICore::instance(), &ICore::coreAboutToClose, this, [this] {
        if (d->m_process && d->m_process->isRunning())
            d->cleanup();
        d->m_process.reset();
    });
}

VcsCommand::~VcsCommand()
{
    if (d->m_process && d->m_process->isRunning())
        d->cleanup();
    delete d;
}

void VcsCommand::setDisplayName(const QString &name)
{
    d->m_displayName = name;
}

void VcsCommand::addFlags(RunFlags f)
{
    d->m_flags |= f;
}

void VcsCommand::addJob(const CommandLine &command, int timeoutS,
                        const FilePath &workingDirectory,
                        const ExitCodeInterpreter &interpreter)
{
    QTC_ASSERT(!command.executable().isEmpty(), return);
    d->m_jobs.push_back({command, timeoutS, !workingDirectory.isEmpty()
                         ? workingDirectory : d->m_defaultWorkingDirectory, interpreter});
}

void VcsCommand::start()
{
    if (d->m_jobs.empty())
        return;

    d->startAll();
}

void VcsCommand::cancel()
{
    if (d->m_process) {
        // TODO: we may want to call cancel here...
        d->m_process->stop();
        // TODO: we may want to not wait here...
        d->m_process->waitForFinished();
        d->m_process.reset();
    }
}

QString VcsCommand::cleanedStdOut() const
{
    return d->m_stdOut;
}

QString VcsCommand::cleanedStdErr() const
{
    return d->m_stdErr;
}

ProcessResult VcsCommand::result() const
{
    return d->m_result;
}

CommandResult VcsCommand::runBlocking(const FilePath &workingDirectory,
                                      const Environment &environment,
                                      const CommandLine &command, RunFlags flags,
                                      int timeoutS, const TextEncoding &codec)
{
    VcsCommand vcsCommand(workingDirectory, environment);
    vcsCommand.addFlags(flags);
    vcsCommand.setEncoding(codec);
    return vcsCommand.runBlockingHelper(command, timeoutS);
}

// TODO: change timeout to std::chrono::seconds
CommandResult VcsCommand::runBlockingHelper(const CommandLine &command, int timeoutS)
{
    Process process;
    if (command.executable().isEmpty())
        return {};

    const Internal::VcsCommandPrivate::Job job{command, timeoutS, d->m_defaultWorkingDirectory};
    d->setupProcess(&process, job);

    const EventLoopMode eventLoopMode = d->eventLoopMode();
    process.setTimeOutMessageBoxEnabled(eventLoopMode == EventLoopMode::On);
    process.runBlocking(seconds(timeoutS), eventLoopMode);
    d->handleDone(&process, job);

    return CommandResult(process);
}

void VcsCommand::setEncoding(const TextEncoding &encoding)
{
    d->m_encoding = encoding;
}

void VcsCommand::setProgressParser(const ProgressParser &parser)
{
    d->m_progressParser = parser;
}

void VcsCommand::setStdOutCallback(const Utils::TextChannelCallback &callback)
{
    d->m_stdOutCallback = callback;
}

void VcsCommand::setStdErrCallback(const Utils::TextChannelCallback &callback)
{
    d->m_stdErrCallback = callback;
}

CommandResult::CommandResult(const Process &process)
    : m_result(process.result())
    , m_exitCode(process.exitCode())
    , m_exitMessage(process.exitMessage())
    , m_cleanedStdOut(process.cleanedStdOut())
    , m_cleanedStdErr(process.cleanedStdErr())
    , m_rawStdOut(process.rawStdOut())
{}

CommandResult::CommandResult(const Process &process, ProcessResult result)
    : CommandResult(process)
{
    m_result = result;
}

CommandResult::CommandResult(const VcsCommand &command)
    : m_result(command.result())
    , m_cleanedStdOut(command.cleanedStdOut())
    , m_cleanedStdErr(command.cleanedStdErr())
{}

ProcessTask vcsProcessTask(const VcsProcessData &data,
                           const std::optional<Storage<CommandResult>> &resultStorage)
{
    const auto onSetup = [data](Process &process) {
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
                    VcsOutputWindow::appendSilently(workingDir, text);
                if (handler)
                    handler(text);
            });
        }

        if (data.flags & RunFlags::SuppressCommandLogging)
            return;

        ProcessProgress *progress = new ProcessProgress(&process);
        if (data.progressParser)
            progress->setProgressParser(data.progressParser);
    };
    const auto onDone = [data, resultStorage](const Process &process) {
        if (data.flags & RunFlags::ExpectRepoChanges)
            GlobalFileChangeBlocker::instance()->forceBlocked(true);
        ProcessResult result;
        if (data.interpreter && process.error() != QProcess::FailedToStart
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
        if (data.flags & RunFlags::ExpectRepoChanges) {
            // TODO tell the document manager that the directory now received all expected changes
            // DocumentManager::unexpectDirectoryChange(workingDirectory);
            VcsManager::emitRepositoryChanged(workingDirectory);
        }
        if (resultStorage)
            **resultStorage = CommandResult(process, result);
        return result == ProcessResult::FinishedWithSuccess;
    };
    return ProcessTask(onSetup, onDone);
}

} // namespace VcsBase
