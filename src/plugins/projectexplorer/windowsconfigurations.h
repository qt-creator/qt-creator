// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace ProjectExplorer::Internal {

class WindowsConfigurations : public Utils::AspectContainer
{
    Q_OBJECT

    WindowsConfigurations();
    friend WindowsConfigurations &windowsConfigurations();

public:
    Utils::FilePathAspect downloadLocation{this};
    Utils::FilePathAspect nugetLocation{this};
    Utils::FilePathAspect windowsAppSdkLocation{this};

    void applyConfig();

signals:
    void aboutToUpdate();
    void updated();
};

WindowsConfigurations &windowsConfigurations();

void setupWindowsConfigurations();

} // namespace ProjectExplorer::Internal
