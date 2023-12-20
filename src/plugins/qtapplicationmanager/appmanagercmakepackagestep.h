// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildstep.h>

#include <QCoreApplication>

namespace AppManager {
namespace Internal {

class AppManagerCMakePackageStepFactory final : public ProjectExplorer::BuildStepFactory
{
    Q_DECLARE_TR_FUNCTIONS(Qtc::AppManager)

public:
    AppManagerCMakePackageStepFactory();
};

} // namespace Internal
} // namespace AppManager
