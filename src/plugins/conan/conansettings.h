// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>
#include <utils/fileutils.h>

#include <QSettings>

namespace ConanPackageManager {
namespace Internal {

class ConanSettings : public Utils::AspectContainer
{
public:
    ConanSettings();

    Utils::StringAspect conanFilePath;
};

} // Internal
} // ConanPackageManager
