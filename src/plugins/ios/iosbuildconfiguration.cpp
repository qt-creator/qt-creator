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
#include "iosbuildsettingswidget.h"

#include "projectexplorer/kitinformation.h"
#include "projectexplorer/namedwidget.h"
#include "projectexplorer/target.h"

#include "qmakeprojectmanager/qmakebuildinfo.h"
#include "qmakeprojectmanager/qmakeprojectmanagerconstants.h"

#include <utils/algorithm.h>

using namespace QmakeProjectManager;
using namespace ProjectExplorer;

namespace Ios {
namespace Internal {

const char qmakeIosTeamSettings[] = "QMAKE_MAC_XCODE_SETTINGS+=qteam qteam.name=DEVELOPMENT_TEAM qteam.value=";
const char qmakeProvisioningProfileSettings[] = "QMAKE_MAC_XCODE_SETTINGS+=qprofile qprofile.name=PROVISIONING_PROFILE_SPECIFIER qprofile.value=";
const char signingIdentifierKey[] = "Ios.SigningIdentifier";
const char autoManagedSigningKey[] = "Ios.AutoManagedSigning";

IosBuildConfiguration::IosBuildConfiguration(Target *target, Core::Id id)
    : QmakeBuildConfiguration(target, id)
{
    m_signingIdentifier = addAspect<BaseStringAspect>();
    m_signingIdentifier->setSettingsKey(signingIdentifierKey);

    m_autoManagedSigning = addAspect<BaseBoolAspect>();
    m_autoManagedSigning->setDefaultValue(true);
    m_autoManagedSigning->setSettingsKey(autoManagedSigningKey);
}

QList<ProjectExplorer::NamedWidget *> IosBuildConfiguration::createSubConfigWidgets()
{
    auto subConfigWidgets = QmakeBuildConfiguration::createSubConfigWidgets();

    Core::Id devType = ProjectExplorer::DeviceTypeKitAspect::deviceTypeId(target()->kit());
    // Ownership of this widget is with BuildSettingsWidget
    auto buildSettingsWidget = new IosBuildSettingsWidget(devType,
                                                          m_signingIdentifier->value(),
                                                          m_autoManagedSigning->value());
    subConfigWidgets.prepend(buildSettingsWidget);
    connect(buildSettingsWidget, &IosBuildSettingsWidget::signingSettingsChanged,
            this, [this](bool autoManagedSigning, QString identifier) {
        if (m_signingIdentifier->value().compare(identifier) != 0
                || m_autoManagedSigning->value() != autoManagedSigning) {
            m_autoManagedSigning->setValue(autoManagedSigning);
            m_signingIdentifier->setValue(identifier);
            updateQmakeCommand();
        }
    });

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

        Core::Id devType = ProjectExplorer::DeviceTypeKitAspect::deviceTypeId(target()->kit());
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
