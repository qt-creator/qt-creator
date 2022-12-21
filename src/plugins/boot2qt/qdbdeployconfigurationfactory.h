// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/deployconfiguration.h>

namespace Qdb {
namespace Internal {

class QdbDeployConfigurationFactory final : public ProjectExplorer::DeployConfigurationFactory
{
public:
    QdbDeployConfigurationFactory();
};

} // namespace Internal
} // namespace Qdb
