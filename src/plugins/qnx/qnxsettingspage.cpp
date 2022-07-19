/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: BlackBerry (qt@blackberry.com)
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

#include "qnxsettingspage.h"

#include "qnxconfiguration.h"
#include "qnxconfigurationmanager.h"
#include "qnxtr.h"

#include <coreplugin/icore.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <qtsupport/qtversionmanager.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpacerItem>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

using namespace Utils;

namespace Qnx {
namespace Internal {

class QnxSettingsWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(Qnx::Internal::QnxSettingsWidget)

public:
    QnxSettingsWidget();

    enum State {
        Activated,
        Deactivated,
        Added,
        Removed
    };

    class ConfigState {
    public:
        bool operator ==(const ConfigState &cs) const
        {
            return config == cs.config && state == cs.state;
        }

        QnxConfiguration *config;
        State state;
    };

    void apply() final;

    void addConfiguration();
    void removeConfiguration();
    void generateKits(bool checked);
    void updateInformation();
    void populateConfigsCombo();

    void setConfigState(QnxConfiguration *config, State state);

private:
    QComboBox *m_configsCombo;
    QCheckBox *m_generateKitsCheckBox;
    QGroupBox *m_groupBox;
    QLabel *m_configName;
    QLabel *m_configVersion;
    QLabel *m_configHost;
    QLabel *m_configTarget;
    QPushButton *m_addButton;
    QPushButton *m_removeButton;

    QnxConfigurationManager *m_qnxConfigManager;
    QList<ConfigState> m_changedConfigs;
};

QnxSettingsWidget::QnxSettingsWidget() :
    m_qnxConfigManager(QnxConfigurationManager::instance())
{
    m_configsCombo = new QComboBox(this);

    m_generateKitsCheckBox = new QCheckBox(this);
    m_generateKitsCheckBox->setText(Tr::tr("Generate kits"));

    m_groupBox = new QGroupBox(this);
    m_groupBox->setMinimumSize(QSize(0, 0));
    m_groupBox->setTitle(Tr::tr("Configuration Information:"));

    m_configName = new QLabel(m_groupBox);
    m_configVersion = new QLabel(m_groupBox);
    m_configTarget = new QLabel(m_groupBox);
    m_configHost = new QLabel(m_groupBox);

    m_addButton = new QPushButton(Tr::tr("Add..."));
    m_removeButton = new QPushButton(Tr::tr("Remove"));

    auto verticalLayout_3 = new QVBoxLayout();
    verticalLayout_3->addWidget(new QLabel(Tr::tr("Name:")));
    verticalLayout_3->addWidget(new QLabel(Tr::tr("Version:")));
    verticalLayout_3->addWidget(new QLabel(Tr::tr("Host:")));
    verticalLayout_3->addWidget(new QLabel(Tr::tr("Target:")));
    verticalLayout_3->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

    auto verticalLayout_4 = new QVBoxLayout();
    verticalLayout_4->addWidget(m_configName);
    verticalLayout_4->addWidget(m_configVersion);
    verticalLayout_4->addWidget(m_configHost);
    verticalLayout_4->addWidget(m_configTarget);
    verticalLayout_4->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

    auto horizontalLayout_3 = new QHBoxLayout(m_groupBox);
    horizontalLayout_3->addLayout(verticalLayout_3);
    horizontalLayout_3->addLayout(verticalLayout_4);
    horizontalLayout_3->addItem(new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum));

    auto verticalLayout_2 = new QVBoxLayout();
    verticalLayout_2->addWidget(m_configsCombo);
    verticalLayout_2->addWidget(m_generateKitsCheckBox);
    verticalLayout_2->addWidget(m_groupBox);

    auto verticalLayout = new QVBoxLayout();
    verticalLayout->setSizeConstraint(QLayout::SetMaximumSize);
    verticalLayout->addWidget(m_addButton);
    verticalLayout->addWidget(m_removeButton);
    verticalLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

    auto horizontalLayout = new QHBoxLayout();
    horizontalLayout->addLayout(verticalLayout_2);
    horizontalLayout->addLayout(verticalLayout);

    auto horizontalLayout_2 = new QHBoxLayout(this);
    horizontalLayout_2->addLayout(horizontalLayout);

    populateConfigsCombo();
    connect(m_addButton, &QAbstractButton::clicked,
            this, &QnxSettingsWidget::addConfiguration);
    connect(m_removeButton, &QAbstractButton::clicked,
            this, &QnxSettingsWidget::removeConfiguration);
    connect(m_configsCombo, &QComboBox::currentIndexChanged,
            this, &QnxSettingsWidget::updateInformation);
    connect(m_generateKitsCheckBox, &QAbstractButton::toggled,
            this, &QnxSettingsWidget::generateKits);
    connect(m_qnxConfigManager, &QnxConfigurationManager::configurationsListUpdated,
            this, &QnxSettingsWidget::populateConfigsCombo);
    connect(QtSupport::QtVersionManager::instance(),
            &QtSupport::QtVersionManager::qtVersionsChanged,
            this, &QnxSettingsWidget::updateInformation);
}

void QnxSettingsWidget::addConfiguration()
{
    QString filter;
    if (Utils::HostOsInfo::isWindowsHost())
        filter = "*.bat file";
    else
        filter = "*.sh file";

    const FilePath envFile = FileUtils::getOpenFilePath(this, Tr::tr("Select QNX Environment File"),
                                                        {}, filter);
    if (envFile.isEmpty())
        return;

    QnxConfiguration *config = new QnxConfiguration(envFile);
    if (m_qnxConfigManager->configurations().contains(config)
            || !config->isValid()) {
        QMessageBox::warning(Core::ICore::dialogParent(),
                             Tr::tr("Warning"),
                             Tr::tr("Configuration already exists or is invalid."));
        delete config;
        return;
    }

    setConfigState(config, Added);
    m_configsCombo->addItem(config->displayName(),
                            QVariant::fromValue(static_cast<void*>(config)));
}

void QnxSettingsWidget::removeConfiguration()
{
    const int currentIndex = m_configsCombo->currentIndex();
    auto config = static_cast<QnxConfiguration*>(
            m_configsCombo->itemData(currentIndex).value<void*>());

    if (!config)
        return;

    QMessageBox::StandardButton button =
            QMessageBox::question(Core::ICore::dialogParent(),
                                  Tr::tr("Remove QNX Configuration"),
                                  Tr::tr("Are you sure you want to remove:\n %1?").arg(config->displayName()),
                                  QMessageBox::Yes | QMessageBox::No);

    if (button == QMessageBox::Yes) {
        setConfigState(config, Removed);
        m_configsCombo->removeItem(currentIndex);
    }
}

void QnxSettingsWidget::generateKits(bool checked)
{
    const int currentIndex = m_configsCombo->currentIndex();
    auto config = static_cast<QnxConfiguration*>(
                m_configsCombo->itemData(currentIndex).value<void*>());
    if (!config)
        return;

    setConfigState(config, checked ? Activated : Deactivated);
}

void QnxSettingsWidget::updateInformation()
{
    const int currentIndex = m_configsCombo->currentIndex();

    auto config = static_cast<QnxConfiguration*>(
            m_configsCombo->itemData(currentIndex).value<void*>());

    // update the checkbox
    m_generateKitsCheckBox->setEnabled(config ? config->canCreateKits() : false);
    m_generateKitsCheckBox->setChecked(config ? config->isActive() : false);

    // update information
    m_configName->setText(config ? config->displayName() : QString());
    m_configVersion->setText(config ? config->version().toString() : QString());
    m_configHost->setText(config ? config->qnxHost().toString() : QString());
    m_configTarget->setText(config ? config->qnxTarget().toString() : QString());
}

void QnxSettingsWidget::populateConfigsCombo()
{
    m_configsCombo->clear();
    foreach (QnxConfiguration *config, m_qnxConfigManager->configurations()) {
        m_configsCombo->addItem(config->displayName(),
                                       QVariant::fromValue(static_cast<void*>(config)));
    }

    updateInformation();
}

void QnxSettingsWidget::setConfigState(QnxConfiguration *config, State state)
{
    State stateToRemove = Activated;
    switch (state) {
    case Added :
        stateToRemove = Removed;
        break;
    case Removed:
        stateToRemove = Added;
        break;
    case Activated:
        stateToRemove = Deactivated;
        break;
    case Deactivated:
        stateToRemove = Activated;
        break;
    }

    for (const ConfigState &configState : qAsConst(m_changedConfigs)) {
        if (configState.config == config && configState.state == stateToRemove)
            m_changedConfigs.removeAll(configState);
    }

     m_changedConfigs.append(ConfigState{config, state});
}

void QnxSettingsWidget::apply()
{
    for (const ConfigState &configState : qAsConst(m_changedConfigs)) {
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


// QnxSettingsPage

QnxSettingsPage::QnxSettingsPage()
{
    setId("DD.Qnx Configuration");
    setDisplayName(Tr::tr("QNX"));
    setCategory(ProjectExplorer::Constants::DEVICE_SETTINGS_CATEGORY);
    setWidgetCreator([] { return new QnxSettingsWidget; });
}

} // namespace Internal
} // namespace Qnx
