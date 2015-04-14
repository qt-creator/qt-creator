/**************************************************************************
**
** Copyright (C) 2015 Brian McGillion and Hugues Delorme
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "vcscommand.h"
#include "vcsbaseplugin.h"

#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/icore.h>
#include <vcsbase/vcsoutputwindow.h>
#include <utils/fileutils.h>
#include <utils/synchronousprocess.h>
#include <utils/runextensions.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QProcess>
#include <QProcessEnvironment>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <QFileInfo>
#include <QCoreApplication>
#include <QVariant>
#include <QStringList>
#include <QTextCodec>
#include <QMutex>

enum { debugExecution = 0 };

/*!
    \fn void VcsBase::ProgressParser::parseProgress(const QString &text)

    Reimplement to parse progress as it appears in the standard output.
    If a progress string is detected, call \c setProgressAndMaximum() to update
    the progress bar accordingly.

    \sa VcsBase::ProgressParser::setProgressAndMaximum()
*/

/*!
    \fn void VcsBase::ProgressParser::setProgressAndMaximum(int value, int maximum)

    Sets progress \a value and \a maximum for current command. Called by \c parseProgress()
    when a progress string is detected.
*/

namespace VcsBase {
namespace Internal {

class VcsCommandPrivate
{
public:
    struct Job {
        explicit Job(const QStringList &a, int t, Utils::ExitCodeInterpreter *interpreter = 0);

        QStringList arguments;
        int timeoutS;
        Utils::ExitCodeInterpreter *exitCodeInterpreter;
    };

    VcsCommandPrivate(const Utils::FileName &binary,
                      const QString &workingDirectory,
                      const QProcessEnvironment &environment);
    ~VcsCommandPrivate();

    const Utils::FileName m_binaryPath;
    const QString m_workingDirectory;
    const QProcessEnvironment m_environment;
    QVariant m_cookie;
    int m_defaultTimeoutS;
    unsigned m_flags;
    QTextCodec *m_codec;
    const QString m_sshPasswordPrompt;
    ProgressParser *m_progressParser;
    bool m_progressiveOutput;
    bool m_hadOutput;
    bool m_preventRepositoryChanged;
    bool m_aborted;
    QFutureWatcher<void> m_watcher;

    QList<Job> m_jobs;

    bool m_lastExecSuccess;
    int m_lastExecExitCode;
};

VcsCommandPrivate::VcsCommandPrivate(const Utils::FileName &binary,
                                     const QString &workingDirectory,
                                     const QProcessEnvironment &environment) :
    m_binaryPath(binary),
    m_workingDirectory(workingDirectory),
    m_environment(environment),
    m_defaultTimeoutS(10),
    m_flags(0),
    m_codec(0),
    m_sshPasswordPrompt(VcsBasePlugin::sshPrompt()),
    m_progressParser(0),
    m_progressiveOutput(false),
    m_hadOutput(false),
    m_preventRepositoryChanged(false),
    m_aborted(false),
    m_lastExecSuccess(false),
    m_lastExecExitCode(-1)
{
}

VcsCommandPrivate::~VcsCommandPrivate()
{
    delete m_progressParser;
}

VcsCommandPrivate::Job::Job(const QStringList &a, int t, Utils::ExitCodeInterpreter *interpreter) :
    arguments(a),
    timeoutS(t),
    exitCodeInterpreter(interpreter)
{
    // Finished cookie is emitted via queued slot, needs metatype
    static const int qvMetaId = qRegisterMetaType<QVariant>();
    Q_UNUSED(qvMetaId)
}

} // namespace Internal

VcsCommand::VcsCommand(const Utils::FileName &binary,
                       const QString &workingDirectory,
                       const QProcessEnvironment &environment) :
    d(new Internal::VcsCommandPrivate(binary, workingDirectory, environment))
{
    connect(Core::ICore::instance(), &Core::ICore::coreAboutToClose,
            this, &VcsCommand::coreAboutToClose);
}

VcsCommand::~VcsCommand()
{
    delete d;
}

const Utils::FileName &VcsCommand::binaryPath() const
{
    return d->m_binaryPath;
}

const QString &VcsCommand::workingDirectory() const
{
    return d->m_workingDirectory;
}

const QProcessEnvironment &VcsCommand::processEnvironment() const
{
    return d->m_environment;
}

int VcsCommand::defaultTimeoutS() const
{
    return d->m_defaultTimeoutS;
}

void VcsCommand::setDefaultTimeoutS(int timeout)
{
    d->m_defaultTimeoutS = timeout;
}

unsigned VcsCommand::flags() const
{
    return d->m_flags;
}

void VcsCommand::addFlags(unsigned f)
{
    d->m_flags |= f;
}

void VcsCommand::addJob(const QStringList &arguments, Utils::ExitCodeInterpreter *interpreter)
{
    addJob(arguments, defaultTimeoutS(), interpreter);
}

void VcsCommand::addJob(const QStringList &arguments, int timeoutS,
                        Utils::ExitCodeInterpreter *interpreter)
{
    d->m_jobs.push_back(Internal::VcsCommandPrivate::Job(arguments, timeoutS, interpreter));
}

void VcsCommand::execute()
{
    d->m_lastExecSuccess = false;
    d->m_lastExecExitCode = -1;

    if (d->m_jobs.empty())
        return;

    // For some reason QtConcurrent::run() only works on this
    QFuture<void> task = QtConcurrent::run(&VcsCommand::run, this);
    d->m_watcher.setFuture(task);
    connect(&d->m_watcher, &QFutureWatcher<void>::canceled, this, &VcsCommand::cancel);
    QString binary = d->m_binaryPath.toFileInfo().baseName();
    if (!binary.isEmpty())
        binary = binary.replace(0, 1, binary[0].toUpper()); // Upper the first letter
    const QString taskName = binary + QLatin1Char(' ') + d->m_jobs.front().arguments.at(0);

    Core::ProgressManager::addTask(task, taskName,
        Core::Id::fromString(binary + QLatin1String(".action")));
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

bool VcsCommand::lastExecutionSuccess() const
{
    return d->m_lastExecSuccess;
}

int VcsCommand::lastExecutionExitCode() const
{
    return d->m_lastExecExitCode;
}

void VcsCommand::run(QFutureInterface<void> &future)
{
    // Check that the binary path is not empty
    if (binaryPath().isEmpty()) {
        emit errorText(tr("Unable to start process, binary is empty"));
        return;
    }

    QString stdOut;
    QString stdErr;

    if (d->m_progressParser)
        d->m_progressParser->setFuture(&future);
    else
        future.setProgressRange(0, 1);
    const int count = d->m_jobs.size();
    d->m_lastExecExitCode = -1;
    d->m_lastExecSuccess = true;
    for (int j = 0; j < count; j++) {
        const Internal::VcsCommandPrivate::Job &job = d->m_jobs.at(j);
        Utils::SynchronousProcessResponse resp
                = runVcs( job.arguments, job.timeoutS, job.exitCodeInterpreter);
        stdOut += resp.stdOut;
        stdErr += resp.stdErr;
        d->m_lastExecExitCode = resp.exitCode;
        d->m_lastExecSuccess = resp.result == Utils::SynchronousProcessResponse::Finished;
        if (!d->m_lastExecSuccess)
            break;
    }

    if (!d->m_aborted) {
        if (!d->m_progressiveOutput) {
            emit output(stdOut);
            if (!stdErr.isEmpty())
                emit errorText(stdErr);
        }

        emit finished(d->m_lastExecSuccess, d->m_lastExecExitCode, cookie());
        if (d->m_lastExecSuccess)
            emit success(cookie());
        future.setProgressValue(future.progressMaximum());
    }

    if (d->m_progressParser)
        d->m_progressParser->setFuture(0);
    // As it is used asynchronously, we need to delete ourselves
    this->deleteLater();
}

class OutputProxy : public QObject
{
    Q_OBJECT

    friend class VcsCommand;

public:
    OutputProxy()
    {
        // Users of this class can either be in the GUI thread or in other threads.
        // Use Qt::AutoConnection to always append in the GUI thread (directly or queued)
        VcsOutputWindow *outputWindow = VcsOutputWindow::instance();
        connect(this, &OutputProxy::append,
                outputWindow, [](const QString &txt) { VcsOutputWindow::append(txt); });
        connect(this, &OutputProxy::appendSilently, outputWindow, &VcsOutputWindow::appendSilently);
        connect(this, &OutputProxy::appendError, outputWindow, &VcsOutputWindow::appendError);
        connect(this, &OutputProxy::appendCommand, outputWindow, &VcsOutputWindow::appendCommand);
        connect(this, &OutputProxy::appendMessage, outputWindow, &VcsOutputWindow::appendMessage);
    }

signals:
    void append(const QString &text);
    void appendSilently(const QString &text);
    void appendError(const QString &text);
    void appendCommand(const QString &workingDirectory,
                       const Utils::FileName &binary,
                       const QStringList &args);
    void appendMessage(const QString &text);
};

Utils::SynchronousProcessResponse VcsCommand::runVcs(const QStringList &arguments, int timeoutS,
                                                     Utils::ExitCodeInterpreter *interpreter)
{
    Utils::SynchronousProcessResponse response;
    OutputProxy outputProxy;

    if (d->m_binaryPath.isEmpty()) {
        response.result = Utils::SynchronousProcessResponse::StartFailed;
        return response;
    }

    if (!(d->m_flags & VcsBasePlugin::SuppressCommandLogging))
        emit outputProxy.appendCommand(d->m_workingDirectory, d->m_binaryPath, arguments);

    const bool sshPromptConfigured = !d->m_sshPasswordPrompt.isEmpty();
    if (debugExecution) {
        QDebug nsp = qDebug().nospace();
        nsp << "Command::runVcs" << d->m_workingDirectory << d->m_binaryPath << arguments
                << timeoutS;
        if (d->m_flags & VcsBasePlugin::ShowStdOutInLogWindow)
            nsp << "stdout";
        if (d->m_flags & VcsBasePlugin::SuppressStdErrInLogWindow)
            nsp << "suppress_stderr";
        if (d->m_flags & VcsBasePlugin::SuppressFailMessageInLogWindow)
            nsp << "suppress_fail_msg";
        if (d->m_flags & VcsBasePlugin::MergeOutputChannels)
            nsp << "merge_channels";
        if (d->m_flags & VcsBasePlugin::SshPasswordPrompt)
            nsp << "ssh (" << sshPromptConfigured << ')';
        if (d->m_flags & VcsBasePlugin::SuppressCommandLogging)
            nsp << "suppress_log";
        if (d->m_flags & VcsBasePlugin::ForceCLocale)
            nsp << "c_locale";
        if (d->m_flags & VcsBasePlugin::FullySynchronously)
            nsp << "fully_synchronously";
        if (d->m_flags & VcsBasePlugin::ExpectRepoChanges)
            nsp << "expect_repo_changes";
        if (d->m_codec)
            nsp << " Codec: " << d->m_codec->name();
    }

    // TODO tell the document manager about expected repository changes
    //    if (d->m_flags & ExpectRepoChanges)
    //        Core::DocumentManager::expectDirectoryChange(d->m_workingDirectory);
    if (d->m_flags & VcsBasePlugin::FullySynchronously) {
        response = runSynchronous(arguments, timeoutS, interpreter);
    } else {
        Utils::SynchronousProcess process;
        process.setExitCodeInterpreter(interpreter);
        connect(this, &VcsCommand::terminate, &process, &Utils::SynchronousProcess::terminate);
        if (!d->m_workingDirectory.isEmpty())
            process.setWorkingDirectory(d->m_workingDirectory);

        QProcessEnvironment env = d->m_environment;
        VcsBasePlugin::setProcessEnvironment(&env,
                                             (d->m_flags & VcsBasePlugin::ForceCLocale),
                                             d->m_sshPasswordPrompt);
        process.setProcessEnvironment(env);
        process.setTimeoutS(timeoutS);
        if (d->m_codec)
            process.setCodec(d->m_codec);

        // Suppress terminal on UNIX for ssh prompts if it is configured.
        if (sshPromptConfigured && (d->m_flags & VcsBasePlugin::SshPasswordPrompt))
            process.setFlags(Utils::SynchronousProcess::UnixTerminalDisabled);

        // connect stderr to the output window if desired
        if (d->m_flags & VcsBasePlugin::MergeOutputChannels) {
            process.setProcessChannelMode(QProcess::MergedChannels);
        } else if (d->m_progressiveOutput
                   || !(d->m_flags & VcsBasePlugin::SuppressStdErrInLogWindow)) {
            process.setStdErrBufferedSignalsEnabled(true);
            connect(&process, &Utils::SynchronousProcess::stdErrBuffered,
                    this, &VcsCommand::bufferedError);
        }

        // connect stdout to the output window if desired
        if (d->m_progressParser || d->m_progressiveOutput
                || (d->m_flags & VcsBasePlugin::ShowStdOutInLogWindow)) {
            process.setStdOutBufferedSignalsEnabled(true);
            connect(&process, &Utils::SynchronousProcess::stdOutBuffered,
                    this, &VcsCommand::bufferedOutput);
        }

        process.setTimeOutMessageBoxEnabled(true);

        // Run!
        response = process.run(d->m_binaryPath.toString(), arguments);
    }

    if (!d->m_aborted) {
        // Success/Fail message in appropriate window?
        if (response.result == Utils::SynchronousProcessResponse::Finished) {
            if (d->m_flags & VcsBasePlugin::ShowSuccessMessage) {
                emit outputProxy.appendMessage(response.exitMessage(d->m_binaryPath.toUserOutput(),
                                                                    timeoutS));
            }
        } else if (!(d->m_flags & VcsBasePlugin::SuppressFailMessageInLogWindow)) {
            emit outputProxy.appendError(response.exitMessage(d->m_binaryPath.toUserOutput(),
                                                              timeoutS));
        }
    }
    emitRepositoryChanged();

    return response;
}

Utils::SynchronousProcessResponse VcsCommand::runSynchronous(const QStringList &arguments,
                                                             int timeoutS,
                                                             Utils::ExitCodeInterpreter *interpreter)
{
    Utils::SynchronousProcessResponse response;

    // Set up process
    unsigned processFlags = 0;
    if (!d->m_sshPasswordPrompt.isEmpty() && (d->m_flags & VcsBasePlugin::SshPasswordPrompt))
        processFlags |= Utils::SynchronousProcess::UnixTerminalDisabled;
    QSharedPointer<QProcess> process = Utils::SynchronousProcess::createProcess(processFlags);
    if (!d->m_workingDirectory.isEmpty())
        process->setWorkingDirectory(d->m_workingDirectory);
    QProcessEnvironment env = d->m_environment;
    VcsBasePlugin::setProcessEnvironment(&env,
                                         (d->m_flags & VcsBasePlugin::ForceCLocale),
                                         d->m_sshPasswordPrompt);
    process->setProcessEnvironment(env);
    if (d->m_flags & VcsBasePlugin::MergeOutputChannels)
        process->setProcessChannelMode(QProcess::MergedChannels);

    // Start
    process->start(d->m_binaryPath.toString(), arguments, QIODevice::ReadOnly);
    process->closeWriteChannel();
    if (!process->waitForStarted()) {
        response.result = Utils::SynchronousProcessResponse::StartFailed;
        return response;
    }

    // process output
    QByteArray stdOut;
    QByteArray stdErr;
    const bool timedOut =
            !Utils::SynchronousProcess::readDataFromProcess(*process.data(), timeoutS,
                                                            &stdOut, &stdErr, true);

    if (!d->m_aborted) {
        OutputProxy outputProxy;
        if (!stdErr.isEmpty()) {
            response.stdErr = Utils::SynchronousProcess::normalizeNewlines(
                        d->m_codec ? d->m_codec->toUnicode(stdErr) : QString::fromLocal8Bit(stdErr));
            if (!(d->m_flags & VcsBasePlugin::SuppressStdErrInLogWindow))
                emit outputProxy.append(response.stdErr);
        }

        if (!stdOut.isEmpty()) {
            response.stdOut = Utils::SynchronousProcess::normalizeNewlines(
                        d->m_codec ? d->m_codec->toUnicode(stdOut) : QString::fromLocal8Bit(stdOut));
            if (d->m_flags & VcsBasePlugin::ShowStdOutInLogWindow) {
                if (d->m_flags & VcsBasePlugin::SilentOutput)
                    emit outputProxy.appendSilently(response.stdOut);
                else
                    emit outputProxy.append(response.stdOut);
            }
        }
    }

    Utils::ExitCodeInterpreter defaultInterpreter(this);
    Utils::ExitCodeInterpreter *currentInterpreter = interpreter ? interpreter : &defaultInterpreter;
    // Result
    if (timedOut)
        response.result = Utils::SynchronousProcessResponse::Hang;
    else if (process->exitStatus() != QProcess::NormalExit)
        response.result = Utils::SynchronousProcessResponse::TerminatedAbnormally;
    else
        response.result = currentInterpreter->interpretExitCode(process->exitCode());
    return response;
}

void VcsCommand::emitRepositoryChanged()
{
    if (d->m_preventRepositoryChanged || !(d->m_flags & VcsBasePlugin::ExpectRepoChanges))
        return;
    // TODO tell the document manager that the directory now received all expected changes
    // Core::DocumentManager::unexpectDirectoryChange(d->m_workingDirectory);
    Core::VcsManager::emitRepositoryChanged(d->m_workingDirectory);
}

bool VcsCommand::runFullySynchronous(const QStringList &arguments, int timeoutS,
                                     QByteArray *outputData, QByteArray *errorData)
{
    if (d->m_binaryPath.isEmpty())
        return false;

    OutputProxy outputProxy;
    if (!(d->m_flags & VcsBasePlugin::SuppressCommandLogging))
        emit outputProxy.appendCommand(d->m_workingDirectory, d->m_binaryPath, arguments);

    // TODO tell the document manager about expected repository changes
    // if (d->m_flags & ExpectRepoChanges)
    //    Core::DocumentManager::expectDirectoryChange(workingDirectory);
    QProcess process;
    process.setWorkingDirectory(d->m_workingDirectory);
    process.setProcessEnvironment(d->m_environment);

    process.start(d->m_binaryPath.toString(), arguments);
    process.closeWriteChannel();
    if (!process.waitForStarted()) {
        if (errorData) {
            const QString msg = QString::fromLatin1("Unable to execute \"%1\": %2:")
                                .arg(d->m_binaryPath.toUserOutput(), process.errorString());
            *errorData = msg.toLocal8Bit();
        }
        return false;
    }

    if (!Utils::SynchronousProcess::readDataFromProcess(process, timeoutS, outputData, errorData, true)) {
        if (errorData)
            errorData->append(tr("Error: Executable timed out after %1s.").arg(timeoutS).toLocal8Bit());
        Utils::SynchronousProcess::stopProcess(process);
        return false;
    }

    emitRepositoryChanged();
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

void VcsCommand::bufferedOutput(const QString &text)
{
    if (d->m_progressParser)
        d->m_progressParser->parseProgress(text);
    if (d->m_flags & VcsBasePlugin::ShowStdOutInLogWindow)
        VcsOutputWindow::append(text);
    if (d->m_progressiveOutput) {
        emit output(text);
        d->m_hadOutput = true;
    }
}

void VcsCommand::bufferedError(const QString &text)
{
    if (!(d->m_flags & VcsBasePlugin::SuppressStdErrInLogWindow))
        VcsOutputWindow::appendError(text);
    if (d->m_progressiveOutput)
        emit errorText(text);
}

void VcsCommand::coreAboutToClose()
{
    d->m_preventRepositoryChanged = true;
    abort();
}

const QVariant &VcsCommand::cookie() const
{
    return d->m_cookie;
}

void VcsCommand::setCookie(const QVariant &cookie)
{
    d->m_cookie = cookie;
}

QTextCodec *VcsCommand::codec() const
{
    return d->m_codec;
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
    m_future(0),
    m_futureMutex(new QMutex)
{
}

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

} // namespace VcsBase

#include "vcscommand.moc"
