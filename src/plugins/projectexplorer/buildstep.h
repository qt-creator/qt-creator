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

#include <utils/qtcassert.h>

#include <QFutureInterface>
#include <QWidget>

#include <atomic>
#include <functional>
#include <memory>

namespace ProjectExplorer {

class BuildConfiguration;
class BuildStepConfigWidget;
class BuildStepFactory;
class BuildStepList;
class DeployConfiguration;
class Target;
class Task;

// Documentation inside.
class PROJECTEXPLORER_EXPORT BuildStep : public ProjectConfiguration
{
    Q_OBJECT

protected:
    friend class BuildStepFactory;
    explicit BuildStep(BuildStepList *bsl, Core::Id id);

public:
    virtual bool init() = 0;
    void run();
    void cancel();
    virtual BuildStepConfigWidget *createConfigWidget();

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    bool enabled() const;
    void setEnabled(bool b);

    BuildConfiguration *buildConfiguration() const;
    DeployConfiguration *deployConfiguration() const;
    ProjectConfiguration *projectConfiguration() const;
    Target *target() const;
    Project *project() const override;

    enum class OutputFormat {
        Stdout, Stderr, // These are for forwarded output from external tools
        NormalMessage, ErrorMessage // These are for messages from Creator itself
    };

    enum OutputNewlineSetting { DoAppendNewline, DontAppendNewline };

    static void reportRunResult(QFutureInterface<bool> &fi, bool success);

    bool isActive() const override;

    bool widgetExpandedByDefault() const;
    void setWidgetExpandedByDefault(bool widgetExpandedByDefault);

    bool isImmutable() const { return m_immutable; }
    void setImmutable(bool immutable) { m_immutable = immutable; }

signals:
    /// Adds a \p task to the Issues pane.
    /// Do note that for linking compile output with tasks, you should first emit the task
    /// and then emit the output. \p linkedOutput lines will be linked. And the last \p skipLines will
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
    virtual void doRun() = 0;
    virtual void doCancel();

    std::atomic_bool m_cancelFlag;
    bool m_enabled = true;
    bool m_immutable = false;
    bool m_widgetExpandedByDefault = true;
    bool m_runInGuiThread = true;
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

    Core::Id id;
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
    Core::Id stepId() const;
    BuildStep *create(BuildStepList *parent, Core::Id id);
    BuildStep *restore(BuildStepList *parent, const QVariantMap &map);

    bool canHandle(BuildStepList *bsl) const;

protected:
    using BuildStepCreator = std::function<BuildStep *(BuildStepList *)>;

    template <class BuildStepType>
    void registerStep(Core::Id id)
    {
        QTC_CHECK(!m_info.creator);
        m_info.id = id;
        m_info.creator = [](BuildStepList *bsl) { return new BuildStepType(bsl); };
    }

    void setSupportedStepList(Core::Id id);
    void setSupportedStepLists(const QList<Core::Id> &ids);
    void setSupportedConfiguration(Core::Id id);
    void setSupportedProjectType(Core::Id id);
    void setSupportedDeviceType(Core::Id id);
    void setSupportedDeviceTypes(const QList<Core::Id> &ids);
    void setRepeatable(bool on) { m_isRepeatable = on; }
    void setDisplayName(const QString &displayName);
    void setFlags(BuildStepInfo::Flags flags);

private:
    BuildStepInfo m_info;

    Core::Id m_supportedProjectType;
    QList<Core::Id> m_supportedDeviceTypes;
    QList<Core::Id> m_supportedStepLists;
    Core::Id m_supportedConfiguration;
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

signals:
    void updateSummary();

private:
    BuildStep *m_step = nullptr;
    QString m_displayName;
    QString m_summaryText;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::BuildStep::OutputFormat)
Q_DECLARE_METATYPE(ProjectExplorer::BuildStep::OutputNewlineSetting)
