// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../buildstep.h"
#include "../projectexplorer_export.h"

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT DeviceCheckBuildStep : public BuildStep
{
    Q_OBJECT

public:
    DeviceCheckBuildStep(BuildStepList *bsl, Utils::Id id);

    bool init() override;
    void doRun() override;

    static Utils::Id stepId();
    static QString displayName();
};

} // namespace ProjectExplorer
