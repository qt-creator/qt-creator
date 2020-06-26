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

#include "projectexplorer_export.h"

#include "buildsteplist.h"
#include "deploymentdata.h"
#include "projectconfiguration.h"

namespace ProjectExplorer {

class BuildStepList;
class Target;
class DeployConfigurationFactory;

class PROJECTEXPLORER_EXPORT DeployConfiguration final : public ProjectConfiguration
{
    Q_OBJECT

private:
    friend class DeployConfigurationFactory;
    explicit DeployConfiguration(Target *target, Utils::Id id);

public:
    ~DeployConfiguration() override = default;

    BuildStepList *stepList();
    const BuildStepList *stepList() const;

    QWidget *createConfigWidget();

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    bool isActive() const;

    bool usesCustomDeploymentData() const { return m_usesCustomDeploymentData; }
    void setUseCustomDeploymentData(bool enabled) { m_usesCustomDeploymentData = enabled; }

    DeploymentData customDeploymentData() const { return m_customDeploymentData; }
    void setCustomDeploymentData(const DeploymentData &data) { m_customDeploymentData = data; }

private:
    BuildStepList m_stepList;
    using WidgetCreator = std::function<QWidget *(DeployConfiguration *)>;
    WidgetCreator m_configWidgetCreator;
    DeploymentData m_customDeploymentData;
    bool m_usesCustomDeploymentData = false;
};

class PROJECTEXPLORER_EXPORT DeployConfigurationFactory
{
public:
    DeployConfigurationFactory();
    DeployConfigurationFactory(const DeployConfigurationFactory &) = delete;
    DeployConfigurationFactory operator=(const DeployConfigurationFactory &) = delete;
    virtual ~DeployConfigurationFactory();

    // return possible addition to a target, invalid if there is none
    Utils::Id creationId() const;
    // the name to display to the user
    QString defaultDisplayName() const;

    DeployConfiguration *create(Target *parent);

    static const QList<DeployConfigurationFactory *> find(Target *parent);
    static DeployConfiguration *restore(Target *parent, const QVariantMap &map);
    static DeployConfiguration *clone(Target *parent, const DeployConfiguration *dc);

    void addSupportedTargetDeviceType(Utils::Id id);
    void setDefaultDisplayName(const QString &defaultDisplayName);
    void setSupportedProjectType(Utils::Id id);

    // Step is only added if condition is not set, or returns true when called.
    void addInitialStep(Utils::Id stepId, const std::function<bool(Target *)> &condition = {});

    bool canHandle(ProjectExplorer::Target *target) const;

    void setConfigWidgetCreator(const DeployConfiguration::WidgetCreator &configWidgetCreator);
    void setUseDeploymentDataView();

    using PostRestore = std::function<void(DeployConfiguration *dc, const QVariantMap &)>;
    void setPostRestore(const PostRestore &postRestore) {  m_postRestore = postRestore; }
    PostRestore postRestore() const { return m_postRestore; }

protected:
    using DeployConfigurationCreator = std::function<DeployConfiguration *(Target *)>;
    void setConfigBaseId(Utils::Id deployConfigBaseId);

private:
    DeployConfiguration *createDeployConfiguration(Target *target);
    Utils::Id m_deployConfigBaseId;
    Utils::Id m_supportedProjectType;
    QList<Utils::Id> m_supportedTargetDeviceTypes;
    QList<BuildStepList::StepCreationInfo> m_initialSteps;
    QString m_defaultDisplayName;
    DeployConfiguration::WidgetCreator m_configWidgetCreator;
    PostRestore m_postRestore;
};

class DefaultDeployConfigurationFactory : public DeployConfigurationFactory
{
public:
    DefaultDeployConfigurationFactory();
};

} // namespace ProjectExplorer
