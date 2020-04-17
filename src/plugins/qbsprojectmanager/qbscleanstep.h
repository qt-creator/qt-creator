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
#include <projectexplorer/projectconfigurationaspects.h>
#include <projectexplorer/task.h>

namespace QbsProjectManager {
namespace Internal {
class ErrorInfo;
class QbsSession;

class QbsCleanStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    QbsCleanStep(ProjectExplorer::BuildStepList *bsl, Core::Id id);
    ~QbsCleanStep() override;

    QbsBuildStepData stepData() const;

    void dropSession();

private:
    bool init() override;
    void doRun() override;
    void doCancel() override;

    void cleaningDone(const ErrorInfo &error);
    void handleTaskStarted(const QString &desciption, int max);
    void handleProgress(int value);

    void createTaskAndOutput(ProjectExplorer::Task::TaskType type,
                             const QString &message, const QString &file, int line);

    ProjectExplorer::BaseBoolAspect *m_dryRunAspect = nullptr;
    ProjectExplorer::BaseBoolAspect *m_keepGoingAspect = nullptr;

    QStringList m_products;
    QbsSession *m_session = nullptr;
    QString m_description;
    int m_maxProgress;
    bool m_showCompilerOutput = true;
};

class QbsCleanStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    QbsCleanStepFactory();
};

} // namespace Internal
} // namespace QbsProjectManager
