// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runconfiguration.h>

namespace Qnx::Internal {

class QnxRunConfigurationFactory final : public ProjectExplorer::RunConfigurationFactory
{
public:
    QnxRunConfigurationFactory();
};

} // Qnx::Internal
