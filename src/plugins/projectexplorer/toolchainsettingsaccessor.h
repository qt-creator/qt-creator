// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/settingsaccessor.h>

#include <QList>

namespace ProjectExplorer {

class Toolchain;

namespace Internal {

class ToolChainSettingsAccessor : public Utils::UpgradingSettingsAccessor
{
public:
    ToolChainSettingsAccessor();

    QList<Toolchain *> restoreToolChains(QWidget *parent) const;

    void saveToolChains(const QList<Toolchain *> &toolchains, QWidget *parent);

private:
    QList<Toolchain *> toolChains(const Utils::Store &data) const;
};

} // namespace Internal
} // namespace ProjectExplorer
