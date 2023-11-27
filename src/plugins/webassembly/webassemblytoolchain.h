// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/gcctoolchain.h>

#include <QVersionNumber>

namespace WebAssembly::Internal {

class WebAssemblyToolChain final : public ProjectExplorer::GccToolchain
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

void setupWebAssemblyToolchain();

} // WebAssembly::Internal
