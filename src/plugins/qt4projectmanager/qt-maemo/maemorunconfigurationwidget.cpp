/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "maemorunconfigurationwidget.h"

#include "maemodeviceconfigurations.h"
#include "maemomanager.h"
#include "maemorunconfiguration.h"
#include "maemosettingspage.h"

#include <coreplugin/icore.h>

#include <QtGui/QComboBox>
#include <QtGui/QCheckBox>
#include <QtGui/QDesktopServices>
#include <QtGui/QFormLayout>
#include <QtGui/QFrame>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QRadioButton>
#include <QtGui/QToolButton>

namespace Qt4ProjectManager {
namespace Internal {

MaemoRunConfigurationWidget::MaemoRunConfigurationWidget(
        MaemoRunConfiguration *runConfiguration, QWidget *parent)
    : QWidget(parent), m_runConfiguration(runConfiguration)
{
    QFormLayout *mainLayout = new QFormLayout;
    setLayout(mainLayout);

    mainLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_configNameLineEdit = new QLineEdit(m_runConfiguration->displayName());
    mainLayout->addRow(tr("Run configuration name:"), m_configNameLineEdit);
    QWidget *devConfWidget = new QWidget;
    QHBoxLayout *devConfLayout = new QHBoxLayout(devConfWidget);
    m_devConfBox = new QComboBox;
    m_devConfBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    devConfLayout->setMargin(0);
    devConfLayout->addWidget(m_devConfBox);
    QLabel *addDevConfLabel
        = new QLabel(tr("<a href=\"#\">Manage device configurations</a>"));
    devConfLayout->addWidget(addDevConfLabel);
    mainLayout->addRow(new QLabel(tr("Device Configuration:")), devConfWidget);
    m_executableLabel = new QLabel(m_runConfiguration->executable());
    mainLayout->addRow(tr("Executable:"), m_executableLabel);
    m_argsLineEdit = new QLineEdit(m_runConfiguration->arguments().join(" "));
    mainLayout->addRow(tr("Arguments:"), m_argsLineEdit);
    m_debuggerLabel = new QLabel(m_runConfiguration->gdbCmd());
    mainLayout->addRow(tr("Debugger:"), m_debuggerLabel);
    mainLayout->addItem(new QSpacerItem(10, 10));
    m_simNameLabel = new QLabel(tr("Simulator:"));
    m_simValueLabel = new QLabel(m_runConfiguration->visibleSimulatorParameter());
    mainLayout->addRow(m_simNameLabel, m_simValueLabel);
    resetDeviceConfigurations();

    connect(m_runConfiguration, SIGNAL(cachedSimulatorInformationChanged()),
        this, SLOT(updateSimulatorPath()));
    connect(m_runConfiguration, SIGNAL(deviceConfigurationsUpdated()),
            this, SLOT(resetDeviceConfigurations()));

    connect(m_configNameLineEdit, SIGNAL(textEdited(QString)), this,
        SLOT(configNameEdited(QString)));
    connect(m_argsLineEdit, SIGNAL(textEdited(QString)), this,
        SLOT(argumentsEdited(QString)));
    connect(m_devConfBox, SIGNAL(activated(QString)), this,
            SLOT(deviceConfigurationChanged(QString)));
    connect(m_runConfiguration, SIGNAL(targetInformationChanged()), this,
        SLOT(updateTargetInformation()));
    connect(addDevConfLabel, SIGNAL(linkActivated(QString)), this,
        SLOT(showSettingsDialog()));
}

void MaemoRunConfigurationWidget::configNameEdited(const QString &text)
{
    m_runConfiguration->setDisplayName(text);
}

void MaemoRunConfigurationWidget::argumentsEdited(const QString &text)
{
    m_runConfiguration->setArguments(text.split(' ', QString::SkipEmptyParts));
}

void MaemoRunConfigurationWidget::updateTargetInformation()
{
    m_executableLabel->setText(m_runConfiguration->executable());
}

void MaemoRunConfigurationWidget::updateSimulatorPath()
{
    m_simValueLabel->setText(m_runConfiguration->visibleSimulatorParameter());
}

void MaemoRunConfigurationWidget::deviceConfigurationChanged(const QString &name)
{
    const MaemoDeviceConfig &devConfig
        = MaemoDeviceConfigurations::instance().find(name);
    setSimInfoVisible(devConfig);
    m_runConfiguration->setDeviceConfig(devConfig);
}

void MaemoRunConfigurationWidget::setSimInfoVisible(const MaemoDeviceConfig &devConf)
{
    const bool isSimulator = devConf.type == MaemoDeviceConfig::Simulator;
    m_simNameLabel->setVisible(isSimulator);
    m_simValueLabel->setVisible(isSimulator);
}

void MaemoRunConfigurationWidget::resetDeviceConfigurations()
{
    m_devConfBox->clear();
    const QList<MaemoDeviceConfig> &devConfs =
        MaemoDeviceConfigurations::instance().devConfigs();
    foreach (const MaemoDeviceConfig &devConf, devConfs)
        m_devConfBox->addItem(devConf.name);
    m_devConfBox->addItem(MaemoDeviceConfig().name);
    const MaemoDeviceConfig &devConf = m_runConfiguration->deviceConfig();
    m_devConfBox->setCurrentIndex(m_devConfBox->findText(devConf.name));
    setSimInfoVisible(devConf);
}

void MaemoRunConfigurationWidget::showSettingsDialog()
{
    MaemoSettingsPage *settingsPage = MaemoManager::instance().settingsPage();
    Core::ICore::instance()->showOptionsDialog(settingsPage->category(),
        settingsPage->id());
}

} // namespace Internal
} // namespace Qt4ProjectManager
