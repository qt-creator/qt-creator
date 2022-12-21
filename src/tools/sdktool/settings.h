// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/fileutils.h>

class Operation;

class Settings
{
public:
    Settings();
    static Settings *instance();

    Utils::FilePath getPath(const QString &file);

    Utils::FilePath sdkPath;
    Operation *operation = nullptr;
};
