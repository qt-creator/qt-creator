// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qbsbuildconfiguration.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/task.h>

#include <utils/aspects.h>

namespace QbsProjectManager {
namespace Internal {

class ErrorInfo;
class QbsSession;

class QbsCleanStep final : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    QbsCleanStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);
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

    Utils::BoolAspect *m_dryRunAspect = nullptr;
    Utils::BoolAspect *m_keepGoingAspect = nullptr;

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
