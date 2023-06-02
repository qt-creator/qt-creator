// Copyright (C) 2016 Brian McGillion and Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcscommand.h"

#include "vcsbaseplugin.h"
#include "vcsoutputwindow.h"

#include <coreplugin/icore.h>

#include <utils/environment.h>
#include <utils/globalfilechangeblocker.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/threadutils.h>

#include <QTextCodec>

using namespace Core;
using namespace Utils;

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

    int timeoutS() const;

    void setup();
    void cleanup();
    void setupProcess(Process *process, const Job &job);
    void installStdCallbacks(Process *process);
    EventLoopMode eventLoopMode() const;
    void handleDone(Process *process);
    void startAll();
    void startNextJob();
    void processDone();

    VcsCommand *q = nullptr;

    QString m_displayName;
    const FilePath m_defaultWorkingDirectory;
    Environment m_environment;
    QTextCodec *m_codec = nullptr;
    ProgressParser m_progressParser = {};
    QList<Job> m_jobs;

    int m_currentJob = 0;
    std::unique_ptr<Process> m_process;
    QString m_stdOut;
    QString m_stdErr;
    ProcessResult m_result = ProcessResult::StartFailed;

    RunFlags m_flags = RunFlags::None;
};

int VcsCommandPrivate::timeoutS() const
{
    return std::accumulate(m_jobs.cbegin(), m_jobs.cend(), 0,
        [](int sum, const Job &job) { return sum + job.timeoutS; });
}

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
    process->setExitCodeInterpreter(job.exitCodeInterpreter);
    process->setTimeoutS(job.timeoutS);
    if (!job.workingDirectory.isEmpty())
        process->setWorkingDirectory(job.workingDirectory);
    if (!(m_flags & RunFlags::SuppressCommandLogging))
        VcsOutputWindow::appendCommand(job.workingDirectory, job.command);
    process->setCommand(job.command);
    process->setDisableUnixTerminal();
    process->setEnvironment(environment());
    if (m_flags & RunFlags::MergeOutputChannels)
        process->setProcessChannelMode(QProcess::MergedChannels);
    if (m_codec)
        process->setCodec(m_codec);
    process->setUseCtrlCStub(true);

    installStdCallbacks(process);

    if (m_flags & RunFlags::SuppressCommandLogging)
        return;

    ProcessProgress *progress = new ProcessProgress(process);
    progress->setDisplayName(m_displayName);
    if (m_progressParser)
        progress->setProgressParser(m_progressParser);
}

void VcsCommandPrivate::installStdCallbacks(Process *process)
{
    if (!(m_flags & RunFlags::MergeOutputChannels) && (m_flags & RunFlags::ProgressiveOutput
                              || m_progressParser || !(m_flags & RunFlags::SuppressStdErr))) {
        process->setTextChannelMode(Channel::Error, TextChannelMode::MultiLine);
        connect(process, &Process::textOnStandardError, this, [this](const QString &text) {
            if (!(m_flags & RunFlags::SuppressStdErr))
                VcsOutputWindow::appendError(text);
            if (m_flags & RunFlags::ProgressiveOutput)
                emit q->stdErrText(text);
        });
    }
    if (m_progressParser || m_flags & RunFlags::ProgressiveOutput
                         || m_flags & RunFlags::ShowStdOut) {
        process->setTextChannelMode(Channel::Output, TextChannelMode::MultiLine);
        connect(process, &Process::textOnStandardOutput, this, [this](const QString &text) {
            if (m_flags & RunFlags::ShowStdOut)
                VcsOutputWindow::append(text);
            if (m_flags & RunFlags::ProgressiveOutput)
                emit q->stdOutText(text);
        });
    }
}

EventLoopMode VcsCommandPrivate::eventLoopMode() const
{
    if ((m_flags & RunFlags::UseEventLoop) && isMainThread())
        return EventLoopMode::On;
    return EventLoopMode::Off;
}

void VcsCommandPrivate::handleDone(Process *process)
{
    // Success/Fail message in appropriate window?
    if (process->result() == ProcessResult::FinishedWithSuccess) {
        if (m_flags & RunFlags::ShowSuccessMessage)
            VcsOutputWindow::appendMessage(process->exitMessage());
    } else if (!(m_flags & RunFlags::SuppressFailMessage)) {
        VcsOutputWindow::appendError(process->exitMessage());
    }
    if (!(m_flags & RunFlags::ExpectRepoChanges))
        return;
    // TODO tell the document manager that the directory now received all expected changes
    // Core::DocumentManager::unexpectDirectoryChange(d->m_workingDirectory);
    VcsManager::emitRepositoryChanged(process->workingDirectory());
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
    handleDone(m_process.get());
    m_stdOut += m_process->cleanedStdOut();
    m_stdErr += m_process->cleanedStdErr();
    m_result = m_process->result();
    ++m_currentJob;
    const bool success = m_process->result() == ProcessResult::FinishedWithSuccess;
    if (m_currentJob < m_jobs.count() && success) {
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
    VcsOutputWindow::setRepository(d->m_defaultWorkingDirectory);
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

CommandResult VcsCommand::runBlocking(const Utils::FilePath &workingDirectory,
                                      const Utils::Environment &environment,
                                      const Utils::CommandLine &command, RunFlags flags,
                                      int timeoutS, QTextCodec *codec)
{
    VcsCommand vcsCommand(workingDirectory, environment);
    vcsCommand.addFlags(flags);
    vcsCommand.setCodec(codec);
    return vcsCommand.runBlockingHelper(command, timeoutS);
}

CommandResult VcsCommand::runBlockingHelper(const CommandLine &command, int timeoutS)
{
    Process process;
    if (command.executable().isEmpty())
        return {};

    d->setupProcess(&process, {command, timeoutS, d->m_defaultWorkingDirectory, {}});

    const EventLoopMode eventLoopMode = d->eventLoopMode();
    process.setTimeOutMessageBoxEnabled(eventLoopMode == EventLoopMode::On);
    process.runBlocking(eventLoopMode);
    d->handleDone(&process);

    return CommandResult(process);
}

void VcsCommand::setCodec(QTextCodec *codec)
{
    d->m_codec = codec;
}

void VcsCommand::setProgressParser(const ProgressParser &parser)
{
    d->m_progressParser = parser;
}

CommandResult::CommandResult(const Process &process)
    : m_result(process.result())
    , m_exitCode(process.exitCode())
    , m_exitMessage(process.exitMessage())
    , m_cleanedStdOut(process.cleanedStdOut())
    , m_cleanedStdErr(process.cleanedStdErr())
    , m_rawStdOut(process.rawStdOut())
{}

CommandResult::CommandResult(const VcsCommand &command)
    : m_result(command.result())
    , m_cleanedStdOut(command.cleanedStdOut())
    , m_cleanedStdErr(command.cleanedStdErr())
{}

} // namespace VcsBase
