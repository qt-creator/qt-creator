/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "abstractprocessstep.h"
#include "ansifilterparser.h"
#include "buildstep.h"
#include "ioutputparser.h"
#include "project.h"
#include "task.h"

#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QTimer>
#include <QDir>

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
    BuildStep(bsl, id), m_timer(0), m_futureInterface(0),
    m_ignoreReturnValue(false), m_process(0),
    m_outputParserChain(0), m_skipFlush(false)
{
}

AbstractProcessStep::AbstractProcessStep(BuildStepList *bsl,
                                         AbstractProcessStep *bs) :
    BuildStep(bsl, bs), m_timer(0), m_futureInterface(0),
    m_ignoreReturnValue(bs->m_ignoreReturnValue),
    m_process(0), m_outputParserChain(0), m_skipFlush(false)
{
}

AbstractProcessStep::~AbstractProcessStep()
{
    delete m_process;
    delete m_timer;
    // do not delete m_futureInterface, we do not own it.
    delete m_outputParserChain;
}

/*!
     Deletes all existing output parsers and starts a new chain with the
     given parser.

     Derived classes need to call this function.
*/

void AbstractProcessStep::setOutputParser(IOutputParser *parser)
{
    delete m_outputParserChain;
    m_outputParserChain = new AnsiFilterParser;
    m_outputParserChain->appendOutputParser(parser);

    if (m_outputParserChain) {
        connect(m_outputParserChain, &IOutputParser::addOutput,
                this, &AbstractProcessStep::outputAdded);
        connect(m_outputParserChain, &IOutputParser::addTask,
                this, &AbstractProcessStep::taskAdded);
    }
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
    return;
}

IOutputParser *AbstractProcessStep::outputParser() const
{
    return m_outputParserChain;
}

void AbstractProcessStep::emitFaultyConfigurationMessage()
{
    emit addOutput(tr("Configuration is faulty. Check the Issues view for details."),
                   BuildStep::MessageOutput);
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
    return true;
}

/*!
    Reimplemented from BuildStep::init(). You need to call this from
    YourBuildStep::run().
*/

void AbstractProcessStep::run(QFutureInterface<bool> &fi)
{
    m_futureInterface = &fi;
    QDir wd(m_param.effectiveWorkingDirectory());
    if (!wd.exists()) {
        if (!wd.mkpath(wd.absolutePath())) {
            emit addOutput(tr("Could not create directory \"%1\"")
                           .arg(QDir::toNativeSeparators(wd.absolutePath())),
                           BuildStep::ErrorMessageOutput);
            fi.reportResult(false);
            emit finished();
            return;
        }
    }

    QString effectiveCommand = m_param.effectiveCommand();
    if (!QFileInfo::exists(effectiveCommand)) {
        processStartupFailed();
        fi.reportResult(false);
        emit finished();
        return;
    }

    m_process = new Utils::QtcProcess();
    if (Utils::HostOsInfo::isWindowsHost())
        m_process->setUseCtrlCStub(true);
    m_process->setWorkingDirectory(wd.absolutePath());
    m_process->setEnvironment(m_param.environment());

    connect(m_process, SIGNAL(readyReadStandardOutput()),
            this, SLOT(processReadyReadStdOutput()));
    connect(m_process, SIGNAL(readyReadStandardError()),
            this, SLOT(processReadyReadStdError()));

    connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)),
            this, SLOT(slotProcessFinished(int,QProcess::ExitStatus)));

    m_process->setCommand(effectiveCommand, m_param.effectiveArguments());
    m_process->start();
    if (!m_process->waitForStarted()) {
        processStartupFailed();
        delete m_process;
        m_process = 0;
        fi.reportResult(false);
        emit finished();
        return;
    }
    processStarted();

    m_timer = new QTimer();
    connect(m_timer, SIGNAL(timeout()), this, SLOT(checkForCancel()));
    m_timer->start(500);
    m_killProcess = false;
}

void AbstractProcessStep::cleanUp()
{
    // The process has finished, leftover data is read in processFinished
    processFinished(m_process->exitCode(), m_process->exitStatus());
    bool returnValue = processSucceeded(m_process->exitCode(), m_process->exitStatus()) || m_ignoreReturnValue;

    // Clean up output parsers
    if (m_outputParserChain) {
        delete m_outputParserChain;
        m_outputParserChain = 0;
    }

    delete m_process;
    m_process = 0;
    m_futureInterface->reportResult(returnValue);
    m_futureInterface = 0;

    emit finished();
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
                   BuildStep::MessageOutput);
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
                       BuildStep::MessageOutput);
    } else if (status == QProcess::NormalExit) {
        emit addOutput(tr("The process \"%1\" exited with code %2.")
                       .arg(command, QString::number(m_process->exitCode())),
                       BuildStep::ErrorMessageOutput);
    } else {
        emit addOutput(tr("The process \"%1\" crashed.").arg(command), BuildStep::ErrorMessageOutput);
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
                   BuildStep::ErrorMessageOutput);
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
    m_process->setReadChannel(QProcess::StandardOutput);
    while (m_process->canReadLine()) {
        QString line = QString::fromLocal8Bit(m_process->readLine());
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
    emit addOutput(line, BuildStep::NormalOutput, BuildStep::DontAppendNewline);
}

void AbstractProcessStep::processReadyReadStdError()
{
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
    emit addOutput(line, BuildStep::ErrorOutput, BuildStep::DontAppendNewline);
}

QFutureInterface<bool> *AbstractProcessStep::futureInterface() const
{
    return m_futureInterface;
}

void AbstractProcessStep::checkForCancel()
{
    if (m_futureInterface->isCanceled() && m_timer->isActive()) {
        if (!m_killProcess) {
            m_process->terminate();
            m_timer->start(5000);
            m_killProcess = true;
        } else {
            m_process->kill();
            m_timer->stop();
        }
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
    if (!filePath.isEmpty() && !QDir::isAbsolutePath(filePath)) {
        // We have no save way to decide which file in which subfolder
        // is meant. Therefore we apply following heuristics:
        // 1. Check if file is unique in whole project
        // 2. Otherwise try again without any ../
        // 3. give up.

        QList<QFileInfo> possibleFiles;
        QString fileName = Utils::FileName::fromString(filePath).fileName();
        foreach (const QString &file, project()->files(Project::AllFiles)) {
            QFileInfo candidate(file);
            if (candidate.fileName() == fileName)
                possibleFiles << candidate;
        }

        if (possibleFiles.count() == 1) {
            editable.file = Utils::FileName(possibleFiles.first());
        } else {
            // More then one filename, so do a better compare
            // Chop of any "../"
            while (filePath.startsWith(QLatin1String("../")))
                filePath.remove(0, 3);
            int count = 0;
            QString possibleFilePath;
            foreach (const QFileInfo &fi, possibleFiles) {
                if (fi.filePath().endsWith(filePath)) {
                    possibleFilePath = fi.filePath();
                    ++count;
                }
            }
            if (count == 1)
                editable.file = Utils::FileName::fromString(possibleFilePath);
            else
                qWarning() << "Could not find absolute location of file " << filePath;
        }
    }
    emit addTask(editable, linkedOutputLines, skipLines);
}

void AbstractProcessStep::outputAdded(const QString &string, BuildStep::OutputFormat format)
{
    emit addOutput(string, format, BuildStep::DontAppendNewline);
}

void AbstractProcessStep::slotProcessFinished(int, QProcess::ExitStatus)
{
    m_timer->stop();
    delete m_timer;
    m_timer = 0;

    QString line = QString::fromLocal8Bit(m_process->readAllStandardError());
    if (!line.isEmpty())
        stdError(line);

    line = QString::fromLocal8Bit(m_process->readAllStandardOutput());
    if (!line.isEmpty())
        stdOutput(line);

    cleanUp();
}
