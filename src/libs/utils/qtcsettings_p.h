// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qtcsettings.h"

class tst_PluginManager;
int main(int, char *[]);

namespace Utils::Internal {

class QTCREATOR_UTILS_EXPORT SettingsSetup final
{
    // Restrict access to items that need it.
    friend int ::main(int, char*[]);
    friend class ::tst_PluginManager;

    static void setupSettings(QtcSettings *userSettings, QtcSettings *installSettings);
    static void destroySettings();
};

} // Utils::Internal
