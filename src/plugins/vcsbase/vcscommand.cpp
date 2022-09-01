// Copyright (C) 2016 Brian McGillion and Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "vcscommand.h"

#include "vcsbaseplugin.h"
#include "vcsoutputwindow.h"
#include "vcsplugin.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/futureprogress.h>
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
    }

    ~VcsCommandPrivate() { delete m_progressParser; }

    Environment environment()
    {
        if (!(m_flags & VcsCommand::ForceCLocale))
            return m_environment;

        m_environment.set("LANG", "C");
        m_environment.set("LANGUAGE", "C");
        return m_environment;
    }

    QString displayName() const;
    int timeoutS() const;

    void setupProcess(QtcProcess *process, const Job &job);
    void setupSynchronous(QtcProcess *process);
    bool isFullySynchronous() const;
    void handleDone(QtcProcess *process);

    VcsCommand *q = nullptr;

    QString m_displayName;
    const FilePath m_defaultWorkingDirectory;
    Environment m_environment;
    QTextCodec *m_codec = nullptr;
    ProgressParser *m_progressParser = nullptr;
    QFutureWatcher<void> m_watcher;
    FutureProgress *m_futureProgress = nullptr;
    QList<Job> m_jobs;

    unsigned m_flags = 0;

    bool m_progressiveOutput = false;
    bool m_aborted = false;
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

void VcsCommandPrivate::setupProcess(QtcProcess *process, const Job &job)
{
    process->setExitCodeInterpreter(job.exitCodeInterpreter);
    // TODO: Handle it properly in QtcProcess when QtcProcess::runBlocking() isn't used.
    process->setTimeoutS(job.timeoutS);
    if (!job.workingDirectory.isEmpty())
        process->setWorkingDirectory(job.workingDirectory);
    if (!(m_flags & VcsCommand::SuppressCommandLogging))
        emit q->appendCommand(job.workingDirectory, job.command);
    process->setCommand(job.command);
    process->setDisableUnixTerminal();
    process->setEnvironment(environment());
    if (m_flags & VcsCommand::MergeOutputChannels)
        process->setProcessChannelMode(QProcess::MergedChannels);
    if (m_codec)
        process->setCodec(m_codec);
}

void VcsCommandPrivate::setupSynchronous(QtcProcess *process)
{
    if (!(m_flags & VcsCommand::MergeOutputChannels)
            && (m_progressiveOutput || !(m_flags & VcsCommand::SuppressStdErr))) {
        process->setStdErrCallback([this](const QString &text) {
            if (m_progressParser)
                m_progressParser->parseProgress(text);
            if (!(m_flags & VcsCommand::SuppressStdErr))
                emit q->appendError(text);
            if (m_progressiveOutput)
                emit q->stdErrText(text);
        });
    }
    // connect stdout to the output window if desired
    if (m_progressParser || m_progressiveOutput || (m_flags & VcsCommand::ShowStdOut)) {
        process->setStdOutCallback([this](const QString &text) {
            if (m_progressParser)
                m_progressParser->parseProgress(text);
            if (m_flags & VcsCommand::ShowStdOut)
                emit q->append(text);
            if (m_progressiveOutput)
                emit q->stdOutText(text);
        });
    }
    // TODO: Implement it here
//    m_process->setTimeOutMessageBoxEnabled(true);
}

bool VcsCommandPrivate::isFullySynchronous() const
{
    return (m_flags & VcsCommand::FullySynchronously) || (!(m_flags & VcsCommand::NoFullySync)
            && QThread::currentThread() == QCoreApplication::instance()->thread());
}

void VcsCommandPrivate::handleDone(QtcProcess *process)
{
    if (!m_aborted) {
        // Success/Fail message in appropriate window?
        if (process->result() == ProcessResult::FinishedWithSuccess) {
            if (m_flags & VcsCommand::ShowSuccessMessage)
                emit q->appendMessage(process->exitMessage());
        } else if (!(m_flags & VcsCommand::SuppressFailMessage)) {
            emit q->appendError(process->exitMessage());
        }
    }
    emit q->runCommandFinished(process->workingDirectory());
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
        abort();
    });
}

void VcsCommand::addTask(const QFuture<void> &future)
{
    if ((d->m_flags & VcsCommand::SuppressCommandLogging))
        return;

    const QString name = d->displayName();
    const auto id = Id::fromString(name + QLatin1String(".action"));
    d->m_futureProgress = ProgressManager::addTask(future, name, id);
    Internal::VcsPlugin::addFuture(future);
}

void VcsCommand::postRunCommand(const FilePath &workingDirectory)
{
    if (!(d->m_flags & VcsCommand::ExpectRepoChanges))
        return;
    // TODO tell the document manager that the directory now received all expected changes
    // Core::DocumentManager::unexpectDirectoryChange(d->m_workingDirectory);
    VcsManager::emitRepositoryChanged(workingDirectory);
}

VcsCommand::~VcsCommand()
{
    if (!d->m_watcher.future().isFinished())
        d->m_watcher.future().cancel();
    delete d;
}

void VcsCommand::setDisplayName(const QString &name)
{
    d->m_displayName = name;
}

void VcsCommand::addFlags(unsigned f)
{
    d->m_flags |= f;
}

void VcsCommand::addJob(const CommandLine &command, int timeoutS,
                        const FilePath &workingDirectory,
                        const ExitCodeInterpreter &interpreter)
{
    d->m_jobs.push_back({command, timeoutS, !workingDirectory.isEmpty()
                         ? workingDirectory : d->m_defaultWorkingDirectory, interpreter});
}

void VcsCommand::execute()
{
    if (d->m_jobs.empty())
        return;

    QFuture<void> task = runAsync(&VcsCommand::run, this);
    d->m_watcher.setFuture(task);
    addTask(task);
}

void VcsCommand::abort()
{
    d->m_aborted = true;
    d->m_watcher.future().cancel();
}

void VcsCommand::cancel()
{
    emit terminate();
}

void VcsCommand::run(QFutureInterface<void> &future)
{
    // Check that the binary path is not empty
    QTC_ASSERT(!d->m_jobs.isEmpty(), return);

    QString stdOut;
    QString stdErr;

    if (d->m_flags & VcsCommand::ExpectRepoChanges) {
        QMetaObject::invokeMethod(this, [] {
            GlobalFileChangeBlocker::instance()->forceBlocked(true);
        });
    }
    if (d->m_progressParser) {
        d->m_progressParser->setFuture(&future);
    } else {
        QMetaObject::invokeMethod(this, [this, future] {
            (void) new ProgressTimer(future, qMax(2, d->timeoutS() / 5), d->m_futureProgress);
        });
    }

    const int count = d->m_jobs.size();
    bool lastExecSuccess = true;
    for (int j = 0; j < count; j++) {
        const Internal::VcsCommandPrivate::Job &job = d->m_jobs.at(j);
        const CommandResult result = runCommand(job.command, job.timeoutS,
                                                job.workingDirectory, job.exitCodeInterpreter);
        stdOut += result.cleanedStdOut();
        stdErr += result.cleanedStdErr();
        lastExecSuccess = result.result() == ProcessResult::FinishedWithSuccess;
        if (!lastExecSuccess)
            break;
    }

    if (!d->m_aborted) {
        if (!d->m_progressiveOutput) {
            emit stdOutText(stdOut);
            if (!stdErr.isEmpty())
                emit stdErrText(stdErr);
        }

        if (d->m_flags & VcsCommand::ExpectRepoChanges) {
            QMetaObject::invokeMethod(this, [] {
                GlobalFileChangeBlocker::instance()->forceBlocked(false);
            });
        }
        emit finished(lastExecSuccess);
        if (lastExecSuccess)
            future.setProgressValue(future.progressMaximum());
        else
            future.cancel(); // sets the progress indicator red
    }

    if (d->m_progressParser)
        d->m_progressParser->setFuture(nullptr);
    // As it is used asynchronously, we need to delete ourselves
    this->deleteLater();
}

CommandResult VcsCommand::runCommand(const Utils::CommandLine &command, int timeoutS)
{
    return runCommand(command, timeoutS, d->m_defaultWorkingDirectory, {});
}

CommandResult VcsCommand::runCommand(const CommandLine &command, int timeoutS,
                                     const FilePath &workingDirectory,
                                     const ExitCodeInterpreter &interpreter)
{
    QtcProcess proc;
    if (command.executable().isEmpty())
        return {};

    d->setupProcess(&proc, {command, timeoutS, workingDirectory, interpreter});
    if (d->isFullySynchronous())
        runFullySynchronous(proc);
    else
        runSynchronous(proc);
    d->handleDone(&proc);

    return CommandResult(proc);
}

void VcsCommand::runFullySynchronous(QtcProcess &process)
{
    process.runBlocking();

    if (!d->m_aborted) {
        const QString stdErr = process.cleanedStdErr();
        if (!stdErr.isEmpty() && !(d->m_flags & SuppressStdErr))
            emit append(stdErr);

        const QString stdOut = process.cleanedStdOut();
        if (!stdOut.isEmpty() && d->m_flags & ShowStdOut) {
            if (d->m_flags & SilentOutput)
                emit appendSilently(stdOut);
            else
                emit append(stdOut);
        }
    }
}

void VcsCommand::runSynchronous(QtcProcess &process)
{
    connect(this, &VcsCommand::terminate, &process, [&process] {
        process.stop();
        process.waitForFinished();
    });
    d->setupSynchronous(&process);
    process.setTimeOutMessageBoxEnabled(true);
    process.runBlocking(EventLoopMode::On);
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

void VcsCommand::setProgressiveOutput(bool progressive)
{
    d->m_progressiveOutput = progressive;
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
