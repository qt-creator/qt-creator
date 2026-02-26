// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ninjaparser.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildstep.h>

namespace GNProjectManager::Internal {

class GNBuildStep final : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
public:
    GNBuildStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);

    QWidget *createConfigWidget() final;
    Utils::CommandLine command();
    QStringList projectTargets();
    void setBuildTarget(const QString &targetName);
    void setCommandArgs(const QString &args);
    const QString &targetName() const { return m_targetName; }
    Q_SIGNAL void targetListChanged();
    Q_SIGNAL void commandChanged();

    void toMap(Utils::Store &map) const override;
    void fromMap(const Utils::Store &map) override;

private:
    void update(bool parsingSuccessful);
    void setupOutputFormatter(Utils::OutputFormatter *formatter) override;
    QString defaultBuildTarget() const;

    QString m_commandArgs;
    QString m_targetName;
    NinjaParser *m_ninjaParser = nullptr;
};

void setupGNBuildStep();
} // namespace GNProjectManager::Internal
