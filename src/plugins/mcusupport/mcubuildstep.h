// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildstep.h>

#include <utils/aspects.h>

#include <QTemporaryDir>

namespace McuSupport::Internal {
class DeployMcuProcessStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

public:
    DeployMcuProcessStep(ProjectExplorer::BuildStepList *bc, Utils::Id id);

    static const Utils::Id id;
    static void showError(const QString &text);

    void updateIncludeDirArgs();

private:
    QString findKitInformation(ProjectExplorer::Kit *kit, const QString &key);

    QTemporaryDir m_tmpDir;
    QStringList arguments;
    Utils::FilePathAspect cmd{this};
    Utils::StringAspect args{this};
    Utils::FilePathAspect outDir{this};
};

class MCUBuildStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    MCUBuildStepFactory();

    static ProjectExplorer::Kit *findMostRecentQulKit();
    static void updateDeployStep(ProjectExplorer::BuildConfiguration *bc, bool enabled);
};

} // namespace McuSupport::Internal
