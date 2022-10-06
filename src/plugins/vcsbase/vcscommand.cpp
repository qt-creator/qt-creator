// Copyright (C) 2016 Brian McGillion and Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "vcscommand.h"

#include "vcsbaseplugin.h"
#include "vcsoutputwindow.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <utils/environment.h>
#include <utils/globalfilechangeblocker.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/runextensions.h>

#include <QFuture>
#include <QFutureWatcher>
#include <QMutex>
#include <QTextCodec>
#include <QThread>
#include <QVariant>

#include <numeric>

/*!
    \fn void Utils::ProgressParser::parseProgress(const QString &text)

    Reimplement to parse progress as it appears in the standard output.
    If a progress string is detected, call \c setProgressAndMaximum() to update
    the progress bar accordingly.

    \sa Utils::ProgressParser::setProgressAndMaximum()
*/

/*!
    \fn void Utils::ProgressParser::setProgressAndMaximum(int value, int maximum)

    Sets progress \a value and \a maximum for current command. Called by \c parseProgress()
    when a progress string is detected.
*/

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
        m_futureInterface.setProgressRange(0, 1);
    }

    ~VcsCommandPrivate() { delete m_progressParser; }

    Environment environment()
    {
        if (!(m_flags & RunFlags::ForceCLocale))
            return m_environment;

        m_environment.set("LANG", "C");
        m_environment.set("LANGUAGE", "C");
        return m_environment;
    }

    QString displayName() const;
    int timeoutS() const;

    void setup();
    void cleanup();
    void setupProcess(QtcProcess *process, const Job &job);
    void installStdCallbacks(QtcProcess *process);
    EventLoopMode eventLoopMode() const;
    void handleDone(QtcProcess *process);
    void startAll();
    void startNextJob();
    void processDone();

    VcsCommand *q = nullptr;

    QString m_displayName;
    const FilePath m_defaultWorkingDirectory;
    Environment m_environment;
    QTextCodec *m_codec = nullptr;
    ProgressParser *m_progressParser = nullptr;
    QFutureWatcher<void> m_watcher;
    QList<Job> m_jobs;

    int m_currentJob = 0;
    std::unique_ptr<QtcProcess> m_process;
    QString m_stdOut;
    QString m_stdErr;
    ProcessResult m_result = ProcessResult::StartFailed;
    QFutureInterface<void> m_futureInterface;

    RunFlags m_flags = RunFlags::None;
};

QString VcsCommandPrivate::displayName() const
{
    if (!m_displayName.isEmpty())
        return m_displayName;
    if (m_jobs.isEmpty())
        return tr("Unknown");
    const Job &job = m_jobs.at(0);
    QString result = job.command.executable().baseName();
    if (!result.isEmpty())
        result[0] = result.at(0).toTitleCase();
    else
        result = tr("UNKNOWN");
    if (!job.command.arguments().isEmpty())
        result += ' ' + job.command.splitArguments().at(0);
    return result;
}

int VcsCommandPrivate::timeoutS() const
{
    return std::accumulate(m_jobs.cbegin(), m_jobs.cend(), 0,
        [](int sum, const Job &job) { return sum + job.timeoutS; });
}

void VcsCommandPrivate::setup()
{
    m_futureInterface.reportStarted();
    if (m_flags & RunFlags::ExpectRepoChanges) {
        QMetaObject::invokeMethod(GlobalFileChangeBlocker::instance(), [] {
            GlobalFileChangeBlocker::instance()->forceBlocked(true);
        });
    }
    if (m_progressParser)
        m_progressParser->setFuture(&m_futureInterface);
}

void VcsCommandPrivate::cleanup()
{
    QTC_ASSERT(m_futureInterface.isRunning(), return);
    m_futureInterface.reportFinished();
    if (m_flags & RunFlags::ExpectRepoChanges) {
        QMetaObject::invokeMethod(GlobalFileChangeBlocker::instance(), [] {
            GlobalFileChangeBlocker::instance()->forceBlocked(false);
        });
    }
    if (m_progressParser)
        m_progressParser->setFuture(nullptr);
}

void VcsCommandPrivate::setupProcess(QtcProcess *process, const Job &job)
{
    process->setExitCodeInterpreter(job.exitCodeInterpreter);
    // TODO: Handle it properly in QtcProcess when QtcProcess::runBlocking() isn't used.
    process->setTimeoutS(job.timeoutS);
    if (!job.workingDirectory.isEmpty())
        process->setWorkingDirectory(job.workingDirectory);
    if (!(m_flags & RunFlags::SuppressCommandLogging))
        emit q->appendCommand(job.workingDirectory, job.command);
    process->setCommand(job.command);
    process->setDisableUnixTerminal();
    process->setEnvironment(environment());
    if (m_flags & RunFlags::MergeOutputChannels)
        process->setProcessChannelMode(QProcess::MergedChannels);
    if (m_codec)
        process->setCodec(m_codec);

    installStdCallbacks(process);
}

void VcsCommandPrivate::installStdCallbacks(QtcProcess *process)
{
    if (!(m_flags & RunFlags::MergeOutputChannels) && (m_flags & RunFlags::ProgressiveOutput
                                                  || !(m_flags & RunFlags::SuppressStdErr))) {
        process->setStdErrCallback([this](const QString &text) {
            if (m_progressParser)
                m_progressParser->parseProgress(text);
            if (!(m_flags & RunFlags::SuppressStdErr))
                emit q->appendError(text);
            if (m_flags & RunFlags::ProgressiveOutput)
                emit q->stdErrText(text);
        });
    }
    // connect stdout to the output window if desired
    if (m_progressParser || m_flags & RunFlags::ProgressiveOutput
                         || m_flags & RunFlags::ShowStdOut) {
        process->setStdOutCallback([this](const QString &text) {
            if (m_progressParser)
                m_progressParser->parseProgress(text);
            if (m_flags & RunFlags::ShowStdOut) {
                if (m_flags & RunFlags::SilentOutput)
                    emit q->appendSilently(text);
                else
                    emit q->append(text);
            }
            if (m_flags & RunFlags::ProgressiveOutput)
                emit q->stdOutText(text);
        });
    }
    // TODO: Implement it here
//    m_process->setTimeOutMessageBoxEnabled(true);
}

EventLoopMode VcsCommandPrivate::eventLoopMode() const
{
    if ((m_flags & RunFlags::UseEventLoop) && QThread::currentThread() == qApp->thread())
        return EventLoopMode::On;
    return EventLoopMode::Off;
}

void VcsCommandPrivate::handleDone(QtcProcess *process)
{
    // Success/Fail message in appropriate window?
    if (process->result() == ProcessResult::FinishedWithSuccess) {
        if (m_flags & RunFlags::ShowSuccessMessage)
            emit q->appendMessage(process->exitMessage());
    } else if (!(m_flags & RunFlags::SuppressFailMessage)) {
        emit q->appendError(process->exitMessage());
    }
    emit q->runCommandFinished(process->workingDirectory());
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
    m_process.reset(new QtcProcess);
    connect(m_process.get(), &QtcProcess::done, this, &VcsCommandPrivate::processDone);
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
    if (!success)
        m_futureInterface.reportCanceled();
    cleanup();
    // As it is used asynchronously, we need to delete ourselves
    q->deleteLater();
}

} // namespace Internal

VcsCommand::VcsCommand(const FilePath &workingDirectory, const Environment &environment) :
    d(new Internal::VcsCommandPrivate(this, workingDirectory, environment))
{
    connect(&d->m_watcher, &QFutureWatcher<void>::canceled, this, &VcsCommand::cancel);

    VcsOutputWindow::setRepository(d->m_defaultWorkingDirectory);
    VcsOutputWindow *outputWindow = VcsOutputWindow::instance(); // Keep me here, just to be sure it's not instantiated in other thread
    connect(this, &VcsCommand::append, outputWindow, [outputWindow](const QString &t) {
        outputWindow->append(t);
    });
    connect(this, &VcsCommand::appendSilently, outputWindow, &VcsOutputWindow::appendSilently);
    connect(this, &VcsCommand::appendError, outputWindow, &VcsOutputWindow::appendError);
    connect(this, &VcsCommand::appendCommand, outputWindow, &VcsOutputWindow::appendCommand);
    connect(this, &VcsCommand::appendMessage, outputWindow, &VcsOutputWindow::appendMessage);
    const auto connection = connect(this, &VcsCommand::runCommandFinished,
                                    this, &VcsCommand::postRunCommand);
    connect(ICore::instance(), &ICore::coreAboutToClose, this, [this, connection] {
        disconnect(connection);
        d->m_process.reset();
        if (d->m_futureInterface.isRunning()) {
            d->m_futureInterface.reportCanceled();
            d->cleanup();
        }
    });
}

void VcsCommand::postRunCommand(const FilePath &workingDirectory)
{
    if (!(d->m_flags & RunFlags::ExpectRepoChanges))
        return;
    // TODO tell the document manager that the directory now received all expected changes
    // Core::DocumentManager::unexpectDirectoryChange(d->m_workingDirectory);
    VcsManager::emitRepositoryChanged(workingDirectory);
}

VcsCommand::~VcsCommand()
{
    if (d->m_futureInterface.isRunning()) {
        d->m_futureInterface.reportCanceled();
        d->cleanup();
    }
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
    d->m_watcher.setFuture(d->m_futureInterface.future());
    if ((d->m_flags & RunFlags::SuppressCommandLogging))
        return;

    const QString name = d->displayName();
    const auto id = Id::fromString(name + QLatin1String(".action"));
    if (d->m_progressParser)
        ProgressManager::addTask(d->m_futureInterface.future(), name, id);
    else
        ProgressManager::addTimedTask(d->m_futureInterface, name, id, qMax(2, d->timeoutS() / 5));
}

void VcsCommand::cancel()
{
    d->m_futureInterface.reportCanceled();
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
    QtcProcess process;
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

//! Use \a parser to parse progress data from stdout. Command takes ownership of \a parser
void VcsCommand::setProgressParser(ProgressParser *parser)
{
    QTC_ASSERT(!d->m_progressParser, return);
    d->m_progressParser = parser;
}

ProgressParser::ProgressParser() :
    m_futureMutex(new QMutex)
{ }

ProgressParser::~ProgressParser()
{
    delete m_futureMutex;
}

void ProgressParser::setProgressAndMaximum(int value, int maximum)
{
    QMutexLocker lock(m_futureMutex);
    if (!m_future)
        return;
    m_future->setProgressRange(0, maximum);
    m_future->setProgressValue(value);
}

void ProgressParser::setFuture(QFutureInterface<void> *future)
{
    QMutexLocker lock(m_futureMutex);
    m_future = future;
}

CommandResult::CommandResult(const QtcProcess &process)
    : m_result(process.result())
    , m_exitCode(process.exitCode())
    , m_exitMessage(process.exitMessage())
    , m_cleanedStdOut(process.cleanedStdOut())
    , m_cleanedStdErr(process.cleanedStdErr())
    , m_rawStdOut(process.rawStdOut())
{}

} // namespace VcsBase
