// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ninjaparser.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildstep.h>

namespace MesonProjectManager {
namespace Internal {

class NinjaBuildStep final : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
public:
    NinjaBuildStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);

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

class MesonBuildStepFactory final : public ProjectExplorer::BuildStepFactory
{
public:
    MesonBuildStepFactory();
};

} // namespace Internal
} // namespace MesonProjectManager
