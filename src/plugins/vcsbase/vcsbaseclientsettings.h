// Copyright (C) 2016 Brian McGillion and Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbase_global.h"

#include <coreplugin/dialogs/ioptionspage.h>

namespace VcsBase {

class VCSBASE_EXPORT VcsBaseSettings : public Core::PagedSettings
{
public:
    VcsBaseSettings();
    ~VcsBaseSettings();

    Utils::FilePathAspect binaryPath;
    Utils::StringAspect userName;
    Utils::StringAspect userEmail;
    Utils::IntegerAspect logCount;
    Utils::IntegerAspect timeout; // Seconds
    Utils::StringAspect path;

    Utils::FilePaths searchPathList() const;
};

} // namespace VcsBase
