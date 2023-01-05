// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace Android::Internal {

class AndroidPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Android.json")

    ~AndroidPlugin() final;

    void initialize() final;

    void kitsRestored();
    void askUserAboutAndroidSetup();

    class AndroidPluginPrivate *d = nullptr;

#ifdef WITH_TESTS
private slots:
   void testAndroidSdkManagerProgressParser_data();
   void testAndroidSdkManagerProgressParser();
   void testAndroidConfigAvailableNdkPlatforms_data();
   void testAndroidConfigAvailableNdkPlatforms();
#endif // WITH_TESTS
};

} // Android::Internal
