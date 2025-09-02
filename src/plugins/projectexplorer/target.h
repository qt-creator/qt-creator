// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/id.h>
#include <utils/store.h>

#include <QObject>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QIcon)

namespace Utils { class MacroExpander; }

namespace ProjectExplorer {
class BuildConfiguration;
class BuildTargetInfo;
class BuildSystem;
class DeployConfiguration;
class Kit;
class Project;
class ProjectConfigurationModel;
class RunConfiguration;

enum class SetActive : int { Cascade, NoCascade };

class TargetPrivate;

class PROJECTEXPLORER_EXPORT Target : public QObject
{
    friend class ProjectManager; // for setActiveBuild and setActiveDeployConfiguration
    Q_OBJECT

public:
    ~Target() override;

    void markAsShuttingDown();
    bool isShuttingDown() const;

    Project *project() const;
    Kit *kit() const;
    BuildSystem *buildSystem() const;

    Utils::Id id() const;
    QString displayName() const;
    QString toolTip() const;

    static Utils::Key displayNameKey();
    static Utils::Key deviceTypeKey();

    // Build configuration
    void addBuildConfiguration(BuildConfiguration *bc);
    bool removeBuildConfiguration(BuildConfiguration *bc);

    const QList<BuildConfiguration *> buildConfigurations() const;
    BuildConfiguration *activeBuildConfiguration() const;
    DeployConfiguration *activeDeployConfiguration() const;
    RunConfiguration *activeRunConfiguration() const;

    QIcon icon() const;
    QIcon overlayIcon() const;
    void setOverlayIcon(const QIcon &icon);
    QString overlayIconToolTip();

    Utils::Store toMap() const;

    void updateDefaultBuildConfigurations();
    void updateDefaultRunConfigurations();

    ProjectConfigurationModel *buildConfigurationModel() const;

    void setActiveBuildConfiguration(BuildConfiguration *bc, SetActive cascade);

signals:
    void iconChanged();
    void kitChanged();

    void removedRunConfiguration(ProjectExplorer::RunConfiguration *rc);
    void addedRunConfiguration(ProjectExplorer::RunConfiguration *rc);
    void activeRunConfigurationChanged(ProjectExplorer::RunConfiguration *rc);

    void removedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void addedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration *);

    void removedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc);
    void addedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc);
    void activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration *dc);

private:
    static std::unique_ptr<Target> create(Project *parent, Kit *k);
    Target(Project *parent, Kit *k);

    bool fromMap(const Utils::Store &map);
    bool addConfigurationsFromMap(const Utils::Store &map, bool setActiveConfigurations);

    void updateDeviceState();

    void handleKitUpdates(ProjectExplorer::Kit *k);
    void handleKitRemoval(ProjectExplorer::Kit *k);

    void setActiveBuildConfiguration(BuildConfiguration *configuration);
    const std::unique_ptr<TargetPrivate> d;

    friend class Project;
};

} // namespace ProjectExplorer
