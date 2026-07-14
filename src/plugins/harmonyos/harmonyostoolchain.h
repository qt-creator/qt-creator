// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/gcctoolchain.h>

namespace HarmonyOs::Internal {

using ToolchainList = QList<ProjectExplorer::Toolchain *>;

class HarmonyOsToolchain : public ProjectExplorer::GccToolchain
{
public:
    HarmonyOsToolchain();

    bool isValid() const override;
    void addToEnvironment(Utils::Environment &env) const override;
    QStringList suggestedMkspecList() const override;
    void fromMap(const Utils::Store &data) override;

protected:
    DetectedAbisResult detectSupportedAbis() const override;
};

ToolchainList autodetectToolchains(const ToolchainList &alreadyKnown);

void setupHarmonyOsToolchain();

} // namespace HarmonyOs::Internal
