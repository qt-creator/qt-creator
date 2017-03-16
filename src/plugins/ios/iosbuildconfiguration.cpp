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
#include "iosmanager.h"

#include "projectexplorer/kitinformation.h"
#include "projectexplorer/namedwidget.h"
#include "projectexplorer/target.h"
#include "qmakeprojectmanager/qmakebuildinfo.h"
#include "utils/algorithm.h"

#include <memory>

using namespace QmakeProjectManager;

namespace Ios {
namespace Internal {

const char qmakeIosTeamSettings[] = "QMAKE_MAC_XCODE_SETTINGS+=qteam qteam.name=DEVELOPMENT_TEAM qteam.value=";
const char qmakeProvisioningProfileSettings[] = "QMAKE_MAC_XCODE_SETTINGS+=qprofile qprofile.name=PROVISIONING_PROFILE_SPECIFIER qprofile.value=";
const char signingIdentifierKey[] = "Ios.SigningIdentifier";
const char autoManagedSigningKey[] = "Ios.AutoManagedSigning";

IosBuildConfiguration::IosBuildConfiguration(ProjectExplorer::Target *target) :
    QmakeBuildConfiguration(target)
{
}

IosBuildConfiguration::IosBuildConfiguration(ProjectExplorer::Target *target, IosBuildConfiguration *source) :
    QmakeBuildConfiguration(target, source)
{
}

QList<ProjectExplorer::NamedWidget *> IosBuildConfiguration::createSubConfigWidgets()
{
    auto subConfigWidgets = QmakeBuildConfiguration::createSubConfigWidgets();

    Core::Id devType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(target()->kit());
    // Ownership of this widget is with BuildSettingsWidget
    auto buildSettingsWidget = new IosBuildSettingsWidget(devType, m_signingIdentifier,
                                                          m_autoManagedSigning);
    subConfigWidgets.prepend(buildSettingsWidget);
    connect(buildSettingsWidget, &IosBuildSettingsWidget::signingSettingsChanged,
            this, &IosBuildConfiguration::onSigningSettingsChanged);
    return subConfigWidgets;
}

QVariantMap IosBuildConfiguration::toMap() const
{
    QVariantMap map(QmakeBuildConfiguration::toMap());
    map.insert(signingIdentifierKey, m_signingIdentifier);
    map.insert(autoManagedSigningKey, m_autoManagedSigning);
    return map;
}

bool IosBuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!QmakeBuildConfiguration::fromMap(map))
        return false;
    m_autoManagedSigning = map.value(autoManagedSigningKey).toBool();
    m_signingIdentifier = map.value(signingIdentifierKey).toString();
    updateQmakeCommand();
    return true;
}

void IosBuildConfiguration::onSigningSettingsChanged(bool autoManagedSigning, QString identifier)
{
    if (m_signingIdentifier.compare(identifier) != 0
            || m_autoManagedSigning != autoManagedSigning) {
        m_autoManagedSigning = autoManagedSigning;
        m_signingIdentifier = identifier;
        updateQmakeCommand();
    }
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
        if (!m_signingIdentifier.isEmpty() )
            extraArgs << forceOverrideArg;

        Core::Id devType = ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(target()->kit());
        if (devType == Constants::IOS_DEVICE_TYPE && !m_signingIdentifier.isEmpty()) {
            if (m_autoManagedSigning) {
                extraArgs << qmakeIosTeamSettings + m_signingIdentifier;
            } else {
                // Get the team id from provisioning profile
                ProvisioningProfilePtr profile =
                        IosConfigurations::provisioningProfile(m_signingIdentifier);
                QString teamId;
                if (profile)
                    teamId = profile->developmentTeam()->identifier();
                else
                    qCDebug(iosLog) << "No provisioing profile found for id:"<< m_signingIdentifier;

                if (!teamId.isEmpty()) {
                    extraArgs << qmakeProvisioningProfileSettings + m_signingIdentifier;
                    extraArgs << qmakeIosTeamSettings + teamId;
                } else {
                    qCDebug(iosLog) << "Development team unavailable for profile:" << profile;
                }
            }
        }

        qmakeStepInstance->setExtraArguments(extraArgs);
    }
}

IosBuildConfigurationFactory::IosBuildConfigurationFactory(QObject *parent)
    : QmakeBuildConfigurationFactory(parent)
{
}


int IosBuildConfigurationFactory::priority(const ProjectExplorer::Kit *k, const QString &projectPath) const
{
    return (QmakeBuildConfigurationFactory::priority(k, projectPath) >= 0
            && IosManager::supportsIos(k)) ? 1 : -1;
}

int IosBuildConfigurationFactory::priority(const ProjectExplorer::Target *parent) const
{
    return (QmakeBuildConfigurationFactory::priority(parent) >= 0
            && IosManager::supportsIos(parent)) ? 1 : -1;
}

ProjectExplorer::BuildConfiguration *IosBuildConfigurationFactory::create(ProjectExplorer::Target *parent,
                                                                          const ProjectExplorer::BuildInfo *info) const
{
    auto qmakeInfo = static_cast<const QmakeBuildInfo *>(info);
    auto bc = new IosBuildConfiguration(parent);
    configureBuildConfiguration(parent, bc, qmakeInfo);
    return bc;
}

ProjectExplorer::BuildConfiguration *IosBuildConfigurationFactory::clone(ProjectExplorer::Target *parent,
                                                                         ProjectExplorer::BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return nullptr;
    auto *oldbc = static_cast<IosBuildConfiguration *>(source);
    return new IosBuildConfiguration(parent, oldbc);
}

ProjectExplorer::BuildConfiguration *IosBuildConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (canRestore(parent, map)) {
        std::unique_ptr<IosBuildConfiguration> bc(new IosBuildConfiguration(parent));
        if (bc->fromMap(map))
            return bc.release();
    }
    return nullptr;
}

} // namespace Internal
} // namespace Ios
