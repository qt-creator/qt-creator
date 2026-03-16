// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "toolchain.h"

#include <coreplugin/dialogs/ioptionspage.h>

namespace ProjectExplorer::Internal {

class ToolChainOptionsPage final : public Core::IOptionsPage
{
public:
    ToolChainOptionsPage();
};

QVariant toolchainBundleData(const std::optional<ToolchainBundle> &bundle, int column, int role);

} // namespace ProjectExplorer::Internal
