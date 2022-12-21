// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/gcctoolchain.h>

#include <QVersionNumber>

namespace WebAssembly {
namespace Internal {

class WebAssemblyToolChain final : public ProjectExplorer::GccToolChain
{
public:
    WebAssemblyToolChain();

    void addToEnvironment(Utils::Environment &env) const override;

    Utils::FilePath makeCommand(const Utils::Environment &environment) const override;
    bool isValid() const override;

    static const QVersionNumber &minimumSupportedEmSdkVersion();
    static void registerToolChains();
    static bool areToolChainsRegistered();
};

class WebAssemblyToolChainFactory : public ProjectExplorer::ToolChainFactory
{
public:
    WebAssemblyToolChainFactory();

    ProjectExplorer::Toolchains autoDetect(
        const ProjectExplorer::ToolchainDetector &detector) const final;
};

} // namespace Internal
} // namespace WebAssembly
