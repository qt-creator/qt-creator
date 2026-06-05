// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace ProjectExplorer { class Project; }

namespace Vcpkg::Internal {

class VcpkgSettings : public Utils::AspectContainer
{
public:
    VcpkgSettings();

    void setVcpkgRootEnvironmentVariable();

    Utils::FilePathAspect vcpkgRoot{this};
};

VcpkgSettings *settings(ProjectExplorer::Project *project = nullptr);

} // Vcpkg::Internal
