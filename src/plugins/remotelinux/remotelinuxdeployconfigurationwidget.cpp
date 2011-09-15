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
    DeployConfigurationWidget(parent), d(new RemoteLinuxDeployConfigurationWidgetPrivate)
{
    d->ui.setupUi(this);
}

RemoteLinuxDeployConfigurationWidget::~RemoteLinuxDeployConfigurationWidget()
{
    delete d;
}

void RemoteLinuxDeployConfigurationWidget::init(DeployConfiguration *dc)
{
    d->deployConfiguration = qobject_cast<RemoteLinuxDeployConfiguration *>(dc);
    Q_ASSERT(d->deployConfiguration);

    connect(d->ui.manageDevConfsLabel, SIGNAL(linkActivated(QString)),
        SLOT(showDeviceConfigurations()));

    d->ui.deviceConfigsComboBox->setModel(d->deployConfiguration->deviceConfigModel().data());
    connect(d->ui.deviceConfigsComboBox, SIGNAL(activated(int)),
        SLOT(handleSelectedDeviceConfigurationChanged(int)));
    connect(d->deployConfiguration, SIGNAL(deviceConfigurationListChanged()),
        SLOT(handleDeviceConfigurationListChanged()));
    handleDeviceConfigurationListChanged();

    d->ui.projectsComboBox->setModel(d->deployConfiguration->deploymentInfo().data());
    connect(d->deployConfiguration->deploymentInfo().data(), SIGNAL(modelAboutToBeReset()),
        SLOT(handleModelListToBeReset()));

    // Queued connection because of race condition with combo box's reaction
    // to modelReset().
    connect(d->deployConfiguration->deploymentInfo().data(), SIGNAL(modelReset()),
        SLOT(handleModelListReset()), Qt::QueuedConnection);

    connect(d->ui.projectsComboBox, SIGNAL(currentIndexChanged(int)), SLOT(setModel(int)));
    handleModelListReset();
}

RemoteLinuxDeployConfiguration *RemoteLinuxDeployConfigurationWidget::deployConfiguration() const
{
    return d->deployConfiguration;
}

DeployableFilesPerProFile *RemoteLinuxDeployConfigurationWidget::currentModel() const
{
    const int modelRow = d->ui.projectsComboBox->currentIndex();
    if (modelRow == -1)
        return 0;
    return d->deployConfiguration->deploymentInfo()->modelAt(modelRow);
}

void RemoteLinuxDeployConfigurationWidget::handleModelListToBeReset()
{
    d->ui.tableView->setModel(0);
}

void RemoteLinuxDeployConfigurationWidget::handleModelListReset()
{
    QTC_ASSERT(d->deployConfiguration->deploymentInfo()->modelCount()
        == d->ui.projectsComboBox->count(), return);

    if (d->deployConfiguration->deploymentInfo()->modelCount() > 0) {
        if (d->ui.projectsComboBox->currentIndex() == -1)
            d->ui.projectsComboBox->setCurrentIndex(0);
        else
            setModel(d->ui.projectsComboBox->currentIndex());
    }
}

void RemoteLinuxDeployConfigurationWidget::setModel(int row)
{
    DeployableFilesPerProFile * const proFileInfo = row == -1
        ? 0 : d->deployConfiguration->deploymentInfo()->modelAt(row);
    d->ui.tableView->setModel(proFileInfo);
    if (proFileInfo)
        d->ui.tableView->resizeRowsToContents();
    emit currentModelChanged(proFileInfo);
}

void RemoteLinuxDeployConfigurationWidget::handleSelectedDeviceConfigurationChanged(int index)
{
    disconnect(d->deployConfiguration, SIGNAL(deviceConfigurationListChanged()), this,
        SLOT(handleDeviceConfigurationListChanged()));
    d->deployConfiguration->setDeviceConfiguration(index);
    connect(d->deployConfiguration, SIGNAL(deviceConfigurationListChanged()),
        SLOT(handleDeviceConfigurationListChanged()));
}

void RemoteLinuxDeployConfigurationWidget::handleDeviceConfigurationListChanged()
{
    const LinuxDeviceConfiguration::ConstPtr &devConf
        = d->deployConfiguration->deviceConfiguration();
    const LinuxDeviceConfiguration::Id internalId
        = LinuxDeviceConfigurations::instance()->internalId(devConf);
    const int newIndex = d->deployConfiguration->deviceConfigModel()->indexForInternalId(internalId);
    d->ui.deviceConfigsComboBox->setCurrentIndex(newIndex);
}

void RemoteLinuxDeployConfigurationWidget::showDeviceConfigurations()
{
    Core::ICore::instance()->showOptionsDialog(LinuxDeviceConfigurationsSettingsPage::pageCategory(),
        LinuxDeviceConfigurationsSettingsPage::pageId());
}

} // namespace RemoteLinux
