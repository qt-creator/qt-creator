/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
**
***************************************************************************/

#include "plugin3.h"

#include <extensionsystem/pluginmanager.h>

#include <QtCore/qplugin.h>
#include <QtCore/QObject>

using namespace Plugin3;

MyPlugin3::MyPlugin3()
    : initializeCalled(false)
{
}

bool MyPlugin3::initialize(const QStringList & /*arguments*/, QString *errorString)
{
    initializeCalled = true;
    QObject *obj = new QObject(this);
    obj->setObjectName("MyPlugin3");
    addAutoReleasedObject(obj);

    bool found2 = false;
    foreach (QObject *object, ExtensionSystem::PluginManager::instance()->allObjects()) {
        if (object->objectName() == "MyPlugin2")
            found2 = true;
    }
    if (found2)
        return true;
    if (errorString)
        *errorString = "object from plugin2 could not be found";
    return false;
}

void MyPlugin3::extensionsInitialized()
{
    if (!initializeCalled)
        return;
    // don't do this at home, it's just done here for the test
    QObject *obj = new QObject(this);
    obj->setObjectName("MyPlugin3_running");
    addAutoReleasedObject(obj);
}

Q_EXPORT_PLUGIN(MyPlugin3)
