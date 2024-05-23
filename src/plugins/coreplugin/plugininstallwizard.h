// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QCoreApplication>

namespace Core {
namespace Internal {

class PluginInstallWizard
{
public:
    static bool exec(const Utils::FilePath &archive = {});
};

} // namespace Internal
} // namespace Core
