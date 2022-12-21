// Copyright (C) 2016 Brian McGillion and Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbase_global.h"

#include <utils/aspects.h>

namespace VcsBase {

class VCSBASE_EXPORT VcsBaseSettings : public Utils::AspectContainer
{
    Q_DECLARE_TR_FUNCTIONS(VcsBase::VcsBaseSettings)

public:
    VcsBaseSettings();
    ~VcsBaseSettings();

    Utils::StringAspect binaryPath;
    Utils::StringAspect userName;
    Utils::StringAspect userEmail;
    Utils::IntegerAspect logCount;
    Utils::IntegerAspect timeout; // Seconds
    Utils::StringAspect path;

    Utils::FilePaths searchPathList() const;

private:
    QString m_settingsGroup;
};

} // namespace VcsBase
