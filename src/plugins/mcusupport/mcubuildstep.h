// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#pragma once

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>

#include <utils/id.h>

#include <QTemporaryDir>

namespace McuSupport::Internal {

class DeployMcuProcessStep : public ProjectExplorer::AbstractProcessStep
{
public:
    static const Utils::Id id;
    static void showError(const QString &text);

    DeployMcuProcessStep(ProjectExplorer::BuildStepList *bc, Utils::Id id);

private:
    QString findKitInformation(ProjectExplorer::Kit *kit, const QString &key);
    QTemporaryDir m_tmpDir;
};

class MCUBuildStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    MCUBuildStepFactory();
    static ProjectExplorer::Kit *findMostRecentQulKit();
    static void updateDeployStep(ProjectExplorer::Target *target, bool enabled);
};

} // namespace McuSupport::Internal
