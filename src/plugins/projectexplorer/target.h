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

#include "subscription.h"

QT_FORWARD_DECLARE_CLASS(QIcon)

namespace Utils { class Environment; }

namespace ProjectExplorer {
class BuildConfiguration;
class BuildTargetInfoList;
class DeployConfiguration;
class DeployConfigurationFactory;
class DeploymentData;
class IBuildConfigurationFactory;
class IRunConfigurationFactory;
class Kit;
class NamedWidget;
class Project;
class RunConfiguration;

class TargetPrivate;

class PROJECTEXPLORER_EXPORT Target : public ProjectConfiguration
{
    friend class SessionManager; // for setActiveBuild and setActiveDeployConfiguration
    Q_OBJECT

public:
    ~Target() override;

    Project *project() const override;

    bool isActive() const final;

    // Kit:
    Kit *kit() const;

    // Build configuration
    void addBuildConfiguration(BuildConfiguration *bc);
    bool removeBuildConfiguration(BuildConfiguration *bc);

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

    QList<ProjectConfiguration *> projectConfigurations() const;

    // Running
    QList<RunConfiguration *> runConfigurations() const;
    void addRunConfiguration(RunConfiguration *rc);
    void removeRunConfiguration(RunConfiguration *rc);

    RunConfiguration *activeRunConfiguration() const;
    void setActiveRunConfiguration(RunConfiguration *rc);

    // Returns whether this target is actually available at he time
    // of the call. A target may become unavailable e.g. when a Qt version
    // is removed.
    //
    // Note: Enabled state is not saved!
    bool isEnabled() const;

    QIcon icon() const;
    QIcon overlayIcon() const;
    void setOverlayIcon(const QIcon &icon);
    QString overlayIconToolTip();

    QVariantMap toMap() const override;

    void updateDefaultBuildConfigurations();
    void updateDefaultDeployConfigurations();
    void updateDefaultRunConfigurations();

    QVariant namedSettings(const QString &name) const;
    void setNamedSettings(const QString &name, const QVariant &value);

    template<typename S, typename R, typename T>
    void subscribeSignal(void (S::*sig)(), R*recv, T (R::*sl)()) {
        new Internal::TargetSubscription([sig, recv, sl, this](ProjectConfiguration *pc) {
            if (S* sender = qobject_cast<S*>(pc))
                return connect(sender, sig, recv, sl);
            return QMetaObject::Connection();
        }, recv, this);
    }

    template<typename S, typename R, typename T>
    void subscribeSignal(void (S::*sig)(), R*recv, T sl) {
        new Internal::TargetSubscription([sig, recv, sl, this](ProjectConfiguration *pc) {
            if (S* sender = qobject_cast<S*>(pc))
                return connect(sender, sig, recv, sl);
            return QMetaObject::Connection();
        }, recv, this);
    }

signals:
    void targetEnabled(bool);
    void iconChanged();
    void overlayIconChanged();

    void kitChanged();

    void aboutToRemoveProjectConfiguration(ProjectExplorer::ProjectConfiguration *pc);
    void removedProjectConfiguration(ProjectExplorer::ProjectConfiguration *pc);
    void addedProjectConfiguration(ProjectExplorer::ProjectConfiguration *pc);

    void activeProjectConfigurationChanged(ProjectExplorer::ProjectConfiguration *pc);

    // TODO clean up signal names
    // might be better to also have aboutToRemove signals
    void removedRunConfiguration(ProjectExplorer::RunConfiguration *rc);
    void addedRunConfiguration(ProjectExplorer::RunConfiguration *rc);
    void activeRunConfigurationChanged(ProjectExplorer::RunConfiguration *rc);

    void removedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void addedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration *);

    void removedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc);
    void addedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc);
    void activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration *dc);

    void deploymentDataChanged();
    void applicationTargetsChanged();

private:
    Target(Project *parent, Kit *k);
    void setEnabled(bool);

    bool fromMap(const QVariantMap &map) override;

    void updateDeviceState();

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
