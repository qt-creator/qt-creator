// Copyright (C) 2019 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/abi.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainconfigwidget.h>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QPushButton;
class QTextEdit;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace ProjectExplorer { class AbiWidget; }

namespace BareMetal::Internal {

// SdccToolChain

class SdccToolChain final : public ProjectExplorer::ToolChain
{
public:
    MacroInspectionRunner createMacroInspectionRunner() const final;

    Utils::LanguageExtensions languageExtensions(const QStringList &cxxflags) const final;
    Utils::WarningFlags warningFlags(const QStringList &cxxflags) const final;

    BuiltInHeaderPathsRunner createBuiltInHeaderPathsRunner(
            const Utils::Environment &) const final;
    void addToEnvironment(Utils::Environment &env) const final;
    QList<Utils::OutputLineParser *> createOutputParsers() const final;

    std::unique_ptr<ProjectExplorer::ToolChainConfigWidget> createConfigurationWidget() final;

    bool operator ==(const ToolChain &other) const final;

    Utils::FilePath makeCommand(const Utils::Environment &env) const final;

private:
    SdccToolChain();

    friend class SdccToolChainFactory;
    friend class SdccToolChainConfigWidget;
};

// SdccToolChainFactory

class SdccToolChainFactory final : public ProjectExplorer::ToolChainFactory
{
public:
    SdccToolChainFactory();

    ProjectExplorer::Toolchains autoDetect(
            const ProjectExplorer::ToolchainDetector &detector) const final;

private:
    ProjectExplorer::Toolchains autoDetectToolchains(const Candidates &candidates,
            const ProjectExplorer::Toolchains &alreadyKnown) const;
    ProjectExplorer::Toolchains autoDetectToolchain(
            const Candidate &candidate, Utils::Id language) const;
};

// SdccToolChainConfigWidget

class SdccToolChainConfigWidget final : public ProjectExplorer::ToolChainConfigWidget
{
public:
    explicit SdccToolChainConfigWidget(SdccToolChain *tc);

private:
    void applyImpl() final;
    void discardImpl() final { setFromToolchain(); }
    bool isDirtyImpl() const final;
    void makeReadOnlyImpl() final;

    void setFromToolchain();
    void handleCompilerCommandChange();

    Utils::PathChooser *m_compilerCommand = nullptr;
    ProjectExplorer::AbiWidget *m_abiWidget = nullptr;
    ProjectExplorer::Macros m_macros;
};

} // BareMetal::Internal
