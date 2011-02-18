/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "maemodeploystepwidget.h"
#include "ui_maemodeploystepwidget.h"

#include "maemodeploystep.h"
#include "maemodeployablelistmodel.h"
#include "maemodeployables.h"
#include "maemodeviceconfigurations.h"
#include "maemosettingspages.h"
#include "maemoglobal.h"
#include "maemomanager.h"
#include "maemopertargetdeviceconfigurationlistmodel.h"
#include "maemorunconfiguration.h"
#include "qt4maemotarget.h"

#include <coreplugin/icore.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QPixmap>

namespace Qt4ProjectManager {
namespace Internal {

MaemoDeployStepWidget::MaemoDeployStepWidget(MaemoDeployStep *step) :
    ProjectExplorer::BuildStepConfigWidget(),
    ui(new Ui::MaemoDeployStepWidget),
    m_step(step)
{
    ui->setupUi(this);
    ui->modelComboBox->setModel(m_step->deployables().data());
    connect(m_step->deployables().data(), SIGNAL(modelAboutToBeReset()),
        SLOT(handleModelListToBeReset()));

    // Queued connection because of race condition with combo box's reaction
    // to modelReset().
    connect(m_step->deployables().data(), SIGNAL(modelReset()),
        SLOT(handleModelListReset()), Qt::QueuedConnection);

    connect(ui->modelComboBox, SIGNAL(currentIndexChanged(int)),
        SLOT(setModel(int)));
    connect(ui->addDesktopFileButton, SIGNAL(clicked()),
        SLOT(addDesktopFile()));
    connect(ui->addIconButton, SIGNAL(clicked()), SLOT(addIcon()));
    handleModelListReset();

}

MaemoDeployStepWidget::~MaemoDeployStepWidget()
{
    delete ui;
}

void MaemoDeployStepWidget::init()
{
    ui->deviceConfigComboBox->setModel(m_step->maemotarget()->deviceConfigurationsModel());
    connect(m_step, SIGNAL(deviceConfigChanged()), SLOT(handleDeviceUpdate()));
    handleDeviceUpdate();
    connect(ui->deviceConfigComboBox, SIGNAL(activated(int)), this,
        SLOT(setCurrentDeviceConfig(int)));
    ui->deployToSysrootCheckBox->setChecked(m_step->isDeployToSysrootEnabled());
    connect(ui->deployToSysrootCheckBox, SIGNAL(toggled(bool)), this,
        SLOT(setDeployToSysroot(bool)));
    connect(ui->manageDevConfsLabel, SIGNAL(linkActivated(QString)),
        SLOT(showDeviceConfigurations()));
}

void MaemoDeployStepWidget::handleDeviceUpdate()
{
    const MaemoDeviceConfig::ConstPtr &devConf = m_step->deviceConfig();
    const MaemoDeviceConfig::Id internalId
        = MaemoDeviceConfigurations::instance()->internalId(devConf);
    const int newIndex = m_step->maemotarget()->deviceConfigurationsModel()
        ->indexForInternalId(internalId);
    ui->deviceConfigComboBox->setCurrentIndex(newIndex);
    emit updateSummary();
}

QString MaemoDeployStepWidget::summaryText() const
{
    return tr("<b>Deploy to device</b>: %1")
        .arg(MaemoGlobal::deviceConfigurationName(m_step->deviceConfig()));
}

QString MaemoDeployStepWidget::displayName() const
{
    return QString();
}

void MaemoDeployStepWidget::setCurrentDeviceConfig(int index)
{
    disconnect(m_step, SIGNAL(deviceConfigChanged()), this,
        SLOT(handleDeviceUpdate()));
    m_step->setDeviceConfig(index);
    connect(m_step, SIGNAL(deviceConfigChanged()), SLOT(handleDeviceUpdate()));
    updateSummary();
}

void MaemoDeployStepWidget::setDeployToSysroot(bool doDeploy)
{
    m_step->setDeployToSysrootEnabled(doDeploy);
}

void MaemoDeployStepWidget::handleModelListToBeReset()
{
    ui->tableView->reset(); // Otherwise we'll crash if the user is currently editing.
    ui->tableView->setModel(0);
    ui->addDesktopFileButton->setEnabled(false);
    ui->addIconButton->setEnabled(false);
}

void MaemoDeployStepWidget::handleModelListReset()
{
    QTC_ASSERT(m_step->deployables()->modelCount() == ui->modelComboBox->count(), return);
    if (m_step->deployables()->modelCount() > 0) {
        if (ui->modelComboBox->currentIndex() == -1)
            ui->modelComboBox->setCurrentIndex(0);
        else
            setModel(ui->modelComboBox->currentIndex());
    }
}

void MaemoDeployStepWidget::setModel(int row)
{
    bool canAddDesktopFile = false;
    bool canAddIconFile = false;
    if (row != -1) {
        MaemoDeployableListModel *const model
            = m_step->deployables()->modelAt(row);
        ui->tableView->setModel(model);
        ui->tableView->resizeRowsToContents();
        canAddDesktopFile = model->canAddDesktopFile();
        canAddIconFile = model->canAddIcon();
    }
    ui->addDesktopFileButton->setEnabled(canAddDesktopFile);
    ui->addIconButton->setEnabled(canAddIconFile);
}

void MaemoDeployStepWidget::addDesktopFile()
{
    const int modelRow = ui->modelComboBox->currentIndex();
    if (modelRow == -1)
        return;
    MaemoDeployableListModel *const model
        = m_step->deployables()->modelAt(modelRow);
    QString error;
    if (!model->addDesktopFile(error)) {
        QMessageBox::warning(this, tr("Could not create desktop file"),
             tr("Error creating desktop file: %1").arg(error));
    }
    ui->addDesktopFileButton->setEnabled(model->canAddDesktopFile());
    ui->tableView->resizeRowsToContents();
}

void MaemoDeployStepWidget::addIcon()
{
    const int modelRow = ui->modelComboBox->currentIndex();
    if (modelRow == -1)
        return;

    MaemoDeployableListModel *const model
        = m_step->deployables()->modelAt(modelRow);
    const QString origFilePath = QFileDialog::getOpenFileName(this,
        tr("Choose Icon (will be scaled to 64x64 pixels, if necessary)"),
        model->projectDir(), QLatin1String("(*.png)"));
    if (origFilePath.isEmpty())
        return;
    QPixmap pixmap(origFilePath);
    if (pixmap.isNull()) {
        QMessageBox::critical(this, tr("Invalid Icon"),
            tr("Unable to read image"));
        return;
    }
    const QSize iconSize(64, 64);
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

    QString error;
    if (!model->addIcon(newFileName, error)) {
        QMessageBox::critical(this, tr("Could Not Add Icon"),
             tr("Error adding icon: %1").arg(error));
    }
    ui->addIconButton->setEnabled(model->canAddIcon());
    ui->tableView->resizeRowsToContents();
}

void MaemoDeployStepWidget::showDeviceConfigurations()
{
    MaemoDeviceConfigurationsSettingsPage *page
        = MaemoManager::instance().deviceConfigurationsSettingsPage();
    Core::ICore::instance()->showOptionsDialog(page->category(), page->id());
}

} // namespace Internal
} // namespace Qt4ProjectManager
