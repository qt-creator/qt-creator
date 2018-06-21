/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "abstractprocessstep.h"
#include "ansifilterparser.h"
#include "buildconfiguration.h"
#include "buildstep.h"
#include "project.h"
#include "task.h"

#include <coreplugin/reaper.h>

#include <utils/qtcassert.h>

#include <QTimer>
#include <QDir>

#include <algorithm>

namespace {
const int CACHE_SOFT_LIMIT = 500;
const int CACHE_HARD_LIMIT = 1000;
} // namespace

using namespace ProjectExplorer;

/*!
    \class ProjectExplorer::AbstractProcessStep

    \brief The AbstractProcessStep class is a convenience class that can be
    used as a base class instead of BuildStep.

    It should be used as a base class if your buildstep just needs to run a process.

    Usage:
    \list
    \li Use processParameters() to configure the process you want to run
    (you need to do that before calling AbstractProcessStep::init()).
    \li Inside YourBuildStep::init() call AbstractProcessStep::init().
    \li Inside YourBuildStep::run() call AbstractProcessStep::run(), which automatically starts the process
    and by default adds the output on stdOut and stdErr to the OutputWindow.
    \li If you need to process the process output override stdOut() and/or stdErr.
    \endlist

    The two functions processStarted() and processFinished() are called after starting/finishing the process.
    By default they add a message to the output window.

    Use setEnabled() to control whether the BuildStep needs to run. (A disabled BuildStep immediately returns true,
    from the run function.)

    \sa ProjectExplorer::ProcessParameters
*/

/*!
    \fn void ProjectExplorer::AbstractProcessStep::setEnabled(bool b)

    Enables or disables a BuildStep.

    Disabled BuildSteps immediately return true from their run function.
    Should be called from init().
*/

/*!
    \fn ProcessParameters *ProjectExplorer::AbstractProcessStep::processParameters()

    Obtains a reference to the parameters for the actual process to run.

     Should be used in init().
*/

AbstractProcessStep::AbstractProcessStep(BuildStepList *bsl, Core::Id id) :
    BuildStep(bsl, id)
{
    m_timer.setInterval(500);
    connect(&m_timer, &QTimer::timeout, this, &AbstractProcessStep::checkForCancel);
}

/*!
     Deletes all existing output parsers and starts a new chain with the
     given parser.

     Derived classes need to call this function.
*/

void AbstractProcessStep::setOutputParser(IOutputParser *parser)
{
    m_outputParserChain.reset(new AnsiFilterParser);
    m_outputParserChain->appendOutputParser(parser);

    connect(m_outputParserChain.get(), &IOutputParser::addOutput, this, &AbstractProcessStep::outputAdded);
    connect(m_outputParserChain.get(), &IOutputParser::addTask, this, &AbstractProcessStep::taskAdded);
}

/*!
    Appends the given output parser to the existing chain of parsers.
*/
void AbstractProcessStep::appendOutputParser(IOutputParser *parser)
{
    if (!parser)
        return;

    QTC_ASSERT(m_outputParserChain, return);
    m_outputParserChain->appendOutputParser(parser);
}

IOutputParser *AbstractProcessStep::outputParser() const
{
    return m_outputParserChain.get();
}

void AbstractProcessStep::emitFaultyConfigurationMessage()
{
    emit addOutput(tr("Configuration is faulty. Check the Issues view for details."),
                   BuildStep::OutputFormat::NormalMessage);
}

bool AbstractProcessStep::ignoreReturnValue()
{
    return m_ignoreReturnValue;
}

/*!
    If \a ignoreReturnValue is set to true, then the abstractprocess step will
    return success even if the return value indicates otherwise.

    Should be called from init.
*/

void AbstractProcessStep::setIgnoreReturnValue(bool b)
{
    m_ignoreReturnValue = b;
}

/*!
    Reimplemented from BuildStep::init(). You need to call this from
    YourBuildStep::init().
*/

bool AbstractProcessStep::init(QList<const BuildStep *> &earlierSteps)
{
    Q_UNUSED(earlierSteps);

    m_candidates.clear();
    const Utils::FileNameList fl = project()->files(Project::AllFiles);
    for (const Utils::FileName &file : fl)
        m_candidates[file.fileName()].push_back(file);

    return !m_process;
}

/*!
    Reimplemented from BuildStep::init(). You need to call this from
    YourBuildStep::run().
*/

void AbstractProcessStep::run(QFutureInterface<bool> &fi)
{
    QDir wd(m_param.effectiveWorkingDirectory());
    if (!wd.exists()) {
        if (!wd.mkpath(wd.absolutePath())) {
            emit addOutput(tr("Could not create directory \"%1\"")
                           .arg(QDir::toNativeSeparators(wd.absolutePath())),
                           BuildStep::OutputFormat::ErrorMessage);
            reportRunResult(fi, false);
            return;
        }
    }

    QString effectiveCommand = m_param.effectiveCommand();
    if (!QFileInfo::exists(effectiveCommand)) {
        processStartupFailed();
        reportRunResult(fi, false);
        return;
    }

    m_futureInterface = &fi;

    m_process.reset(new Utils::QtcProcess());
    m_process->setUseCtrlCStub(Utils::HostOsInfo::isWindowsHost());
    m_process->setWorkingDirectory(wd.absolutePath());
    m_process->setEnvironment(m_param.environment());
    m_process->setCommand(effectiveCommand, m_param.effectiveArguments());

    connect(m_process.get(), &QProcess::readyReadStandardOutput,
            this, &AbstractProcessStep::processReadyReadStdOutput);
    connect(m_process.get(), &QProcess::readyReadStandardError,
            this, &AbstractProcessStep::processReadyReadStdError);
    connect(m_process.get(), static_cast<void (QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished),
            this, &AbstractProcessStep::slotProcessFinished);

    m_process->start();
    if (!m_process->waitForStarted()) {
        processStartupFailed();
        m_process.reset();
        m_outputParserChain.reset();
        reportRunResult(fi, false);
        return;
    }
    processStarted();
    m_timer.start();
}

void AbstractProcessStep::cleanUp(QProcess *process)
{
    // The process has finished, leftover data is read in processFinished
    processFinished(process->exitCode(), process->exitStatus());
    const bool returnValue = processSucceeded(process->exitCode(), process->exitStatus()) || m_ignoreReturnValue;

    m_outputParserChain.reset();
    m_process.reset();

    // Report result
    reportRunResult(*m_futureInterface, returnValue);
}

/*!
    Called after the process is started.

    The default implementation adds a process-started message to the output
    message.
*/

void AbstractProcessStep::processStarted()
{
    emit addOutput(tr("Starting: \"%1\" %2")
                   .arg(QDir::toNativeSeparators(m_param.effectiveCommand()),
                        m_param.prettyArguments()),
                   BuildStep::OutputFormat::NormalMessage);
}

/*!
    Called after the process is finished.

    The default implementation adds a line to the output window.
*/

void AbstractProcessStep::processFinished(int exitCode, QProcess::ExitStatus status)
{
    if (m_outputParserChain)
        m_outputParserChain->flush();

    QString command = QDir::toNativeSeparators(m_param.effectiveCommand());
    if (status == QProcess::NormalExit && exitCode == 0) {
        emit addOutput(tr("The process \"%1\" exited normally.").arg(command),
                       BuildStep::OutputFormat::NormalMessage);
    } else if (status == QProcess::NormalExit) {
        emit addOutput(tr("The process \"%1\" exited with code %2.")
                       .arg(command, QString::number(exitCode)),
                       BuildStep::OutputFormat::ErrorMessage);
    } else {
        emit addOutput(tr("The process \"%1\" crashed.").arg(command), BuildStep::OutputFormat::ErrorMessage);
    }
}

/*!
    Called if the process could not be started.

    By default, adds a message to the output window.
*/

void AbstractProcessStep::processStartupFailed()
{
    emit addOutput(tr("Could not start process \"%1\" %2")
                   .arg(QDir::toNativeSeparators(m_param.effectiveCommand()),
                        m_param.prettyArguments()),
                   BuildStep::OutputFormat::ErrorMessage);
    m_timer.stop();
}

/*!
    Called to test whether a process succeeded or not.
*/

bool AbstractProcessStep::processSucceeded(int exitCode, QProcess::ExitStatus status)
{
    if (outputParser() && outputParser()->hasFatalErrors())
        return false;

    return exitCode == 0 && status == QProcess::NormalExit;
}

void AbstractProcessStep::processReadyReadStdOutput()
{
    if (!m_process)
        return;
    m_process->setReadChannel(QProcess::StandardOutput);
    const bool utf8Output = buildConfiguration()->environment().hasKey("VSLANG");

    while (m_process->canReadLine()) {
        QString line = utf8Output ? QString::fromUtf8(m_process->readLine())
                                  : QString::fromLocal8Bit(m_process->readLine());
        stdOutput(line);
    }
}

/*!
    Called for each line of output on stdOut().

    The default implementation adds the line to the application output window.
*/

void AbstractProcessStep::stdOutput(const QString &line)
{
    if (m_outputParserChain)
        m_outputParserChain->stdOutput(line);
    emit addOutput(line, BuildStep::OutputFormat::Stdout, BuildStep::DontAppendNewline);
}

void AbstractProcessStep::processReadyReadStdError()
{
    if (!m_process)
        return;
    m_process->setReadChannel(QProcess::StandardError);
    while (m_process->canReadLine()) {
        QString line = QString::fromLocal8Bit(m_process->readLine());
        stdError(line);
    }
}

/*!
    Called for each line of output on StdErrror().

    The default implementation adds the line to the application output window.
*/

void AbstractProcessStep::stdError(const QString &line)
{
    if (m_outputParserChain)
        m_outputParserChain->stdError(line);
    emit addOutput(line, BuildStep::OutputFormat::Stderr, BuildStep::DontAppendNewline);
}

QFutureInterface<bool> *AbstractProcessStep::futureInterface() const
{
    return m_futureInterface;
}

void AbstractProcessStep::checkForCancel()
{
    if (m_futureInterface->isCanceled() && m_timer.isActive()) {
        m_timer.stop();

        Core::Reaper::reap(m_process.release());
    }
}

void AbstractProcessStep::taskAdded(const Task &task, int linkedOutputLines, int skipLines)
{
    // Do not bother to report issues if we do not care about the results of
    // the buildstep anyway:
    if (m_ignoreReturnValue)
        return;

    // flush out any pending tasks before proceeding:
    if (!m_skipFlush && m_outputParserChain) {
        m_skipFlush = true;
        m_outputParserChain->flush();
        m_skipFlush = false;
    }

    Task editable(task);
    QString filePath = task.file.toString();

    auto it = m_filesCache.find(filePath);
    if (it != m_filesCache.end()) {
        editable.file = it.value().first;
        it.value().second = ++m_cacheCounter;
    } else if (!filePath.isEmpty() && !filePath.startsWith('<') && !QDir::isAbsolutePath(filePath)) {
        // We have no save way to decide which file in which subfolder
        // is meant. Therefore we apply following heuristics:
        // 1. Check if file is unique in whole project
        // 2. Otherwise try again without any ../
        // 3. give up.

        QString sourceFilePath = filePath;
        Utils::FileNameList possibleFiles = m_candidates.value(Utils::FileName::fromString(filePath).fileName());

        if (possibleFiles.count() == 1) {
            editable.file = possibleFiles.first();
        } else {
            // More then one filename, so do a better compare
            // Chop of any "../"
            while (filePath.startsWith("../"))
                filePath.remove(0, 3);

            int count = 0;
            Utils::FileName possibleFilePath;
            foreach (const Utils::FileName &fn, possibleFiles) {
                if (fn.endsWith(filePath)) {
                    possibleFilePath = fn;
                    ++count;
                }
            }
            if (count == 1)
                editable.file = possibleFilePath;
            else
                qWarning() << "Could not find absolute location of file " << filePath;
        }

        insertInCache(sourceFilePath, editable.file);
    }

    emit addTask(editable, linkedOutputLines, skipLines);
}

void AbstractProcessStep::outputAdded(const QString &string, BuildStep::OutputFormat format)
{
    emit addOutput(string, format, BuildStep::DontAppendNewline);
}

void AbstractProcessStep::slotProcessFinished(int, QProcess::ExitStatus)
{
    m_timer.stop();

    QProcess *process = m_process.get();
    if (!process) // Happens when the process was canceled and handed over to the Reaper.
        process = qobject_cast<QProcess *>(sender()); // The process was canceled!

    const QString stdErrLine = process ? QString::fromLocal8Bit(process->readAllStandardError()) : QString();
    for (const QString &l : stdErrLine.split('\n'))
        stdError(l);

    const QString stdOutLine = process ? QString::fromLocal8Bit(process->readAllStandardOutput()) : QString();
    for (const QString &l : stdOutLine.split('\n'))
        stdError(l);

    purgeCache(true);
    cleanUp(process);
}

void AbstractProcessStep::purgeCache(bool useSoftLimit)
{
    const int limit = useSoftLimit ? CACHE_SOFT_LIMIT : CACHE_HARD_LIMIT;
    if (m_filesCache.size() <= limit)
        return;

    const quint64 minCounter = m_cacheCounter - static_cast<quint64>(limit);
    std::remove_if(m_filesCache.begin(), m_filesCache.end(),
                   [minCounter](const QPair<Utils::FileName, quint64> &entry) {
        return entry.second <= minCounter;
    });
}

void AbstractProcessStep::insertInCache(const QString &relativePath, const Utils::FileName &absPath)
{
    purgeCache(false);
    m_filesCache.insert(relativePath, qMakePair(absPath, ++m_cacheCounter));
}
