/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/
#include "remotelinuxdeployconfigurationwidget.h"
#include "ui_remotelinuxdeployconfigurationwidget.h"

#include "deployablefilesperprofile.h"
#include "deploymentinfo.h"
#include "linuxdeviceconfigurations.h"
#include "remotelinuxdeployconfiguration.h"
#include "remotelinuxsettingspages.h"
#include "typespecificdeviceconfigurationlistmodel.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

class RemoteLinuxDeployConfigurationWidgetPrivate
{
public:
    Ui::RemoteLinuxDeployConfigurationWidget ui;
    RemoteLinuxDeployConfiguration *deployConfiguration;
};

} // namespace Internal

using namespace Internal;

RemoteLinuxDeployConfigurationWidget::RemoteLinuxDeployConfigurationWidget(QWidget *parent) :
    DeployConfigurationWidget(parent), m_d(new RemoteLinuxDeployConfigurationWidgetPrivate)
{
    m_d->ui.setupUi(this);
}

RemoteLinuxDeployConfigurationWidget::~RemoteLinuxDeployConfigurationWidget()
{
    delete m_d;
}

void RemoteLinuxDeployConfigurationWidget::init(DeployConfiguration *dc)
{
    m_d->deployConfiguration = qobject_cast<RemoteLinuxDeployConfiguration *>(dc);
    Q_ASSERT(m_d->deployConfiguration);

    connect(m_d->ui.manageDevConfsLabel, SIGNAL(linkActivated(QString)),
        SLOT(showDeviceConfigurations()));

    m_d->ui.deviceConfigsComboBox->setModel(m_d->deployConfiguration->deviceConfigModel().data());
    connect(m_d->ui.deviceConfigsComboBox, SIGNAL(activated(int)),
        SLOT(handleSelectedDeviceConfigurationChanged(int)));
    connect(m_d->deployConfiguration, SIGNAL(deviceConfigurationListChanged()),
        SLOT(handleDeviceConfigurationListChanged()));
    handleDeviceConfigurationListChanged();

    m_d->ui.projectsComboBox->setModel(m_d->deployConfiguration->deploymentInfo().data());
    connect(m_d->deployConfiguration->deploymentInfo().data(), SIGNAL(modelAboutToBeReset()),
        SLOT(handleModelListToBeReset()));

    // Queued connection because of race condition with combo box's reaction
    // to modelReset().
    connect(m_d->deployConfiguration->deploymentInfo().data(), SIGNAL(modelReset()),
        SLOT(handleModelListReset()), Qt::QueuedConnection);

    connect(m_d->ui.projectsComboBox, SIGNAL(currentIndexChanged(int)), SLOT(setModel(int)));
    handleModelListReset();
}

RemoteLinuxDeployConfiguration *RemoteLinuxDeployConfigurationWidget::deployConfiguration() const
{
    return m_d->deployConfiguration;
}

DeployableFilesPerProFile *RemoteLinuxDeployConfigurationWidget::currentModel() const
{
    const int modelRow = m_d->ui.projectsComboBox->currentIndex();
    if (modelRow == -1)
        return 0;
    return m_d->deployConfiguration->deploymentInfo()->modelAt(modelRow);
}

void RemoteLinuxDeployConfigurationWidget::handleModelListToBeReset()
{
    m_d->ui.tableView->setModel(0);
}

void RemoteLinuxDeployConfigurationWidget::handleModelListReset()
{
    QTC_ASSERT(m_d->deployConfiguration->deploymentInfo()->modelCount()
        == m_d->ui.projectsComboBox->count(), return);

    if (m_d->deployConfiguration->deploymentInfo()->modelCount() > 0) {
        if (m_d->ui.projectsComboBox->currentIndex() == -1)
            m_d->ui.projectsComboBox->setCurrentIndex(0);
        else
            setModel(m_d->ui.projectsComboBox->currentIndex());
    }
}

void RemoteLinuxDeployConfigurationWidget::setModel(int row)
{
    DeployableFilesPerProFile * const proFileInfo = row == -1
        ? 0 : m_d->deployConfiguration->deploymentInfo()->modelAt(row);
    m_d->ui.tableView->setModel(proFileInfo);
    if (proFileInfo)
        m_d->ui.tableView->resizeRowsToContents();
    emit currentModelChanged(proFileInfo);
}

void RemoteLinuxDeployConfigurationWidget::handleSelectedDeviceConfigurationChanged(int index)
{
    disconnect(m_d->deployConfiguration, SIGNAL(deviceConfigurationListChanged()), this,
        SLOT(handleDeviceConfigurationListChanged()));
    m_d->deployConfiguration->setDeviceConfiguration(index);
    connect(m_d->deployConfiguration, SIGNAL(deviceConfigurationListChanged()),
        SLOT(handleDeviceConfigurationListChanged()));
}

void RemoteLinuxDeployConfigurationWidget::handleDeviceConfigurationListChanged()
{
    const LinuxDeviceConfiguration::ConstPtr &devConf
        = m_d->deployConfiguration->deviceConfiguration();
    const LinuxDeviceConfiguration::Id internalId
        = LinuxDeviceConfigurations::instance()->internalId(devConf);
    const int newIndex = m_d->deployConfiguration->deviceConfigModel()->indexForInternalId(internalId);
    m_d->ui.deviceConfigsComboBox->setCurrentIndex(newIndex);
}

void RemoteLinuxDeployConfigurationWidget::showDeviceConfigurations()
{
    Core::ICore::instance()->showOptionsDialog(LinuxDeviceConfigurationsSettingsPage::pageCategory(),
        LinuxDeviceConfigurationsSettingsPage::pageId());
}

} // namespace RemoteLinux
