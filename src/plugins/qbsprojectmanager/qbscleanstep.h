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

class QbsCleanStepConfigWidget;

class QbsCleanStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    explicit QbsCleanStep(ProjectExplorer::BuildStepList *bsl);
    ~QbsCleanStep() override;

    bool init(QList<const BuildStep *> &earlierSteps) override;

    void run(QFutureInterface<bool> &fi) override;

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;

    bool runInGuiThread() const override;
    void cancel() override;

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    bool dryRun() const;
    bool keepGoing() const;
    int maxJobs() const;

signals:
    void changed();

private:
    void cleaningDone(bool success);
    void handleTaskStarted(const QString &desciption, int max);
    void handleProgress(int value);

    void createTaskAndOutput(ProjectExplorer::Task::TaskType type,
                             const QString &message, const QString &file, int line);

    void setDryRun(bool dr);
    void setKeepGoing(bool kg);
    void setMaxJobs(int jobcount);

    qbs::CleanOptions m_qbsCleanOptions;
    QStringList m_products;

    QFutureInterface<bool> *m_fi = nullptr;
    qbs::CleanJob *m_job = nullptr;
    int m_progressBase;
    bool m_showCompilerOutput = true;
    ProjectExplorer::IOutputParser *m_parser = nullptr;

    friend class QbsCleanStepConfigWidget;
};

namespace Ui { class QbsCleanStepConfigWidget; }

class QbsCleanStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    QbsCleanStepConfigWidget(QbsCleanStep *step);
    ~QbsCleanStepConfigWidget();
    QString summaryText() const;
    QString displayName() const;

private:
    void updateState();

    void changeDryRun(bool dr);
    void changeKeepGoing(bool kg);
    void changeJobCount(int jobcount);

    Ui::QbsCleanStepConfigWidget *m_ui;

    QbsCleanStep *m_step;
    QString m_summary;
};

class QbsCleanStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    QbsCleanStepFactory();
};

} // namespace Internal
} // namespace QbsProjectManager
