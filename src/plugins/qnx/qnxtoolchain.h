// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/toolchainconfigwidget.h>

namespace ProjectExplorer { class AbiWidget; }
namespace Utils { class PathChooser; }

namespace Qnx::Internal {

class QnxToolChain : public ProjectExplorer::GccToolChain
{
public:
    QnxToolChain();

    std::unique_ptr<ProjectExplorer::ToolChainConfigWidget> createConfigurationWidget() override;

    void addToEnvironment(Utils::Environment &env) const override;
    QStringList suggestedMkspecList() const override;

    QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &data) override;

    Utils::FilePath sdpPath() const;
    void setSdpPath(const Utils::FilePath &sdpPath);
    QString cpuDir() const;
    void setCpuDir(const QString &cpuDir);

    bool operator ==(const ToolChain &) const override;

protected:
    DetectedAbisResult detectSupportedAbis() const override;

private:
    Utils::FilePath m_sdpPath;
    QString m_cpuDir;
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
