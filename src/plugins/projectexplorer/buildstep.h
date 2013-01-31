/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef BUILDSTEP_H
#define BUILDSTEP_H

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
    BuildStep(BuildStepList *bsl, const Core::Id id);
    BuildStep(BuildStepList *bsl, BuildStep *bs);

public:
    virtual ~BuildStep();

    virtual bool init() = 0;

    virtual void run(QFutureInterface<bool> &fi) = 0;

    virtual BuildStepConfigWidget *createConfigWidget() = 0;

    virtual bool immutable() const;
    virtual bool runInGuiThread() const;
    virtual void cancel();

    virtual bool fromMap(const QVariantMap &map);
    virtual QVariantMap toMap() const;

    bool enabled() const;
    void setEnabled(bool b);

    BuildConfiguration *buildConfiguration() const;
    DeployConfiguration *deployConfiguration() const;
    ProjectConfiguration *projectConfiguration() const;
    Target *target() const;
    Project *project() const;

    enum OutputFormat { NormalOutput, ErrorOutput, MessageOutput, ErrorMessageOutput };
    enum OutputNewlineSetting { DoAppendNewline, DontAppendNewline };

signals:
    void addTask(const ProjectExplorer::Task &task);

    void addOutput(const QString &string, ProjectExplorer::BuildStep::OutputFormat format,
        ProjectExplorer::BuildStep::OutputNewlineSetting newlineSetting = DoAppendNewline) const;

    void finished();

    void enabledChanged();
private:
    bool m_enabled;
};

class PROJECTEXPLORER_EXPORT IBuildStepFactory :
    public QObject
{
    Q_OBJECT

public:
    explicit IBuildStepFactory(QObject *parent = 0);
    virtual ~IBuildStepFactory();

    // used to show the list of possible additons to a target, returns a list of types
    virtual QList<Core::Id> availableCreationIds(BuildStepList *parent) const = 0;
    // used to translate the types to names to display to the user
    virtual QString displayNameForId(const Core::Id id) const = 0;

    virtual bool canCreate(BuildStepList *parent, const Core::Id id) const = 0;
    virtual BuildStep *create(BuildStepList *parent, const Core::Id id) = 0;
    // used to recreate the runConfigurations when restoring settings
    virtual bool canRestore(BuildStepList *parent, const QVariantMap &map) const = 0;
    virtual BuildStep *restore(BuildStepList *parent, const QVariantMap &map) = 0;
    virtual bool canClone(BuildStepList *parent, BuildStep *product) const = 0;
    virtual BuildStep *clone(BuildStepList *parent, BuildStep *product) = 0;
};

class PROJECTEXPLORER_EXPORT BuildStepConfigWidget
    : public QWidget
{
    Q_OBJECT
public:
    BuildStepConfigWidget()
        : QWidget()
        {}
    virtual QString summaryText() const = 0;
    virtual QString additionalSummaryText() const { return QString(); }
    virtual QString displayName() const = 0;
    virtual bool showWidget() const { return true; }

signals:
    void updateSummary();
    void updateAdditionalSummary();
};

class PROJECTEXPLORER_EXPORT SimpleBuildStepConfigWidget
    : public BuildStepConfigWidget
{
    Q_OBJECT
public:
    SimpleBuildStepConfigWidget(BuildStep *step)
        : m_step(step)
    {
        connect(m_step, SIGNAL(displayNameChanged()), SIGNAL(updateSummary()));
    }

    ~SimpleBuildStepConfigWidget() {}

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

#endif // BUILDSTEP_H
