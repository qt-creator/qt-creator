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

#ifndef QBSINSTALLSTEP_H
#define QBSINSTALLSTEP_H

#include "qbsbuildconfiguration.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/task.h>

#include <qbs.h>

namespace QbsProjectManager {
namespace Internal {

class QbsInstallStepConfigWidget;

class QbsInstallStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    explicit QbsInstallStep(ProjectExplorer::BuildStepList *bsl);
    QbsInstallStep(ProjectExplorer::BuildStepList *bsl, const QbsInstallStep *other);
    ~QbsInstallStep() override;

    bool init(QList<const BuildStep *> &earlierSteps) override;

    void run(QFutureInterface<bool> &fi) override;

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;

    bool runInGuiThread() const override;
    void cancel() override;

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    qbs::InstallOptions installOptions() const;
    QString installRoot() const;
    QString absoluteInstallRoot() const;
    bool removeFirst() const;
    bool dryRun() const;
    bool keepGoing() const;

signals:
    void changed();

private slots:
    void installDone(bool success);
    void handleTaskStarted(const QString &desciption, int max);
    void handleProgress(int value);

private:
    void createTaskAndOutput(ProjectExplorer::Task::TaskType type,
                             const QString &message, const QString &file, int line);

    void setInstallRoot(const QString &ir);
    void setRemoveFirst(bool rf);
    void setDryRun(bool dr);
    void setKeepGoing(bool kg);

    qbs::InstallOptions m_qbsInstallOptions;

    QFutureInterface<bool> *m_fi;
    qbs::InstallJob *m_job;
    int m_progressBase;
    bool m_showCompilerOutput;
    ProjectExplorer::IOutputParser *m_parser;

    friend class QbsInstallStepConfigWidget;
};

namespace Ui { class QbsInstallStepConfigWidget; }

class QbsInstallStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    QbsInstallStepConfigWidget(QbsInstallStep *step);
    ~QbsInstallStepConfigWidget();
    QString summaryText() const;
    QString displayName() const;

private slots:
    void updateState();

    void changeInstallRoot();
    void changeRemoveFirst(bool rf);
    void changeDryRun(bool dr);
    void changeKeepGoing(bool kg);

private:
    Ui::QbsInstallStepConfigWidget *m_ui;

    QbsInstallStep *m_step;
    QString m_summary;
    bool m_ignoreChange;
};

class QbsInstallStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit QbsInstallStepFactory(QObject *parent = 0);

    // used to show the list of possible additons to a target, returns a list of types
    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *parent) const override;
    // used to translate the types to names to display to the user
    QString displayNameForId(Core::Id id) const override;

    bool canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const override;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id) override;
    // used to recreate the runConfigurations when restoring settings
    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const override;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) override;
    bool canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const override;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) override;
};

} // namespace Internal
} // namespace QbsProjectManager

#endif // QBSINSTALLSTEP_H
