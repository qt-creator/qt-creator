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

#include "iosbuildsettingswidget.h"
#include "ui_iosbuildsettingswidget.h"
#include "iosconfigurations.h"
#include "iosconstants.h"

#include "utils/utilsicons.h"
#include "utils/algorithm.h"
#include "qmakeprojectmanager/qmakeproject.h"
#include "qmakeprojectmanager/qmakenodes.h"
#include "utils/detailswidget.h"

#include <QLoggingCategory>
#include <QVBoxLayout>

using namespace QmakeProjectManager;

namespace Ios {
namespace Internal {

namespace {
Q_LOGGING_CATEGORY(iosSettingsLog, "qtc.ios.common")
}

static const int IdentifierRole = Qt::UserRole+1;

IosBuildSettingsWidget::IosBuildSettingsWidget(const Core::Id &deviceType,
                                               const QString &signingIdentifier,
                                               bool isSigningAutoManaged, QWidget *parent) :
    ProjectExplorer::NamedWidget(parent),
    ui(new Ui::IosBuildSettingsWidget),
    m_detailsWidget(new Utils::DetailsWidget(this)),
    m_deviceType(deviceType)
{
    auto rootLayout = new QVBoxLayout(this);
    rootLayout->setMargin(0);
    rootLayout->addWidget(m_detailsWidget);

    auto container = new QWidget(m_detailsWidget);
    ui->setupUi(container);
    ui->m_autoSignCheckbox->setChecked(isSigningAutoManaged);
    connect(ui->m_qmakeDefaults, &QPushButton::clicked, this, &IosBuildSettingsWidget::onReset);

    ui->m_infoIconLabel->hide();
    ui->m_infoIconLabel->setPixmap(Utils::Icons::INFO.pixmap());
    ui->m_infoLabel->hide();

    ui->m_warningIconLabel->hide();
    ui->m_warningIconLabel->setPixmap(Utils::Icons::WARNING.pixmap());
    ui->m_warningLabel->hide();

    m_detailsWidget->setState(Utils::DetailsWidget::NoSummary);
    m_detailsWidget->setWidget(container);

    setDisplayName(tr("iOS Settings"));

    const bool isDevice = m_deviceType == Constants::IOS_DEVICE_TYPE;
    if (isDevice) {
        connect(IosConfigurations::instance(), &IosConfigurations::provisioningDataChanged,
                this, &IosBuildSettingsWidget::populateDevelopmentTeams);
        connect(ui->m_signEntityCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this, &IosBuildSettingsWidget::onSigningEntityComboIndexChanged);
        connect(ui->m_autoSignCheckbox, &QCheckBox::toggled,
                this, &IosBuildSettingsWidget::configureSigningUi);
        configureSigningUi(ui->m_autoSignCheckbox->isChecked());
        setDefaultSigningIdentfier(signingIdentifier);
    }

    ui->m_autoSignCheckbox->setEnabled(isDevice);
    ui->m_signEntityCombo->setEnabled(isDevice);
    ui->m_qmakeDefaults->setEnabled(isDevice);
    ui->m_signEntityLabel->setEnabled(isDevice);
    adjustSize();
}

IosBuildSettingsWidget::~IosBuildSettingsWidget()
{
    delete ui;
}

void IosBuildSettingsWidget::setDefaultSigningIdentfier(const QString &identifier) const
{
    if (identifier.isEmpty()) {
        ui->m_signEntityCombo->setCurrentIndex(0);
        return;
    }

    int defaultIndex = -1;
    for (int index = 0; index < ui->m_signEntityCombo->count(); ++index) {
        QString teamID = ui->m_signEntityCombo->itemData(index, IdentifierRole).toString();
        if (teamID == identifier) {
            defaultIndex = index;
            break;
        }
    }
    if (defaultIndex > -1) {
        ui->m_signEntityCombo->setCurrentIndex(defaultIndex);
    } else {
        // Reset to default
        ui->m_signEntityCombo->setCurrentIndex(0);
        qCDebug(iosSettingsLog) << "Can not find default"
                                << (ui->m_autoSignCheckbox->isChecked() ? "team": "provisioning profile")
                                << ". Identifier: " << identifier;
    }
}

bool IosBuildSettingsWidget::isSigningAutomaticallyManaged() const
{
    return ui->m_autoSignCheckbox->isChecked() && ui->m_signEntityCombo->currentIndex() > 0;
}

void IosBuildSettingsWidget::onSigningEntityComboIndexChanged()
{
    QString identifier = selectedIdentifier();
    (ui->m_autoSignCheckbox->isChecked() ? m_lastTeamSelection : m_lastProfileSelection) = identifier;

    updateInfoText();
    updateWarningText();
    emit signingSettingsChanged(ui->m_autoSignCheckbox->isChecked(), identifier);
}

void IosBuildSettingsWidget::onReset()
{
    m_lastTeamSelection.clear();
    m_lastProfileSelection.clear();
    ui->m_autoSignCheckbox->setChecked(true);
    setDefaultSigningIdentfier("");
}

void IosBuildSettingsWidget::configureSigningUi(bool autoManageSigning)
{
    ui->m_signEntityLabel->setText(autoManageSigning ? tr("Development team:")
                                                     : tr("Provisioning profile:"));
    if (autoManageSigning)
        populateDevelopmentTeams();
    else
        populateProvisioningProfiles();

    updateInfoText();
    emit signingSettingsChanged(autoManageSigning, selectedIdentifier());
}

void IosBuildSettingsWidget::populateDevelopmentTeams()
{
    // Populate Team id's
    ui->m_signEntityCombo->blockSignals(true);
    ui->m_signEntityCombo->clear();
    ui->m_signEntityCombo->addItem(tr("Default"));
    foreach (auto team, IosConfigurations::developmentTeams()) {
        ui->m_signEntityCombo->addItem(team->displayName());
        const int index = ui->m_signEntityCombo->count() - 1;
        ui->m_signEntityCombo->setItemData(index, team->identifier(), IdentifierRole);
        ui->m_signEntityCombo->setItemData(index, team->details(), Qt::ToolTipRole);
    }
    ui->m_signEntityCombo->blockSignals(false);
    // Maintain previous selection.
    setDefaultSigningIdentfier(m_lastTeamSelection);
    updateWarningText();
}

void IosBuildSettingsWidget::populateProvisioningProfiles()
{
    // Populate Team id's
    ui->m_signEntityCombo->blockSignals(true);
    ui->m_signEntityCombo->clear();
    ProvisioningProfiles profiles = IosConfigurations::provisioningProfiles();
    if (profiles.count() > 0) {
        foreach (auto profile, profiles) {
            ui->m_signEntityCombo->addItem(profile->displayName());
            const int index = ui->m_signEntityCombo->count() - 1;
            ui->m_signEntityCombo->setItemData(index, profile->identifier(), IdentifierRole);
            ui->m_signEntityCombo->setItemData(index, profile->details(), Qt::ToolTipRole);
        }
    } else {
        ui->m_signEntityCombo->addItem(tr("None"));
    }
    ui->m_signEntityCombo->blockSignals(false);
    // Maintain previous selection.
    setDefaultSigningIdentfier(m_lastProfileSelection);
    updateWarningText();
}

QString IosBuildSettingsWidget::selectedIdentifier() const
{
    return ui->m_signEntityCombo->currentData(IdentifierRole).toString();
}

void IosBuildSettingsWidget::updateInfoText()
{
    if (m_deviceType != Constants::IOS_DEVICE_TYPE)
        return;

    QString infoMessage;
    auto addMessage = [&infoMessage](const QString &msg) {
        if (!infoMessage.isEmpty())
            infoMessage += "\n";
        infoMessage += msg;
    };

    QString identifier = selectedIdentifier();
    bool configuringTeams = ui->m_autoSignCheckbox->isChecked();

    if (identifier.isEmpty()) {
        // No signing entity selection.
        if (configuringTeams)
            addMessage(tr("Development team is not selected."));
        else
            addMessage(tr("Provisioning profile is not selected."));

        addMessage(tr("Using default development team and provisioning profile."));
    } else {
        if (!configuringTeams) {
            ProvisioningProfilePtr profile =  IosConfigurations::provisioningProfile(identifier);
            QTC_ASSERT(profile, return);
            auto team = profile->developmentTeam();
            if (team) {
                // Display corresponding team information.
                addMessage(tr("Development team: %1 (%2)").arg(team->displayName())
                           .arg(team->identifier()));
                addMessage(tr("Settings defined here override the QMake environment."));
            } else {
                qCDebug(iosSettingsLog) << "Development team not found for profile" << profile;
            }
        } else {
            addMessage(tr("Settings defined here override the QMake environment."));
        }
    }

    ui->m_infoIconLabel->setVisible(!infoMessage.isEmpty());
    ui->m_infoLabel->setVisible(!infoMessage.isEmpty());
    ui->m_infoLabel->setText(infoMessage);
}

void IosBuildSettingsWidget::updateWarningText()
{
    if (m_deviceType != Constants::IOS_DEVICE_TYPE)
        return;

    QString warningText;
    bool configuringTeams = ui->m_autoSignCheckbox->isChecked();
    if (ui->m_signEntityCombo->count() < 2) {
        warningText = tr("%1 not configured. Use Xcode and Apple developer account to configure the "
                         "provisioning profiles and teams.")
                .arg(configuringTeams ? tr("Development teams") : tr("Provisioning profiles"));
    } else {
        QString identifier = selectedIdentifier();
        if (configuringTeams) {
            auto team = IosConfigurations::developmentTeam(identifier);
            if (team && !team->hasProvisioningProfile())
                warningText = tr("No provisioning profile found for the selected team.");
        } else {
            auto profile = IosConfigurations::provisioningProfile(identifier);
            if (profile && QDateTime::currentDateTimeUtc() > profile->expirationDate()) {
               warningText = tr("Provisioning profile expired. Expiration date: %1")
                       .arg(profile->expirationDate().toLocalTime().toString(Qt::SystemLocaleLongDate));
            }
        }
    }

    ui->m_warningLabel->setVisible(!warningText.isEmpty());
    ui->m_warningIconLabel->setVisible(!warningText.isEmpty());
    ui->m_warningLabel->setText(warningText);
}

} // namespace Internal
} // namespace Ios
