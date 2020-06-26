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

#pragma once

#include "projectconfiguration.h"
#include "projectexplorer_export.h"

#include <memory>

QT_FORWARD_DECLARE_CLASS(QIcon)

namespace Utils { class MacroExpander; }

namespace ProjectExplorer {
class BuildConfiguration;
class BuildTargetInfo;
class BuildSystem;
class DeployConfiguration;
class DeploymentData;
class Kit;
class MakeInstallCommand;
class Project;
class ProjectConfigurationModel;
class RunConfiguration;

class TargetPrivate;

class PROJECTEXPLORER_EXPORT Target : public QObject
{
    friend class SessionManager; // for setActiveBuild and setActiveDeployConfiguration
    Q_OBJECT

    struct _constructor_tag { explicit _constructor_tag() = default; };

public:
    Target(Project *parent, Kit *k, _constructor_tag);
    ~Target() override;

    bool isActive() const;

    Project *project() const;
    Kit *kit() const;
    BuildSystem *buildSystem() const;

    Utils::Id id() const;
    QString displayName() const;
    QString toolTip() const;

    static QString displayNameKey();
    static QString deviceTypeKey();

    // Build configuration
    void addBuildConfiguration(BuildConfiguration *bc);
    bool removeBuildConfiguration(BuildConfiguration *bc);

    const QList<BuildConfiguration *> buildConfigurations() const;
    BuildConfiguration *activeBuildConfiguration() const;

    // DeployConfiguration
    void addDeployConfiguration(DeployConfiguration *dc);
    bool removeDeployConfiguration(DeployConfiguration *dc);

    const QList<DeployConfiguration *> deployConfigurations() const;
    DeployConfiguration *activeDeployConfiguration() const;

    // Running
    const QList<RunConfiguration *> runConfigurations() const;
    void addRunConfiguration(RunConfiguration *rc);
    void removeRunConfiguration(RunConfiguration *rc);

    RunConfiguration *activeRunConfiguration() const;
    void setActiveRunConfiguration(RunConfiguration *rc);

    QIcon icon() const;
    QIcon overlayIcon() const;
    void setOverlayIcon(const QIcon &icon);
    QString overlayIconToolTip();

    QVariantMap toMap() const;

    void updateDefaultBuildConfigurations();
    void updateDefaultDeployConfigurations();
    void updateDefaultRunConfigurations();

    QVariant namedSettings(const QString &name) const;
    void setNamedSettings(const QString &name, const QVariant &value);

    QVariant additionalData(Utils::Id id) const;
    MakeInstallCommand makeInstallCommand(const QString &installRoot) const;

    Utils::MacroExpander *macroExpander() const;

    ProjectConfigurationModel *buildConfigurationModel() const;
    ProjectConfigurationModel *deployConfigurationModel() const;
    ProjectConfigurationModel *runConfigurationModel() const;

    BuildSystem *fallbackBuildSystem() const;

    DeploymentData deploymentData() const;
    DeploymentData buildSystemDeploymentData() const;
    BuildTargetInfo buildTarget(const QString &buildKey) const;

    QString activeBuildKey() const; // Build key of active run configuaration

signals:
    void targetEnabled(bool);
    void iconChanged();
    void overlayIconChanged();

    void kitChanged();

    void parsingStarted();
    void parsingFinished(bool);
    void buildSystemUpdated(ProjectExplorer::BuildSystem *bs);

    // TODO clean up signal names
    // might be better to also have aboutToRemove signals
    void removedRunConfiguration(ProjectExplorer::RunConfiguration *rc);
    void addedRunConfiguration(ProjectExplorer::RunConfiguration *rc);
    void activeRunConfigurationChanged(ProjectExplorer::RunConfiguration *rc);

    void removedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void addedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration *);
    void buildEnvironmentChanged(ProjectExplorer::BuildConfiguration *bc);

    void removedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc);
    void addedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc);
    void activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration *dc);

    void deploymentDataChanged();

private:
    bool fromMap(const QVariantMap &map);

    void updateDeviceState();

    void changeDeployConfigurationEnabled();
    void changeRunConfigurationEnabled();
    void handleKitUpdates(ProjectExplorer::Kit *k);
    void handleKitRemoval(ProjectExplorer::Kit *k);

    void setActiveBuildConfiguration(BuildConfiguration *configuration);
    void setActiveDeployConfiguration(DeployConfiguration *configuration);
    const std::unique_ptr<TargetPrivate> d;

    friend class Project;
};

} // namespace ProjectExplorer
