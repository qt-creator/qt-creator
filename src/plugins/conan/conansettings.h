// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace Conan::Internal {

class ConanSettings : public Utils::AspectContainer
{
public:
    ConanSettings();

    Utils::FilePathAspect conanFilePath{this};
};

ConanSettings &settings();

} // Conan::Internal
