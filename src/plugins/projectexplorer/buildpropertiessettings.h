// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/aspects.h>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT BuildPropertiesSettings : public Utils::AspectContainer
{
public:
    BuildPropertiesSettings();

    class BuildTriStateAspect : public Utils::TriStateAspect
    {
    public:
        explicit BuildTriStateAspect(AspectContainer *container);
    };

    Utils::StringAspect buildDirectoryTemplate{this};
    BuildTriStateAspect separateDebugInfo{this};
    BuildTriStateAspect qmlDebugging{this};
    BuildTriStateAspect qtQuickCompiler{this};

    static void showQtSettings(); // Called by the Qt support plugin
};

PROJECTEXPLORER_EXPORT BuildPropertiesSettings &buildPropertiesSettings();

} // namespace ProjectExplorer
