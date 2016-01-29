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

#ifndef QBSBUILDSTEP_H
#define QBSBUILDSTEP_H

#include "qbsbuildconfiguration.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/task.h>

#include <qbs.h>

namespace Utils { class FancyLineEdit; }

namespace QbsProjectManager {
namespace Internal {
class QbsProject;

class QbsBuildStepConfigWidget;

class QbsBuildStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    explicit QbsBuildStep(ProjectExplorer::BuildStepList *bsl);
    QbsBuildStep(ProjectExplorer::BuildStepList *bsl, const QbsBuildStep *other);
    ~QbsBuildStep() override;

    bool init(QList<const BuildStep *> &earlierSteps) override;

    void run(QFutureInterface<bool> &fi) override;

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;

    bool runInGuiThread() const override;
    void cancel() override;

    QVariantMap qbsConfiguration() const;
    void setQbsConfiguration(const QVariantMap &config);

    bool keepGoing() const;
    bool showCommandLines() const;
    bool install() const;
    bool cleanInstallRoot() const;
    int maxJobs() const;
    QString buildVariant() const;

    bool isQmlDebuggingEnabled() const;

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

signals:
    void qbsConfigurationChanged();
    void qbsBuildOptionsChanged();

private slots:
    void buildingDone(bool success);
    void reparsingDone(bool success);
    void handleTaskStarted(const QString &desciption, int max);
    void handleProgress(int value);
    void handleCommandDescriptionReport(const QString &highlight, const QString &message);
    void handleProcessResultReport(const qbs::ProcessResult &result);

private:
    void createTaskAndOutput(ProjectExplorer::Task::TaskType type,
                             const QString &message, const QString &file, int line);

    void setBuildVariant(const QString &variant);
    QString profile() const;

    void setKeepGoing(bool kg);
    void setMaxJobs(int jobcount);
    void setShowCommandLines(bool show);
    void setInstall(bool install);
    void setCleanInstallRoot(bool clean);

    void parseProject();
    void build();
    void finish();

    QbsProject *qbsProject() const;

    QVariantMap m_qbsConfiguration;
    qbs::BuildOptions m_qbsBuildOptions;

    // Temporary data:
    QStringList m_changedFiles;
    QStringList m_activeFileTags;
    QStringList m_products;

    QFutureInterface<bool> *m_fi;
    qbs::BuildJob *m_job;
    int m_progressBase;
    bool m_lastWasSuccess;
    ProjectExplorer::IOutputParser *m_parser;
    bool m_parsingProject;

    friend class QbsBuildStepConfigWidget;
};

namespace Ui { class QbsBuildStepConfigWidget; }

class QbsBuildStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    QbsBuildStepConfigWidget(QbsBuildStep *step);
    ~QbsBuildStepConfigWidget();
    QString summaryText() const;
    QString displayName() const;

private slots:
    void updateState();
    void updateQmlDebuggingOption();
    void updatePropertyEdit(const QVariantMap &data);

    void changeBuildVariant(int);
    void changeShowCommandLines(bool show);
    void changeKeepGoing(bool kg);
    void changeJobCount(int count);
    void changeInstall(bool install);
    void changeCleanInstallRoot(bool clean);
    void applyCachedProperties();

    // QML debugging:
    void linkQmlDebuggingLibraryChecked(bool checked);

private:
    bool validateProperties(Utils::FancyLineEdit *edit, QString *errorMessage);

    Ui::QbsBuildStepConfigWidget *m_ui;

    QList<QPair<QString, QString> > m_propertyCache;
    QbsBuildStep *m_step;
    QString m_summary;
    bool m_ignoreChange;
};

class QbsBuildStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit QbsBuildStepFactory(QObject *parent = 0);

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

#endif // QBSBUILDSTEP_H
