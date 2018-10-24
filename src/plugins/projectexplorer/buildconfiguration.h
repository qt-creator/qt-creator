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
#include "projectconfiguration.h"

#include <utils/environment.h>
#include <utils/fileutils.h>

namespace ProjectExplorer {

class BuildInfo;
class NamedWidget;
class BuildStepList;
class Node;
class Kit;
class Target;
class IOutputParser;

class PROJECTEXPLORER_EXPORT BuildConfiguration : public ProjectConfiguration
{
    Q_OBJECT

protected:
    friend class IBuildConfigurationFactory;
    explicit BuildConfiguration(Target *target, Core::Id id);

public:
    Utils::FileName buildDirectory() const;
    Utils::FileName rawBuildDirectory() const;
    void setBuildDirectory(const Utils::FileName &dir);

    virtual NamedWidget *createConfigWidget() = 0;
    virtual QList<NamedWidget *> createSubConfigWidgets();

    // Maybe the BuildConfiguration is not the best place for the environment
    Utils::Environment baseEnvironment() const;
    QString baseEnvironmentText() const;
    Utils::Environment environment() const;
    void setUserEnvironmentChanges(const QList<Utils::EnvironmentItem> &diff);
    QList<Utils::EnvironmentItem> userEnvironmentChanges() const;
    bool useSystemEnvironment() const;
    void setUseSystemEnvironment(bool b);

    virtual void addToEnvironment(Utils::Environment &env) const;

    QList<Core::Id> knownStepLists() const;
    BuildStepList *stepList(Core::Id id) const;

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    Target *target() const;
    Project *project() const override;

    virtual bool isEnabled() const;
    virtual QString disabledReason() const;

    virtual bool regenerateBuildFiles(Node *node);

    enum BuildType {
        Unknown,
        Debug,
        Profile,
        Release
    };
    virtual BuildType buildType() const = 0;

    static QString buildTypeName(BuildType type);

    bool isActive() const override;

    void prependCompilerPathToEnvironment(Utils::Environment &env) const;
    static void prependCompilerPathToEnvironment(Kit *k, Utils::Environment &env);
    void updateCacheAndEmitEnvironmentChanged();

signals:
    void environmentChanged();
    void buildDirectoryChanged();
    void enabledChanged();
    void buildTypeChanged();

protected:
    virtual void initialize(const BuildInfo *info);

private:
    void emitBuildDirectoryChanged();

    bool m_clearSystemEnvironment = false;
    QList<Utils::EnvironmentItem> m_userEnvironmentChanges;
    QList<BuildStepList *> m_stepLists;
    Utils::FileName m_buildDirectory;
    Utils::FileName m_lastEmmitedBuildDirectory;
    mutable Utils::Environment m_cachedEnvironment;
};

class PROJECTEXPLORER_EXPORT IBuildConfigurationFactory : public QObject
{
    Q_OBJECT

protected:
    IBuildConfigurationFactory();
    ~IBuildConfigurationFactory() override;

public:
    // The priority is negative if this factory cannot create anything for the target.
    // It is 0 for the "default" factory that wants to handle the target.
    // Add 100 for each specialization.
    virtual int priority(const Target *parent) const;
    // List of build information that can be used to create a new build configuration via
    // "Add Build Configuration" button.
    virtual QList<BuildInfo *> availableBuilds(const Target *parent) const = 0;

    virtual int priority(const Kit *k, const QString &projectPath) const;
    // List of build information that can be used to initially set up a new build configuration.
    virtual QList<BuildInfo *> availableSetups(const Kit *k, const QString &projectPath) const = 0;

    BuildConfiguration *create(Target *parent, const BuildInfo *info) const;

    static BuildConfiguration *restore(Target *parent, const QVariantMap &map);
    static BuildConfiguration *clone(Target *parent, const BuildConfiguration *source);

    static IBuildConfigurationFactory *find(const Kit *k, const QString &projectPath);
    static IBuildConfigurationFactory *find(Target *parent);

protected:
    bool supportsTargetDeviceType(Core::Id id) const;
    void setSupportedProjectType(Core::Id id);
    void setSupportedProjectMimeTypeName(const QString &mimeTypeName);
    void setSupportedTargetDeviceTypes(const QList<Core::Id> &ids);
    void setDefaultDisplayName(const QString &defaultDisplayName);
    void setBasePriority(int basePriority);

    using BuildConfigurationCreator = std::function<BuildConfiguration *(Target *)>;

    template <class BuildConfig>
    void registerBuildConfiguration(Core::Id buildConfigId)
    {
        setObjectName(buildConfigId.toString() + "BuildConfigurationFactory");
        m_creator = [buildConfigId](Target *t) { return new BuildConfig(t, buildConfigId); };
        m_buildConfigId = buildConfigId;
    }

private:
    bool canHandle(const ProjectExplorer::Target *t) const;

    BuildConfigurationCreator m_creator;
    Core::Id m_buildConfigId;
    Core::Id m_supportedProjectType;
    QList<Core::Id> m_supportedTargetDeviceTypes;
    QString m_supportedProjectMimeTypeName;
    int m_basePriority = 0; // Use higher numbers (1, 2, ...) for higher priorities.
};

} // namespace ProjectExplorer
