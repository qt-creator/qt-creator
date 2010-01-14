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

namespace {
const char * const PROCESS_STEP_ID("ProjectExplorer.ProcessStep");

const char * const PROCESS_COMMAND_KEY("ProjectExplorer.ProcessStep.Command");
const char * const PROCESS_WORKINGDIRECTORY_KEY("ProjectExplorer.ProcessStep.WorkingDirectory");
const char * const PROCESS_ARGUMENTS_KEY("ProjectExplorer.ProcessStep.Arguments");
const char * const PROCESS_ENABLED_KEY("ProjectExplorer.ProcessStep.Enabled");
}

ProcessStep::ProcessStep(BuildConfiguration *bc) :
    AbstractProcessStep(bc, QLatin1String(PROCESS_STEP_ID))
{
    ctor();
}

ProcessStep::ProcessStep(BuildConfiguration *bc, const QString &id) :
    AbstractProcessStep(bc, id)
{
    ctor();
}

ProcessStep::ProcessStep(BuildConfiguration *bc, ProcessStep *bs) :
    AbstractProcessStep(bc, bs),
    m_name(bs->m_name),
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
    setDisplayName(tr("Custom Process Step", "item in combobox"));
}

ProcessStep::~ProcessStep()
{
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
    setOutputParser(0);
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
    setArguments(map.value(QLatin1String(PROCESS_ARGUMENTS_KEY)).toStringList());
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

bool ProcessStepFactory::canCreate(BuildConfiguration *parent, const QString &id) const
{
    Q_UNUSED(parent);
    return id == QLatin1String(PROCESS_STEP_ID);
}

BuildStep *ProcessStepFactory::create(BuildConfiguration *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    return new ProcessStep(parent);
}

bool ProcessStepFactory::canClone(BuildConfiguration *parent, BuildStep *bs) const
{
    return canCreate(parent, bs->id());
}

BuildStep *ProcessStepFactory::clone(BuildConfiguration *parent, BuildStep *bs)
{
    if (!canClone(parent, bs))
        return 0;
    return new ProcessStep(parent, static_cast<ProcessStep *>(bs));
}

bool ProcessStepFactory::canRestore(BuildConfiguration *parent, const QVariantMap &map) const
{
    QString id(ProjectExplorer::idFromMap(map));
    return canCreate(parent, id);
}

BuildStep *ProcessStepFactory::restore(BuildConfiguration *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;

    ProcessStep *bs(new ProcessStep(parent));
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QStringList ProcessStepFactory::availableCreationIds(BuildConfiguration *parent) const
{
    Q_UNUSED(parent)
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
    return m_step->displayName();
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
