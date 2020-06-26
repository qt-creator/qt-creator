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

namespace Utils { class MacroExpander; }

namespace ProjectExplorer {

namespace Internal { class BuildConfigurationPrivate; }

class BuildDirectoryAspect;
class BuildInfo;
class BuildSystem;
class BuildStepList;
class Kit;
class NamedWidget;
class Node;
class RunConfiguration;
class Target;

class PROJECTEXPLORER_EXPORT BuildConfiguration : public ProjectConfiguration
{
    Q_OBJECT

protected:
    friend class BuildConfigurationFactory;
    explicit BuildConfiguration(Target *target, Utils::Id id);

public:
    ~BuildConfiguration() override;

    Utils::FilePath buildDirectory() const;
    Utils::FilePath rawBuildDirectory() const;
    void setBuildDirectory(const Utils::FilePath &dir);

    virtual BuildSystem *buildSystem() const;

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

    const QList<Utils::Id> customParsers() const;
    void setCustomParsers(const QList<Utils::Id> &parsers);

    BuildStepList *buildSteps() const;
    BuildStepList *cleanSteps() const;

    void appendInitialBuildStep(Utils::Id id);
    void appendInitialCleanStep(Utils::Id id);

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    virtual bool isEnabled() const;
    virtual QString disabledReason() const;

    virtual bool regenerateBuildFiles(Node *node);

    virtual void restrictNextBuild(const RunConfiguration *rc);

    enum BuildType {
        Unknown,
        Debug,
        Profile,
        Release
    };
    virtual BuildType buildType() const;

    static QString buildTypeName(BuildType type);

    bool isActive() const;

    static void prependCompilerPathToEnvironment(Kit *k, Utils::Environment &env);
    void updateCacheAndEmitEnvironmentChanged();

    ProjectExplorer::BuildDirectoryAspect *buildDirectoryAspect() const;
    void setConfigWidgetDisplayName(const QString &display);
    void setBuildDirectoryHistoryCompleter(const QString &history);
    void setConfigWidgetHasFrame(bool configWidgetHasFrame);
    void setBuildDirectorySettingsKey(const QString &key);

    void addConfigWidgets(const std::function<void (NamedWidget *)> &adder);

    void doInitialize(const BuildInfo &info);

    Utils::MacroExpander *macroExpander() const;

    bool createBuildDirectory();

signals:
    void environmentChanged();
    void buildDirectoryChanged();
    void enabledChanged();
    void buildTypeChanged();

protected:
    void setInitializer(const std::function<void(const BuildInfo &info)> &initializer);

private:
    void emitBuildDirectoryChanged();
    Internal::BuildConfigurationPrivate *d = nullptr;
};

class PROJECTEXPLORER_EXPORT BuildConfigurationFactory
{
protected:
    BuildConfigurationFactory();
    BuildConfigurationFactory(const BuildConfigurationFactory &) = delete;
    BuildConfigurationFactory &operator=(const BuildConfigurationFactory &) = delete;

    virtual ~BuildConfigurationFactory(); // Needed for dynamic_casts in importers.

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
    using BuildGenerator
        = std::function<QList<BuildInfo>(const Kit *, const Utils::FilePath &, bool)>;
    void setBuildGenerator(const BuildGenerator &buildGenerator);

    bool supportsTargetDeviceType(Utils::Id id) const;
    void setSupportedProjectType(Utils::Id id);
    void setSupportedProjectMimeTypeName(const QString &mimeTypeName);
    void addSupportedTargetDeviceType(Utils::Id id);
    void setDefaultDisplayName(const QString &defaultDisplayName);

    using BuildConfigurationCreator = std::function<BuildConfiguration *(Target *)>;

    template <class BuildConfig>
    void registerBuildConfiguration(Utils::Id buildConfigId)
    {
        m_creator = [buildConfigId](Target *t) { return new BuildConfig(t, buildConfigId); };
        m_buildConfigId = buildConfigId;
    }

private:
    bool canHandle(const ProjectExplorer::Target *t) const;

    BuildConfigurationCreator m_creator;
    Utils::Id m_buildConfigId;
    Utils::Id m_supportedProjectType;
    QList<Utils::Id> m_supportedTargetDeviceTypes;
    QString m_supportedProjectMimeTypeName;
    IssueReporter m_issueReporter;
    BuildGenerator m_buildGenerator;
};

} // namespace ProjectExplorer
