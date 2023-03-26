// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/abstractprocessstep.h>

namespace Haskell {
namespace Internal {

class StackBuildStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

public:
    StackBuildStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);

    QWidget *createConfigWidget() override;

    static QString trDisplayName();

protected:
    bool init() override;
};

class StackBuildStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    StackBuildStepFactory();
};

} // namespace Internal
} // namespace Haskell
