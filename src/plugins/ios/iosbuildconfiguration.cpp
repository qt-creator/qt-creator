// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iosbuildconfiguration.h"

#include "iosconfigurations.h"
#include "iosconstants.h"
#include "iostr.h"

#include <projectexplorer/kitaspects.h>
#include <projectexplorer/namedwidget.h>
#include <projectexplorer/target.h>

#include <cmakeprojectmanager/cmakeprojectconstants.h>

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
using namespace CMakeProjectManager;
using namespace ProjectExplorer;
using namespace Utils;

namespace Ios {
namespace Internal {

static Q_LOGGING_CATEGORY(iosSettingsLog, "qtc.ios.common", QtWarningMsg)

const char qmakeIosTeamSettings[] = "QMAKE_MAC_XCODE_SETTINGS+=qteam qteam.name=DEVELOPMENT_TEAM qteam.value=";
const char qmakeProvisioningProfileSettings[] = "QMAKE_MAC_XCODE_SETTINGS+=qprofile qprofile.name=PROVISIONING_PROFILE_SPECIFIER qprofile.value=";
const char signingIdentifierKey[] = "Ios.SigningIdentifier";
const char autoManagedSigningKey[] = "Ios.AutoManagedSigning";

const int IdentifierRole = Qt::UserRole+1;

class IosSigningSettingsWidget : public NamedWidget
{
public:
    explicit IosSigningSettingsWidget(BuildConfiguration *buildConfiguration,
                                      BoolAspect *autoManagedSigning,
                                      StringAspect *signingIdentifier);

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
    BoolAspect *m_autoManagedSigning = nullptr;
    StringAspect *m_signingIdentifier = nullptr;
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

IosSigningSettingsWidget::IosSigningSettingsWidget(BuildConfiguration *buildConfiguration,
                                                   BoolAspect *autoManagedSigning,
                                                   StringAspect *signingIdentifier)
    : NamedWidget(Tr::tr("iOS Settings"))
    , m_autoManagedSigning(autoManagedSigning)
    , m_signingIdentifier(signingIdentifier)
    , m_isDevice(DeviceTypeKitAspect::deviceTypeId(buildConfiguration->kit())
                 == Constants::IOS_DEVICE_TYPE)
{
    auto detailsWidget = new Utils::DetailsWidget(this);
    auto container = new QWidget(detailsWidget);

    m_qmakeDefaults = new QPushButton(container);
    QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    m_qmakeDefaults->setSizePolicy(sizePolicy);
    m_qmakeDefaults->setText(Tr::tr("Reset"));
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
    m_autoSignCheckbox->setText(Tr::tr("Automatically manage signing"));
    m_autoSignCheckbox->setChecked(m_autoManagedSigning->value());
    m_autoSignCheckbox->setEnabled(m_isDevice);

    m_signEntityLabel = new QLabel(container);

    m_infoLabel = new Utils::InfoLabel({}, Utils::InfoLabel::Information, container);

    m_warningLabel = new Utils::InfoLabel({}, Utils::InfoLabel::Warning, container);

    m_signEntityLabel->setText(Tr::tr("Development team:"));

    connect(m_qmakeDefaults, &QPushButton::clicked, this, &IosSigningSettingsWidget::onReset);

    m_infoLabel->hide();

    m_warningLabel->hide();

    detailsWidget->setState(Utils::DetailsWidget::NoSummary);
    detailsWidget->setWidget(container);

    if (m_isDevice) {
        connect(IosConfigurations::instance(), &IosConfigurations::provisioningDataChanged,
                this, &IosSigningSettingsWidget::populateDevelopmentTeams);
        connect(m_signEntityCombo, &QComboBox::currentIndexChanged,
                this, &IosSigningSettingsWidget::onSigningEntityComboIndexChanged);
        connect(m_autoSignCheckbox, &QCheckBox::toggled,
                this, &IosSigningSettingsWidget::configureSigningUi);
        const QString signingIdentifier = m_signingIdentifier->value();
        configureSigningUi(m_autoSignCheckbox->isChecked());
        setDefaultSigningIdentfier(signingIdentifier);
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

void IosSigningSettingsWidget::setDefaultSigningIdentfier(const QString &identifier) const
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

bool IosSigningSettingsWidget::isSigningAutomaticallyManaged() const
{
    return m_autoSignCheckbox->isChecked() && m_signEntityCombo->currentIndex() > 0;
}

void IosSigningSettingsWidget::onSigningEntityComboIndexChanged()
{
    QString identifier = selectedIdentifier();
    (m_autoSignCheckbox->isChecked() ? m_lastTeamSelection : m_lastProfileSelection) = identifier;

    updateInfoText();
    updateWarningText();
    announceSigningChanged(m_autoSignCheckbox->isChecked(), identifier);
}

void IosSigningSettingsWidget::onReset()
{
    m_lastTeamSelection.clear();
    m_lastProfileSelection.clear();
    m_autoSignCheckbox->setChecked(true);
    setDefaultSigningIdentfier("");
}

void IosSigningSettingsWidget::configureSigningUi(bool autoManageSigning)
{
    m_signEntityLabel->setText(autoManageSigning
                                   ? Tr::tr("Development team:")
                                   : Tr::tr("Provisioning profile:"));
    if (autoManageSigning)
        populateDevelopmentTeams();
    else
        populateProvisioningProfiles();

    updateInfoText();
    announceSigningChanged(autoManageSigning, selectedIdentifier());
}

void IosSigningSettingsWidget::announceSigningChanged(bool autoManagedSigning, QString identifier)
{
    if (m_signingIdentifier->value().compare(identifier) != 0
        || m_autoManagedSigning->value() != autoManagedSigning) {
        m_autoManagedSigning->setValue(autoManagedSigning);
        m_signingIdentifier->setValue(identifier);
    }
}

void IosSigningSettingsWidget::populateDevelopmentTeams()
{
    {
        QSignalBlocker blocker(m_signEntityCombo);
        // Populate Team id's
        m_signEntityCombo->clear();
        m_signEntityCombo->addItem(Tr::tr("Default"));
        const auto teams = IosConfigurations::developmentTeams();
        for (auto team : teams) {
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

void IosSigningSettingsWidget::populateProvisioningProfiles()
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
            m_signEntityCombo->addItem(Tr::tr("None"));
        }
    }
    // Maintain previous selection.
    setDefaultSigningIdentfier(m_lastProfileSelection);
    updateWarningText();
}

QString IosSigningSettingsWidget::selectedIdentifier() const
{
    return m_signEntityCombo->currentData(IdentifierRole).toString();
}

void IosSigningSettingsWidget::updateInfoText()
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
            addMessage(Tr::tr("Development team is not selected."));
        else
            addMessage(Tr::tr("Provisioning profile is not selected."));

        addMessage(Tr::tr("Using default development team and provisioning profile."));
    } else {
        if (!configuringTeams) {
            ProvisioningProfilePtr profile =  IosConfigurations::provisioningProfile(identifier);
            QTC_ASSERT(profile, return);
            auto team = profile->developmentTeam();
            if (team) {
                // Display corresponding team information.
                addMessage(Tr::tr("Development team: %1 (%2)")
                               .arg(team->displayName())
                               .arg(team->identifier()));
                addMessage(Tr::tr("Settings defined here override the QMake environment."));
            } else {
                qCDebug(iosSettingsLog) << "Development team not found for profile" << profile;
            }
        } else {
            addMessage(Tr::tr("Settings defined here override the QMake environment."));
        }
    }

    m_infoLabel->setVisible(!infoMessage.isEmpty());
    m_infoLabel->setText(infoMessage);
}

void IosSigningSettingsWidget::updateWarningText()
{
    if (!m_isDevice)
        return;

    QString warningText;
    bool configuringTeams = m_autoSignCheckbox->isChecked();
    if (m_signEntityCombo->count() < 2) {
        warningText = Tr::tr("%1 not configured. Use Xcode and Apple "
                             "developer account to configure the "
                             "provisioning profiles and teams.")
                          .arg(configuringTeams
                                   ? Tr::tr("Development teams")
                                   : Tr::tr("Provisioning profiles"));
    } else {
        QString identifier = selectedIdentifier();
        if (configuringTeams) {
            auto team = IosConfigurations::developmentTeam(identifier);
            if (team && !team->hasProvisioningProfile())
                warningText = Tr::tr("No provisioning profile found for the selected team.");
        } else {
            auto profile = IosConfigurations::provisioningProfile(identifier);
            if (profile && QDateTime::currentDateTimeUtc() > profile->expirationDate()) {
                warningText
                    = Tr::tr(
                          "Provisioning profile expired. Expiration date: %1")
                          .arg(QLocale::system().toString(profile->expirationDate().toLocalTime(),
                                                          QLocale::LongFormat));
            }
        }
    }

    m_warningLabel->setVisible(!warningText.isEmpty());
    m_warningLabel->setText(warningText);
}

// IosQmakeBuildConfiguration

class IosQmakeBuildConfiguration : public QmakeProjectManager::QmakeBuildConfiguration
{
public:
    IosQmakeBuildConfiguration(Target *target, Id id);

private:
    QList<NamedWidget *> createSubConfigWidgets() override;
    void fromMap(const Store &map) override;

    void updateQmakeCommand();

    StringAspect m_signingIdentifier{this};
    BoolAspect m_autoManagedSigning{this};
};

IosQmakeBuildConfiguration::IosQmakeBuildConfiguration(Target *target, Id id)
    : QmakeBuildConfiguration(target, id)
{
    m_signingIdentifier.setSettingsKey(signingIdentifierKey);

    m_autoManagedSigning.setDefaultValue(true);
    m_autoManagedSigning.setSettingsKey(autoManagedSigningKey);

    connect(&m_signingIdentifier,
            &BaseAspect::changed,
            this,
            &IosQmakeBuildConfiguration::updateQmakeCommand);
    connect(&m_autoManagedSigning,
            &BaseAspect::changed,
            this,
            &IosQmakeBuildConfiguration::updateQmakeCommand);
}

QList<NamedWidget *> IosQmakeBuildConfiguration::createSubConfigWidgets()
{
    auto subConfigWidgets = QmakeBuildConfiguration::createSubConfigWidgets();

    // Ownership of this widget is with BuildSettingsWidget
    auto buildSettingsWidget = new IosSigningSettingsWidget(this,
                                                            &m_autoManagedSigning,
                                                            &m_signingIdentifier);
    subConfigWidgets.prepend(buildSettingsWidget);
    return subConfigWidgets;
}

void IosQmakeBuildConfiguration::fromMap(const Store &map)
{
    QmakeBuildConfiguration::fromMap(map);
    if (!hasError())
        updateQmakeCommand();
}

static QString teamIdForProvisioningProfile(const QString &id)
{
    // Get the team id from provisioning profile
    ProvisioningProfilePtr profile = IosConfigurations::provisioningProfile(id);
    QString teamId;
    if (profile)
        teamId = profile->developmentTeam()->identifier();
    else
        qCDebug(iosLog) << "No provisioing profile found for id:" << id;

    if (teamId.isEmpty())
        qCDebug(iosLog) << "Development team unavailable for profile:" << profile;
    return teamId;
}

void IosQmakeBuildConfiguration::updateQmakeCommand()
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
        const QString signingIdentifier =  m_signingIdentifier();
        if (signingIdentifier.isEmpty() )
            extraArgs << forceOverrideArg;

        Utils::Id devType = DeviceTypeKitAspect::deviceTypeId(kit());
        if (devType == Constants::IOS_DEVICE_TYPE && !signingIdentifier.isEmpty()) {
            if (m_autoManagedSigning()) {
                extraArgs << qmakeIosTeamSettings + signingIdentifier;
            } else {
                const QString teamId = teamIdForProvisioningProfile(signingIdentifier);
                if (!teamId.isEmpty()) {
                    extraArgs << qmakeProvisioningProfileSettings + signingIdentifier;
                    extraArgs << qmakeIosTeamSettings + teamId;
                }
            }
        }

        qmakeStepInstance->setExtraArguments(extraArgs);
    }
}

IosQmakeBuildConfigurationFactory::IosQmakeBuildConfigurationFactory()
{
    registerBuildConfiguration<IosQmakeBuildConfiguration>(
        QmakeProjectManager::Constants::QMAKE_BC_ID);
    addSupportedTargetDeviceType(Constants::IOS_DEVICE_TYPE);
    addSupportedTargetDeviceType(Constants::IOS_SIMULATOR_TYPE);
}

// IosCMakeBuildConfiguration

class IosCMakeBuildConfiguration : public CMakeProjectManager::CMakeBuildConfiguration
{
public:
    IosCMakeBuildConfiguration(Target *target, Id id);

private:
    QList<NamedWidget *> createSubConfigWidgets() override;

    CMakeProjectManager::CMakeConfig signingFlags() const final;

    StringAspect m_signingIdentifier{this};
    BoolAspect m_autoManagedSigning{this};
};

IosCMakeBuildConfiguration::IosCMakeBuildConfiguration(Target *target, Id id)
    : CMakeBuildConfiguration(target, id)
{
    m_signingIdentifier.setSettingsKey(signingIdentifierKey);

    m_autoManagedSigning.setDefaultValue(true);
    m_autoManagedSigning.setSettingsKey(autoManagedSigningKey);

    connect(&m_signingIdentifier,
            &BaseAspect::changed,
            this,
            &IosCMakeBuildConfiguration::signingFlagsChanged);
    connect(&m_autoManagedSigning,
            &BaseAspect::changed,
            this,
            &IosCMakeBuildConfiguration::signingFlagsChanged);
}

QList<NamedWidget *> IosCMakeBuildConfiguration::createSubConfigWidgets()
{
    auto subConfigWidgets = CMakeBuildConfiguration::createSubConfigWidgets();

    // Ownership of this widget is with BuildSettingsWidget
    auto buildSettingsWidget = new IosSigningSettingsWidget(this,
                                                            &m_autoManagedSigning,
                                                            &m_signingIdentifier);
    subConfigWidgets.prepend(buildSettingsWidget);
    return subConfigWidgets;
}

CMakeConfig IosCMakeBuildConfiguration::signingFlags() const
{
    if (DeviceTypeKitAspect::deviceTypeId(kit()) != Constants::IOS_DEVICE_TYPE)
        return {};
    const QString signingIdentifier = m_signingIdentifier();
    if (m_autoManagedSigning()) {
        const DevelopmentTeams teams = IosConfigurations::developmentTeams();
        const QString teamId = signingIdentifier.isEmpty() && !teams.isEmpty()
                                   ? teams.first()->identifier()
                                   : signingIdentifier;
        CMakeConfigItem provisioningConfig("CMAKE_XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER",
                                           "");
        provisioningConfig.isUnset = true;
        return {{"CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM", teamId.toUtf8()}, provisioningConfig};
    }
    const QString teamId = teamIdForProvisioningProfile(signingIdentifier);
    if (!teamId.isEmpty())
        return {{"CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM", teamId.toUtf8()},
                {"CMAKE_XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER",
                 signingIdentifier.toUtf8()}};
    return {};
}

IosCMakeBuildConfigurationFactory::IosCMakeBuildConfigurationFactory()
{
    registerBuildConfiguration<IosCMakeBuildConfiguration>(
        CMakeProjectManager::Constants::CMAKE_BUILDCONFIGURATION_ID);
    addSupportedTargetDeviceType(Constants::IOS_DEVICE_TYPE);
    addSupportedTargetDeviceType(Constants::IOS_SIMULATOR_TYPE);
}

} // namespace Internal
} // namespace Ios
