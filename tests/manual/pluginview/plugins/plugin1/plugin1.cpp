// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "plugin1.h"

#include <extensionsystem/pluginmanager.h>

using namespace Plugin1;

MyPlugin1::~MyPlugin1()
{
    ExtensionSystem::PluginManager::removeObject(object1);
    ExtensionSystem::PluginManager::removeObject(object2);
}

bool MyPlugin1::initialize(const QStringList & /*arguments*/, QString *errorString)
{
    initializeCalled = true;
    object1 = new QObject(this);
    object1->setObjectName("MyPlugin1");
    ExtensionSystem::PluginManager::addObject(object1);

    bool found2 = false;
    bool found3 = false;
    const QList<QObject *> objects = ExtensionSystem::PluginManager::instance()->allObjects();
    for (QObject *object : objects) {
        if (object->objectName() == "MyPlugin2")
            found2 = true;
        else if (object->objectName() == "MyPlugin3")
            found3 = true;
    }
    if (found2 && found3)
        return true;
    if (errorString) {
        QString error = "object(s) missing from plugin(s):";
        if (!found2)
            error.append(" plugin2");
        if (!found3)
            error.append(" plugin3");
        *errorString = error;
    }
    return false;
}

void MyPlugin1::extensionsInitialized()
{
    if (!initializeCalled)
        return;
    // don't do this at home, it's just done here for the test
    object2 = new QObject(this);
    object2->setObjectName("MyPlugin1_running");
    ExtensionSystem::PluginManager::addObject(object2);
}
