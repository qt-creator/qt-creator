// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#pragma once

#include "projectexplorer/kit.h"
#include "projectexplorer/project.h"
#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildstep.h>
#include <utils/id.h>

#include <QTemporaryDir>

namespace QmlProjectManager {

class DeployMcuProcessStep : public ProjectExplorer::AbstractProcessStep
{
public:
    static const Utils::Id id;
    static void showError(const QString& text);

    DeployMcuProcessStep(ProjectExplorer::BuildStepList *bc, Utils::Id id);

private:
    bool init() override;
    void doRun() override;
    QString findKitInformation(ProjectExplorer::Kit* kit, const QString& key);

    static const QString processCommandKey;
    static const QString processArgumentsKey;
    static const QString processWorkingDirectoryKey;
    QTemporaryDir m_tmpDir;
};

class MCUBuildStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    MCUBuildStepFactory();
    static void attachToTarget(ProjectExplorer::Target *target);
    static ProjectExplorer::Kit* findMostRecentQulKit( );
};

} // namespace QmlProjectManager
