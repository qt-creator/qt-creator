// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QCoreApplication>

namespace Core {
namespace Internal {

class PluginInstallWizard
{
    Q_DECLARE_TR_FUNCTIONS(Core::Internal::PluginInstallWizard)

public:
    static bool exec();
};

} // namespace Internal
} // namespace Core
