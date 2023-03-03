// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

class Operation;

class Settings
{
public:
    Settings();
    static Settings *instance();

    QString getPath(const QString &file);

    QString sdkPath;
    Operation *operation = nullptr;
};
