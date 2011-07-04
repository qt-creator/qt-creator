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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "maemodeployconfigurationwidget.h"
#include "ui_maemodeployconfigurationwidget.h"

#include "deployablefilesperprofile.h"
#include "deploymentinfo.h"
#include "linuxdeviceconfigurations.h"
#include "maemoglobal.h"
#include "maemopertargetdeviceconfigurationlistmodel.h"
#include "maemosettingspages.h"
#include "qt4maemodeployconfiguration.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QPixmap>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace Internal {

MaemoDeployConfigurationWidget::MaemoDeployConfigurationWidget(QWidget *parent)
    : DeployConfigurationWidget(parent),
      ui(new Ui::MaemoDeployConfigurationWidget)
{
    ui->setupUi(this);
}

MaemoDeployConfigurationWidget::~MaemoDeployConfigurationWidget()
{
    delete ui;
}

void MaemoDeployConfigurationWidget::init(DeployConfiguration *dc)
{
    m_deployConfig = qobject_cast<Qt4MaemoDeployConfiguration *>(dc);
    Q_ASSERT(m_deployConfig);

    connect(ui->manageDevConfsLabel, SIGNAL(linkActivated(QString)),
        SLOT(showDeviceConfigurations()));

    ui->deviceConfigsComboBox->setModel(m_deployConfig->deviceConfigModel().data());
    connect(ui->deviceConfigsComboBox, SIGNAL(activated(int)),
        SLOT(handleSelectedDeviceConfigurationChanged(int)));
    connect(m_deployConfig, SIGNAL(deviceConfigurationListChanged()),
        SLOT(handleDeviceConfigurationListChanged()));
    handleDeviceConfigurationListChanged();

    ui->projectsComboBox->setModel(m_deployConfig->deploymentInfo().data());
    connect(m_deployConfig->deploymentInfo().data(), SIGNAL(modelAboutToBeReset()),
        SLOT(handleModelListToBeReset()));

    // Queued connection because of race condition with combo box's reaction
    // to modelReset().
    connect(m_deployConfig->deploymentInfo().data(), SIGNAL(modelReset()),
        SLOT(handleModelListReset()), Qt::QueuedConnection);

    connect(ui->projectsComboBox, SIGNAL(currentIndexChanged(int)),
        SLOT(setModel(int)));
    connect(ui->addDesktopFileButton, SIGNAL(clicked()),
        SLOT(addDesktopFile()));
    connect(ui->addIconButton, SIGNAL(clicked()), SLOT(addIcon()));
    handleModelListReset();
}

void MaemoDeployConfigurationWidget::handleModelListToBeReset()
{
    ui->tableView->reset(); // Otherwise we'll crash if the user is currently editing.
    ui->tableView->setModel(0);
    ui->addDesktopFileButton->setEnabled(false);
    ui->addIconButton->setEnabled(false);
}

void MaemoDeployConfigurationWidget::handleModelListReset()
{
    QTC_ASSERT(m_deployConfig->deploymentInfo()->modelCount() == ui->projectsComboBox->count(), return);
    if (m_deployConfig->deploymentInfo()->modelCount() > 0) {
        if (ui->projectsComboBox->currentIndex() == -1)
            ui->projectsComboBox->setCurrentIndex(0);
        else
            setModel(ui->projectsComboBox->currentIndex());
    }
}

void MaemoDeployConfigurationWidget::setModel(int row)
{
    bool canAddDesktopFile = false;
    bool canAddIconFile = false;
    if (row != -1) {
        DeployableFilesPerProFile * const model
            = m_deployConfig->deploymentInfo()->modelAt(row);
        ui->tableView->setModel(model);
        ui->tableView->resizeRowsToContents();
        canAddDesktopFile = model->canAddDesktopFile();
        canAddIconFile = model->canAddIcon();
    }
    ui->addDesktopFileButton->setEnabled(canAddDesktopFile);
    ui->addIconButton->setEnabled(canAddIconFile);
}

void MaemoDeployConfigurationWidget::handleSelectedDeviceConfigurationChanged(int index)
{
    disconnect(m_deployConfig, SIGNAL(deviceConfigurationListChanged()), this,
        SLOT(handleDeviceConfigurationListChanged()));
    m_deployConfig->setDeviceConfiguration(index);
    connect(m_deployConfig, SIGNAL(deviceConfigurationListChanged()),
        SLOT(handleDeviceConfigurationListChanged()));
}

void MaemoDeployConfigurationWidget::handleDeviceConfigurationListChanged()
{
    const LinuxDeviceConfiguration::ConstPtr &devConf = m_deployConfig->deviceConfiguration();
    const LinuxDeviceConfiguration::Id internalId
        = LinuxDeviceConfigurations::instance()->internalId(devConf);
    const int newIndex = m_deployConfig->deviceConfigModel()->indexForInternalId(internalId);
    ui->deviceConfigsComboBox->setCurrentIndex(newIndex);
}

void MaemoDeployConfigurationWidget::addDesktopFile()
{
    const int modelRow = ui->projectsComboBox->currentIndex();
    if (modelRow == -1)
        return;
    DeployableFilesPerProFile *const model
        = m_deployConfig->deploymentInfo()->modelAt(modelRow);
    model->addDesktopFile();
    ui->addDesktopFileButton->setEnabled(model->canAddDesktopFile());
    ui->tableView->resizeRowsToContents();
}

void MaemoDeployConfigurationWidget::addIcon()
{
    const int modelRow = ui->projectsComboBox->currentIndex();
    if (modelRow == -1)
        return;

    DeployableFilesPerProFile *const model
        = m_deployConfig->deploymentInfo()->modelAt(modelRow);
    const int iconDim = MaemoGlobal::applicationIconSize(MaemoGlobal::osType(model->qtVersion()->qmakeCommand()));
    const QString origFilePath = QFileDialog::getOpenFileName(this,
        tr("Choose Icon (will be scaled to %1x%1 pixels, if necessary)").arg(iconDim),
        model->projectDir(), QLatin1String("(*.png)"));
    if (origFilePath.isEmpty())
        return;
    QPixmap pixmap(origFilePath);
    if (pixmap.isNull()) {
        QMessageBox::critical(this, tr("Invalid Icon"),
            tr("Unable to read image"));
        return;
    }
    const QSize iconSize(iconDim, iconDim);
    if (pixmap.size() != iconSize)
        pixmap = pixmap.scaled(iconSize);
    const QString newFileName = model->projectName() + QLatin1Char('.')
            + QFileInfo(origFilePath).suffix();
    const QString newFilePath = model->projectDir() + QLatin1Char('/')
        + newFileName;
    if (!pixmap.save(newFilePath)) {
        QMessageBox::critical(this, tr("Failed to Save Icon"),
            tr("Could not save icon to '%1'.").arg(newFilePath));
        return;
    }

    model->addIcon(newFileName);
    ui->addIconButton->setEnabled(model->canAddIcon());
    ui->tableView->resizeRowsToContents();
}

void MaemoDeployConfigurationWidget::showDeviceConfigurations()
{
    Core::ICore::instance()->showOptionsDialog(MaemoDeviceConfigurationsSettingsPage::Category,
        MaemoDeviceConfigurationsSettingsPage::Id);
}

} // namespace Internal
} // namespace RemoteLinux
