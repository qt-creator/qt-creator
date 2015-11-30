/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
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

#ifndef TARGET_H
#define TARGET_H

#include "projectconfiguration.h"
#include "projectexplorer_export.h"

QT_FORWARD_DECLARE_CLASS(QIcon)

namespace Utils { class Environment; }

namespace ProjectExplorer {
class BuildTargetInfoList;
class DeploymentData;
class RunConfiguration;
class BuildConfiguration;
class DeployConfiguration;
class IBuildConfigurationFactory;
class DeployConfigurationFactory;
class IRunConfigurationFactory;
class Kit;
class Project;
class NamedWidget;

class TargetPrivate;

class PROJECTEXPLORER_EXPORT Target : public ProjectConfiguration
{
    friend class SessionManager; // for setActiveBuild and setActiveDeployConfiguration
    Q_OBJECT

public:
    ~Target() override;

    Project *project() const;

    // Kit:
    Kit *kit() const;

    // Build configuration
    void addBuildConfiguration(BuildConfiguration *configuration);
    bool removeBuildConfiguration(BuildConfiguration *configuration);

    QList<BuildConfiguration *> buildConfigurations() const;
    BuildConfiguration *activeBuildConfiguration() const;

    // DeployConfiguration
    void addDeployConfiguration(DeployConfiguration *dc);
    bool removeDeployConfiguration(DeployConfiguration *dc);

    QList<DeployConfiguration *> deployConfigurations() const;
    DeployConfiguration *activeDeployConfiguration() const;

    void setDeploymentData(const DeploymentData &deploymentData);
    DeploymentData deploymentData() const;

    void setApplicationTargets(const BuildTargetInfoList &appTargets);
    BuildTargetInfoList applicationTargets() const;

    // Running
    QList<RunConfiguration *> runConfigurations() const;
    void addRunConfiguration(RunConfiguration *runConfiguration);
    void removeRunConfiguration(RunConfiguration *runConfiguration);

    RunConfiguration *activeRunConfiguration() const;
    void setActiveRunConfiguration(RunConfiguration *runConfiguration);

    // Returns whether this target is actually available at he time
    // of the call. A target may become unavailable e.g. when a Qt version
    // is removed.
    //
    // Note: Enabled state is not saved!
    bool isEnabled() const;

    QIcon icon() const;
    void setIcon(QIcon icon);
    QIcon overlayIcon() const;
    void setOverlayIcon(QIcon icon);
    QString toolTip() const;
    void setToolTip(const QString &text);

    QVariantMap toMap() const override;

    void updateDefaultBuildConfigurations();
    void updateDefaultDeployConfigurations();
    void updateDefaultRunConfigurations();

    QVariant namedSettings(const QString &name) const;
    void setNamedSettings(const QString &name, const QVariant &value);
signals:
    void targetEnabled(bool);
    void iconChanged();
    void overlayIconChanged();
    void toolTipChanged();

    void kitChanged();

    // TODO clean up signal names
    // might be better to also have aboutToRemove signals
    void removedRunConfiguration(ProjectExplorer::RunConfiguration *);
    void addedRunConfiguration(ProjectExplorer::RunConfiguration *);
    void activeRunConfigurationChanged(ProjectExplorer::RunConfiguration *);

    void removedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void addedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration *);

    void removedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc);
    void addedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc);
    void activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration *dc);

    /// convenience signal, emitted if either the active buildconfiguration emits
    /// environmentChanged() or if the active build configuration changes
    void environmentChanged();

    /// convenience signal, emitted if either the active configuration emits
    /// enabledChanged() or if the active build configuration changes
    void buildConfigurationEnabledChanged();
    void deployConfigurationEnabledChanged();
    void runConfigurationEnabledChanged();

    void deploymentDataChanged();
    void applicationTargetsChanged();

    // Remove all the signals below, they are stupid
    /// Emitted whenever the current build configuartion changed or the build directory of the current
    /// build configuration was changed.
    void buildDirectoryChanged();

private:
    Target(Project *parent, Kit *k);
    void setEnabled(bool);

    bool fromMap(const QVariantMap &map) override;

    void updateDeviceState();
    void onBuildDirectoryChanged();

    void changeEnvironment();
    void changeBuildConfigurationEnabled();
    void changeDeployConfigurationEnabled();
    void changeRunConfigurationEnabled();
    void handleKitUpdates(ProjectExplorer::Kit *k);
    void handleKitRemoval(ProjectExplorer::Kit *k);

    void setActiveBuildConfiguration(BuildConfiguration *configuration);
    void setActiveDeployConfiguration(DeployConfiguration *configuration);
    TargetPrivate *d;

    friend class Project;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::Target *)

#endif // TARGET_H
