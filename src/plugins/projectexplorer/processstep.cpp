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

#include "processstep.h"

#include "abstractprocessstep.h"
#include "buildconfiguration.h"
#include "kit.h"
#include "processparameters.h"
#include "projectconfigurationaspects.h"
#include "projectexplorerconstants.h"
#include "projectexplorer_export.h"
#include "target.h"

#include <utils/fileutils.h>
#include <utils/outputformatter.h>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

const char PROCESS_COMMAND_KEY[] = "ProjectExplorer.ProcessStep.Command";
const char PROCESS_WORKINGDIRECTORY_KEY[] = "ProjectExplorer.ProcessStep.WorkingDirectory";
const char PROCESS_ARGUMENTS_KEY[] = "ProjectExplorer.ProcessStep.Arguments";

class ProcessStep final : public AbstractProcessStep
{
    Q_DECLARE_TR_FUNCTIONS(ProjectExplorer::ProcessStep)

public:
    ProcessStep(BuildStepList *bsl, Utils::Id id);

    bool init() final;
    void setupOutputFormatter(Utils::OutputFormatter *formatter);
    void setupProcessParameters(ProcessParameters *pp);

    BaseStringAspect *m_command;
    BaseStringAspect *m_arguments;
    BaseStringAspect *m_workingDirectory;
};

ProcessStep::ProcessStep(BuildStepList *bsl, Utils::Id id)
    : AbstractProcessStep(bsl, id)
{
    //: Default ProcessStep display name
    setDefaultDisplayName(tr("Custom Process Step"));

    m_command = addAspect<BaseStringAspect>();
    m_command->setSettingsKey(PROCESS_COMMAND_KEY);
    m_command->setDisplayStyle(BaseStringAspect::PathChooserDisplay);
    m_command->setLabelText(tr("Command:"));
    m_command->setExpectedKind(Utils::PathChooser::Command);
    m_command->setHistoryCompleter("PE.ProcessStepCommand.History");

    m_arguments = addAspect<BaseStringAspect>();
    m_arguments->setSettingsKey(PROCESS_ARGUMENTS_KEY);
    m_arguments->setDisplayStyle(BaseStringAspect::LineEditDisplay);
    m_arguments->setLabelText(tr("Arguments:"));

    m_workingDirectory = addAspect<BaseStringAspect>();
    m_workingDirectory->setSettingsKey(PROCESS_WORKINGDIRECTORY_KEY);
    m_workingDirectory->setValue(Constants::DEFAULT_WORKING_DIR);
    m_workingDirectory->setDisplayStyle(BaseStringAspect::PathChooserDisplay);
    m_workingDirectory->setLabelText(tr("Working directory:"));
    m_workingDirectory->setExpectedKind(Utils::PathChooser::Directory);

    setSummaryUpdater([this] {
        QString display = displayName();
        if (display.isEmpty())
            display = tr("Custom Process Step");
        ProcessParameters param;
        setupProcessParameters(&param);
        return param.summary(display);
    });

    addMacroExpander();
}

bool ProcessStep::init()
{
    setupProcessParameters(processParameters());
    return AbstractProcessStep::init();
}

void ProcessStep::setupOutputFormatter(OutputFormatter *formatter)
{
    formatter->addLineParsers(target()->kit()->createOutputParsers());
    AbstractProcessStep::setupOutputFormatter(formatter);
}

void ProcessStep::setupProcessParameters(ProcessParameters *pp)
{
    QString workingDirectory = m_workingDirectory->value();
    if (workingDirectory.isEmpty())
        workingDirectory = fallbackWorkingDirectory();

    pp->setMacroExpander(macroExpander());
    pp->setEnvironment(buildEnvironment());
    pp->setWorkingDirectory(FilePath::fromString(workingDirectory));
    pp->setCommandLine({m_command->filePath(), m_arguments->value(), CommandLine::Raw});
    pp->resolveAll();
}

// ProcessStepFactory

ProcessStepFactory::ProcessStepFactory()
{
    registerStep<ProcessStep>("ProjectExplorer.ProcessStep");
    setDisplayName(ProcessStep::tr("Custom Process Step", "item in combobox"));
}

} // Internal
} // ProjectExplorer
