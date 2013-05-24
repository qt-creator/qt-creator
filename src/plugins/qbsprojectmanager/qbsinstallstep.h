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
    QbsInstallStep(ProjectExplorer::BuildStepList *bsl);
    QbsInstallStep(ProjectExplorer::BuildStepList *bsl, const QbsInstallStep *other);
    ~QbsInstallStep();

    bool init();

    void run(QFutureInterface<bool> &fi);

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();

    bool runInGuiThread() const;
    void cancel();

    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;

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
    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *parent) const;
    // used to translate the types to names to display to the user
    QString displayNameForId(const Core::Id id) const;

    bool canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, const Core::Id id);
    // used to recreate the runConfigurations when restoring settings
    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map);
    bool canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product) const;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *product);
};


} // namespace Internal
} // namespace QbsProjectManager

#endif // QBSINSTALLSTEP_H
