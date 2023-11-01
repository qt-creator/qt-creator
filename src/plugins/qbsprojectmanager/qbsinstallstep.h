// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildstep.h>

namespace QbsProjectManager::Internal {

class QbsBuildConfiguration;
class QbsBuildStepData;

class QbsInstallStep final : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    QbsInstallStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);

    Utils::FilePath installRoot() const;

private:
    bool init() override;
    Tasking::GroupItem runRecipe() final;
    QWidget *createConfigWidget() override;

    const QbsBuildConfiguration *buildConfig() const;

    Utils::BoolAspect cleanInstallRoot{this};
    Utils::BoolAspect dryRun{this};
    Utils::BoolAspect keepGoing{this};

    friend class QbsInstallStepConfigWidget;
};

class QbsInstallStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    QbsInstallStepFactory();
};

} // namespace QbsProjectManager::Internal
