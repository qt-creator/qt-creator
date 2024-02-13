// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/runconfigurationaspects.h>

namespace ProjectExplorer { class Kit; }

namespace Python {

class PythonKitAspect
{
public:
    static std::optional<ProjectExplorer::Interpreter> python(const ProjectExplorer::Kit *kit);
    static void setPython(ProjectExplorer::Kit *k, const QString &interpreterId);
    static Utils::Id id();
};

}; // namespace Python
