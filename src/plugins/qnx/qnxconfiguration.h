// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qnxconstants.h"
#include "qnxversionnumber.h"

#include <utils/fileutils.h>
#include <utils/environment.h>

#include <projectexplorer/abi.h>

#include <debugger/debuggeritemmanager.h>

#include <QVariant>

namespace ProjectExplorer
{
    class Kit;
    class ToolChain;
}

namespace Qnx::Internal {

class QnxToolChain;
class QnxQtVersion;

class QnxConfiguration
{
public:
    QnxConfiguration();
    QnxConfiguration(const Utils::FilePath &sdpEnvFile);
    QnxConfiguration(const QVariantMap &data);

    Utils::FilePath envFile() const;
    Utils::FilePath qnxTarget() const;
    Utils::FilePath qnxHost() const;
    Utils::FilePath qccCompilerPath() const;
    Utils::EnvironmentItems qnxEnv() const;
    QnxVersionNumber version() const;
    QVariantMap toMap() const;

    bool isValid() const;

    QString displayName() const;
    bool activate();
    void deactivate();
    bool isActive() const;
    Utils::FilePath sdpPath() const;

    QList<ProjectExplorer::ToolChain *> autoDetect(
            const QList<ProjectExplorer::ToolChain *> &alreadyKnown);

private:
    QList<ProjectExplorer::ToolChain *> findToolChain(
            const QList<ProjectExplorer::ToolChain *> &alreadyKnown,
            const ProjectExplorer::Abi &abi);

    QString validationErrorMessage() const;

    void setVersion(const QnxVersionNumber& version);

    void readInformation();

    void setDefaultConfiguration(const Utils::FilePath &envScript);

    Utils::EnvironmentItems qnxEnvironmentItems() const;

    QString m_configName;

    Utils::FilePath m_envFile;
    Utils::FilePath m_qnxConfiguration;
    Utils::FilePath m_qnxTarget;
    Utils::FilePath m_qnxHost;
    Utils::FilePath m_qccCompiler;
    Utils::EnvironmentItems m_qnxEnv;
    QnxVersionNumber m_version;

    class Target
    {
    public:
        Target(const ProjectExplorer::Abi &abi, const Utils::FilePath &path)
            : m_abi(abi), m_path(path)
        {
        }

        QString shortDescription() const;
        QString cpuDir() const;

        ProjectExplorer::Abi m_abi;
        Utils::FilePath m_path;
        Utils::FilePath m_debuggerPath;
    };

    QList<Target> m_targets;

    QnxQtVersion *qnxQtVersion(const Target &target) const;

    void createTools(const Target &target);
    QVariant createDebugger(const Target &target);

    using QnxToolChainMap = std::map<const char*, QnxToolChain*>;

    QnxToolChainMap createToolChain(const Target &target);
    void createKit(const Target &target, const QnxToolChainMap &toolChain, const QVariant &debugger);

    const Target *findTargetByDebuggerPath(const Utils::FilePath &path) const;

    void updateTargets();
    void assignDebuggersToTargets();
};

} // Qnx::Internal
