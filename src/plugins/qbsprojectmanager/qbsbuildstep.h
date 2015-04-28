/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    QbsBuildStep(ProjectExplorer::BuildStepList *bsl);
    QbsBuildStep(ProjectExplorer::BuildStepList *bsl, const QbsBuildStep *other);
    ~QbsBuildStep();

    bool init();

    void run(QFutureInterface<bool> &fi);

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();

    bool runInGuiThread() const;
    void cancel();

    QVariantMap qbsConfiguration() const;
    void setQbsConfiguration(const QVariantMap &config);

    bool dryRun() const;
    bool keepGoing() const;
    bool showCommandLines() const;
    bool install() const;
    bool cleanInstallRoot() const;
    int maxJobs() const;
    QString buildVariant() const;

    bool isQmlDebuggingEnabled() const;

    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

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

    void setDryRun(bool dr);
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
    void changeDryRun(bool dr);
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
    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *parent) const;
    // used to translate the types to names to display to the user
    QString displayNameForId(Core::Id id) const;

    bool canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id);
    // used to recreate the runConfigurations when restoring settings
    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map);
    bool canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product);
};


} // namespace Internal
} // namespace QbsProjectManager

#endif // QBSBUILDSTEP_H
