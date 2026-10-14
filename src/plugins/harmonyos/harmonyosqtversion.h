// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qtsupport/baseqtversion.h>

namespace HarmonyOs::Internal {

class HarmonyOsQtVersion : public QtSupport::QtVersion
{
public:
    HarmonyOsQtVersion();

    QString description() const override;
    ProjectExplorer::Abis detectQtAbis() const override;
    QSet<Utils::Id> targetDeviceTypes() const override;
    QSet<Utils::Id> availableFeatures() const override;
    void addToBuildEnvironment(const ProjectExplorer::Kit *k,
                               Utils::Environment &env) const override;
    Utils::FilePath qmakeFilePath() const override;
    Utils::CommandLine qmakeCommand() const override;
    QString mkspecFor(ProjectExplorer::Toolchain *tc) const override;
};

void setupHarmonyOsQtVersion();

} // namespace HarmonyOs::Internal
