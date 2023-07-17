// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstractprocessstep.h"

#include "processparameters.h"
#include "projectexplorer.h"
#include "projectexplorersettings.h"
#include "projectexplorertr.h"

#include <utils/outputformatter.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QTextDecoder>

#include <algorithm>
#include <memory>

using namespace Tasking;
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

    AbstractProcessStep *q;
    ProcessParameters m_param;
    ProcessParameters *m_displayedParams = &m_param;
    std::function<CommandLine()> m_commandLineProvider;
    std::function<FilePath()> m_workingDirectoryProvider;
    std::function<void(Environment &)> m_environmentModifier;
    bool m_ignoreReturnValue = false;
    bool m_lowPriority = false;
    std::unique_ptr<QTextDecoder> stdOutDecoder;
    std::unique_ptr<QTextDecoder> stdErrDecoder;
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
    if (!setupProcessParameters(processParameters()))
        return false;

    d->stdOutDecoder = std::make_unique<QTextDecoder>(buildEnvironment().hasKey("VSLANG")
        ? QTextCodec::codecForName("UTF-8") : QTextCodec::codecForLocale());
    d->stdErrDecoder = std::make_unique<QTextDecoder>(QTextCodec::codecForLocale());
    return true;
}

void AbstractProcessStep::setupOutputFormatter(OutputFormatter *formatter)
{
    formatter->setDemoteErrorsToWarnings(d->m_ignoreReturnValue);
    d->outputFormatter = formatter;
    BuildStep::setupOutputFormatter(formatter);
}

GroupItem AbstractProcessStep::defaultProcessTask()
{
    const auto onSetup = [this](Process &process) {
        return setupProcess(process) ? SetupResult::Continue : SetupResult::StopWithError;
    };
    const auto onEnd = [this](const Process &process) { handleProcessDone(process); };
    return ProcessTask(onSetup, onEnd, onEnd);
}

bool AbstractProcessStep::setupProcess(Process &process)
{
    const FilePath workingDir = d->m_param.effectiveWorkingDirectory();
    if (!workingDir.exists() && !workingDir.createDir()) {
        emit addOutput(Tr::tr("Could not create directory \"%1\"").arg(workingDir.toUserOutput()),
                       OutputFormat::ErrorMessage);
        return false;
    }
    if (!d->m_param.effectiveCommand().isExecutableFile()) {
        emit addOutput(Tr::tr("The program \"%1\" does not exist or is not executable.")
                           .arg(d->m_displayedParams->effectiveCommand().toUserOutput()),
                       OutputFormat::ErrorMessage);
        return false;
    }

    process.setUseCtrlCStub(HostOsInfo::isWindowsHost());
    process.setWorkingDirectory(workingDir);
    // Enforce PWD in the environment because some build tools use that.
    // PWD can be different from getcwd in case of symbolic links (getcwd resolves symlinks).
    // For example Clang uses PWD for paths in debug info, see QTCREATORBUG-23788
    Environment envWithPwd = d->m_param.environment();
    envWithPwd.set("PWD", workingDir.path());
    process.setEnvironment(envWithPwd);
    process.setCommand({d->m_param.effectiveCommand(), d->m_param.effectiveArguments(),
                        CommandLine::Raw});
    if (d->m_lowPriority && ProjectExplorerPlugin::projectExplorerSettings().lowBuildPriority)
        process.setLowPriority();

    connect(&process, &Process::readyReadStandardOutput, this, [this, &process] {
        emit addOutput(d->stdOutDecoder->toUnicode(process.readAllRawStandardOutput()),
                       OutputFormat::Stdout, DontAppendNewline);
    });
    connect(&process, &Process::readyReadStandardError, this, [this, &process] {
        emit addOutput(d->stdErrDecoder->toUnicode(process.readAllRawStandardError()),
                       OutputFormat::Stderr, DontAppendNewline);
    });
    connect(&process, &Process::started, this, [this] {
        ProcessParameters *params = d->m_displayedParams;
        emit addOutput(Tr::tr("Starting: \"%1\" %2")
                       .arg(params->effectiveCommand().toUserOutput(), params->prettyArguments()),
                       OutputFormat::NormalMessage);
    });
    return true;
}

void AbstractProcessStep::handleProcessDone(const Process &process)
{
    const QString command = d->m_displayedParams->effectiveCommand().toUserOutput();
    if (process.result() == ProcessResult::FinishedWithSuccess) {
        emit addOutput(Tr::tr("The process \"%1\" exited normally.").arg(command),
                       OutputFormat::NormalMessage);
    } else if (process.result() == ProcessResult::FinishedWithError) {
        emit addOutput(Tr::tr("The process \"%1\" exited with code %2.")
                           .arg(command, QString::number(process.exitCode())),
                       OutputFormat::ErrorMessage);
    } else if (process.result() == ProcessResult::StartFailed) {
        emit addOutput(Tr::tr("Could not start process \"%1\" %2.")
                           .arg(command, d->m_displayedParams->prettyArguments()),
                       OutputFormat::ErrorMessage);
        const QString errorString = process.errorString();
        if (!errorString.isEmpty())
            emit addOutput(errorString, OutputFormat::ErrorMessage);
    } else {
        emit addOutput(Tr::tr("The process \"%1\" crashed.").arg(command),
                       OutputFormat::ErrorMessage);
    }
}

void AbstractProcessStep::setLowPriority()
{
    d->m_lowPriority = true;
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

void AbstractProcessStep::setDisplayedParameters(ProcessParameters *params)
{
    d->m_displayedParams = params;
}

GroupItem AbstractProcessStep::runRecipe()
{
    return Group { ignoreReturnValue() ? finishAllAndDone : stopOnError, defaultProcessTask() };
}

} // namespace ProjectExplorer
