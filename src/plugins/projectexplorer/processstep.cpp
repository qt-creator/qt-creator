/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "processstep.h"
#include "buildstep.h"
#include "project.h"

#include <coreplugin/ifile.h>

#include <QtCore/QDebug>
#include <QtGui/QFileDialog>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

ProcessStep::ProcessStep(Project *pro)
        : AbstractProcessStep(pro)
{

}

bool ProcessStep::init(const QString &buildConfiguration)
{
    setEnvironment(buildConfiguration, project()->environment(buildConfiguration));
    QVariant wd = value(buildConfiguration, "workingDirectory").toString();
    QString workingDirectory;
    if(!wd.isValid() || wd.toString().isEmpty())
        workingDirectory = "$BUILDDIR";
    else
        workingDirectory = wd.toString();
    setWorkingDirectory(buildConfiguration, workingDirectory.replace("$BUILDDIR", project()->buildDirectory(buildConfiguration)));
    return AbstractProcessStep::init(buildConfiguration);
}

void ProcessStep::run(QFutureInterface<bool> & fi)
{
    return AbstractProcessStep::run(fi);
}

QString ProcessStep::name()
{
    return "projectexplorer.processstep";
}

void ProcessStep::setDisplayName(const QString &name)
{
    setValue("ProjectExplorer.ProcessStep.DisplayName", name);
    emit displayNameChanged(this, name);
}

QString ProcessStep::displayName()
{
    QVariant displayName = value("ProjectExplorer.ProcessStep.DisplayName");
    if (displayName.isValid())
        return displayName.toString();
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

BuildStep *ProcessStepFactory::create(Project *pro, const QString &name) const
{
    Q_UNUSED(name);
    return new ProcessStep(pro);
}

QStringList ProcessStepFactory::canCreateForProject(Project *pro) const
{
    Q_UNUSED(pro)
    return QStringList()<<"projectexplorer.processstep";
}
QString ProcessStepFactory::displayNameForName(const QString &name) const
{
    Q_UNUSED(name);
    return "Custom Process Step";
}

//*******
// ProcessStepConfigWidget
//*******

ProcessStepConfigWidget::ProcessStepConfigWidget(ProcessStep *step)
        : m_step(step)
{
    m_ui.setupUi(this);
    connect(m_ui.commandBrowseButton, SIGNAL(clicked(bool)),
            this, SLOT(commandBrowseButtonClicked()));
    connect(m_ui.workingDirBrowseButton, SIGNAL(clicked(bool)),
            this, SLOT(workingDirBrowseButtonClicked()));

    connect(m_ui.nameLineEdit, SIGNAL(textEdited(const QString&)),
            this, SLOT(nameLineEditTextEdited()));
    connect(m_ui.commandLineEdit, SIGNAL(textEdited(const QString&)),
            this, SLOT(commandLineEditTextEdited()));
    connect(m_ui.workingDirectoryLineEdit, SIGNAL(textEdited(const QString&)),
            this, SLOT(workingDirectoryLineEditTextEdited()));
    connect(m_ui.commandArgumentsLineEdit, SIGNAL(textEdited(const QString&)),
            this, SLOT(commandArgumentsLineEditTextEdited()));
    connect(m_ui.enabledGroupBox, SIGNAL(clicked(bool)),
            this, SLOT(enabledGroupBoxClicked(bool)));
}

QString ProcessStepConfigWidget::displayName() const
{
    return m_step->name();
}

void ProcessStepConfigWidget::workingDirBrowseButtonClicked()
{
    QString workingDirectory = QFileDialog::getExistingDirectory(this, "Select the working directory", m_ui.workingDirectoryLineEdit->text());
    if(workingDirectory.isEmpty())
        return;
    m_ui.workingDirectoryLineEdit->setText(workingDirectory);
    workingDirectoryLineEditTextEdited();
}

void ProcessStepConfigWidget::commandBrowseButtonClicked()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select the executable");
    if(filename.isEmpty())
        return;
    m_ui.commandLineEdit->setText(filename);
    commandLineEditTextEdited();
}

void ProcessStepConfigWidget::init(const QString &buildConfiguration)
{
    m_buildConfiguration = buildConfiguration;
    if(buildConfiguration != QString::null) {
        m_ui.commandLineEdit->setText(m_step->command(buildConfiguration));

        QString workingDirectory = m_step->value(buildConfiguration, "workingDirectory").toString();
        if (workingDirectory.isEmpty())
            workingDirectory = "$BUILDDIR";
        m_ui.workingDirectoryLineEdit->setText(workingDirectory);

        m_ui.commandArgumentsLineEdit->setText(m_step->arguments(buildConfiguration).join(" "));
        m_ui.enabledGroupBox->setChecked(m_step->enabled(buildConfiguration));
    }
    m_ui.nameLineEdit->setText(m_step->displayName());
}

void ProcessStepConfigWidget::nameLineEditTextEdited()
{
    m_step->setDisplayName(m_ui.nameLineEdit->text());
}

void ProcessStepConfigWidget::commandLineEditTextEdited()
{
    m_step->setCommand(m_buildConfiguration, m_ui.commandLineEdit->text());
}

void ProcessStepConfigWidget::workingDirectoryLineEditTextEdited()
{
    QString wd = m_ui.workingDirectoryLineEdit->text();
    m_step->setValue(m_buildConfiguration, "workingDirectory", wd);
}

void ProcessStepConfigWidget::commandArgumentsLineEditTextEdited()
{
    m_step->setArguments(m_buildConfiguration, m_ui.commandArgumentsLineEdit->text().split(" ",
          QString::SkipEmptyParts));
}

void ProcessStepConfigWidget::enabledGroupBoxClicked(bool)
{
    m_step->setEnabled(m_buildConfiguration, m_ui.enabledGroupBox->isChecked());
}
