// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qbsbuildconfiguration.h"
#include "qbssession.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/task.h>

#include <utils/aspects.h>

namespace QbsProjectManager {
namespace Internal {

class ErrorInfo;
class QbsSession;

class QbsInstallStep final : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    QbsInstallStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);
    ~QbsInstallStep() override;

    QString installRoot() const;
    QbsBuildStepData stepData() const;

private:
    bool init() override;
    void doRun() override;
    void doCancel() override;
    QWidget *createConfigWidget() override;

    const QbsBuildConfiguration *buildConfig() const;
    void installDone(const ErrorInfo &error);
    void handleTaskStarted(const QString &desciption, int max);
    void handleProgress(int value);

    void createTaskAndOutput(ProjectExplorer::Task::TaskType type,
                             const QString &message, const Utils::FilePath &file, int line);

    Utils::BoolAspect *m_cleanInstallRoot = nullptr;
    Utils::BoolAspect *m_dryRun = nullptr;
    Utils::BoolAspect *m_keepGoing = nullptr;

    QbsSession *m_session = nullptr;
    QString m_description;
    int m_maxProgress;

    friend class QbsInstallStepConfigWidget;
};

class QbsInstallStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    QbsInstallStepFactory();
};

} // namespace Internal
} // namespace QbsProjectManager
