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
const char PROCESS_ARGUMENTS_KEY[] = "ProjectExplorer.ProcessStep.Arguments";

class ProcessStep final : public AbstractProcessStep
{
public:
    ProcessStep(BuildStepList *bsl, Id id)
        : AbstractProcessStep(bsl, id)
    {
        command.setSettingsKey(PROCESS_COMMAND_KEY);
        command.setLabelText(Tr::tr("Command:"));
        command.setExpectedKind(PathChooser::Command);
        command.setHistoryCompleter("PE.ProcessStepCommand.History");

        arguments.setSettingsKey(PROCESS_ARGUMENTS_KEY);
        arguments.setDisplayStyle(StringAspect::LineEditDisplay);
        arguments.setLabelText(Tr::tr("Arguments:"));

        workingDirectory.setSettingsKey(PROCESS_WORKINGDIRECTORY_KEY);
        workingDirectory.setValue(QString(Constants::DEFAULT_WORKING_DIR));
        workingDirectory.setLabelText(Tr::tr("Working directory:"));
        workingDirectory.setExpectedKind(PathChooser::Directory);

        setWorkingDirectoryProvider([this] {
            const FilePath workingDir = workingDirectory();
            if (workingDir.isEmpty())
                return FilePath::fromString(fallbackWorkingDirectory());
            return workingDir;
        });

        setCommandLineProvider([this] {
            return CommandLine{command(), arguments(), CommandLine::Raw};
        });

        setSummaryUpdater([this] {
            QString display = displayName();
            if (display.isEmpty())
                display = Tr::tr("Custom Process Step");
            ProcessParameters param;
            setupProcessParameters(&param);
            return param.summary(display);
        });

        addMacroExpander();
    }

private:
    void setupOutputFormatter(OutputFormatter *formatter) final
    {
        formatter->addLineParsers(kit()->createOutputParsers());
        AbstractProcessStep::setupOutputFormatter(formatter);
    }

    FilePathAspect command{this};
    StringAspect arguments{this};
    FilePathAspect workingDirectory{this};
};

// ProcessStepFactory

ProcessStepFactory::ProcessStepFactory()
{
    registerStep<ProcessStep>("ProjectExplorer.ProcessStep");
    //: Default ProcessStep display name
    setDisplayName(Tr::tr("Custom Process Step"));
}

} // ProjectExplorer::Internal
