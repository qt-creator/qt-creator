// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainconfigwidget.h>

namespace Utils { class PathChooser; }

namespace Nim {

class NimToolChain;

class NimToolChainFactory : public ProjectExplorer::ToolChainFactory
{
public:
    NimToolChainFactory();

    ProjectExplorer::Toolchains autoDetect(const ProjectExplorer::ToolchainDetector &detector) const final;
    ProjectExplorer::Toolchains detectForImport(const ProjectExplorer::ToolChainDescription &tcd) const final;
};

class NimToolChainConfigWidget : public ProjectExplorer::ToolChainConfigWidget
{
    Q_OBJECT

public:
    explicit NimToolChainConfigWidget(NimToolChain *tc);

protected:
    void applyImpl() final;
    void discardImpl() final;
    bool isDirtyImpl() const final;
    void makeReadOnlyImpl() final;

private:
    void fillUI();

    Utils::PathChooser *m_compilerCommand;
    QLineEdit *m_compilerVersion;
};

} // Nim
