// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <QCoreApplication>

namespace ProjectExplorer {
namespace Internal {

class ToolChainOptionsPage final : public Core::IOptionsPage
{
public:
    ToolChainOptionsPage();
};

} // namespace Internal
} // namespace ProjectExplorer
