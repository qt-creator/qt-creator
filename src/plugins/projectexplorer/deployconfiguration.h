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
#include "projectconfiguration.h"

namespace ProjectExplorer {

class BuildStepList;
class Target;
class DeployConfigurationFactory;
class NamedWidget;

class PROJECTEXPLORER_EXPORT DeployConfiguration : public ProjectConfiguration
{
    Q_OBJECT

protected:
    friend class DeployConfigurationFactory;
    explicit DeployConfiguration(Target *target, Core::Id id);

public:
    ~DeployConfiguration() override = default;

    BuildStepList *stepList();
    const BuildStepList *stepList() const;

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    virtual NamedWidget *createConfigWidget();

    virtual bool isEnabled() const;
    virtual QString disabledReason() const;

    Target *target() const;
    Project *project() const override;

    bool isActive() const override;

signals:
    void enabledChanged();

private:
    BuildStepList m_stepList;
};

class PROJECTEXPLORER_EXPORT DeployConfigurationFactory
{
public:
    DeployConfigurationFactory();
    DeployConfigurationFactory(const DeployConfigurationFactory &) = delete;
    DeployConfigurationFactory operator=(const DeployConfigurationFactory &) = delete;
    virtual ~DeployConfigurationFactory();

    // return possible addition to a target, invalid if there is none
    Core::Id creationId() const;
    // the name to display to the user
    QString defaultDisplayName() const;

    bool canCreate(Target *parent, Core::Id id) const;
    virtual DeployConfiguration *create(Target *parent, Core::Id id);

    static const QList<DeployConfigurationFactory *> find(Target *parent);
    static DeployConfiguration *restore(Target *parent, const QVariantMap &map);
    static DeployConfiguration *clone(Target *parent, const DeployConfiguration *dc);

    void addSupportedTargetDeviceType(Core::Id id);
    void setDefaultDisplayName(const QString &defaultDisplayName);
    void setSupportedProjectType(Core::Id id);

    // Step is only added if condition is not set, or returns true when called.
    void addInitialStep(Core::Id stepId, const std::function<bool(Target *)> &condition = {});

    virtual bool canHandle(ProjectExplorer::Target *target) const;

protected:
    using DeployConfigurationCreator = std::function<DeployConfiguration *(Target *)>;

    template <class DeployConfig>
    void registerDeployConfiguration(Core::Id deployConfigBaseId)
    {
        m_creator = [this, deployConfigBaseId](Target *t) {
            auto dc = new DeployConfig(t, deployConfigBaseId);
            dc->setDefaultDisplayName(m_defaultDisplayName);
            return dc;
        };
        m_deployConfigBaseId = deployConfigBaseId;
    }

private:
    struct DeployStepCreationInfo {
        Core::Id deployStepId;
        std::function<bool(Target *)> condition; // unset counts as unrestricted
    };
    DeployConfigurationCreator m_creator;
    Core::Id m_deployConfigBaseId;
    Core::Id m_supportedProjectType;
    QList<Core::Id> m_supportedTargetDeviceTypes;
    QList<DeployStepCreationInfo> m_initialSteps;
    QString m_defaultDisplayName;
};

class DefaultDeployConfigurationFactory : public DeployConfigurationFactory
{
public:
    DefaultDeployConfigurationFactory();

private:
    bool canHandle(Target *parent) const override;
};

} // namespace ProjectExplorer
