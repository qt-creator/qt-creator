// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/toolchain.h>
#include <projectexplorer/headerpath.h>

namespace Nim {

class NimToolchain : public ProjectExplorer::Toolchain
{
public:
    NimToolchain();
    explicit NimToolchain(Utils::Id typeId);

    MacroInspectionRunner createMacroInspectionRunner() const override;
    Utils::LanguageExtensions languageExtensions(const QStringList &flags) const final;
    Utils::WarningFlags warningFlags(const QStringList &flags) const final;

    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner(
            const Utils::Environment &) const override;
    void addToEnvironment(Utils::Environment &env) const final;
    Utils::FilePath makeCommand(const Utils::Environment &env) const final;
    QString compilerVersion() const;
    QList<Utils::OutputLineParser *> createOutputParsers() const final;

    void fromMap(const Utils::Store &data) final;

    static bool parseVersion(const Utils::FilePath &path, std::tuple<int, int, int> &version);

private:
    std::tuple<int, int, int> m_version;
};

class NimToolchainFactory : public ProjectExplorer::ToolchainFactory
{
public:
    NimToolchainFactory();

    ProjectExplorer::Toolchains autoDetect(const ProjectExplorer::ToolchainDetector &detector) const final;
    ProjectExplorer::Toolchains detectForImport(const ProjectExplorer::ToolchainDescription &tcd) const final;
    std::unique_ptr<ProjectExplorer::ToolchainConfigWidget> createConfigurationWidget(
        const ProjectExplorer::ToolchainBundle &bundle) const final;
};

} // Nim
