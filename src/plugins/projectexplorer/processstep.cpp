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
#include "buildstep.h"
#include "buildconfiguration.h"
#include "projectexplorerconstants.h"
#include "target.h"
#include "kit.h"

#include <coreplugin/variablechooser.h>

#include <utils/macroexpander.h>

#include <QDebug>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

namespace {
const char PROCESS_STEP_ID[] = "ProjectExplorer.ProcessStep";
const char PROCESS_COMMAND_KEY[] = "ProjectExplorer.ProcessStep.Command";
const char PROCESS_WORKINGDIRECTORY_KEY[] = "ProjectExplorer.ProcessStep.WorkingDirectory";
const char PROCESS_ARGUMENTS_KEY[] = "ProjectExplorer.ProcessStep.Arguments";
}

ProcessStep::ProcessStep(BuildStepList *bsl) : AbstractProcessStep(bsl, Core::Id(PROCESS_STEP_ID))
{
    ctor();
}

ProcessStep::ProcessStep(BuildStepList *bsl, ProcessStep *bs) : AbstractProcessStep(bsl, bs),
    m_command(bs->m_command),
    m_arguments(bs->m_arguments),
    m_workingDirectory(bs->m_workingDirectory)
{
    ctor();
}

void ProcessStep::ctor()
{
    //: Default ProcessStep display name
    setDefaultDisplayName(tr("Custom Process Step"));
    if (m_workingDirectory.isEmpty())
        m_workingDirectory = Constants::DEFAULT_WORKING_DIR;
}

bool ProcessStep::init(QList<const BuildStep *> &earlierSteps)
{
    BuildConfiguration *bc = buildConfiguration();
    if (!bc)
        bc = target()->activeBuildConfiguration();
    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc ? bc->macroExpander() : Utils::globalMacroExpander());
    pp->setEnvironment(bc ? bc->environment() : Utils::Environment::systemEnvironment());
    pp->setWorkingDirectory(workingDirectory());
    pp->setCommand(m_command);
    pp->setArguments(m_arguments);
    pp->resolveAll();

    setOutputParser(target()->kit()->createOutputParser());
    return AbstractProcessStep::init(earlierSteps);
}

void ProcessStep::run(QFutureInterface<bool> & fi)
{
    AbstractProcessStep::run(fi);
}

BuildStepConfigWidget *ProcessStep::createConfigWidget()
{
    return new ProcessStepConfigWidget(this);
}

bool ProcessStep::immutable() const
{
    return false;
}

QString ProcessStep::command() const
{
    return m_command;
}

QString ProcessStep::arguments() const
{
    return m_arguments;
}

QString ProcessStep::workingDirectory() const
{
    return m_workingDirectory;
}

void ProcessStep::setCommand(const QString &command)
{
    m_command = command;
}

void ProcessStep::setArguments(const QString &arguments)
{
    m_arguments = arguments;
}

void ProcessStep::setWorkingDirectory(const QString &workingDirectory)
{
    if (workingDirectory.isEmpty())
        if (target()->activeBuildConfiguration())
            m_workingDirectory = Constants::DEFAULT_WORKING_DIR;
        else
            m_workingDirectory = Constants::DEFAULT_WORKING_DIR_ALTERNATE;
    else
        m_workingDirectory = workingDirectory;
}

QVariantMap ProcessStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());
    map.insert(PROCESS_COMMAND_KEY, command());
    map.insert(PROCESS_ARGUMENTS_KEY, arguments());
    map.insert(PROCESS_WORKINGDIRECTORY_KEY, workingDirectory());
    return map;
}

bool ProcessStep::fromMap(const QVariantMap &map)
{
    setCommand(map.value(PROCESS_COMMAND_KEY).toString());
    setArguments(map.value(PROCESS_ARGUMENTS_KEY).toString());
    setWorkingDirectory(map.value(PROCESS_WORKINGDIRECTORY_KEY).toString());
    return AbstractProcessStep::fromMap(map);
}

//*******
// ProcessStepFactory
//*******

QList<BuildStepInfo> ProcessStepFactory::availableSteps(BuildStepList *parent) const
{
    Q_UNUSED(parent);
    return {{PROCESS_STEP_ID, ProcessStep::tr("Custom Process Step", "item in combobox")}};
}

BuildStep *ProcessStepFactory::create(BuildStepList *parent, Core::Id id)
{
    Q_UNUSED(id);
    return new ProcessStep(parent);
}

BuildStep *ProcessStepFactory::clone(BuildStepList *parent, BuildStep *bs)
{
    return new ProcessStep(parent, static_cast<ProcessStep *>(bs));
}

//*******
// ProcessStepConfigWidget
//*******

ProcessStepConfigWidget::ProcessStepConfigWidget(ProcessStep *step) :
    m_step(step)
{
    m_ui.setupUi(this);
    m_ui.command->setExpectedKind(Utils::PathChooser::Command);
    m_ui.command->setHistoryCompleter("PE.ProcessStepCommand.History");
    m_ui.workingDirectory->setExpectedKind(Utils::PathChooser::Directory);

    BuildConfiguration *bc = m_step->buildConfiguration();
    if (!bc)
        bc = m_step->target()->activeBuildConfiguration();
    Utils::Environment env = bc ? bc->environment() : Utils::Environment::systemEnvironment();
    m_ui.command->setEnvironment(env);
    m_ui.command->setPath(m_step->command());

    m_ui.workingDirectory->setEnvironment(env);
    m_ui.workingDirectory->setPath(m_step->workingDirectory());

    m_ui.commandArgumentsLineEdit->setText(m_step->arguments());

    updateDetails();

    connect(m_ui.command, &Utils::PathChooser::rawPathChanged,
            this, &ProcessStepConfigWidget::commandLineEditTextEdited);
    connect(m_ui.workingDirectory, &Utils::PathChooser::rawPathChanged,
            this, &ProcessStepConfigWidget::workingDirectoryLineEditTextEdited);

    connect(m_ui.commandArgumentsLineEdit, &QLineEdit::textEdited,
            this, &ProcessStepConfigWidget::commandArgumentsLineEditTextEdited);
    Core::VariableChooser::addSupportForChildWidgets(this, m_step->macroExpander());
}

void ProcessStepConfigWidget::updateDetails()
{
    QString displayName = m_step->displayName();
    if (displayName.isEmpty())
        displayName = tr("Custom Process Step");
    ProcessParameters param;
    BuildConfiguration *bc = m_step->buildConfiguration();
    if (!bc) // iff the step is actually in the deploy list
        bc = m_step->target()->activeBuildConfiguration();
    param.setMacroExpander(bc ? bc->macroExpander() : Utils::globalMacroExpander());
    param.setEnvironment(bc ? bc->environment() : Utils::Environment::systemEnvironment());

    param.setWorkingDirectory(m_step->workingDirectory());
    param.setCommand(m_step->command());
    param.setArguments(m_step->arguments());
    m_summaryText = param.summary(displayName);
    emit updateSummary();
}

QString ProcessStepConfigWidget::displayName() const
{
    return m_step->displayName();
}

QString ProcessStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

void ProcessStepConfigWidget::commandLineEditTextEdited()
{
    m_step->setCommand(m_ui.command->rawPath());
    updateDetails();
}

void ProcessStepConfigWidget::workingDirectoryLineEditTextEdited()
{
    m_step->setWorkingDirectory(m_ui.workingDirectory->rawPath());
}

void ProcessStepConfigWidget::commandArgumentsLineEditTextEdited()
{
    m_step->setArguments(m_ui.commandArgumentsLineEdit->text());
    updateDetails();
}
