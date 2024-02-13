// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/settingsaccessor.h>

#include <QList>

namespace ProjectExplorer {

class Toolchain;

namespace Internal {

class ToolchainSettingsAccessor : public Utils::UpgradingSettingsAccessor
{
public:
    ToolchainSettingsAccessor();

    QList<Toolchain *> restoreToolchains(QWidget *parent) const;

    void saveToolchains(const QList<Toolchain *> &toolchains, QWidget *parent);

private:
    QList<Toolchain *> toolChains(const Utils::Store &data) const;
};

} // namespace Internal
} // namespace ProjectExplorer
