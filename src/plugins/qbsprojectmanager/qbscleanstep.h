// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildstep.h>

namespace QbsProjectManager::Internal {

class QbsCleanStep final : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    QbsCleanStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);

private:
    bool init() override;
    Tasking::GroupItem runRecipe() final;

    Utils::BoolAspect dryRun{this};
    Utils::BoolAspect keepGoing{this};
    Utils::StringAspect effectiveCommand{this};

    QStringList m_products;
};

class QbsCleanStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    QbsCleanStepFactory();
};

} // namespace QbsProjectManager::Internal
