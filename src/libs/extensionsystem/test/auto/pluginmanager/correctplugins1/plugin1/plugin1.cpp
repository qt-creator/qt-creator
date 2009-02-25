/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "plugin1.h"

#include <extensionsystem/pluginmanager.h>

#include <QtCore/qplugin.h>
#include <QtCore/QObject>

using namespace Plugin1;

MyPlugin1::MyPlugin1()
    : initializeCalled(false)
{
}

bool MyPlugin1::initialize(const QStringList & /*arguments*/, QString *errorString)
{
    initializeCalled = true;
    QObject *obj = new QObject;
    obj->setObjectName("MyPlugin1");
    addAutoReleasedObject(obj);

    bool found2 = false;
    bool found3 = false;
    foreach (QObject *object, ExtensionSystem::PluginManager::instance()->allObjects()) {
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
    QObject *obj = new QObject;
    obj->setObjectName("MyPlugin1_running");
    addAutoReleasedObject(obj);
}

Q_EXPORT_PLUGIN(MyPlugin1)

