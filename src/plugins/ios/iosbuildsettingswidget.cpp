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

#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakenodes.h>

#include <projectexplorer/runconfiguration.h>

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QLoggingCategory>
#include <QVBoxLayout>

namespace Ios {
namespace Internal {

namespace {
Q_LOGGING_CATEGORY(iosSettingsLog, "qtc.ios.common", QtWarningMsg)
}

static const int IdentifierRole = Qt::UserRole+1;

IosBuildSettingsWidget::IosBuildSettingsWidget(const Core::Id &deviceType,
                                               const QString &signingIdentifier,
                                               bool isSigningAutoManaged, QWidget *parent) :
    ProjectExplorer::NamedWidget(parent),
    m_deviceType(deviceType)
{
    const bool isDevice = m_deviceType == Constants::IOS_DEVICE_TYPE;

    auto detailsWidget = new Utils::DetailsWidget(this);
    auto container = new QWidget(detailsWidget);

    m_qmakeDefaults = new QPushButton(container);
    QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    m_qmakeDefaults->setSizePolicy(sizePolicy);
    m_qmakeDefaults->setText(tr("Reset"));
    m_qmakeDefaults->setEnabled(isDevice);

    m_signEntityCombo = new QComboBox(container);
    QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Fixed);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    m_signEntityCombo->setSizePolicy(sizePolicy1);

    m_autoSignCheckbox = new QCheckBox(container);
    QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Fixed);
    sizePolicy2.setHorizontalStretch(0);
    sizePolicy2.setVerticalStretch(0);
    m_autoSignCheckbox->setSizePolicy(sizePolicy2);
    m_autoSignCheckbox->setChecked(true);
    m_autoSignCheckbox->setText(tr("Automatically manage signing"));
    m_autoSignCheckbox->setChecked(isSigningAutoManaged);
    m_autoSignCheckbox->setEnabled(isDevice);

    m_signEntityLabel = new QLabel(container);

    m_infoIconLabel = new QLabel(container);
    QSizePolicy sizePolicy3(QSizePolicy::Maximum, QSizePolicy::Preferred);
    sizePolicy3.setHorizontalStretch(0);
    sizePolicy3.setVerticalStretch(0);
    m_infoIconLabel->setSizePolicy(sizePolicy3);

    m_infoLabel = new QLabel(container);
    QSizePolicy sizePolicy4(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy4.setHorizontalStretch(0);
    sizePolicy4.setVerticalStretch(0);
    m_infoLabel->setSizePolicy(sizePolicy4);
    m_infoLabel->setWordWrap(false);

    m_warningIconLabel = new QLabel(container);
    m_warningIconLabel->setSizePolicy(sizePolicy3);

    m_warningLabel = new QLabel(container);
    m_warningLabel->setSizePolicy(sizePolicy4);
    m_warningLabel->setWordWrap(true);

    m_signEntityLabel->setText(QApplication::translate("Ios::Internal::IosBuildSettingsWidget", "Development team:", nullptr));

    connect(m_qmakeDefaults, &QPushButton::clicked, this, &IosBuildSettingsWidget::onReset);

    m_infoIconLabel->hide();
    m_infoIconLabel->setPixmap(Utils::Icons::INFO.pixmap());
    m_infoLabel->hide();

    m_warningIconLabel->hide();
    m_warningIconLabel->setPixmap(Utils::Icons::WARNING.pixmap());
    m_warningLabel->hide();

    detailsWidget->setState(Utils::DetailsWidget::NoSummary);
    detailsWidget->setWidget(container);

    setDisplayName(tr("iOS Settings"));

    if (isDevice) {
        connect(IosConfigurations::instance(), &IosConfigurations::provisioningDataChanged,
                this, &IosBuildSettingsWidget::populateDevelopmentTeams);
        connect(m_signEntityCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                this, &IosBuildSettingsWidget::onSigningEntityComboIndexChanged);
        connect(m_autoSignCheckbox, &QCheckBox::toggled,
                this, &IosBuildSettingsWidget::configureSigningUi);
        configureSigningUi(m_autoSignCheckbox->isChecked());
        setDefaultSigningIdentfier(signingIdentifier);
    }

    m_signEntityCombo->setEnabled(isDevice);
    m_signEntityLabel->setEnabled(isDevice);
    adjustSize();

    auto rootLayout = new QVBoxLayout(this);
    rootLayout->setMargin(0);
    rootLayout->addWidget(detailsWidget);

    auto gridLayout = new QGridLayout();
    gridLayout->addWidget(m_signEntityLabel, 0, 0, 1, 1);
    gridLayout->addWidget(m_signEntityCombo, 0, 1, 1, 1);
    gridLayout->addWidget(m_autoSignCheckbox, 0, 2, 1, 1);
    gridLayout->addWidget(m_qmakeDefaults, 1, 1, 1, 1);

    auto horizontalLayout = new QHBoxLayout();
    horizontalLayout->addWidget(m_infoIconLabel);
    horizontalLayout->addWidget(m_infoLabel);

    auto horizontalLayout_2 = new QHBoxLayout();
    horizontalLayout_2->addWidget(m_warningIconLabel);
    horizontalLayout_2->addWidget(m_warningLabel);

    auto verticalLayout = new QVBoxLayout(container);
    verticalLayout->addLayout(gridLayout);
    verticalLayout->addLayout(horizontalLayout);
    verticalLayout->addLayout(horizontalLayout_2);
}

void IosBuildSettingsWidget::setDefaultSigningIdentfier(const QString &identifier) const
{
    if (identifier.isEmpty()) {
        m_signEntityCombo->setCurrentIndex(0);
        return;
    }

    int defaultIndex = -1;
    for (int index = 0; index < m_signEntityCombo->count(); ++index) {
        QString teamID = m_signEntityCombo->itemData(index, IdentifierRole).toString();
        if (teamID == identifier) {
            defaultIndex = index;
            break;
        }
    }
    if (defaultIndex > -1) {
        m_signEntityCombo->setCurrentIndex(defaultIndex);
    } else {
        // Reset to default
        m_signEntityCombo->setCurrentIndex(0);
        qCDebug(iosSettingsLog) << "Cannot find default"
                                << (m_autoSignCheckbox->isChecked() ? "team": "provisioning profile")
                                << ". Identifier: " << identifier;
    }
}

bool IosBuildSettingsWidget::isSigningAutomaticallyManaged() const
{
    return m_autoSignCheckbox->isChecked() && m_signEntityCombo->currentIndex() > 0;
}

void IosBuildSettingsWidget::onSigningEntityComboIndexChanged()
{
    QString identifier = selectedIdentifier();
    (m_autoSignCheckbox->isChecked() ? m_lastTeamSelection : m_lastProfileSelection) = identifier;

    updateInfoText();
    updateWarningText();
    emit signingSettingsChanged(m_autoSignCheckbox->isChecked(), identifier);
}

void IosBuildSettingsWidget::onReset()
{
    m_lastTeamSelection.clear();
    m_lastProfileSelection.clear();
    m_autoSignCheckbox->setChecked(true);
    setDefaultSigningIdentfier("");
}

void IosBuildSettingsWidget::configureSigningUi(bool autoManageSigning)
{
    m_signEntityLabel->setText(autoManageSigning ? tr("Development team:")
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
    {
        QSignalBlocker blocker(m_signEntityCombo);
        // Populate Team id's
        m_signEntityCombo->clear();
        m_signEntityCombo->addItem(tr("Default"));
        foreach (auto team, IosConfigurations::developmentTeams()) {
            m_signEntityCombo->addItem(team->displayName());
            const int index = m_signEntityCombo->count() - 1;
            m_signEntityCombo->setItemData(index, team->identifier(), IdentifierRole);
            m_signEntityCombo->setItemData(index, team->details(), Qt::ToolTipRole);
        }
    }
    // Maintain previous selection.
    setDefaultSigningIdentfier(m_lastTeamSelection);
    updateWarningText();
}

void IosBuildSettingsWidget::populateProvisioningProfiles()
{
    {
        // Populate Team id's
        QSignalBlocker blocker(m_signEntityCombo);
        m_signEntityCombo->clear();
        const ProvisioningProfiles profiles = IosConfigurations::provisioningProfiles();
        if (profiles.count() > 0) {
            for (auto profile : profiles) {
                m_signEntityCombo->addItem(profile->displayName());
                const int index = m_signEntityCombo->count() - 1;
                m_signEntityCombo->setItemData(index, profile->identifier(), IdentifierRole);
                m_signEntityCombo->setItemData(index, profile->details(), Qt::ToolTipRole);
            }
        } else {
            m_signEntityCombo->addItem(tr("None"));
        }
    }
    // Maintain previous selection.
    setDefaultSigningIdentfier(m_lastProfileSelection);
    updateWarningText();
}

QString IosBuildSettingsWidget::selectedIdentifier() const
{
    return m_signEntityCombo->currentData(IdentifierRole).toString();
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
    bool configuringTeams = m_autoSignCheckbox->isChecked();

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

    m_infoIconLabel->setVisible(!infoMessage.isEmpty());
    m_infoLabel->setVisible(!infoMessage.isEmpty());
    m_infoLabel->setText(infoMessage);
}

void IosBuildSettingsWidget::updateWarningText()
{
    if (m_deviceType != Constants::IOS_DEVICE_TYPE)
        return;

    QString warningText;
    bool configuringTeams = m_autoSignCheckbox->isChecked();
    if (m_signEntityCombo->count() < 2) {
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

    m_warningLabel->setVisible(!warningText.isEmpty());
    m_warningIconLabel->setVisible(!warningText.isEmpty());
    m_warningLabel->setText(warningText);
}

} // namespace Internal
} // namespace Ios
