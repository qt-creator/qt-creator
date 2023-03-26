// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "plugin3.h"

#include <extensionsystem/pluginmanager.h>

using namespace Plugin3;

MyPlugin3::~MyPlugin3()
{
    ExtensionSystem::PluginManager::removeObject(object1);
    ExtensionSystem::PluginManager::removeObject(object2);
}

bool MyPlugin3::initialize(const QStringList & /*arguments*/, QString *errorString)
{
    initializeCalled = true;
    object1 = new QObject(this);
    object1->setObjectName(QLatin1String("MyPlugin3"));
    ExtensionSystem::PluginManager::addObject(object1);

    bool found2 = false;
    for (QObject *object : ExtensionSystem::PluginManager::allObjects()) {
        if (object->objectName() == QLatin1String("MyPlugin2"))
            found2 = true;
    }
    if (found2)
        return true;
    if (errorString)
        *errorString = QLatin1String("object from plugin2 could not be found");
    return false;
}

void MyPlugin3::extensionsInitialized()
{
    if (!initializeCalled)
        return;
    // don't do this at home, it's just done here for the test
    object2 = new QObject(this);
    object2->setObjectName(QLatin1String("MyPlugin3_running"));
    ExtensionSystem::PluginManager::addObject(object2);
}
