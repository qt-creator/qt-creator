/****************************************************************************
**
** Copyright (C) 2016 Brian McGillion and Hugues Delorme
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

#include "vcscommand.h"

#include "vcsbaseplugin.h"
#include "vcsoutputwindow.h"
#include "vcsplugin.h"

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

class VcsCommandPrivate
{
public:
    struct Job {
        explicit Job(const FilePath &wd, const CommandLine &command, int t,
                     const ExitCodeInterpreter &interpreter);

        FilePath workingDirectory;
        CommandLine command;
        ExitCodeInterpreter exitCodeInterpreter;
        int timeoutS;
    };

    VcsCommandPrivate(const FilePath &defaultWorkingDirectory, const Environment &environment)
        : m_defaultWorkingDirectory(defaultWorkingDirectory),
          m_environment(environment)
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

    QString m_displayName;
    const FilePath m_defaultWorkingDirectory;
    Environment m_environment;
    QVariant m_cookie;
    QTextCodec *m_codec = nullptr;
    ProgressParser *m_progressParser = nullptr;
    QFutureWatcher<void> m_watcher;
    QFutureInterface<void> m_futureInterface;
    QList<Job> m_jobs;

    unsigned m_flags = 0;
    int m_defaultTimeoutS = 10;

    bool m_progressiveOutput = false;
    bool m_hadOutput = false;
    bool m_aborted = false;
};

VcsCommandPrivate::Job::Job(const FilePath &wd, const CommandLine &command,
                              int t, const ExitCodeInterpreter &interpreter) :
    workingDirectory(wd),
    command(command),
    exitCodeInterpreter(interpreter),
    timeoutS(t)
{
    // Finished cookie is emitted via queued slot, needs metatype
    static const int qvMetaId = qRegisterMetaType<QVariant>();
    Q_UNUSED(qvMetaId)
}

} // namespace Internal

VcsCommand::VcsCommand(const FilePath &workingDirectory, const Environment &environment) :
    d(new Internal::VcsCommandPrivate(workingDirectory, environment))
{
    connect(&d->m_watcher, &QFutureWatcher<void>::canceled, this, &VcsCommand::cancel);

    VcsOutputWindow::setRepository(defaultWorkingDirectory().toString());
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

    const QString name = displayName();
    const auto id = Id::fromString(name + QLatin1String(".action"));
    if (d->m_progressParser) {
        ProgressManager::addTask(future, name, id);
    } else {
        ProgressManager::addTimedTask(d->m_futureInterface, name, id, qMax(2, timeoutS() / 5));
    }

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
    d->m_futureInterface.reportFinished();
    delete d;
}

QString VcsCommand::displayName() const
{
    if (!d->m_displayName.isEmpty())
        return d->m_displayName;
    if (!d->m_jobs.isEmpty()) {
        const Internal::VcsCommandPrivate::Job &job = d->m_jobs.at(0);
        QString result = job.command.executable().baseName();
        if (!result.isEmpty())
            result[0] = result.at(0).toTitleCase();
        else
            result = tr("UNKNOWN");

        if (!job.command.arguments().isEmpty())
            result += ' ' + job.command.splitArguments().at(0);

        return result;
    }
    return tr("Unknown");
}

void VcsCommand::setDisplayName(const QString &name)
{
    d->m_displayName = name;
}

const FilePath &VcsCommand::defaultWorkingDirectory() const
{
    return d->m_defaultWorkingDirectory;
}

int VcsCommand::defaultTimeoutS() const
{
    return d->m_defaultTimeoutS;
}

void VcsCommand::setDefaultTimeoutS(int timeout)
{
    d->m_defaultTimeoutS = timeout;
}

void VcsCommand::addFlags(unsigned f)
{
    d->m_flags |= f;
}

void VcsCommand::addJob(const CommandLine &command,
                          const FilePath &workingDirectory,
                          const ExitCodeInterpreter &interpreter)
{
    addJob(command, defaultTimeoutS(), workingDirectory, interpreter);
}

void VcsCommand::addJob(const CommandLine &command, int timeoutS,
                          const FilePath &workingDirectory,
                          const ExitCodeInterpreter &interpreter)
{
    d->m_jobs.push_back(Internal::VcsCommandPrivate::Job(workDirectory(workingDirectory), command,
                                                           timeoutS, interpreter));
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

int VcsCommand::timeoutS() const
{
    return std::accumulate(d->m_jobs.cbegin(), d->m_jobs.cend(), 0,
                           [](int sum, const Internal::VcsCommandPrivate::Job &job) {
        return sum + job.timeoutS;
    });
}

FilePath VcsCommand::workDirectory(const FilePath &wd) const
{
    if (!wd.isEmpty())
        return wd;
    return defaultWorkingDirectory();
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
    if (d->m_progressParser)
        d->m_progressParser->setFuture(&future);
    else
        future.setProgressRange(0, 1);
    const int count = d->m_jobs.size();
    bool lastExecSuccess = true;
    for (int j = 0; j < count; j++) {
        const Internal::VcsCommandPrivate::Job &job = d->m_jobs.at(j);
        const CommandResult result = runCommand(job.command, job.workingDirectory,
                                                job.timeoutS, job.exitCodeInterpreter);
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
        emit finished(lastExecSuccess, cookie());
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

CommandResult VcsCommand::runCommand(const CommandLine &command, const FilePath &workingDirectory,
                                     int timeoutS, const ExitCodeInterpreter &interpreter)
{
    QtcProcess proc;
    if (command.executable().isEmpty())
        return {};

    proc.setExitCodeInterpreter(interpreter);
    proc.setTimeoutS(timeoutS);

    const FilePath dir = workDirectory(workingDirectory);
    if (!dir.isEmpty())
        proc.setWorkingDirectory(dir);

    if (!(d->m_flags & SuppressCommandLogging))
        emit appendCommand(dir, command);

    proc.setCommand(command);
    proc.setDisableUnixTerminal();
    proc.setEnvironment(d->environment());
    if (d->m_flags & MergeOutputChannels)
        proc.setProcessChannelMode(QProcess::MergedChannels);
    if (d->m_codec)
        proc.setCodec(d->m_codec);

    if ((d->m_flags & FullySynchronously)
            || (!(d->m_flags & NoFullySync)
                && QThread::currentThread() == QCoreApplication::instance()->thread())) {
        runFullySynchronous(proc);
    } else {
        runSynchronous(proc);
    }

    if (!d->m_aborted) {
        // Success/Fail message in appropriate window?
        if (proc.result() == ProcessResult::FinishedWithSuccess) {
            if (d->m_flags & ShowSuccessMessage)
                emit appendMessage(proc.exitMessage());
        } else if (!(d->m_flags & SuppressFailMessage)) {
            emit appendError(proc.exitMessage());
        }
    }
    emit runCommandFinished(dir);
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
    // connect stderr to the output window if desired
    if (!(d->m_flags & MergeOutputChannels)
            && (d->m_progressiveOutput || !(d->m_flags & SuppressStdErr))) {
        process.setStdErrCallback([this](const QString &text) {
            if (d->m_progressParser)
                d->m_progressParser->parseProgress(text);
            if (!(d->m_flags & SuppressStdErr))
                emit appendError(text);
            if (d->m_progressiveOutput)
                emit stdErrText(text);
        });
    }

    // connect stdout to the output window if desired
    if (d->m_progressParser || d->m_progressiveOutput || (d->m_flags & ShowStdOut)) {
        process.setStdOutCallback([this](const QString &text) {
            if (d->m_progressParser)
                d->m_progressParser->parseProgress(text);
            if (d->m_flags & ShowStdOut)
                emit append(text);
            if (d->m_progressiveOutput) {
                emit stdOutText(text);
                d->m_hadOutput = true;
            }
        });
    }

    process.setTimeOutMessageBoxEnabled(true);
    process.runBlocking(EventLoopMode::On);
}

const QVariant &VcsCommand::cookie() const
{
    return d->m_cookie;
}

void VcsCommand::setCookie(const QVariant &cookie)
{
    d->m_cookie = cookie;
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
