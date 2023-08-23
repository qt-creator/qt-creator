// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include "buildconfiguration.h"
#include "projectconfiguration.h"

#include <utils/qtcassert.h>

#include <functional>
#include <optional>

namespace Utils {
class MacroExpander;
class OutputFormatter;
} // Utils

namespace Tasking { class GroupItem; }

namespace ProjectExplorer {

class BuildStepFactory;
class BuildStepList;
class BuildSystem;
class DeployConfiguration;
class Task;

// Documentation inside.
class PROJECTEXPLORER_EXPORT BuildStep : public ProjectConfiguration
{
    Q_OBJECT

protected:
    friend class BuildStepFactory;
    explicit BuildStep(BuildStepList *bsl, Utils::Id id);

public:
    virtual bool init() = 0;

    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &map) const override;

    bool enabled() const;
    void setEnabled(bool b);

    BuildStepList *stepList() const;

    BuildConfiguration *buildConfiguration() const;

    BuildSystem *buildSystem() const;
    BuildConfiguration::BuildType buildType() const;
    Utils::MacroExpander *macroExpander() const;

    enum class OutputFormat {
        Stdout, Stderr, // These are for forwarded output from external tools
        NormalMessage, ErrorMessage // These are for messages from Creator itself
    };

    enum OutputNewlineSetting { DoAppendNewline, DontAppendNewline };

    enum Flags {
        Uncreatable = 1 << 0,
        Unclonable  = 1 << 1,
        UniqueStep  = 1 << 8    // Can't be used twice in a BuildStepList
    };

    bool widgetExpandedByDefault() const;
    bool hasUserExpansionState() const { return m_wasExpanded.has_value(); }
    bool wasUserExpanded() const { return m_wasExpanded.value_or(false); }
    void setUserExpanded(bool expanded) { m_wasExpanded = expanded; }
    bool isImmutable() const { return m_immutable; }
    virtual QVariant data(Utils::Id id) const;
    QString summaryText() const;
    QWidget *doCreateConfigWidget();

signals:
    void updateSummary();

    /// Adds a \p task to the Issues pane.
    /// Do note that for linking compile output with tasks, you should first emit the output
    /// and then emit the task. \p linkedOutput lines will be linked. And the last \p skipLines will
    /// be skipped.
    void addTask(const Task &task, int linkedOutputLines = 0, int skipLines = 0);

    /// Adds \p string to the compile output view, formatted in \p format
    void addOutput(const QString &string, OutputFormat format,
                   OutputNewlineSetting newlineSetting = DoAppendNewline);

    void enabledChanged();

    void progress(int percentage, const QString &message);

protected:
    void setWidgetExpandedByDefault(bool widgetExpandedByDefault);
    void setImmutable(bool immutable) { m_immutable = immutable; }
    void setSummaryUpdater(const std::function<QString()> &summaryUpdater);
    void addMacroExpander() { m_addMacroExpander = true; }
    void setSummaryText(const QString &summaryText);

    DeployConfiguration *deployConfiguration() const;
    Utils::Environment buildEnvironment() const;
    Utils::FilePath buildDirectory() const;
    QString fallbackWorkingDirectory() const;

    virtual QWidget *createConfigWidget();
    virtual void setupOutputFormatter(Utils::OutputFormatter *formatter);

private:
    friend class BuildManager;
    virtual Tasking::GroupItem runRecipe() = 0;
    ProjectConfiguration *projectConfiguration() const;

    BuildStepList * const m_stepList;
    bool m_enabled = true;
    bool m_immutable = false;
    bool m_widgetExpandedByDefault = true;
    bool m_addMacroExpander = false;
    std::optional<bool> m_wasExpanded;
    std::function<QString()> m_summaryUpdater;

    QString m_summaryText;
};

class PROJECTEXPLORER_EXPORT BuildStepFactory
{
public:
    BuildStepFactory();
    BuildStepFactory(const BuildStepFactory &) = delete;
    BuildStepFactory &operator=(const BuildStepFactory &) = delete;
    virtual ~BuildStepFactory();

    static const QList<BuildStepFactory *> allBuildStepFactories();

    BuildStep::Flags stepFlags() const;
    Utils::Id stepId() const;
    BuildStep *create(BuildStepList *parent);
    BuildStep *restore(BuildStepList *parent, const Utils::Store &map);

    bool canHandle(BuildStepList *bsl) const;

    QString displayName() const;

protected:
    using BuildStepCreator = std::function<BuildStep *(BuildStepList *)>;

    template <class BuildStepType>
    void registerStep(Utils::Id id)
    {
        QTC_CHECK(!m_creator);
        m_stepId = id;
        m_creator = [id](BuildStepList *bsl) { return new BuildStepType(bsl, id); };
    }
    void cloneStepCreator(Utils::Id exitstingStepId, Utils::Id overrideNewStepId = {});

    void setSupportedStepList(Utils::Id id);
    void setSupportedStepLists(const QList<Utils::Id> &ids);
    void setSupportedConfiguration(Utils::Id id);
    void setSupportedProjectType(Utils::Id id);
    void setSupportedDeviceType(Utils::Id id);
    void setSupportedDeviceTypes(const QList<Utils::Id> &ids);
    void setRepeatable(bool on) { m_isRepeatable = on; }
    void setDisplayName(const QString &displayName);
    void setFlags(BuildStep::Flags flags);

private:
    Utils::Id m_stepId;
    QString m_displayName;
    BuildStep::Flags m_flags = {};
    BuildStepCreator m_creator;

    Utils::Id m_supportedProjectType;
    QList<Utils::Id> m_supportedDeviceTypes;
    QList<Utils::Id> m_supportedStepLists;
    Utils::Id m_supportedConfiguration;
    bool m_isRepeatable = true;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::BuildStep::OutputFormat)
Q_DECLARE_METATYPE(ProjectExplorer::BuildStep::OutputNewlineSetting)
