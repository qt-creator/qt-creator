// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "processstep.h"

#include "abstractprocessstep.h"
#include "buildconfiguration.h"
#include "kit.h"
#include "processparameters.h"
#include "projectexplorerconstants.h"
#include "projectexplorertr.h"

#include <utils/aspects.h>
#include <utils/outputformatter.h>

using namespace Utils;

namespace ProjectExplorer::Internal {

const char PROCESS_COMMAND_KEY[] = "ProjectExplorer.ProcessStep.Command";
const char PROCESS_WORKINGDIRECTORY_KEY[] = "ProjectExplorer.ProcessStep.WorkingDirectory";
const char PROCESS_WORKINGDIRECTORYRELATIVEBASE_KEY[]
    = "ProjectExplorer.ProcessStep.WorkingDirectoryRelativeBasePath";
const char PROCESS_ARGUMENTS_KEY[] = "ProjectExplorer.ProcessStep.Arguments";

ProcessStep::ProcessStep(BuildStepList *bsl, Id id)
    : AbstractProcessStep(bsl, id)
{
    m_command.setSettingsKey(PROCESS_COMMAND_KEY);
    m_command.setLabelText(Tr::tr("Command:"));
    m_command.setExpectedKind(PathChooser::Command);
    m_command.setHistoryCompleter("PE.ProcessStepCommand.History");

    m_arguments.setSettingsKey(PROCESS_ARGUMENTS_KEY);
    m_arguments.setDisplayStyle(StringAspect::LineEditDisplay);
    m_arguments.setLabelText(Tr::tr("Arguments:"));

    m_workingDirectory.setSettingsKey(PROCESS_WORKINGDIRECTORY_KEY);
    m_workingDirectory.setValue(QString(Constants::DEFAULT_WORKING_DIR));
    m_workingDirectory.setLabelText(Tr::tr("Working directory:"));
    m_workingDirectory.setExpectedKind(PathChooser::Directory);

    m_workingDirRelativeBasePath.setSettingsKey(PROCESS_WORKINGDIRECTORYRELATIVEBASE_KEY);
    m_workingDirRelativeBasePath.setValue(QString());
    m_workingDirRelativeBasePath.setVisible(false);
    m_workingDirRelativeBasePath.setExpectedKind(PathChooser::Directory);

    setWorkingDirectoryProvider([this] {
        const FilePath workingDir = m_workingDirectory();
        const FilePath relativeBasePath = m_workingDirRelativeBasePath();
        if (workingDir.isEmpty())
            return FilePath::fromString(fallbackWorkingDirectory());
        else if (workingDir.isRelativePath() && !relativeBasePath.isEmpty())
            return relativeBasePath.resolvePath(workingDir);
        return workingDir;
    });

    setCommandLineProvider([this] {
        return CommandLine{m_command(), m_arguments(), CommandLine::Raw};
    });

    setSummaryUpdater([this] {
        QString display = displayName();
        if (display.isEmpty())
            display = Tr::tr("Custom Process Step");
        ProcessParameters param;
        setupProcessParameters(&param);
        return param.summary(display);
    });
}

void ProcessStep::setCommand(const Utils::FilePath &command)
{
    m_command.setValue(command);
}

void ProcessStep::setArguments(const QStringList &arguments)
{
    m_arguments.setValue(arguments.join(" "));
}

void ProcessStep::setWorkingDirectory(
    const Utils::FilePath &workingDirectory, const Utils::FilePath &relativeBasePath)
{
    m_workingDirectory.setValue(workingDirectory);
    m_workingDirRelativeBasePath.setValue(relativeBasePath);
}

void ProcessStep::setupOutputFormatter(OutputFormatter *formatter)
{
    formatter->addLineParsers(kit()->createOutputParsers());
    AbstractProcessStep::setupOutputFormatter(formatter);
}

// ProcessStepFactory

ProcessStepFactory::ProcessStepFactory()
{
    registerStep<ProcessStep>(Constants::CUSTOM_PROCESS_STEP);
    //: Default ProcessStep display name
    setDisplayName(Tr::tr("Custom Process Step"));
}

} // ProjectExplorer::Internal
