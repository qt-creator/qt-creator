// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/gcctoolchain.h>

namespace Qnx::Internal {

class QnxToolChain : public ProjectExplorer::GccToolChain
{
public:
    QnxToolChain();

    std::unique_ptr<ProjectExplorer::ToolChainConfigWidget> createConfigurationWidget() override;

    void addToEnvironment(Utils::Environment &env) const override;
    QStringList suggestedMkspecList() const override;

    Utils::FilePathAspect sdpPath{this};
    Utils::StringAspect cpuDir{this};

    bool operator ==(const ToolChain &) const override;

protected:
    DetectedAbisResult detectSupportedAbis() const override;
};

// --------------------------------------------------------------------------
// QnxToolChainFactory
// --------------------------------------------------------------------------

class QnxToolChainFactory : public ProjectExplorer::ToolChainFactory
{
public:
    QnxToolChainFactory();

    ProjectExplorer::Toolchains autoDetect(
            const ProjectExplorer::ToolchainDetector &detector) const final;
};

} // Qnx::Internal
