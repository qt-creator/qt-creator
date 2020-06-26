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

#include "buildconfiguration.h"
#include "projectexplorer_export.h"

#include <utils/optional.h>
#include <utils/qtcassert.h>

#include <QFutureInterface>
#include <QWidget>

#include <atomic>
#include <functional>
#include <memory>

namespace Utils {
class Environment;
class FilePath;
class MacroExpander;
class OutputFormatter;
} // Utils

namespace ProjectExplorer {

class BuildConfiguration;
class BuildStepConfigWidget;
class BuildStepFactory;
class BuildStepList;
class BuildSystem;
class DeployConfiguration;
class Target;
class Task;

// Documentation inside.
class PROJECTEXPLORER_EXPORT BuildStep : public ProjectConfiguration
{
    Q_OBJECT

protected:
    friend class BuildStepFactory;
    explicit BuildStep(BuildStepList *bsl, Utils::Id id);

public:
    ~BuildStep() override;
    virtual bool init() = 0;
    void run();
    void cancel();
    virtual BuildStepConfigWidget *createConfigWidget();

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    bool enabled() const;
    void setEnabled(bool b);

    BuildStepList *stepList() const;

    BuildConfiguration *buildConfiguration() const;
    DeployConfiguration *deployConfiguration() const;
    ProjectConfiguration *projectConfiguration() const;

    BuildSystem *buildSystem() const;
    Utils::Environment buildEnvironment() const;
    Utils::FilePath buildDirectory() const;
    BuildConfiguration::BuildType buildType() const;
    Utils::MacroExpander *macroExpander() const;
    QString fallbackWorkingDirectory() const;

    virtual void setupOutputFormatter(Utils::OutputFormatter *formatter);

    enum class OutputFormat {
        Stdout, Stderr, // These are for forwarded output from external tools
        NormalMessage, ErrorMessage // These are for messages from Creator itself
    };

    enum OutputNewlineSetting { DoAppendNewline, DontAppendNewline };

    static void reportRunResult(QFutureInterface<bool> &fi, bool success);

    bool widgetExpandedByDefault() const;
    void setWidgetExpandedByDefault(bool widgetExpandedByDefault);

    bool hasUserExpansionState() const { return m_wasExpanded.has_value(); }
    bool wasUserExpanded() const { return m_wasExpanded.value_or(false); }
    void setUserExpanded(bool expanded) { m_wasExpanded = expanded; }

    bool isImmutable() const { return m_immutable; }
    void setImmutable(bool immutable) { m_immutable = immutable; }

    virtual QVariant data(Utils::Id id) const;
    void setSummaryUpdater(const std::function<QString ()> &summaryUpdater);

    void addMacroExpander();

signals:
    /// Adds a \p task to the Issues pane.
    /// Do note that for linking compile output with tasks, you should first emit the output
    /// and then emit the task. \p linkedOutput lines will be linked. And the last \p skipLines will
    /// be skipped.
    void addTask(const ProjectExplorer::Task &task, int linkedOutputLines = 0, int skipLines = 0);

    /// Adds \p string to the compile output view, formatted in \p format
    void addOutput(const QString &string, ProjectExplorer::BuildStep::OutputFormat format,
        ProjectExplorer::BuildStep::OutputNewlineSetting newlineSetting = DoAppendNewline);

    void enabledChanged();

    void progress(int percentage, const QString &message);
    void finished(bool result);

protected:
    void runInThread(const std::function<bool()> &syncImpl);

    std::function<bool()> cancelChecker() const;
    bool isCanceled() const;

private:
    using ProjectConfiguration::parent;

    virtual void doRun() = 0;
    virtual void doCancel();

    std::atomic_bool m_cancelFlag;
    bool m_enabled = true;
    bool m_immutable = false;
    bool m_widgetExpandedByDefault = true;
    bool m_runInGuiThread = true;
    bool m_addMacroExpander = false;
    Utils::optional<bool> m_wasExpanded;
    std::function<QString()> m_summaryUpdater;
};

class PROJECTEXPLORER_EXPORT BuildStepInfo
{
public:
    enum Flags {
        Uncreatable = 1 << 0,
        Unclonable  = 1 << 1,
        UniqueStep  = 1 << 8    // Can't be used twice in a BuildStepList
    };

    using BuildStepCreator = std::function<BuildStep *(BuildStepList *)>;

    Utils::Id id;
    QString displayName;
    Flags flags = Flags();
    BuildStepCreator creator;
};

class PROJECTEXPLORER_EXPORT BuildStepFactory
{
public:
    BuildStepFactory();
    BuildStepFactory(const BuildStepFactory &) = delete;
    BuildStepFactory &operator=(const BuildStepFactory &) = delete;
    virtual ~BuildStepFactory();

    static const QList<BuildStepFactory *> allBuildStepFactories();

    BuildStepInfo stepInfo() const;
    Utils::Id stepId() const;
    BuildStep *create(BuildStepList *parent, Utils::Id id);
    BuildStep *restore(BuildStepList *parent, const QVariantMap &map);

    bool canHandle(BuildStepList *bsl) const;

protected:
    using BuildStepCreator = std::function<BuildStep *(BuildStepList *)>;

    template <class BuildStepType>
    void registerStep(Utils::Id id)
    {
        QTC_CHECK(!m_info.creator);
        m_info.id = id;
        m_info.creator = [id](BuildStepList *bsl) { return new BuildStepType(bsl, id); };
    }

    void setSupportedStepList(Utils::Id id);
    void setSupportedStepLists(const QList<Utils::Id> &ids);
    void setSupportedConfiguration(Utils::Id id);
    void setSupportedProjectType(Utils::Id id);
    void setSupportedDeviceType(Utils::Id id);
    void setSupportedDeviceTypes(const QList<Utils::Id> &ids);
    void setRepeatable(bool on) { m_isRepeatable = on; }
    void setDisplayName(const QString &displayName);
    void setFlags(BuildStepInfo::Flags flags);

private:
    BuildStepInfo m_info;

    Utils::Id m_supportedProjectType;
    QList<Utils::Id> m_supportedDeviceTypes;
    QList<Utils::Id> m_supportedStepLists;
    Utils::Id m_supportedConfiguration;
    bool m_isRepeatable = true;
};

class PROJECTEXPLORER_EXPORT BuildStepConfigWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BuildStepConfigWidget(BuildStep *step);

    QString summaryText() const;
    QString displayName() const;
    BuildStep *step() const { return m_step; }

    void setDisplayName(const QString &displayName);
    void setSummaryText(const QString &summaryText);

    void setSummaryUpdater(const std::function<QString()> &summaryUpdater);
    void recreateSummary();

signals:
    void updateSummary();

private:
    BuildStep *m_step = nullptr;
    QString m_displayName;
    QString m_summaryText;
    std::function<QString()> m_summaryUpdater;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::BuildStep::OutputFormat)
Q_DECLARE_METATYPE(ProjectExplorer::BuildStep::OutputNewlineSetting)
