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

#include "customexecutableconfigurationwidget.h"
#include "customexecutablerunconfiguration.h"
#include "target.h"
#include "project.h"
#include "environmentwidget.h"

#include <coreplugin/helpmanager.h>
#include <utils/detailswidget.h>
#include <utils/environment.h>
#include <utils/pathchooser.h>
#include <utils/debuggerlanguagechooser.h>

#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QFormLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>


namespace ProjectExplorer {
namespace Internal {

CustomExecutableConfigurationWidget::CustomExecutableConfigurationWidget(CustomExecutableRunConfiguration *rc)
    : m_ignoreChange(false), m_runConfiguration(rc)
{
    QFormLayout *layout = new QFormLayout;
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    layout->setMargin(0);

    m_executableChooser = new Utils::PathChooser(this);
    m_executableChooser->setExpectedKind(Utils::PathChooser::Command);
    m_executableChooser->setEnvironment(rc->environment());
    layout->addRow(tr("Executable:"), m_executableChooser);

    m_commandLineArgumentsLineEdit = new QLineEdit(this);
    m_commandLineArgumentsLineEdit->setMinimumWidth(200); // this shouldn't be fixed here...
    layout->addRow(tr("Arguments:"), m_commandLineArgumentsLineEdit);

    m_workingDirectory = new Utils::PathChooser(this);
    m_workingDirectory->setExpectedKind(Utils::PathChooser::Directory);
    m_workingDirectory->setBaseDirectory(rc->target()->project()->projectDirectory());
    m_workingDirectory->setEnvironment(rc->environment());
    layout->addRow(tr("Working directory:"), m_workingDirectory);

    m_useTerminalCheck = new QCheckBox(tr("Run in &Terminal"), this);
    layout->addRow(QString(), m_useTerminalCheck);

    QWidget *debuggerLabelWidget = new QWidget(this);
    QVBoxLayout *debuggerLabelLayout = new QVBoxLayout(debuggerLabelWidget);
    debuggerLabelLayout->setMargin(0);
    debuggerLabelLayout->setSpacing(0);
    debuggerLabelWidget->setLayout(debuggerLabelLayout);
    QLabel *debuggerLabel = new QLabel(tr("Debugger:"), this);
    debuggerLabelLayout->addWidget(debuggerLabel);
    debuggerLabelLayout->addStretch(10);

    m_debuggerLanguageChooser = new Utils::DebuggerLanguageChooser(this);
    layout->addRow(debuggerLabelWidget, m_debuggerLanguageChooser);

    m_debuggerLanguageChooser->setCppChecked(m_runConfiguration->useCppDebugger());
    m_debuggerLanguageChooser->setQmlChecked(m_runConfiguration->useQmlDebugger());
    m_debuggerLanguageChooser->setQmlDebugServerPort(m_runConfiguration->qmlDebugServerPort());

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(0);

    m_detailsContainer = new Utils::DetailsWidget(this);
    m_detailsContainer->setState(Utils::DetailsWidget::NoSummary);
    vbox->addWidget(m_detailsContainer);

    QWidget *detailsWidget = new QWidget(m_detailsContainer);
    m_detailsContainer->setWidget(detailsWidget);
    detailsWidget->setLayout(layout);

    QLabel *environmentLabel = new QLabel(this);
    environmentLabel->setText(tr("Run Environment"));
    QFont f = environmentLabel->font();
    f.setBold(true);
    f.setPointSizeF(f.pointSizeF() *1.2);
    environmentLabel->setFont(f);
    vbox->addWidget(environmentLabel);

    QWidget *baseEnvironmentWidget = new QWidget;
    QHBoxLayout *baseEnvironmentLayout = new QHBoxLayout(baseEnvironmentWidget);
    baseEnvironmentLayout->setMargin(0);
    QLabel *label = new QLabel(tr("Base environment for this runconfiguration:"), this);
    baseEnvironmentLayout->addWidget(label);
    m_baseEnvironmentComboBox = new QComboBox(this);
    m_baseEnvironmentComboBox->addItems(QStringList()
                                        << tr("Clean Environment")
                                        << tr("System Environment")
                                        << tr("Build Environment"));
    m_baseEnvironmentComboBox->setCurrentIndex(rc->baseEnvironmentBase());
    connect(m_baseEnvironmentComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(baseEnvironmentSelected(int)));
    baseEnvironmentLayout->addWidget(m_baseEnvironmentComboBox);
    baseEnvironmentLayout->addStretch(10);

    m_environmentWidget = new EnvironmentWidget(this, baseEnvironmentWidget);
    m_environmentWidget->setBaseEnvironment(rc->baseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(rc->baseEnvironmentText());
    m_environmentWidget->setUserChanges(rc->userEnvironmentChanges());
    vbox->addWidget(m_environmentWidget);

    changed();

    connect(m_executableChooser, SIGNAL(changed(QString)),
            this, SLOT(executableEdited()));
    connect(m_commandLineArgumentsLineEdit, SIGNAL(textEdited(const QString&)),
            this, SLOT(argumentsEdited(const QString&)));
    connect(m_workingDirectory, SIGNAL(changed(QString)),
            this, SLOT(workingDirectoryEdited()));
    connect(m_useTerminalCheck, SIGNAL(toggled(bool)),
            this, SLOT(termToggled(bool)));

    connect(m_debuggerLanguageChooser, SIGNAL(cppLanguageToggled(bool)),
            this, SLOT(useCppDebuggerToggled(bool)));
    connect(m_debuggerLanguageChooser, SIGNAL(qmlLanguageToggled(bool)),
            this, SLOT(useQmlDebuggerToggled(bool)));
    connect(m_debuggerLanguageChooser, SIGNAL(qmlDebugServerPortChanged(uint)),
            this, SLOT(qmlDebugServerPortChanged(uint)));
    connect(m_debuggerLanguageChooser, SIGNAL(openHelpUrl(QString)),
            Core::HelpManager::instance(), SLOT(handleHelpRequest(QString)));

    connect(m_runConfiguration, SIGNAL(changed()), this, SLOT(changed()));

    connect(m_environmentWidget, SIGNAL(userChangesChanged()),
            this, SLOT(userChangesChanged()));

    connect(m_runConfiguration, SIGNAL(baseEnvironmentChanged()),
            this, SLOT(baseEnvironmentChanged()));
    connect(m_runConfiguration, SIGNAL(userEnvironmentChangesChanged(QList<Utils::EnvironmentItem>)),
            this, SLOT(userEnvironmentChangesChanged()));
}

void CustomExecutableConfigurationWidget::userChangesChanged()
{
    m_runConfiguration->setUserEnvironmentChanges(m_environmentWidget->userChanges());
}

void CustomExecutableConfigurationWidget::baseEnvironmentSelected(int index)
{
    m_ignoreChange = true;
    m_runConfiguration->setBaseEnvironmentBase(CustomExecutableRunConfiguration::BaseEnvironmentBase(index));

    m_environmentWidget->setBaseEnvironment(m_runConfiguration->baseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(m_runConfiguration->baseEnvironmentText());
    m_ignoreChange = false;
}

void CustomExecutableConfigurationWidget::useCppDebuggerToggled(bool toggled)
{
    m_runConfiguration->setUseCppDebugger(toggled);
}

void CustomExecutableConfigurationWidget::useQmlDebuggerToggled(bool toggled)
{
    m_runConfiguration->setUseQmlDebugger(toggled);
}

void CustomExecutableConfigurationWidget::qmlDebugServerPortChanged(uint port)
{
    m_runConfiguration->setQmlDebugServerPort(port);
}

void CustomExecutableConfigurationWidget::baseEnvironmentChanged()
{
    if (m_ignoreChange)
        return;

    int index = CustomExecutableRunConfiguration::BaseEnvironmentBase(
            m_runConfiguration->baseEnvironmentBase());
    m_baseEnvironmentComboBox->setCurrentIndex(index);
    m_environmentWidget->setBaseEnvironment(m_runConfiguration->baseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(m_runConfiguration->baseEnvironmentText());
}

void CustomExecutableConfigurationWidget::userEnvironmentChangesChanged()
{
    m_environmentWidget->setUserChanges(m_runConfiguration->userEnvironmentChanges());
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
    m_runConfiguration->setRunMode(on ? LocalApplicationRunConfiguration::Console
                                      : LocalApplicationRunConfiguration::Gui);
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
    m_useTerminalCheck->setChecked(m_runConfiguration->runMode() == LocalApplicationRunConfiguration::Console);
}

} // namespace Internal
} // namespace ProjectExplorer
