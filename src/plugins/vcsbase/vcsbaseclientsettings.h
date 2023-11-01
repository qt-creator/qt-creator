// Copyright (C) 2016 Brian McGillion and Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbase_global.h"

#include <utils/aspects.h>

namespace VcsBase {

class VCSBASE_EXPORT VcsBaseSettings : public Utils::AspectContainer
{
public:
    VcsBaseSettings();
    ~VcsBaseSettings();

    Utils::FilePathAspect binaryPath{this};
    Utils::StringAspect userName{this};
    Utils::StringAspect userEmail{this};
    Utils::IntegerAspect logCount{this};
    Utils::IntegerAspect timeout{this}; // Seconds
    Utils::StringAspect path{this};

    Utils::FilePaths searchPathList() const;
};

} // namespace VcsBase
