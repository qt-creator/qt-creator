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
#include "task.h"

#include <utils/environment.h>
#include <utils/fileutils.h>

namespace ProjectExplorer {

class BaseStringAspect;
class BuildInfo;
class BuildStepList;
class Kit;
class NamedWidget;
class Node;
class Target;

class PROJECTEXPLORER_EXPORT BuildConfiguration : public ProjectConfiguration
{
    Q_OBJECT

protected:
    friend class BuildConfigurationFactory;
    explicit BuildConfiguration(Target *target, Core::Id id);

public:
    Utils::FilePath buildDirectory() const;
    Utils::FilePath rawBuildDirectory() const;
    void setBuildDirectory(const Utils::FilePath &dir);

    virtual NamedWidget *createConfigWidget();
    virtual QList<NamedWidget *> createSubConfigWidgets();

    // Maybe the BuildConfiguration is not the best place for the environment
    Utils::Environment baseEnvironment() const;
    QString baseEnvironmentText() const;
    Utils::Environment environment() const;
    void setUserEnvironmentChanges(const Utils::EnvironmentItems &diff);
    Utils::EnvironmentItems userEnvironmentChanges() const;
    bool useSystemEnvironment() const;
    void setUseSystemEnvironment(bool b);

    virtual void addToEnvironment(Utils::Environment &env) const;

    QList<Core::Id> knownStepLists() const;
    BuildStepList *stepList(Core::Id id) const;

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    virtual bool isEnabled() const;
    virtual QString disabledReason() const;

    virtual bool regenerateBuildFiles(Node *node);

    enum BuildType {
        Unknown,
        Debug,
        Profile,
        Release
    };
    virtual BuildType buildType() const { return m_initialBuildType; }

    BuildType initialBuildType() const { return m_initialBuildType; } // FIXME: Remove.
    Utils::FilePath initialBuildDirectory() const { return m_initialBuildDirectory; } // FIXME: Remove.
    QString initialDisplayName() const; // FIXME: Remove.
    QVariant extraInfo() const { return m_extraInfo; } // FIXME: Remove.

    static QString buildTypeName(BuildType type);

    bool isActive() const override;

    static void prependCompilerPathToEnvironment(Kit *k, Utils::Environment &env);
    void updateCacheAndEmitEnvironmentChanged();

    ProjectExplorer::BaseStringAspect *buildDirectoryAspect() const;
    void setConfigWidgetDisplayName(const QString &display);
    void setBuildDirectoryHistoryCompleter(const QString &history);
    void setConfigWidgetHasFrame(bool configWidgetHasFrame);
    void setBuildDirectorySettingsKey(const QString &key);

signals:
    void environmentChanged();
    void buildDirectoryChanged();
    void enabledChanged();
    void buildTypeChanged();

protected:
    virtual void initialize();

private:
    void emitBuildDirectoryChanged();

    bool m_clearSystemEnvironment = false;
    Utils::EnvironmentItems m_userEnvironmentChanges;
    QList<BuildStepList *> m_stepLists;
    ProjectExplorer::BaseStringAspect *m_buildDirectoryAspect = nullptr;
    Utils::FilePath m_lastEmmitedBuildDirectory;
    mutable Utils::Environment m_cachedEnvironment;
    QString m_configWidgetDisplayName;
    bool m_configWidgetHasFrame = false;

    // FIXME: Remove.
    BuildConfiguration::BuildType m_initialBuildType = BuildConfiguration::Unknown;
    Utils::FilePath m_initialBuildDirectory;
    QString m_initialDisplayName;
    QVariant m_extraInfo;
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
    const QList<BuildInfo>
        allAvailableSetups(const Kit *k, const Utils::FilePath &projectPath) const;

    BuildConfiguration *create(Target *parent, const BuildInfo &info) const;

    static BuildConfiguration *restore(Target *parent, const QVariantMap &map);
    static BuildConfiguration *clone(Target *parent, const BuildConfiguration *source);

    static BuildConfigurationFactory *find(const Kit *k, const Utils::FilePath &projectPath);
    static BuildConfigurationFactory *find(Target *parent);

    using IssueReporter = std::function<Tasks(Kit *, const QString &, const QString &)>;
    void setIssueReporter(const IssueReporter &issueReporter);
    const Tasks reportIssues(ProjectExplorer::Kit *kit,
                             const QString &projectPath, const QString &buildDir) const;

protected:
    virtual QList<BuildInfo>
        availableBuilds(const Kit *k, const Utils::FilePath &projectPath, bool forSetup) const = 0;

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
