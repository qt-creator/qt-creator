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

#include <QFutureInterface>
#include <QWidget>

namespace ProjectExplorer {
class Task;
class BuildConfiguration;
class BuildStepList;
class DeployConfiguration;
class Target;

class BuildStepConfigWidget;

// Documentation inside.
class PROJECTEXPLORER_EXPORT BuildStep : public ProjectConfiguration
{
    Q_OBJECT

protected:
    BuildStep(BuildStepList *bsl, Core::Id id);
    BuildStep(BuildStepList *bsl, BuildStep *bs);

public:
    virtual bool init(QList<const BuildStep *> &earlierSteps) = 0;

    virtual void run(QFutureInterface<bool> &fi) = 0;

    virtual BuildStepConfigWidget *createConfigWidget() = 0;

    virtual bool immutable() const;
    virtual bool runInGuiThread() const;
    virtual void cancel();

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

private:
    void ctor();

    bool m_enabled = true;
};

class PROJECTEXPLORER_EXPORT BuildStepInfo
{
public:
    enum Flags {
        Uncreatable = 1 << 0,
        Unclonable  = 1 << 1,
        UniqueStep  = 1 << 8    // Can't be used twice in a BuildStepList
    };

    BuildStepInfo() {}
    BuildStepInfo(Core::Id id, const QString &displayName, Flags flags = Flags())
        : id(id), displayName(displayName), flags(flags)
    {}

    Core::Id id;
    QString displayName;
    Flags flags = Flags();
};

class PROJECTEXPLORER_EXPORT IBuildStepFactory : public QObject
{
    Q_OBJECT

public:
    explicit IBuildStepFactory(QObject *parent = nullptr);

    virtual QList<BuildStepInfo> availableSteps(BuildStepList *parent) const = 0;
    virtual BuildStep *create(BuildStepList *parent, Core::Id id) = 0;
    virtual BuildStep *restore(BuildStepList *parent, const QVariantMap &map);
    virtual BuildStep *clone(BuildStepList *parent, BuildStep *product) = 0;
};

class PROJECTEXPLORER_EXPORT BuildStepConfigWidget : public QWidget
{
    Q_OBJECT
public:
    virtual QString summaryText() const = 0;
    virtual QString additionalSummaryText() const { return QString(); }
    virtual QString displayName() const = 0;
    virtual bool showWidget() const { return true; }

signals:
    void updateSummary();
    void updateAdditionalSummary();
};

class PROJECTEXPLORER_EXPORT SimpleBuildStepConfigWidget : public BuildStepConfigWidget
{
    Q_OBJECT
public:
    SimpleBuildStepConfigWidget(BuildStep *step) : m_step(step)
    {
        connect(m_step, &ProjectConfiguration::displayNameChanged,
                this, &BuildStepConfigWidget::updateSummary);
    }

    QString summaryText() const { return QLatin1String("<b>") + displayName() + QLatin1String("</b>"); }
    QString displayName() const { return m_step->displayName(); }
    bool showWidget() const { return false; }
    BuildStep *step() const { return m_step; }

private:
    BuildStep *m_step;
};

} // namespace ProjectExplorer

Q_DECLARE_METATYPE(ProjectExplorer::BuildStep::OutputFormat)
Q_DECLARE_METATYPE(ProjectExplorer::BuildStep::OutputNewlineSetting)
