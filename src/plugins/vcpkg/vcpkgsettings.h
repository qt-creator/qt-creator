// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace ProjectExplorer {
class Project;
}

namespace Vcpkg::Internal {

class VcpkgSettings : public Utils::AspectContainer
{
public:
    VcpkgSettings(ProjectExplorer::Project *project, bool autoApply);

    void readSettings() final;
    void writeSettings() const final;


    void setVcpkgRootEnvironmentVariable();

    Utils::FilePathAspect vcpkgRoot{this};

    bool useGlobalSettings{true};

private:
    ProjectExplorer::Project *m_project{nullptr};
};

VcpkgSettings *settings(ProjectExplorer::Project *project);

} // Vcpkg::Internal
