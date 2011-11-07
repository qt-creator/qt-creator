/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmljsinspectorplugin.h"

#include "qmljsclientproxy.h"
#include "qmljsinspector.h"
#include "qmljsinspectorconstants.h"
#include "qmljsinspectortoolbar.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/modemanager.h>
#include <debugger/debuggerconstants.h>
#include <debugger/qml/qmladapter.h>
#include <extensionsystem/pluginmanager.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <qmlprojectmanager/qmlproject.h>
#include <utils/qtcassert.h>

#include <QtCore/QStringList>
#include <QtCore/QtPlugin>
#include <QtCore/QTimer>

#include <QtGui/QHBoxLayout>
#include <QtGui/QToolButton>

#include <QtCore/QDebug>

using namespace QmlJSInspector::Internal;
using namespace QmlJSInspector::Constants;

InspectorPlugin::InspectorPlugin()
    : IPlugin()
    , m_clientProxy(0)
{
    m_inspectorUi = new InspectorUi(this);
}

InspectorPlugin::~InspectorPlugin()
{
}

QmlJS::ModelManagerInterface *InspectorPlugin::modelManager() const
{
    return ExtensionSystem::PluginManager::instance()->getObject<QmlJS::ModelManagerInterface>();
}

InspectorUi *InspectorPlugin::inspector() const
{
    return m_inspectorUi;
}

ExtensionSystem::IPlugin::ShutdownFlag InspectorPlugin::aboutToShutdown()
{
    m_inspectorUi->saveSettings();
    return SynchronousShutdown;
}

bool InspectorPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorString);

    return true;
}

void InspectorPlugin::extensionsInitialized()
{
    ExtensionSystem::PluginManager *pluginManager = ExtensionSystem::PluginManager::instance();

    connect(pluginManager, SIGNAL(objectAdded(QObject*)), SLOT(objectAdded(QObject*)));
    connect(Core::ModeManager::instance(), SIGNAL(currentModeAboutToChange(Core::IMode*)),
            this, SLOT(modeAboutToChange(Core::IMode*)));
}

void InspectorPlugin::objectAdded(QObject *object)
{
    Debugger::QmlAdapter *adapter = qobject_cast<Debugger::QmlAdapter *>(object);
    if (adapter) {
        //Disconnect inspector plugin when qml adapter emits disconnected
        connect(adapter, SIGNAL(disconnected()), this, SLOT(disconnect()));
        m_clientProxy = new ClientProxy(adapter);
        if (m_clientProxy->isConnected()) {
            clientProxyConnected();
        } else {
            connect(m_clientProxy, SIGNAL(connected()), this, SLOT(clientProxyConnected()));
        }
        return;
    }

    if (object->objectName() == QLatin1String("QmlEngine"))
        m_inspectorUi->setDebuggerEngine(object);
}

void InspectorPlugin::disconnect()
{
    if (m_inspectorUi->isConnected()) {
        m_inspectorUi->disconnected();
        delete m_clientProxy;
        m_clientProxy = 0;
    }
}

void InspectorPlugin::clientProxyConnected()
{
    m_inspectorUi->connected(m_clientProxy);
}

void InspectorPlugin::modeAboutToChange(Core::IMode *newMode)
{
    QTC_ASSERT(newMode, return);

    if (newMode->id() == Debugger::Constants::MODE_DEBUG) {
        m_inspectorUi->setupUi();

        // Make sure we're not called again.
        QObject::disconnect(Core::ModeManager::instance(), SIGNAL(currentModeAboutToChange(Core::IMode*)),
                   this, SLOT(modeAboutToChange(Core::IMode*)));
    }
}

Q_EXPORT_PLUGIN(InspectorPlugin)
