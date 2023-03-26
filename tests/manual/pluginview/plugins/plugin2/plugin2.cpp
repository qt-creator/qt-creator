// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "plugin2.h"

#include <extensionsystem/pluginmanager.h>

using namespace Plugin2;

MyPlugin2::~MyPlugin2()
{
    ExtensionSystem::PluginManager::removeObject(object1);
    ExtensionSystem::PluginManager::removeObject(object2);
}

bool MyPlugin2::initialize(const QStringList & /*arguments*/, QString *errorString)
{
    Q_UNUSED(errorString)
    initializeCalled = true;
    object1 = new QObject(this);
    object1->setObjectName("MyPlugin2");
    ExtensionSystem::PluginManager::addObject(object1);

    return true;
}

void MyPlugin2::extensionsInitialized()
{
    if (!initializeCalled)
        return;
    // don't do this at home, it's just done here for the test
    object2 = new QObject(this);
    object2->setObjectName("MyPlugin2_running");
    ExtensionSystem::PluginManager::addObject(object2);
}
