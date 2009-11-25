/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "processstep.h"
#include "buildstep.h"
#include "project.h"
#include "buildconfiguration.h"

#include <coreplugin/ifile.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtGui/QFileDialog>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

static const char * const PROCESS_COMMAND          = "abstractProcess.command";
static const char * const PROCESS_WORKINGDIRECTORY = "abstractProcess.workingDirectory";
static const char * const PROCESS_ARGUMENTS        = "abstractProcess.arguments";
static const char * const PROCESS_ENABLED          = "abstractProcess.enabled";

ProcessStep::ProcessStep(BuildConfiguration *bc)
    : AbstractProcessStep(bc)
{

}

ProcessStep::ProcessStep(ProcessStep *bs, BuildConfiguration *bc)
    : AbstractProcessStep(bs, bc)
{
    m_name = bs->m_name;
    m_command = bs->m_command;
    m_arguments = bs->m_arguments;
    m_workingDirectory = bs->m_workingDirectory;
    m_env = bs->m_env;
    m_enabled = bs->m_enabled;
}

bool ProcessStep::init()
{
    setEnvironment(buildConfiguration()->environment());
    QString wd = workingDirectory();
    if (wd.isEmpty())
        wd = "$BUILDDIR";

    AbstractProcessStep::setWorkingDirectory(wd.replace("$BUILDDIR", buildConfiguration()->buildDirectory()));
    AbstractProcessStep::setCommand(m_command);
    AbstractProcessStep::setEnabled(m_enabled);
    AbstractProcessStep::setArguments(m_arguments);
    return AbstractProcessStep::init();
}

void ProcessStep::run(QFutureInterface<bool> & fi)
{
    return AbstractProcessStep::run(fi);
}

QString ProcessStep::name()
{
    return "projectexplorer.processstep";
}

void ProcessStep::restoreFromGlobalMap(const QMap<QString, QVariant> &map)
{
    QMap<QString, QVariant>::const_iterator it = map.constFind("ProjectExplorer.ProcessStep.DisplayName");
    if (it != map.constEnd())
        m_name = (*it).toString();
    ProjectExplorer::AbstractProcessStep::restoreFromGlobalMap(map);
}

void ProcessStep::restoreFromLocalMap(const QMap<QString, QVariant> &map)
{
    // TODO checking for PROCESS_*
    setCommand(map.value(PROCESS_COMMAND).toString());
    setWorkingDirectory(map.value(PROCESS_WORKINGDIRECTORY).toString());
    setArguments(map.value(PROCESS_ARGUMENTS).toStringList());
    setEnabled(map.value(PROCESS_ENABLED).toBool());

    QMap<QString, QVariant>::const_iterator it = map.constFind("ProjectExplorer.ProcessStep.DisplayName");
    if (it != map.constEnd())
        m_name = (*it).toString();

    ProjectExplorer::AbstractProcessStep::restoreFromLocalMap(map);
}

void ProcessStep::storeIntoLocalMap(QMap<QString, QVariant> &map)
{
    map[PROCESS_COMMAND] = command();
    map[PROCESS_WORKINGDIRECTORY] = workingDirectory();
    map[PROCESS_ARGUMENTS] = arguments();
    map[PROCESS_ENABLED] = enabled();
    map["ProjectExplorer.ProcessStep.DisplayName"] = m_name;
    ProjectExplorer::AbstractProcessStep::storeIntoLocalMap(map);
}


void ProcessStep::setDisplayName(const QString &name)
{
    if (name.isEmpty())
        m_name = QString::null;
    else
        m_name = name;
}

QString ProcessStep::displayName()
{
    if (!m_name.isEmpty())
        return m_name;
    else
        return tr("Custom Process Step");
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

QStringList ProcessStep::arguments() const
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

void ProcessStep::setArguments(const QStringList &arguments)
{
    m_arguments = arguments;
}

void ProcessStep::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

void ProcessStep::setWorkingDirectory(const QString &workingDirectory)
{
    m_workingDirectory = workingDirectory;
}

//*******
// ProcessStepFactory
//*******

ProcessStepFactory::ProcessStepFactory()
{

}

bool ProcessStepFactory::canCreate(const QString &name) const
{
    return name == "projectexplorer.processstep";
}

BuildStep *ProcessStepFactory::create(BuildConfiguration *bc, const QString &name) const
{
    Q_UNUSED(name)
    return new ProcessStep(bc);
}

BuildStep *ProcessStepFactory::clone(BuildStep *bs, BuildConfiguration *bc) const
{
    return new ProcessStep(static_cast<ProcessStep *>(bs), bc);
}

QStringList ProcessStepFactory::canCreateForProject(BuildConfiguration *bc) const
{
    Q_UNUSED(bc)
    return QStringList()<<"projectexplorer.processstep";
}
QString ProcessStepFactory::displayNameForName(const QString &name) const
{
    Q_UNUSED(name)
    return ProcessStep::tr("Custom Process Step", "item in combobox");
}

//*******
// ProcessStepConfigWidget
//*******

ProcessStepConfigWidget::ProcessStepConfigWidget(ProcessStep *step)
        : m_step(step)
{
    m_ui.setupUi(this);
    m_ui.command->setExpectedKind(Utils::PathChooser::File);
    connect(m_ui.command, SIGNAL(changed(QString)),
            this, SLOT(commandLineEditTextEdited()));
    connect(m_ui.workingDirectory, SIGNAL(changed(QString)),
            this, SLOT(workingDirectoryLineEditTextEdited()));

    connect(m_ui.nameLineEdit, SIGNAL(textEdited(const QString&)),
            this, SLOT(nameLineEditTextEdited()));
    connect(m_ui.commandArgumentsLineEdit, SIGNAL(textEdited(const QString&)),
            this, SLOT(commandArgumentsLineEditTextEdited()));
    connect(m_ui.enabledCheckBox, SIGNAL(clicked(bool)),
            this, SLOT(enabledCheckBoxClicked(bool)));
}

void ProcessStepConfigWidget::updateDetails()
{
    QString displayName = m_step->displayName();
    if (displayName.isEmpty())
        displayName = "Custom Process Step";
    m_summaryText = tr("<b>%1</b> %2 %3 %4")
                    .arg(displayName,
                         m_step->command(),
                         m_step->arguments().join(" "),
                         m_step->enabled() ? "" : tr("(disabled)"));
    emit updateSummary();
}

QString ProcessStepConfigWidget::displayName() const
{
    return m_step->name();
}

void ProcessStepConfigWidget::init()
{
    m_ui.command->setPath(m_step->command());

    QString workingDirectory = m_step->workingDirectory();
    if (workingDirectory.isEmpty())
        workingDirectory = "$BUILDDIR";
    m_ui.workingDirectory->setPath(workingDirectory);

    m_ui.commandArgumentsLineEdit->setText(m_step->arguments().join(" "));
    m_ui.enabledCheckBox->setChecked(m_step->enabled());

    m_ui.nameLineEdit->setText(m_step->displayName());
    updateDetails();
}

QString ProcessStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

void ProcessStepConfigWidget::nameLineEditTextEdited()
{
    m_step->setDisplayName(m_ui.nameLineEdit->text());
    emit updateDetails();
}

void ProcessStepConfigWidget::commandLineEditTextEdited()
{
    m_step->setCommand(m_ui.command->path());
    updateDetails();
}

void ProcessStepConfigWidget::workingDirectoryLineEditTextEdited()
{
    m_step->setWorkingDirectory(m_ui.workingDirectory->path());
}

void ProcessStepConfigWidget::commandArgumentsLineEditTextEdited()
{
    m_step->setArguments(m_ui.commandArgumentsLineEdit->text().split(" ",
          QString::SkipEmptyParts));
    updateDetails();
}

void ProcessStepConfigWidget::enabledCheckBoxClicked(bool)
{
    m_step->setEnabled(m_ui.enabledCheckBox->isChecked());
    updateDetails();
}
