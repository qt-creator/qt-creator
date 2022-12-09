// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/abstractprocessstep.h>

namespace CMakeProjectManager::Internal {

class CMakeAbstractProcessStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
public:
    CMakeAbstractProcessStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);

protected:
    bool init() override;
};


} // namespace CMakeProjectManager::Internal
