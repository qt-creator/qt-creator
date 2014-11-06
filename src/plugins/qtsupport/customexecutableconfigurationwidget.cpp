/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "customexecutableconfigurationwidget.h"
#include "customexecutablerunconfiguration.h"

#include <coreplugin/variablechooser.h>
#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <utils/detailswidget.h>
#include <utils/pathchooser.h>

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>


namespace QtSupport {
namespace Internal {

CustomExecutableConfigurationWidget::CustomExecutableConfigurationWidget(CustomExecutableRunConfiguration *rc, ApplyMode mode)
    : m_ignoreChange(false), m_runConfiguration(rc)
{
    QFormLayout *layout = new QFormLayout;
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    layout->setMargin(0);

    m_executableChooser = new Utils::PathChooser(this);
    m_executableChooser->setHistoryCompleter(QLatin1String("Qt.CustomExecutable.History"));
    m_executableChooser->setExpectedKind(Utils::PathChooser::Command);
    layout->addRow(tr("Executable:"), m_executableChooser);

    m_commandLineArgumentsLineEdit = new QLineEdit(this);
    m_commandLineArgumentsLineEdit->setMinimumWidth(200); // this shouldn't be fixed here...
    layout->addRow(tr("Arguments:"), m_commandLineArgumentsLineEdit);

    m_workingDirectory = new Utils::PathChooser(this);
    m_workingDirectory->setHistoryCompleter(QLatin1String("Qt.WorkingDir.History"));
    m_workingDirectory->setExpectedKind(Utils::PathChooser::Directory);
    m_workingDirectory->setBaseFileName(rc->target()->project()->projectDirectory());

    layout->addRow(tr("Working directory:"), m_workingDirectory);

    m_useTerminalCheck = new QCheckBox(tr("Run in &terminal"), this);
    layout->addRow(QString(), m_useTerminalCheck);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);

    m_detailsContainer = new Utils::DetailsWidget(this);
    m_detailsContainer->setState(Utils::DetailsWidget::NoSummary);
    vbox->addWidget(m_detailsContainer);

    QWidget *detailsWidget = new QWidget(m_detailsContainer);
    m_detailsContainer->setWidget(detailsWidget);
    detailsWidget->setLayout(layout);

    changed();

    if (mode == InstantApply) {
        connect(m_executableChooser, SIGNAL(changed(QString)),
                this, SLOT(executableEdited()));
        connect(m_commandLineArgumentsLineEdit, SIGNAL(textEdited(QString)),
                this, SLOT(argumentsEdited(QString)));
        connect(m_workingDirectory, SIGNAL(changed(QString)),
                this, SLOT(workingDirectoryEdited()));
        connect(m_useTerminalCheck, SIGNAL(toggled(bool)),
                this, SLOT(termToggled(bool)));
    } else {
        connect(m_executableChooser, SIGNAL(changed(QString)),
                this, SIGNAL(validChanged()));
        connect(m_commandLineArgumentsLineEdit, SIGNAL(textEdited(QString)),
                this, SIGNAL(validChanged()));
        connect(m_workingDirectory, SIGNAL(changed(QString)),
                this, SIGNAL(validChanged()));
        connect(m_useTerminalCheck, SIGNAL(toggled(bool)),
                this, SIGNAL(validChanged()));
    }

    ProjectExplorer::EnvironmentAspect *aspect = rc->extraAspect<ProjectExplorer::EnvironmentAspect>();
    if (aspect) {
        connect(aspect, SIGNAL(environmentChanged()), this, SLOT(environmentWasChanged()));
        environmentWasChanged();
    }

    // If we are in mode InstantApply, we keep us in sync with the rc
    // otherwise we ignore changes to the rc and override them on apply,
    // or keep them on cancel
    if (mode == InstantApply)
        connect(m_runConfiguration, SIGNAL(changed()), this, SLOT(changed()));

    Core::VariableChooser::addSupportForChildWidgets(this, m_runConfiguration->macroExpander());
}

void CustomExecutableConfigurationWidget::environmentWasChanged()
{
    ProjectExplorer::EnvironmentAspect *aspect
            = m_runConfiguration->extraAspect<ProjectExplorer::EnvironmentAspect>();
    QTC_ASSERT(aspect, return);
    m_workingDirectory->setEnvironment(aspect->environment());
    m_executableChooser->setEnvironment(aspect->environment());
}

void CustomExecutableConfigurationWidget::executableEdited()
{
    m_ignoreChange = true;
    m_runConfiguration->setExecutable(m_executableChooser->rawPath());
    m_ignoreChange = false;
}
void CustomExecutableConfigurationWidget::argumentsEdited(const QString &arguments)
{
    m_ignoreChange = true;
    m_runConfiguration->setCommandLineArguments(arguments);
    m_ignoreChange = false;
}
void CustomExecutableConfigurationWidget::workingDirectoryEdited()
{
    m_ignoreChange = true;
    m_runConfiguration->setBaseWorkingDirectory(m_workingDirectory->rawPath());
    m_ignoreChange = false;
}

void CustomExecutableConfigurationWidget::termToggled(bool on)
{
    m_ignoreChange = true;
    m_runConfiguration->setRunMode(on ? ProjectExplorer::ApplicationLauncher::Console
                                      : ProjectExplorer::ApplicationLauncher::Gui);
    m_ignoreChange = false;
}

void CustomExecutableConfigurationWidget::changed()
{
    // We triggered the change, don't update us
    if (m_ignoreChange)
        return;

    m_executableChooser->setPath(m_runConfiguration->rawExecutable());
    m_commandLineArgumentsLineEdit->setText(m_runConfiguration->rawCommandLineArguments());
    m_workingDirectory->setPath(m_runConfiguration->baseWorkingDirectory());
    m_useTerminalCheck->setChecked(m_runConfiguration->runMode()
                                   == ProjectExplorer::ApplicationLauncher::Console);
}

void CustomExecutableConfigurationWidget::apply()
{
    m_ignoreChange = true;
    m_runConfiguration->setExecutable(m_executableChooser->rawPath());
    m_runConfiguration->setCommandLineArguments(m_commandLineArgumentsLineEdit->text());
    m_runConfiguration->setBaseWorkingDirectory(m_workingDirectory->rawPath());
    m_runConfiguration->setRunMode(m_useTerminalCheck->isChecked() ? ProjectExplorer::ApplicationLauncher::Console
                                                                   : ProjectExplorer::ApplicationLauncher::Gui);
    m_ignoreChange = false;
}

bool CustomExecutableConfigurationWidget::isValid() const
{
    return !m_executableChooser->rawPath().isEmpty();
}

} // namespace Internal
} // namespace QtSupport
