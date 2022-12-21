// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qnxsettingspage.h"

#include "qnxconfiguration.h"
#include "qnxconfigurationmanager.h"
#include "qnxtr.h"

#include <coreplugin/icore.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <qtsupport/qtversionmanager.h>

#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>

using namespace Utils;

namespace Qnx::Internal {

class QnxSettingsWidget final : public Core::IOptionsPageWidget
{
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
    QComboBox *m_configsCombo = new QComboBox;
    QCheckBox *m_generateKitsCheckBox = new QCheckBox(Tr::tr("Generate kits"));
    QLabel *m_configName = new QLabel;
    QLabel *m_configVersion = new QLabel;
    QLabel *m_configHost = new QLabel;
    QLabel *m_configTarget = new QLabel;

    QnxConfigurationManager *m_qnxConfigManager = QnxConfigurationManager::instance();
    QList<ConfigState> m_changedConfigs;
};

QnxSettingsWidget::QnxSettingsWidget()
{
    auto addButton = new QPushButton(Tr::tr("Add..."));
    auto removeButton = new QPushButton(Tr::tr("Remove"));

    using namespace Layouting;

    Row {
        Column {
            m_configsCombo,
            Row { m_generateKitsCheckBox, st },
            Group {
                title(Tr::tr("Configuration Information:")),
                Form {
                    Tr::tr("Name:"), m_configName, br,
                    Tr::tr("Version:"), m_configVersion, br,
                    Tr::tr("Host:"), m_configHost, br,
                    Tr::tr("Target:"), m_configTarget
                }
            },
            st
        },
        Column {
            addButton,
            removeButton,
            st
        }
    }.attachTo(this);

    populateConfigsCombo();
    connect(addButton, &QAbstractButton::clicked,
            this, &QnxSettingsWidget::addConfiguration);
    connect(removeButton, &QAbstractButton::clicked,
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
    const QList<QnxConfiguration *> configList = m_qnxConfigManager->configurations();
    for (QnxConfiguration *config : configList) {
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

    for (const ConfigState &configState : std::as_const(m_changedConfigs)) {
        if (configState.config == config && configState.state == stateToRemove)
            m_changedConfigs.removeAll(configState);
    }

     m_changedConfigs.append(ConfigState{config, state});
}

void QnxSettingsWidget::apply()
{
    for (const ConfigState &configState : std::as_const(m_changedConfigs)) {
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

} // Qnx::Internal
