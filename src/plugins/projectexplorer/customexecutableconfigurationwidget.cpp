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

#include "customexecutableconfigurationwidget.h"

#include "customexecutablerunconfiguration.h"
#include "environmentaspect.h"
#include "project.h"
#include "runconfigurationaspects.h"
#include "target.h"

#include <coreplugin/variablechooser.h>
#include <utils/detailswidget.h>
#include <utils/pathchooser.h>

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>

using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

CustomExecutableConfigurationWidget::CustomExecutableConfigurationWidget(CustomExecutableRunConfiguration *rc, ApplyMode mode)
    : m_runConfiguration(rc)
{
    auto layout = new QFormLayout;
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    layout->setMargin(0);

    m_executableChooser = new PathChooser(this);
    m_executableChooser->setHistoryCompleter(QLatin1String("Qt.CustomExecutable.History"));
    m_executableChooser->setExpectedKind(PathChooser::ExistingCommand);
    layout->addRow(tr("Executable:"), m_executableChooser);

    auto argumentsAspect = rc->extraAspect<ArgumentsAspect>();
    if (mode == InstantApply) {
        argumentsAspect->addToMainConfigurationWidget(this, layout);
    } else {
        m_temporaryArgumentsAspect = argumentsAspect->clone(rc);
        m_temporaryArgumentsAspect->addToMainConfigurationWidget(this, layout);
        connect(m_temporaryArgumentsAspect, &ArgumentsAspect::argumentsChanged,
                this, &CustomExecutableConfigurationWidget::validChanged);
    }

    m_workingDirectory = new PathChooser(this);
    m_workingDirectory->setHistoryCompleter(QLatin1String("Qt.WorkingDir.History"));
    m_workingDirectory->setExpectedKind(PathChooser::Directory);
    m_workingDirectory->setBaseFileName(rc->target()->project()->projectDirectory());

    layout->addRow(tr("Working directory:"), m_workingDirectory);

    auto terminalAspect = rc->extraAspect<TerminalAspect>();
    if (mode == InstantApply) {
        terminalAspect->addToMainConfigurationWidget(this, layout);
    } else {
        m_temporaryTerminalAspect = terminalAspect->clone(rc);
        m_temporaryTerminalAspect->addToMainConfigurationWidget(this, layout);
        connect(m_temporaryTerminalAspect, &TerminalAspect::useTerminalChanged,
                this, &CustomExecutableConfigurationWidget::validChanged);
    }

    auto vbox = new QVBoxLayout(this);
    vbox->setMargin(0);

    m_detailsContainer = new DetailsWidget(this);
    m_detailsContainer->setState(DetailsWidget::NoSummary);
    vbox->addWidget(m_detailsContainer);

    auto detailsWidget = new QWidget(m_detailsContainer);
    m_detailsContainer->setWidget(detailsWidget);
    detailsWidget->setLayout(layout);

    changed();

    if (mode == InstantApply) {
        connect(m_executableChooser, &PathChooser::rawPathChanged,
                this, &CustomExecutableConfigurationWidget::executableEdited);
        connect(m_workingDirectory, &PathChooser::rawPathChanged,
                this, &CustomExecutableConfigurationWidget::workingDirectoryEdited);
    } else {
        connect(m_executableChooser, &PathChooser::rawPathChanged,
                this, &CustomExecutableConfigurationWidget::validChanged);
        connect(m_workingDirectory, &PathChooser::rawPathChanged,
                this, &CustomExecutableConfigurationWidget::validChanged);
    }

    auto enviromentAspect = rc->extraAspect<EnvironmentAspect>();
    connect(enviromentAspect, &EnvironmentAspect::environmentChanged,
            this, &CustomExecutableConfigurationWidget::environmentWasChanged);
    environmentWasChanged();

    // If we are in InstantApply mode, we keep us in sync with the rc
    // otherwise we ignore changes to the rc and override them on apply,
    // or keep them on cancel
    if (mode == InstantApply) {
        connect(m_runConfiguration, &CustomExecutableRunConfiguration::changed,
                this, &CustomExecutableConfigurationWidget::changed);
    }

    Core::VariableChooser::addSupportForChildWidgets(this, m_runConfiguration->macroExpander());
}

CustomExecutableConfigurationWidget::~CustomExecutableConfigurationWidget()
{
    delete m_temporaryArgumentsAspect;
    delete m_temporaryTerminalAspect;
}

void CustomExecutableConfigurationWidget::environmentWasChanged()
{
    auto aspect = m_runConfiguration->extraAspect<EnvironmentAspect>();
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

void CustomExecutableConfigurationWidget::workingDirectoryEdited()
{
    m_ignoreChange = true;
    m_runConfiguration->setBaseWorkingDirectory(m_workingDirectory->rawPath());
    m_ignoreChange = false;
}

void CustomExecutableConfigurationWidget::changed()
{
    // We triggered the change, don't update us
    if (m_ignoreChange)
        return;

    m_executableChooser->setPath(m_runConfiguration->rawExecutable());
    m_workingDirectory->setPath(m_runConfiguration->baseWorkingDirectory());
}

void CustomExecutableConfigurationWidget::apply()
{
    m_ignoreChange = true;
    m_runConfiguration->setExecutable(m_executableChooser->rawPath());
    m_runConfiguration->setCommandLineArguments(m_temporaryArgumentsAspect->unexpandedArguments());
    m_runConfiguration->setBaseWorkingDirectory(m_workingDirectory->rawPath());
    m_runConfiguration->setRunMode(m_temporaryTerminalAspect->runMode());
    m_ignoreChange = false;
}

bool CustomExecutableConfigurationWidget::isValid() const
{
    return !m_executableChooser->rawPath().isEmpty();
}

} // namespace Internal
} // namespace ProjectExplorer
