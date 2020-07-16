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

#include "iosbuildconfiguration.h"

#include "iosconfigurations.h"
#include "iosconstants.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/namedwidget.h>
#include <projectexplorer/target.h>

#include <qmakeprojectmanager/qmakebuildinfo.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/infolabel.h>

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

using namespace QmakeProjectManager;
using namespace ProjectExplorer;

namespace Ios {
namespace Internal {

static Q_LOGGING_CATEGORY(iosSettingsLog, "qtc.ios.common", QtWarningMsg)

const char qmakeIosTeamSettings[] = "QMAKE_MAC_XCODE_SETTINGS+=qteam qteam.name=DEVELOPMENT_TEAM qteam.value=";
const char qmakeProvisioningProfileSettings[] = "QMAKE_MAC_XCODE_SETTINGS+=qprofile qprofile.name=PROVISIONING_PROFILE_SPECIFIER qprofile.value=";
const char signingIdentifierKey[] = "Ios.SigningIdentifier";
const char autoManagedSigningKey[] = "Ios.AutoManagedSigning";

const int IdentifierRole = Qt::UserRole+1;


class IosBuildSettingsWidget : public NamedWidget
{
public:
    explicit IosBuildSettingsWidget(IosBuildConfiguration *iosBuildConfiguration);

    bool isSigningAutomaticallyManaged() const;

private:
    void announceSigningChanged(bool isAutoManaged, QString identifier);
    void onSigningEntityComboIndexChanged();
    void onReset();

    void setDefaultSigningIdentfier(const QString &identifier) const;
    void configureSigningUi(bool autoManageSigning);
    void populateDevelopmentTeams();
    void populateProvisioningProfiles();
    QString selectedIdentifier() const;
    void updateInfoText();
    void updateWarningText();

private:
    IosBuildConfiguration *m_bc = nullptr;
    QString m_lastProfileSelection;
    QString m_lastTeamSelection;
    const bool m_isDevice;

    QPushButton *m_qmakeDefaults;
    QComboBox *m_signEntityCombo;
    QCheckBox *m_autoSignCheckbox;
    QLabel *m_signEntityLabel;
    Utils::InfoLabel *m_infoLabel;
    Utils::InfoLabel *m_warningLabel;
};

IosBuildSettingsWidget::IosBuildSettingsWidget(IosBuildConfiguration *bc)
    : NamedWidget(IosBuildConfiguration::tr("iOS Settings")),
      m_bc(bc),
      m_isDevice(DeviceTypeKitAspect::deviceTypeId(bc->target()->kit())
                 == Constants::IOS_DEVICE_TYPE)
{
    auto detailsWidget = new Utils::DetailsWidget(this);
    auto container = new QWidget(detailsWidget);

    m_qmakeDefaults = new QPushButton(container);
    QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    m_qmakeDefaults->setSizePolicy(sizePolicy);
    m_qmakeDefaults->setText(IosBuildConfiguration::tr("Reset"));
    m_qmakeDefaults->setEnabled(m_isDevice);

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
    m_autoSignCheckbox->setText(IosBuildConfiguration::tr("Automatically manage signing"));
    m_autoSignCheckbox->setChecked(bc->m_autoManagedSigning->value());
    m_autoSignCheckbox->setEnabled(m_isDevice);

    m_signEntityLabel = new QLabel(container);

    m_infoLabel = new Utils::InfoLabel({}, Utils::InfoLabel::Information, container);

    m_warningLabel = new Utils::InfoLabel({}, Utils::InfoLabel::Warning, container);

    m_signEntityLabel->setText(IosBuildConfiguration::tr("Development team:"));

    connect(m_qmakeDefaults, &QPushButton::clicked, this, &IosBuildSettingsWidget::onReset);

    m_infoLabel->hide();

    m_warningLabel->hide();

    detailsWidget->setState(Utils::DetailsWidget::NoSummary);
    detailsWidget->setWidget(container);

    if (m_isDevice) {
        connect(IosConfigurations::instance(), &IosConfigurations::provisioningDataChanged,
                this, &IosBuildSettingsWidget::populateDevelopmentTeams);
        connect(m_signEntityCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &IosBuildSettingsWidget::onSigningEntityComboIndexChanged);
        connect(m_autoSignCheckbox, &QCheckBox::toggled,
                this, &IosBuildSettingsWidget::configureSigningUi);
        configureSigningUi(m_autoSignCheckbox->isChecked());
        setDefaultSigningIdentfier(bc->m_signingIdentifier->value());
    }

    m_signEntityCombo->setEnabled(m_isDevice);
    m_signEntityLabel->setEnabled(m_isDevice);
    adjustSize();

    auto rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->addWidget(detailsWidget);

    auto gridLayout = new QGridLayout();
    gridLayout->addWidget(m_signEntityLabel, 0, 0, 1, 1);
    gridLayout->addWidget(m_signEntityCombo, 0, 1, 1, 1);
    gridLayout->addWidget(m_autoSignCheckbox, 0, 2, 1, 1);
    gridLayout->addWidget(m_qmakeDefaults, 1, 1, 1, 1);

    auto verticalLayout = new QVBoxLayout(container);
    verticalLayout->addLayout(gridLayout);
    verticalLayout->addWidget(m_infoLabel);
    verticalLayout->addWidget(m_warningLabel);
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
    announceSigningChanged(m_autoSignCheckbox->isChecked(), identifier);
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
    m_signEntityLabel->setText(autoManageSigning ? IosBuildConfiguration::tr("Development team:")
                                                 : IosBuildConfiguration::tr("Provisioning profile:"));
    if (autoManageSigning)
        populateDevelopmentTeams();
    else
        populateProvisioningProfiles();

    updateInfoText();
    announceSigningChanged(autoManageSigning, selectedIdentifier());
}

void IosBuildSettingsWidget::announceSigningChanged(bool autoManagedSigning, QString identifier)
{
    if (m_bc->m_signingIdentifier->value().compare(identifier) != 0
            || m_bc->m_autoManagedSigning->value() != autoManagedSigning) {
        m_bc->m_autoManagedSigning->setValue(autoManagedSigning);
        m_bc->m_signingIdentifier->setValue(identifier);
        m_bc->updateQmakeCommand();
    }
}

void IosBuildSettingsWidget::populateDevelopmentTeams()
{
    {
        QSignalBlocker blocker(m_signEntityCombo);
        // Populate Team id's
        m_signEntityCombo->clear();
        m_signEntityCombo->addItem(IosBuildConfiguration::tr("Default"));
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
        if (!profiles.isEmpty()) {
            for (const auto &profile : profiles) {
                m_signEntityCombo->addItem(profile->displayName());
                const int index = m_signEntityCombo->count() - 1;
                m_signEntityCombo->setItemData(index, profile->identifier(), IdentifierRole);
                m_signEntityCombo->setItemData(index, profile->details(), Qt::ToolTipRole);
            }
        } else {
            m_signEntityCombo->addItem(IosBuildConfiguration::tr("None"));
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
    if (!m_isDevice)
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
            addMessage(IosBuildConfiguration::tr("Development team is not selected."));
        else
            addMessage(IosBuildConfiguration::tr("Provisioning profile is not selected."));

        addMessage(IosBuildConfiguration::tr("Using default development team and provisioning profile."));
    } else {
        if (!configuringTeams) {
            ProvisioningProfilePtr profile =  IosConfigurations::provisioningProfile(identifier);
            QTC_ASSERT(profile, return);
            auto team = profile->developmentTeam();
            if (team) {
                // Display corresponding team information.
                addMessage(IosBuildConfiguration::tr("Development team: %1 (%2)").arg(team->displayName())
                           .arg(team->identifier()));
                addMessage(IosBuildConfiguration::tr("Settings defined here override the QMake environment."));
            } else {
                qCDebug(iosSettingsLog) << "Development team not found for profile" << profile;
            }
        } else {
            addMessage(IosBuildConfiguration::tr("Settings defined here override the QMake environment."));
        }
    }

    m_infoLabel->setVisible(!infoMessage.isEmpty());
    m_infoLabel->setText(infoMessage);
}

void IosBuildSettingsWidget::updateWarningText()
{
    if (!m_isDevice)
        return;

    QString warningText;
    bool configuringTeams = m_autoSignCheckbox->isChecked();
    if (m_signEntityCombo->count() < 2) {
        warningText = IosBuildConfiguration::tr("%1 not configured. Use Xcode and Apple "
                                                "developer account to configure the "
                                                "provisioning profiles and teams.")
                .arg(configuringTeams ? IosBuildConfiguration::tr("Development teams")
                                      : IosBuildConfiguration::tr("Provisioning profiles"));
    } else {
        QString identifier = selectedIdentifier();
        if (configuringTeams) {
            auto team = IosConfigurations::developmentTeam(identifier);
            if (team && !team->hasProvisioningProfile())
                warningText = IosBuildConfiguration::tr("No provisioning profile found for the selected team.");
        } else {
            auto profile = IosConfigurations::provisioningProfile(identifier);
            if (profile && QDateTime::currentDateTimeUtc() > profile->expirationDate()) {
               warningText = IosBuildConfiguration::tr("Provisioning profile expired. Expiration date: %1")
                       .arg(QLocale::system().toString(profile->expirationDate().toLocalTime(),
                                                       QLocale::LongFormat));
            }
        }
    }

    m_warningLabel->setVisible(!warningText.isEmpty());
    m_warningLabel->setText(warningText);
}


// IosBuildConfiguration

IosBuildConfiguration::IosBuildConfiguration(Target *target, Utils::Id id)
    : QmakeBuildConfiguration(target, id)
{
    m_signingIdentifier = addAspect<BaseStringAspect>();
    m_signingIdentifier->setSettingsKey(signingIdentifierKey);

    m_autoManagedSigning = addAspect<BaseBoolAspect>();
    m_autoManagedSigning->setDefaultValue(true);
    m_autoManagedSigning->setSettingsKey(autoManagedSigningKey);
}

QList<NamedWidget *> IosBuildConfiguration::createSubConfigWidgets()
{
    auto subConfigWidgets = QmakeBuildConfiguration::createSubConfigWidgets();

    // Ownership of this widget is with BuildSettingsWidget
    auto buildSettingsWidget = new IosBuildSettingsWidget(this);
    subConfigWidgets.prepend(buildSettingsWidget);
    return subConfigWidgets;
}

bool IosBuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!QmakeBuildConfiguration::fromMap(map))
        return false;
    updateQmakeCommand();
    return true;
}

void IosBuildConfiguration::updateQmakeCommand()
{
    QMakeStep *qmakeStepInstance = qmakeStep();
    const QString forceOverrideArg("-after");
    if (qmakeStepInstance) {
        QStringList extraArgs = qmakeStepInstance->extraArguments();
        // remove old extra arguments.
        Utils::erase(extraArgs, [forceOverrideArg](const QString& arg) {
            return arg.startsWith(qmakeIosTeamSettings)
                    || arg.startsWith(qmakeProvisioningProfileSettings)
                    || arg == forceOverrideArg;
        });

        // Set force ovveride qmake switch
        const QString signingIdentifier =  m_signingIdentifier->value();
        if (signingIdentifier.isEmpty() )
            extraArgs << forceOverrideArg;

        Utils::Id devType = DeviceTypeKitAspect::deviceTypeId(target()->kit());
        if (devType == Constants::IOS_DEVICE_TYPE && !signingIdentifier.isEmpty()) {
            if (m_autoManagedSigning->value()) {
                extraArgs << qmakeIosTeamSettings + signingIdentifier;
            } else {
                // Get the team id from provisioning profile
                ProvisioningProfilePtr profile =
                        IosConfigurations::provisioningProfile(signingIdentifier);
                QString teamId;
                if (profile)
                    teamId = profile->developmentTeam()->identifier();
                else
                    qCDebug(iosLog) << "No provisioing profile found for id:" << signingIdentifier;

                if (!teamId.isEmpty()) {
                    extraArgs << qmakeProvisioningProfileSettings + signingIdentifier;
                    extraArgs << qmakeIosTeamSettings + teamId;
                } else {
                    qCDebug(iosLog) << "Development team unavailable for profile:" << profile;
                }
            }
        }

        qmakeStepInstance->setExtraArguments(extraArgs);
    }
}

IosBuildConfigurationFactory::IosBuildConfigurationFactory()
{
    registerBuildConfiguration<IosBuildConfiguration>(QmakeProjectManager::Constants::QMAKE_BC_ID);
    addSupportedTargetDeviceType(Constants::IOS_DEVICE_TYPE);
    addSupportedTargetDeviceType(Constants::IOS_SIMULATOR_TYPE);
}

} // namespace Internal
} // namespace Ios
