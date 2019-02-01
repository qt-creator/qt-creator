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
class Task;
class IOutputParser;

class PROJECTEXPLORER_EXPORT BuildConfiguration : public ProjectConfiguration
{
    Q_OBJECT

protected:
    friend class BuildConfigurationFactory;
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
    virtual void initialize(const BuildInfo &info);

private:
    void emitBuildDirectoryChanged();

    bool m_clearSystemEnvironment = false;
    QList<Utils::EnvironmentItem> m_userEnvironmentChanges;
    QList<BuildStepList *> m_stepLists;
    Utils::FileName m_buildDirectory;
    Utils::FileName m_lastEmmitedBuildDirectory;
    mutable Utils::Environment m_cachedEnvironment;
};

class PROJECTEXPLORER_EXPORT BuildConfigurationFactory : public QObject
{
    Q_OBJECT

protected:
    BuildConfigurationFactory();
    ~BuildConfigurationFactory() override;

public:
    // List of build information that can be used to create a new build configuration via
    // "Add Build Configuration" button.
    const QList<BuildInfo> allAvailableBuilds(const Target *parent) const;

    // List of build information that can be used to initially set up a new build configuration.
    const QList<BuildInfo> allAvailableSetups(const Kit *k, const QString &projectPath) const;

    BuildConfiguration *create(Target *parent, const BuildInfo &info) const;

    static BuildConfiguration *restore(Target *parent, const QVariantMap &map);
    static BuildConfiguration *clone(Target *parent, const BuildConfiguration *source);

    static BuildConfigurationFactory *find(const Kit *k, const QString &projectPath);
    static BuildConfigurationFactory *find(Target *parent);

    using IssueReporter = std::function<QList<ProjectExplorer::Task>(Kit *, const QString &, const QString &)>;
    void setIssueReporter(const IssueReporter &issueReporter);
    const QList<Task> reportIssues(ProjectExplorer::Kit *kit,
                                   const QString &projectPath, const QString &buildDir) const;

protected:
    virtual QList<BuildInfo> availableBuilds(const Target *parent) const = 0;
    virtual QList<BuildInfo> availableSetups(const Kit *k, const QString &projectPath) const = 0;

    bool supportsTargetDeviceType(Core::Id id) const;
    void setSupportedProjectType(Core::Id id);
    void setSupportedProjectMimeTypeName(const QString &mimeTypeName);
    void addSupportedTargetDeviceType(Core::Id id);
    void setDefaultDisplayName(const QString &defaultDisplayName);

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
    IssueReporter m_issueReporter;
};

} // namespace ProjectExplorer
