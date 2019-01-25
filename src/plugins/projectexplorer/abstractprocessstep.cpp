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
#include "ioutputparser.h"
#include "processparameters.h"
#include "project.h"
#include "target.h"
#include "task.h"

#include <coreplugin/reaper.h>

#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QDir>
#include <QTimer>
#include <QHash>
#include <QPair>

#include <algorithm>
#include <memory>

namespace {
const int CACHE_SOFT_LIMIT = 500;
const int CACHE_HARD_LIMIT = 1000;
} // namespace

namespace ProjectExplorer {

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

class AbstractProcessStep::Private
{
public:
    Private(AbstractProcessStep *q) : q(q) {}

    AbstractProcessStep *q;
    std::unique_ptr<Utils::QtcProcess> m_process;
    std::unique_ptr<IOutputParser> m_outputParserChain;
    ProcessParameters m_param;
    QHash<QString, QPair<Utils::FileName, quint64>> m_filesCache;
    QHash<QString, Utils::FileNameList> m_candidates;
    QByteArray deferredText;
    quint64 m_cacheCounter = 0;
    bool m_ignoreReturnValue = false;
    bool m_skipFlush = false;

    void readData(void (AbstractProcessStep::*func)(const QString &), bool isUtf8 = false);
    void processLine(const QByteArray &data,
                     void (AbstractProcessStep::*func)(const QString &),
                     bool isUtf8 = false);
};

AbstractProcessStep::AbstractProcessStep(BuildStepList *bsl, Core::Id id) :
    BuildStep(bsl, id),
    d(new Private(this))
{
}

AbstractProcessStep::~AbstractProcessStep()
{
    delete d;
}

/*!
     Deletes all existing output parsers and starts a new chain with the
     given parser.

     Derived classes need to call this function.
*/

void AbstractProcessStep::setOutputParser(IOutputParser *parser)
{
    d->m_outputParserChain.reset(new AnsiFilterParser);
    d->m_outputParserChain->appendOutputParser(parser);

    connect(d->m_outputParserChain.get(), &IOutputParser::addOutput, this, &AbstractProcessStep::outputAdded);
    connect(d->m_outputParserChain.get(), &IOutputParser::addTask, this, &AbstractProcessStep::taskAdded);
}

/*!
    Appends the given output parser to the existing chain of parsers.
*/
void AbstractProcessStep::appendOutputParser(IOutputParser *parser)
{
    if (!parser)
        return;

    QTC_ASSERT(d->m_outputParserChain, return);
    d->m_outputParserChain->appendOutputParser(parser);
}

IOutputParser *AbstractProcessStep::outputParser() const
{
    return d->m_outputParserChain.get();
}

void AbstractProcessStep::emitFaultyConfigurationMessage()
{
    emit addOutput(tr("Configuration is faulty. Check the Issues view for details."),
                   BuildStep::OutputFormat::NormalMessage);
}

bool AbstractProcessStep::ignoreReturnValue()
{
    return d->m_ignoreReturnValue;
}

/*!
    If \a ignoreReturnValue is set to true, then the abstractprocess step will
    return success even if the return value indicates otherwise.

    Should be called from init.
*/

void AbstractProcessStep::setIgnoreReturnValue(bool b)
{
    d->m_ignoreReturnValue = b;
}

/*!
    Reimplemented from BuildStep::init(). You need to call this from
    YourBuildStep::init().
*/

bool AbstractProcessStep::init()
{
    d->m_candidates.clear();
    const Utils::FileNameList fl = project()->files(Project::AllFiles);
    for (const Utils::FileName &file : fl)
        d->m_candidates[file.fileName()].push_back(file);

    return !d->m_process;
}

/*!
    Reimplemented from BuildStep::init(). You need to call this from
    YourBuildStep::run().
*/

void AbstractProcessStep::doRun()
{
    QDir wd(d->m_param.effectiveWorkingDirectory());
    if (!wd.exists()) {
        if (!wd.mkpath(wd.absolutePath())) {
            emit addOutput(tr("Could not create directory \"%1\"")
                           .arg(QDir::toNativeSeparators(wd.absolutePath())),
                           BuildStep::OutputFormat::ErrorMessage);
            finish(false);
            return;
        }
    }

    QString effectiveCommand = d->m_param.effectiveCommand();
    if (!QFileInfo::exists(effectiveCommand)) {
        processStartupFailed();
        finish(false);
        return;
    }

    d->m_process.reset(new Utils::QtcProcess());
    d->m_process->setUseCtrlCStub(Utils::HostOsInfo::isWindowsHost());
    d->m_process->setWorkingDirectory(wd.absolutePath());
    d->m_process->setEnvironment(d->m_param.environment());
    d->m_process->setCommand(effectiveCommand, d->m_param.effectiveArguments());

    connect(d->m_process.get(), &QProcess::readyReadStandardOutput,
            this, &AbstractProcessStep::processReadyReadStdOutput);
    connect(d->m_process.get(), &QProcess::readyReadStandardError,
            this, &AbstractProcessStep::processReadyReadStdError);
    connect(d->m_process.get(), static_cast<void (QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished),
            this, &AbstractProcessStep::slotProcessFinished);

    d->m_process->start();
    if (!d->m_process->waitForStarted()) {
        processStartupFailed();
        d->m_process.reset();
        d->m_outputParserChain.reset();
        finish(false);
        return;
    }
    processStarted();
}

void AbstractProcessStep::doCancel()
{
    Core::Reaper::reap(d->m_process.release());
}

ProcessParameters *AbstractProcessStep::processParameters()
{
    return &d->m_param;
}

void AbstractProcessStep::cleanUp(QProcess *process)
{
    // The process has finished, leftover data is read in processFinished
    processFinished(process->exitCode(), process->exitStatus());
    const bool returnValue = processSucceeded(process->exitCode(), process->exitStatus()) || d->m_ignoreReturnValue;

    d->m_outputParserChain.reset();
    d->m_process.reset();

    // Report result
    finish(returnValue);
}

/*!
    Called after the process is started.

    The default implementation adds a process-started message to the output
    message.
*/

void AbstractProcessStep::processStarted()
{
    emit addOutput(tr("Starting: \"%1\" %2")
                   .arg(QDir::toNativeSeparators(d->m_param.effectiveCommand()),
                        d->m_param.prettyArguments()),
                   BuildStep::OutputFormat::NormalMessage);
}

/*!
    Called after the process is finished.

    The default implementation adds a line to the output window.
*/

void AbstractProcessStep::processFinished(int exitCode, QProcess::ExitStatus status)
{
    if (d->m_outputParserChain)
        d->m_outputParserChain->flush();

    QString command = QDir::toNativeSeparators(d->m_param.effectiveCommand());
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
                   .arg(QDir::toNativeSeparators(d->m_param.effectiveCommand()),
                        d->m_param.prettyArguments()),
                   BuildStep::OutputFormat::ErrorMessage);
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
    if (!d->m_process)
        return;
    d->m_process->setReadChannel(QProcess::StandardOutput);
    BuildConfiguration *bc = buildConfiguration();
    if (!bc)
        bc = target()->activeBuildConfiguration();
    const bool utf8Output = bc && bc->environment().hasKey("VSLANG");
    d->readData(&AbstractProcessStep::stdOutput, utf8Output);
}

void AbstractProcessStep::Private::readData(void (AbstractProcessStep::*func)(const QString &),
                                            bool isUtf8)
{
    while (m_process->bytesAvailable()) {
        const bool hasLine = m_process->canReadLine();
        const QByteArray data = hasLine ? m_process->readLine() : m_process->readAll();
        int startPos = 0;
        int crPos = -1;
        while ((crPos = data.indexOf('\r', startPos)) >= 0)  {
            if (data.size() > crPos + 1 && data.at(crPos + 1) == '\n')
                break;
            processLine(data.mid(startPos, crPos - startPos + 1), func, isUtf8);
            startPos = crPos + 1;
        }
        if (hasLine)
            processLine(data.mid(startPos), func, isUtf8);
        else if (startPos < data.count())
            deferredText += data.mid(startPos);
    }
}

void AbstractProcessStep::Private::processLine(const QByteArray &data,
                                               void (AbstractProcessStep::*func)(const QString &),
                                               bool isUtf8)
{
    const QByteArray text = deferredText + data;
    deferredText.clear();
    const QString line = isUtf8 ? QString::fromUtf8(text) : QString::fromLocal8Bit(text);
    (q->*func)(line);
}

/*!
    Called for each line of output on stdOut().

    The default implementation adds the line to the application output window.
*/

void AbstractProcessStep::stdOutput(const QString &line)
{
    if (d->m_outputParserChain)
        d->m_outputParserChain->stdOutput(line);
    emit addOutput(line, BuildStep::OutputFormat::Stdout, BuildStep::DontAppendNewline);
}

void AbstractProcessStep::processReadyReadStdError()
{
    if (!d->m_process)
        return;
    d->m_process->setReadChannel(QProcess::StandardError);
    d->readData(&AbstractProcessStep::stdError);
}

/*!
    Called for each line of output on StdErrror().

    The default implementation adds the line to the application output window.
*/

void AbstractProcessStep::stdError(const QString &line)
{
    if (d->m_outputParserChain)
        d->m_outputParserChain->stdError(line);
    emit addOutput(line, BuildStep::OutputFormat::Stderr, BuildStep::DontAppendNewline);
}

void AbstractProcessStep::finish(bool success)
{
    emit finished(success);
}

void AbstractProcessStep::taskAdded(const Task &task, int linkedOutputLines, int skipLines)
{
    // Do not bother to report issues if we do not care about the results of
    // the buildstep anyway:
    if (d->m_ignoreReturnValue)
        return;

    // flush out any pending tasks before proceeding:
    if (!d->m_skipFlush && d->m_outputParserChain) {
        d->m_skipFlush = true;
        d->m_outputParserChain->flush();
        d->m_skipFlush = false;
    }

    Task editable(task);
    QString filePath = task.file.toString();

    auto it = d->m_filesCache.find(filePath);
    if (it != d->m_filesCache.end()) {
        editable.file = it.value().first;
        it.value().second = ++d->m_cacheCounter;
    } else if (!filePath.isEmpty() && !filePath.startsWith('<') && !QDir::isAbsolutePath(filePath)) {
        // We have no save way to decide which file in which subfolder
        // is meant. Therefore we apply following heuristics:
        // 1. Check if file is unique in whole project
        // 2. Otherwise try again without any ../
        // 3. give up.

        QString sourceFilePath = filePath;
        Utils::FileNameList possibleFiles = d->m_candidates.value(Utils::FileName::fromString(filePath).fileName());

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
    QProcess *process = d->m_process.get();
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
    if (d->m_filesCache.size() <= limit)
        return;

    const quint64 minCounter = d->m_cacheCounter - static_cast<quint64>(limit);
    std::remove_if(d->m_filesCache.begin(), d->m_filesCache.end(),
                   [minCounter](const QPair<Utils::FileName, quint64> &entry) {
        return entry.second <= minCounter;
    });
}

void AbstractProcessStep::insertInCache(const QString &relativePath, const Utils::FileName &absPath)
{
    purgeCache(false);
    d->m_filesCache.insert(relativePath, qMakePair(absPath, ++d->m_cacheCounter));
}

} // namespace ProjectExplorer
