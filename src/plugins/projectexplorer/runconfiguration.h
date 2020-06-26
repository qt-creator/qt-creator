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

#include "applicationlauncher.h"
#include "buildtargetinfo.h"
#include "devicesupport/idevice.h"
#include "projectconfiguration.h"
#include "projectexplorerconstants.h"
#include "task.h"

#include <utils/environment.h>
#include <utils/macroexpander.h>
#include <utils/port.h>

#include <QWidget>

#include <functional>
#include <memory>

namespace Utils { class OutputFormatter; }

namespace ProjectExplorer {
class BuildConfiguration;
class BuildSystem;
class GlobalOrProjectAspect;
class ProjectNode;
class Runnable;
class RunConfigurationFactory;
class RunConfiguration;
class RunConfigurationCreationInfo;
class Target;

/**
 * An interface for a hunk of global or per-project
 * configuration data.
 *
 */

class PROJECTEXPLORER_EXPORT ISettingsAspect : public QObject
{
    Q_OBJECT

public:
    ISettingsAspect();

    /// Create a configuration widget for this settings aspect.
    QWidget *createConfigWidget() const;

protected:
    using ConfigWidgetCreator = std::function<QWidget *()>;
    void setConfigWidgetCreator(const ConfigWidgetCreator &configWidgetCreator);

    friend class GlobalOrProjectAspect;
    /// Converts current object into map for storage.
    virtual void toMap(QVariantMap &map) const = 0;
    /// Read object state from @p map.
    virtual void fromMap(const QVariantMap &map) = 0;

    ConfigWidgetCreator m_configWidgetCreator;
};


/**
 * An interface to facilitate switching between hunks of
 * global and per-project configuration data.
 *
 */

class PROJECTEXPLORER_EXPORT GlobalOrProjectAspect : public ProjectConfigurationAspect
{
    Q_OBJECT

public:
    GlobalOrProjectAspect();
    ~GlobalOrProjectAspect() override;

    void setProjectSettings(ISettingsAspect *settings);
    void setGlobalSettings(ISettingsAspect *settings);

    bool isUsingGlobalSettings() const { return m_useGlobalSettings; }
    void setUsingGlobalSettings(bool value);
    void resetProjectToGlobalSettings();

    ISettingsAspect *projectSettings() const { return m_projectSettings; }
    ISettingsAspect *globalSettings() const { return m_globalSettings; }
    ISettingsAspect *currentSettings() const;

protected:
    friend class RunConfiguration;
    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &data) const override;

private:
    bool m_useGlobalSettings = false;
    ISettingsAspect *m_projectSettings = nullptr; // Owned if present.
    ISettingsAspect *m_globalSettings = nullptr;  // Not owned.
};

// Documentation inside.
class PROJECTEXPLORER_EXPORT RunConfiguration : public ProjectConfiguration
{
    Q_OBJECT

public:
    ~RunConfiguration() override;

    virtual QString disabledReason() const;
    virtual bool isEnabled() const;

    QWidget *createConfigurationWidget();

    bool isConfigured() const { return checkForIssues().isEmpty(); }
    virtual Tasks checkForIssues() const { return {}; }

    using CommandLineGetter = std::function<Utils::CommandLine()>;
    void setCommandLineGetter(const CommandLineGetter &cmdGetter);
    Utils::CommandLine commandLine() const;

    virtual Runnable runnable() const;

    // Return a handle to the build system target that created this run configuration.
    // May return an empty string if no target built the executable!
    QString buildKey() const { return m_buildKey; }
    // The BuildTargetInfo corresponding to the buildKey.
    BuildTargetInfo buildTargetInfo() const;

    ProjectExplorer::ProjectNode *productNode() const;

    template <class T = ISettingsAspect> T *currentSettings(Utils::Id id) const
    {
        if (auto a = qobject_cast<GlobalOrProjectAspect *>(aspect(id)))
            return qobject_cast<T *>(a->currentSettings());
        return nullptr;
    }

    using AspectFactory = std::function<ProjectConfigurationAspect *(Target *)>;
    template <class T> static void registerAspect()
    {
        addAspectFactory([](Target *target) { return new T(target); });
    }

    QMap<Utils::Id, QVariantMap> aspectData() const;

    void update();

    const Utils::MacroExpander *macroExpander() const { return &m_expander; }

signals:
    void enabledChanged();

protected:
    RunConfiguration(Target *target, Utils::Id id);

    /// convenience function to get current build system. Try to avoid.
    BuildSystem *activeBuildSystem() const;

    using Updater = std::function<void()>;
    void setUpdater(const Updater &updater);

    Task createConfigurationIssue(const QString &description) const;

private:
    // Any additional data should be handled by aspects.
    bool fromMap(const QVariantMap &map) final;
    QVariantMap toMap() const final;

    static void addAspectFactory(const AspectFactory &aspectFactory);

    friend class RunConfigurationCreationInfo;
    friend class RunConfigurationFactory;
    friend class Target;

    QString m_buildKey;
    CommandLineGetter m_commandLineGetter;
    Updater m_updater;
    Utils::MacroExpander m_expander;
};

class RunConfigurationCreationInfo
{
public:
    enum CreationMode {AlwaysCreate, ManualCreationOnly};
    RunConfiguration *create(Target *target) const;

    const RunConfigurationFactory *factory = nullptr;
    QString buildKey;
    QString displayName;
    QString displayNameUniquifier;
    Utils::FilePath projectFilePath;
    CreationMode creationMode = AlwaysCreate;
    bool useTerminal = false;
};

class PROJECTEXPLORER_EXPORT RunConfigurationFactory
{
public:
    RunConfigurationFactory();
    RunConfigurationFactory(const RunConfigurationFactory &) = delete;
    RunConfigurationFactory operator=(const RunConfigurationFactory &) = delete;
    virtual ~RunConfigurationFactory();

    static RunConfiguration *restore(Target *parent, const QVariantMap &map);
    static RunConfiguration *clone(Target *parent, RunConfiguration *source);
    static const QList<RunConfigurationCreationInfo> creatorsForTarget(Target *parent);

    Utils::Id runConfigurationId() const { return m_runConfigurationId; }

    static QString decoratedTargetName(const QString &targetName, Target *kit);

protected:
    virtual QList<RunConfigurationCreationInfo> availableCreators(Target *target) const;

    using RunConfigurationCreator = std::function<RunConfiguration *(Target *)>;

    template <class RunConfig>
    void registerRunConfiguration(Utils::Id runConfigurationId)
    {
        m_creator = [runConfigurationId](Target *t) -> RunConfiguration * {
            return new RunConfig(t, runConfigurationId);
        };
        m_runConfigurationId = runConfigurationId;
    }

    void addSupportedProjectType(Utils::Id projectTypeId);
    void addSupportedTargetDeviceType(Utils::Id deviceTypeId);
    void setDecorateDisplayNames(bool on);

private:
    bool canHandle(Target *target) const;
    RunConfiguration *create(Target *target) const;

    friend class RunConfigurationCreationInfo;
    RunConfigurationCreator m_creator;
    Utils::Id m_runConfigurationId;
    QList<Utils::Id> m_supportedProjectTypes;
    QList<Utils::Id> m_supportedTargetDeviceTypes;
    bool m_decorateDisplayNames = false;
};

class PROJECTEXPLORER_EXPORT FixedRunConfigurationFactory : public RunConfigurationFactory
{
public:
    explicit FixedRunConfigurationFactory(const QString &displayName,
                                          bool addDeviceName = false);

    QList<RunConfigurationCreationInfo> availableCreators(Target *parent) const override;

private:
    const QString m_fixedBuildTarget;
    const bool m_decorateTargetName;
};

} // namespace ProjectExplorer
