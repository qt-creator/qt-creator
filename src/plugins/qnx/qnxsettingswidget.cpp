/**************************************************************************
**
** Copyright (C) 2015 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qnxsettingswidget.h"
#include "ui_qnxsettingswidget.h"
#include "qnxconfiguration.h"
#include "qnxconfigurationmanager.h"

#include <qtsupport/qtversionmanager.h>
#include <coreplugin/icore.h>

#include <QFileDialog>
#include <QMessageBox>

#include <qdebug.h>

namespace Qnx {
namespace Internal {

QnxSettingsWidget::QnxSettingsWidget(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui_QnxSettingsWidget),
    m_qnxConfigManager(QnxConfigurationManager::instance())
{
    m_ui->setupUi(this);

    populateConfigsCombo();
    connect(m_ui->addButton, SIGNAL(clicked()),
            this, SLOT(addConfiguration()));
    connect(m_ui->removeButton, SIGNAL(clicked()),
            this, SLOT(removeConfiguration()));
    connect(m_ui->configsCombo, SIGNAL(currentIndexChanged(QString)),
            this, SLOT(updateInformation()));
    connect(m_ui->generateKitsCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(generateKits(bool)));
    connect(m_qnxConfigManager, SIGNAL(configurationsListUpdated()),
            this, SLOT(populateConfigsCombo()));
    connect(QtSupport::QtVersionManager::instance(),
            SIGNAL(qtVersionsChanged(QList<int>,QList<int>,QList<int>)),
            this, SLOT(updateInformation()));
}

QnxSettingsWidget::~QnxSettingsWidget()
{
    delete m_ui;
}

QList<QnxSettingsWidget::ConfigState> QnxSettingsWidget::changedConfigs()
{
    return m_changedConfigs;
}

void QnxSettingsWidget::addConfiguration()
{
    QString filter;
    if (Utils::HostOsInfo::isWindowsHost())
        filter = QLatin1String("*.bat file");
    else
        filter = QLatin1String("*.sh file");

    const QString envFile = QFileDialog::getOpenFileName(this, tr("Select QNX Environment File"),
                                                         QString(), filter);
    if (envFile.isEmpty())
        return;

    QnxConfiguration *config = new QnxConfiguration(Utils::FileName::fromString(envFile));
    if (m_qnxConfigManager->configurations().contains(config)
            || !config->isValid()) {
        QMessageBox::warning(Core::ICore::mainWindow(), tr("Warning"),
                             tr("Configuration already exists or is invalid."));
        delete config;
        return;
    }

    setConfigState(config, Added);
    m_ui->configsCombo->addItem(config->displayName(),
                                   QVariant::fromValue(static_cast<void*>(config)));
}

void QnxSettingsWidget::removeConfiguration()
{
    const int currentIndex = m_ui->configsCombo->currentIndex();
    QnxConfiguration *config = static_cast<QnxConfiguration*>(
            m_ui->configsCombo->itemData(currentIndex).value<void*>());

    if (!config)
        return;

    QMessageBox::StandardButton button =
            QMessageBox::question(Core::ICore::mainWindow(),
                                  tr("Remove QNX Configuration"),
                                  tr("Are you sure you want to remove:\n %1?").arg(config->displayName()),
                                  QMessageBox::Yes | QMessageBox::No);

    if (button == QMessageBox::Yes) {
        setConfigState(config, Removed);
        m_ui->configsCombo->removeItem(currentIndex);
    }
}

void QnxSettingsWidget::generateKits(bool checked)
{
    const int currentIndex = m_ui->configsCombo->currentIndex();
    QnxConfiguration *config = static_cast<QnxConfiguration*>(
                m_ui->configsCombo->itemData(currentIndex).value<void*>());
    if (!config)
        return;

    setConfigState(config, checked ? Activated : Deactivated);
}

void QnxSettingsWidget::updateInformation()
{
    const int currentIndex = m_ui->configsCombo->currentIndex();

    QnxConfiguration *config = static_cast<QnxConfiguration*>(
            m_ui->configsCombo->itemData(currentIndex).value<void*>());

    // update the checkbox
    m_ui->generateKitsCheckBox->setEnabled(config ? config->canCreateKits() : false);
    m_ui->generateKitsCheckBox->setChecked(config ? config->isActive() : false);

    // update information
    m_ui->configName->setText(config ? config->displayName() : QString());
    m_ui->configVersion->setText(config ? config->version().toString() : QString());
    m_ui->configHost->setText(config ? config->qnxHost().toString() : QString());
    m_ui->configTarget->setText(config ? config->qnxTarget().toString() : QString());

}

void QnxSettingsWidget::populateConfigsCombo()
{
    m_ui->configsCombo->clear();
    foreach (QnxConfiguration *config,
             m_qnxConfigManager->configurations()) {
        m_ui->configsCombo->addItem(config->displayName(),
                                       QVariant::fromValue(static_cast<void*>(config)));
    }

    updateInformation();
}

void QnxSettingsWidget::setConfigState(QnxConfiguration *config,
                                       QnxSettingsWidget::State state)
{
    QnxSettingsWidget::State stateToRemove;
    switch (state) {
    case QnxSettingsWidget::Added :
        stateToRemove = QnxSettingsWidget::Removed;
        break;
    case QnxSettingsWidget::Removed:
        stateToRemove = QnxSettingsWidget::Added;
        break;
    case QnxSettingsWidget::Activated:
        stateToRemove = QnxSettingsWidget::Deactivated;
        break;
    case QnxSettingsWidget::Deactivated:
        stateToRemove = QnxSettingsWidget::Activated;
    }

    foreach (const ConfigState &configState, m_changedConfigs) {
        if (configState.config == config && configState.state == stateToRemove)
            m_changedConfigs.removeAll(configState);
    }

     m_changedConfigs.append(ConfigState(config, state));
}

void QnxSettingsWidget::applyChanges()
{
    foreach (const ConfigState &configState, m_changedConfigs) {
        switch (configState.state) {
        case Activated :
            configState.config->activate();
            break;
        case Deactivated:
            configState.config->deactivate();
            break;
        case Added:
            m_qnxConfigManager->addConfiguration(configState.config);
            break;
        case Removed:
            configState.config->deactivate();
            m_qnxConfigManager->removeConfiguration(configState.config);
            break;
        }
    }

    m_changedConfigs.clear();
}

} // namespace Internal
} // namespace Qnx
