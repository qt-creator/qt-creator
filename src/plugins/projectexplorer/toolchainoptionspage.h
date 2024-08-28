// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "toolchain.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <utils/treemodel.h>

#include <QCoreApplication>

namespace ProjectExplorer {
namespace Internal {

class ToolChainOptionsPage final : public Core::IOptionsPage
{
public:
    ToolChainOptionsPage();
};

class ToolchainTreeItem : public Utils::TreeItem
{
public:
    ToolchainTreeItem(const ToolchainBundle &bundle) : bundle(bundle) {}
    ToolchainTreeItem() = default;

    static const int BundleIdRole = Qt::UserRole;
    QVariant data(int column, int role) const override;

    std::optional<ToolchainBundle> bundle;
};

} // namespace Internal
} // namespace ProjectExplorer
