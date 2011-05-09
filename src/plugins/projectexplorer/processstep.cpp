/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "processstep.h"
#include "buildstep.h"
#include "project.h"
#include "buildconfiguration.h"
#include "projectexplorerconstants.h"

#include <coreplugin/ifile.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtGui/QFileDialog>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

namespace {
const char * const PROCESS_STEP_ID("ProjectExplorer.ProcessStep");

const char * const PROCESS_COMMAND_KEY("ProjectExplorer.ProcessStep.Command");
const char * const PROCESS_WORKINGDIRECTORY_KEY("ProjectExplorer.ProcessStep.WorkingDirectory");
const char * const PROCESS_ARGUMENTS_KEY("ProjectExplorer.ProcessStep.Arguments");
const char * const PROCESS_ENABLED_KEY("ProjectExplorer.ProcessStep.Enabled");
}

ProcessStep::ProcessStep(BuildStepList *bsl) :
    AbstractProcessStep(bsl, QLatin1String(PROCESS_STEP_ID))
{
    ctor();
}

ProcessStep::ProcessStep(BuildStepList *bsl, const QString &id) :
    AbstractProcessStep(bsl, id)
{
    ctor();
}

ProcessStep::ProcessStep(BuildStepList *bsl, ProcessStep *bs) :
    AbstractProcessStep(bsl, bs),
    m_command(bs->m_command),
    m_arguments(bs->m_arguments),
    m_workingDirectory(bs->m_workingDirectory),
    m_env(bs->m_env),
    m_enabled(bs->m_enabled)
{
    ctor();
}

void ProcessStep::ctor()
{
    //: Default ProcessStep display name
    setDefaultDisplayName(tr("Custom Process Step"));
    if (m_workingDirectory.isEmpty())
        m_workingDirectory = QLatin1String(ProjectExplorer::Constants::DEFAULT_WORKING_DIR);
}

ProcessStep::~ProcessStep()
{
}

bool ProcessStep::init()
{
    BuildConfiguration *bc = buildConfiguration();
    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setEnvironment(bc->environment());
    pp->setWorkingDirectory(workingDirectory());
    pp->setCommand(m_command);
    pp->setArguments(m_arguments);
    AbstractProcessStep::setEnabled(m_enabled);
    setOutputParser(bc->createOutputParser());
    return AbstractProcessStep::init();
}

void ProcessStep::run(QFutureInterface<bool> & fi)
{
    return AbstractProcessStep::run(fi);
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

bool ProcessStep::enabled() const
{
    return m_enabled;
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

void ProcessStep::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

void ProcessStep::setWorkingDirectory(const QString &workingDirectory)
{
    if (workingDirectory.isEmpty())
        m_workingDirectory = QLatin1String(ProjectExplorer::Constants::DEFAULT_WORKING_DIR);
    else
        m_workingDirectory = workingDirectory;
}

QVariantMap ProcessStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());
    map.insert(QLatin1String(PROCESS_COMMAND_KEY), command());
    map.insert(QLatin1String(PROCESS_ARGUMENTS_KEY), arguments());
    map.insert(QLatin1String(PROCESS_WORKINGDIRECTORY_KEY), workingDirectory());
    map.insert(QLatin1String(PROCESS_ENABLED_KEY), enabled());

    return map;
}

bool ProcessStep::fromMap(const QVariantMap &map)
{
    setCommand(map.value(QLatin1String(PROCESS_COMMAND_KEY)).toString());
    setArguments(map.value(QLatin1String(PROCESS_ARGUMENTS_KEY)).toString());
    setWorkingDirectory(map.value(QLatin1String(PROCESS_WORKINGDIRECTORY_KEY)).toString());
    setEnabled(map.value(QLatin1String(PROCESS_ENABLED_KEY), false).toBool());
    return AbstractProcessStep::fromMap(map);
}

//*******
// ProcessStepFactory
//*******

ProcessStepFactory::ProcessStepFactory()
{
}

ProcessStepFactory::~ProcessStepFactory()
{
}

bool ProcessStepFactory::canCreate(BuildStepList *parent, const QString &id) const
{
    Q_UNUSED(parent);
    return id == QLatin1String(PROCESS_STEP_ID);
}

BuildStep *ProcessStepFactory::create(BuildStepList *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    return new ProcessStep(parent);
}

bool ProcessStepFactory::canClone(BuildStepList *parent, BuildStep *bs) const
{
    return canCreate(parent, bs->id());
}

BuildStep *ProcessStepFactory::clone(BuildStepList *parent, BuildStep *bs)
{
    if (!canClone(parent, bs))
        return 0;
    return new ProcessStep(parent, static_cast<ProcessStep *>(bs));
}

bool ProcessStepFactory::canRestore(BuildStepList *parent, const QVariantMap &map) const
{
    QString id(ProjectExplorer::idFromMap(map));
    return canCreate(parent, id);
}

BuildStep *ProcessStepFactory::restore(BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    ProcessStep *bs(new ProcessStep(parent));
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QStringList ProcessStepFactory::availableCreationIds(BuildStepList *parent) const
{
    Q_UNUSED(parent);
    return QStringList() << QLatin1String(PROCESS_STEP_ID);
}
QString ProcessStepFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(PROCESS_STEP_ID))
        return ProcessStep::tr("Custom Process Step", "item in combobox");
    return QString();
}

//*******
// ProcessStepConfigWidget
//*******

ProcessStepConfigWidget::ProcessStepConfigWidget(ProcessStep *step)
        : m_step(step)
{
    m_ui.setupUi(this);
    m_ui.command->setExpectedKind(Utils::PathChooser::Command);
    m_ui.workingDirectory->setExpectedKind(Utils::PathChooser::Directory);
    connect(m_ui.command, SIGNAL(changed(QString)),
            this, SLOT(commandLineEditTextEdited()));
    connect(m_ui.workingDirectory, SIGNAL(changed(QString)),
            this, SLOT(workingDirectoryLineEditTextEdited()));

    connect(m_ui.commandArgumentsLineEdit, SIGNAL(textEdited(const QString&)),
            this, SLOT(commandArgumentsLineEditTextEdited()));
    connect(m_ui.enabledCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(enabledCheckBoxClicked(bool)));
}

void ProcessStepConfigWidget::updateDetails()
{
    QString displayName = m_step->displayName();
    if (displayName.isEmpty())
        displayName = tr("Custom Process Step");
    ProcessParameters param;
    param.setMacroExpander(m_step->buildConfiguration()->macroExpander());
    param.setEnvironment(m_step->buildConfiguration()->environment());
    param.setWorkingDirectory(m_step->workingDirectory());
    param.setCommand(m_step->command());
    param.setArguments(m_step->arguments());
    m_summaryText = param.summary(displayName);
    if (!m_step->enabled()) {
        //: %1 is the custom process step summary
        m_summaryText = tr("%1 (disabled)").arg(m_summaryText);
    }
    emit updateSummary();
}

QString ProcessStepConfigWidget::displayName() const
{
    return m_step->displayName();
}

void ProcessStepConfigWidget::init()
{
    m_ui.command->setEnvironment(m_step->buildConfiguration()->environment());
    m_ui.command->setPath(m_step->command());

    m_ui.workingDirectory->setEnvironment(m_step->buildConfiguration()->environment());
    m_ui.workingDirectory->setPath(m_step->workingDirectory());

    m_ui.commandArgumentsLineEdit->setText(m_step->arguments());
    m_ui.enabledCheckBox->setChecked(m_step->enabled());

    updateDetails();
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

void ProcessStepConfigWidget::enabledCheckBoxClicked(bool)
{
    m_step->setEnabled(m_ui.enabledCheckBox->isChecked());
    updateDetails();
}
