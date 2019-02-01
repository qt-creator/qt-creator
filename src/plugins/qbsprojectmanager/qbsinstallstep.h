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
    ~QbsInstallStep() override;

    qbs::InstallOptions installOptions() const;
    QString installRoot() const;
    bool removeFirst() const;
    bool dryRun() const;
    bool keepGoing() const;

signals:
    void changed();

private:
    bool init() override;
    void doRun() override;
    void doCancel() override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    const QbsBuildConfiguration *buildConfig() const;
    void installDone(bool success);
    void handleTaskStarted(const QString &desciption, int max);
    void handleProgress(int value);

    void createTaskAndOutput(ProjectExplorer::Task::TaskType type,
                             const QString &message, const QString &file, int line);

    void setRemoveFirst(bool rf);
    void setDryRun(bool dr);
    void setKeepGoing(bool kg);
    void handleBuildConfigChanged();

    qbs::InstallOptions m_qbsInstallOptions;

    qbs::InstallJob *m_job = nullptr;
    QString m_description;
    int m_maxProgress;
    bool m_showCompilerOutput = true;
    ProjectExplorer::IOutputParser *m_parser = nullptr;

    friend class QbsInstallStepConfigWidget;
};

namespace Ui { class QbsInstallStepConfigWidget; }

class QbsInstallStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    QbsInstallStepConfigWidget(QbsInstallStep *step);
    ~QbsInstallStepConfigWidget() override;

private:
    void updateState();

    void changeRemoveFirst(bool rf);
    void changeDryRun(bool dr);
    void changeKeepGoing(bool kg);

private:
    Ui::QbsInstallStepConfigWidget *m_ui;

    QbsInstallStep *m_step;
    bool m_ignoreChange;
};

class QbsInstallStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    QbsInstallStepFactory();
};

} // namespace Internal
} // namespace QbsProjectManager
