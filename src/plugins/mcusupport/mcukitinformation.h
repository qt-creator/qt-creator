// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/kitaspects.h>

namespace McuSupport::Internal {

class McuDependenciesKitAspect final
{
public:
    static Utils::Id id();
    static Utils::NameValueItems dependencies(const ProjectExplorer::Kit *kit);
    static void setDependencies(ProjectExplorer::Kit *kit, const Utils::NameValueItems &dependencies);
    static Utils::NameValuePairs configuration(const ProjectExplorer::Kit *kit);
};

} // McuSupport::Internal
