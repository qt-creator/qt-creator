// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstractprocessstep.h"

#include "buildconfiguration.h"
#include "buildstep.h"
#include "processparameters.h"
#include "projectexplorer.h"
#include "projectexplorersettings.h"
#include "projectexplorertr.h"

#include <utils/fileutils.h>
#include <utils/outputformatter.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QTextDecoder>

#include <algorithm>
#include <memory>

using namespace Utils;

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

    void cleanUp(int exitCode, QProcess::ExitStatus status);

    AbstractProcessStep *q;
    std::unique_ptr<QtcProcess> m_process;
    std::unique_ptr<TaskTree> m_taskTree;
    ProcessParameters m_param;
    ProcessParameters *m_displayedParams = &m_param;
    std::function<CommandLine()> m_commandLineProvider;
    std::function<FilePath()> m_workingDirectoryProvider;
    std::function<void(Environment &)> m_environmentModifier;
    bool m_ignoreReturnValue = false;
    bool m_lowPriority = false;
    std::unique_ptr<QTextDecoder> stdoutStream;
    std::unique_ptr<QTextDecoder> stderrStream;
    OutputFormatter *outputFormatter = nullptr;
};

AbstractProcessStep::AbstractProcessStep(BuildStepList *bsl, Id id) :
    BuildStep(bsl, id),
    d(new Private(this))
{
}

AbstractProcessStep::~AbstractProcessStep()
{
    delete d;
}

void AbstractProcessStep::emitFaultyConfigurationMessage()
{
    emit addOutput(Tr::tr("Configuration is faulty. Check the Issues view for details."),
                   OutputFormat::NormalMessage);
}

bool AbstractProcessStep::ignoreReturnValue() const
{
    return d->m_ignoreReturnValue;
}

/*!
    If \a ignoreReturnValue is set to true, then the abstractprocess step will
    return success even if the return value indicates otherwise.
*/

void AbstractProcessStep::setIgnoreReturnValue(bool b)
{
    d->m_ignoreReturnValue = b;
}

void AbstractProcessStep::setEnvironmentModifier(const std::function<void (Environment &)> &modifier)
{
    d->m_environmentModifier = modifier;
}

void AbstractProcessStep::setUseEnglishOutput()
{
    d->m_environmentModifier = [](Environment &env) { env.setupEnglishOutput(); };
}

void AbstractProcessStep::setCommandLineProvider(const std::function<CommandLine()> &provider)
{
    d->m_commandLineProvider = provider;
}

void AbstractProcessStep::setWorkingDirectoryProvider(const std::function<FilePath()> &provider)
{
    d->m_workingDirectoryProvider = provider;
}

/*!
    Reimplemented from BuildStep::init(). You need to call this from
    YourBuildStep::init().
*/

bool AbstractProcessStep::init()
{
    if (d->m_process || d->m_taskTree)
        return false;

    if (!setupProcessParameters(processParameters()))
        return false;

    return true;
}

void AbstractProcessStep::setupOutputFormatter(OutputFormatter *formatter)
{
    formatter->setDemoteErrorsToWarnings(d->m_ignoreReturnValue);
    d->outputFormatter = formatter;
    BuildStep::setupOutputFormatter(formatter);
}

/*!
    Reimplemented from BuildStep::init(). You need to call this from
    YourBuildStep::run().
*/

void AbstractProcessStep::doRun()
{
    if (!checkWorkingDirectory())
        return;

    if (!d->m_param.effectiveCommand().isExecutableFile()) {
        processStartupFailed();
        return;
    }

    setupStreams();

    d->m_process.reset(new QtcProcess);
    setupProcess(d->m_process.get());
    connect(d->m_process.get(), &QtcProcess::done, this, &AbstractProcessStep::handleProcessDone);
    d->m_process->start();
}

bool AbstractProcessStep::checkWorkingDirectory()
{
    const FilePath wd = d->m_param.effectiveWorkingDirectory();
    if (!wd.exists()) {
        if (!wd.createDir()) {
            emit addOutput(Tr::tr("Could not create directory \"%1\"").arg(wd.toUserOutput()),
                           OutputFormat::ErrorMessage);
            finish(ProcessResult::StartFailed);
            return false;
        }
    }
    return true;
}

void AbstractProcessStep::setupStreams()
{
    d->stdoutStream = std::make_unique<QTextDecoder>(buildEnvironment().hasKey("VSLANG")
            ? QTextCodec::codecForName("UTF-8") : QTextCodec::codecForLocale());
    d->stderrStream = std::make_unique<QTextDecoder>(QTextCodec::codecForLocale());
}

void AbstractProcessStep::setupProcess(QtcProcess *process)
{
    process->setUseCtrlCStub(HostOsInfo::isWindowsHost());
    process->setWorkingDirectory(d->m_param.effectiveWorkingDirectory());
    // Enforce PWD in the environment because some build tools use that.
    // PWD can be different from getcwd in case of symbolic links (getcwd resolves symlinks).
    // For example Clang uses PWD for paths in debug info, see QTCREATORBUG-23788
    Environment envWithPwd = d->m_param.environment();
    envWithPwd.set("PWD", process->workingDirectory().path());
    process->setEnvironment(envWithPwd);
    process->setCommand({d->m_param.effectiveCommand(), d->m_param.effectiveArguments(),
                        CommandLine::Raw});
    if (d->m_lowPriority && ProjectExplorerPlugin::projectExplorerSettings().lowBuildPriority)
        process->setLowPriority();

    connect(process, &QtcProcess::readyReadStandardOutput, this, [this, process] {
        emit addOutput(d->stdoutStream->toUnicode(process->readAllRawStandardOutput()),
                       OutputFormat::Stdout, DontAppendNewline);
    });
    connect(process, &QtcProcess::readyReadStandardError, this, [this, process] {
        emit addOutput(d->stderrStream->toUnicode(process->readAllRawStandardError()),
                       OutputFormat::Stderr, DontAppendNewline);
    });
    connect(process, &QtcProcess::started, this, [this] {
        ProcessParameters *params = displayedParameters();
        emit addOutput(Tr::tr("Starting: \"%1\" %2")
                       .arg(params->effectiveCommand().toUserOutput(), params->prettyArguments()),
                       OutputFormat::NormalMessage);
    });
}

void AbstractProcessStep::runTaskTree(const Tasking::Group &recipe)
{
    setupStreams();

    d->m_taskTree.reset(new TaskTree(recipe));
    connect(d->m_taskTree.get(), &TaskTree::progressValueChanged, this, [this](int value) {
        emit progress(qRound(double(value) * 100 / std::max(d->m_taskTree->progressMaximum(), 1)), {});
    });
    connect(d->m_taskTree.get(), &TaskTree::done, this, [this] {
        emit finished(true);
        d->m_taskTree.release()->deleteLater();
    });
    connect(d->m_taskTree.get(), &TaskTree::errorOccurred, this, [this] {
        emit finished(false);
        d->m_taskTree.release()->deleteLater();
    });
    d->m_taskTree->start();
}

void AbstractProcessStep::setLowPriority()
{
    d->m_lowPriority = true;
}

void AbstractProcessStep::doCancel()
{
    if (d->m_process) {
        d->cleanUp(-1, QProcess::CrashExit);
    }
    if (d->m_taskTree) {
        d->m_taskTree.reset();
        emit addOutput(Tr::tr("The build step was ended forcefully."), OutputFormat::ErrorMessage);
        emit finished(false);
    }
}

ProcessParameters *AbstractProcessStep::processParameters()
{
    return &d->m_param;
}

bool AbstractProcessStep::setupProcessParameters(ProcessParameters *params) const
{
    params->setMacroExpander(macroExpander());

    Environment env = buildEnvironment();
    if (d->m_environmentModifier)
        d->m_environmentModifier(env);
    params->setEnvironment(env);

    if (d->m_commandLineProvider)
        params->setCommandLine(d->m_commandLineProvider());

    FilePath workingDirectory;
    if (d->m_workingDirectoryProvider)
        workingDirectory = d->m_workingDirectoryProvider();
    else
        workingDirectory = buildDirectory();

    const FilePath executable = params->effectiveCommand();

    // E.g. the QMakeStep doesn't have set up anything when this is called
    // as it doesn't set a command line provider, so executable might be empty.
    const bool looksGood = executable.isEmpty() || executable.ensureReachable(workingDirectory);
    QTC_ASSERT(looksGood, return false);

    params->setWorkingDirectory(executable.withNewPath(workingDirectory.path()));

    return true;
}

ProcessParameters *AbstractProcessStep::displayedParameters() const
{
    return d->m_displayedParams;
}

void AbstractProcessStep::setDisplayedParameters(ProcessParameters *params)
{
    d->m_displayedParams = params;
}

void AbstractProcessStep::Private::cleanUp(int exitCode, QProcess::ExitStatus status)
{
    const QString command = q->displayedParameters()->effectiveCommand().toUserOutput();
    if (status == QProcess::NormalExit && exitCode == 0) {
        emit q->addOutput(Tr::tr("The process \"%1\" exited normally.").arg(command),
                          OutputFormat::NormalMessage);
    } else if (status == QProcess::NormalExit) {
        emit q->addOutput(Tr::tr("The process \"%1\" exited with code %2.")
                          .arg(command, QString::number(exitCode)),
                          OutputFormat::ErrorMessage);
    } else {
        emit q->addOutput(Tr::tr("The process \"%1\" crashed.").arg(command),
                          OutputFormat::ErrorMessage);
    }

    if (m_process)
        m_process.release()->deleteLater();
    const ProcessResult result = (status == QProcess::NormalExit && exitCode == 0
                                  && !outputFormatter->hasFatalErrors())
            ? ProcessResult::FinishedWithSuccess : ProcessResult::FinishedWithError;
    q->finish(result);
}

/*!
    Called if the process could not be started.

    By default, adds a message to the output window.
*/

void AbstractProcessStep::processStartupFailed()
{
    ProcessParameters *params = displayedParameters();
    emit addOutput(Tr::tr("Could not start process \"%1\" %2.")
                   .arg(params->effectiveCommand().toUserOutput(), params->prettyArguments()),
                   OutputFormat::ErrorMessage);

    const QString err = d->m_process ? d->m_process->errorString() : QString();
    if (!err.isEmpty())
        emit addOutput(err, OutputFormat::ErrorMessage);
    finish(ProcessResult::StartFailed);
}

bool AbstractProcessStep::isSuccess(ProcessResult result) const
{
    return result == ProcessResult::FinishedWithSuccess
            || (result == ProcessResult::FinishedWithError && d->m_ignoreReturnValue);
}

void AbstractProcessStep::finish(ProcessResult result)
{
    emit finished(isSuccess(result));
}

void AbstractProcessStep::handleProcessDone()
{
    QTC_ASSERT(d->m_process.get(), return);
    if (d->m_process->error() == QProcess::FailedToStart) {
        processStartupFailed();
        d->m_process.release()->deleteLater();
        return;
    }
    d->cleanUp(d->m_process->exitCode(), d->m_process->exitStatus());
}

} // namespace ProjectExplorer
